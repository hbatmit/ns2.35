// -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-

/*
 * Copyright (C) 2004-2006 by the University of Southern California,
 * 			     		   Information Sciences Institute
 *                    2002 by Dina Katabi
 * $Id: xcp-end-sys.cc,v 1.12 2006/05/30 20:30:30 pradkin Exp $
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
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation, advertising
 * materials, and other materials related to such distribution and use
 * acknowledge that the software was developed by the University of
 * Southern California, Information Sciences Institute.  The name of the
 * University may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef lint
static const char rcsid[] =
"@(#) $Header: /cvsroot/nsnam/ns-2/xcp/xcp-end-sys.cc,v 1.12 2006/05/30 20:30:30 pradkin Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "ip.h"
#include "tcp.h"
#include "flags.h"

#include "agent.h"
#include "packet.h"

#include "flags.h"
#include "tcp-sink.h"
#include "xcp-end-sys.h"


#define TRACE 0 // when 0, we don't print any debugging info.

unsigned int XcpEndsys::next_xcp_ = 0;

XcpEndsys::XcpEndsys(TcpAgent *tcp) : tcp_(tcp)
{
	init_rtt_vars();
	// do not waste sequence numbers on sinks (tcp == 0)
	// makes it easier to trace senders
	tcpId_				= (tcp == 0) ? 0xffff : next_xcp_++;
	current_positive_feedback_	= 0.0;
	xcp_srtt_			= 0;
	xcp_rev_fb_			= 0;
}

void 
XcpEndsys::trace_var(const char *var_name, double var) const
{
	if (tcp_ && tcp_->channel_) {
		const size_t SIZE=512;
		char wrk[SIZE];
		int n = snprintf(wrk, SIZE, "%g x x x x %s %g\n", 
				 Scheduler::instance().clock(), var_name, var);
		(void)Tcl_Write(tcp_->channel_, wrk, n);
	}
}

void
XcpEndsys::rtt_init()
{
	init_rtt_vars();
}

void XcpEndsys::rtt_update(double tao)
{
	if (!tcp_)
		return;
#define FIX1 1 /* 1/0 : 1 for experimental XCP changes, works only with timestamps */
	double now = Scheduler::instance().clock();
	double sendtime = now - tao; // XXX instead, better pass send/recv times as args
	int t_rtt;
	double tick = tcp_->tcp_tick_;
	if (tcp_->ts_option_) {
#if FIX1
		int send_tick = int(sendtime/tick);
		int recv_tick = int(now/tick);
		t_rtt = recv_tick - send_tick;
#else
		t_rtt = int(tao / tick + 0.5);
#endif /* FIX1 */
	} else {
		// XXX I don't understand this business with
		// boot_time_, and so not quite sure what FIX1 should
		// look like in this case perhaps something like:
		//      t_rtt_ = int(now/tcp_tick_) - int((sendtime - tickoff)/tcp_tick_);
		// for now FIX1 works only with timestamps.
 
		sendtime += tcp_->boot_time_;
		double tickoff = fmod(sendtime, tick);
		t_rtt = int((tao + tickoff) / tick);
	}
	assert(t_rtt >= 0);
	if (xcp_srtt_ != 0)
		xcp_srtt_ = XCP_UPDATE_SRTT(xcp_srtt_, t_rtt);
	else 
		xcp_srtt_ = XCP_INIT_SRTT(t_rtt);
	//if (xcp_srtt_ == 0)
	//	++xcp_srtt_; //zero rtts are bad, mkey
	srtt_estimate_ = double(xcp_srtt_) * tick / double(1 << XCP_RTT_SHIFT);

	if (TRACE) {
		printf("%d:  %g  SRTT %g, RTT %g \n", tcpId_, now, srtt_estimate_, tao);
	}
	return;
}

void 
XcpEndsys::recv(Packet *p)
{

	hdr_xcp *xh = hdr_xcp::access(p);		
	if (xh->xcp_enabled_ != hdr_xcp::XCP_DISABLED) {
		xcp_rev_fb_ += xh->delta_throughput_;
		if (tcp_) {
			if (tcp_->channel_) {
				trace_var("reverse_feedback_", xh->reverse_feedback_);
				trace_var("controlling_hop_", xh->controlling_hop_);
			}
			
			double delta_cwnd = 0;
			
			delta_cwnd = (xh->reverse_feedback_
				      * srtt_estimate_ 
				      / tcp_->size_ );
		
			double newcwnd = (tcp_->cwnd_ + delta_cwnd);
			
			if (tcp_->maxcwnd_ && (newcwnd > tcp_->maxcwnd_))
				newcwnd = tcp_->maxcwnd_;
			if (newcwnd < 1)
				newcwnd = 1;
			tcp_->cwnd_ = newcwnd;
			if (tcp_->channel_)
				trace_var("cwnd", double(tcp_->cwnd_));
		}
	}
}

void
XcpEndsys::send(Packet *p, int datalen)
{
	hdr_xcp *xh = hdr_xcp::access(p);

	xh->xcp_enabled_ = (datalen) ? hdr_xcp::XCP_ENABLED : hdr_xcp::XCP_ACK;
	xh->rtt_ = srtt_estimate_;
	xh->xcpId_ = tcpId_;
	xh->reverse_feedback_ = xcp_rev_fb_;
	xcp_rev_fb_ = 0;
	
	xh->x_ = 0;
	xh->delta_throughput_ = 0;
	xh->cwnd_ = 0;

	if (!tcp_) return;

	xh->cwnd_ = double(tcp_->cwnd_);
	double tput = 0;
	if (xh->xcp_enabled_) {
#define MAX_THROUGHPUT        1e24
		if (srtt_estimate_ != 0) {
			tput = tcp_->window() * tcp_->size_ / srtt_estimate_;
			xh->x_ = srtt_estimate_/tcp_->window();
			xh->delta_throughput_ = (MAX_THROUGHPUT 
						 - tput);
		} else {
			//XXX can do xh->xcp_enabled_ = hdr_xcp::XCP_DISABLED;
			xh->x_ = 0;
			xh->delta_throughput_ = 0;
		}
		if (tcp_->channel_) {
			trace_var("throughput", tput);
		}
	}
}

XcpNewRenoFullTcpAgent::XcpNewRenoFullTcpAgent() 
	: NewRenoFullTcpAgent(), XcpEndsys(this)
{
	type_ = PT_XCP;
}

void 
XcpNewRenoFullTcpAgent::delay_bind_init_all() 
{
	NewRenoFullTcpAgent::delay_bind_init_all();
}

int
XcpNewRenoFullTcpAgent::delay_bind_dispatch(const char *varName, 
					    const char *localName, 
					    TclObject *tracer)
{
	return NewRenoFullTcpAgent::delay_bind_dispatch(varName, localName, tracer);
}

void
XcpNewRenoFullTcpAgent::rtt_update(double tao)
{
	//order seems unimportant here
	XcpEndsys::rtt_update(tao);
	NewRenoFullTcpAgent::rtt_update(tao);
}

void 
XcpNewRenoFullTcpAgent::rtt_init()
{
	//order seems unimportant here
	NewRenoFullTcpAgent::rtt_init();
	XcpEndsys::rtt_init();
}

void 
XcpNewRenoFullTcpAgent::recv(Packet *p,  Handler *h)
{
	// xcp must be done before tcp because tcp will consume the packet
	XcpEndsys::recv(p);
	NewRenoFullTcpAgent::recv(p, h);
}

void 
XcpNewRenoFullTcpAgent::sendpacket(int seqno, int ackno, int pflags, 
				   int datalen, int reason, Packet *p)
{
	// xcp must be done before tcp because tcp will consume the packet
	if (!p) p = allocpkt();
	XcpEndsys::send(p, datalen);
	NewRenoFullTcpAgent::sendpacket(seqno, ackno, pflags, 
					datalen, reason, p);
}


XcpRenoTcpAgent::XcpRenoTcpAgent()
	: RenoTcpAgent(), XcpEndsys(this)
{
	type_ = PT_XCP; 
}

void
XcpRenoTcpAgent::delay_bind_init_all()
{
	RenoTcpAgent::delay_bind_init_all();
}

int
XcpRenoTcpAgent::delay_bind_dispatch(const char *varName, 
				     const char *localName, 
				     TclObject *tracer)
{
	return RenoTcpAgent::delay_bind_dispatch(varName, localName, tracer);
}

void
XcpRenoTcpAgent::output_helper(Packet* p)
{
	int datalen = hdr_cmn::access(p)->size();
	XcpEndsys::send(p, datalen);
}

void
XcpRenoTcpAgent::recv_newack_helper(Packet *p)
{
	RenoTcpAgent::recv_newack_helper(p);
	XcpEndsys::recv(p);
}

void
XcpRenoTcpAgent::rtt_update(double tao)
{
	XcpEndsys::rtt_update(tao);
	RenoTcpAgent::rtt_update(tao);
}

void 
XcpRenoTcpAgent::rtt_init()
{
	RenoTcpAgent::rtt_init();
	XcpEndsys::rtt_init();
}

XcpTcpSink::XcpTcpSink(Acker *a) 
	: TcpSink(a), XcpEndsys(0)
{
}

void
XcpTcpSink::delay_bind_init_all()
{
	TcpSink::delay_bind_init_all();
}

int
XcpTcpSink::delay_bind_dispatch(const char *varName, 
			     const char *localName, 
			     TclObject *tracer)
{
	return TcpSink::delay_bind_dispatch(varName, localName, tracer);
}

void
XcpTcpSink::recv(Packet* p, Handler *h)
{
	XcpEndsys::recv(p);
	TcpSink::recv(p, h);
}
	
void
XcpTcpSink::add_to_ack(Packet *p)
{
	int datalen = 0;
	XcpEndsys::send(p, datalen);
	TcpSink::add_to_ack(p);
}

//static tclcl goo follows
int hdr_xcp::offset_;
static class XCPHeaderClass : public PacketHeaderClass {
public:
        XCPHeaderClass() : PacketHeaderClass("PacketHeader/XCP",
					     sizeof(hdr_xcp)) {
		bind_offset(&hdr_xcp::offset_);
	}
} class_xcphdr;

static class XcpNewRenoFullTcpAgentClass : public TclClass {
public:
	XcpNewRenoFullTcpAgentClass() : TclClass("Agent/TCP/FullTcp/Newreno/XCP") {}
	virtual TclObject* create(int, const char* const*) {
		return (new XcpNewRenoFullTcpAgent());
	}
} class_xcp_newreno_full_tcp_agent;

static class XcpRenoTcpAgentClass : public TclClass {
public:
	XcpRenoTcpAgentClass() : TclClass("Agent/TCP/Reno/XCP") {}
	TclObject* create(int, const char*const*) {
		return (new XcpRenoTcpAgent());
	}
} class_xcp_reno_tcp_agent;

static class XcpTcpSinkClass : public TclClass {
public:
	XcpTcpSinkClass() : TclClass("Agent/TCPSink/XCPSink") {}
	TclObject* create(int, const char*const*) {
		return (new XcpTcpSink(new Acker));
	}
} class_xcp_tcpsink_agent;
