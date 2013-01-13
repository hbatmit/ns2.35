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

#ifndef lint
static const char rcsid[] =
    "@(#) /home/ctk21/cvsroot//hssstcp/ns/ns-2.1b9/tcp/tcp-fack.cc,v 1.2 2002/08/12 10:43:58 ctk21 Exp (PSC)";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "ip.h"
#include "tcp.h"
#include "flags.h"
#include "scoreboard.h"
#include "random.h"
#include "tcp-fack.h"
#include "template.h"

static class FackTcpClass : public TclClass {
public:
	FackTcpClass() : TclClass("Agent/TCP/Fack") {}
	TclObject* create(int, const char*const*) {
		return (new FackTcpAgent());
	}
} class_fack;


FackTcpAgent::FackTcpAgent() : 	timeout_(FALSE), wintrim_(0),
	wintrimmult_(.5), rampdown_(0), fack_(-1), retran_data_(0),
	ss_div4_(0)		// What about fastrecov_ and scb_
{
	bind_bool("ss-div4_", &ss_div4_);
	bind_bool("rampdown_", &rampdown_);
	scb_ = new ScoreBoard(new ScoreBoardNode[SBSIZE],SBSIZE);
}

FackTcpAgent::~FackTcpAgent(){
	delete [] scb_;
}

int FackTcpAgent::window() 
{
	int win;
	win = int((cwnd_ < wnd_ ? (double) cwnd_ : (double) wnd_) + wintrim_);
	return (win);
}

void FackTcpAgent::reset ()
{
	scb_->ClearScoreBoard();
	TcpAgent::reset ();
}

/*
 * Process a dupack.
 */
void FackTcpAgent::oldack(Packet* pkt)
{
	hdr_tcp *tcph = hdr_tcp::access(pkt);

	last_ack_ = tcph->seqno();
	highest_ack_ = last_ack_;
	fack_ = max(fack_,highest_ack_);
	/* 
	 * There are conditions under which certain versions of TCP (e.g., tcp-fs)
	 * retract maxseq_. The following line of code helps in those cases. For
	 * versions of TCP, it is a NOP.
*/
	maxseq_ = max(maxseq_, highest_ack_);
	if (t_seqno_ < last_ack_ + 1)
		t_seqno_ = last_ack_ + 1;
	newtimer(pkt);
	if (rtt_active_ && tcph->seqno() >= rtt_seq_) { 
		rtt_active_ = 0;
		t_backoff_ = 1;
	}
	/* with timestamp option */
	double tao = Scheduler::instance().clock() - tcph->ts_echo();
	rtt_update(tao);
	if (ts_resetRTO_) {
		// From Andrei Gurtov
		// FACK has not been updated to make sure the ECN works
		//   correctly in this case - Sally.
		t_backoff_ = 1;
	}
	/* update average window */
	awnd_ *= 1.0 - wnd_th_;
	awnd_ += wnd_th_ * cwnd_;
	/* if the connection is done, call finish() */
	if ((highest_ack_ >= curseq_-1) && !closed_) {
		closed_ = 1;
		finish();
	}
}


int FackTcpAgent::maxsack(Packet *pkt)
{
	hdr_tcp *tcph = hdr_tcp::access(pkt);
	int maxsack=-1, sack_index; 

	for (sack_index=0; sack_index < tcph->sa_length(); sack_index++) {
		if (tcph->sa_right(sack_index) > maxsack)
			maxsack = tcph->sa_right(sack_index);
	}
	return (maxsack-1);
}


void FackTcpAgent::recv_newack_helper(Packet *pkt) {
	newack(pkt);
	opencwnd();
	/* if the connection is done, call finish() */
	if ((highest_ack_ >= curseq_-1) && !closed_) {
		closed_ = 1;
		finish();
	}
}

void FackTcpAgent::recv(Packet *pkt, Handler*)
{
	hdr_tcp *tcph = hdr_tcp::access(pkt);
	int ms; 

#ifdef notdef
	if (pkt->type_ != PT_ACK) {
		Tcl::instance().evalf("%s error \"received non-ack\"",
				      name());
		Packet::free(pkt);
		return;
	}
#endif

	ts_peer_ = tcph->ts();
	if (hdr_flags::access(pkt)->ecnecho() && ecn_)
		ecn(tcph->seqno());
	recv_helper(pkt);

	if (!fastrecov_) {	// Not in fast recovery 
		if ((int)tcph->seqno() > last_ack_ && tcph->sa_length() == 0) {
			/*
			 * regular ACK not in fast recovery... normal
			 */
			recv_newack_helper(pkt);
			fack_ = last_ack_;
			timeout_ = FALSE;
			scb_->ClearScoreBoard();
			retran_data_ = 0;
			wintrim_ = 0;
		} else if ((int)tcph->seqno() < last_ack_) {
			// Do nothing; ack may have been misordered

		} else {
			retran_data_ -= scb_->UpdateScoreBoard (highest_ack_, tcph);
			oldack(pkt);
			ms = maxsack(pkt);
			if (ms > fack_)
				fack_ = ms;
			if (fack_ >= t_seqno_)
				t_seqno_ = fack_ + 1;
			dupacks_ = (fack_ - last_ack_) - 1;
			/* 
			 * a duplicate ACK
			 */
			if (dupacks_ >= numdupacks_) {
				/*
				 * Assume we dropped just one packet.
				 * Retransmit last ack + 1
				 * and try to resume the sequence.
				 */
				recover_ = t_seqno_;
				last_cwnd_action_ = CWND_ACTION_DUPACK;
				if ((ss_div4_ == 1) && (cwnd_ <= ssthresh_ + .5)) {
					cwnd_ /= 2;
					wintrimmult_ = .75;
				} else {
					wintrimmult_ = .5;
				}
				slowdown(CLOSE_SSTHRESH_HALF|CLOSE_CWND_RESTART);

				if (rampdown_) {
					wintrim_ = (t_seqno_ - fack_ - 1) * wintrimmult_;
				}
				reset_rtx_timer(1,0);
				fastrecov_ = TRUE;
				scb_->MarkRetran (last_ack_+1, t_seqno_);
				retran_data_++;
				output(last_ack_ + 1, TCP_REASON_DUPACK);
			}
		}
		if (dupacks_ == 0)
			send_much(FALSE, 0, maxburst_);
	} else {
		// we are in fast recovery
		oldack(pkt);
		ms = maxsack(pkt);
		if (ms > fack_) {
			if (rampdown_) {
				wintrim_ -= ((float)ms - (float)fack_)* wintrimmult_;
				if (wintrim_< 0) 
					wintrim_ = 0;
			}
			fack_ = ms;
		}		
		
		if (fack_ >= t_seqno_)
			t_seqno_ = fack_ + 1;

		retran_data_ -= scb_->UpdateScoreBoard (highest_ack_, tcph);
	
		// If the retransmission was lost again, timeout_ forced to TRUE
		// if timeout_ TRUE, this shuts off send()
		timeout_ |= scb_->CheckSndNxt (tcph);
		
		opencwnd();

		if (retran_data_ < 0) {
			printf("Error, retran_data_ < 0");
		}
				
		if ((int)tcph->sa_length() == 0 && (last_ack_ >= recover_)) {
			// No SACK blocks indicates fast recovery is over
			fastrecov_ = FALSE;
			timeout_ = FALSE;
			scb_->ClearScoreBoard();
			retran_data_ = 0;
			wintrim_ = 0;
			dupacks_ = 0;
		}
		send_much(FALSE, 0, maxburst_);
	}

	Packet::free(pkt);
#ifdef notyet
	if (trace_)
		plot();
#endif
}

void FackTcpAgent::timeout(int tno)
{
	if (tno == TCP_TIMER_RTX) {
		if (highest_ack_ == maxseq_ && !slow_start_restart_) {
			/*
			 * TCP option:
			 * If no outstanding data, then don't do anything.
			 */
			return;
		};
		// Do not clear fastrecov_ or alter recover_ variable
		timeout_ = FALSE;
		if (highest_ack_ > last_ack_)
			last_ack_ = highest_ack_;
#ifdef DEBUGSACK1A
		printf ("timeout. highest_ack: %d seqno: %d\n", 
			highest_ack_, t_seqno_);
#endif
		retran_data_ = 0;
		last_cwnd_action_ = CWND_ACTION_TIMEOUT;
		/* if there is no outstanding data, don't cut down ssthresh_ */
		if (highest_ack_ == maxseq_ && restart_bugfix_) 
			slowdown(CLOSE_CWND_INIT);
		else {
			// close down to 1 segment
			slowdown(CLOSE_SSTHRESH_HALF|CLOSE_CWND_RESTART);
		}
		scb_->ClearScoreBoard();
		/* if there is no outstanding data, don't back off rtx timer */
		if (highest_ack_ == maxseq_)
			reset_rtx_timer(TCP_REASON_TIMEOUT,0);
		else
			reset_rtx_timer(TCP_REASON_TIMEOUT,1);
		fack_ = last_ack_;
		t_seqno_ = last_ack_ + 1;
		send_much(0, TCP_REASON_TIMEOUT);
	} else {
		TcpAgent::timeout(tno);
	}
}


void FackTcpAgent::send_much(int force, int reason, int maxburst)
{
	register int found, npacket = 0;
	send_idle_helper();
	int win = window();
	int xmit_seqno;

	if (!force && delsnd_timer_.status() == TIMER_PENDING)
		return;
	/* 
	 * If TCP_TIMER_BURSTSND is pending, cancel it. The timer is
	 * set again, if necessary, after the maxburst pakts have been
	 * sent out.
*/
	if (burstsnd_timer_.status() == TIMER_PENDING)
		burstsnd_timer_.cancel();
	found = 1;
	/*
* as long as the pipe is open and there is app data to send...
*/
	while (( t_seqno_ <= fack_ + win - retran_data_) && (!timeout_)) {
		if (overhead_ == 0 || force) {
			found = 0;
			xmit_seqno = scb_->GetNextRetran ();
#ifdef DEBUGSACK1A
			printf("highest_ack: %d xmit_seqno: %d timeout: %d seqno: %d fack: % d win: %d retran_data: %d\n",
			       highest_ack_, xmit_seqno, timeout_, t_seqno_, fack_, win, retran_data_);
#endif

			if (xmit_seqno == -1) {  // no retransmissions to send
				/* 
				 * if there is no more application data to send,
				 * do nothing
				 */
				if (t_seqno_ >= curseq_) 
					return;
			        /* if window-limited */
				if (fastrecov_ && 
				      t_seqno_>highest_ack_+int(wnd_)) 
					break;
				found = 1;
				xmit_seqno = t_seqno_++;
#ifdef DEBUGSACK1A
				printf("sending %d fastrecovery: %d win %d\n",
				       xmit_seqno, fastrecov_, win);
#endif
			} else {
				found = 1;
				scb_->MarkRetran (xmit_seqno, t_seqno_);
				retran_data_++;
				win = window();
			}
			if (found) {
				output(xmit_seqno, reason);
				if (t_seqno_ <= xmit_seqno) {
					printf("Hit a strange case 2.\n");
					t_seqno_ = xmit_seqno + 1;
				}
				npacket++;
			}
		} else if (!(delsnd_timer_.status() == TIMER_PENDING)) {
			/*
			 * Set a delayed send timeout.
			 */
			delsnd_timer_.resched(Random::uniform(overhead_));
			return;
		}
		if (maxburst && npacket == maxburst)
			break;
	} /* while */
	/* call helper function */
	send_helper(maxburst);
}

/*
 * open up the congestion window -- Hack hack hack
 */
void FackTcpAgent::opencwnd()
{
	TcpAgent::opencwnd();

	// if maxcwnd_ is set (nonzero), make it the cwnd limit
	if (maxcwnd_ && (int (cwnd_) > maxcwnd_))
		cwnd_ = maxcwnd_;
}


void FackTcpAgent::plot()
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
