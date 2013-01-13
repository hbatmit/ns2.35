/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/* Copyright (c) 2004  Nils-Erik Mattsson 
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: dccp_tfrc.h,v 1.1 2010/04/06 05:09:01 tom_henderson Exp $ */

#ifndef ns_dccp_tfrc_h
#define ns_dccp_tfrc_h

#include "dccp.h"
#include "bsd_queue.h"

/* use traced variables */
#define DCCP_TFRC_USE_TRACED_VARS

#define DCCP_TFRC_CCID 3

/* features */
#define DCCP_TFRC_FEAT_LOSS_EVENT_RATE 192
#define DCCP_TFRC_FEAT_RTT_SCHEME 191      /* experimental */

/* rtt schemes (experimental) */
#define DCCP_TFRC_RTT_SCHEME_WIN_COUNT 0
#define DCCP_TFRC_RTT_SCHEME_OPTION 1

#define DCCP_TFRC_FEAT_DEF_LOSS_EVENT_RATE 1
#define DCCP_TFRC_FEAT_DEF_RTT_SCHEME DCCP_TFRC_RTT_SCHEME_WIN_COUNT

/* options */
#define DCCP_TFRC_OPT_LOSS_EVENT_RATE 192
#define DCCP_TFRC_OPT_RECV_RATE 194
#define DCCP_TFRC_OPT_RTT 191             /* experimental */
#define DCCP_TFRC_OPT_RTT_UNIT 0.000001

#define DCCP_TFRC_TIMER_SEND 128
#define DCCP_TFRC_TIMER_NO_FEEDBACK 129

#define DCCP_TFRC_MAX_WC_INC 5              /* limit increase in wc to this between two packets */
#define DCCP_TFRC_WIN_COUNT_PER_RTT  4

//quiescence
#define DCCP_TFRC_MIN_T 0.2          //T value for quiescence detection
#define DCCP_TFRC_QUIESCENCE_OPT_RATIO 1

#define DCCP_TFRC_STD_PACKET_SIZE    500     /* standard packet size*/


/* 
 * TFRC sender 
 */

/* TFRC sender states */
#define DCCP_TFRC_NUM_S_STATES 3
enum dccp_tfrc_s_state { TFRC_S_STATE_NO_SENT = 0,
			 TFRC_S_STATE_NO_FBACK = 1,
			 TFRC_S_STATE_FBACK = 2 };

/* Mechanism parameters */

/* initial sending rate rfc 3448 */
#define DCCP_TFRC_INIT_SEND_RATE DCCP_TFRC_STD_PACKET_SIZE

#define DCCP_TFRC_OPSYS_TIME_GRAN    0.01    /* operating system timer granularity in s */
#define DCCP_TFRC_SMALLEST_P         0.00001 /* smallest value of p allowed */

#define DCCP_TFRC_RTT_Q   0.9   /* RTT Filter constant */ 
#define DCCP_TFRC_RTT_Q2  0.9   /* Long term RTT filter constant */
#define DCCP_TFRC_MBI  64.0     /* max backoff interval */
#define DCCP_TFRC_INIT_RTO  2.0 /* initial rto value */


/* Packet history */
STAILQ_HEAD(s_hist_head,s_hist_entry); 
 
struct s_hist_entry {
	STAILQ_ENTRY(s_hist_entry) linfo_;   /* Tail queue. */
	u_int32_t seq_num_;        /* Sequence number */
	double t_sent_;            /* When the packet was sent */
	u_int32_t win_count_;      /* Windowcounter for packet */
	u_int8_t ecn_;             /* ecn nonce (0 or 1) sent */
};


/* 
 * TFRC Receiver 
 */

/* TFRC receiver states */
#define DCCP_TFRC_NUM_R_STATES 2
enum dccp_tfrc_r_state { TFRC_R_STATE_NO_DATA = 0,
			 TFRC_R_STATE_DATA = 1};

/* Receiver mechanism parameters */
#define DCCP_TFRC_NUM_W  8          /* length(w[]) */
#define DCCP_TFRC_P_TOLERANCE 0.05  /* tolerance allowed when inverting the throughput equation */

/* Packet history */
STAILQ_HEAD(r_hist_head,r_hist_entry); 
 
struct r_hist_entry {
	STAILQ_ENTRY(r_hist_entry) linfo_;  /* Tail queue. */
	u_int32_t seq_num_;        /* Sequence number */
	double t_recv_;            /* When the packet was received */
	u_int8_t win_count_;       /* Window counter for that packet */
	dccp_packet_type type_;    /* Packet type received */
	u_int8_t ndp_;             /* no data packets value */
	u_int16_t size_;           /* size of data in packet */
	dccp_ecn_codepoint ecn_;   /* ecn codepoint */
};

/* Loss interval history */
TAILQ_HEAD(li_hist_head,li_hist_entry); 
 
struct li_hist_entry {
	TAILQ_ENTRY(li_hist_entry) linfo_;   /* Tail queue. */
	u_int32_t interval_;       /* Loss interval */
	u_int32_t seq_num_;        /* Sequence number of the packet that started the interval */
	u_int8_t win_count_;       /* Window counter for previous received packet */
};

//forward decleration of class DCCPTCPlikeAgent
class DCCPTFRCAgent;

//The DCCPTFRCSendTimer sends packets according to a predetermined sending rate
class DCCPTFRCSendTimer : public TimerHandler {
protected:
	DCCPTFRCAgent *agent_;    //the owning agent
public:
	/* Constructor
	 * arg: agent - the owning agent (to notify about timeout)
	 * ret: A new DCCPTFRCSendTimer
	 */
	DCCPTFRCSendTimer(DCCPTFRCAgent* agent);

	/* Called when the timer has expired
	 * arg: e - The event that happened (i.e. the timer expired)
	 */
	virtual void expire(Event *e);
};

/* The DCCPTFRCNoFeedbackTimer expires when no feedback has been received */ 
class DCCPTFRCNoFeedbackTimer : public TimerHandler {
protected:
	DCCPTFRCAgent *agent_;    //the owning agent
public:
	/* Constructor
	 * arg: agent - the owning agent (to notify about timeout)
	 * ret: A new DCCPTFRCNoFeedbackTimer
	 */
	DCCPTFRCNoFeedbackTimer(DCCPTFRCAgent* agent);

	/* Called when the timer has expired
	 * arg: e - The event that happened (i.e. the timer expired)
	 */
	virtual void expire(Event *e);
};

/* The DCCPTFRC agent is based on the TFRC code from the FreeBSD
 * implementation at www.dccp.org.
 * If changes are made to tcl variables, the agent should be reset
 * to enable all changes.
 * Conforms to the October 2003 drafts.
 */
class DCCPTFRCAgent : public DCCPAgent {
private:
	/* The number of packets with higher sequence number needed before
	 * a delayed packet is considered lost (similiar to NUMDUPACKS) */
	int num_dup_acks_;

	double p_tol_;              /* tolerance in p when inverting thrp eq */

	//features
	int use_loss_rate_local_;
	int use_loss_rate_remote_;

	int rtt_scheme_local_;
	int rtt_scheme_remote_;
	
	int win_count_per_rtt_;     /* Number of wincount ticks in one RTT */
	int max_wc_inc_;            /* Maximum increment of win count */
	
	double q_min_t_;            /* T value for quiescence detection */
	//sender

#ifdef DCCP_TFRC_USE_TRACED_VARS
	TracedDouble s_x_;                /* Current sending rate (bytes/s) */
	TracedDouble s_x_inst_;           /* Instantaneous send rate */
	TracedDouble s_x_recv_;           /* Receive rate  (bytes/s) */
	TracedDouble s_r_sample_;         /* Last rtt sample */
	TracedDouble s_rtt_;              /* Estimate of current round trip time (s) */
	TracedDouble s_r_sqmean_;         /* Long term rtt */
	TracedDouble s_p_;                /* Current loss event rate */
#else
	double s_x_;                /* Current sending rate (bytes/s) */
	double s_x_inst_;           /* Instantaneous send rate */
	double s_x_recv_;           /* Receive rate  (bytes/s) */
	double s_r_sample_;         /* Last rtt sample */
	double s_rtt_;              /* Estimate of current round trip time (s) */
	double s_r_sqmean_;         /* Long term rtt */
	double s_p_;                /* Current loss event rate */
#endif
	double s_initial_x_;        /* Initial send rate (in bytes/s) */
	double s_initial_rto_;      /* Initial timeout (in s) */
	double s_rtt_q_;            /* RTT Filter constant */ 
	double s_rtt_q2_;           /* Long term RTT filter constant */
	double s_t_mbi_;            /* Max back-off interval (in s) */

	int s_use_osc_prev_;        /* Use oscillation prevention? */

	/* Timers */
	DCCPTFRCSendTimer *s_timer_send_;
	DCCPTFRCNoFeedbackTimer	*s_timer_no_feedback_;

	dccp_tfrc_s_state s_state_; /* Sender state */
	
	double   s_x_calc_;         /* Calculated send rate (bytes/s) */ 
	
	int      s_s_;              /* Packet size (bytes) */

	double   s_smallest_p_;     /* Smallest p value */
	
	u_int32_t s_last_win_count_; /* Last window counter sent */
	u_int32_t s_wc_;             /* Window counter */
	u_int32_t s_recv_wc_;        /* Last window counter of last ack packet */
	double s_t_last_win_count_; /* Timestamp of earliest packet with
				        last_win_count value sent */
	u_int8_t s_idle_;
	double   s_t_rto_;          /* Time out value = 4*rtt */
	double   s_t_ld_;           /* Time last doubled during slow start */
	
	double   s_t_nom_,          /* Nominal send time of next packet */
		 s_t_ipi_,          /* Interpacket (send) interval */
		 s_delta_;          /* Send timer delta */    

	double   s_os_time_gran_;   /* Operating system timer granularity */
	
	struct s_hist_head s_hist_; /* Packet history */
	
	u_int32_t s_hist_last_seq_; /* last sequence number in history */

	
	//quiescence
	double s_t_last_data_sent_;  /* timestamp of when last data pkt was sent */ 
	int s_q_opt_ratio_;          /* add Q opt on each opt ratio acks sent */
	int s_q_packets_wo_opt_;     /* ack packets sent without Q opt */

	/* Clear the send history */
	void clearSendHistory();

	/* Find a specific packet in the send history
	 * arg: seq_num - sequence number of packet to find
	 *      start_elm - start searching from this element
	 * ret: pointer to the correct element if the element is found
	 *      otherwise the closest packet with lower seq num is returned
	 *      which can be NULL.
	 */
	struct s_hist_entry* findPacketInSendHistory(u_int32_t seq_num, s_hist_entry* start_elm);

	/* Remove packets from send history
	 * arg: trim_to - remove packets with lower sequence numbers than this
	 * Note: if trim_to does not exist in the history, trim_to is set
	 *       to the next lower sequence number found before trimming. 
	 */
	void trimSendHistory(u_int32_t trim_to);
	
	/* Prints the contents of the send history */
	void printSendHistory();
	
	/* from project (slightly modified) */
	
	/* Halve the sending rate when no feedback timer expires */
	void  tfrc_time_no_feedback();

	/* Set the send timer
	 * arg: t_now - time now
	 */
	void  tfrc_set_send_timer(double t_now);

	/* Update the sending rate
	 * arg: t_now - time now
	 */
	void  tfrc_updateX(double t_now);

	/* Calculate sending rate from the throughput equation
	 * arg: s - packet size (in bytes)
	 *      R - round trip time (int seconds)
	 *      p - loss event rate
	 * ret: maximum allowed sending rate
	 */
	double tfrc_calcX(u_int16_t s, double R, double p);

	/* Find the loss event rate corresponding to a send rate
	 * in the TCP throughput eq.
	 * args: s - packet size (in bytes)
	 *       R - Round trip time  (in seconds)
	 *       x - send rate (in bytes/s)
	 * returns:  the loss eventrate accurate to 5% with resp. to x
	 */
	double tfrc_calcP(u_int16_t s, double R, double x);

	/* Similar to send_askPermToSend(), send_packetSent(), send_packetRecv() */
	int tfrc_send_packet(int datasize);
	void tfrc_send_packet_sent(Packet *pkt, int moreToSend, int datasize);
	void tfrc_send_packet_recv(Packet *pkt);

	//receiver

#ifdef DCCP_TFRC_USE_TRACED_VARS
	TracedDouble    r_rtt_;          //receivers estimation of rtt
	TracedDouble    r_p_;            /* Loss event rate */
#else
	double    r_rtt_;                //receivers estimation of rtt
	double    r_p_;                  /* Loss event rate */
#endif
	dccp_tfrc_r_state r_state_;      /* Receiver state */
	
	struct li_hist_head r_li_hist_;  /* Loss interval history */
	
	u_int8_t  r_last_counter_;       /* Highest value of the window counter
                                            received when last feedback was sent */
	u_int32_t r_seq_last_counter_;   /* Sequence number of the packet above */
	
	double    r_t_last_feedback_;    /* Timestamp of when last feedback was sent */
	u_int32_t r_bytes_recv_;         /* Bytes received since t_last_feedback */
	
	struct r_hist_head r_hist_;      /* Packet history */
	
	int       r_s_;                  /* Packet size */

	int r_num_w_;                    /* Number of weights */
	double *r_w_;                    /* Weights */
	
	u_int32_t r_high_seq_recv_;      //highest sequence number received 
	int       r_high_ndp_recv_;      //ndp value of high_seq_recv_ 
	double    r_t_high_recv_;        //timestamp of high_seq_recv_
	r_hist_entry *r_last_data_pkt_;  //pointer to last data packet recv
	
	//quiescence
	double r_q_t_data_;             //timestamp of last data pkt observed
	bool r_sender_quiescent_;      //the sender is quiescent (q opt recv)

	/* Clear the history of received packets */
	void clearRecvHistory();

	/* Insert a packet in the receive history
	 * Will not insert "lost" packets or duplicates.
	 * arg: packet - entry representing the packet to insert
	 * ret: true if packet was inserted, otherwise false.
	 */
	bool insertInRecvHistory(r_hist_entry *packet);

	/* Prints the contents of the receive history */
	void printRecvHistory();

	/* Trim receive history
	 * arg: time - remove packet with recv time less than this
	 *      seq_num - remove packet with seq num less than this
	 * Note: both of the above condition must be true for a packet
	 *       to be removed. Furthermore, the function always keeps
	 *       at least num_dup_acks_ packets in the history.
	 */
	void trimRecvHistory(double time, u_int32_t seq_num);

	/* Remove all acks after the num_dup_acks_ limit */
	void removeAcksRecvHistory();

	/* Find a data packet in the receive history
	 * arg: start - pointer to the first packet to search from
	 * ret: pointer to the found data packet or
	 *      or NULL if none is found.
         */
	r_hist_entry *findDataPacketInRecvHistory(r_hist_entry *start);

	/* Detects packet loss in recv history.
	 * arg: seq_start, seq_end - the lost packet is in [start,end]
	 *      win_count          - window counter of packet before the loss
	 * ret: true if a packet loss was detected, otherwise false.
	 */
	bool detectLossRecv(u_int32_t *seq_start, u_int32_t *seq_end, u_int8_t *win_count);

	/* Detect packet marks in recv history before num_dup_ack limit.
	 * arg: seq_num - sequence number of marked packet (if any)
	 *      win_count - win_count of marked packet (if any)
	 * ret: true if a packet mark was found, otherwise false.
	 */
	bool detectECNRecv(u_int32_t *seq_num, u_int8_t *win_count);

	/* Calculate the total amount of data in recent packets.
	 * arg: time - sum data in packets received later than this time
	 * ret: total amount of data
	 */
	u_int32_t sumPktSizes(double time);

	/* Sample the round trip time
	 * arg: rtt - the obtained rtt
	 *      last_seq - sequence number of oldest packet used
	 * ret: true if the rtt could succesfully be estimated from wc
	 *      false otherwise. Note that rtt above is always valid.
	 * Note: If not enough packets exist to use wc to estimate rtt,
	 *       the rtt measured on handshake packets are used.
	 *       The function will disregard packets with wc >= max_wc_inc_. 
	 */
	bool sampleRTT(double *rtt, u_int32_t *last_seq);


	/* Calculate the receive rate to send on feedbacks
	 * ret: the receive rate
	 */
	double calcXrecv();

	/* Clear the loss event history */
	void clearLIHistory();

	/* Print the loss event history */
	void printLIHistory();
	
	/* Detects quiescence of the corresponding sender
	 * ret: true if sender is quiescent
	 *      false otherwise
	 */ 
	bool detectQuiescence();

	/* from project (slightly modified) */

	/* Calculate the average loss interval
	 * ret: average loss interval (1/p)
	 */
	double tfrc_calcImean();

	/* Prepare and send a feedback packet */
	void tfrc_recv_send_feedback();

	/* Approximate the length of the first loss interval
	 * ret: the length of the first loss interval
	 */
	u_int32_t tfrc_recv_calcFirstLI();

	/* Update the loss interval history */
	void tfrc_recv_updateLI();

	/* Similar to recv_packetRecv */
	void tfrc_recv_packet_recv(Packet* pkt, int dataSize);
	
protected:
	
	/* Methods inherited from DCCPAgent.
	 * See dccp.h For detailed information regarding arguments etc.
	 */
	
	virtual void delay_bind_init_all();
        virtual int delay_bind_dispatch(const char *varName, const char *localName, TclObject *tracer);
	
	virtual void reset();
	virtual void cancelTimers();

	/* Process incoming options.
	 * This function will process:
	 *      ack vectors (verify ECN Nonce Echo)
	 *      quiescence option
	 *      rtt option
	 */
	virtual bool processOption(u_int8_t type, u_char* data, u_int8_t size, Packet *pkt);

	virtual void buildInitialFeatureList();

	virtual int setFeature(u_int8_t num, dccp_feature_location location,
			       u_char* data, u_int8_t size, bool testSet = false);

	virtual int getFeature(u_int8_t num, dccp_feature_location location,
			       u_char* data, u_int8_t maxSize);
	
	virtual dccp_feature_type getFeatureType(u_int8_t num);
	
	virtual bool send_askPermToSend(int dataSize, Packet *pkt);
	virtual void send_packetSent(Packet *pkt, bool moreToSend, int dataSize);
	virtual void send_packetRecv(Packet *pkt, int dataSize);
	virtual void recv_packetRecv(Packet *pkt, int dataSize);

	virtual void traceAll();
	virtual void traceVar(TracedVar* v);
public:
	/* Constructor
	 * ret: a new DCCPTCPlikeAgent
	 */
	DCCPTFRCAgent();
	
	/* Destructor */
	virtual ~DCCPTFRCAgent();

	/* Process a "function call" from OTCl
         * 
	 * Supported function call handled by this agent:
	 * weights <count>+<w1>+<w2>....  - set the weights for avg li calc.
	 * example  $dccp weights 8+1.0+1.0+1.0+1.0+0.8+0.6+0.4+0.2
	 */
	int command(int argc, const char*const* argv);

	/* A timeout has occured
	 * arg: tno - id of timeout event
	 * Handles: Send timer - DCCP_TFRC_TIMER_SEND
	 *          No feedback timer - DCCP_TFRC_TIMER_NO_FEEDBACK
	 */
	virtual void timeout(int tno);

};
#endif

