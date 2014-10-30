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
 * $Id: dccp_tcplike.h,v 1.1 2010/04/06 05:09:01 tom_henderson Exp $ */

#ifndef ns_dccp_tcplike_h
#define ns_dccp_tcplike_h

#include "dccp.h"
#include "bsd_queue.h"

// Use tracevars for window and rtt variables
#define DCCP_TCPLIKE_USE_TRACED_VARS

#define DCCP_TCPLIKE_CCID 2

//initial values
#define DCCP_TCPLIKE_INIT_CWND 3     //rfc2581 says 2 but rfc3390 allows more
#define DCCP_TCPLIKE_CWND_TO 1       //cwnd is set to this value on timeout
#define DCCP_TCPLIKE_INIT_SSTHRESH 0xFFFF

//RTO parameters - rfc 2988
#define DCCP_TCPLIKE_INIT_RTO 3
#define DCCP_TCPLIKE_ALPHA 0.125
#define DCCP_TCPLIKE_BETA 0.25
#define DCCP_TCPLIKE_K 4
#define DCCP_TCPLIKE_G 0.010
#define DCCP_TCPLIKE_MIN_RTO 1.0

//Timers
#define DCCP_TCPLIKE_TIMER_TO 128
#define DCCP_TCPLIKE_TIMER_DACK 129

#define DCCP_TCPLIKE_DACK_DELAY 0.2  //delay for delayed acknowledgments
#define DCCP_TCPLIKE_MIN_T 0.2       //T value for quiescence detection 

//Quiescence
#define DCCP_TCPLIKE_QUIESCENCE_OPT_RATIO 1 
#define DCCP_TCPLIKE_ACKV_SIZE_LIM 10   //ack an ack when recv ackv > this

//forward decleration of class DCCPTCPlikeAgent
class DCCPTCPlikeAgent;

//The DCCPTCPlikeTOTimer is similar to TCP's retransmission timeouts
class DCCPTCPlikeTOTimer : public TimerHandler {
protected:
	DCCPTCPlikeAgent *agent_;    //the owning agent
public:
	/* Constructor
	 * arg: agent - the owning agent (to notify about timeout)
	 * ret: A new DCCPTCPlikeTOTimer
	 */
	DCCPTCPlikeTOTimer(DCCPTCPlikeAgent* agent);

	/* Called when the timer has expired
	 * arg: e - The event that happened (i.e. the timer expired)
	 */
	virtual void expire(Event *e);
};

/* The DCCPTCPlikeDelayedAckTimer expires when a data packet has not been
 * acknowledged within dack_delay_ s of its arrival */ 
class DCCPTCPlikeDelayedAckTimer : public TimerHandler {
protected:
	DCCPTCPlikeAgent *agent_;    //the owning agent
public:
	/* Constructor
	 * arg: agent - the owning agent (to notify about timeout)
	 * ret: A new DCCPTCPlikeDelayedAckTimer
	 */
	DCCPTCPlikeDelayedAckTimer(DCCPTCPlikeAgent* agent);

	/* Called when the timer has expired
	 * arg: e - The event that happened (i.e. the timer expired)
	 */
	virtual void expire(Event *e);
};

//packet history of sent data packets
STAILQ_HEAD(dccp_tcplike_send_hist,dccp_tcplike_send_hist_entry); 


struct dccp_tcplike_send_hist_entry {
	STAILQ_ENTRY(dccp_tcplike_send_hist_entry) linfo_;    
	u_int32_t seq_num_;     //sequence number
	double t_sent_;         //timestamp of sent packet
	u_int8_t ecn_;          //ecn nonce (0 or 1) sent
};

//packet history over received packets
STAILQ_HEAD(dccp_tcplike_send_recv_hist,dccp_tcplike_send_recv_hist_entry); 

struct dccp_tcplike_send_recv_hist_entry {
	STAILQ_ENTRY(dccp_tcplike_send_recv_hist_entry) linfo_;    
	u_int32_t seq_num_;     //sequence number
	dccp_packet_type type_; //packet type
	u_int8_t ndp_;          //ndp value
};

/* The class DCCPTCPlikeAgent implements a two way DCCP agent using
 * TCP-like (CCID 2) congestion control mechanism.
 * If changes are made to tcl variables, the agent should be reset
 * to enable all changes.
 * Conforms to the October 2003 drafts.
 */ 
class DCCPTCPlikeAgent : public DCCPAgent {
private:
	//sender

	int initial_cwnd_;        //initial congestion window
	int cwnd_timeout_;        //initial value for the cwnd after a timeout
	int initial_ssthresh_;    //initial value for slow start threshold
	double initial_rto_;      //initial rto value

#ifdef DCCP_TCPLIKE_USE_TRACED_VARS
	TracedInt cwnd_;          //congestion window
	TracedInt cwnd_frac_;     //fractional part of cwnd (in unit of 1/cwnd)
	TracedInt ssthresh_;      //slow start threshold
	TracedInt pipe_;          //outstanding packets in the network
	TracedDouble rto_;        //"retransmission" timeout value
	TracedDouble srtt_;       //smoothed rtt
	TracedDouble rttvar_;     //rtt variance
	TracedDouble rtt_sample_; //latest rtt sample
#else
	int cwnd_;
	int cwnd_frac_;
	int ssthresh_;
	int pipe_;
	double rto_;
	double srtt_;
	double rttvar_;
	double rtt_sample_;
#endif
	double alpha_;            //smoothing factor for rtt/rto calculations
	double beta_;             //smoothing factor for rtt/rto calculations
	double g_;                //timer granularity
	int k_;                   //k value

	double min_rto_;          //minimum rto allowed
	
	/* The number of packets with higher sequence number needed before
	 * a delayed packet is considered lost (similiar to NUMDUPACKS) */
	int num_dup_acks_;        

	//congestion control on data packets related attributes
	
	struct dccp_tcplike_send_hist send_hist_;  //history over sent packets
	u_int32_t hist_last_seq_; //last sequence number in history

	DCCPAckVector *stored_ackv_;  //state information about recv packets
	
	u_int32_t high_ack_recv_; //highest ack received so far
	u_int32_t seq_all_done_;  //seq nums <= this number have been processed

	/* The sequence number that started the new window after the
	 * last congestion event */
	u_int32_t seq_win_start_; 

	/* Decrease pipe for all packets acknowledged after and including
	 * this sequence number */
	u_int32_t seq_pipe_start_;
	
	//ack ratio related attributes
	int num_ack_in_win_;      //number of packets acknowledged
	int num_win_acked_;       //number of cwnd acked without lost/marked acks
	/* The sequence number that started the new window after the
	 * last congestion event regarding acks */
	u_int32_t ack_win_start_;
	
	bool skip_ack_loss_;      //if true, ack loss is ignored
	u_int16_t recv_ack_ratio_;//the ackratio to report to the receiver 

	//history over received packets
	struct dccp_tcplike_send_recv_hist send_recv_hist_;

	DCCPTCPlikeTOTimer *timer_to_;  //timeout timer

	//quiescence
	double t_last_data_sent_;  //timestamp of when last data pkt was sent 
	int q_opt_ratio_;          //add Q opt on each opt ratio acks sent
	int q_packets_wo_opt_;     //ack packets sent without Q opt

	int ackv_size_lim_;        //send an ack of ack if recv ackv >= this
	u_int32_t ackv_lim_seq_;   //sequencenumber of last ack of ack sent due to size limit
	
	/* Clear the send history */
	void clearSendHistory();

	/* Find a specific packet in the send history
	 * arg: seq_num - sequence number of packet to find
	 *      start_elm - start searching from this element
	 * ret: pointer to the correct element if the element is found
	 *      otherwise the closest packet with lower seq num is returned
	 *      which can be NULL.
	 */
	struct dccp_tcplike_send_hist_entry* findPacketInSendHistory(u_int32_t seq_num, dccp_tcplike_send_hist_entry* start_elm);

	/* Remove packets from send history
	 * arg: trim_to - remove packets with lower sequence numbers than this
	 * Note: if trim_to does not exist in the history, trim_to is set
	 *       to the next lower sequence number found before trimming. 
	 */
	void trimSendHistory(u_int32_t trim_to);
	
	/* Prints the contents of the send history */
	void printSendHistory();

	/* Clear the history of received packets */
	void clearSendRecvHistory();

	/* Insert a packet in the receive history
	 * Will not insert "lost" packets or duplicates.
	 * arg: packet - entry representing the packet to insert
	 * ret: true if packet was inserted, otherwise false.
	 */
	bool insertInSendRecvHistory(dccp_tcplike_send_recv_hist_entry *packet);
	/* Prints the contents of the receive history */
	void printSendRecvHistory();
	
	/* Detects packet loss in recv history.
	 * Trims the history to num_dup_acks_+1 length.
	 * arg: seq_start, seq_end - the lost packet is in [start,end]
	 * ret: true if a packet loss was detected, otherwise false.
	 */
	bool detectLossSendRecv(u_int32_t *seq_start, u_int32_t *seq_end);

	/* Check the constraints on the ack ratio (recv_ack_ratio_)
	 * ret: true if the ack ratio was changed due to constraints
	 *      false otherwise
	 */
	bool checkAckRatio();

	/* Compares recv_ack_ratio_ with ack_ratio_remote_ and
	 * tries to change the feauture if they differ.
         */
	void changeRemoteAckRatio();

	/* Update the ack ratio
	 * arg: num_ack - number of newly acknowledged packets
	 */
	void updateAckRatio(int num_ack);

	/* Take action on lost or marked ack packet */
	void lostOrMarkedAck();

	/* Update the congestion window
	 * arg: num_inc_cwnd - number of newly acknowledged packets
	 */
	void updateCwnd(int num_inc_cwnd);

	/* Take action on lost or marked data packet */
	void lostOrMarkedData();
	
	//receiver

	u_int32_t high_seq_recv_; //highest sequence number received 
	int high_ndp_recv_;       //ndp value of high_seq_recv_ 
	double t_high_recv_;      //timestamp of high_seq_recv_
	int unacked_;             //number of unacknowledged data packets

	//quiescence
	u_int32_t q_high_data_recv_;  //highest data packet received 
	double q_min_t_;           //T value for quiescence detection
	double q_t_data_;          //timestamp of last data pkt observed

	bool sender_quiescent_;      //the sender is quiescent (q opt recv)
	
	double dack_delay_;        //delay for delayed acknowledgments
	DCCPTCPlikeDelayedAckTimer *timer_dack_;   //delayed acknowledgments timer
	/* Detects quiescence of the corresponding sender
	 * ret: true if sender is quiescent
	 *      false otherwise
	 */ 
	bool detectQuiescence();
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
	 */
	virtual bool processOption(u_int8_t type, u_char* data, u_int8_t size, Packet *pkt);

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
	DCCPTCPlikeAgent();
	
	/* Destructor */
	virtual ~DCCPTCPlikeAgent();

	int command(int argc, const char*const* argv);

	/* A timeout has occured
	 * arg: tno - id of timeout event
	 * Handles: Delayed acknowledgement timer - DCCP_TCPLIKE_TIMER_DACK
	 *          Timeout timer - DCCP_TCPLIKE_TIMER_TO
	 */
	virtual void timeout(int tno);

};

#endif

