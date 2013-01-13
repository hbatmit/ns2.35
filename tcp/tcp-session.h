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
 * Contributed by the Daedalus Research Group, http://daedalus.cs.berkeley.edu
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/tcp/tcp-session.h,v 1.8 1999/09/09 03:25:26 salehi Exp $ (UCB)
 */

#ifndef nc_tcp_session_h
#define ns_tcp_session_h

#include "agent.h"
#include "timer-handler.h"
#include "chost.h"

#define FINE_ROUND_ROBIN 1
#define COARSE_ROUND_ROBIN 2
#define RANDOM 3

class TcpSessionAgent;

class SessionRtxTimer : public TimerHandler {
public: 
	SessionRtxTimer(TcpSessionAgent *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	TcpSessionAgent *a_;
};

class SessionResetTimer : public TimerHandler {
public: 
	SessionResetTimer(TcpSessionAgent *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	TcpSessionAgent *a_;
};

class SessionBurstSndTimer : public TimerHandler {
public: 
	SessionBurstSndTimer(TcpSessionAgent *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	TcpSessionAgent *a_;
};

/*
 * This class implements the notion of a TCP session. It integrates congestion 
 * control and loss detection functionality across the one or more IntTcp 
 * connections that comprise the session.
 */
class TcpSessionAgent : public CorresHost {
public:
	TcpSessionAgent();
	int command(int argc, const char*const* argv);
	void reset_rtx_timer(int mild, int backoff = 1); /* XXX mild needed ? */
	void set_rtx_timer();
	void cancel_rtx_timer();
	void cancel_timers();
	void newack(Packet *pkt);
	int fs_pkt();
	void rtt_update_exact(double tao);
	void timeout(int tno);
	virtual Segment* add_pkts(int size, int seqno, int sessionSeqno, int daddr, 
			int dport, int sport, double ts, IntTcpAgent *sender); 
	virtual void add_agent(IntTcpAgent *agent, int size, double winMult,
			int winInc, int ssthresh);
	int window();
	void set_weight(IntTcpAgent *tcp, int wt);
	void reset_dyn_weights();
	IntTcpAgent *who_to_snd(int how);
	void send_much(IntTcpAgent *agent, int force, int reason); 
	void recv(IntTcpAgent *agent, Packet *pkt, int amt_data_acked);
	void setflags(Packet *pkt);
	int findSessionSeqno(IntTcpAgent *sender, int seqno);
	void removeSessionSeqno(int sessionSeqno);
	void quench(int how, IntTcpAgent *sender, int seqno);
	virtual void traceVar(TracedVar* v);

// 	inline nsaddr_t& addr() {return addr_;}
// 	inline nsaddr_t& dst() {return dst_;}
	static Islist<TcpSessionAgent> sessionList_;

protected:
	SessionRtxTimer rtx_timer_;
	SessionBurstSndTimer burstsnd_timer_;
	int sessionSeqno_;
	double last_send_time_;
	Segment *last_seg_sent_;
	IntTcpAgent *curConn_;
	int numConsecSegs_;
	int schedDisp_;
	int wtSum_;
	int dynWtSum_;
};

#endif
