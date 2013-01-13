/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 The Regents of the University of California.
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
 *      This product includes software developed by the Network Research
 *      Group at Lawrence Berkeley National Laboratory.
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
 * @(#) /home/ctk21/cvsroot//hssstcp/ns/ns-2.1b9/tcp/tcp-fack.h,v 1.2 2002/08/12 10:44:38 ctk21 Exp (LBL)
 */

#ifndef ns_tcp_fack_h
#define ns_tcp_fack_h

#include "tcp.h"
#include "scoreboard.h"

#define TRUE    1
#define FALSE   0
#define RECOVER_DUPACK  1
#define RECOVER_TIMEOUT 2
#define RECOVER_QUENCH  3

/* TCP Fack */
class FackTcpAgent : public TcpAgent {
 public:
	FackTcpAgent();
	virtual ~FackTcpAgent();
	virtual void recv(Packet *pkt, Handler*);
	virtual void timeout(int tno);
	virtual void opencwnd();
	virtual int window();
	void oldack (Packet* pkt);
	int maxsack (Packet* pkt); 
	void plot();
	void reset();
	virtual void send_much(int force, int reason, int maxburst = 0);
	virtual void recv_newack_helper(Packet* pkt);
 protected:
	u_char timeout_;	/* flag: sent pkt from timeout; */
	u_char fastrecov_;	/* flag: in fast recovery */
	double wintrim_;
	double wintrimmult_;
	int rampdown_;
	int fack_;
	int retran_data_;
	int ss_div4_;

	ScoreBoard* scb_;
	static const int SBSIZE=1024;
};


#endif
