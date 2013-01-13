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
 * TCP-Linux module for NS2 
 *
 * May 2006
 *
 * Author: Xiaoliang (David) Wei  (DavidWei@acm.org)
 *
 * NetLab, the California Institute of Technology 
 * http://netlab.caltech.edu
 *
 * Module: tcp-linux.cc 
 *      This is the main NS-2 module for TCP-Linux.
 *
 *
 * See a mini-tutorial about TCP-Linux at: http://netlab.caltech.edu/projects/ns2tcplinux/
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "ip.h"
#include "tcp.h"
#include "flags.h"
#include "random.h"
#include "tcp-linux.h"
#include "template.h"

CongestionControlManager cong_ops_manager;

CongestionControlManager::CongestionControlManager() {
	scan();
};
void CongestionControlManager::scan() {
	struct tcp_congestion_ops *e;
	num_ = 1;
	list_for_each_entry_rcu(e, &ns_tcp_cong_list, list) {
		num_++;
	};
	ops_list = new struct tcp_congestion_ops*[num_];

	ops_list[0] = &tcp_reno;
	int i=1;
	list_for_each_entry_rcu(e, &tcp_cong_list, list) {
		ops_list[i] = e;
		i++;
	}
	cc_list_changed = 0;
	//printf("congestion control algorithm initialized\n");
};

struct tcp_congestion_ops* CongestionControlManager::get_ops(const char* name) {
	if (cc_list_changed) scan();
	for (int i=0; i< num_; i++) {
		if (strcmp(name, ops_list[i]->name)==0)
			return ops_list[i];
	}
	return 0;
};

void CongestionControlManager::dump() {
	printf("List of cc (total %d) :", num_);
	for (int i=0; i< num_; i++) {
		printf(" %s", ops_list[i]->name);
	}
	printf("\n");
};



static class LinuxTcpClass : public TclClass {
public:
	LinuxTcpClass() : TclClass("Agent/TCP/Linux") {}
	TclObject* create(int, const char*const*) {
		return (new LinuxTcpAgent());
	}
} class_linux;


LinuxTcpAgent::LinuxTcpAgent() :
	initialized_(false),
	next_pkts_in_flight_(0)
{
	bind("next_pkts_in_flight_", &next_pkts_in_flight_);
	scb_ = new ScoreBoard1();
	linux_.icsk_ca_ops = NULL;
        linux_.snd_cwnd_stamp = 0;
	linux_.icsk_ca_state = TCP_CA_Open;
	linux_.snd_cwnd = 2;
	linux_.snd_cwnd_cnt = 0;
	linux_.bytes_acked = 0;
	//load_to_linux_once();
//	scb_->test();
}

LinuxTcpAgent::~LinuxTcpAgent(){
	delete scb_;
	remove_congestion_control();
}

int LinuxTcpAgent::window() 
{
	return (linux_.snd_cwnd);
}
double LinuxTcpAgent::windowd()
{
	return (linux_.snd_cwnd);
//+ min((double)linux_.snd_cwnd_cnt/(double)linux_.snd_cwnd, 1));
}
void LinuxTcpAgent::reset()
{
	scb_->ClearScoreBoard();
	linux_.icsk_ca_state = TCP_CA_Open;
	linux_.snd_cwnd_stamp = 0;
	linux_.snd_cwnd = 2;
	linux_.bytes_acked = 0;
	initialized_ = false;

	TcpAgent::reset();

	load_to_linux_once();
	load_to_linux();
}

////////////////// Ack processing part //////////////////////////////
unsigned char LinuxTcpAgent::ack_processing(Packet* pkt, unsigned char flag)
{
	hdr_tcp *tcph = hdr_tcp::access(pkt);
//update sequence numbers
	if (flag&FLAG_DATA_ACKED) {
		highest_ack_ = tcph->seqno();
		linux_.snd_una = (highest_ack_+1)*linux_.mss_cache;
		maxseq_ = max(maxseq_, highest_ack_);
		if (t_seqno_ < highest_ack_ + 1) {
			t_seqno_ = highest_ack_ + 1;
			linux_.snd_nxt = t_seqno_*linux_.mss_cache;
		}
		return flag;
	} else {
		return flag | FLAG_UNSURE_TSTAMP;
	};
}
void LinuxTcpAgent::time_processing(Packet* pkt, unsigned char flag, s32* seq_urtt_p/*, s32* seq_usrtt_p*/)
{
        hdr_tcp *tcph = hdr_tcp::access(pkt);

	#define renew_timer {if (t_seqno_ > highest_ack_|| highest_ack_< maxseq_ || linux_.snd_cwnd < 1) set_rtx_timer(); else cancel_rtx_timer();}
	if (!timerfix_) renew_timer;

//update time:
	double now = Scheduler::instance().clock();
	ts_peer_ = tcph->ts();          //record timestamp for echoing

	/* 
	 * Update RTT only if it's OK to do so from info in the flags header.
	 * This is needed for protocols in which intermediate agents
	 * in the network intersperse acks (e.g., ack-reconstructors) for
	 * various reasons (without violating e2e semantics).
	 */	
	hdr_flags *fh = hdr_flags::access(pkt);
	linux_.rx_opt.saw_tstamp = 0;
	if (!fh->no_ts_) {
		if (ts_option_) {
			ts_echo_ = tcph->ts_echo();
			linux_.rx_opt.saw_tstamp = 1;
			linux_.rx_opt.rcv_tsecr = (__u32) (round(ts_echo_*JIFFY_RATIO));
			linux_.rx_opt.rcv_tsval = (__u32) (round(tcph->ts()*JIFFY_RATIO));
			if (flag & FLAG_UNSURE_TSTAMP)
				// we are not sure which packet generates this ack
				rtt_update(now - tcph->ts_echo(), 0);
			else
				//we are sure this packet generates this ack
				rtt_update(now - tcph->ts_echo(), tcph->seqno());
			(*seq_urtt_p) = (s32)(round((now - tcph->ts_echo())*JIFFY_RATIO));
			/*(*seq_usrtt_p) = (s32)(round((now - tcph->ts_echo())*US_RATIO));*/
			if (ts_resetRTO_) { 
				t_backoff_ = 1;
				if (!ect_ || !ecn_backoff_ || !hdr_flags::access(pkt)->ecnecho()) { 
					// From Andrei Gurtov
					/* 
					 * Don't end backoff if still in ECN-Echo with
				 	 * a congestion window of 1 packet. 
					 */
					ecn_backoff_ = 0;
				}
			}
		}
	}
	if (rtt_active_ && tcph->seqno() >= rtt_seq_) {
		if (!ect_ || !ecn_backoff_ || 
			!hdr_flags::access(pkt)->ecnecho()) {
			/* 
			 * Don't end backoff if still in ECN-Echo with
		 	 * a congestion window of 1 packet. 
			 */
			t_backoff_ = 1;
			ecn_backoff_ = 0;
		}
		rtt_active_ = 0;
		if (!linux_.rx_opt.saw_tstamp)
			rtt_update(now - rtt_ts_, 0);
	}
	

	if (timerfix_) renew_timer;
	/* update average window */
	awnd_ *= 1.0 - wnd_th_;
	awnd_ += wnd_th_ * cwnd_;
	/* if the connection is done, call finish() */
	if ((highest_ack_ >= curseq_-1) && !closed_) {
		closed_ = 1;
		finish();
	}
};

void LinuxTcpAgent::rtt_update(double tao, unsigned long pkt_seq_no)
{
	double now = Scheduler::instance().clock();
	if (ts_option_)
		t_rtt_ = int(tao /tcp_tick_ + 0.5);
	else {
		double sendtime = now - tao;
		sendtime += boot_time_;
		double tickoff = fmod(sendtime, tcp_tick_);
		t_rtt_ = int((tao + tickoff) / tcp_tick_);
	}


	/*Record microsecond timestamp*/
	if ((linux_.icsk_ca_ops) && (linux_.icsk_ca_ops->rtt_sample)) {
		if ( bugfix_ts_ && pkt_seq_no && tss ) {
			//if we use the tss value when we have the timestamp
			
			//unsigned long a = (unsigned long)(round((now-tss[pkt_seq_no % tss_size_])*US_RATIO));
			//unsigned long ha = highest_ack_;
			//if (a < 100)
			//printf("%lf (%p): RTT=%lu, seq:%lu, highest_ack:%lu tss_size: %lu timesamp:%lf\n", now, this, a, pkt_seq_no, ha, tss_size_, tss[pkt_seq_no % tss_size_]);
			
			linux_.icsk_ca_ops->rtt_sample(&linux_, (unsigned long)(round((now-tss[pkt_seq_no % tss_size_])*US_RATIO)));
		} else { 
			//otherwise, use an approximation
			linux_.icsk_ca_ops->rtt_sample(&linux_, (unsigned long)(round(tao*US_RATIO)));
		}
	}

	if (t_rtt_ < 1)
		t_rtt_ = 1;
	//
	// t_srtt_ has 3 bits to the right of the binary point
	// t_rttvar_ has 2
        // Thus "t_srtt_ >> T_SRTT_BITS" is the actual srtt, 
  	//   and "t_srtt_" is 8*srtt.
	// Similarly, "t_rttvar_ >> T_RTTVAR_BITS" is the actual rttvar,
	//   and "t_rttvar_" is 4*rttvar.
	//
        if (t_srtt_ != 0) {
		register short delta;
		delta = t_rtt_ - (t_srtt_ >> T_SRTT_BITS);	// d = (m - a0)
		if ((t_srtt_ += delta) <= 0)	// a1 = 7/8 a0 + 1/8 m
			t_srtt_ = 1;
		if (delta < 0) {
			delta = -delta;
			delta -= (t_rttvar_ >> T_RTTVAR_BITS);
			if (delta>0)
				delta >>= T_SRTT_BITS;
		} else 
			delta -= (t_rttvar_ >> T_RTTVAR_BITS);
		t_rttvar_ += delta;
//		if ((t_rttvar_ += delta) <= 0)	// var1 = 3/4 var0 + 1/4 |d|
//			t_rttvar_ = 1;
	} else {
		t_srtt_ = t_rtt_ << T_SRTT_BITS;		// srtt = rtt
		t_rttvar_ = t_rtt_ << (T_RTTVAR_BITS-1);	// rttvar = rtt / 2
	}
	//
	// Current retransmit value is 
	//    (unscaled) smoothed round trip estimate
	//    plus 2^rttvar_exp_ times (unscaled) rttvar. 
	//
	t_rtxcur_ = (((t_rttvar_ << (rttvar_exp_ + (T_SRTT_BITS - T_RTTVAR_BITS))) +
		t_srtt_)  >> T_SRTT_BITS ) * tcp_tick_;
	linux_.srtt = t_srtt_;
	return;
}

void LinuxTcpAgent::recv(Packet *pkt, Handler*)
//equivalence to tcp_ack
{
	hdr_tcp *tcph = hdr_tcp::access(pkt);

	struct sock* sk = &linux_;
	u32 prior_snd_una = highest_ack_+1;
	u32 ack = tcph->seqno()+1;	//in linux, the concept of unack is one packet ahead (first non-acked)
	u32 prior_in_flight;
	s32 seq_rtt;
	unsigned char flag=0;

	tcp_time_stamp = (unsigned long) (trunc(Scheduler::instance().clock() * JIFFY_RATIO)); 
	ktime_get_real = (s64)trunc(Scheduler::instance().clock()*1000000000);
#ifdef notdef
	if (pkt->type_ != PT_ACK) {
		Tcl::instance().evalf("%s error \"received non-ack\"",
				      name());
		Packet::free(pkt);
		return;
	}
#endif

	++nackpack_;


	if (ack > (unsigned long)t_seqno_) return;	 // uninteresting_ack
	if (ack < prior_snd_una) return; // old_ack; only worth for D-SACK. but let's pass it.

	DEBUG(5, "received an ack packet %lu\n", ack);

	if (linux_.icsk_ca_ops) {
		//This has to be done before the first call to linux_.icsk_ca_ops.
		paramManager.load_local();
		//After this call, all return must be done after a paramManager.restore_default, or save_to_linux (which inclues restore_default).
	};

	if (hdr_flags::access(pkt)->ecnecho() && ecn_ && (ack>1)) {
		//ecn(tcph->seqno());
		flag |= FLAG_ECE;			//ECN
	}

	prior_in_flight = packets_in_flight();

	if (ack>prior_snd_una) {
		linux_.bytes_acked += (ack - prior_snd_una)*linux_.mss_cache;
		flag |= (FLAG_DATA_ACKED);
	};

	flag |= ack_processing(pkt, flag);
	DEBUG(5, "ack_processed prior_snd_una=%lu ack=%lu\n", prior_snd_una, ack);

	if ((tcph->sa_length()> 0) || (FLAG_DATA_ACKED && (!scb_->IsEmpty()))) {
		flag |= scb_->UpdateScoreBoard(highest_ack_, tcph);
	};
	DEBUG(5, "sack processed sack len=%d \n", tcph->sa_length());

	time_processing(pkt, flag, &seq_rtt);
	DEBUG(5, "time processed\n");


	if (linux_.icsk_ca_ops) {
		if ((!initialized_)) {
			if  (linux_.icsk_ca_ops->init)
				linux_.icsk_ca_ops->init(&linux_);	
			initialized_ = true;
		}
		if ((flag & FLAG_NOT_DUP) && (linux_.icsk_ca_ops->pkts_acked)){
			ktime_t last_ackt;
			if ((flag & FLAG_UNSURE_TSTAMP) || (!bugfix_ts_)) {
				last_ackt = 0;
			} else {
			        double then = tss[tcph->seqno() % tss_size_];
				last_ackt = (s64)trunc(then*1000000000);
			};	
			linux_.icsk_ca_ops->pkts_acked(sk, ack - prior_snd_una, last_ackt);
		}

		if ((linux_.icsk_ca_state==TCP_CA_Open)&& (!(flag&FLAG_CA_ALERT)) && (flag&FLAG_NOT_DUP)) {
			tcp_ca_event(CA_EVENT_FAST_ACK);
		}
		else {
			tcp_ca_event(CA_EVENT_SLOW_ACK);
		}
	}

// 	if (tp->frto_counter) tcp_process_frto(sk, prior_snd_una);	//We haven't done FRTO yet.
	
	#define tcp_ack_is_dubious(flag) (!(flag & FLAG_NOT_DUP) || (flag & FLAG_CA_ALERT) || linux_.icsk_ca_state != TCP_CA_Open)

	#define tcp_may_raise_cwnd(flag) (!(flag & FLAG_ECE) || linux_.snd_cwnd < linux_.snd_ssthresh) && \
				!((1 << linux_.icsk_ca_state) & (TCPF_CA_Recovery | TCPF_CA_CWR))

	#define tcp_cong_avoid(ack, rtt, in_flight, good) \
	{	\
		if (linux_.icsk_ca_ops) {\
			linux_.icsk_ca_ops->cong_avoid(sk, ack*linux_.mss_cache, rtt, in_flight, good);\
		} else {\
			opencwnd();\
			load_to_linux();\
		}\
		touch_cwnd();	\
	}
	DEBUG(5, "Event processed\n");
	if (tcp_ack_is_dubious(flag)){
		if ((flag & FLAG_DATA_ACKED) && tcp_may_raise_cwnd(flag))
			tcp_cong_avoid(ack, seq_rtt, prior_in_flight, 0);
		DEBUG(5, "dubious track cc finished\n");
		tcp_fastretrans_alert(flag);
	} else {
		if ((flag & FLAG_DATA_ACKED)) {
			prev_highest_ack_ = highest_ack_ ;
			tcp_cong_avoid(ack, seq_rtt, prior_in_flight, 1);
		}
	};
	DEBUG(5, "cc all finished\n");
	
	if (linux_.icsk_ca_ops) {
		save_from_linux();
		paramManager.restore_default();
	};
	send_much(FALSE, 0, maxburst_);	//anytime we can do send_much by checking it.
	DEBUG(5, "data sent\n");
	Packet::free(pkt);

#ifdef notyet
	if (trace_)
		plot();
#endif
}

//////////////////////////////  Congestion Control part ////////////////////////////
void LinuxTcpAgent::touch_cwnd() {
	linux_.snd_cwnd_stamp = tcp_time_stamp; /* we touch the congestion window in this function */
}
void LinuxTcpAgent::tcp_moderate_cwnd()
{
	linux_.snd_cwnd = min((int)linux_.snd_cwnd, packets_in_flight()+ tcp_max_burst);	//max
	touch_cwnd();
}

void LinuxTcpAgent::tcp_fastretrans_alert(unsigned char flag)
{	
	struct inet_connection_sock *icsk = &linux_;


	// We don't have tcp_mark_head_lost yet

	/* E. Check state exit conditions. State can be terminated
	 *    when high_seq is ACKed. */
	if (icsk->icsk_ca_state == TCP_CA_Open)	{
		//no need to exit unless frto.	
	} else {
		if (highest_ack_ >= recover_) {
			DEBUG(5,"clear scoread board\n");
			scb_->ClearScoreBoard();
			if (linux_.icsk_ca_ops == NULL) load_to_linux();
			if (linux_.snd_cwnd < linux_.snd_ssthresh)
			{
				linux_.snd_cwnd = linux_.snd_ssthresh;
				tcp_moderate_cwnd();
			}
			tcp_set_ca_state(TCP_CA_Open);
			next_pkts_in_flight_ = 0; //stop rate halving
			return;
		}	
	}
	
	DEBUG(5, "before step F: %d\n", linux_.icsk_ca_state);
	/* F. Process state. */
	switch (linux_.icsk_ca_state) {
	case TCP_CA_Recovery: break;
	case TCP_CA_Loss:
		//let it through
	default:
		//TCP_CA_Open and have loss or ECE
		if (flag&(FLAG_DATA_LOST | FLAG_ECE)) {
			recover_ = maxseq_;
			last_cwnd_action_ = CWND_ACTION_DUPACK;
		        touch_cwnd();
			next_pkts_in_flight_ = linux_.snd_cwnd;	//we do rate halving
		        if (linux_.icsk_ca_ops==NULL) {
                		slowdown(CLOSE_SSTHRESH_HALF|CLOSE_CWND_HALF);
				load_to_linux();
        		} else {
				//ok this is the linux part
				DEBUG(5, "check ssthresh\n");
				linux_.snd_ssthresh = icsk->icsk_ca_ops->ssthresh(&linux_);
				linux_.snd_cwnd_cnt = 0;
				linux_.bytes_acked = 0;
				DEBUG(5, "check min_cwnd %p\n", icsk->icsk_ca_ops->min_cwnd);
				if (icsk->icsk_ca_ops->min_cwnd)
					linux_.snd_cwnd = icsk->icsk_ca_ops->min_cwnd(&linux_);
				else
					linux_.snd_cwnd = linux_.snd_ssthresh;
				ncwndcuts_++;
				cong_action_ = TRUE;
				// Linux uses a CWR process to halve rate, we have a simpler one.
			}
			if (flag & FLAG_ECE) ++necnresponses_;
			tcp_set_ca_state(TCP_CA_Recovery);
		}
	}
}

void LinuxTcpAgent::enter_loss() 
{
	paramManager.load_local();
	touch_cwnd();
	if (linux_.icsk_ca_ops==NULL) {
		slowdown(CLOSE_SSTHRESH_HALF|CLOSE_CWND_RESTART);
		load_to_linux();
	} else {
	
		//ok this is the linux part

	//	if (icsk->icsk_ca_state <= TCP_CA_Disorder || tp->snd_una == tp->high_seq ||
	//	    (icsk->icsk_ca_state == TCP_CA_Loss && !icsk->icsk_retransmits)) {
			//tp->prior_ssthresh = tcp_current_ssthresh(sk);
	// icsk->icsk_retransmits: this is the # of unrecovered loss. in our case, all the loss are unrecovered.
//		if (linux_icsk_ca_state <= TCP_CA_Disorder || (highest_ack_+1) == recover_ || linux_icsk_ca_state== TCP_CA_LOSS) {
		
		linux_.snd_ssthresh = linux_.icsk_ca_ops->ssthresh(&linux_);
		tcp_ca_event(CA_EVENT_LOSS);
		linux_.snd_cwnd = 1;
		linux_.snd_cwnd_cnt = 0;
		linux_.bytes_acked = 0;
		//We don't have undo yet, otherwise, we should have:
		//tp->undo_marker = tp->snd_una;

		//We don't have reording yet, otherwise, we should have:
		//tp->reordering = min_t(unsigned int, tp->reordering, sysctl_tcp_reordering);

		//tp->high_seq = tp->snd_nxt;

		//We don't have ECN yet, otherwise, we should have: 
		//TCP_ECN_queue_cwr(tp);
		
		ncwndcuts_++;
		cong_action_ = TRUE;
	}
	recover_ = maxseq_;
	tcp_set_ca_state(TCP_CA_Loss); 
	//scb_->ClearScoreBoard();
	scb_->MarkLoss(highest_ack_+1, t_seqno_);
	//In Linux, we don't clear scoreboard in timeout, unless it's SACK Renege. We don't consider SACK Renege here.
	paramManager.restore_default();

}

void LinuxTcpAgent::timeout(int tno)
{
	if (tno == TCP_TIMER_RTX) {
		if (highest_ack_ == maxseq_ && !slow_start_restart_) {
			/*
			 * TCP option:
			 * If no outstanding data, then don't do anything.
			 */
			return;
		};
		last_cwnd_action_ = CWND_ACTION_TIMEOUT;
		/* if there is no outstanding data, don't cut down ssthresh_ */
		if (highest_ack_ == maxseq_ && restart_bugfix_) {
			if (linux_.icsk_ca_ops) save_from_linux();
			slowdown(CLOSE_CWND_INIT);
			load_to_linux();
		} else {
			// close down to 1 segment
			enter_loss();
			++nrexmit_;
		};
		/* if there is no outstanding data, don't back off rtx timer */
		if (highest_ack_ == maxseq_)
			reset_rtx_timer(TCP_REASON_TIMEOUT,0);
		else
			reset_rtx_timer(TCP_REASON_TIMEOUT,1);
		next_pkts_in_flight_ = 0;
		linux_.bytes_acked = 0;
		save_from_linux();
		send_much(0, TCP_REASON_TIMEOUT);
	} else {
		/* we do not know what it is */
		if (linux_.icsk_ca_ops) save_from_linux();
		TcpAgent::timeout(tno);
		load_to_linux();
	};
}


///////////////////////////   Sending control part ///////////////////
int LinuxTcpAgent::packets_in_flight()
{
	return (scb_->packets_in_flight(highest_ack_+1, t_seqno_));
}

bool LinuxTcpAgent::is_congestion() 
{
	return ( packets_in_flight() >= (int)linux_.snd_cwnd);
}

void LinuxTcpAgent::send_much(int force, int reason, int maxburst)
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
	next_pkts_in_flight_ = min(next_pkts_in_flight_, packets_in_flight()+1);

	while ( packets_in_flight() < max(win, next_pkts_in_flight_) ) {
		if (overhead_ == 0 || force) {
			found = 0;
			xmit_seqno = scb_->GetNextRetran ();

			if (xmit_seqno == -1) {  // no retransmissions to send
				/* 
				 * if there is no more application data to send,
				 * do nothing
				 */
				if (t_seqno_ >= curseq_) 
					return;
				found = 1;
				xmit_seqno = t_seqno_++;
			} else {
				found = 1;
				DEBUG(5, "%lf (%p) : Retran %d\n", Scheduler::instance().clock(), this, xmit_seqno);
				scb_->MarkRetran (xmit_seqno, t_seqno_);
				win = window();
			}
			if (found) {
				output(xmit_seqno, reason);
				next_pkts_in_flight_ = min( next_pkts_in_flight_, max(packets_in_flight()-1,1));
				if (t_seqno_ <= xmit_seqno) {
					printf("Hit a strange case 2.\n");
					t_seqno_ = xmit_seqno + 1;
				}
				linux_.snd_nxt = t_seqno_*linux_.mss_cache;
				npacket++;
			}
		} else if (!(delsnd_timer_.status() == TIMER_PENDING)) {
			/*
			 * Set a delayed send timeout.
			 */
			delsnd_timer_.resched(Random::uniform(overhead_));
			return;
		}
		if (maxburst && npacket >= maxburst)
			break;
	} /* while */

	/* call helper function */
	send_helper(maxburst);
}


////////////////////   Linux Module control part /////////////////////////////
void LinuxTcpAgent::load_to_linux()
{

	//TODO
	linux_.snd_ssthresh = (int)ssthresh_;

	if ((next_pkts_in_flight_ > (int)linux_.snd_cwnd) && (cwnd_ >= (int)linux_.snd_cwnd)) {
		//We are in the process of rate-halving and the traditional ns-2 does not ask for further reduction
		next_pkts_in_flight_ = (int)(trunc(cwnd_));
	} else {
		//We are not in the process of rate-halving -- safe to load
		linux_.snd_cwnd = (int)(trunc(cwnd_));
		//linux_.snd_cwnd_cnt = (int) ((cwnd_ - (double)linux_.snd_cwnd)* (double)linux_.snd_cwnd);
	}

//read only:
	linux_.snd_cwnd_clamp = (int) wnd_;
	
	//read only for linux

//	linux_.snd_nxt  this variable is directly controlled by ack_processing() and send_much()
//	linux_.snd_una  this variable is directly controlled by ack_processing()
//	linux_.srtt     this variable is directly controlled by rtt_update()
//      snd_cwnd_stamp  this variable is directly controlled by touch_cwnd()
//	linux_.rx_opt.rcv_tsecr is directly controled in ack_processing()
//	linux_.rx_opt.saw_tstamp is directly controled in ack_processing()
//	snd_cwnd_stamp	this variable is directly controled by touch_cwnd()

//remark:
	//snd_una == (highest_ack+1)*mss_cache
	//snd_nxt == (t_seqno)*mss_cache
	
};

void LinuxTcpAgent::save_from_linux()
{

	//TODO
	if (next_pkts_in_flight_ > (int)linux_.snd_cwnd) 
		cwnd_ = next_pkts_in_flight_; 
	else
		cwnd_ = linux_.snd_cwnd;
// + min(((double)linux_.snd_cwnd_cnt/(double)linux_.snd_cwnd),1);

	ssthresh_ = linux_.snd_ssthresh;
	// for legacy variables:
	last_ack_ = highest_ack_;

};

void LinuxTcpAgent::load_to_linux_once() {
	linux_.snd_cwnd_clamp = (long unsigned) round(wnd_);
	linux_.snd_ssthresh = linux_.snd_cwnd_clamp;
	linux_.mss_cache = size_;
}

char LinuxTcpAgent::install_congestion_control(const char* name)
{
	struct tcp_congestion_ops* newops = cong_ops_manager.get_ops(name);
	if (newops) {
		if (linux_.icsk_ca_ops!=newops) {
			//release any existing congestion control algorithm before install
                	if (linux_.icsk_ca_ops !=NULL ) {
				if (linux_.icsk_ca_ops->release) 
					linux_.icsk_ca_ops->release(&linux_);
				save_from_linux();
			} else {
				load_to_linux_once();
				load_to_linux();	//it was controlled by NS2, load to Linux
				tcp_tick_ = 1.0/(double)JIFFY_RATIO;
			}
			linux_.icsk_ca_ops = newops;

			if ((initialized_) && (newops->init)) {
				//if the algorithm is changed in the middle, we need to intialize the module
				newops->init(&linux_);
			}

			if ((linux_.icsk_ca_ops->flags & TCP_CONG_RTT_STAMP)  || (linux_.icsk_ca_ops->rtt_sample)) {
				bugfix_ts_=1;
				// if rtt_sample exists, we must turn on the sender-side timestamp record to provide accurate rtt in microsecond.
			}

			save_from_linux();
		}
		return TRUE;
	} else {
		return FALSE;
	}
};

void LinuxTcpAgent::remove_congestion_control()
{
	if (linux_.icsk_ca_ops != NULL) {
		if (linux_.icsk_ca_ops->release)
			linux_.icsk_ca_ops->release(&linux_);
		save_from_linux();
		linux_.icsk_ca_ops = NULL;		
	}
};

int LinuxTcpAgent::command(int argc, const char*const* argv) 
{
	if ((argc>=3) && (strcmp(argv[1], "select_ca")==0)) {
		printf("%s %s %s\n", argv[0], argv[1], argv[2]);
		if (install_congestion_control(argv[2])==FALSE) {
			printf("Error: do not find %s as a congestion control algorithm\n", argv[2]);
			cong_ops_manager.dump();
		}
		return (TCL_OK);
	};
	if ((argc>=5) && (strcmp(argv[1], "set_ca_param")==0)) {
		printf("%s %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3], argv[4]);
		if (!paramManager.set_param(argv[2], argv[3], atoi(argv[4]))) {
			printf("Error: do not find %s as a parameter for congestion control algorithm %s\n", argv[3], argv[2]);
		};
		return (TCL_OK);
	};
	if ((argc>=4) && (strcmp(argv[1], "get_ca_param")==0)) {
		printf("%s %s %s %s\n", argv[0], argv[1], argv[2], argv[3]);
		int res;
		if (!paramManager.get_param(argv[2], argv[3], &res)) {
			printf("Error: do not find %s as a parameter for congestion control algorithm %s\n", argv[3], argv[2]);
		} else {
			printf("tcp_%s.%s = %d\n", argv[2], argv[3], res);
		};
		return (TCL_OK);
	};
	if ((argc>=5) && (strcmp(argv[1], "set_ca_default_param")==0)) {
		printf("%s %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3], argv[4]);
		if (!paramManager.set_default_param(argv[2], argv[3], atoi(argv[4]))) {
			printf("Error: do not find %s as a parameter for congestion control algorithm %s\n", argv[3], argv[2]);
		};
		return (TCL_OK);
	};
	if ((argc>=4) && (strcmp(argv[1], "get_ca_default_param")==0)) {
		printf("%s %s %s %s\n", argv[0], argv[1], argv[2], argv[3]);
		int res;
		if (!paramManager.get_default_param(argv[2], argv[3], &res)) {
			printf("Error: do not find %s as a parameter for congestion control algorithm %s\n", argv[3], argv[2]);
		} else {
			printf("tcp_%s.%s = %d\n", argv[2], argv[3], res);
		};
		return (TCL_OK);
	};
	if ((argc==3) && (strcmp(argv[1], "get_ca_param")==0)) {
		printf("%s %s %s\n", argv[0], argv[1], argv[2]);
		if (!paramManager.query_param(argv[2])) {
			printf("Error: %s is not a congestion control algorithm or has no parameter\n", argv[2]);
		};
		return (TCL_OK);
	};
	return (TcpAgent::command(argc, argv));

}

void LinuxTcpAgent::plot()
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


/* Implementation of LinuxParamManager */
struct cc_list* LinuxParamManager::find_cc_by_proto(const char* proto) {
	struct cc_list* p = cc_list_head;
	char proto_name[100];
	snprintf(proto_name, 100, "tcp_%s.c", proto);
	while (p!=NULL && (strcmp(p->proto, proto_name)!=0)) p=p->next;
	return p;
};

struct cc_param_list* LinuxParamManager::find_param_by_proto_name(const char* proto, const char* name) {
	struct cc_list* p = find_cc_by_proto(proto);
	if (!p) return NULL;
	struct cc_param_list* q = p->param_head;
	while (q!=NULL && ((strcmp(q->name, name)!=0)||(strcmp(q->type, "int")!=0))) q=q->next;
	return q;
};

bool LinuxParamManager::set_default_param(const char* proto, const char* param, const int value) {
	struct cc_param_list* p = find_param_by_proto_name(proto, param);
	if (!p) return false;
	(*(int*)p->ptr) = value;
	return true;
};

bool LinuxParamManager::set_param(const char* proto, const char* param, const int value) {
	struct cc_param_list* p = find_param_by_proto_name(proto, param);
	if (!p) return false;
	localValues.set_param((int*)p->ptr, value);
	return true;
};

bool LinuxParamManager::get_default_param(const char* proto, const char* param, int* valuep) {
	struct cc_param_list* p = find_param_by_proto_name(proto, param);
	if (!p) return false;
	*valuep = *((int*)(p->ptr));
	return true;
};

bool LinuxParamManager::get_param(const char* proto, const char* param, int* valuep) {
	struct cc_param_list* p = find_param_by_proto_name(proto, param);
	if (!p) return false;
	if (localValues.get_param((int*)p->ptr, valuep)) return true;
	*valuep = *((int*)(p->ptr));
	return true;
};
bool LinuxParamManager::query_param(const char* proto) {
	struct cc_list* p = find_cc_by_proto(proto);
	if (!p) {
		printf("%s has no parameter registered\n", proto);
		return false;
	};
	printf("Parameter list for %s:\n", proto);
	struct cc_param_list* q = p->param_head;
	while (q != NULL) {
		printf(" %s = %d (%s)\n", q->name, *((int*)q->ptr), q->description);
		q = q->next;
	}; 
	return true;
};
/** Get a local value for a TCP */
bool ParamList::get_param(int* address, int* valuep) {
	struct paramNode *p = head;
	while (p && (p->addr != address)) p = p->next;
	if (p) {
		*valuep = p->value;
		return true;
	};
	return false;
};

/** Set a local value for a TCP */
void ParamList::set_param(int* address, int value) {
	struct paramNode *p = head;
	while ((p) && (p->addr != address)) {
		p = p->next;
	};
	if (p) {
		//we find one.
		p->value = value;
		return;
	};
	//Cannot find any
	//Create a new entry
	p = new struct paramNode;
	p->addr = address; 
	p->value = value;
	p->default_value = *address;
	p->next = head;
	head = p;
};

/** Refresh all the values in the list */
void ParamList::refresh_default() {
	struct paramNode *p = head;
	while (p) {
		p->default_value = *(p->addr);
		p = p->next;
	};
};

void ParamList::restore_default() {
	struct paramNode *p = head;
	while (p) {
		p->value = *(p->addr);
		*(p->addr) = p->default_value;
		p = p->next;
	};
};

void ParamList::load_local() {
	struct paramNode *p = head;
	while (p) {
		p->default_value = *(p->addr);
		*(p->addr) = p->value;
		p = p->next;
	};
};

ParamList::~ParamList() {
	while (head) {
		struct paramNode *p = head;
		head = head->next;
		delete p;
	};
};

