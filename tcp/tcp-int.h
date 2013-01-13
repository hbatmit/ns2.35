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
 * 	This product includes software developed by the Daedalus Research
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
 *
 * Contributed by the Daedalus Research Group, U.C.Berkeley
 * http://daedalus.cs.berkeley.edu
 */

#ifndef TCPINT_H
#define TCPINT_H

#include "tcp.h"
#include "nilist.h"

/*class CorresHost;*/
class CorresHost;
class TcpSessionAgent;
class Segment;

class IntTcpAgent : public TcpAgent, public slink {
	friend class CorresHost;
	friend class TcpSessionAgent;
  public:
	IntTcpAgent();
	int command(int argc, const char*const* argv);
	void createTcpSession();
	void send_much(int force, int reason, int maxburst = 0);
	void send_one(int sessionSeqno);
	void recv(Packet *pkt, Handler *);
	void opencwnd();
	void closecwnd(int how);
	Segment *rxmit_last(int reason, int seqno, int sessionSeqno, double ts);
	void output(int seqno, int reason = 0);
	void output_helper(Packet *p);
	int data_left_to_send();
	void newack(Packet* pkt);
	
  protected:
	class TcpSessionAgent *session_;
	int uniqTS_;
	int rightEdge_;
	double closecwTS_;
	double lastTS_;
	double winMult_;
	int winInc_;
	int count_;		/* used in window increment algorithms */
	int daddr_;
	int dport_;
	int sport_;
	int shift_;
	int mask_;
	int wt_;
	int dynWt_;
	int wndIncSeqno_;       /* used to mark RTTs for window inc. algorithm */
	int num_thresh_dupack_segs_;
};
#endif
