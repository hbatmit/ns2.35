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
 * 4. Neither the name of the University nor of the research group may be used
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
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/mac/mac-multihop.cc,v 1.14 2000/11/02 22:46:37 johnh Exp $ (UCB)";
#endif

#include "template.h"
#include "channel.h"
#include "mac-multihop.h"

/* 
 * For debugging.
 */
void dump_iphdr(hdr_ip *iph) 
{
        printf("\tsrc = %d, ", iph->saddr());
        printf("\tdst = %d\n", iph->daddr());
}       

static class MultihopMacClass : public TclClass {
public:
	MultihopMacClass() : TclClass("Mac/Multihop") {}
	TclObject* create(int, const char*const*) {
		return (new MultihopMac);
	}
} class_mac_multihop;

MultihopMac::MultihopMac() : mode_(MAC_IDLE), peer_(0),
	pendingPollEvent_(0), pkt_(0),
	ph_(this), pah_(this), pnh_(this), pth_(this), bh_(this)
{
	/* Bind a bunch of variables to access from Tcl */
	bind_time("tx_rx_", &tx_rx_);
	bind_time("rx_tx_", &rx_tx_);
	bind_time("rx_rx_", &rx_rx_);
	bind_time("backoffBase_", &backoffBase_);
	backoffTime_ = backoffBase_;
}

/*
 * Returns 1 iff the specified MAC is in the prescribed state, AND all
 * the other MACs are IDLE.
 */
int MultihopMac::checkInterfaces(int state) 
{
	MultihopMac *p;
	
	if (!(mode_ & state))
		return 0;
	else if (macList_ == 0)
		return 1;
	for (p = (MultihopMac *)macList_; p != this && p != NULL; 
	     p = (MultihopMac *)(p->macList())) {
		if (p->mode() != MAC_IDLE) {
			return 0;
		}
	}
	return 1;
}

/*
 * Poll a peer node prior to a send.  There can be at most one POLL 
 * outstanding from a node at any point in time.  This is achieved implicitly
 * because there can be at most one packet down from LL (thru IFQ) to this MAC.
 */
void MultihopMac::poll(Packet *p)
{
	Scheduler& s = Scheduler::instance();
	MultihopMac *pm = (MultihopMac*) getPeerMac(p);
	PollEvent *pe = new PollEvent(pm, this);

	pendingPollEvent_ = new PollEvent(pm, this);
	pkt_ = p->copy();	/* local copy for poll retries */
	double timeout = max(pm->rx_tx(), tx_rx_) + 4*pollTxtime(MAC_POLLSIZE);
	s.schedule(&bh_, pendingPollEvent_, timeout);

	/*  If the other interfaces are idle, then go ahead, else not. */
	if (checkInterfaces(MAC_IDLE)) { 
		mode_ = MAC_POLLING;
		peer_ = pm;
		s.schedule(pm->ph(), (Event *)pe, pollTxtime(MAC_POLLSIZE));
	}
}

/*
 * Handle a POLL request from a peer node's MAC.
 */
void
PollHandler::handle(Event *e)
{
	PollEvent *pe = (PollEvent *) e;
	Scheduler& s = Scheduler::instance();
	MultihopMac* pm = mac_->peer(); /* sensible val only in MAC_RCV mode */
	
	/*
	 * Send POLLACK if either IDLE or currently receiving 
	 * from same mac as the poller.
	 */
	if (mac_->checkInterfaces(MAC_IDLE)) { // all interfaces must be IDLE
		mac_->mode(MAC_RCV);
		pm = pe->peerMac();
		mac_->peer(pm);
		PollEvent *pae = new PollEvent(pm, mac_); // POLLACK event
		double t = mac_->pollTxtime(MAC_POLLACKSIZE) + 
			max(mac_->tx_rx(), pm->rx_tx());
		s.schedule(pm->pah(), pae, t);
	} else {
		// printf("ignoring poll %d\n", mac_->label());
		// could send NACKPOLL but don't (at least for now)
	}
}

/*
 * Handle a POLLACK from a peer node's MAC.
 */
void
PollAckHandler::handle(Event *e)
{
	PollEvent *pe = (PollEvent *) e;
	Scheduler& s = Scheduler::instance();

	if (mac_->checkInterfaces(MAC_POLLING | MAC_IDLE)) {
		mac_->backoffTime(mac_->backoffBase());
		mac_->mode(MAC_SND);
		mac_->peer(pe->peerMac());
		s.cancel(mac_->pendingPE()); /* cancel pending timeout */
		free(mac_->pendingPE());  // and free the event
		mac_->pendingPE(NULL);
		mac_->send(mac_->pkt()); /* send saved packet */
	}
}

void
PollNackHandler::handle(Event *)
{
}

void
BackoffHandler::handle(Event *)
{
	Scheduler& s = Scheduler::instance();
	if (mac_->mode() == MAC_POLLING) 
		mac_->mode(MAC_IDLE);
	double bTime = mac_->backoffTime(2*mac_->backoffTime());
	bTime = (1+Random::integer(MAC_TICK)*1./MAC_TICK)*bTime + 
		2*mac_->backoffBase();

//	printf("backing off %d\n", mac_->label());
	s.schedule(mac_->pth(), mac_->pendingPE(), bTime);
}

void 
PollTimeoutHandler::handle(Event *)
{
	mac_->poll(mac_->pkt());
}


/*
 * Actually send the data frame.
 */
void MultihopMac::send(Packet *p)
{
	Scheduler& s = Scheduler::instance();
	if (mode_ != MAC_SND)
		return;

	double txt = txtime(p);
	hdr_mac::access(p)->txtime() = txt;
	channel_->send(p, txt); // target is peer's mac handler
	s.schedule(callback_, &intr_, txt); // callback to higher layer (LL)
	mode_ = MAC_IDLE;
}

/*
 * This is the call from the higher layer of the protocol stack (i.e., LL)
 */
void MultihopMac::recv(Packet* p, Handler *h)
{
	if (h == 0) {		/* from MAC classifier (pass pkt to LL) */
		mode_ = MAC_IDLE;
		Scheduler::instance().schedule(target_, p, delay_);
		return;
	}
	callback_ = h;
	hdr_mac* mh = hdr_mac::access(p);
	mh->macSA() = addr_;
	if (mh->ftype() == MF_ACK) {
		mode_ = MAC_SND;
		send(p);
	} else {
		mh->ftype() = MF_DATA;
		poll(p);		/* poll first */
	}
}
