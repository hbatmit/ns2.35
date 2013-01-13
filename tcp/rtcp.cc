/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
 *
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
 * 	This product includes software developed by the MASH Research
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
    "@(#) $Header: /cvsroot/nsnam/ns-2/tcp/rtcp.cc,v 1.17 2000/09/01 03:04:06 haoboy Exp $";
#endif

#include <stdlib.h>

#include "config.h"
#include "agent.h"
#include "random.h"
#include "rtp.h"

class RTCPAgent;

class RTCP_Timer : public TimerHandler {
public: 
	RTCP_Timer(RTCPAgent *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	RTCPAgent *a_;
};

class RTCPAgent : public Agent {
public:
	RTCPAgent();
	virtual void timeout(int);
	virtual void recv(Packet* p, Handler* h);
	int command(int argc, const char*const* argv);
protected:
	void start();
	void stop();
	void sendpkt();

	int running_;
	int random_;
	int seqno_;
	double interval_;
	RTPSession* session_;

	RTCP_Timer rtcp_timer_;
};

static class RTCPAgentClass : public TclClass {
public:
	RTCPAgentClass() : TclClass("Agent/RTCP") {}
	TclObject* create(int, const char*const*) {
		return (new RTCPAgent());
	}
} class_rtcp_agent;

/* XXX Could perhaps derive this from CBR.  If so, use cbr_timer_ */
RTCPAgent::RTCPAgent()
	: Agent(PT_RTCP), session_(0), rtcp_timer_(this)
{
	size_ = 128;
	bind_time("interval_", &interval_);
	bind("random_", &random_);
	bind("seqno_", &seqno_);
	running_ = 0;
}

void RTCPAgent::start()
{
	running_ = 1;
	rtcp_timer_.resched(interval_);
}

void RTCPAgent::stop()
{
	rtcp_timer_.cancel();
	running_ = 0;
}

void RTCPAgent::recv(Packet* p, Handler*)
{
	session_->recv_ctrl(p);
}

void RTCPAgent::sendpkt()
{
	Packet* p = allocpkt();
	hdr_rtp* rh = hdr_rtp::access(p);

	/* Fill in srcid_ and seqno */
	rh->seqno() = seqno_++;
	rh->srcid() = session_->srcid();
	target_->recv(p);
}

void RTCPAgent::timeout(int)
{
	if (running_) {
		size_ = session_->build_report(0);
		sendpkt();
		double t = interval_;
		if (random_)
			/* add some zero-mean white noise */
			t += interval_ * Random::uniform(-0.5, 0.5);	
		rtcp_timer_.resched(t);
		/* XXX */
		Tcl::instance().evalf("%s rtcp_timeout", session_->name());
	}
}

int RTCPAgent::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "start") == 0) {
			start();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "stop") == 0) {
			stop();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "bye") == 0) {
			size_ = session_->build_report(1);
			sendpkt();
			stop();
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "session") == 0) {
			session_ = (RTPSession*)TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
	}

	return (Agent::command(argc, argv));
}

void RTCP_Timer::expire(Event* /*e*/) {
	a_->timeout(0);
}
