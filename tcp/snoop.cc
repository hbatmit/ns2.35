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
 * 	This product includes software developed by the Daedalus Research
 * 	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
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
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/tcp/snoop.cc,v 1.26 2010/03/08 05:54:54 tom_henderson Exp $ (UCB)";
#endif

#include "snoop.h"

int hdr_snoop::offset_;

class SnoopHeaderClass : public PacketHeaderClass {
public:
        SnoopHeaderClass() : PacketHeaderClass("PacketHeader/Snoop",
						sizeof(hdr_snoop)) {
		bind_offset(&hdr_snoop::offset_);
	}
} class_snoophdr;

static class LLSnoopClass : public TclClass {
public:
	LLSnoopClass() : TclClass("LL/LLSnoop") {}
	TclObject* create(int, const char*const*) {
		return (new LLSnoop());
	}
} llsnoop_class;

static class SnoopClass : public TclClass {
public:
	SnoopClass() : TclClass("Snoop") {}
	TclObject* create(int, const char*const*) {
		return (new Snoop());
	}
} snoop_class;

Snoop::Snoop() : NsObject(),
	fstate_(0), lastSeen_(-1), lastAck_(-1), 
	expNextAck_(0), expDupacks_(0), bufhead_(0), 
	toutPending_(0), buftail_(0),
	wl_state_(SNOOP_WLEMPTY), wl_lastSeen_(-1), wl_lastAck_(-1), 
	wl_bufhead_(0), wl_buftail_(0)
{
	bind("snoopDisable_", &snoopDisable_);
	bind_time("srtt_", &srtt_);
	bind_time("rttvar_", &rttvar_);
	bind("maxbufs_", &maxbufs_);
	bind("snoopTick_", &snoopTick_);
	bind("g_", &g_);
	bind("tailTime_", &tailTime_);
	bind("rxmitStatus_", &rxmitStatus_);
	bind("lru_", &lru_);

	rxmitHandler_ = new SnoopRxmitHandler(this);

	int i;
	for (i = 0; i < SNOOP_MAXWIND; i++) /* data from wired->wireless */
		pkts_[i] = 0;
	for (i = 0; i < SNOOP_WLSEQS; i++) {/* data from wireless->wired */
		wlseqs_[i] = (hdr_seq *) malloc(sizeof(hdr_seq));
		wlseqs_[i]->seq = wlseqs_[i]->num = 0;
	}
	if (maxbufs_ == 0)
		maxbufs_ = SNOOP_MAXWIND;
}

void
Snoop::reset()
{
//	printf("%x resetting\n", this);
	fstate_ = 0;
	lastSeen_ = -1;
	lastAck_ = -1;
	expNextAck_ = 0;
	expDupacks_ = 0;
	bufhead_ = buftail_ = 0;
	if (toutPending_) {
		Scheduler::instance().cancel(toutPending_);
		// xxx: I think that toutPending_ doesn't need to be freed because snoop didn't allocate it (but I'm not sure).
		toutPending_ = 0;
	};
	for (int i = 0; i < SNOOP_MAXWIND; i++) {
		if (pkts_[i]) {
			Packet::free(pkts_[i]);
			pkts_[i] = 0;
		}
	}
}

void 
Snoop::wlreset()
{
	wl_state_ = SNOOP_WLEMPTY;
	wl_bufhead_ = wl_buftail_ = 0;
	for (int i = 0; i < SNOOP_WLSEQS; i++) {
		wlseqs_[i]->seq = wlseqs_[i]->num = 0;
	}
}


int 
Snoop::command(int argc, const char*const* argv)
{
	//Tcl& tcl = Tcl::instance();

	if (argc == 3) {
		if (strcmp(argv[1], "llsnoop") == 0) {
			parent_ = (LLSnoop *) TclObject::lookup(argv[2]);
			if (parent_)
				recvtarget_ = parent_->uptarget();
			return (TCL_OK);
		}
		
		if (strcmp(argv[1], "check-rxmit") == 0) {
			if (empty_()) {
				rxmitStatus_ = SNOOP_PROPAGATE;
				return (TCL_OK);
			}

			Packet *p = pkts_[buftail_];
			hdr_snoop *sh = hdr_snoop::access(p);

			if (sh->sndTime()!=-1 && sh->sndTime()<atoi(argv[2]) &&
			    sh->numRxmit() == 0)
				/* candidate for retransmission */
				rxmitStatus_ = snoop_rxmit(p);
			else
				rxmitStatus_ = SNOOP_PROPAGATE;
			return (TCL_OK);
		}
	}
	return NsObject::command(argc, argv);
}

void LLSnoop::recv(Packet *p, Handler *h)
{
	Tcl &tcl = Tcl::instance();
	hdr_ip *iph = hdr_ip::access(p);

	/* get-snoop creates a snoop object if none currently exists */
	hdr_cmn *ch = HDR_CMN(p);
	if(ch->direction() == hdr_cmn::UP) 
		/* get-snoop creates a snoop object if none currently exists */
		/* In ns, addresses have ports embedded in them. */
		tcl.evalf("%s get-snoop %d %d", name(), iph->daddr(),
			  iph->saddr()); 
       
	else  
		tcl.evalf("%s get-snoop %d %d", name(), iph->saddr(),
			  iph->daddr());
	
	Snoop *snoop = (Snoop *) TclObject::lookup(tcl.result());
	
	snoop->recv(p, h);
	
	if (integrate_)
		tcl.evalf("%s integrate %d %d", name(), iph->saddr(),
			  iph->daddr());
	if (h)			/* resume higher layer (queue) */
		Scheduler::instance().schedule(h, &intr_, 0.000001);
	return;
}

/*
 * Receive a packet from higher layer or from the network.
 * Call snoop_data() if TCP packet and forward it on if it's an ack.
 */
void
Snoop::recv(Packet* p, Handler*)
{	
	
	hdr_cmn *ch = HDR_CMN(p);	
	if(ch->direction() == hdr_cmn::UP) {
		handle((Event *) p);
		return;
	}
	
	packet_t type = hdr_cmn::access(p)->ptype();
	/* Put packet (if not ack) in cache after checking, and send it on */
	
	if (type == PT_TCP) 
		snoop_data(p);
	
	else if (type == PT_ACK)
		snoop_wired_ack(p);
	
	ch->direction() = hdr_cmn::DOWN;  // Ben added
	parent_->sendDown(p);	/* vector to LLSnoop's sendto() */
}

/*
 * Handle a packet received from peer across wireless link.  Check first
 * for packet errors, then call snoop_ack() or pass it up as necessary.
 */
void
Snoop::handle(Event *e)
{

	Packet *p = (Packet *) e;
	packet_t type = hdr_cmn::access(p)->ptype();
	//int seq = hdr_tcp::access(p)->seqno();
	int prop = SNOOP_PROPAGATE; // by default;  propagate ack or packet
	Scheduler& s = Scheduler::instance();

	//hdr_ll *llh = hdr_ll::access(p);
	if (hdr_cmn::access(p)->error()) {
		parent_->drop(p);	// drop packet if it's been corrupted
		return;
	}

	if (type == PT_ACK) 
		prop = snoop_ack(p); 

	else if (type == PT_TCP) /* XXX what about TELNET? */
		snoop_wless_data(p);

	if (prop == SNOOP_PROPAGATE)
		s.schedule(recvtarget_, e, parent_->delay());
	else {			// suppress ack
		/*		printf("---- %f suppressing ack %d\n", s.clock(), seq);*/
		Packet::free(p);
	}
}

/*
 * Data packet processing.  p is guaranteed to be of type PT_TCP when 
 * this function is called.
 */
void
Snoop::snoop_data(Packet *p)
{
	Scheduler &s = Scheduler::instance();
	int seq = hdr_tcp::access(p)->seqno();
	int resetPending = 0;
	
	//	printf("%x snoop_data: %f sending packet %d\n", this, s.clock(), seq);
	if (fstate_ & SNOOP_ALIVE && seq == 0)
		reset();
	fstate_ |= SNOOP_ALIVE;
	if ((fstate_ & SNOOP_FULL) && !lru_) {
//		printf("snoop full, fwd'ing\n t %d h %d", buftail_, bufhead_);
		if (seq > lastSeen_)
			lastSeen_ = seq;
		return;
	}
	/* 
	 * Only if the ifq is NOT full do we insert, since otherwise we want
	 * congestion control to kick in.
	 */

	if (parent_->ifq()->length() < parent_->ifq()->limit()-1)
		resetPending = snoop_insert(p);
	if (toutPending_ && resetPending == SNOOP_TAIL) {
		s.cancel(toutPending_);
		// xxx: I think that toutPending_ doesn't need to be freed because snoop didn't allocate it (but I'm not sure).
		toutPending_ = 0;
	}
	if (!toutPending_ && !empty_()) {
		toutPending_ = (Event *) (pkts_[buftail_]);
		s.schedule(rxmitHandler_, toutPending_, timeout());
	}
	return;
}

/* 
 * snoop_insert() does all the hard work for snoop_data(). It traverses the 
 * snoop cache and looks for the right place to insert this packet (or
 * determines if its already been cached). It then decides whether
 * this is a packet in the normal increasing sequence, whether it
 * is a sender-rexmitted-but-lost-due-to-congestion (or network 
 * out-of-order) packet, or if it is a sender-rexmitted packet that
 * was buffered by us before.
 */
int
Snoop::snoop_insert(Packet *p)
{



	int i, seq = hdr_tcp::access(p)->seqno(), retval=0;

	if (seq <= lastAck_) 
		return retval;
	
	if (fstate_ & SNOOP_FULL) {
		/* free tail and go on */
		printf("snoop full, making room\n");
		Packet::free(pkts_[buftail_]);
		pkts_[buftail_] = 0;
		buftail_ = next(buftail_);
		fstate_ |= ~SNOOP_FULL;
	}

	if (seq > lastSeen_ || pkts_[buftail_] == 0) { // in-seq or empty cache
		i = bufhead_;
		bufhead_ = next(bufhead_);
	} else if (seq < hdr_snoop::access(pkts_[buftail_])->seqno()) {
		buftail_ = prev(buftail_);
		i = buftail_;
	} else {
		for (i = buftail_; i != bufhead_; i = next(i)) {
			hdr_snoop *sh = hdr_snoop::access(pkts_[i]);
			if (sh->seqno() == seq) {  // cached before

				sh->numRxmit() = 0;
				sh->senderRxmit() = 1; //must be a sender retr
				sh->sndTime() = Scheduler::instance().clock();
				return SNOOP_TAIL;
			} else if (sh->seqno() > seq) { 

				//not cached before, should insert in the middle
				// find the position it should be: prev(i)
 
				Packet *temp = pkts_[prev(buftail_)];
				for (int j = buftail_; j != i; j = next(j)) 
					pkts_[prev(j)] = pkts_[j];
				i = prev(i);
				pkts_[i] = temp;   // seems not necessary. Ben comments
				buftail_ = prev(buftail_);
				break;
			}
		}

		// This should not happen, since seq must be > lastSeen, which is 
		// handled before in the first if.   Ben comments
		if (i == bufhead_)
			bufhead_ = next(bufhead_);
	}
	
	// save in the buffer
	savepkt_(p, seq, i);
	
	if (bufhead_ == buftail_)
		fstate_ |= SNOOP_FULL;
	/* 
	 * If we have one of the following packets:
	 * 1. a network-out-of-order packet, or
	 * 2. a fast rxmit packet, or 3. a sender retransmission 
	 * AND it hasn't already been buffered, 
	 * then seq will be < lastSeen_. 
	 * We mark this packet as having been due to a sender rexmit 
	 * and use this information in snoop_ack(). We let the dupacks
	 * for this packet go through according to expDupacks_.
	 */
	if (seq < lastSeen_) { /* not in-order -- XXX should it be <= ? */
		if (buftail_ == i) {
			hdr_snoop *sh = hdr_snoop::access(pkts_[i]);
			sh->senderRxmit() = 1;
			sh->numRxmit() = 0;
		}
		expNextAck_ = buftail_;
		retval = SNOOP_TAIL;
	} else
		lastSeen_ = seq;
	
	return retval;
}

void
Snoop::savepkt_(Packet *p, int seq, int i)
{
	pkts_[i] = p->copy();
	Packet *pkt = pkts_[i];
	hdr_snoop *sh = hdr_snoop::access(pkt);
	sh->seqno() = seq;
	sh->numRxmit() = 0;
	sh->senderRxmit() = 0;
	sh->sndTime() = Scheduler::instance().clock();
}

/*
 * Ack processing in snoop protocol.  We know for sure that this is an ack.
 * Return SNOOP_SUPPRESS if ack is to be suppressed and SNOOP_PROPAGATE o.w.
 */
int
Snoop::snoop_ack(Packet *p)
{
	Packet *pkt;

	int ack = hdr_tcp::access(p)->seqno();

	/*
	 * There are 3 cases:
	 * 1. lastAck_ > ack.  In this case what has happened is
	 *    that the acks have come out of order, so we don't
	 *    do any local processing but forward it on.
	 * 2. lastAck_ == ack.  This is a duplicate ack. If we have
	 *    the packet we resend it, and drop the dupack.
	 *    Otherwise we never got it from the fixed host, so we
	 *    need to let the dupack get through.
	 *    Set expDupacks_ to number of packets already sent
	 *    This is the number of dup acks to ignore.
	 * 3. lastAck_ < ack.  Set lastAck_ = ack, and update
	 *    the head of the buffer queue. Also clean up ack'd packets.
	 */
	if (fstate_ & SNOOP_CLOSED || lastAck_ > ack) 
		return SNOOP_PROPAGATE;	// send ack onward

	if (lastAck_ == ack) {	
		/* A duplicate ack; pure window updates don't occur in ns. */

		pkt = pkts_[buftail_];
		
		if (pkt == 0) 
			return SNOOP_PROPAGATE;
		
		hdr_snoop *sh = hdr_snoop::access(pkt);

		if (pkt == 0 || sh->seqno() > ack + 1) 
			/* don't have packet, letting thru' */
		        return SNOOP_PROPAGATE;

		/* 
		 * We have the packet: one of 3 possibilities:
		 * 1. We are not expecting any dupacks (expDupacks_ == 0)
		 * 2. We are expecting dupacks (expDupacks_ > 0)
		 * 3. We are in an inconsistent state (expDupacks_ == -1)
		 */

			
		if (expDupacks_ == 0) {	// not expecting it
#define RTX_THRESH 1
			
			static int thresh = 0;
			if (thresh++ < RTX_THRESH) 
				/* no action if under RTX_THRESH */
				return SNOOP_PROPAGATE;
			
			thresh = 0;
			
			// if the packet is a sender retransmission, pass on
			if (sh->senderRxmit()) 
				return SNOOP_PROPAGATE;
			
			/*
			 * Otherwise, not triggered by sender.  If this is
			 * the first dupack recd., we must determine how many
			 * dupacks will arrive that must be ignored, and also
			 * rexmit the desired packet.  Note that expDupacks_
			 * will be -1 if we miscount for some reason.
			 */
			
			
			expDupacks_ = bufhead_ - expNextAck_;
			if (expDupacks_ < 0)
				expDupacks_ += SNOOP_MAXWIND;
			expDupacks_ -= RTX_THRESH + 1;
			expNextAck_ = next(buftail_);

			if (sh->numRxmit() == 0) 
				return snoop_rxmit(pkt);
		} else if (expDupacks_ > 0) {
			expDupacks_--;
			return SNOOP_SUPPRESS;
		} else if (expDupacks_ == -1) {
			if (sh->numRxmit() < 2) {
				return snoop_rxmit(pkt);
			}
		} else		// let sender deal with it
			return SNOOP_PROPAGATE;
	} else {		// a new ack

		fstate_ &= ~SNOOP_NOACK; // have seen at least 1 new ack

		/* free buffers */
		double sndTime = snoop_cleanbufs_(ack);
		
		if (sndTime != -1)
			snoop_rtt(sndTime);

		expDupacks_ = 0;
		expNextAck_ = buftail_;
		lastAck_ = ack;
	}
	return SNOOP_PROPAGATE;
}

/* 
 * Handle data packets that arrive from a wireless link, and we're not
 * the end recipient.  See if there are any holes in the transmission, and
 * if there are, mark them as candidates for wireless loss.  Then, when
 * (dup)acks troop back for this loss, set the ELN bit in their header, to
 * help the sender (or a snoop agent downstream) retransmit.
 */
void
Snoop::snoop_wless_data(Packet *p)
{
	hdr_tcp *th = hdr_tcp::access(p);
	int i, seq = th->seqno();

	if (wl_state_ & SNOOP_WLALIVE && seq == 0)
		wlreset();
	wl_state_ |= SNOOP_WLALIVE;

	if (wl_state_ & SNOOP_WLEMPTY && seq >= wl_lastAck_) {
		wlseqs_[wl_bufhead_]->seq = seq;
		wlseqs_[wl_bufhead_]->num = 1;
		wl_buftail_ = wl_bufhead_;
		wl_bufhead_ = wl_next(wl_bufhead_);
		wl_lastSeen_ = seq;
		wl_state_ &= ~SNOOP_WLEMPTY;
		return;
	}
	/* WL data list definitely not empty at this point. */
	if (seq >= wl_lastSeen_) {
		wl_lastSeen_ = seq;
		i = wl_prev(wl_bufhead_);
		if (wlseqs_[i]->seq + wlseqs_[i]->num == seq) {
			wlseqs_[i]->num++;
			return;
		}
		i = wl_bufhead_;
		wl_bufhead_ = wl_next(wl_bufhead_);
	} else if (seq == wlseqs_[i = wl_buftail_]->seq - 1) {
	} else
		return;

	wlseqs_[i]->seq = seq;
	wlseqs_[i]->num++;

	/* Ignore network out-of-ordering and retransmissions for now */
	return;
}

/*
 * Ack from wired side (for sender on "other" side of wireless link.
 */
void 
Snoop::snoop_wired_ack(Packet *p)
{
	hdr_tcp *th = hdr_tcp::access(p);
	int ack = th->seqno();
	int i;
	
	if (ack == wl_lastAck_ && snoop_wlessloss(ack)) {
		hdr_flags::access(p)->eln_ = 1;
	} else if (ack > wl_lastAck_) {
		/* update info about unack'd data */
		for (i = wl_buftail_; i != wl_bufhead_; i = wl_next(i)) {
			hdr_seq *t = wlseqs_[i];
			if (t->seq + t->num - 1 <= ack) {
				t->seq = t->num = 0;
			} else if (ack < t->seq) {
				break;
			} else if (ack < t->seq + t->num - 1) {
				/* ack for part of a block */
				t->num -= ack - t->seq +1;
				t->seq = ack + 1;
				break;
			}
		}
		wl_buftail_ = i;
		if (wl_buftail_ == wl_bufhead_)
			wl_state_ |= SNOOP_WLEMPTY;
		wl_lastAck_ = ack;
		/* Even a new ack could cause an ELN to be set. */
		if (wl_bufhead_ != wl_buftail_ && snoop_wlessloss(ack))
			hdr_flags::access(p)->eln_ = 1;
	}
}

/* 
 * Return 1 if we think this packet loss was not congestion-related, and 
 * 0 otherwise.  This function simply implements the lookup into the table
 * that maintains this info; most of the hard work is done in 
 * snoop_wless_data() and snoop_wired_ack().
 */
int
Snoop::snoop_wlessloss(int ack)
{
	if ((wl_bufhead_ == wl_buftail_) || wlseqs_[wl_buftail_]->seq > ack+1)
		return 1;
	return 0;
}

/*
 * clean snoop cache of packets that have been acked.
 */
double
Snoop::snoop_cleanbufs_(int ack)
{
	Scheduler &s = Scheduler::instance();
	double sndTime = -1;

	if (toutPending_) {
		s.cancel(toutPending_);
		// xxx: I think that toutPending_ doesn't need to be freed because snoop didn't allocate it (but I'm not sure).
		toutPending_ = 0;
	};

	if (empty_())
		return sndTime;

	int i = buftail_;
	do {
		hdr_snoop *sh = hdr_snoop::access(pkts_[i]);
		int seq = hdr_tcp::access(pkts_[i])->seqno();

		if (seq <= ack) {
			sndTime = sh->sndTime();
			Packet::free(pkts_[i]);
			pkts_[i] = 0;
			fstate_ &= ~SNOOP_FULL;	/* XXX redundant? */
		} else if (seq > ack)
			break;
		i = next(i);
	} while (i != bufhead_);

	if ((i != buftail_) || (bufhead_ != buftail_)) {
		fstate_ &= ~SNOOP_FULL;
		buftail_ = i;
	}
	if (!empty_()) {
		toutPending_ = (Event *) (pkts_[buftail_]);
		s.schedule(rxmitHandler_, toutPending_, timeout());
		hdr_snoop *sh = hdr_snoop::access(pkts_[buftail_]);
		tailTime_ = sh->sndTime();
	}

	return sndTime;
}

/* 
 * Calculate smoothed rtt estimate and linear deviation.
 */
void
Snoop::snoop_rtt(double sndTime)
{
	double rtt = Scheduler::instance().clock() - sndTime;

	if (parent_->integrate()) {
		parent_->snoop_rtt(sndTime);
		return;
	}
	
	if (rtt > 0) {
		srtt_ = g_*srtt_ + (1-g_)*rtt;
		double delta = rtt - srtt_;
		if (delta < 0)
			delta = -delta;
		if (rttvar_ != 0)
			rttvar_ = g_*delta + (1-g_)*rttvar_;
		else 
			rttvar_ = delta;
	}
}


/* 
 * Calculate smoothed rtt estimate and linear deviation.
 */
void
LLSnoop::snoop_rtt(double sndTime)
{
	double rtt = Scheduler::instance().clock() - sndTime;
	if (rtt > 0) {
		srtt_ = g_*srtt_ + (1-g_)*rtt;
		double delta = rtt - srtt_;
		if (delta < 0)
			delta = -delta;
		if (rttvar_ != 0)
			rttvar_ = g_*delta + (1-g_)*rttvar_;
		else 
			rttvar_ = delta;
	}
}

/*
 * Returns 1 if recent queue length is <= half the maximum and 0 otherwise.
 */
int 
Snoop::snoop_qlong()
{
	/* For now only instantaneous lengths */
	//	if (parent_->ifq()->length() <= 3*parent_->ifq()->limit()/4)
	
	return 1;
		//	return 0;
}

/*
 * Ideally, would like to schedule snoop retransmissions at higher priority.
 */
int
Snoop::snoop_rxmit(Packet *pkt)
{
	Scheduler& s = Scheduler::instance();
	if (pkt != 0) {
		hdr_snoop *sh = hdr_snoop::access(pkt);
		if (sh->numRxmit() < SNOOP_MAX_RXMIT && snoop_qlong()) {
			/*			&& sh->seqno() == lastAck_+1)  */
			
#if 0
			printf("%f Rxmitting packet %d\n", s.clock(), 
			       hdr_tcp::access(pkt)->seqno());
#endif
			
			// need to specify direction, in this case, down
			hdr_cmn *ch = HDR_CMN(pkt);       
			ch->direction() = hdr_cmn::DOWN;  // Ben added

			sh->sndTime() = s.clock();
			sh->numRxmit() = sh->numRxmit() + 1;
			Packet *p = pkt->copy();
			parent_->sendDown(p);
		} else 
			return SNOOP_PROPAGATE;
	}
	/* Reset timeout for later time. */
	if (toutPending_) {
		s.cancel(toutPending_);
		// xxx: I think that toutPending_ doesn't need to be freed because snoop didn't allocate it (but I'm not sure).
	};
	toutPending_ = (Event *)pkt;
	s.schedule(rxmitHandler_, toutPending_, timeout());
	return SNOOP_SUPPRESS;
}

void 
Snoop::snoop_cleanup()
{
}

void
SnoopRxmitHandler::handle(Event *)
{
	Packet *p = snoop_->pkts_[snoop_->buftail_];
	snoop_->toutPending_ = 0;
	if (p == 0)
		return;
	hdr_snoop *sh = hdr_snoop::access(p);
	if (sh->seqno() != snoop_->lastAck_ + 1)
		return;
	if ((snoop_->bufhead_ != snoop_->buftail_) || 
	    (snoop_->fstate_ & SNOOP_FULL)) {
		//		printf("%f Snoop timeout\n", Scheduler::instance().clock());
		if (snoop_->snoop_rxmit(p) == SNOOP_SUPPRESS)
			snoop_->expNextAck_ = snoop_->next(snoop_->buftail_);
	}
}



