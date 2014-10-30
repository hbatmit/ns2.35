
/*
 * mac-simple.cc
 * Copyright (C) 2003 by the University of Southern California
 * $Id: mac-simple.cc,v 1.8 2010/03/08 05:54:52 tom_henderson Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */

#include "ll.h"
#include "mac.h"
#include "mac-simple.h"
#include "random.h"

// Added by Sushmita to support event tracing (singal@nunki.usc.edu)
#include "agent.h"
#include "basetrace.h"

#include "cmu-trace.h"

static class MacSimpleClass : public TclClass {
public:
	MacSimpleClass() : TclClass("Mac/Simple") {}
	TclObject* create(int, const char*const*) {
		return new MacSimple();
	}
} class_macsimple;


// Added by Sushmita to support event tracing (singal@nunki.usc.edu).
void MacSimple::trace_event(char *eventtype, Packet *p)
{
	if (et_ == NULL) return;
	char *wrk = et_->buffer();
	char *nwrk = et_->nbuffer();

	hdr_ip *iph = hdr_ip::access(p);
	char *src_nodeaddr =
		Address::instance().print_nodeaddr(iph->saddr());
	char *dst_nodeaddr =
		Address::instance().print_nodeaddr(iph->daddr());

	if (wrk != 0) 
	{
		sprintf(wrk, "E -t "TIME_FORMAT" %s %s %s",
			et_->round(Scheduler::instance().clock()),
			eventtype,
			src_nodeaddr,
			dst_nodeaddr);
	}
	if (nwrk != 0)
	{
		sprintf(nwrk, "E -t "TIME_FORMAT" %s %s %s",
		et_->round(Scheduler::instance().clock()),
		eventtype,
		src_nodeaddr,
		dst_nodeaddr);
	}
	et_->dump();
}

MacSimple::MacSimple() : Mac() {
	rx_state_ = tx_state_ = MAC_IDLE;
	tx_active_ = 0;
	waitTimer = new MacSimpleWaitTimer(this);
	sendTimer = new MacSimpleSendTimer(this);
	recvTimer = new MacSimpleRecvTimer(this);
	// Added by Sushmita to support event tracing (singal@nunki.usc.edu)
	et_ = new EventTrace();
	busy_ = 0;
	bind("fullduplex_mode_", &fullduplex_mode_);
}

// Added by Sushmita to support event tracing (singal@nunki.usc.edu)
int 
MacSimple::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if(strcmp(argv[1], "eventtrace") == 0) {
			et_ = (EventTrace *)TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
	}
	return Mac::command(argc, argv);
}

void MacSimple::recv(Packet *p, Handler *h) {

	struct hdr_cmn *hdr = HDR_CMN(p);
	/* let MacSimple::send handle the outgoing packets */
	if (hdr->direction() == hdr_cmn::DOWN) {
		send(p,h);
		return;
	}

	/* handle an incoming packet */

	/*
	 * If we are transmitting, then set the error bit in the packet
	 * so that it will be thrown away
	 */
	
	// in full duplex mode it can recv and send at the same time
	if (!fullduplex_mode_ && tx_active_)
	{
		hdr->error() = 1;

	}

	/*
	 * check to see if we're already receiving a different packet
	 */
	
	if (rx_state_ == MAC_IDLE) {
		/*
		 * We aren't already receiving any packets, so go ahead
		 * and try to receive this one.
		 */
		rx_state_ = MAC_RECV;
		pktRx_ = p;
		/* schedule reception of the packet */
		recvTimer->start(txtime(p));
	} else {
		/*
		 * We are receiving a different packet, so decide whether
		 * the new packet's power is high enough to notice it.
		 */
		if (pktRx_->txinfo_.RxPr / p->txinfo_.RxPr
			>= p->txinfo_.CPThresh) {
			/* power too low, ignore the packet */
			Packet::free(p);
		} else {
			/* power is high enough to result in collision */
			rx_state_ = MAC_COLL;

			/*
			 * look at the length of each packet and update the
			 * timer if necessary
			 */

			if (txtime(p) > recvTimer->expire()) {
				recvTimer->stop();
				Packet::free(pktRx_);
				pktRx_ = p;
				recvTimer->start(txtime(pktRx_));
			} else {
				Packet::free(p);
			}
		}
	}
}


double
MacSimple::txtime(Packet *p)
 {
	 struct hdr_cmn *ch = HDR_CMN(p);
	 double t = ch->txtime();
	 if (t < 0.0)
	 	t = 0.0;
	 return t;
 }



void MacSimple::send(Packet *p, Handler *h)
{
	hdr_cmn* ch = HDR_CMN(p);

	/* store data tx time */
 	ch->txtime() = Mac::txtime(ch->size());

	// Added by Sushmita to support event tracing (singal@nunki.usc.edu)
	trace_event("SENSING_CARRIER",p);

	/* check whether we're idle */
	if (tx_state_ != MAC_IDLE) {
		// already transmitting another packet .. drop this one
		// Note that this normally won't happen due to the queue
		// between the LL and the MAC .. the queue won't send us
		// another packet until we call its handler in sendHandler()

		Packet::free(p);
		return;
	}

	pktTx_ = p;
	txHandler_ = h;
	// rather than sending packets out immediately, add in some
	// jitter to reduce chance of unnecessary collisions
	double jitter = Random::random()%40 * 100/bandwidth_;

	if(rx_state_ != MAC_IDLE) {
		trace_event("BACKING_OFF",p);
	}

	if (rx_state_ == MAC_IDLE ) {
		// we're idle, so start sending now
		waitTimer->restart(jitter);
		sendTimer->restart(jitter + ch->txtime());
	} else {
		// we're currently receiving, so schedule it after
		// we finish receiving
		waitTimer->restart(jitter);
		sendTimer->restart(jitter + ch->txtime()
				 + HDR_CMN(pktRx_)->txtime());
	}
}


void MacSimple::recvHandler()
{
	hdr_cmn *ch = HDR_CMN(pktRx_);
	Packet* p = pktRx_;
	MacState state = rx_state_;
	pktRx_ = 0;
	int dst = hdr_dst((char*)HDR_MAC(p));
	
	//busy_ = 0;

	rx_state_ = MAC_IDLE;

	// in full duplex mode we can send and recv at the same time
	// as different chanels are used for tx and rx'ing
	if (!fullduplex_mode_ && tx_active_) {
		// we are currently sending, so discard packet
		Packet::free(p);
	} else if (state == MAC_COLL) {
		// recv collision, so discard the packet
		drop(p, DROP_MAC_COLLISION);
		//Packet::free(p);
	} else if (dst != index_ && (u_int32_t)dst != MAC_BROADCAST) {
		
		/*  address filtering
		 *  We don't want to log this event, so we just free
		 *  the packet instead of calling the drop routine.
		 */
		Packet::free(p);
	} else if (ch->error()) {
		// packet has errors, so discard it
		//Packet::free(p);
		drop(p, DROP_MAC_PACKET_ERROR);
	
	} else {
		uptarget_->recv(p, (Handler*) 0);
	}
}

void MacSimple::waitHandler()
{
	tx_state_ = MAC_SEND;
	tx_active_ = 1;

	downtarget_->recv(pktTx_, txHandler_);
}

void MacSimple::sendHandler()
{
	Handler *h = txHandler_;
	Packet *p = pktTx_;

	pktTx_ = 0;
	txHandler_ = 0;
	tx_state_ = MAC_IDLE;
	tx_active_ = 0;

	//busy_ = 1;
	//busy_ = 0;
	
	
	// I have to let the guy above me know I'm done with the packet
	h->handle(p);
}




//  Timers

void MacSimpleTimer::restart(double time)
{
	if (busy_)
		stop();
	start(time);
}

	

void MacSimpleTimer::start(double time)
{
	Scheduler &s = Scheduler::instance();

	assert(busy_ == 0);
	
	busy_ = 1;
	stime = s.clock();
	rtime = time;
	assert(rtime >= 0.0);

	s.schedule(this, &intr, rtime);
}

void MacSimpleTimer::stop(void)
{
	Scheduler &s = Scheduler::instance();

	assert(busy_);
	s.cancel(&intr);
	
	busy_ = 0;
	stime = rtime = 0.0;
}


void MacSimpleWaitTimer::handle(Event *)
{
	busy_ = 0;
	stime = rtime = 0.0;

	mac->waitHandler();
}

void MacSimpleSendTimer::handle(Event *)
{
	busy_ = 0;
	stime = rtime = 0.0;

	mac->sendHandler();
}

void MacSimpleRecvTimer::handle(Event *)
{
	busy_ = 0;
	stime = rtime = 0.0;

	mac->recvHandler();
}


