/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1990, 1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Lawrence Berkeley Laboratory,
 * Berkeley, CA.  The name of the University may not be used to
 * endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
 * This is TCP SACK with Rate-Halving, contributed code from Matt
 * Mathis, Jeff Semke, Jamshid Mahdavi, and Kevin Lahey, and
 * described at "http://www.psc.edu/networking/rate-halving/". 
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/tcp/tcp-sack-rh.cc,v 1.7 2011/10/02 22:32:35 tom_henderson Exp $ (PSC-SACKRH)";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "ip.h"
#include "tcp.h"
#include "flags.h"
#include "scoreboard-rh.h"
#include "random.h"

#define TRUE    1
#define FALSE   0

#define RH_INCR 0
#define RH_EXACT 11
#define RH_EST 12
#define RH_EST_REPAIR 13

#define RHEH_NONE 0
#define RHEH_COUNTING 1
#define RHEH_NEW_HOLE 2
#define RHEH_RETRANS_OCCURRED 3

//#define DEBUGRH 

class SackRHTcpAgent : public TcpAgent {
 public:
        SackRHTcpAgent();
	virtual int window();
	virtual void recv(Packet *pkt, Handler*);
	virtual void timeout(int tno);
	void plot();
	virtual void send_much(int force, int reason, int maxburst);
	virtual void boundparms();
	virtual void estadjust();
	virtual void rhclear();
	virtual void computefack();
	virtual void newack(Packet* pkt);
 protected:
	int fack_;	          /* the FACK state variable  */
	int retran_data_;         /* the number of retransmitted packets in the pipe  */
	int num_dupacks_;         /* sigh, I think we still want this for the NewReno case */
	int rh_state_;            /* The rate halving state we are in  */
	double prior_cwnd_;       /* The value of cwnd at the start of RH  */
	int prior_max_seq_;       /* The old value of max_seq  */
	int rh_id_;               /* A unique ID for the current adjustment interval  */
	int rh_max_;              /* The max ID which has been used so far  */
	int rh_est_hole_state_;   /* Status of NewReno style holes during estimation */
	int num_retrans_;         /* Number of retransmissions during the estimation state */
	int rh_retran_flag_;      /* Flag to indicate a retransmission has occured */
	int rh_ecn_flag_;         /* Flag to indicate an ECN was seen during this recovery */
	ScoreBoardRH scb_;
};

static class SackRHTcpClass : public TclClass {
public:
	SackRHTcpClass() : TclClass("Agent/TCP/SackRH") {}
	TclObject* create(int, const char*const*) {
		return (new SackRHTcpAgent());
	}
} class_sack_rh;

int SackRHTcpAgent::window()
{
	return(cwnd_ < wnd_ ? (int) cwnd_ : (int) wnd_);
}

SackRHTcpAgent::SackRHTcpAgent() : fack_(-1), 
	retran_data_(0), num_dupacks_(0), rh_state_(RH_INCR), rh_id_(0), rh_max_(0),
	rh_est_hole_state_(0), rh_retran_flag_(0), rh_ecn_flag_(0), scb_(&numdupacks_)
{
}

void SackRHTcpAgent::recv(Packet *pkt, Handler*)
{
	int old_fack, old_retran_data, old_ack;
	hdr_tcp *tcph = hdr_tcp::access(pkt);

	int ecnecho = (hdr_flags::access(pkt)->ecnecho() && last_ack_ != -1);
	int sackpresent = (tcph->sa_length() > 0);

	++nackpack_;

#ifdef DEBUGRH
	printf ("ENTER: RH=%02d snd.nxt=%3d fack=%3d ack=%3d retran_data=%03d \n",
		rh_state_, (int)t_seqno_, fack_, last_ack_, retran_data_);
	printf ("       prior_max_seq=%3d prior_cwnd=%f rheh=%2d num_dupacks=%2d\n",
		prior_max_seq_, prior_cwnd_, rh_est_hole_state_, num_dupacks_);
	printf ("       cwnd:%f=%d curseq=%d\n",
		(float)cwnd_, int(cwnd_), (int)curseq_);
#endif
	old_ack = last_ack_;
	newack(pkt);

	if (ecnecho) rh_ecn_flag_=1;   /*  Remember that we saw an ECN  */

	/*  Process any SACK blocks:  */
	old_fack = fack_;
	old_retran_data = retran_data_;
	retran_data_ -= scb_.UpdateScoreBoard (last_ack_, tcph, rh_id_);

       	if (!sackpresent && (old_ack == last_ack_)) {
		num_dupacks_++;
		if ((rh_est_hole_state_ == RHEH_COUNTING) && 
		    (num_dupacks_ == numdupacks_)) {
			rh_est_hole_state_ = RHEH_NEW_HOLE;
		}
	}

	computefack();


	/*  7.8  Reordering Exit transitions */
	if (!rh_retran_flag_ && !rh_ecn_flag_) {
		if (((rh_state_ == RH_EXACT) && !sackpresent && !ecnecho) ||
		    ((rh_state_ == RH_EST) && (last_ack_ > old_ack))) {
			rh_state_ = RH_INCR;
			cwnd_ = prior_cwnd_;
			rhclear();
			computefack();  /* In case we were estimating it before */
		}
	}

	/*  7.10 Completion of RH_EXACT  */
	if (rh_state_ == RH_EXACT) {
		if ((fack_ >= prior_max_seq_) || scb_.RetranSacked(rh_id_)) {
			boundparms();
			rh_state_ = RH_INCR;
			rhclear();
		}
	}

	/*  7.11 Completion of RH_EST*  */
	if ((rh_state_ == RH_EST) || (rh_state_ == RH_EST_REPAIR)) {
		if (last_ack_ >= prior_max_seq_) {
			retran_data_ = 0;     /*  Just like a partial ACK, this
						  means a retransmission has left
						  the network.  */
			estadjust();
			boundparms();
			rh_state_ = RH_INCR;
			rhclear();
			computefack();   /*  This is needed to restore 
					     fack now that we are done estimating */
		}
	}

	/*  7.12 NewReno case  */
	if ((rh_state_ == RH_EST) && (cwnd_ <= prior_cwnd_/2)) {
		rh_state_ = RH_EST_REPAIR;
	}

	/*  7.9  Processing partial Acks in RH_EST or RH_EST_REPAIR */
	if (((rh_state_ == RH_EST) || (rh_state_ == RH_EST_REPAIR))
	    && (last_ack_ > old_ack)) {
		retran_data_ = 0;
		num_dupacks_ -= (last_ack_ - old_ack - 1);
		if (num_dupacks_ >= numdupacks_) {
			rh_est_hole_state_ = RHEH_NEW_HOLE;
		}
		else {
			/*  We don't have 3 dupacks on this hole, so 
			    go back to counting...  */
			rh_est_hole_state_ = RHEH_COUNTING;
		}
		computefack();  /* Recompute the estimate */
	}
		
	/*  Now check the entry conditions:  */

	/*  7.4  Entering RH_EXACT  */
	if ((rh_state_ == RH_INCR) && (ecnecho || scb_.GetNewHoles() != 0)) {
		rh_state_ = RH_EXACT;
		prior_cwnd_ = cwnd_;
		prior_max_seq_ =  t_seqno_;
		cong_action_ = TRUE;
		rh_id_ = ++rh_max_;
	}

	/*  7.5  Entering RH_EST  */
	if ((rh_state_ == RH_INCR)  && (num_dupacks_ > 0)) {
		rh_state_ = RH_EST;
		rh_est_hole_state_ = RHEH_COUNTING;
		prior_cwnd_ = cwnd_;
		prior_max_seq_ =  t_seqno_;
		cong_action_ = TRUE;
		rh_id_ = ++rh_max_;
	}
		
	if ((rh_state_ == RH_EXACT)  && (num_dupacks_ > 0)) {
		rh_state_ = RH_EST;
		rh_est_hole_state_ = RHEH_COUNTING;
		/*  cong_action_ = TRUE;  */   /*  Don't need it, 
		                                   since we already
						   sent it  */
		rh_id_ = ++rh_max_;
	}

	if ((rh_state_ == RH_EST_REPAIR)  && (ecnecho)) {
		rh_state_ = RH_EST;
		prior_cwnd_ = cwnd_;
		prior_max_seq_ =  t_seqno_;
		cong_action_ = TRUE;
		rh_id_ = ++rh_max_;
	}

	/*  Updating the window:  */
	switch (rh_state_) {
	case RH_INCR:
		/*  Case 7.2:  */
		if ((t_seqno_ - old_fack + old_retran_data + 1) >= (int)cwnd_) {
			opencwnd();
		}
		break;
	case RH_EXACT:
		/*  Case 7.6  */
		cwnd_ -= ((float)((fack_ - old_fack) + (scb_.GetNewHoles()))) / 2.0;
		break;
	case RH_EST:
		/*  Case 7.7  */
		cwnd_ -= 0.5;
		break;
	case RH_EST_REPAIR:
		/*  Do not modify cwnd in this state  */
		break;
	}

#ifdef DEBUGRH
	printf ("EXIT:  RH=%02d snd.nxt=%3d fack=%3d ack=%3d retran_data=%03d \n       cwnd:%f=%d curseq=%d\n",
		rh_state_, (int)t_seqno_, fack_, last_ack_, retran_data_, (float)cwnd_, int(cwnd_),
	        (int)curseq_);
#endif
	send_much(FALSE, 0, maxburst_);

	Packet::free(pkt);
#ifdef notyet
	if (trace_)
		plot();
#endif
}

void SackRHTcpAgent::rhclear()
{
	rh_id_ = prior_max_seq_ = num_dupacks_ = rh_est_hole_state_ = 
		rh_ecn_flag_ = num_retrans_ = rh_retran_flag_ = 0;
	prior_cwnd_ = 0;
}

/*  Compute FACK:  */
void SackRHTcpAgent::computefack()
{
	if ((rh_state_ == RH_INCR) || (rh_state_ == RH_EXACT)) {
		fack_ = scb_.GetFack (last_ack_);
	}
	else {
		fack_ = last_ack_ + num_dupacks_ + 1;
	}
}

/*  7.14: Bounding Parameters  */
void SackRHTcpAgent::boundparms()
{
	if (cwnd_ > prior_cwnd_/2.0) 
		cwnd_ = prior_cwnd_/2.0;
	ssthresh_ = cwnd_;
	if (ssthresh_ < prior_cwnd_/4.0)
		ssthresh_ = (int) (prior_cwnd_/4.0);
}

/*  7.13: Corrections to Estimation  */
void SackRHTcpAgent::estadjust()
{
	cwnd_ = (prior_cwnd_ - num_retrans_)/2.0;
}

/*  7.15: Retransmission Timeout  */
void SackRHTcpAgent::timeout(int tno)
{
	/* retransmit timer */
	if (tno == TCP_TIMER_RTX) {
		if (highest_ack_ == maxseq_ && !slow_start_restart_)
			/*
			 * TCP option:
			 * If no outstanding data, then don't do anything.
			 */
			return;
		if (highest_ack_ == -1 && wnd_init_option_ == 2)
			/* 
			 * First packet dropped, so don't use larger
			 * initial windows. 
			 */
			wnd_init_option_ = 1;

#ifdef DEBUGRH
		printf ("timeout. last_ack: %d seqno: %d\n", 
			(int)last_ack_, (int)t_seqno_);
#endif

		if (rh_state_ != RH_INCR) {
			/*  4.6  We took a timeout in the middle of
			    rate-halving.  Pretend like RH
			    never started:  */
			cwnd_ = prior_cwnd_;
			rhclear();
			rh_state_ = RH_INCR;
			
		}
		++nrexmit_;
		slowdown(CLOSE_SSTHRESH_HALF|CLOSE_CWND_RESTART);

		retran_data_ = 0;
		scb_.TimeoutScoreBoard(t_seqno_);
		computefack();

		/*  Pull back the sequence number -- pretend you never
		    sent packets beyond FACK  */
		if (t_seqno_ > fack_+1) {
			t_seqno_ = fack_+1;
		}
		reset_rtx_timer(1,1);

		last_cwnd_action_ = CWND_ACTION_TIMEOUT;
		send_much(0, TCP_REASON_TIMEOUT, maxburst_);
	} 
	else {
		timeout_nonrtx(tno);
	}
}


void SackRHTcpAgent::send_much(int force, int reason, int maxburst)
{
	register int found, npacket = 0;
	int xmit_seqno;

	found = 1;
	if (!force && delsnd_timer_.status() == TIMER_PENDING)
		return;
	/*
	 * as long as the pipe is open and there is app data to send...
	 */

	/*  7.3: data transmission  */
	while (int(cwnd_) >= (t_seqno_ - fack_ + retran_data_)
	        && found) {
		if (overhead_ == 0 || force) {
			found = 0;
			xmit_seqno = scb_.GetNextRetran ();

			if (xmit_seqno == -1) { 
				/*  Check to see if we have identified a new hole... */
				if (((rh_state_ == RH_EST) || (rh_state_ == RH_EST_REPAIR)) &&
				    (rh_est_hole_state_ == RHEH_NEW_HOLE)) {
					xmit_seqno = last_ack_ + 1;
					rh_est_hole_state_ = RHEH_RETRANS_OCCURRED;
					num_retrans_++;
					retran_data_++;
					rh_retran_flag_ = 1;
					found = 1;
				}
				else if ((t_seqno_ < curseq_) && 
					 (t_seqno_<=last_ack_+int(wnd_))) {
					found = 1;
					xmit_seqno = t_seqno_++;
				}
			} else if (xmit_seqno<=last_ack_+int(wnd_)) {
				found = 1;
				scb_.MarkRetran (xmit_seqno, t_seqno_, rh_id_);
				retran_data_++;
				rh_retran_flag_ = 1;
			}
			if (found) {
#ifdef DEBUGRH
				printf ("      Sending: %3d\n", xmit_seqno);
#endif				
				output(xmit_seqno, reason);
				if (t_seqno_ <= xmit_seqno)
					t_seqno_ = xmit_seqno + 1;
				npacket++;
			}
		} else if (!(delsnd_timer_.status() == TIMER_PENDING)) {
			/*
			 * Set a delayed send timeout.
			 * This is only for the simulator,to add some
			 *   randomization if speficied.
			 */
			delsnd_timer_.resched(Random::uniform(overhead_));
			return;
		}
		if (maxburst && npacket == maxburst)
			break;
	} /* while */
}

void SackRHTcpAgent::plot()
{
#ifdef notyet
	double t = Scheduler::instance().clock();
	sprintf(trace_->buffer(), "t %g %d rtt %g\n", 
		t, class_, t_rtt_ * tcp_tick_);
	trace_->dump();
	sprintf(trace_->buffer(), "t %g %d dev %g\n", 
		t, class_, t_rttvar_ * tcp_tick_);
	trace_->dump();
	sprintf(trace_->buffer(), "t %g %d win %f\n", t, class_, cwnd_);
	trace_->dump();
	sprintf(trace_->buffer(), "t %g %d bck %d\n", t, class_, t_backoff_);
	trace_->dump();
#endif
}

/*
 * Process a packet that acks previously unacknowleged data.
 *
 * The only thing we change here is the policy for clearing
 * RTO backoff...
 *
 */

void SackRHTcpAgent::newack(Packet* pkt)
{
	double now = Scheduler::instance().clock();
	hdr_tcp *tcph = hdr_tcp::access(pkt);
	/* 
	 * Wouldn't it be better to set the timer *after*
	 * updating the RTT, instead of *before*? 
	 */
	newtimer(pkt);
	last_ack_ = tcph->seqno();
	highest_ack_ = last_ack_;

	if (t_seqno_ < last_ack_ + 1)
		t_seqno_ = last_ack_ + 1;
	/* 
	 * Update RTT only if it's OK to do so from info in the flags header.
	 * This is needed for protocols in which intermediate agents
	 * in the network intersperse acks (e.g., ack-reconstructors) for
	 * various reasons (without violating e2e semantics).
	 */	
	hdr_flags *fh = hdr_flags::access(pkt);
	if (!fh->no_ts_) {
		if (ts_option_)
			rtt_update(now - tcph->ts_echo());

		if (rtt_active_ && tcph->seqno() >= rtt_seq_) {
			if (!hdr_flags::access(pkt)->ecnecho()) {
				/* 
				 * Don't end backoff if there is still an ECN-Echo
				 * on the packet.
				 */
				t_backoff_ = 1;
			}
			rtt_active_ = 0;
			if (!ts_option_)
				rtt_update(now - rtt_ts_);
		}
	}
	/* update average window */
	awnd_ *= 1.0 - wnd_th_;
	awnd_ += wnd_th_ * cwnd_;
}
