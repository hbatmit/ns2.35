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
 */

#ifndef ns_tcp_fs_h
#define ns_tcp_fs_h

#include "tcp.h"
#include "ip.h"
#include "flags.h"
#include "random.h"
#include "template.h"
#include "tcp-fack.h"

class ResetTimer : public TimerHandler {
public: 
	ResetTimer(TcpAgent *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	TcpAgent *a_;
};

/* TCP-FS with Tahoe */
class TcpFsAgent : public virtual TcpAgent {
public:
	TcpFsAgent() : t_exact_srtt_(0), t_exact_rttvar_(0), last_recv_time_(0), 
		fs_startseq_(0), fs_endseq_(0), fs_mode_(0), count_bytes_acked_(0),
		reset_timer_(this) 
	{
		bind_bool("fast_loss_recov_", &fast_loss_recov_);
		bind_bool("fast_reset_timer_", &fast_reset_timer_);
		bind_bool("count_bytes_acked_", &count_bytes_acked_);
		bind_bool("fs_enable_", &fs_enable_);
	}

	/* helper functions */
	virtual void output_helper(Packet* pkt);
	virtual void recv_helper(Packet* pkt);
	virtual void send_helper(int maxburst);
	virtual void send_idle_helper();
	virtual void recv_newack_helper(Packet* pkt);
	virtual void partialnewack_helper(Packet*) {};

	virtual void set_rtx_timer();
	virtual void cancel_rtx_timer();
	virtual void cancel_timers();
	virtual void timeout_nonrtx(int tno);
	virtual void timeout_nonrtx_helper(int tno);
	double rtt_exact_timeout() { return (t_exact_srtt_ + 4*t_exact_rttvar_);}
protected:
	double t_exact_srtt_;
	double t_exact_rttvar_;
	double last_recv_time_;
	int fs_startseq_;
	int fs_endseq_;
	int fs_mode_;
	int fs_enable_;
	int fast_loss_recov_;
	int fast_reset_timer_;
	int count_bytes_acked_;
	ResetTimer reset_timer_;
};

/* TCP-FS with Reno */
class RenoTcpFsAgent : public RenoTcpAgent, public TcpFsAgent {
public:
	RenoTcpFsAgent() : RenoTcpAgent(), TcpFsAgent() {}

	/* helper functions */
	virtual void output_helper(Packet* pkt) {TcpFsAgent::output_helper(pkt);}
	virtual void recv_helper(Packet* pkt) {TcpFsAgent::recv_helper(pkt);}
	virtual void send_helper(int maxburst) {TcpFsAgent::send_helper(maxburst);}
	virtual void send_idle_helper() {TcpFsAgent::send_idle_helper();}
	virtual void recv_newack_helper(Packet* pkt) {TcpFsAgent::recv_newack_helper(pkt);}

	virtual void set_rtx_timer() {TcpFsAgent::set_rtx_timer();}
	virtual void cancel_rtx_timer() {TcpFsAgent::cancel_rtx_timer();}
	virtual void cancel_timers(){TcpFsAgent::cancel_timers();}
	virtual void timeout_nonrtx(int tno) {TcpFsAgent::timeout_nonrtx(tno);}
	virtual void timeout_nonrtx_helper(int tno);
};

/* TCP-FS with NewReno */
class NewRenoTcpFsAgent : public virtual NewRenoTcpAgent, public TcpFsAgent {
public:
	NewRenoTcpFsAgent() : NewRenoTcpAgent(), TcpFsAgent() {}

	/* helper functions */
	virtual void output_helper(Packet* pkt) {TcpFsAgent::output_helper(pkt);}
	virtual void recv_helper(Packet* pkt) {TcpFsAgent::recv_helper(pkt);}
	virtual void send_helper(int maxburst) {TcpFsAgent::send_helper(maxburst);}
	virtual void send_idle_helper() {TcpFsAgent::send_idle_helper();}
	virtual void recv_newack_helper(Packet* pkt) {TcpFsAgent::recv_newack_helper(pkt);}
	virtual void partialnewack_helper(Packet* pkt);

	virtual void set_rtx_timer() {TcpFsAgent::set_rtx_timer();}
	virtual void cancel_rtx_timer() {TcpFsAgent::cancel_rtx_timer();}
	virtual void cancel_timers(){TcpFsAgent::cancel_timers();}
	virtual void timeout_nonrtx(int tno) {TcpFsAgent::timeout_nonrtx(tno);}
	virtual void timeout_nonrtx_helper(int tno);
};

#ifdef USE_FACK
/* TCP-FS with Fack */
class FackTcpFsAgent : public FackTcpAgent, public TcpFsAgent {
public:
	FackTcpFsAgent() : FackTcpAgent(), TcpFsAgent() {}

	/* helper functions */
	virtual void output_helper(Packet* pkt) {TcpFsAgent::output_helper(pkt);}
	virtual void recv_helper(Packet* pkt) {TcpFsAgent::recv_helper(pkt);}
	virtual void send_helper(int maxburst);
	virtual void send_idle_helper() {TcpFsAgent::send_idle_helper();}
	virtual void recv_newack_helper(Packet* pkt) {TcpFsAgent::recv_newack_helper(pkt);}
	virtual void set_rtx_timer() {TcpFsAgent::set_rtx_timer();}
	virtual void cancel_rtx_timer() {TcpFsAgent::cancel_rtx_timer();}
	virtual void cancel_timers(){TcpFsAgent::cancel_timers();}
	virtual void timeout_nonrtx(int tno) {TcpFsAgent::timeout_nonrtx(tno);}
	virtual void timeout_nonrtx_helper(int tno);
};
#endif

#endif
