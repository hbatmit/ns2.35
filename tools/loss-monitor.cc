/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1994-1997 Regents of the University of California.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
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
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/tools/loss-monitor.cc,v 1.18 2000/09/01 03:04:06 haoboy Exp $ (LBL)";
#endif

#include <tclcl.h>

#include "agent.h"
#include "config.h"
#include "packet.h"
#include "ip.h"
#include "rtp.h"
#include "loss-monitor.h"

static class LossMonitorClass : public TclClass {
public:
	LossMonitorClass() : TclClass("Agent/LossMonitor") {}
	TclObject* create(int, const char*const*) {
		return (new LossMonitor());
	}
} class_loss_mon;

LossMonitor::LossMonitor() : Agent(PT_NTYPE)
{
	bytes_ = 0;
	nlost_ = 0;
	npkts_ = 0;
	expected_ = -1;
	last_packet_time_ = 0.;
	seqno_ = 0;
	bind("nlost_", &nlost_);
	bind("npkts_", &npkts_);
	bind("bytes_", &bytes_);
	bind("lastPktTime_", &last_packet_time_);
	bind("expected_", &expected_);
}

void LossMonitor::recv(Packet* pkt, Handler*)
{
	hdr_rtp* p = hdr_rtp::access(pkt);
	seqno_ = p->seqno();
	bytes_ += hdr_cmn::access(pkt)->size();
	++npkts_;
	/*
	 * Check for lost packets
	 */
	if (expected_ >= 0) {
		int loss = seqno_ - expected_;
		if (loss > 0) {
			nlost_ += loss;
			Tcl::instance().evalf("%s log-loss", name());
		}
	}
	last_packet_time_ = Scheduler::instance().clock();
	expected_ = seqno_ + 1;
	Packet::free(pkt);
}

/*
 * $proc interval $interval
 * $proc size $size
 */
int LossMonitor::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "clear") == 0) {
			expected_ = -1;
			return (TCL_OK);
		}
	}
	return (Agent::command(argc, argv));
}
