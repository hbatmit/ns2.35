/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * tcp-rbp.cc
 * Copyright (C) 1997 by the University of Southern California
 * $Id: tcp-rbp.cc,v 1.22 2005/08/25 18:58:12 johnh Exp $
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
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */

/*
 * Tcp-vegas with Rate-based pacing by John Heidemann <johnh@isi.edu>
 * and Vikram Visweswaraiah <visweswa@isi.edu>.
 * The original SunOS implementation was by Vikram Visweswaraiah
 * and Ashish Savla <asavla@usc.edu>.
 *
 * Rate-based pacing is an experimental addition to TCP
 * to address the slow-start restart problem.
 * See <http://www.isi.edu/lsam/publications/rate_based_pacing/index.html>
 * for details.
 *
 * A paper analysing RBP performance is in progress (as of 19-Jun-97).
 */


#ifndef lint
static const char rcsid[] =
"@(#) $Header: /cvsroot/nsnam/ns-2/tcp/tcp-rbp.cc,v 1.22 2005/08/25 18:58:12 johnh Exp $ (NCSU/IBM)";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "ip.h"
#include "tcp.h"
#include "flags.h"

#ifndef MIN
#define MIN(x, y) ((x)<(y) ? (x) : (y))
#endif /* ! MIN */

#if 0
#define RBP_DEBUG_PRINTF(x) printf x
#else /* ! 0 */
#define RBP_DEBUG_PRINTF(x)
#endif /* 0 */


#define RBP_MIN_SEGMENTS 2

class RBPVegasTcpAgent;

class RBPVegasPaceTimer : public TimerHandler {
public:
	RBPVegasPaceTimer(RBPVegasTcpAgent *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	RBPVegasTcpAgent *a_;
};
// Hmmm... ``a is a'' in the construction of the RBPVegasPaceTimer edifice :->

class RBPVegasTcpAgent : public virtual VegasTcpAgent {
	friend class RBPVegasPaceTimer;
 public:
	RBPVegasTcpAgent();
	virtual void recv(Packet *pkt, Handler *);
	virtual void timeout(int tno);
	virtual void send_much(int force, int reason, int maxburst);

	double rbp_scale_;   // conversion from actual -> rbp send rates
	enum rbp_rate_algorithms { RBP_NO_ALGORITHM, RBP_VEGAS_RATE_ALGORITHM, RBP_CWND_ALGORITHM };
	int rbp_rate_algorithm_;
protected:
	void paced_send_one();
	int able_to_rbp_send_one();

	// stats on what we did
	int rbp_segs_actually_paced_;

	enum rbp_modes { RBP_GOING, RBP_POSSIBLE, RBP_OFF };
	enum rbp_modes rbp_mode_;
	double rbp_inter_pace_delay_;
	RBPVegasPaceTimer pace_timer_;
};

static class RBPVegasTcpClass : public TclClass {
public:
	RBPVegasTcpClass() : TclClass("Agent/TCP/Vegas/RBP") {}
	TclObject* create(int, const char*const*) {
		return (new RBPVegasTcpAgent());
	}
} class_vegas_rbp;


void RBPVegasPaceTimer::expire(Event *) { a_->paced_send_one(); }


RBPVegasTcpAgent::RBPVegasTcpAgent() : VegasTcpAgent(),
	rbp_mode_(RBP_OFF),
	pace_timer_(this)
{
	bind("rbp_scale_", &rbp_scale_);
	bind("rbp_rate_algorithm_", &rbp_rate_algorithm_);
	bind("rbp_segs_actually_paced_", &rbp_segs_actually_paced_);
	bind("rbp_inter_pace_delay_", &rbp_inter_pace_delay_);
}

void
RBPVegasTcpAgent::recv(Packet *pkt, Handler *hand)
{
	if (rbp_mode_ != RBP_OFF) {
		// reciept of anything disables rbp
		rbp_mode_ = RBP_OFF;
		// Vegas takes care of cwnd.
	};
	VegasTcpAgent::recv(pkt, hand);
}

void
RBPVegasTcpAgent::timeout(int tno)
{
	if (tno == TCP_TIMER_RTX) {
		if (highest_ack_ == maxseq_) {
			// Idle for a while => RBP next time.
			rbp_mode_ = RBP_POSSIBLE;
			return;
		};
	};
	VegasTcpAgent::timeout(tno);
}

void
RBPVegasTcpAgent::send_much(int force, int reason, int maxburst)
{
	if (rbp_mode_ == RBP_POSSIBLE && able_to_rbp_send_one()) {
		// start paced mode
		rbp_mode_ = RBP_GOING; 
		rbp_segs_actually_paced_ = 0;
		double rbwin_vegas;
		switch (rbp_rate_algorithm_) {
		case RBP_VEGAS_RATE_ALGORITHM:
			// Try to follow tcp_output.c here
			// Calculate the vegas window as its reported rate
			// times the rtt.
			rbwin_vegas = v_actual_ * v_rtt_;
			RBP_DEBUG_PRINTF(("-----------------\n"));
			RBP_DEBUG_PRINTF(("rbwin_vegas = %g\nv_actual = %g\nv_rtt =%g\nbase_rtt=%g\n",
					  rbwin_vegas, v_actual_, v_rtt_, v_baseRTT_));
			// Smooth the vegas window
			rbwin_vegas *= rbp_scale_;
			break;
		case RBP_CWND_ALGORITHM:
			// Pace out scaled cwnd.
			rbwin_vegas = cwnd_ * rbp_scale_;
			break;
		default:
			// quiet the compiler.
			rbwin_vegas = 0.0;
			abort();
		};
		rbwin_vegas = int(rbwin_vegas + 0.5);   // round
		// Always pace at least RBP_MIN_SEGMENTS
		if (rbwin_vegas <= RBP_MIN_SEGMENTS) {
			rbwin_vegas = RBP_MIN_SEGMENTS;
		};

		// Conservatively set the congestion window to min of
		// congestion window and the smoothed rbwin_vegas
		RBP_DEBUG_PRINTF(("cwnd before check = %g\n", double(cwnd_)));
		cwnd_ = MIN(cwnd_,(TracedDouble) rbwin_vegas);
		RBP_DEBUG_PRINTF(("cwnd after check = %g\n", double(cwnd_)));
		RBP_DEBUG_PRINTF(("recv win = %g\n", wnd_));
		// RBP timer calculations must be based on the actual
		// window which is the min of the receiver's
		// advertised window and the congestion window.
		// TcpAgent::window() does this job.
		// What this means is we expect to send window() pkts
		// in v_rtt_ time.
		rbp_inter_pace_delay_ = (v_rtt_)/(window() * 1.0);
		RBP_DEBUG_PRINTF(("window is %d\n", window()));
		RBP_DEBUG_PRINTF(("ipt = %g\n", rbp_inter_pace_delay_));
		paced_send_one();
	} else {
		VegasTcpAgent::send_much(force,reason, maxburst);
	}
}

void
RBPVegasTcpAgent::paced_send_one()
{
	if (rbp_mode_ == RBP_GOING && able_to_rbp_send_one()) {
		RBP_DEBUG_PRINTF(("Sending one rbp packet\n"));
		// send one packet
		output(t_seqno_++, TCP_REASON_RBP);
		rbp_segs_actually_paced_++;
		// schedule next pkt
		pace_timer_.resched(rbp_inter_pace_delay_);
	};
}

int
RBPVegasTcpAgent::able_to_rbp_send_one()
{
	return t_seqno_ < curseq_ && t_seqno_ <= highest_ack_ + window();
}


/***********************************************************************
 *
 * The reno-based version
 *
 */


class RBPRenoTcpAgent;

class RBPRenoPaceTimer : public TimerHandler {
public:
	RBPRenoPaceTimer(RBPRenoTcpAgent *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	RBPRenoTcpAgent *a_;
};
// Hmmm... ``a is a'' in the construction of the RBPRenoPaceTimer edifice :->

class RBPRenoTcpAgent : public virtual RenoTcpAgent {
	friend class RBPRenoPaceTimer;
 public:
	RBPRenoTcpAgent();
	virtual void recv(Packet *pkt, Handler *);
	virtual void timeout(int tno);
	virtual void send_much(int force, int reason, int maxburst);

	double rbp_scale_;   // conversion from actual -> rbp send rates
	// enum rbp_rate_algorithms { RBP_NO_ALGORITHM, RBP_VEGAS_RATE_ALGORITHM, RBP_CWND_ALGORITHM };
	// int rbp_rate_algorithm_;
protected:
	void paced_send_one();
	int able_to_rbp_send_one();

	// stats on what we did
	int rbp_segs_actually_paced_;

	enum rbp_modes { RBP_GOING, RBP_POSSIBLE, RBP_OFF };
	enum rbp_modes rbp_mode_;
	double rbp_inter_pace_delay_;
	RBPRenoPaceTimer pace_timer_;
};

static class RBPRenoTcpClass : public TclClass {
public:
	RBPRenoTcpClass() : TclClass("Agent/TCP/Reno/RBP") {}
	TclObject* create(int, const char*const*) {
		return (new RBPRenoTcpAgent());
	}
} class_reno_rbp;


void RBPRenoPaceTimer::expire(Event *) { a_->paced_send_one(); }

RBPRenoTcpAgent::RBPRenoTcpAgent() : TcpAgent(),
	rbp_mode_(RBP_OFF),
	pace_timer_(this)
{
	bind("rbp_scale_", &rbp_scale_);
	// algorithm is not used in Reno
	// bind("rbp_rate_algorithm_", &rbp_rate_algorithm_);
	bind("rbp_segs_actually_paced_", &rbp_segs_actually_paced_);
	bind("rbp_inter_pace_delay_", &rbp_inter_pace_delay_);
}

void
RBPRenoTcpAgent::recv(Packet *pkt, Handler *hand)
{
	if (rbp_mode_ != RBP_OFF) {
		// reciept of anything disables rbp
		rbp_mode_ = RBP_OFF;

		// reset cwnd such that we're now ack clocked.
		hdr_tcp *tcph = hdr_tcp::access(pkt);
		if (tcph->seqno() > last_ack_) {
			/* reno does not do rate adjustments as Vegas;
			 * normally, one wouldn't do any adjustments to
			 * cwnd and allow the sliding window to do its job
			 * But, if cwnd >> amt_paced, then there's a
			 * bunch of data that can be sent asap, plus the
			 * two (typically, due to delacks) that get opened
			 * up due to the first ack. This would lead to
			 * a burst, defeating the purpose of pacing.
			 * Ideally, one would want cwnd = amt_paced
			 * ALWAYS. Since this doesn't necessarily happen,
			 * `cap' cwnd to the amt paced and THEN let
			 * sliding windows take over. Note that this
			 * mechanism will typically result in 3 segs
			 * being sent out when the first ack is received.
			 */
			cwnd_ = maxseq_ - last_ack_;
			RBP_DEBUG_PRINTF(("\ncwnd-after-first-ack=%g\n", (double)cwnd_));
		};

	};
	RenoTcpAgent::recv(pkt, hand);
}

void
RBPRenoTcpAgent::timeout(int tno)
{
	if (tno == TCP_TIMER_RTX) {
		if (highest_ack_ == maxseq_) {
			// Idle for a while => RBP next time.
			rbp_mode_ = RBP_POSSIBLE;
			return;
		};
	};
	RenoTcpAgent::timeout(tno);
}

void
RBPRenoTcpAgent::send_much(int force, int reason, int maxburst)
{
	if (rbp_mode_ == RBP_POSSIBLE && able_to_rbp_send_one()) {
		// start paced mode
		rbp_mode_ = RBP_GOING; 
		rbp_segs_actually_paced_ = 0;

		// Pace out scaled cwnd.
		double rbwin_reno;
		rbwin_reno = cwnd_ * rbp_scale_;

		rbwin_reno = int(rbwin_reno + 0.5);   // round
		// Always pace at least RBP_MIN_SEGMENTS
		if (rbwin_reno <= RBP_MIN_SEGMENTS) {
			rbwin_reno = RBP_MIN_SEGMENTS;
		};

		// Conservatively set the congestion window to min of
		// congestion window and the smoothed rbwin_reno
		RBP_DEBUG_PRINTF(("cwnd before check = %g\n", double(cwnd_)));
		cwnd_ = MIN(cwnd_,(TracedDouble) rbwin_reno);
		RBP_DEBUG_PRINTF(("cwnd after check = %g\n", double(cwnd_)));
		RBP_DEBUG_PRINTF(("recv win = %g\n", wnd_));
		// RBP timer calculations must be based on the actual
		// window which is the min of the receiver's
		// advertised window and the congestion window.
		// TcpAgent::window() does this job.
		// What this means is we expect to send window() pkts
		// in v_srtt_ time.
		static double srtt_scale = 0.0;
		if (srtt_scale == 0.0) {  // yuck yuck yuck!
			srtt_scale = 1.0; // why are we doing fixed point?
			int i;
			for (i = T_SRTT_BITS; i > 0; i--) {
				srtt_scale /= 2.0;
			};
		}
		rbp_inter_pace_delay_ = (t_srtt_ * srtt_scale * tcp_tick_) / (window() * 1.0);
		RBP_DEBUG_PRINTF(("window is %d\n", window()));
		RBP_DEBUG_PRINTF(("ipt = %g\n", rbp_inter_pace_delay_));
		paced_send_one();
	} else {
		RenoTcpAgent::send_much(force,reason, maxburst);
	};
}

void
RBPRenoTcpAgent::paced_send_one()
{
	if (rbp_mode_ == RBP_GOING && able_to_rbp_send_one()) {
		RBP_DEBUG_PRINTF(("Sending one rbp packet\n"));
		// send one packet
		output(t_seqno_++, TCP_REASON_RBP);
		rbp_segs_actually_paced_++;
		// schedule next pkt
		pace_timer_.resched(rbp_inter_pace_delay_);
	};
}

int
RBPRenoTcpAgent::able_to_rbp_send_one()
{
	return t_seqno_ < curseq_ && t_seqno_ <= highest_ack_ + window();
}
