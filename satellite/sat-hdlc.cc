/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Daedalus Research
 *	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Contributed by the Daedalus Research Group, http://daedalus.cs.berkeley.edu
 */

#include <sat-hdlc.h>
#include <mac.h>

// setting this allows HDLC to reset once it reaches the max-retries.
#define RESET_HDLC 1

int HDLC::uidcnt_;
// int hdr_hdlc::offset_;

// static class HDLCHeaderClass : public PacketHeaderClass {
// public:
// 	HDLCHeaderClass()	: PacketHeaderClass("PacketHeader/HDLC",
// 					    sizeof(hdr_hdlc)) {
// 		bind_offset(&hdr_hdlc::offset_);
// 	}
// } class_hdr_hdlc;

static class HDLCClass : public TclClass {
public:
	HDLCClass() : TclClass("LL/Sat/HDLC") {}
	TclObject* create(int, const char*const*) {
		return (new HDLC);
	}
} class_hdlc;

void HdlcTimer::expire(Event *)
{
	(*agent_.*callback_)(a_);
	
}


HDLC::HDLC() : SatLL(), list_head_(0)
{
	bind("window_size_", &wnd_);
	bind("queue_size_", &queueSize_);
	bind_time("timeout_", &timeout_);
	bind("max_timeouts_", &maxTimeouts_);
	bind("selRepeat_", &selRepeat_);
	bind("delAck_", &delAck_);
	bind("delAckVal_", &delAckVal_);
	
	wndmask_ = HDLC_MWM;
}

void HDLC::recv(Packet* p, Handler* )
{
	hdr_cmn *ch = HDR_CMN(p);

	/*
	 * Sanity Check
	 */
	assert(initialized());

	// If direction = UP, then HDLC is recv'ing pkt from the network
	// Otherwise, set direction to DOWN and pass it down the stack
	if (ch->direction() == hdr_cmn::UP) {
		if (ch->ptype_ == PT_ARP)
			arptable_->arpinput(p, this);
		else 
			recvIncoming(p);
		return;
	}

	ch->direction() = hdr_cmn::DOWN;
	recvOutgoing(p);
}

// Functions for sending packets out to network

void HDLC::recvOutgoing(Packet* p)
{
	ARQstate *a;
	int next_hop = getRoute(p);
	
	//if (disconnect_) {
		
		// if (!sentDISC_) {
// 			sendUA(p, DISC);
// 			sentDISC_ = 1;
// 			set_rtx_timer();
// 			drop(p);
			
// 			return;
// 		}
// 		drop(p);
// 		return;
// 	}
	
	a = checkState(next_hop);
	if (a == 0)
		a = createState(next_hop);
	
	if (!(a->SABME_req_) && a->t_seqno_ == 0) {
		// this is the first pkt being sent
		// send out SABME request to start connection
		
		sendUA(p, SABME);

		// set SABME request flag to 1
		// have sent request, not yet confirmed
		a->SABME_req_ = 1;
		
		// set some timer for SABME?
		set_rtx_timer(a);
		
	}
        
        // place pkt in outgoing queue
	inSendBuffer(p, a);
	
	// send data pkts only after recving UA
	// in reply to SABME request
	if (a->SABME_req_ == 2) 
		sendMuch(a);
	
}


void HDLC::inSendBuffer(Packet *p, ARQstate *a) 
{
	// check for HDLC buffer overflow
	if (a->sendBuf_.length() >= queueSize_) {
		drop(p, "HDLC queue full");
		return;
	}
		
	hdr_cmn *ch = HDR_CMN(p);
	hdr_ip *ih = HDR_IP(p);
	hdr_hdlc *hh = HDR_HDLC(p);
	struct I_frame* ifr = (struct I_frame *)&(hh->hdlc_fc_);
	
	nsaddr_t src = (nsaddr_t)Address::instance().get_nodeaddr(ih->saddr());
	nsaddr_t dst = (nsaddr_t)Address::instance().get_nodeaddr(ih->daddr());
	
	hh->fc_type_ = HDLC_I_frame;
	hh->saddr_ = src;
	hh->daddr_ = dst;
	
	ifr->send_seqno = a->seqno_++;
	
	ch->size() += HDLC_HDR_LEN;
	
	a->sendBuf_.enque(p);
	
}

Packet *HDLC::dataToSend(ARQstate *a)
{
	Packet *dp;
	
	// if have any data to send
	if (a->t_seqno_ <= a->highest_ack_ + wnd_ && \
	    (dp = getPkt(a->sendBuf_, a->t_seqno_)) != 0) {
		
		// XXcheck if the destinations match for data and RR??
		
		Packet *np = dp->copy();
		return np;
	}
	return NULL;
}



// try to send as much as possible if have any pkts to send
void HDLC::sendMuch(ARQstate *a)
{
	// sanity check for a connection
	assert(a->SABME_req_ == 2);
	
	Packet *p;
	
	while (a->t_seqno_ <= a->highest_ack_ + wnd_ && (p = getPkt(a->sendBuf_, a->t_seqno_)) != 0) {
		Packet *dp = p->copy();
		output(dp, a, a->t_seqno_);
		a->t_seqno_++;
		
	} 
	
}


void HDLC::output(Packet *p, ARQstate *a, int seqno)
{
	int force_set_rtx_timer = 0;

	hdr_hdlc* hh = HDR_HDLC(p);
	struct I_frame* ifr = (struct I_frame *)&(hh->hdlc_fc_);

	// piggyback the last seqno recvd
	if (selRepeat_)
		ifr->recv_seqno = a->nextpkt_;
	else
		ifr->recv_seqno = a->recv_seqno_;

	// cancel the delay timer if pending
	if (a->delay_timer_->status() == TIMER_PENDING) 
		cancel_delay_timer(a);
	if (a->save_ != NULL) {
                Packet::free(a->save_);
                a->save_ = NULL;
        }
	
	sendDown(p);
	
	// if no outstanding data set rtx timer again
	if (a->highest_ack_ == a->maxseq_)
		force_set_rtx_timer = 1;
	if (seqno > a->maxseq_) 
		a->maxseq_ = seqno;
	else if (seqno < a->maxseq_)
		++(a->nrexmit_);
	
	if (!(a->rtx_timer_->status() == TIMER_PENDING) || force_set_rtx_timer)

		// No timer pending.  Schedule one.
		set_rtx_timer(a);
}


void HDLC::sendUA(Packet *p, COMMAND_t cmd)
{
	Packet *np = Packet::alloc();
	
	hdr_cmn *ch = HDR_CMN(p);
	hdr_cmn *nch = HDR_CMN(np);
	hdr_ip *ih = HDR_IP(p);
	hdr_ip *nih = HDR_IP(np);
	struct hdr_hdlc* nhh = HDR_HDLC(np);
	struct U_frame* uf = (struct U_frame *)&(nhh->hdlc_fc_);
	
	nsaddr_t src = (nsaddr_t)Address::instance().get_nodeaddr(ih->saddr());
	nsaddr_t dst = (nsaddr_t)Address::instance().get_nodeaddr(ih->daddr());

        // setup common hdr
	nch->addr_type() = ch->addr_type();
	nch->uid() = uidcnt_++;
	nch->ptype() = PT_HDLC;
	nch->size() = HDLC_HDR_LEN;
	nch->error() = 0;
	nch->iface() = -2;
	
	//nih->daddr() = ih->daddr();
	
	nhh->fc_type_ = HDLC_U_frame;

	switch(cmd) {
		
	case SABME:
	case DISC:
		// use same dst and src address
		nih->daddr() = ih->daddr();
		nih->saddr() = ih->saddr();
		nhh->daddr_ = dst;
		nhh->saddr_ = src;
		break;
		
	case UA:
		// use reply mode; src and dst get reversed
		nih->daddr() = ih->saddr();
		nih->saddr() = ih->daddr();
		nhh->daddr_ = src;
		nhh->saddr_ = dst;
		break;
		
	default:
		fprintf(stderr, "Unknown type of U frame\n");
		exit(1);
	}
	
	uf->utype = cmd;
	sendDown(np);
	
	//return (np);
}


void HDLC::sendDISC(Packet *p)
{
	sendUA(p, DISC);
	
}

void HDLC::delayTimeout(ARQstate *state)
{
	// The delay timer expired so we ACK the last pkt seen
	if (state->save_ != NULL) {
		Packet* pkt = state->save_;
		ack(pkt);
		state->save_ = NULL;
		Packet::free(pkt);
	}
}

void HDLC::ack(Packet *p)
{

	hdr_cmn *ch = HDR_CMN(p);
	int last_hop = ch->last_hop_;
	ARQstate *a = checkState(last_hop);

	// sanity check; but should not come here.
	if (a == 0) {
		printf("ack(): No state found for %d\n",last_hop);
		return;
	}
	
	Packet *dp;
	
	if (a->t_seqno_ > 0 && (dp = dataToSend(a)) != NULL) {
		output(dp, a, a->t_seqno_);	
		a->t_seqno_++;
		
	} else {
		
		sendRR(p,a);
	}
	
}


void HDLC::sendRR(Packet *p, ARQstate *a)
{
	Packet *np = Packet::alloc();
	
	hdr_cmn *ch = HDR_CMN(p);
	hdr_ip *ih = HDR_IP(p);
	struct hdr_hdlc *hh = HDR_HDLC(p);

	hdr_cmn *nch = HDR_CMN(np);
	hdr_ip *nih = HDR_IP(np);
	struct hdr_hdlc *nhh = HDR_HDLC(np);
	struct S_frame *sf = (struct S_frame*)&(nhh->hdlc_fc_);
	

	// common hdr
	nch->addr_type() = ch->addr_type();
	nch->uid() = uidcnt_++;
	nch->ptype() = PT_HDLC;
	nch->size() = HDLC_HDR_LEN;
	nch->error() = 0;
	nch->iface() = -2;
	
	nih->daddr() = ih->saddr();
	nih->saddr() = ih->daddr();
	
	nhh->fc_type_ = HDLC_S_frame;

	if (selRepeat_)
		sf->recv_seqno = a->nextpkt_;
	else
		sf->recv_seqno = a->recv_seqno_;
	
	sf->stype = RR;
	
	nhh->saddr_ = hh->daddr();
	nhh->daddr_ = hh->saddr();

	sendDown(np);
	
}

void HDLC::sendRNR(Packet *)
{}

void HDLC::sendREJ(Packet *p, ARQstate *a)
{
	Packet *np = Packet::alloc();

	hdr_cmn *ch = HDR_CMN(p);
	hdr_ip *ih = HDR_IP(p);
	struct hdr_hdlc *hh = HDR_HDLC(p);

	hdr_cmn *nch = HDR_CMN(np);
	hdr_ip *nih = HDR_IP(np);
	struct hdr_hdlc *nhh = HDR_HDLC(np);
	struct S_frame *sf = (struct S_frame *)&(nhh->hdlc_fc_);
	

	// common hdr
	nch->addr_type() = ch->addr_type();
	nch->uid() = uidcnt_++;
	nch->ptype() = PT_HDLC;
	nch->size() = HDLC_HDR_LEN;
	nch->error() = 0;
	nch->iface() = -2;
	
	nih->daddr() = ih->saddr();
	nih->saddr() = ih->daddr();
	
	nhh->fc_type_ = HDLC_S_frame;
	sf->recv_seqno = a->recv_seqno_;
	sf->stype = REJ;
	
	nhh->saddr_ = hh->daddr();
	nhh->daddr_ = hh->saddr();

	sendDown(np);
	
}

void HDLC::sendSREJ(Packet *p, int seq)
{
	Packet *np = Packet::alloc();

	hdr_cmn *ch = HDR_CMN(p);
	hdr_ip *ih = HDR_IP(p);
	struct hdr_hdlc *hh = HDR_HDLC(p);

	hdr_cmn *nch = HDR_CMN(np);
	hdr_ip *nih = HDR_IP(np);
	struct hdr_hdlc *nhh = HDR_HDLC(np);
	struct S_frame *sf = (struct S_frame *)&(nhh->hdlc_fc_);

	// common hdr
	nch->addr_type() = ch->addr_type();
	nch->uid() = uidcnt_++;
	nch->ptype() = PT_HDLC;
	nch->size() = HDLC_HDR_LEN;
	nch->error() = 0;
	nch->iface() = -2;
	
	nih->daddr() = ih->saddr();
	nih->saddr() = ih->daddr();
	
	nhh->fc_type_ = HDLC_S_frame;

	sf->recv_seqno = seq;
	sf->stype = SREJ;

	nhh->saddr_ = hh->daddr();
	nhh->daddr_ = hh->saddr();

	sendDown(np);

}



void HDLC::sendDown(Packet* p)
{	
	hdr_cmn *ch = HDR_CMN(p);
	
	char *mh = (char*)p->access(hdr_mac::offset_);
	int peer_mac_;
	SatChannel* satchannel_;

	getRoute(p);
	
	// Set mac src, type, and dst
	mac_->hdr_src(mh, mac_->addr());
	mac_->hdr_type(mh, ETHERTYPE_IP); // We'll just use ETHERTYPE_IP
	
	nsaddr_t dst = ch->next_hop();
	// a value of -1 is IP_BROADCAST
	if (dst < -1) {
		printf("Error:  next_hop_ field not set by routing agent\n");
		exit(1);
	}

	switch(ch->addr_type()) {

	case NS_AF_INET:
	case NS_AF_NONE:
		if (IP_BROADCAST == (u_int32_t) dst)
			{
			mac_->hdr_dst((char*) HDR_MAC(p), MAC_BROADCAST);
			break;
		}
		/* 
		 * Here is where arp would normally occur.  In the satellite
		 * case, we don't arp (for now).  Instead, use destination
		 * address to find the mac address corresponding to the
		 * peer connected to this channel.  If someone wants to
		 * add arp, look at how the wireless code does it.
		 */ 
		// Cache latest value used
		if (dst == arpcachedst_) {
			mac_->hdr_dst((char*) HDR_MAC(p), arpcache_);
			break;
		}
		// Search for peer's mac address (this is the pseudo-ARP)
		satchannel_ = (SatChannel*) channel();
		peer_mac_ = satchannel_->find_peer_mac_addr(dst);
		if (peer_mac_ < 0 ) {
			printf("Error:  couldn't find dest mac on channel ");
			printf("for src/dst %d %d at NOW %f\n", 
			    ch->last_hop_, dst, NOW); 
			exit(1);
		} else {
			mac_->hdr_dst((char*) HDR_MAC(p), peer_mac_);
			arpcachedst_ = dst;
			arpcache_ = peer_mac_;
			break;
		} 

	default:
		printf("Error:  addr_type not set to NS_AF_INET or NS_AF_NONE\n");
		exit(1);
	}
	
	// let mac decide when to take a new packet from the queue.
	Scheduler& s = Scheduler::instance();
	s.schedule(downtarget_, p, delay_);
}





// functions for receiving pkts from the network

void HDLC::recvIncoming(Packet* p)
{
	// This pkt is coming from the network
	// check if pkt has error
	
	if (hdr_cmn::access(p)->error() > 0)
		drop(p);
	else {
		hdr_hdlc* hh = HDR_HDLC(p);
		switch (hh->fc_type_) {
		case HDLC_I_frame:
			recvIframe(p);
			break;
		case HDLC_S_frame:
			recvSframe(p);
			break;
		case HDLC_U_frame:
			recvUframe(p);
			break;
		default:
			fprintf(stderr, "Invalid HDLC control type\n");
			exit(1);
		}
	}
}

// recv data pkt
void HDLC::recvIframe(Packet *p) 
{
	struct I_frame* ifr = (struct I_frame *)&(HDR_HDLC(p)->hdlc_fc_);
	int rseq = ((struct I_frame*)&(HDR_HDLC(p)->hdlc_fc_))->recv_seqno;
	
	// check if any ack is piggybacking
	// and update highest_ack_
	int last_hop = HDR_CMN(p)->last_hop_;
	ARQstate *a = checkState(last_hop);
	
	// if state doesn't exist, or connection not setup yet
	// drop data pkt
	if (a == 0 || a->SABME_req_ == 1) {
		printf("recvIframe(): No state/connection found for %d\n",last_hop);
		return;
	}
	
	// if this is the first data pkt we recv, set the
	// SABME flag, as this is the third leg of the 3 way handshake
	// for connection setup for HDLC
	if (a->recv_seqno_ == -1 && a->SABME_req_ == 0)
		a->SABME_req_ = 2;
	
	if (rseq > -1)  // valid ack
		handlePiggyAck(p,a);
	
	// recvd first data pkt
	if (ifr->send_seqno == 0 && a->recv_seqno_ == -1) 
		a->recv_seqno_ = 0;

	if (selRepeat_) // do selective repeat
	
		selectiveRepeatMode(p);
	else
		// default to go back N
		goBackNMode(p);
	
	
	// if we had got a valid piggy ack
	// see if we can send more
	if (rseq > -1)  
		sendMuch(a);
	
}

void HDLC::recvSframe(Packet* p)
{
	hdr_hdlc* hh = HDR_HDLC(p);
	struct S_frame *sf = (struct S_frame*)&(hh->hdlc_fc_);
	
	
	switch(sf->stype)
	{
	case RR:
		handleRR(p);
		break;
		
	case REJ:
		handleREJ(p);
		break;
		
	case RNR:
		handleRNR(p);
		break;
		
	case SREJ:
		handleSREJ(p);
		break;
		
	default:
		fprintf(stderr, "Unknown type of S frame\n");
		exit(0);
		
	}
	
}

void HDLC::recvUframe(Packet* p)
{
	// U frames supported for now are ABME/UA/DISC
	hdr_hdlc* hh = HDR_HDLC(p);
	struct U_frame *uf = (struct U_frame*)&(hh->hdlc_fc_);
	
	switch(uf->utype) 
	{
	case SABME:
		handleSABMErequest(p);
		break;
		
	case UA:
		handleUA(p);
		break;
		
	case DISC:
		handleDISC(p);
		break;
		
	default:
		fprintf(stderr, "Unknown type of U frame\n");
		exit(1);
	}
	
}

void HDLC::handleSABMErequest(Packet *p)
{
	hdr_cmn *ch = HDR_CMN(p);
	int last_hop = ch->last_hop_;

	ARQstate *a = checkState(last_hop);

#ifdef RESET_HDLC
	if (a != 0) {
		
		// For HDLC resetting option, we might recv a SABME req since the sender got reset but the recvr didn't. In that case the recvr should reset in order to synchronise with sender.
		printf("Got SABME req for existing connection; resetting\n");
		reset(a);
	}
	
	// create new state
	a = createState(last_hop);
#else
	// create state only if one is not present already
	if (a == 0)
		a = createState(last_hop);
#endif	
	
        // got a request to either open a connection
	// or reset an old connection as sender may have reset due to timeout
        // ack back an UA
	//if (a->recv_seqno_ == -1) {
	sendUA(p, UA);
	//closed_ = 0;
	//}

	Packet::free(p);
}


void HDLC::handleDISC(Packet *p)
{
	hdr_cmn *ch = HDR_CMN(p);
	int last_hop = ch->last_hop_;
	ARQstate *a = checkState(last_hop);
	
        // got request for disconnect
	// send UA 
	reset(a);
	sendUA(p, UA);
	Packet::free(p);

}

void HDLC::handleUA(Packet *p)
{
	hdr_cmn *ch = HDR_CMN(p);
	int last_hop = ch->last_hop_;
	ARQstate *a = checkState(last_hop);
	
	if (a == 0) {
		printf("handleUA: No state found for %d\n", last_hop);
		return;
	}
	
	// recv ok for connection
	// if waiting to send, start sending
	// ?? shouldn't I match the addresses here??
	if (a->t_seqno_ == 0 && a->SABME_req_ == 1) {
		// set SABME request flag to 2
		// indicating a confirmed connection
		a->SABME_req_ = 2;
		
		// cancel the SABME timers
		cancel_rtx_timer(a);
		
		sendMuch(a);
		//closed_ = 0;
		
	} else { // have I sent a DISCONNECT?
		if (a->disconnect_) {
			// recvd confirmation on disconnect
			reset(a);
		}
	}
	Packet::free(p);
	
}


void HDLC::handlePiggyAck(Packet *p, ARQstate *a)
{
	hdr_hdlc* hh = HDR_HDLC(p);
	struct I_frame *ifr =  (struct I_frame*)&(hh->hdlc_fc_);

	int seqno = ifr->recv_seqno - 1;
	if (seqno > a->highest_ack_) {  // recvd a new ack
		
		Packet *datapkt = getPkt(a->sendBuf_, seqno);
		a->sendBuf_.remove(datapkt);
		Packet::free(datapkt);

		// update highest_ack_
		a->highest_ack_ = seqno;
		
		// set retx_timer
		if (a->t_seqno_ > seqno || seqno < a->maxseq_)
			set_rtx_timer(a);
		
		else
			cancel_rtx_timer(a);
		
	} else { // got duplicate acks
		// do nothing as piggyback ack
	}
	
	// if had timeouts, should reset it now
	a->ntimeouts_ = 0;
	
}


// receiver ready - ack for a pkt successfully recvd by receiver, send next pkt pl
void HDLC::handleRR(Packet *p)
{
	// recvd an ack for a pkt
	// remove data pkt from outgoing buffer, if applicable
	struct S_frame *sf = (struct S_frame *)&(HDR_HDLC(p)->hdlc_fc_);
	int seqno = sf->recv_seqno - 1;
	int last_hop = HDR_CMN(p)->last_hop_;
	ARQstate *a = checkState(last_hop);
	
	if (a == 0 || a->SABME_req_ != 2){
		printf("handleRR: No state/connection found for %d\n", last_hop);
		return;
	}

	if (seqno > a->highest_ack_) {  // recvd a new ack
		
		Packet *datapkt = getPkt(a->sendBuf_, seqno);
		if (datapkt != NULL) {
			a->sendBuf_.remove(datapkt);
			Packet::free(datapkt);
			
			// update highest_ack_
			a->highest_ack_ = seqno;
		}
		
		// set retx_timer
		if ((a->t_seqno_-1) > seqno || seqno < a->maxseq_ )
			set_rtx_timer(a);
		
		else
			cancel_rtx_timer(a);
		
	} else { // got duplicate acks
		
		drop(p);
	}
	
	// if had timeouts, should reset it now
	a->ntimeouts_ = 0;
	
	// try to send more
	Packet::free(p);
	sendMuch(a);
	
}


Packet *HDLC::getPkt(PacketQueue buffer, int seqno)
{
	Packet *p;
	
	buffer.resetIterator();
	for (p = buffer.getNext(); p != 0; p = buffer.getNext()) {
		
		if (((struct I_frame *)&(HDR_HDLC(p)->hdlc_fc_))->send_seqno == seqno)
			return p;
	}

	return(NULL);
}

// receiver not ready
void HDLC::handleRNR(Packet *)
{
	// stop sending pkts and wait until RR is recved.
	// wait for how long?
}

// handle reject or a go-back-N request from receiver
void HDLC::handleREJ(Packet *rejp)
{
	// set seqno_ = seqno in REJ
	// and start sending
	struct S_frame *sf = (struct S_frame *)&(HDR_HDLC(rejp)->hdlc_fc_);
	int seqno = sf->recv_seqno;
	int last_hop = HDR_CMN(rejp)->last_hop_;
	ARQstate *a = checkState(last_hop);
	
	if (a == 0 || a->SABME_req_ != 2) {
		printf("handleREJ: No state/connection found for %d\n", last_hop);
		return;
	}

	a->t_seqno_ = seqno;
	sendMuch(a);
	
	Packet::free(rejp);
}


// selective reject or a NACK for a data pkt not recvd from the receiver
void HDLC::handleSREJ(Packet *rejp)
{
	
	struct S_frame *sf = (struct S_frame *)&(HDR_HDLC(rejp)->hdlc_fc_);
	int seqno = sf->recv_seqno;
	int last_hop = HDR_CMN(rejp)->last_hop_;
	ARQstate *a = checkState(last_hop);
	
	if (a == 0 || a->SABME_req_ != 2) {
		printf("handleSREJ: No state/connection found for %d\n", last_hop);
		return;
	}

	// resend only the pkt that was requested
	Packet *p = getPkt(a->sendBuf_, seqno);
	Packet *dp = p->copy();
	
	output(dp, a, seqno);
	
	Packet::free(rejp);
	
}

void HDLC::reset(ARQstate *a)
{
	Packet *p;
	int n;
	
	// cancel all pending timeouts
	if (a->rtx_timer_->status() == TIMER_PENDING) 
		cancel_rtx_timer(a);
	if (a->reset_timer_->status() == TIMER_PENDING)
		cancel_reset_timer(a);
	if (a->delay_timer_->status() == TIMER_PENDING)
		cancel_delay_timer(a);
	
	// now delete timers
	delete a->rtx_timer_;
 	delete a->reset_timer_;
 	delete a->delay_timer_;

	// purge send buffer, if any
	n = a->highest_ack_ + 1;
	while ((p = getPkt(a->sendBuf_, n)) != NULL) 
 	{
 		a->sendBuf_.remove(p);
 		Packet::free(p);
 		n++;
 	}

	// purge recv buffer, if any
	if (selRepeat_)
 	{
 		// purge recv side buffer
 		// to send up out-of-order pkts
 		// should we drop these pkts instead ??
		for (n=0; n <= HDLC_MWM; n++) {
			
 			Packet *p = a->seen_[n];
			if (p != 0) {
				uptarget_ ? sendUp(p) : drop(p);
				a->seen_[n] = 0;
			}
 		}
		delete a->seen_;
	}
	if (a->save_)
		delete a->save_;

	removeState(a->nh_);
	
}

// void HDLC::reset_recvr(ARQstate *a)
// {
// 	int n = 0;
	
// 	if (selRepeat_)
// 	{
// 		// purge recv side buffer
// 		// to send up out-of-order pkts
// 		// should we drop these pkts instead ??
// 		while (a->seen_[n] != NULL){
// 			Packet *p = a->seen_[n];
// 			a->seen_[n] = 0;
// 			uptarget_ ? sendUp(p) : drop(p);
// 			++n;
// 		}
// 		delete a->seen_;
// 		delete a->save_;
		
// 	}
	
// 	delete rtx_timer_;
// 	delete reset_timer_;
// 	delete delay_timer_;
	
// 	removeState(a);
// }

		
		

// void HDLC::reset_sender(ARQstate *a)
// {
// 	Packet *p;
// 	int n = a->highest_ack_ + 1;
	
// 	// purge pkts in sendBuf
// 	while ((p = getPkt(a->sendBuf_, n)) != NULL) 
// 	{
// 		a->sendBuf_.remove(p);
// 		Packet::free(p);
// 		n++;
// 	}
	
//         // delete timers
	
	
	 
// }

// void HDLC::set_ack_timer()
// {
// 	ack_timer_.sched(DELAY_ACK_VAL);
// }


void HDLC::reset_rtx_timer(ARQstate *a, int backoff)
{
	if (backoff)
		rtt_backoff();
	set_rtx_timer(a);
	a->t_seqno_ = a->highest_ack_ + 1;
}

void HDLC::set_rtx_timer(ARQstate *a)
{
	a->rtx_timer_->resched(rtt_timeout());
}

void HDLC::set_reset_timer(ARQstate *a)
{
	a->reset_timer_->resched(reset_timeout());
}


void HDLC::timeout(ARQstate *a)
{
	//char buf[SMALL_LEN];
	
	//if (disconnect_) {
	//sentDISC_ = 0;
	//return;
	//}
	double now = Scheduler::instance().clock();
	
	a->ntimeouts_++;
	
	printf("hdlc TIMEOUT:%f, nh=%d, nto=%d\n", now, a->nh_,a->ntimeouts_);
	//trace_event(buf);
	
	if (a->ntimeouts_ > maxTimeouts_) {
		//disconnect_ = 1;
		//sendDISC();

#ifdef RESET_HDLC
		reset(a);
#endif
		return;
	}

	// SABME timeout
	if (a->SABME_req_ && a->t_seqno_ == 0 ) {
		
		if (a->ntimeouts_ <= maxTimeouts_) {
			Packet *p = getPkt(a->sendBuf_, 0);
			sendUA(p, SABME);
			set_rtx_timer(a);
		
		} else {
			fprintf(stderr,"SABME timeout: No connection\n");
#ifdef RESET_HDLC
			// flush all pkts 
			reset(a);
#endif			
		}
		return;
	}
	
	
	
	if (a->highest_ack_ == a->maxseq_) {
		// no outstanding data
		// then don't do  anything
		reset_rtx_timer(a,0);

	} else {
		a->t_seqno_ = a->highest_ack_ + 1;
		sendMuch(a);
		
	}
}

double HDLC::rtt_timeout()
{
	return timeout_;
}

double HDLC::reset_timeout()
{
	if (maxTimeouts_ < 1)
		maxTimeouts_ = 1;
	
	return (timeout_ * maxTimeouts_);
}

void HDLC::rtt_backoff()
{
	// no backoff for now
}


// doing GoBAckN error recovery
void HDLC::goBackNMode(Packet *p)
{
	hdr_cmn *ch = HDR_CMN(p);
	hdr_hdlc* hh = HDR_HDLC(p);
	struct I_frame* ifr = (struct I_frame *)&(hh->hdlc_fc_);
	int last_hop = ch->last_hop_;
	ARQstate *a = checkState(last_hop);
	
	// recv data in order
	if (ifr->send_seqno == a->recv_seqno_) {
		
		if (a->sentREJ_)
			a->sentREJ_ = 0;
		
		a->recv_seqno_++;
		
                // strip off hdlc hdr
		ch->size() -= HDLC_HDR_LEN;

		// send ack back
		// start a timer to delay the ack so that
		// we can try and piggyback the ack in some data pkt
		if (delAck_) {
			
			if (a->delay_timer_->status() != TIMER_PENDING) {
				assert(a->save_ == NULL);
				a->save_ = p->copy();
				a->delay_timer_->sched(delAckVal_);
			} 
			
		} else {
			ack(p);
			
		}
		
		uptarget_ ? sendUp(p) : drop(p);

		
	} else if (ifr->send_seqno > a->recv_seqno_) {

		if (!a->sentREJ_) {
			a->sentREJ_ = 1;
			// since GoBackN send REJ for the first
			// out of order pkt
			sendREJ(p,a);
			// set a timer for resetting recvr on timeout
			// waiting for in order pkt
#ifdef RESET_HDLC
			set_reset_timer(a);
#endif
			
		}
		
		drop(p, "Pkt out of order");
		
	} else {
		// send_seqno < recv_seqno; duplicate data pkts
		// send ack back as previous RR maybe lost
		ack(p);
		//ack();
		drop(p, "Duplicate pkt");
	}
	
}


// Selective Repeat mode of error recovery
// in case of a missing pkt, send SREJ for that pkt only

void HDLC::selectiveRepeatMode(Packet* p)
{
	hdr_cmn *ch = HDR_CMN(p);
	int seq =  ((struct I_frame *)&(HDR_HDLC(p)->hdlc_fc_))->send_seqno;
	bool just_marked_as_seen = FALSE;
	int last_hop = ch->last_hop_;
	ARQstate *a = checkState(last_hop);

	// resize buffers
	// while (seq + 1 - next >= wndmask_) {
	
// 		resize_buffers((wndmask_+1)*2);
// 	}
	
	// strip off hdlc hdr
	ch->size() -= HDLC_HDR_LEN;
	
	if (seq > a->maxseen_) {
		// the packet is the highest we've seen so far
		int i;
		for (i = a->maxseen_ + 1; i < seq; i++) {
			sendSREJ(p, i);
		}

		// we record the packets between the old maximum and
		// the new max as being "unseen" i.e. 0 
		a->maxseen_ = seq;
		
		// place pkt in buffer
		a->seen_[a->maxseen_ & wndmask_] = p;
		
		// necessary so this packet isn't confused as being a duplicate
		just_marked_as_seen = TRUE;
		
	}
	
	if (seq < a->nextpkt_) {
		// Duplicate packet case 1: the packet is to the left edge of
		// the receive window; therefore we must have seen it
		// before
#ifdef DEBUGHDLC
		printf("%f\t Received duplicate packet %d\n",Scheduler::instance().clock(),seq);
#endif
		ack(p);
		drop(p);
		return;
		
	}
	
	int next = a->nextpkt_;
	
	if (seq >= a->nextpkt_ && seq <= a->maxseen_) {
		// next is the left edge of the recv window; maxseen_
		// is the right edge; execute this block if there are
		// missing packets in the recv window AND if current
		// packet falls within those gaps

		if (a->seen_[seq & wndmask_] && !just_marked_as_seen) {
		// Duplicate case 2: the segment has already been
		// recorded as being received (AND not because we just
		// marked it as such)
#ifdef DEBUGHDLC
			printf("%f\t Received duplicate packet %d\n",Scheduler::instance().clock(),seq);
#endif
			ack(p);
			drop(p);
			return;
			
		}

		// record the packet as being seen
		a->seen_[seq & wndmask_] = p;

		while ( a->seen_[next & wndmask_] != NULL ) {
			// this loop first gets executed if seq==next;
			// i.e., this is the next packet in order that
			// we've been waiting for.  the loop sets how
			// many pkt we can now deliver to the
			// application, due to this packet arriving
			// (and the prior arrival of any pkts
			// immediately to the right)

			Packet* rpkt = a->seen_[next & wndmask_];
			a->seen_[next & wndmask_] = 0;
			
			uptarget_ ? sendUp(rpkt) : drop(rpkt);
			
			++next;
		}

		// store the new left edge of the window
		a->nextpkt_ = next;

		// send ack 
		ack(p);
	}
	
}

ARQstate *HDLC::newEntry(int next_hop)
{
	ARQstate *a = new ARQstate;
	
	a->nh_ = next_hop;
	a->t_seqno_ = 0;
	a->seqno_ = 0;
	a->maxseq_ = 0;
	a->highest_ack_ = -1;
	a->recv_seqno_ = -1;
	a->nrexmit_ = 0;
	a->ntimeouts_ = 0;
	a->closed_ = 0;
	a->disconnect_ = 0;
	a->sentDISC_ = 0;
	a->SABME_req_ = 0;
	a->sentREJ_ = 0;
	a->save_ = NULL;
	a->rtx_timer_ = new HdlcTimer(this, a, &HDLC::timeout);
	a->reset_timer_ = new HdlcTimer(this, a, &HDLC::reset);
        a->delay_timer_ = new HdlcTimer(this, a, &HDLC::delayTimeout);
	
	a->nextpkt_ = 0;
	a->next_ = 0;

	if (selRepeat_) {
		
		a->seen_ = new Packet*[(HDLC_MWM+1)];
		memset(a->seen_, 0, (sizeof(Packet *) * (HDLC_MWM+1)));
	}
	
	return a;
	
}

ARQstate *HDLC::createState(int next_hop)
{
	ARQstate *a = list_head_;
	
	if (list_head_ == 0 ) {
	
		list_head_ = newEntry(next_hop);
		return list_head_;
		
	} else {
		while (a->next_)
			a = a->next_;
		a->next_ = newEntry(next_hop);
		return (a->next_);
	}
}

	

ARQstate *HDLC::checkState(int next_hop)
{
	ARQstate *a;

	if (list_head_ == 0)
		return 0;
	
	for(a=list_head_; a != 0; a=a->next_) {
		if (a->nh_ == next_hop)
			return a;
	}
		
	// no existing state
	return 0;
}



void HDLC::removeState(int next_hop)
{
	ARQstate *a, *p;
	
	for(p=0, a=list_head_; a != 0; p=a, a=a->next_) {
		if (a->nh_ == next_hop) {
			
			if (a == list_head_) {
				if (a->next_ == 0)
					list_head_ = 0;
				else
					list_head_ = a->next_;
				
				delete a;
				return;
				

			} else {
				p->next_ = a->next_;
				delete a;
				return;
				
			}
		}
	}
	
	fprintf(stderr, "HDLC:removeState() couldn't find state\n");
}
	
