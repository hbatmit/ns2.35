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
 * Contributed by Giao Nguyen, http://daedalus.cs.berkeley.edu/~gnguyen
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/mac/mac-csma.cc,v 1.26 1998/11/17 23:36:35 yuriy Exp $ (UCB)";
#endif

#include "template.h"
#include "random.h"
#include "channel.h"
#include "mac-csma.h"

static class MacCsmaClass : public TclClass {
public:
	MacCsmaClass() : TclClass("Mac/Csma") {}
	TclObject* create(int, const char*const*) {
		return (new MacCsma);
	}
} class_mac_csma;

static class MacCsmaCdClass : public TclClass {
public:
	MacCsmaCdClass() : TclClass("Mac/Csma/Cd") {}
	TclObject* create(int, const char*const*) {
		return (new MacCsmaCd);
	}
} class_mac_csma_cd;

static class MacCsmaCaClass : public TclClass {
public:
	MacCsmaCaClass() : TclClass("Mac/Csma/Ca") {}
	TclObject* create(int, const char*const*) {
		return (new MacCsmaCa);
	}
} class_mac_csma_ca;


void
MacHandlerEoc::handle(Event* e)
{
	mac_->endofContention((Packet*)e);
}


MacCsma::MacCsma() : txstart_(0), rtx_(0), csense_(1), hEoc_(this)
{
	bind_time("ifs_", &ifs_);
	bind_time("slotTime_", &slotTime_);
	bind("cwmin_", &cwmin_);
	bind("cwmax_", &cwmax_);
	bind("rtxLimit_", &rtxLimit_);
	bind("csense_", &csense_);
	cw_ = cwmin_;
}


void MacCsma::resume(Packet* p)
{
	Scheduler& s = Scheduler::instance();
	s.schedule(callback_, &intr_, ifs_ + slotTime_ * cwmin_);
	if (p != 0)
		drop(p);
	callback_ = 0;
	state(MAC_IDLE);
	rtx_ = 0;
	cw_ = cwmin_;
}


void MacCsma::send(Packet* p)
{
	Scheduler& s = Scheduler::instance();
	double delay = channel_->txstop() + ifs_ - s.clock();

	// if channel is not ready, then wait
	// else content for the channel

	/* XXX floating point operations differences have been
	   observed on the resulting delay value on Pentium II and
	   SunSparc.  E.g.
	                           PentiumII                   SunSparc
	                           -------------------------------
	   channel_->txstop_=      0.11665366666666668         0.11665366666666668
	                   binary    0x3fbddd03c34ab4a2          0x3fbddd03c34ab4a2
	   ifs_=                   5.1999999999999997e-05      5.1999999999999997e-05
	                   binary    0x3f0b43526527a205          0x3f0b43526527a205
	   s.clock_=               0.11670566666666668         0.11670566666666668
	                   binary    0x3fbde06c2d975996          0x3fbde06c2d975996

	   delay=                  3.5033282698437862e-18      0
	                   binary    0x3c50280000000000          0x0000000000000000

	   Because of that the value of (csense_ && delay > 0) was different.  Fixed by
	   changing 0 to EPS
	 */
	static const double EPS= 1.0e-12; //seems appropriate (less than nanosec)
  	if (csense_ && delay > EPS)
		s.schedule(&hSend_, p, delay + 0.000001);
	else {
		txstart_ = s.clock();
		channel_->contention(p, &hEoc_);
	}
}


void MacCsma::backoff(Handler* h, Packet* p, double delay)
{
	Scheduler& s = Scheduler::instance();
	double now = s.clock();

	// if retransmission time within limit, do exponential backoff
	// else drop the packet and resume
	if (++rtx_ < rtxLimit_) {
		delay += max(channel_->txstop() + ifs_ - now, 0.0);
		int slot = Random::integer(cw_);
		s.schedule(h, p, delay + slotTime_ * slot);
		cw_ = min(2 * cw_, cwmax_);
	}
	else
		resume(p);
}


void MacCsma::endofContention(Packet* p)
{
	Scheduler& s = Scheduler::instance();
	double txt = txtime(p) - (s.clock() - txstart_);
	hdr_mac::access(p)->txtime() = txt;
	channel_->send(p, txt);
	s.schedule(&hRes_, &eEoc_, txt);
	rtx_ = 0;
	cw_ = cwmin_;
}


void MacCsmaCd::endofContention(Packet* p)
{
	// If there is a collision, backoff
	if (channel_->collision()) {
		channel_->jam(0);
		backoff(&hSend_, p);
	}
	else
		MacCsma::endofContention(p);
}


void MacCsmaCa::send(Packet* p)
{
	Scheduler& s = Scheduler::instance();
	double delay = channel_->txstop() + ifs_ - s.clock();

	if (csense_ && delay > 0)
		backoff(&hSend_, p);
	else {
		txstart_ = s.clock();
		channel_->contention(p, &hEoc_);
	}
}
