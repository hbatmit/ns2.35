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
 * @(#) $Header: /cvsroot/nsnam/ns-2/apps/telnet.cc,v 1.5 1998/08/14 20:09:33 tomh Exp $
 */

#include "random.h"
#include "tcp.h"
#include "telnet.h"

extern double tcplib_telnet_interarrival();

static class TelnetAppClass : public TclClass {
 public:
	TelnetAppClass() : TclClass("Application/Telnet") {}
	TclObject* create(int, const char*const*) {
		return (new TelnetApp);
	}
} class_app_telnet;


TelnetApp::TelnetApp() : running_(0), timer_(this)
{
	bind("interval_", &interval_);
}


void TelnetAppTimer::expire(Event*)
{
        t_->timeout();
}


void TelnetApp::start()
{
        running_ = 1;
	double t = next();
	timer_.sched(t);
}

void TelnetApp::stop()
{
        running_ = 0;
}

void TelnetApp::timeout()
{
        if (running_) {
	        /* call the TCP advance method */
		agent_->sendmsg(agent_->size());
		/* reschedule the timer */
		double t = next();
		timer_.resched(t);
	}
}

double TelnetApp::next()
{
        if (interval_ == 0)
	        /* use tcplib */
	        return tcplib_telnet_interarrival();
	else
	        return Random::exponential() * interval_;
}
