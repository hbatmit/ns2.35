/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */ /*
 * Copyright (c) 1991-1997 Regents of the University of California.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
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
 * @(#) $Header: /cvsroot/nsnam/ns-2/tcp/tcp.h,v 1.132 2011/08/26 19:29:57 tom_henderson Exp $ (LBL)
 */
#ifndef ns_tcp_h
#define ns_tcp_h

#include "agent.h"
#include "packet.h"

//class EventTrace;

struct hdr_tcp {
#define NSA 3
	double ts_;             /* time packet generated (at source) */
	double ts_echo_;        /* the echoed timestamp (originally sent by
	                           the peer) */
	int seqno_;             /* sequence number */
	int reason_;            /* reason for a retransmit */
	int sack_area_[NSA+1][2];	/* sack blocks: start, end of block */
	int sa_length_;         /* Indicate the number of SACKs in this  *
	                         * packet.  Adds 2+sack_length*8 bytes   */ 
	int ackno_;             /* ACK number for FullTcp */
	int hlen_;              /* header len (bytes) for FullTcp */
	int tcp_flags_;         /* TCP flags for FullTcp */
	int last_rtt_;		/* more recent RTT measurement in ms, */
				/*   for statistics only */

	static int offset_;	// offset for this header
	inline static int& offset() { return offset_; }
	inline static hdr_tcp* access(Packet* p) {
		return (hdr_tcp*) p->access(offset_);
	}

	/* per-field member functions */
	double& ts() { return (ts_); }
	double& ts_echo() { return (ts_echo_); }
	int& seqno() { return (seqno_); }
	int& reason() { return (reason_); }
	int& sa_left(int n) { return (sack_area_[n][0]); }
	int& sa_right(int n) { return (sack_area_[n][1]); }
	int& sa_length() { return (sa_length_); }
	int& hlen() { return (hlen_); }
	int& ackno() { return (ackno_); }  
	int& flags() { return (tcp_flags_); }
	int& last_rtt() { return (last_rtt_); }
};

/* these are used to mark packets as to why we xmitted them */
#define TCP_REASON_TIMEOUT	0x01
#define	TCP_REASON_DUPACK	0x02
#define	TCP_REASON_RBP		0x03   // used only in tcp-rbp.cc
#define TCP_REASON_PARTIALACK   0x04

/* these are reasons we adjusted our congestion window */

#define	CWND_ACTION_DUPACK	1	// dup acks/fast retransmit
#define	CWND_ACTION_TIMEOUT	2	// retransmission timeout
#define	CWND_ACTION_ECN		3	// ECN bit [src quench if supported]
#define CWND_ACTION_EXITED      4       // congestion recovery has ended
					// (when previously CWND_ACTION_DUPACK)

/* these are bits for how to change the cwnd and ssthresh values */

#define	CLOSE_SSTHRESH_HALF	0x00000001
#define	CLOSE_CWND_HALF		0x00000002
#define	CLOSE_CWND_RESTART	0x00000004
#define	CLOSE_CWND_INIT		0x00000008
#define	CLOSE_CWND_ONE		0x00000010
#define CLOSE_SSTHRESH_HALVE	0x00000020
#define CLOSE_CWND_HALVE	0x00000040
#define THREE_QUARTER_SSTHRESH  0x00000080
#define CLOSE_CWND_HALF_WAY 	0x00000100
#define CWND_HALF_WITH_MIN	0x00000200
#define TCP_IDLE		0x00000400
#define NO_OUTSTANDING_DATA     0x00000800

/*
 * tcp_tick_:
 * default 0.1,
 * 0.3 for 4.3 BSD, 
 * 0.01 for new window algorithms,
 */

#define NUMDUPACKS 3		/* This is no longer used.  The variable */
				/* numdupacks_ is used instead. */
#define TCP_MAXSEQ 1073741824   /* Number that curseq_ is set to for */
				/* "infinite send" (2^30)            */

#define TCP_TIMER_RTX		0
#define TCP_TIMER_DELSND	1
#define TCP_TIMER_BURSTSND	2
#define TCP_TIMER_DELACK	3
#define TCP_TIMER_Q         4
#define TCP_TIMER_RESET        5 

class TcpAgent;

class RtxTimer : public TimerHandler {
public: 
	RtxTimer(TcpAgent *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	TcpAgent *a_;
};

class DelSndTimer : public TimerHandler {
public: 
	DelSndTimer(TcpAgent *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	TcpAgent *a_;
};

class BurstSndTimer : public TimerHandler {
public: 
	BurstSndTimer(TcpAgent *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	TcpAgent *a_;
};

/*
 * Variables for HighSpeed TCP.
 */
//int *hs_win_;		// array of cwnd values
//int *hs_increase_;	// array of increase values
//double *hs_decrease_;	// array of decrease values
struct hstcp {
	double low_p;  // low_p
	double dec1;	// for computing the decrease parameter
	double dec2;    // for computing the decrease parameter
	double p1;	// for computing p
        double p2;	// for computing p
	/* The next three parameters are for CPU overhead, for computing */
	/*   the HighSpeed parameters less frequently.  A better solution */
 	/*   might be just to have a look-up array.  */
	double cwnd_last_;	/* last cwnd for computed parameters */
        double increase_last_;	/* increase param for cwnd_last_ */
	hstcp() : low_p(0.0), dec1(0.0), dec2(0.0), p1(0.0), p2(0.0),
	    cwnd_last_(0.0), increase_last_(0.0) { }
};

class TcpAgent : public Agent {
	friend class XcpEndsys;
public:
	TcpAgent();
	virtual ~TcpAgent() {free(tss);}
        virtual void recv(Packet*, Handler*);
	virtual void timeout(int tno);
	virtual void timeout_nonrtx(int tno);
	int command(int argc, const char*const* argv);
	virtual void sendmsg(int nbytes, const char *flags = 0);

	void trace(TracedVar* v);
	virtual void advanceby(int delta);

	virtual void reset();

	/* These two functions aid Tmix one-way TCP agents */
	int is_closed() {return closed_;} 
	void clr_closed() {closed_ = 0;}
protected:
	virtual int window();
	virtual double windowd();
	void print_if_needed(double memb_time);
	void traceAll();
	virtual void traceVar(TracedVar* v);
	virtual int headersize();   // a tcp header

	virtual void delay_bind_init_all();
	virtual int delay_bind_dispatch(const char *varName, const char *localName, TclObject *tracer);

	double boot_time_;	/* where between 'ticks' this sytem came up */
	double overhead_;
	double wnd_;
	double wnd_const_;
	double wnd_th_;		/* window "threshold" */
	double wnd_init_;
	double wnd_restart_;
	double tcp_tick_;	/* clock granularity */
	int wnd_option_;
	int wnd_init_option_;   /* 1 for using wnd_init_ */
				/* 2 for using large initial windows */
	double decrease_num_;   /* factor for multiplicative decrease */
	double increase_num_;   /* factor for additive increase */
	int tcpip_base_hdr_size_;  /* size of base TCP/IP header */
	int maxcwnd_;		/* max # cwnd can ever be */
        int numdupacks_;	/* dup ACKs before fast retransmit */
	int numdupacksFrac_;	/* for a larger numdupacks_ with large */
				/* windows */

	/* connection and packet dynamics */
	virtual void output(int seqno, int reason = 0);
	virtual void send_much(int force, int reason, int maxburst = 0);
	virtual void newtimer(Packet*);
	virtual void dupack_action();		/* do this on dupacks */
	virtual void send_one();		/* do this on 1-2 dupacks */
	virtual void opencwnd();

	void slowdown(int how);			/* reduce cwnd/ssthresh */
	void ecn(int seqno);		/* react to quench */
	virtual void set_initial_window();	/* set IW */
	double initial_window();		/* what is IW? */
	void newack(Packet*);
	void finish(); /* called when the connection is terminated */
	int network_limited();	/* Sending limited by network? */
	double limited_slow_start(double cwnd, int max_ssthresh, double increment);
				/* Limited slow-start for high windows */
	virtual int numdupacks(double cwnd); 	/* for getting numdupacks_ */
	/* End of section of connection and packet dynamics.  */

	/* General dynamic state. */
	TracedInt t_seqno_;	/* sequence number */
	TracedInt dupacks_;	/* number of duplicate acks */
	TracedInt curseq_;	/* highest seqno "produced by app" */
	TracedInt highest_ack_;	/* not frozen during Fast Recovery */
	TracedDouble cwnd_;	/* current window */
	TracedInt ssthresh_;	/* slow start threshold */
	TracedInt maxseq_;	/* used for Karn algorithm */
				/* highest seqno sent so far */
	int last_ack_;		/* largest consecutive ACK, frozen during
				 *		Fast Recovery */
	int recover_;		/* highest pkt sent before dup acks, */
				/*   timeout, or source quench/ecn */
	int last_cwnd_action_;	/* CWND_ACTION_{TIMEOUT,DUPACK,ECN} */
	int count_;		/* used in window increment algorithms */
	int rtt_active_;	/* 1 if a rtt sample is pending */
	int rtt_seq_;		/* seq # of timed seg if rtt_active_ is 1 */
	double rtt_ts_;		/* time at which rtt_seq_ was sent */
	double firstsent_;	/* When first packet was sent  --Allman */
	double lastreset_;	/* W.N. Last time connection was reset - for */
				/* detecting pkts from previous incarnations */
	int closed_;            /* whether this connection has closed */
	/* End of general dynamic state. */

	/*
	 * State encompassing the round-trip-time estimate.
	 * srtt and rttvar are stored as fixed point;
	 * srtt has 3 bits to the right of the binary point, rttvar has 2.
	 */
	TracedInt t_rtt_;      	/* round trip time */
	TracedInt t_srtt_;     	/* smoothed round-trip time */
	TracedInt t_rttvar_;   	/* variance in round-trip time */
	TracedInt t_backoff_;	/* current multiplier of RTO, */
				/*   1 if not backed off */
	#define T_RTT_BITS 0
	int T_SRTT_BITS;        /* exponent of weight for updating t_srtt_ */
	int srtt_init_;		/* initial value for computing t_srtt_ */
	int T_RTTVAR_BITS;      /* exponent of weight for updating t_rttvar_ */ 
	int rttvar_exp_;        /* exponent of multiple for t_rtxcur_ */
	int rttvar_init_;       /* initial value for computing t_rttvar_ */
	double t_rtxcur_;	/* current retransmit value */
	double rtxcur_init_;    /* initial value for t_rtxcur_ */
	virtual void rtt_init();
	virtual double rtt_timeout();	/* provide RTO based on RTT estimates */
	virtual void rtt_update(double tao);	/* update RTT estimate */
	virtual void rtt_backoff();		/* double multiplier */
	/* End of state for the round-trip-time estimate. */

        /* RTOs: */
	double maxrto_;		/* max value of an RTO */
	double minrto_;         /* min value of an RTO */
	int ts_resetRTO_;	/* Un-backoff RTO after any valid RTT, */
				/*   including from a retransmitted pkt?  */
				/* The old version was "false". */
				/* But "true" gives better performance, and */
                                /* seems conformant with RFC 2988. */
        /* End of section for RTOs. */

	/* Timestamps. */
	double ts_peer_;        /* the most recent timestamp the peer sent */
	double ts_echo_;        /* the most recent timestamp the peer echoed */
	int ts_option_size_;    // header bytes in a ts option
        double *tss;            // To store sent timestamps, with bugfix_ts_
        int tss_size_;          // Current capacity of tss
	int ts_option_;		/* use RFC1323-like timestamps? */
	/* End of timestamps. */

	/* Helper functions.  Used by tcp-asym */
	virtual void output_helper(Packet*) { return; }
	virtual void send_helper(int) { return; }
	virtual void send_idle_helper() { return; }
	virtual void recv_helper(Packet*) { return; }
	virtual void recv_frto_helper(Packet*);
	virtual void recv_newack_helper(Packet*);
	virtual void partialnewack_helper(Packet*) {};
	/* End of helper functions. */

	int force_wnd(int num);
	void spurious_timeout();

	/* Timers */
	RtxTimer rtx_timer_;
	DelSndTimer delsnd_timer_;
	BurstSndTimer burstsnd_timer_;
	virtual void cancel_timers() {
		rtx_timer_.force_cancel();
		burstsnd_timer_.force_cancel();
		delsnd_timer_.force_cancel();
	}
	virtual void cancel_rtx_timer() {
		rtx_timer_.force_cancel();
	}
	virtual void set_rtx_timer();
	void reset_rtx_timer(int mild, int backoff = 1);
	int timerfix_;		/* set to true to update timer *after* */
				/* update the RTT, instead of before   */
	int rfc2988_;		/* Use updated RFC 2988 timers */
	/* End of timers. */ 


	/* For modeling SYN and SYN/ACK packets. */
	int syn_;		/* 1 for modeling SYN/ACK exchange */
	int delay_growth_;  	/* delay opening cwnd until 1st data recv'd */
        int max_connects_;  	/* max number of transmits for syn packet */
				/* -1 to allow infinite number of transmits */
	/* End of modeling SYN and SYN/ACK packets. */

	/* Dynamic state for SYN packet retransmissions. */
	int syn_connects_;	/* number of transmits of syn packet */
	/* End of dynamic state for SYN packet retransmissions. */

	/* F-RTO */
	int frto_enabled_;	/* != 0 to enable F-RTO */
	int sfrto_enabled_;	/* != 0 to enabled SACK-based F-RTO */
	int spurious_response_;	/* Response variant to spurious RTO */
	/* End of R-RTO */

	/* Parameters for backwards compatility with old code. */ 
	int bug_fix_;		/* 1 for multiple-fast-retransmit fix */
	int less_careful_;	/* 1 for Less Careful variant of bug_fix_, */
				/*  for illustration only  */
	int exitFastRetrans_;	/* True to clean exits of Fast Retransmit */ 
				/* False for buggy old behavior */
	int bugfix_ack_;        // 1 to enable ACK heuristic, to allow
				//  multiple-fast-retransmits in special cases.
				// From Andrei Gurtov
	int bugfix_ts_;         // 1 to enable timestamp heuristic, to allow
				//  multiple-fast-retransmits in special cases.
				// From Andrei Gurtov
				// Not implemented yet.
	int old_ecn_;		/* For backwards compatibility with the 
				 * old ECN implementation, which never
				 * reduced the congestion window below
				 * one packet. */ 
	int bugfix_ss_;		// 1 to use window of one when SYN
				//  packet is dropped
	/* End of parameters for backwards compatility. */

	/* Parameters for alternate congestion control mechanisms. */
	double k_parameter_;     /* k parameter in binomial controls */
	double l_parameter_;     /* l parameter in binomial controls */
	int precision_reduce_;  /* non-integer reduction of cwnd */
	int maxburst_;		/* max # packets can send back-2-back */
	int aggressive_maxburst_;	/* Send on a non-valid ack? */
	/* End of parameters for alternate congestion control mechanisms. */

	FILE *plotfile_;

	/* Dynamic state used for alternate congestion control mechanisms */
	double awnd_;		/* averaged window */
	int first_decrease_;	/* First decrease of congestion window.  */
				/* Used for decrease_num_ != 0.5. */
	double fcnt_;		/* used in window increment algorithms */
	double base_cwnd_;	/* base window (for experimental purposes) */
	/* End of state for alternate congestion control mechanisms */

	/* Dynamic state only used for monitoring */
	int trace_all_oneline_;	/* TCP tracing vars all in one line or not? */
	int nam_tracevar_;      /* Output nam's variable trace or just plain 
				   text variable trace? */
        TracedInt ndatapack_;   /* number of data packets sent */
        TracedInt ndatabytes_;  /* number of data bytes sent */
        TracedInt nackpack_;    /* number of ack packets received */
        TracedInt nrexmit_;     /* number of retransmit timeouts 
				   when there was data outstanding */
        TracedInt nrexmitpack_; /* number of retransmited packets */
        TracedInt nrexmitbytes_; /* number of retransmited bytes */
        TracedInt necnresponses_; /* number of times cwnd was reduced
			   	   in response to an ecn packet -- sylvia */
        TracedInt ncwndcuts_; 	/* number of times cwnd was reduced 
				   for any reason -- sylvia */
        TracedInt ncwndcuts1_;     /* number of times cwnd was reduced 
                                   due to congestion (as opposed to idle
                                   periods */
	/* end of dynamic state for monitoring */

	/* Specifying variants in TCP algorithms.  */
	int slow_start_restart_; /* boolean: re-init cwnd after connection 
				    goes idle.  On by default. */
	int restart_bugfix_;    /* ssthresh is cut down because of
				   timeouts during a connection's idle period.
				   Setting this boolean fixes this problem.
				   For now, it is off by default. */ 
        TracedInt singledup_;   /* Send on a single dup ack.  */
	int LimTransmitFix_;	/* To fix a bug in Limited Transmit. */
	int noFastRetrans_;	/* No Fast Retransmit option.  */
	int oldCode_;		/* Use old code. */
	int useHeaders_;	/* boolean: Add TCP/IP header sizes */
	/* end of specifying variants */

	/* Used for ECN */
	int ecn_;		/* Explicit Congestion Notification */
	int cong_action_;	/* Congestion Action.  True to indicate
				   that the sender responded to congestion. */
        int ecn_burst_;		/* True when the previous ACK packet
				 *  carried ECN-Echo. */
	int ecn_backoff_;	/* True when retransmit timer should begin
			  	    to be backed off.  */
	int ect_;       	/* turn on ect bit now? */
	int SetCWRonRetransmit_;  /* True to allow setting CWR on */
				  /*  retransmitted packets.   Affects */
				  /*  performance for Reno with ECN.  */
	int use_rtt_;	     /* Use RTT for timeout for ECN-marked SYN-ACK */
	/* end of ECN */

	/* used for Explicit Loss Notification */
	void tcp_eln(Packet *pkt); /* reaction to ELN (usually wireless) */
        int eln_;               /* Explicit Loss Notification (wireless) */
        int eln_rxmit_thresh_;  /* Threshold for ELN-triggered rxmissions */
        int eln_last_rxmit_;    /* Last packet rxmitted due to ELN info */
	/* end of Explicit Loss Notification */

	/* for High-Speed TCP, RFC 3649 */
	double linear(double x, double x_1, double y_1, double x_2, double y_2);
	  /* the "linear" function is for experimental highspeed TCP */
	/* These four parameters define the HighSpeed response function. */
	int low_window_;	/* window for turning on high-speed TCP */
	int high_window_;	/* target window for new response function */
	double high_p_;		/* target drop rate for new response function */
	double high_decrease_;	/* decrease rate at target window */
	/* The next parameter is for Limited Slow-Start. */
	int max_ssthresh_;	/* max value for ssthresh_ */

	/* These two functions are just an easy structuring of the code. */ 
	double increase_param();  /* get increase parameter for current cwnd */
	double decrease_param();  /* get decrease parameter for current cwnd */
	int cwnd_range_;	/* for determining when to recompute params. */
	hstcp hstcp_;		/* HighSpeed TCP variables */
        /* end of section for experimental high-speed TCP */

	/* for Quick-Start, RFC 4782 */
	virtual void processQuickStart(Packet *pkt);
	virtual void endQuickStart();
	int lossQuickStart();
	int rate_request_;      /* Rate request in KBps, for QuickStart.  */
	int qs_enabled_;        /* to enable QuickStart. */
	int qs_requested_;
	int qs_approved_;
        int qs_window_;  /* >0: there are outstanding non-acked segments
                            from QS window */
        int qs_cwnd_; /* Initial window for Quick-Start */
        int tcp_qs_recovery_; /* != 0 if we apply slow start on packet
                                 losses during QS window */
	int qs_request_mode_;   /* 1 = Try to avoid unnecessary QS requests
				   for short flows. Use qs_rtt_ as the RTT
				   used in window calculation.
				   Other: Always request 'rate_request_' bytes,
				   regardless of flow size */
	int qs_thresh_;         /* Do not use QS if there are less data to send
				   than this. Applies only if
				   qs_request_mode_ == 1 */
	int qs_rtt_;            /* QS needs some assumption of the RTT in
				   in order to be able to determine how much
				   it needs for rate request with given amount
				   of data to send. milliseconds. */
	int print_request_;	/* true to print Quick-Start request */
	int ttl_diff_;
        /* end of section for Quick-Start. */

	/* F-RTO: !=0 when F-RTO recovery is underway, N:th round-trip
	 * since RTO. Can have values between 0-2 */
	int frto_;
	int pipe_prev_; /* window size when timeout last occurred */

        /* support for event-tracing */
        //EventTrace *et_;
        void trace_event(char *eventtype);

	/* these function are now obsolete, see other above */
	void closecwnd(int how);
	void quench(int how);
        
	/* TCP quiescence, reducing cwnd after an idle period */
	void process_qoption_after_send() ;
	void process_qoption_after_ack(int seqno) ;
	void reset_qoption();	/* for QOption with EnblRTTCtr_ */
	void rtt_counting();	/* for QOption with EnblRTTCtr_ */
	int QOption_ ; /* TCP quiescence option */
	int EnblRTTCtr_ ; /* are we using a corase grained timer? */
	int T_full ; /* last time the window was full */
	int T_last ;
	int T_prev ;
	int T_start ;
	int RTT_count ;
	int RTT_prev ;
	int RTT_goodcount ;
	int F_counting ;
	int W_used ; 
	int W_timed ;
	int F_full ; 
	int Backoffs ;
	int control_increase_ ; /* If true, don't increase cwnd if sender */
				/*  is not window-limited.  */
	int prev_highest_ack_ ; /* Used to determine if sender is */
				/*  window-limited.  */
   	/* end of TCP quiescence */
};

/* TCP Reno */
class RenoTcpAgent : public virtual TcpAgent {
 public:
	RenoTcpAgent();
	virtual int window();
	virtual double windowd();
	virtual void recv(Packet *pkt, Handler*);
	virtual void timeout(int tno);
	virtual void dupack_action();
 protected:
	int allow_fast_retransmit(int last_cwnd_action_);
	unsigned int dupwnd_;
};

/* TCP New Reno */
class NewRenoTcpAgent : public virtual RenoTcpAgent {
 public:
	NewRenoTcpAgent();
	virtual void recv(Packet *pkt, Handler*);
	virtual void partialnewack_helper(Packet* pkt);
	virtual void dupack_action();
 protected:
	int newreno_changes_;	/* 0 for fixing unnecessary fast retransmits */
				/* 1 for additional code from Allman, */
				/* to implement other algorithms from */
				/* Hoe's paper, including sending a new */
				/* packet for every two duplicate ACKs. */
				/* The default is set to 0. */
	int newreno_changes1_;  /* Newreno_changes1_ set to 0 gives the */
				/* Slow-but-Steady variant of NewReno from */
				/* RFC 2582, with the retransmit timer reset */
				/* after each partial new ack. */  
				/* Newreno_changes1_ set to 1 gives the */
				/* Impatient variant of NewReno from */
				/* RFC 2582, with the retransmit timer reset */
				/* only for the first partial new ack. */
				/* The default is set to 0 */
	void partialnewack(Packet *pkt);
	int allow_fast_retransmit(int last_cwnd_action_);
	int acked_, new_ssthresh_;  /* used if newreno_changes_ == 1 */
	double ack2_, ack3_, basertt_; /* used if newreno_changes_ == 1 */
	int firstpartial_; 	/* For the first partial ACK. */ 
	int partial_window_deflation_; /* 0 if set cwnd to ssthresh upon */
				       /* partial new ack (default) */
				       /* 1 if deflate (cwnd + dupwnd) by */
				       /* amount of data acked */
				       /* "Partial window deflation" is */
				       /* discussed in RFC 2582. */
	int exit_recovery_fix_;	 /* 0 for setting cwnd to ssthresh upon */
				 /* leaving fast recovery (default) */
				 /* 1 for setting cwnd to min(ssthresh, */
				 /* amnt. of data in network) when leaving */
};

/* TCP vegas (VegasTcpAgent) */
class VegasTcpAgent : public virtual TcpAgent {
 public:
	VegasTcpAgent();
	~VegasTcpAgent();
	virtual void recv(Packet *pkt, Handler *);
	virtual void timeout(int tno);
protected:
	double vegastime() {
		return(Scheduler::instance().clock() - firstsent_);
	}
	virtual void output(int seqno, int reason = 0);
	virtual void recv_newack_helper(Packet*);
	int vegas_expire(Packet*); 
	void reset();
	void vegas_inflate_cwnd(int win, double current_time);

	virtual void delay_bind_init_all();
	virtual int delay_bind_dispatch(const char *varName, const char *localName, TclObject *tracer);

	double t_cwnd_changed_; // last time cwnd changed
	double firstrecv_;	// time recv the 1st ack

	int    v_alpha_;    	// vegas thruput thresholds in pkts
	int    v_beta_;  	    	

	int    v_gamma_;    	// threshold to change from slow-start to
				// congestion avoidance, in pkts

	int    v_slowstart_;    // # of pkts to send after slow-start, deflt(2)
	int    v_worried_;      // # of pkts to chk after dup ack (1 or 2)

	double v_timeout_;      // based on fine-grained timer
	double v_rtt_;		
	double v_sa_;		
	double v_sd_;	

	int    v_cntRTT_;       // # of rtt measured within one rtt
	double v_sumRTT_;       // sum of rtt measured within one rtt

	double v_begtime_;	// tagged pkt sent
	int    v_begseq_;	// tagged pkt seqno

	double* v_sendtime_;	// each unacked pkt's sendtime is recorded.
	int*   v_transmits_;	// # of retx for an unacked pkt

	int    v_maxwnd_;	// maxwnd size for v_sendtime_[]
	double v_newcwnd_;	// record un-inflated cwnd

	double v_baseRTT_;	// min of all rtt

	double v_incr_;		// amount cwnd is increased in the next rtt
	int    v_inc_flag_;	// if cwnd is allowed to incr for this rtt

	double v_actual_;	// actual send rate (pkt/s; needed for tcp-rbp)

	int ns_vegas_fix_level_;   // see comment at end of tcp-vegas.cc for details of fixes
};

// Local Variables:
// mode:c++
// c-basic-offset: 8
// End:

#endif
