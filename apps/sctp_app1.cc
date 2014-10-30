/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Protocol Engineering Lab, U of Delaware
 * Janardhan Iyengar   <iyengar@@cis,udel,edu>
 *
 * This SCTP test application is a modified version of the telnet code
 * developed by the University of California. The original copyright is below.
 */
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
 * @(#) $Header: /cvsroot/nsnam/ns-2/apps/sctp_app1.cc,v 1.1 2003/08/21 18:25:59 haldar Exp $ */

#include "random.h"
#include "sctp.h"
#include "sctp_app1.h"

extern double tcplib_telnet_interarrival();
 
AppData_S *data_to_transport = NULL;

static class SctpApp1Class : public TclClass {
 public:
	SctpApp1Class() : TclClass("Application/SctpApp1") {}
	TclObject* create(int, const char*const*) {
		return (new SctpApp1);
	}
} class_app_sctp;


SctpApp1::SctpApp1() : running_(0), timer_(this)
{
	bind("interval_", &interval_);
	bind("numUnreliable_", &numUnreliable);
	bind("numStreams_", &numStreams);
	bind("reliability_", &reliability);
}


void SctpApp1Timer::expire(Event*)
{
        t_->timeout();
}


void SctpApp1::start()
{
        running_ = 1;
	double t = next();
	timer_.sched(t);
}

void SctpApp1::stop()
{
        running_ = 0;
}

void SctpApp1::timeout()
{
        if (running_) {
		data_to_transport = new AppData_S;
		memset(data_to_transport, 0, sizeof(AppData_S));
		data_to_transport->usNumStreams = numStreams;
		data_to_transport->usNumUnreliable = numUnreliable;
		data_to_transport->usReliability = reliability;
		data_to_transport->uiNumBytes = 257;
		agent_->sendmsg(agent_->size(), (char*)data_to_transport);
		/* reschedule the timer */
		double t = next();
		timer_.resched(t);
	}
}

double SctpApp1::next()
{
        if (interval_ == 0)
	        /* use tcplib */
		return tcplib_telnet_interarrival();
	else
	        return Random::exponential() * interval_;
}
