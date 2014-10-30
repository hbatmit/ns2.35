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
 * $Id: dccp.h,v 1.1 2010/04/06 05:09:01 tom_henderson Exp $ */

#ifndef ns_dccp_h
#define ns_dccp_h

#include "agent.h"
#include "dccp_types.h"
#include "dccp_packets.h"
#include "packet.h"
#include "dccp_sb.h"
#include "dccp_opt.h"
#include "dccp_ackv.h"
#include "rng.h"

//ccid for DCCP agent (non-standard!)
#define DCCP_CCID 254

//limits for ndp and ccval
#define DCCP_NDP_LIMIT 8
#define DCCP_CCVAL_LIMIT 16

//checksum coverage
#define DCCP_CSCOV_ALL 0
#define DCCP_CSCOV_HEADER_ONLY 1

//timer identifiers
#define DCCP_TIMER_RTX 0
#define DCCP_TIMER_SND 1

//retransmission parameters
#define	DCCP_INITIAL_RTX_TO 3.0
#define DCCP_MAX_RTX_TO 75.0
#define DCCP_RESP_TO 75.0

//send delay for send timer
#define DCCP_SND_DELAY 0.0001

#define DCCP_SB_SIZE 1000   //maximum number of packets in the send buffer 
#define DCCP_OPT_SIZE 512   //maximum size of options per packet
#define DCCP_FEAT_SIZE 24   //maximum size of ongoing feat neg
#define DCCP_ACKV_SIZE 20   //initial maximum size of ackvector

//features
#define DCCP_FEAT_CC 1
#define DCCP_FEAT_ECN 2
#define DCCP_FEAT_ACK_RATIO 3
#define DCCP_FEAT_ACKV 4

//Quiescence features (experimental)
#define DCCP_FEAT_Q_SCHEME 126  //quiescence scheme to use
#define DCCP_FEAT_Q 127         //quiescence feature

/* Quiescence scheme */
#define DCCP_Q_SCHEME_NORMAL 0
#define DCCP_Q_SCHEME_Q_FEAT  1
#define DCCP_Q_SCHEME_Q_OPT 2

//default values for features
#define DCCP_FEAT_DEF_CCID 2
#define DCCP_FEAT_DEF_ECN 1
#define DCCP_FEAT_DEF_ACK_RATIO 2
#define DCCP_FEAT_DEF_ACKV 0
#define DCCP_FEAT_DEF_Q_SCHEME DCCP_Q_SCHEME_NORMAL
#define DCCP_FEAT_DEF_Q 0

//error values for features
#define DCCP_FEAT_OK 0
#define DCCP_FEAT_UNKNOWN -1
#define DCCP_FEAT_NOT_PREFERED -2
#define DCCP_FEAT_ERR_SIZE -3
#define DCCP_FEAT_ERR_TEST -4

#define DCCP_NUM_DUP_ACKS 3

//forward decleration of class DCCPAgent
class DCCPAgent;

/* The DCCPRetransmitTimer is used for retransmission of packet
 * or as a timeout while waiting for packets.
 */
class DCCPRetransmitTimer : public TimerHandler {
protected:
	DCCPAgent *agent_;     //the owning agent
	double delay_;         //the scheduled delay
	double tot_delay_;     //total delayed time so far
	double max_tot_delay_; //maximum delayed time
	bool backoff_failed_;  //true if max_tot_dealay_ is reached
public:
	/* Constructor
	 * arg: agent - the owning agent (to notify about timeout)
	 * ret: A new DCCPRetransmitTimer
	 */
	DCCPRetransmitTimer(DCCPAgent* agent);

	/* Initialize the timer.
	 * arg: delay       - initial delay
	 *      maxToTDelay - total maximum delay
	 */
	void init(double delay, double maxTotDelay);

	/* Called when the timer has expired
	 * arg: e - The event that happened (i.e. the timer expired)
	 */
	virtual void expire(Event *e);

	/* Back off the timer (by a factor of 2)
	 * The last back off will reach the max_tot_delay.
	 */
	void backOff();

	/* Check if the back-off failed (that is if max_tot_delay is reached)
	 * ret: true if back-off failed, otherwise false. 
	 */
	bool backOffFailed();
};

/* The DCCPSendTimer models the time it takes to send a packet */
class DCCPSendTimer : public TimerHandler {
protected:
	DCCPAgent *agent_;  //the owning agent
public:
	/* Constructor
	 * arg: agent - the owning agent (to notify about timeout)
	 * ret: A new DCCPSendTimer
	 */
	DCCPSendTimer(DCCPAgent* agent);

	/* Called when the timer has expired
	 * arg: e - The event that happened (i.e. the timer expired)
	 */
	virtual void expire(Event *e);
};

/* Class DCCPAgent implements a two-way agent using the DCCP protocol.
 * Conforms to the October 2003 drafts.
 * If changes are made to tcl variables, the agent should be reset
 * to enable all changes.
 * It provides a "cc" that simulates that it takes send_delay_ seconds
 * to send a packet.
 * Acks ackording to ack ratio.
 * Can use ecn and/or ack vectors (no ecn verification) */
class DCCPAgent : public Agent {
private:
	RNG* nonces_;   //random number generator

	//string representation of types
	static char* state_str_[DCCP_NUM_STATES];
	static int hdr_size_[DCCP_NUM_PTYPES];
	static char* ptype_str_[DCCP_NUM_PTYPES];
	static char* reset_reason_str_[DCCP_NUM_RESET_REASONS];
	static char* feature_location_str_[DCCP_NUM_FEAT_LOCS];
	
	bool server_;         //true if this agent i a server
	dccp_state state_;    //current protocol state
	bool packet_sent_;    //true after the first packet has been sent
	bool packet_recv_;    //true after the first packet has been received

	bool close_on_empty_; //if true, the connection is closed when the sendbuffer is empty
	
	//list of feature currently under negotiation
	//consider these variables as one list
        u_int8_t *feat_list_num_;              //feature number
	dccp_feature_location *feat_list_loc_; //feature location
	u_int32_t *feat_list_seq_num_;         //sequence number of the packet the feature was last negotiated on
	bool *feat_list_first_;                //true if this feat has not been neg before
	u_int8_t feat_list_used_;   //number of entries in the list

	//list over features to confirm
	u_int8_t *feat_conf_num_;              //feature number
	dccp_feature_location *feat_conf_loc_; //feature location
	u_int8_t feat_conf_used_;   //number of entries in the list

	int allow_mult_neg_;       //allow multiple nn feature negotiations at one time
	
	//sequence number for the packet with last neg option processed
	u_int32_t seq_last_feat_neg_; 
	bool feat_first_in_pkt_;  //is this the first feat neg opt in pkt?
	
	DCCPRetransmitTimer* timer_rtx_;  //retransmit timer

	DCCPSendTimer* timer_snd_;        //send timer
	double snd_delay_;    //the delay of the send timer

	/* Creates a new packet and fills in some header fields
	 * ret: a new packet
	 */
	Packet* newPacket();

	/* Check if an incoming packet is valid
	 * arg: pkt - incoming packet
	 * ret: true if the packet is valid, otherwise false
	 */
	bool checkPacket(Packet* pkt);

	/* Add feature negotiation options on packets for features
	 * currently under negotiation. Will use getFeature() to obtain values.
	 */
	void addFeatureOptions();

	/* Finish feature negotiation for a feature abd remove it from
	 * the list of ongoing feature negotiations.
	 * arg: num      - feature number
	 *      location - feature location
	 * ret: true if the feature is removed
	 *      false if it doesn't exist in the list
	 */
	bool finishFeatureNegotiation(u_int8_t num, dccp_feature_location location);

	/* Add a feature to the list of features to confirm
	 * Will only add one entry for each (feat,loc) pair
	 * arg: num      - feature number
	 *      location - feature location
	 */
	void confirmFeature(u_int8_t num, dccp_feature_location location);

	/* Find a feature in the list of ongoing negotiations
	 * arg: num      - feature number
	 *      location - feature location
	 * ret: position in the feature list if successfull
	 *      otherwise -1
	 */
	int findFeatureInList(u_int8_t num, dccp_feature_location location);
	
protected:
	int ndp_limit_;       //ndp limit
	int ccval_limit_;     //ccval limit
	
	DCCPSendBuffer* sb_;  //send buffer. Contains packet sizes to send.
	bool infinite_send_;  //send an unlimited amount of data
	
	u_int8_t ndp_;      //ndp of last packet sent
	u_int32_t seq_num_; //next sequence number to send

	u_int32_t seq_num_recv_;  //highest sequence number received so far
	u_int32_t ack_num_recv_;  //highest ack received so far

	DCCPAckVector *ackv_recv_;      //the received ack vector (if any)
	u_int32_t elapsed_time_recv_;   //the received time elapsed (if any)
	
	u_int32_t ack_num_; //sequence number to ack
	bool send_ack_;     //send an ack
	u_int8_t ccval_;    //cc value to send
	int cscov_;         //checksum coverage value to send
	
	bool conn_est_;     //true if the connection is established

	bool output_;       //run output() after packet has been received
	bool output_flag_;  //flag to hand to output. i.e. output(output_flag_)
	
	DCCPOptions *opt_;  //options to attach to the next outgoing packet

	//feature variables (name_location_)
	int ccid_;             //ccid is the same on both locations
	int use_ecn_local_;   
	int use_ecn_remote_;
	int ack_ratio_local_;
	int ack_ratio_remote_;
	int use_ackv_local_;
	int use_ackv_remote_;
	int q_scheme_;         //quiescence scheme is the same on both locations
	int q_local_;          //specifies if the remote sender is quiescent
	int q_remote_;         //specifies if I am quiescent
	
	DCCPAckVector *ackv_;  //local ack_vector of all incoming packets
	bool send_ackv_;       //if true, send ack vector on all acks 
	bool manage_ackv_;     //if true, add incoming packets to ackv_ and clear state on ack-of-ack
	
	int nam_tracevar_;    
	int trace_all_oneline_;

	double initial_rtx_to_; //inital value for rtx timer delay
	double max_rtx_to_;     //maximum total rtx delay (then back-off fails)
	double resp_to_;        //response timer delay

	int sb_size_;           //send buffer size
	int opt_size_;          //maximum size of options on packets
	int feat_size_;         //maximum number of ongoing feat neg
	int ackv_size_;         //initial ackv_ size

	int ar_unacked_;        //unacked data packets received

	double rtt_conn_est_;   //rtt measured during connection establishment
	
	/* Return the current (simulated) time
	 * ret: current time
	 */
	inline double now() { return (&Scheduler::instance() == NULL ? 0 : Scheduler::instance().clock()); }

	/* OTcl binding of variables */
	virtual void delay_bind_init_all();
        virtual int delay_bind_dispatch(const char *varName, const char *localName, TclObject *tracer);

	/* Return a string representation of a type */
	const char* stateAsStr(dccp_state state);
	const char* packetTypeAsStr(dccp_packet_type type);
	const char* resetReasonAsStr(dccp_reset_reason reason);
	const char* featureLocationAsStr(dccp_feature_location location);
	/* Return the header size (without options) of a packet type
	 * arg: type - packet type
	 * ret: header size
	 */
	int headerSize(dccp_packet_type type);

	/* Extract the ecn codepoint
	 * arg: pkt - packet
	 * ret: ecn codepoint of packet
	 */
	static dccp_ecn_codepoint getECNCodePoint(Packet* pkt);

	/* Get the packet nonce
	 * arg: pkt - packet
	 * ret: 0 if ECT(0) is set
	 *      1 if ECT(1) is set
	 *      otherwise -1
	 */
	static int getNonce(Packet* pkt);

	/* Changes state (and cancel/init/sched timers)
	 * arg: new_state - new state
	 */
	void changeState(dccp_state new_state);

	/* Reinitialize the agent */
	virtual void reset();

	/* Send a packet.
	 * arg: try_pure_ack - if true, try to send a pure ack if cc
	 *                     refuses to send a dataack
	 */		       
	void output(bool try_pure_ack = false);

	/* Send a reset packet.
	 * arg: reason  - reason for reset
	 *      data<X> - data to include on reset packet
	 */
	void sendReset(dccp_reset_reason reason, u_int8_t data1,
		       u_int8_t data2, u_int8_t data3);

	/* Parse options in a packet.
	 * Will call parseOption() on every option found.
	 * Fails if parseOption() fails.
	 * arg: pkt - packet
	 * ret: true if parse is successful
	 *      false if the parse failed and the connection should be reset
	 */
	virtual bool parseOptions(Packet *pkt);

	/* Process an incoming option.
	 * arg: type - option type
	 *      data - option data
	 *      size - size of opption data
	 *      pkt  - the packet the option was received on
	 * ret: true if option processing was successful
	 *      false if it fails
	 * This function will process:
	 *      feature negotiation options
 	 *      ack vectors (extraction only)
	 *      elapsed time (extraction only)      
	 */	
	virtual bool processOption(u_int8_t type, u_char* data, u_int8_t size, Packet *pkt);

	/* Change a feature (i.e. initiate a feature negotiation).
	 * arg: num      - feature number
	 *      location - feature location
	 * ret: true if the feature is added to the list of ongoing feat neg
	 *      false if the list is full or the feature is already present
	 */
	bool changeFeature(u_int8_t num, dccp_feature_location location);

	/* Check if a feature is currently under negotiation.
	 * arg: num      - feature number
	 *      location - feature location
	 * ret: true if the feature is under negotiation
	 *      false otherwise
	 */
	bool featureIsChanging(u_int8_t num, dccp_feature_location location);

	/* Build the list of features to neg on DCCP-Request packet */
	virtual void buildInitialFeatureList();

	/* Set a value for a specific feature.
	 * arg: num      - feature number
	 *      location - feature location
	 *      data     - feature data
	 *      size     - size of data
	 *      testSet  - if true, only check value for correctness, don't set
	 * ret: DCCP_FEAT_OK  if successful
	 *      DCCP_FEAT_UNKNOWN  if the feature number is unknown
	 *      DCCP_FEAT_NOT_PREFERED  if the value is not prefered
	 *      DCCP_FEAT_ERR_SIZE  if the size does not match the feature
	 *      DCCP_FEAT_ERR_TEST  if a non-neg feature failed the test.
	 *                          a negotiable feature that fails returns NOT_PREFERED
	 * Note: DCCP_FEAT_NOT_PREFERED is not applicable for non neg features.
	 *       NOT_PREFERED and ERR_SIZE will trigger a reset
	 */
	virtual int setFeature(u_int8_t num, dccp_feature_location location,
			       u_char* data, u_int8_t size, bool testSet = false);

	/* Obtain a value of a feature.
	 * Used in addFeatureOptions() to get the feature value.
	 * arg: num      - feature number
	 *      location - feature location
	 *      data     - feature data
	 *      size     - maximum size of data
	 * ret: DCCP_FEAT_OK            if successful
	 *      DCCP_FEAT_UNKNOWN       if the feature number is unknown
	 *      DCCP_FEAT_ERR_SIZE      if the size is too small
	 */
	virtual int getFeature(u_int8_t num, dccp_feature_location location,
			       u_char* data, u_int8_t maxSize);

	/* Obtain the feature type
	 * arg: num      - feature number
	 * ret: the feature type, or DCCP_FEAT_TYPE_UKNOWN if unknown
	 */
	virtual dccp_feature_type getFeatureType(u_int8_t num);

	/* Cancel all timers */
	virtual void cancelTimers();

	//Interface to cc

	/* Ask sender permission to send a packet
	 * arg: dataSize - size of data in packet (0 = pure ACK)
	 *      pkt      - the packet to send
	 * ret: true if permission is granted, otherwise false
	 * Note: Packet should not be used for other than manipulating
	 *       ecn marks!
	 */
	virtual bool send_askPermToSend(int dataSize, Packet *pkt);

	/* A(n) ACK/DATA/DATAACK packet has been sent (sender)
	 * arg: pkt        - packet sent
	 *      moreToSend - true if there exist more data to send
	 *      dataSize   - size of data sent
	 */
	virtual void send_packetSent(Packet *pkt, bool moreToSend, int dataSize);
	/* A(n) ACK/DATA/DATAACK packet has been received (sender)
	 * arg: pkt        - packet received
	 *      dataSize   - size of data in packet
	 * If this function would like to send a packet, set output_ = true
	 * and output_flag_ if appropriate.
	 */
	virtual void send_packetRecv(Packet *pkt, int dataSize);

	/* A ACK/DATA/DATAACK packet has been received (receiver)
	 * arg: pkt        - packet received
	 *      dataSize   - size of data in packet
	 * If this function would like to send a packet, set output_ = true
	 * and output_flag_ if appropriate.
	 */
	virtual void recv_packetRecv(Packet *pkt, int dataSize);

	/* Tracing functions */
	virtual void traceAll();
	virtual void traceVar(TracedVar* v);
public:

	//statistics on sent packets
	
	TracedInt num_data_pkt_;
	TracedInt num_ack_pkt_;
	TracedInt num_dataack_pkt_;
	
	/* Constructor
	 * ret: A new DCCPAgent
	 */
	DCCPAgent();

	/* Destructor */
	virtual ~DCCPAgent();

	
	/* Process a "function call" from OTCl
	 * arg: argc - number of arguments
	 *      argv - arguments
	 * ret: TCL_OK if successful, TCL_ERROR otherwise
         *
	 * Supported function call handled by this agent:
	 * advanceby x   -  advances num packets
	 * advance x     -  = advanceby x  
	 */
	int command(int argc, const char*const* argv);

	/* Receive a packet
	 * arg: pkt     - Packet received
	 *      handler - handler
	 */
	virtual void recv(Packet* pkt, Handler* handler);

	/* Send a packet,
	 * If this is the first packet to send, initiate a connection.
	 * arg: nbytes - number of bytes in packet (-1 -> infinite send)
	 *      flags  - Flags:
	 *                 "MSG_EOF" will close the connection when all data
	 *                  has been flag
	 */
	virtual void sendmsg(int nbytes, const char *flags = 0);

	/* Close the connection */
	virtual void close();

	/* Listen for incoming connections
	 * Use close() to stop listen
	 */ 
	virtual void listen();

	/* A timeout has occured
	 * arg: tno - id of timeout event
	 * Handles: Retransmit timer - DCCP_TIMER_RTX
	 *          Send timer - DCCP_TIMER_SND
	 */
	virtual void timeout(int tno);

	/* Trace a variable.
	 * arg: v  - traced variable
	 */
	void trace(TracedVar* v);

	/* Send delta packets of packetSize_ size
	 * arg: delta - number of packets to send
	 */
	void advanceby(int delta);
};

#endif

