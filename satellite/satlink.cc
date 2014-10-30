/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1999 Regents of the University of California.
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
 *      This product includes software developed by the MASH Research
 *      Group at the University of California Berkeley.
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
 *
 * Contributed by Tom Henderson, UCB Daedalus Research Group, June 1999
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/satellite/satlink.cc,v 1.14 2005/09/21 21:45:04 haldar Exp $";
#endif

/*
 * Contains source code for:
 *	SatLinkHead
 *  	SatLL
 * 	SatMac
 * 	SatPhy
 * 	SatChannel
 */

#include "satlink.h"
#include "sattrace.h"
#include "satposition.h"
#include "satgeometry.h"
#include "satnode.h"
#include "satroute.h"
#include "errmodel.h"
#include "sat-hdlc.h"

/*==========================================================================*/
/*
 * _SatLinkHead
 */

static class SatLinkHeadClass : public TclClass {
public:
	SatLinkHeadClass() : TclClass("Connector/LinkHead/Sat") {}
	TclObject* create(int, const char*const*) {
		return (new SatLinkHead);
	}
} class_sat_link_head;

SatLinkHead::SatLinkHead() : linkup_(1), phy_tx_(0), phy_rx_(0), mac_(0), satll_(0), queue_(0), errmodel_(0)
{
}

int SatLinkHead::command(int argc, const char*const* argv)
{
	if (argc == 2) {
	} else if (argc == 3) {
		if (strcmp(argv[1], "set_type") == 0) {
			if (strcmp(argv[2], "geo") == 0) {
				type_ = LINK_GSL_GEO;
				return TCL_OK;
			} else if (strcmp(argv[2], "polar") == 0) {
				type_ = LINK_GSL_POLAR;
				return TCL_OK;
			} else if (strcmp(argv[2], "gsl") == 0) {
				type_ = LINK_GSL;
				return TCL_OK;
			} else if (strcmp(argv[2], "gsl-repeater") == 0) {
				type_ = LINK_GSL_REPEATER;
				return TCL_OK;
			} else if (strcmp(argv[2], "interplane") == 0) {
				type_ = LINK_ISL_INTERPLANE;
				return TCL_OK;
			} else if (strcmp(argv[2], "intraplane") == 0) {
				type_ = LINK_ISL_INTRAPLANE;
				return TCL_OK;
			} else if (strcmp(argv[2], "crossseam") == 0) {
				type_ = LINK_ISL_CROSSSEAM;
				return TCL_OK;
			} else {
				printf("Unknown link type: %s\n", argv[2]);
				exit(1);
			} 
		}
		if (strcmp(argv[1], "setll") == 0) {
			satll_ = (SatLL*) TclObject::lookup(argv[2]);
			if (satll_ == 0)
				return TCL_ERROR;
			return TCL_OK;
		} else if(strcmp(argv[1], "setphytx") == 0) {
			phy_tx_ = (SatPhy*) TclObject::lookup(argv[2]);
			if (phy_tx_ == 0)
				return TCL_ERROR;
			return TCL_OK;
		} else if(strcmp(argv[1], "setphyrx") == 0) {
			phy_rx_ = (SatPhy*) TclObject::lookup(argv[2]);
			if (phy_rx_ == 0)
				return TCL_ERROR;
			return TCL_OK;
		} else if(strcmp(argv[1], "setmac") == 0) {
			mac_ = (SatMac*) TclObject::lookup(argv[2]);
			if (mac_ == 0)
				return TCL_ERROR;
			return TCL_OK;
		} else if(strcmp(argv[1], "setqueue") == 0) {
			queue_ = (Queue*) TclObject::lookup(argv[2]);
			if (queue_ == 0)
				return TCL_ERROR;
			return TCL_OK;
		} else if(strcmp(argv[1], "seterrmodel") == 0) {
			errmodel_ = (ErrorModel*) TclObject::lookup(argv[2]);
			if (errmodel_ == 0)
				return TCL_ERROR;
			return TCL_OK;
		}
	}
	return (LinkHead::command(argc, argv));
}

/*==========================================================================*/
/*
 * _SatLL
 */

static class SatLLClass : public TclClass {
public:
	SatLLClass() : TclClass("LL/Sat") {}
	TclObject* create(int, const char*const*) {
		return (new SatLL);
	}
} sat_class_ll;


void SatLL::recv(Packet* p, Handler* /*h*/)
{
	hdr_cmn *ch = HDR_CMN(p);
	
	/*
	 * Sanity Check
	 */
	assert(initialized());
	
	// If direction = UP, then pass it up the stack
	// Otherwise, set direction to DOWN and pass it down the stack
	if(ch->direction() == hdr_cmn::UP) {
		uptarget_ ? sendUp(p) : drop(p);
		return;
	}

	ch->direction() = hdr_cmn::DOWN;
	sendDown(p);
}
int SatLL::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "setnode") == 0) {
			satnode_ = (SatNode*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
	}
	return LL::command(argc, argv);
}

int SatLL::getRoute(Packet *p)
{
	hdr_cmn *ch = HDR_CMN(p);
        // wired-satellite integration
	if (SatRouteObject::instance().wiredRouting()) {
		
		hdr_ip *ip = HDR_IP(p);
		RouteLogic *routelogic_;
		int next_hopIP = -1; // Initialize in case route not found
		int myaddr_;
		// Wired/satellite integration
		// We need to make sure packet headers are set correctly
		// This code adapted from virtual-classifier.cc
		
		Tcl &tcl = Tcl::instance();
		tcl.evalc("[Simulator instance] get-routelogic");
		routelogic_ = (RouteLogic*) TclObject::lookup(tcl.result());
		char* adst = Address::instance().print_nodeaddr(ip->daddr());
		myaddr_ = satnode()->ragent()->myaddr();
		//char* asrc = Address::instance().print_nodeaddr(h->saddr());
		char* asrc = Address::instance().print_nodeaddr(myaddr_);
		routelogic_->lookup_flat(asrc, adst, next_hopIP);
		delete [] adst;
		delete [] asrc;
		// The following fields are usually set by routeagent
		// forwardPacket() in satroute.cc (when wiredRouting_ == 0)
		ch->next_hop_ = next_hopIP;
		if (satnode()) {
			ch->last_hop_ = satnode()->ragent()->myaddr();
		} else {
			printf("Error:  LL has no satnode_ pointer set\n");
			exit(1);
		}
        }
	// else (if no wired rtg) next-hop field is populated by rtg agent
	
	return ch->next_hop_;
	
}


// Encode link layer sequence number, type, and mac address fields
void SatLL::sendDown(Packet* p)
{	
	hdr_cmn *ch = HDR_CMN(p);
	hdr_ll *llh = HDR_LL(p);
	
	char *mh = (char*)p->access(hdr_mac::offset_);
	int peer_mac_;
	SatChannel* satchannel_;

	llh->seqno_ = ++seqno_;
	llh->lltype() = LL_DATA;

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

void SatLL::sendUp(Packet* p)
{
	Scheduler& s = Scheduler::instance();
	if (hdr_cmn::access(p)->error() > 0)
		drop(p);
	else
		s.schedule(uptarget_, p, delay_);
}

// Helper function 
Channel* SatLL::channel()
{
	Phy* phy_ = (Phy*) mac_->downtarget();
	return phy_->channel();
}

/*==========================================================================*/
/*
 * _SatMac
 */

static class SatMacClass : public TclClass {
public:
	SatMacClass() : TclClass("Mac/Sat") {}
	TclObject* create(int, const char*const*) {
		return (new SatMac);
	}
} sat_class_mac;

void MacSendTimer::expire(Event*)
{
        a_->send_timer();
}

void MacRecvTimer::expire(Event*)
{
        a_->recv_timer();
}

SatMac::SatMac() : Mac(), send_timer_(this), recv_timer_(this)
{
	bind_bool("trace_collisions_", &trace_collisions_);
	bind_bool("trace_drops_", &trace_drops_);
}

int SatMac::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if(argc == 2) {
	}
	else if (argc == 3) {
		TclObject *obj;
		if( (obj = TclObject::lookup(argv[2])) == 0) {
			fprintf(stderr, "%s lookup failed\n", argv[1]);
			return TCL_ERROR;                
		}
		if (strcmp(argv[1], "channel") == 0) {
			//channel_ = (Channel*) obj;
			return (TCL_OK);
		}
		if (strcmp(argv[1], "set_drop_trace") == 0) {
			drop_trace_ = (SatTrace *) TclObject::lookup(argv[2]);
			if (drop_trace_ == 0) {
				tcl.resultf("no such object %s", argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "set_coll_trace") == 0) {
			coll_trace_ = (SatTrace *) TclObject::lookup(argv[2]);
			if (coll_trace_ == 0) {
				tcl.resultf("no such object %s", argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
	}
	return Mac::command(argc, argv);
}

void SatMac::sendUp(Packet* p) 
{
	hdr_mac* mh = HDR_MAC(p);
	int dst = this->hdr_dst((char*)mh); // mac destination address
	
	if (((u_int32_t)dst != MAC_BROADCAST) && (dst != index_)) {
		drop(p);
		return;
	}
	// First bit of packet has arrived-- wait for 
	// (txtime + delay_) before sending up
	Scheduler::instance().schedule(uptarget_, p, delay_ + mh->txtime());
}



void SatMac::sendDown(Packet* p)
{
	Scheduler& s = Scheduler::instance();
	double txt;
	// LINK_HDRSIZE is defined in satlink.h.  This is the size of header
	// information for all layers below IP.  Alternatively, one could
	// derive this information dynamically from packet headers. 
	int packetsize_ = HDR_CMN(p)->size() + LINK_HDRSIZE;
	assert (bandwidth_ != 0);
	txt = txtime(packetsize_);
	// For convenience, we encode the transmit time in the Mac header
	// The packet will be held (for collision detection) for txtime 
	// at the receiving mac.
        HDR_MAC(p)->txtime() = txt;
	downtarget_->recv(p, this);
	// Callback for when this packet's transmission will be done
	s.schedule(&hRes_, &intr_, txt);
}

static class UnslottedAlohaMacClass : public TclClass {
public:
	UnslottedAlohaMacClass() : TclClass("Mac/Sat/UnslottedAloha") {}
	TclObject* create(int, const char*const*) {
		return (new UnslottedAlohaMac());
	}
} sat_class_unslottedalohamac;

/*==========================================================================*/
/*
 * _UnslottedAlohaMac
 */

UnslottedAlohaMac::UnslottedAlohaMac() : SatMac(), tx_state_(MAC_IDLE), 
    rx_state_(MAC_IDLE), rtx_(0), end_of_contention_(0) 
{
	bind_time("mean_backoff_", &mean_backoff_);
	bind("rtx_limit_", &rtx_limit_);
	bind_time("send_timeout_", &send_timeout_);
}

void UnslottedAlohaMac::send_timer() 
{
	switch (tx_state_) {
	
	case MAC_SEND:
		// We've timed out on send-- back off
		backoff();
		break;
	case MAC_COLL:
		// Our backoff timer has expired-- resend
		sendDown(snd_pkt_);
		break;
	default:
		printf("Error: wrong tx_state in unslotted aloha: %d\n",
		    tx_state_);
		break;
	}
}

void UnslottedAlohaMac::recv_timer() 
{
	switch (rx_state_) {

	case MAC_RECV:
		// We've successfully waited out the reception
		end_of_contention(rcv_pkt_);
		break;
	default:
		printf("Error: wrong rx_state in unslotted aloha: %d\n",
		    rx_state_);
		break;
	}
	
}

void UnslottedAlohaMac::sendUp(Packet* p) 
{
	hdr_mac* mh = HDR_MAC(p);
	int dst;
	
	if (rx_state_ == MAC_IDLE) {
		// First bit of packet has arrived-- wait for 
		// txtime to make sure no collisions occur 
		rcv_pkt_ = p;
		end_of_contention_ = NOW + mh->txtime();
		rx_state_ = MAC_RECV;
		recv_timer_.resched(mh->txtime());
	} else {
		// Collision: figure out if contention phase must be lengthened
		if ( (NOW + mh->txtime()) > end_of_contention_ ) {
			recv_timer_.resched(mh->txtime());
		}
		// If this is the first collision, we will also have a
		// rcv_pkt_ pending
		if (rcv_pkt_) {
			// Before dropping rcv_pkt_, trace the collision
			// if it was intended for us
			mh = HDR_MAC(rcv_pkt_);
			dst = this->hdr_dst((char*)mh); // mac dest. address
			if (((u_int32_t)dst == MAC_BROADCAST)||(dst == index_))
				if (coll_trace_ && trace_collisions_)
					coll_trace_->traceonly(rcv_pkt_);
			drop(rcv_pkt_);
		}
		rcv_pkt_ = 0;
		// Again, before we drop this packet, log a collision if
		// it was intended for us
		mh = HDR_MAC(p);
		dst = this->hdr_dst((char*)mh); // mac destination address
		if (((u_int32_t)dst == MAC_BROADCAST) || (dst == index_))
			if (coll_trace_ && trace_collisions_)
				coll_trace_->traceonly(p);
		drop(p);
	}
}

void UnslottedAlohaMac::sendDown(Packet* p)
{
	double txt;
	
	// compute transmission delay:
	int packetsize_ = HDR_CMN(p)->size() + LINK_HDRSIZE;
	assert (bandwidth_ != 0);
	txt = txtime(packetsize_);
        HDR_MAC(p)->txtime() = txt;

	// Send the packet down 
	tx_state_ = MAC_SEND;
	snd_pkt_ = p->copy();  // save a copy in case it gets retransmitted
	downtarget_->recv(p, this);

	// Set a timer-- if we do not hear our own transmission within this
	// interval (and cancel the timer), the send_timer will expire and
	// we will backoff and retransmit.
	send_timer_.resched(send_timeout_ + txt);
}

// Called when contention period ends
void UnslottedAlohaMac::end_of_contention(Packet* p) 
{
	rx_state_ = MAC_IDLE;
	if (!p)  
		return; // No packet to free or send up.

	hdr_mac* mh = HDR_MAC(p);
	int dst = this->hdr_dst((char*)mh); // mac destination address
	int src = this->hdr_src((char*)mh); // mac source address
	
	if (((u_int32_t)dst != MAC_BROADCAST) && (dst != index_) && 
    	    (src != index_)) {
		drop(p); // Packet not intended for our station
		return;
	} 
	if (src == index_) {
		// received our own packet: free up transmit side, drop this
		// packet, and perform callback to queue which is blocked
		if (!callback_) {
			printf("Error, queue callback_ is not valid\n");
			exit(1);
		}
		send_timer_.force_cancel();
		tx_state_ = MAC_IDLE;
		rtx_ = 0;
		drop(snd_pkt_); // Free the packet cached for retransmission
		resume(p);
	} else {
		// wait for processing delay (delay_) to send packet upwards 
		Scheduler::instance().schedule(uptarget_, p, delay_);
	}
}

void UnslottedAlohaMac::backoff(double delay)
{
	double backoff_ = Random::exponential(mean_backoff_);

	// if number of retransmissions is within limit, do exponential backoff
	// else drop the packet and resume
	if (++rtx_ <= rtx_limit_) {
		tx_state_ = MAC_COLL;
		delay += backoff_;
		send_timer_.resched(delay);
	} else {
		tx_state_ = MAC_IDLE;
		rtx_ = 0;
		// trace the dropped packet
		if (drop_trace_ && trace_drops_)
			drop_trace_->traceonly(snd_pkt_);
		resume(snd_pkt_);
	}
}


/*==========================================================================*/
/*
 * _SatPhy
 */

static class SatPhyClass: public TclClass {
public:
	SatPhyClass() : TclClass("Phy/Sat") {}
	TclObject* create(int, const char*const*) {
		return (new SatPhy);
	}
} class_SatPhy;

void SatPhy::sendDown(Packet *p)
{
	if (channel_)
		channel_->recv(p, this);
	else {
		// it is possible for routing to change (and a channel to
		// be disconnected) while a packet
		// is moving down the stack.  Therefore, just log a drop
		// if there is no channel
		if ( ((SatNode*) head()->node())->trace() )
			((SatNode*) head()->node())->trace()->traceonly(p);
		Packet::free(p);
	}
}

// Note that this doesn't do that much right now.  If you want to incorporate
// an error model, you could insert a "propagation" object like in the
// wireless case.
int SatPhy::sendUp(Packet * /* pkt */)
{
	return TRUE;
}

int
SatPhy::command(int argc, const char*const* argv) {
	if (argc == 2) {
	} else if (argc == 3) {
		TclObject *obj;

		if( (obj = TclObject::lookup(argv[2])) == 0) {
			fprintf(stderr, "%s lookup failed\n", argv[1]);
			return TCL_ERROR;
		}
	}
	return Phy::command(argc, argv);
}

static class RepeaterPhyClass: public TclClass {
public:
	RepeaterPhyClass() : TclClass("Phy/Repeater") {}
	TclObject* create(int, const char*const*) {
		return (new RepeaterPhy);
	}
} class_RepeaterPhy;

void RepeaterPhy::recv(Packet* p, Handler*)
{
	struct hdr_cmn *hdr = HDR_CMN(p);
	if (hdr->direction() == hdr_cmn::UP) {
		// change direction and send to uptarget (which is
		// really a Phy_tx that is also a RepeaterPhy)
		hdr->direction() = hdr_cmn::DOWN;
		uptarget_->recv(p, (Handler*) 0);
	} else {
		sendDown(p);
	}
}

void RepeaterPhy::sendDown(Packet *p)
{
	struct hdr_cmn *hdr = HDR_CMN(p);
	hdr->direction() =  hdr_cmn::DOWN;

	if (channel_)
		channel_->recv(p, this);
	else {
		printf("Error, no channel on repeater\n");
		exit(1);
	}
}

/*==========================================================================*/
/*
 * _SatChannel
 */

static class SatChannelClass : public TclClass {
public:
	SatChannelClass() : TclClass("Channel/Sat") {}
	TclObject* create(int, const char*const*) {
		return (new SatChannel);
	}
} class_Sat_channel;

SatChannel::SatChannel(void) : Channel() {
}

double
SatChannel::get_pdelay(Node* tnode, Node* rnode)
{
	coordinate a = ((SatNode*)tnode)->position()->coord();
	coordinate b = ((SatNode*)rnode)->position()->coord();
	return (SatGeometry::propdelay(a, b));
}

// This is a helper function that attaches a SatChannel to a Phy
void SatChannel::add_interface(Phy* phy_)
{
	phy_->setchnl(this); // Attach phy to this channel
	phy_->insertchnl(&ifhead_); // Add phy_ to list of phys on the channel
}

// Remove a phy from a channel
void SatChannel::remove_interface(Phy* phy_)
{
	phy_->setchnl(NULL); // Set phy_'s channel pointer to NULL
	phy_->removechnl(); // Remove phy_ to list of phys on the channel
}

// Search for destination mac address on this channel.  Look through list
// of phys on the channel.  If the channel connects to a geo repeater, look
// for the destination on the corresponding downlink channel.  
int SatChannel::find_peer_mac_addr(int dst)
{
	Phy *n;
	Channel* chan_;
	chan_ = this;
	n = ifhead_.lh_first; 
	if (n->head()->type() == LINK_GSL_REPEATER) {
		SatLinkHead* slh = (SatLinkHead*) n->head();
		chan_ = slh->phy_tx()->channel();
	}
	for(n = chan_->ifhead_.lh_first; n; n = n->nextchnl() ) {
		if (n->node()->address() == dst) {
			return (((SatMac*) n->uptarget())->addr());
		}
	}
	return -1;
}


