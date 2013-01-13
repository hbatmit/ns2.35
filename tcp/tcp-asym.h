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
 * Contributed by the Daedalus Research Group, U.C.Berkeley
 * http://daedalus.cs.berkeley.edu
 *
 * $Header: /cvsroot/nsnam/ns-2/tcp/tcp-asym.h,v 1.4 2000/09/01 03:04:07 haoboy Exp $
 */

#ifndef ns_tcp_asym_h
#define ns_tcp_asym_h
#include "tcp.h"
#include "ip.h"
#include "flags.h"
#include "random.h"
#include "template.h"

/* 
 * The TCP asym header. The sender includes information that the receiver could
 * use to decide by how much to delay acks.
 * XXXX some of the fields may not be needed
 */ 
struct hdr_tcpasym {
	int ackcount_;          /* the number of segments this ack represents */
	int win_;               /* the amount of window remaining */
	int highest_ack_;       /* the highest ack seen */
	int max_left_to_send_;  /* the max. amount of data that remains to be sent */

	static int offset_;	// offset for this header
	inline static int& offset() { return offset_; }
	inline static hdr_tcpasym* access(const Packet* p) {
		return (hdr_tcpasym*) p->access(offset_);
	}

	int& ackcount() { return (ackcount_); }
	int& win() { return (win_); }
	int& highest_ack() { return (highest_ack_); }
	int& max_left_to_send() { return (max_left_to_send_); }
};


/* TCP Asym with Tahoe */
class TcpAsymAgent : public virtual TcpAgent {
public:
	TcpAsymAgent();

	/* helper functions */
	virtual void output_helper(Packet* p);
	virtual void send_helper(int maxburst);
	virtual void recv_helper(Packet* p);
	virtual void recv_newack_helper(Packet* pkt);
	virtual void traceAll();
	virtual void traceVar(TracedVar* v);
protected:
	int ecn_to_echo_;
	TracedDouble t_exact_srtt_;
/*	TracedDouble avg_win_; */
	double g_;		/* gain used for exact_srtt_ smoothing */
	int damp_;
};

/* TCP Asym with Reno */
class TcpRenoAsymAgent : public RenoTcpAgent,
			 public TcpAsymAgent {
public:
	 TcpRenoAsymAgent() : RenoTcpAgent(), TcpAsymAgent() { }

	/* helper functions */
	virtual void output_helper(Packet* p) { TcpAsymAgent::output_helper(p); }
	virtual void send_helper(int maxburst) { TcpAsymAgent::send_helper(maxburst); }
	virtual void recv_helper(Packet* p) { TcpAsymAgent::recv_helper(p); }
	virtual void recv_newack_helper(Packet* pkt) { TcpAsymAgent::recv_newack_helper(pkt); }

};

/* TCP Asym with New Reno */
class NewRenoTcpAsymAgent : public virtual NewRenoTcpAgent,
			    public TcpAsymAgent {
public:
	 NewRenoTcpAsymAgent() : NewRenoTcpAgent(), TcpAsymAgent() { }

	/* helper functions */
	virtual void output_helper(Packet* p) { TcpAsymAgent::output_helper(p); }
	virtual void send_helper(int maxburst) { TcpAsymAgent::send_helper(maxburst); }
	virtual void recv_helper(Packet* p) { TcpAsymAgent::recv_helper(p); }
	virtual void recv_newack_helper(Packet* pkt) { TcpAsymAgent::recv_newack_helper(pkt); }

};

#endif
