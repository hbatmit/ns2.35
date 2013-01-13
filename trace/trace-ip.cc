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

#include "ip.h"
#include "trace.h"
#include "mac.h"
//#include "packet.h"


class TraceIp : public Trace {
public:
	TraceIp(int type) : Trace(type) {
		bind("mask_", &mask_);
		bind("shift_", &shift_);
	}
	void recv(Packet*, Handler*);
protected:
	int mask_;
	int shift_;
};

class TraceIpMac : public TraceIp {
public:
	TraceIpMac(int type) : TraceIp(type) {}
	void recv(Packet*, Handler*);
};


class TraceIpClass : public TclClass {
public:
	TraceIpClass() : TclClass("TraceIp") { }
	TclObject* create(int args, const char*const* argv) {
		if (args >= 5)
			return (new TraceIp(*argv[4]));
		else
			return NULL;
	}
} traceip_class;

class TraceIpMacClass : public TclClass {
public:
	TraceIpMacClass() : TclClass("TraceIp/Mac") { }
	TclObject* create(int args, const char*const* argv) {
		if (args >= 5)
			return (new TraceIpMac(*argv[4]));
		else
			return NULL;
	}
} trace_ip_mac_class;


void TraceIp::recv(Packet* p, Handler* h)
{
	// XXX: convert IP address to node number
	hdr_ip *iph = hdr_ip::access(p);
	int src = (src_ >= 0) ? src_ : (iph->saddr() >> shift_) & mask_;
	int dst = (iph->daddr() >> shift_) & mask_;
	format(type_, src, dst , p);
	pt_->dump();
	target_ ? send(p, h) : Packet::free(p);
}


void TraceIpMac::recv(Packet* p, Handler* h)
{
	// XXX: convert IP address to node number
	// hdr_ip *iph = hdr_ip::access(p);
	
	hdr_ip *iph = HDR_IP(p);
	
	int src = (src_ >= 0) ? src_ : (iph->saddr() >> shift_) & mask_;
	int dst = (iph->daddr() >> shift_) & mask_;

	hdr_mac* mh = HDR_MAC(p);
	
	if (mh->ftype() == MF_ACK || mh->ftype() == MF_CTS)
		format(type_, dst, src , p);
	else
		format(type_, src, dst , p);
	pt_->dump();
	target_ ? send(p, h) : Packet::free(p);
}
