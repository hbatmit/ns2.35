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
    "@(#) $Header: /cvsroot/nsnam/ns-2/apps/rtp.cc,v 1.27 2005/08/22 05:08:32 tomh Exp $";
#endif


#include <stdlib.h>

#include "config.h"
#include "agent.h"
#include "random.h"
#include "rtp.h"

int hdr_rtp::offset_;

class RTPHeaderClass : public PacketHeaderClass {
public: 
        RTPHeaderClass() : PacketHeaderClass("PacketHeader/RTP",
					     sizeof(hdr_rtp)) {
		bind_offset(&hdr_rtp::offset_);
	}
} class_rtphdr;

static class RTPAgentClass : public TclClass {
public:
	RTPAgentClass() : TclClass("Agent/RTP") {}
	TclObject* create(int, const char*const*) {
		return (new RTPAgent());
	}
} class_rtp_agent;

RTPAgent::RTPAgent() : Agent(PT_RTP), session_(0), lastpkttime_(-1e6), 
    running_(0), rtp_timer_(this)
{
	bind("seqno_", &seqno_);
	bind_time("interval_", &interval_);
	bind("packetSize_", &size_);
	bind("maxpkts_", &maxpkts_);
	bind("random_", &random_);
}

void RTPAgent::start()
{
        running_ = 1;
        sendpkt();
        rtp_timer_.resched(interval_);
}

void RTPAgent::stop()
{
        rtp_timer_.force_cancel();
        finish();
}

void RTPAgent::sendmsg(int nbytes, const char* /*flags*/)
{
        Packet *p;
        int n;

	assert (size_ > 0);

        if (++seqno_ < maxpkts_) {
		n = nbytes / size_;

                if (nbytes == -1) {
                        start();
                        return;
                }
                while (n-- > 0) {
                        p = allocpkt();
                        hdr_rtp* rh = hdr_rtp::access(p);
                        rh->seqno() = seqno_;
                        target_->recv(p);
                }
                n = nbytes % size_;
                if (n > 0) {
                        p = allocpkt();
                        hdr_rtp* rh = hdr_rtp::access(p);
                        rh->seqno() = seqno_;
                        target_->recv(p);
                }
                idle();
        } else {
                finish();
                // xxx: should we deschedule the timer here? */
        };
}

void RTPAgent::timeout(int) 
{
	if (running_) {
		sendpkt();
		if (session_)
			session_->localsrc_update(size_);
		double t = interval_;
		if (random_)
			/* add some zero-mean white noise */
			t += interval_ * Random::uniform(-0.5, 0.5);
		rtp_timer_.resched(t);
	}
}

/*
 * finish() is called when we must stop (either by request or because
 * we're out of packets to send.
 */
void RTPAgent::finish()
{
        running_ = 0;
        Tcl::instance().evalf("%s done", this->name());
}

void RTPAgent::advanceby(int delta)
{
        maxpkts_ += delta;
        if (seqno_ < maxpkts_ && !running_)
                start();
}               


void RTPAgent::recv(Packet* p, Handler*)
{
	if (session_)
		session_->recv(p, 0);
	else
		Packet::free(p);
}

int RTPAgent::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "rate-change") == 0) {
			rate_change();
			return (TCL_OK);
		} else if (strcmp(argv[1], "start") == 0) {
                        start();
                        return (TCL_OK);
                } else if (strcmp(argv[1], "stop") == 0) {
                        stop();
                        return (TCL_OK);
                }
	} else if (argc == 3) {
		if (strcmp(argv[1], "session") == 0) {
			session_ = (RTPSession*)TclObject::lookup(argv[2]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "advance") == 0) {
                        int newseq = atoi(argv[2]);
                        advanceby(newseq - seqno_);
                        return (TCL_OK); 
                } else if (strcmp(argv[1], "advanceby") == 0) {
                        advanceby(atoi(argv[2]));
                        return (TCL_OK);
                }       
	}
	return (Agent::command(argc, argv));
}

/* 
 * We modify the rate in this way to get a faster reaction to the a rate
 * change since a rate change from a very low rate to a very fast rate may 
 * take an undesireably long time if we have to wait for timeout at the old
 * rate before we can send at the new (faster) rate.
 */
void RTPAgent::rate_change()
{
	rtp_timer_.force_cancel();
	
	double t = lastpkttime_ + interval_;
	
	double now = Scheduler::instance().clock();
	if ( t > now)
		rtp_timer_.resched(t - now);
	else {
		sendpkt();
		rtp_timer_.resched(interval_);
	}
}

void RTPAgent::sendpkt()
{
	Packet* p = allocpkt();
	lastpkttime_ = Scheduler::instance().clock();
	makepkt(p);
	target_->recv(p, (Handler*)0);
}

void RTPAgent::makepkt(Packet* p)
{
	hdr_rtp *rh = hdr_rtp::access(p);
	/* Fill in srcid_ and seqno */
	rh->seqno() = seqno_++;
	rh->srcid() = session_ ? session_->srcid() : 0;
}

void RTPTimer::expire(Event* /*e*/) {
        a_->timeout(0);
}

