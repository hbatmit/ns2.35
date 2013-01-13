/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 The Regents of the University of California.
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
 * 	Group at the University of California at Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be used
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
 * ack-recons.cc: contributed by the Daedalus Research Group, 
 * UC Berkeley (http://daedalus.cs.berkeley.edu).
 *
 * $Header: /cvsroot/nsnam/ns-2/tcp/ack-recons.cc,v 1.6 2000/09/01 03:04:05 haoboy Exp $
 */

/*
 * TCP Ack reconstructor.  This object sits on the other end of a constrained 
 * link and intersperses TCP acks to the source (without violating the e2e 
 * semantics of TCP acks).  This allows us to get good performance for TCP 
 * over various asymmetric networks, in conjunction with techniques to reduce 
 * the frequency of acks (such as ack filtering) with making any changes to 
 * the TCP source (e.g., like those implemented in tcp-asym.cc).  
 */

#include "template.h"
#include "ack-recons.h"

static class AckReconsControllerClass : public TclClass {
public:
	AckReconsControllerClass() : TclClass("AckReconsControllerClass") { }
	TclObject* create(int, const char*const*) {
		return (new AckReconsController);
	}
} class_ackrecons_controller;

static class AckReconsClass : public TclClass {
public:
	AckReconsClass() : TclClass("Agent/AckReconsClass") { }
	TclObject* create(int, const char*const* argv) {
		return new AckRecons(atoi(argv[4]), atoi(argv[5]));
	}
} class_ackrecons;

/*
 * Demux a packet to the right ack reconstructor.
 */
void
AckReconsController::recv(Packet *p, Handler *)
{
	Tcl& tcl = Tcl::instance();
	hdr_ip *ip = hdr_ip::access(p);
	tcl.evalf("%s demux %d %d", name(),
		  ip->saddr(), ip->daddr());
	AckRecons *ackRecons = 
		(AckRecons *) TclObject::lookup(tcl.result());
	if (ackRecons == NULL) {
		printf("Error: malformed ack reconstructor\n");
		abort();
	}
	ackRecons->spq_ = spq_;
	ackRecons->recv(p);
}

void 
AckRecons::recv(Packet *pkt)
{
	double now = Scheduler::instance().clock();
	hdr_tcp *tcph = hdr_tcp::access(pkt);
	int &ack = tcph->seqno(), a, i;
	Tcl& tcl = Tcl::instance();
#ifdef DEBUG
	printf("%f\tRecd ack %d\n", now, ack);
#endif
	if (ackTemplate_ == 0)
		ackTemplate_ = pkt->copy();
	if (adaptive_)
		tcl.evalf("%s ackbw %d %f\n", name(), ack, now);
	/* The ack spacing policy is implemented in Tcl for flexibility */
	tcl.evalf("%s spacing %d\n", name(), ack);
	/* 
	 * If the difference in acks is less than a threshold, let
	 * it go through.  Later, we will look for rapid ack arrivals
	 * to smooth them out and avoid the adverse effects of ack comp.
	 */
	if ((!ackPending_ && ack-lastAck_ <= deltaAckThresh_) || dupacks_) {
		if (ack == lastRealAck_)
			dupacks_++;
		else if (ack > lastAck_) {
			dupacks_ = 0;
			lastAck_ = ack;
			lastTime_ = now;
		}
		spq_->reconsAcks_ = 0;
		spq_->enque(pkt);
		spq_->reconsAcks_ = 1;
#ifdef DEBUG
		printf("\t%f\tEnqueuing ack %d in order\n", now, ack);
#endif
	} else {
		if (ack == lastRealAck_)
			dupacks_++;
		/* Intersperse some acks and schedule their transmissions. */
		double starttime = max(now, lastTime_);
		for (a = lastAck_+delack_, i=0; a <= ack; a += delack_, i++)
			sendack(a, starttime + i*ackSpacing_ - now);
		if ((a-ack)%delack_)
			sendack(ack, starttime + i*ackSpacing_ - now);
		Packet::free(pkt);
	}
	if (ack >= lastRealAck_) {
		lastRealAck_ = ack;
		lastRealTime_ = now;
	}
}

int
AckRecons::command(int argc, const char*const* argv)
{
	return Agent::command(argc, argv);
}

/*
 * Arrange to send ack a at time t from now.
 */
void
AckRecons::sendack(int ack, double t)
{
	Packet *ackp = ackTemplate_->copy();
	Scheduler &s = Scheduler::instance();
	hdr_tcp *th = hdr_tcp::access(ackp);
	th->seqno() = ack;
	/* Set no_ts_ in flags because we don't want an rtt sample for this */
	if (th->ts() == hdr_tcp::access(ackp)->ts()) {
		hdr_flags *fh = hdr_flags::access(ackp);
		fh->no_ts_ = 1;
		th->ts_ = s.clock();	/* for debugging purposes only */
	}
	s.schedule((Handler *)this, (Event *)ackp, t);
	ackPending_++;
#ifdef DEBUG
	printf("\t%f\tScheduling ack %d to be sent at %f\n", 
	       s.clock(), ack, s.clock() + t);
#endif
}

/* 
 * Handle scheduling of acks.
 */
void
AckRecons::handle(Event *e)
{
	Packet *p = (Packet *) e;
	hdr_tcp *th = hdr_tcp::access(p);
	ackPending_--;
	if (lastAck_ < th->seqno()) {
		spq_->reconsAcks_ = 0;
		/* 
		 * need to do queue's recv here, so that a deque is
		 * forced if the queue isn't blocked.  It's not
		 * sufficient to call spq_->recv() alone.
		 */
		target_->recv(p); /* maybe do acksfirst for this ack? */
		spq_->reconsAcks_ = 1;
		lastTime_ = Scheduler::instance().clock();
		lastAck_ = th->seqno();
#ifdef DEBUG
		printf("%f\tSending scheduled ack %d\n",lastTime_,th->seqno());
#endif
	} else {
		Packet::free(p);
#ifdef DEBUG
		printf("%f\tack %d superceded by ack %d at %f\n", 
		       Scheduler::instance().clock(), th->seqno(), lastAck_,
		       lastTime_);
#endif
	}
}
