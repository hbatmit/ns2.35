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
    "@(#) $Header: /cvsroot/nsnam/ns-2/plm/loss-monitor-plm.cc,v 1.2 2000/09/01 03:04:11 haoboy Exp $ (LBL)";
#endif

#include <tclcl.h>
#include "loss-monitor.h"

class PLMLossMonitor : public LossMonitor {
public:
	PLMLossMonitor();
	virtual void recv(Packet* pkt, Handler*);
protected:
	// PLM only
	int flag_PP_;
	double packet_time_PP_;
	int fid_PP_;
};

static class PLMLossMonitorClass : public TclClass {
public:
	PLMLossMonitorClass() : TclClass("Agent/LossMonitor/PLM") {}
	TclObject* create(int, const char*const*) {
		return (new PLMLossMonitor());
	}
} class_loss_mon_plm;

PLMLossMonitor::PLMLossMonitor() : LossMonitor()
{
	flag_PP_ = 0;
	bind("flag_PP_", &flag_PP_);
	bind("packet_time_PP_", &packet_time_PP_);
	bind("fid_PP_", &fid_PP_);
	bind("seqno_", &seqno_);
}

void PLMLossMonitor::recv(Packet* pkt, Handler*)
{
	packet_time_PP_ = Scheduler::instance().clock();
	hdr_ip* iph = HDR_IP(pkt);
	fid_PP_ = iph->flowid();

	hdr_rtp* p = HDR_RTP(pkt);
	seqno_ = p->seqno();
	bytes_ += HDR_CMN(pkt)->size();
	flag_PP_ = p->flags();
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
	Tcl::instance().evalf("%s log-PP", name());
	last_packet_time_ = Scheduler::instance().clock();
	expected_ = seqno_ + 1;
	Packet::free(pkt);
}
