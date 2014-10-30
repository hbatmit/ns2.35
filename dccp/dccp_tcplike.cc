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
 * $Id: dccp_tcplike.cc,v 1.1 2010/04/06 05:09:01 tom_henderson Exp $ */

#include "ip.h"
#include "dccp_tcplike.h"
#include "flags.h"

#define DCCP_TCPLIKE_IN_WINDOW 1
#define DCCP_TCPLIKE_AFTER_WINDOW 0

//OTcl linkage for DCCP TCPlike agent
static class DCCPTCPlikeClass : public TclClass {
public:
	DCCPTCPlikeClass() : TclClass("Agent/DCCP/TCPlike") {};
	TclObject* create(int argc, const char*const* argv){
		return (new DCCPTCPlikeAgent());
	}
} class_dccptcplike;

//methods for timer classes

/* Constructor
 * arg: agent - the owning agent (to notify about timeout)
 * ret: A new DCCPTCPlikeTOTimer
 */
DCCPTCPlikeTOTimer::DCCPTCPlikeTOTimer(DCCPTCPlikeAgent* agent) : TimerHandler(){
	agent_ = agent;
}

/* Called when the timer has expired
 * arg: e - The event that happened (i.e. the timer expired)
 */
void DCCPTCPlikeTOTimer::expire(Event *e){
	agent_->timeout(DCCP_TCPLIKE_TIMER_TO);
}

/* Constructor
 * arg: agent - the owning agent (to notify about timeout)
 * ret: A new DCCPTCPlikeDelayedAckTimer
 */
DCCPTCPlikeDelayedAckTimer::DCCPTCPlikeDelayedAckTimer(DCCPTCPlikeAgent* agent) : TimerHandler(){
	agent_ = agent;
}

/* Called when the timer has expired
 * arg: e - The event that happened (i.e. the timer expired)
 */
void DCCPTCPlikeDelayedAckTimer::expire(Event *e){
	agent_->timeout(DCCP_TCPLIKE_TIMER_DACK);
}

//private methods


/* Clear the send history */
inline void DCCPTCPlikeAgent::clearSendHistory(){
	struct dccp_tcplike_send_hist_entry *elm, *elm2;

	/* Empty packet history */
	elm = STAILQ_FIRST(&send_hist_);
	while (elm != NULL) {
		elm2 = STAILQ_NEXT(elm, linfo_);
		delete elm;
		elm = elm2;
	}
	
	STAILQ_INIT(&send_hist_);
}

/* Find a specific packet in the send history
 * arg: seq_num - sequence number of packet to find
 *      start_elm - start searching from this element
 * ret: pointer to the correct element if the element is found
 *      otherwise the closest packet with lower seq num is returned
 *      which can be NULL.
 */
inline struct dccp_tcplike_send_hist_entry* DCCPTCPlikeAgent::findPacketInSendHistory(u_int32_t seq_num, dccp_tcplike_send_hist_entry* start_elm){
	struct dccp_tcplike_send_hist_entry *elm = start_elm;
	while (elm != NULL && elm->seq_num_ > seq_num)
		elm = STAILQ_NEXT(elm, linfo_);
	return elm;
}

/* Remove packets from send history
 * arg: trim_to - remove packets with lower sequence numbers than this
 * Note: if trim_to does not exist in the history, trim_to is set to the next
 *       lower sequence number found before trimming. 
 */
void DCCPTCPlikeAgent::trimSendHistory(u_int32_t trim_to){
	if (trim_to > hist_last_seq_){
		struct dccp_tcplike_send_hist_entry *elm, *elm2;
		//find packet corresponding to trim_to
		elm = findPacketInSendHistory(trim_to, STAILQ_FIRST(&send_hist_));
		if (elm != NULL){
			//remove older packets
			elm2 = STAILQ_NEXT(elm, linfo_);	
			while (elm2 != NULL){
				STAILQ_REMOVE(&send_hist_,elm2,dccp_tcplike_send_hist_entry,linfo_);
				delete elm2;
				elm2 = STAILQ_NEXT(elm, linfo_);	
			}
		}
		hist_last_seq_ = trim_to;
	}
}

/* Prints the contents of the send history */
void DCCPTCPlikeAgent::printSendHistory(){
	struct dccp_tcplike_send_hist_entry *elm = STAILQ_FIRST(&send_hist_);
	if (elm == NULL)
		fprintf(stdout, "Packet history is empty (send)\n");
	else {
		fprintf(stdout, "Packet history (send):\n");
		while(elm != NULL){
			fprintf(stdout,"Packet: seq %d, t_sent %f, ecn %d\n", elm->seq_num_, elm->t_sent_, elm->ecn_);
			elm = STAILQ_NEXT(elm, linfo_);
		}
	}
}

/* Clear the history of received packets */
inline void DCCPTCPlikeAgent::clearSendRecvHistory(){
	struct dccp_tcplike_send_recv_hist_entry *elm, *elm2;

	/* Empty packet history */
	elm = STAILQ_FIRST(&send_recv_hist_);
	while (elm != NULL) {
		elm2 = STAILQ_NEXT(elm, linfo_);
		delete elm;
		elm = elm2;
	}
	
	STAILQ_INIT(&send_recv_hist_);
}

/* Insert a packet in the receive history
 * Will not insert "lost" packets or duplicates.
 * arg: packet - entry representing the packet to insert
 * ret: true if packet was inserted, otherwise false.
 */
bool DCCPTCPlikeAgent::insertInSendRecvHistory(dccp_tcplike_send_recv_hist_entry *packet){
	struct dccp_tcplike_send_recv_hist_entry *elm, *elm2;
	int num_later = 0;
	
	if (STAILQ_EMPTY(&send_recv_hist_)){  //history is empty
		STAILQ_INSERT_HEAD(&send_recv_hist_, packet, linfo_);
	} else {  //history contains at least one entry
		elm = STAILQ_FIRST(&send_recv_hist_);
		if (packet->seq_num_ > elm->seq_num_)  //insert first in history
			STAILQ_INSERT_HEAD(&send_recv_hist_, packet, linfo_);
		else if (packet->seq_num_ == elm->seq_num_) //duplicate
			return false;
		else {  //packet should be inserted somewhere after the head
			num_later = 1;

			//walk through the history to find the correct place
			elm2 = STAILQ_NEXT(elm,linfo_);
			while(elm2 != NULL){
				if (packet->seq_num_ > elm2->seq_num_){
					STAILQ_INSERT_AFTER(&send_recv_hist_, elm, packet, linfo_);
					break;
				} else if (packet->seq_num_ == elm2->seq_num_) {
					return false;  //duplicate
				}
				
				elm = elm2;
				elm2 = STAILQ_NEXT(elm,linfo_);
				
				num_later++;
				
				if (num_later == num_dup_acks_){  //packet is "lost"
					return false;
				}
			}
			
			if(elm2 == NULL && num_later < num_dup_acks_){
				STAILQ_INSERT_TAIL(&send_recv_hist_, packet, linfo_);
			}
			
		}
	}
	return true;
}

/* Prints the contents of the receive history */
void DCCPTCPlikeAgent::printSendRecvHistory(){
	struct dccp_tcplike_send_recv_hist_entry *elm = STAILQ_FIRST(&send_recv_hist_);
	if (elm == NULL)
		fprintf(stdout, "Packet history is empty (send recv)\n");
	else {
		fprintf(stdout, "Packet history (send recv):\n");
		while(elm != NULL){
			fprintf(stdout,"Packet: seq %d, type %s, ndp %d\n", elm->seq_num_, packetTypeAsStr(elm->type_), elm->ndp_);
			elm = STAILQ_NEXT(elm, linfo_);
		}
	}	
}

/* Detects packet loss in recv history.
 * Trims the history to num_dup_acks_+1 length.
 * arg: seq_start, seq_end - the lost packet is in [start,end]
 * ret: true if a packet loss was detected, otherwise false.
 */
bool DCCPTCPlikeAgent::detectLossSendRecv(u_int32_t *seq_start, u_int32_t *seq_end){
	struct dccp_tcplike_send_recv_hist_entry *before = STAILQ_FIRST(&send_recv_hist_);
	int num_later = 1;
	bool result = false;
	//find the packet before the num_dup_acks_ limit
	while (before != NULL && num_later < num_dup_acks_){
		num_later++;
		before = STAILQ_NEXT(before, linfo_);
	}
	
	struct dccp_tcplike_send_recv_hist_entry *after = NULL;
	//find the packet after the limit
	if (before != NULL)
		after = STAILQ_NEXT(before, linfo_);
	
	if (before == NULL || after == NULL)
		return false;
	
	if (before->seq_num_ - after->seq_num_ != 1){
		//we have loss, check if it includes ack packet by comparing ndp values
		result =  (before->type_ == DCCP_ACK ? ((before->ndp_ - 1 + ndp_limit_) % ndp_limit_ != after->ndp_) : ( before->ndp_ != after->ndp_));
		
		if (result){  //we found a loss of an ack packet
			*seq_start = before->seq_num_ - 1;
			*seq_end = after->seq_num_ + 1;
		}
	}

	//trim history
	if (after != NULL){
		before = after;
		after = STAILQ_NEXT(before, linfo_);
		while(after != NULL){
			STAILQ_REMOVE(&send_recv_hist_,after,dccp_tcplike_send_recv_hist_entry,linfo_);
			delete after;
			after = STAILQ_NEXT(before, linfo_);	
		}
	}
	return result;
}

/* Check the constraints on the ack ratio (recv_ack_ratio_)
 * ret: true if the ack ratio was changed due to constraints
 *      false otherwise
 */
bool DCCPTCPlikeAgent::checkAckRatio(){
	bool result = false;
	int temp = cwnd_ / 2;
	if (cwnd_ % 2 > 0)
		temp++;

	//ackratio is never greater than half cwnd (rounded up)
	if(recv_ack_ratio_ > temp){  
		recv_ack_ratio_ = temp;
		result = true;
	}
	//ack ratio always >= 2 for cwnd >= 4
	if (cwnd_ >= 4 && recv_ack_ratio_ < 2){
		recv_ack_ratio_ = 2;
		result = true;
	} else if (recv_ack_ratio_ == 0){  //always an integer > 0
		recv_ack_ratio_ = 1;
		result = true;
	}

	return result;
}

/* Compares recv_ack_ratio_ with ack_ratio_remote_ and
 * tries to change the feauture if they differ.
 */
void DCCPTCPlikeAgent::changeRemoteAckRatio(){
	if (recv_ack_ratio_ != ack_ratio_remote_ && changeFeature(DCCP_FEAT_ACK_RATIO,DCCP_FEAT_LOC_REMOTE)){
		debug("%f, DCCP/TCPlike(%s)::changeRemoteAckRatio() - Changed Remote ack ratio: before %d, after %d\n",now(),name(),ack_ratio_remote_,recv_ack_ratio_);
		ack_ratio_remote_ = recv_ack_ratio_;
	}
}

/* Update the ack ratio
 * arg: num_ack - number of newly acknowledged packets
 */
void DCCPTCPlikeAgent::updateAckRatio(int num_ack){
	debug("%f, DCCP/TCPlike(%s)::updateAckRatio() - Before: num_ack %d, in_win %d, wins %d, cwnd %d, ack_ratio %d\n", now(), name(), num_ack, num_ack_in_win_, num_win_acked_, (int) cwnd_, recv_ack_ratio_);
	if (recv_ack_ratio_ == 1){ //if ack ration is 1, we can't decrease it more
		num_ack_in_win_ = 0;
		num_win_acked_ = 0;
	} else {
		num_ack_in_win_ += num_ack;
		//only check limit when a whole window has been acked
		if (num_ack > 0 && num_ack_in_win_ >= cwnd_){
			num_win_acked_++;
			num_ack_in_win_ -= cwnd_;
			
			double ack_ratio_dec = ((double) cwnd_)/((double) (recv_ack_ratio_*(recv_ack_ratio_ - 1)));

			if (num_win_acked_ > ack_ratio_dec){
				debug("%f, DCCP/TCPlike(%s)::updateAckRatio() - Ack ratio decreased since num_win_acked_ > ack_ratio_dec\n", now(), name());
				recv_ack_ratio_--;
				checkAckRatio();
				num_win_acked_ = 0;
			}
		}
	}
	debug("%f, DCCP/TCPlike(%s)::updateAckRatio() - After: num_ack %d, in_win %d, wins %d, cwnd %d, ack_ratio %d\n", now(), name(), num_ack, num_ack_in_win_, num_win_acked_, (int) cwnd_, recv_ack_ratio_);
}

/* Take action on lost or marked ack packet */
void DCCPTCPlikeAgent::lostOrMarkedAck(){
	ack_win_start_ = seq_num_;
	num_win_acked_ = 0;
	num_ack_in_win_ = 0;
	skip_ack_loss_ = true;
	recv_ack_ratio_ *= 2;
	checkAckRatio();
}

/* Update the congestion window
 * arg: num_inc_cwnd - number of newly acknowledged packets
 */
void DCCPTCPlikeAgent::updateCwnd(int num_inc_cwnd){
	debug("%f, DCCP/TCPlike(%s)::updateCwnd() - Before: num_inc_cwnd %d,  cwnd_ %d, cwnd_frac_ %d, ssthresh_ %d, ack_ratio_remote_ %d\n", now(),name(), num_inc_cwnd, (int) cwnd_, (int) cwnd_frac_,(int) ssthresh_, ack_ratio_remote_);
	int old_cwnd = cwnd_;

	if (cwnd_ < ssthresh_){ //slow start
		if (num_inc_cwnd > ack_ratio_remote_){
			cwnd_ += ack_ratio_remote_;
			num_inc_cwnd -= ack_ratio_remote_;
		} else {
			cwnd_ += num_inc_cwnd;
			num_inc_cwnd = 0;
		}
		
		if (cwnd_ <  ssthresh_)
			num_inc_cwnd = 0;
		else {
			num_inc_cwnd += (cwnd_ - ssthresh_);
			cwnd_ = ssthresh_;
		}
		if (cwnd_ == ssthresh_){ //we passed ssthresh_
			cwnd_frac_ = 0;
			//check quiescence and send ack of ack if needed
			if (detectQuiescence()){ 
				//output_ = true;
				send_ack_ = true;
				ack_num_ = seq_num_recv_;
			}
		}
	}
	
	if (num_inc_cwnd > 0 && cwnd_ >= ssthresh_){ //congestion avoidance
		cwnd_frac_ += num_inc_cwnd;
		while (cwnd_frac_ >= cwnd_){
			cwnd_frac_ -= cwnd_;
			cwnd_++;
		}

		//check quiescence and send ack of ack if needed
		if (cwnd_ != old_cwnd && detectQuiescence()){
			//output_ = true;
			send_ack_ = true;
			ack_num_ = seq_num_recv_;
		} 
	}

	if (old_cwnd < cwnd_)
		if (checkAckRatio())
			debug("%f, DCCP/TCPlike(%s)::updateCwnd() - Ack ratio changed! old_cwnd %d cwnd_ %d\n", now(), name(), old_cwnd, (int) cwnd_);
	
	debug("%f, DCCP/TCPlike(%s)::updateCwnd() - After: num_inc_cwnd %d, cwnd_ %d, cwnd_frac_ %d, ssthresh_ %d, ack_ratio_remote_ %d\n", now(), name(), num_inc_cwnd, (int) cwnd_, (int) cwnd_frac_, (int) ssthresh_, ack_ratio_remote_);
}

/* Take action on lost or marked data packet */
void DCCPTCPlikeAgent::lostOrMarkedData(){
	if (cwnd_ >= 2)  //i.e . cwnd_ != 1
		cwnd_ = cwnd_ / 2;
	cwnd_frac_ = 0;
	ssthresh_ = cwnd_;
	seq_win_start_ = seq_num_;
	debug("%f, DCCP/TCPlike(%s)::lostOrMarkedData() - updated cwnd and ssthresh to: cwnd_ %d, ssthresh_ %d, seq_win_start %d\n", now(), name(), (int) cwnd_, (int) ssthresh_, seq_win_start_);
}

/* Detects quiescence of the corresponding sender
 * ret: true if sender is quiescent
 *      false otherwise
 */
bool DCCPTCPlikeAgent::detectQuiescence(){
	debug("%f, DCCP/TCPlike(%s)::detectQuiescence() - scheme %d, seq high data %d, last in vect %d, time now %f, time high data %f, srtt %f\n", now(), name(), q_scheme_, q_high_data_recv_, ackv_->getLastSeqNum(),now(),q_t_data_,(double) srtt_);

	if (q_scheme_ == DCCP_Q_SCHEME_Q_OPT){
		if(sender_quiescent_)
			debug("%f, DCCP/TCPlike(%s)::detectQuiescence() - Receiver detected that the corresponding sender is quiescent (Q_OPT)\n", now(),name());
		return sender_quiescent_;
	} else if (q_scheme_ == DCCP_Q_SCHEME_Q_FEAT){
		if (q_local_)
			debug("%f, DCCP/TCPlike(%s)::detectQuiescence() - Receiver detected that the corresponding sender is quiescent (Q_FEAT)\n", now(),name());
		return q_local_;
	}
	
	if (srtt_ <= 0.0)
		return false;
	
	double t = q_min_t_;
	if (t < 2*srtt_)  
		t = 2*srtt_; 
	if (ackv_->getLastSeqNum() > q_high_data_recv_ && now()-q_t_data_ >= t)
		debug("%f, DCCP/TCPlike(%s)::detectQuiescence() - Receiver detected that the corresponding sender is quiescent (NORMAL)\n", now(), name());
	return (ackv_->getLastSeqNum() > q_high_data_recv_ && now()-q_t_data_ >= t);
}

//protected methods

/* Methods inherited from DCCPAgent.
 * See dccp.h For detailed information regarding arguments etc.
 */

void DCCPTCPlikeAgent::delay_bind_init_all(){
	delay_bind_init_one("initial_cwnd_");
	delay_bind_init_one("cwnd_timeout_");
	delay_bind_init_one("initial_ssthresh_");
	delay_bind_init_one("cwnd_");
	delay_bind_init_one("cwnd_frac_");
	delay_bind_init_one("ssthresh_");
	delay_bind_init_one("pipe_");
	delay_bind_init_one("initial_rto_");
	delay_bind_init_one("min_rto_");
	delay_bind_init_one("rto_");
	delay_bind_init_one("srtt_");
	delay_bind_init_one("rttvar_");
	delay_bind_init_one("rtt_sample_");
	delay_bind_init_one("alpha_");
	delay_bind_init_one("beta_");
	delay_bind_init_one("k_");
	delay_bind_init_one("g_");
	delay_bind_init_one("num_dup_acks_");
	delay_bind_init_one("q_min_t_");
	delay_bind_init_one("dack_delay_");
	delay_bind_init_one("q_opt_ratio_");
	delay_bind_init_one("ackv_size_lim_");
	DCCPAgent::delay_bind_init_all();
}

int DCCPTCPlikeAgent::delay_bind_dispatch(const char *varName, const char *localName, TclObject *tracer){
	if (delay_bind(varName, localName, "initial_cwnd_", &initial_cwnd_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "cwnd_timeout_", &cwnd_timeout_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "initial_ssthresh_", &initial_ssthresh_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "cwnd_", &cwnd_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "cwnd_frac_", &cwnd_frac_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "ssthresh_", &ssthresh_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "pipe_", &pipe_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "initial_rto_", &initial_rto_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "min_rto_", &min_rto_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "rto_", &rto_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "srtt_", &srtt_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "rttvar_", &rttvar_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "rtt_sample_", &rtt_sample_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "alpha_", &alpha_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "beta_", &beta_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "k_", &k_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "g_", &g_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "num_dup_acks_", &num_dup_acks_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "q_min_t_", &q_min_t_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "dack_delay_", &dack_delay_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "q_opt_ratio_", &q_opt_ratio_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "ackv_size_lim_", &ackv_size_lim_, tracer)) return TCL_OK;
	return DCCPAgent::delay_bind_dispatch(varName, localName, tracer);
}


void DCCPTCPlikeAgent::reset(){
	DCCPAgent::reset();
	
	cwnd_ = initial_cwnd_;
	cwnd_frac_ = 0;
	ssthresh_ = initial_ssthresh_;
	pipe_ = 0;
	clearSendHistory();
	clearSendRecvHistory();

	t_last_data_sent_ = (double) (-q_min_t_)*2;
	q_packets_wo_opt_ = 0;
	
	rto_ = initial_rto_;
	srtt_ = -1.0;
	rttvar_ = 0.0;
	rtt_sample_ = 0.0;

	hist_last_seq_ = 0;
	high_ack_recv_ = 0;
	
	high_seq_recv_ = 0;
	high_ndp_recv_ = -1;
	unacked_ = 0;

	delete stored_ackv_;
	stored_ackv_ = new DCCPAckVector(ackv_size_);
	seq_all_done_ = 0;
	seq_win_start_ = 0;
	seq_pipe_start_ = 0;
	num_ack_in_win_ = 0;
	num_win_acked_ = 0;
	ack_win_start_ = 0;
	skip_ack_loss_ = false;
	recv_ack_ratio_ = ack_ratio_remote_;

	q_high_data_recv_ = 0;
	q_t_data_ = 0.0;
	ackv_lim_seq_ = 0;

	sender_quiescent_ = false;
	t_high_recv_ = 0.0;
}

void DCCPTCPlikeAgent::cancelTimers(){
	timer_to_->force_cancel();
	timer_dack_->force_cancel();
	DCCPAgent::cancelTimers();
}

/* Process incoming options.
 * This function will process:
 *      ack vectors (verify ECN Nonce Echo)
 *      quiescence option
 */
bool DCCPTCPlikeAgent::processOption(u_int8_t type, u_char* data, u_int8_t size, Packet *pkt){
	bool result = DCCPAgent::processOption(type, data, size, pkt);
	if (result){
		switch(type){
		case DCCP_OPT_ACK_VECTOR_N0:
		case DCCP_OPT_ACK_VECTOR_N1:
			//check if we received a new ack vector
			if (ackv_recv_ != NULL && ackv_recv_->getFirstSeqNum() >= high_ack_recv_){
				//check if we got all needed packets in the history
				if(hist_last_seq_ <= ackv_recv_->getLastSeqNum()){
					
					u_int8_t ene = 0;

					u_int32_t seqnum;
					dccp_packet_state state;
					
					struct dccp_tcplike_send_hist_entry *elm = STAILQ_FIRST(&send_hist_);
					//walk through the ack vector and compute the ECN Nonce Echo
					ackv_recv_->startPacketWalk();
					while(ackv_recv_->nextPacket(&seqnum, &state)){
						if (state == DCCP_PACKET_RECV){
							elm = findPacketInSendHistory(seqnum,elm);
							if (elm != NULL && elm->seq_num_ == seqnum){
								ene = ene ^ elm->ecn_;
							} else if (elm == NULL)
								break;
						}
					}
					//compare with the received echo
					if (ene != ackv_recv_->getENE()){
						fprintf(stdout,"%f, DCCP/TCPlike(%s)::processOption() - ECN check failed! \n", now(), name());
						sendReset(DCCP_RST_AGG_PEN,0,0,0);
						result = false;
					}
				} else
					debug("%f, DCCP/TCPlike(%s)::processOption() - Ack vector includes packets not in history. Skipping ecn check for now...\n", now(), name());
			} else if (ackv_recv_ != NULL)
				debug("%f, DCCP/TCPlike(%s)::processOption() - Old ack detected. Skipping ecn check\n", now(), name());
			break;
		case DCCP_OPT_QUIESCENCE:
			sender_quiescent_ = true;
			break;
		}
	}
	return result;
}


bool DCCPTCPlikeAgent::send_askPermToSend(int dataSize, Packet *pkt){
	ack_num_ = seq_num_recv_;
	bool result = ( pipe_ < cwnd_ || dataSize == 0);
	u_int32_t el_time32 = (u_int32_t) ((now() - t_high_recv_)/DCCP_OPT_ELAPSED_TIME_UNIT);
	double t;
	if (q_scheme_ == DCCP_Q_SCHEME_Q_OPT && !infinite_send_ && dataSize == 0){
		t = q_min_t_;
		if (t < 2*srtt_)  
			t = 2*srtt_;
		if (now() - t_last_data_sent_ > t && sb_->empty()){
			q_packets_wo_opt_++;
			if (q_packets_wo_opt_ >= q_opt_ratio_){
				opt_->addOption(DCCP_OPT_QUIESCENCE,NULL,0);
				q_packets_wo_opt_ = 0;
			}
		}

	} else if (q_scheme_ == DCCP_Q_SCHEME_Q_FEAT && !infinite_send_){
		if(dataSize > 0 && q_remote_){
			if (changeFeature(DCCP_FEAT_Q,DCCP_FEAT_LOC_REMOTE))
				q_remote_ = 0;
		} else if (dataSize == 0) {
			t = q_min_t_;
			if (t < 2*srtt_)  
				t = 2*srtt_;
			if (now() - t_last_data_sent_ > t && sb_->empty() && !q_remote_){
				if (changeFeature(DCCP_FEAT_Q,DCCP_FEAT_LOC_REMOTE))
					q_remote_ = 1;
					
			}
		}
		
	}
				
	//add elapsed time on acknowledgments if needed
	if(result && send_ack_ && el_time32 > 0){
		debug("%f, DCCP/TCPlike(%s)::send_askPermToSend() - Ack is delayed by %d (t_high_recv_ %f)\n", now(), name(),el_time32,t_high_recv_);
		if (el_time32 > 0 && el_time32 <= 0xFFFF){
			u_int16_t el_time16 = (u_int16_t) el_time32;
			opt_->addOption(DCCP_OPT_ELAPSED_TIME,((u_char*) &el_time16), 2);
		} else if (el_time32 > 0)
			opt_->addOption(DCCP_OPT_ELAPSED_TIME,((u_char*) &el_time32), 4); 
	}
		
	return (result);
}

void DCCPTCPlikeAgent::send_packetSent(Packet *pkt, bool moreToSend, int dataSize){
	struct dccp_tcplike_send_hist_entry *new_packet;
	int result;
	hdr_dccp *dccph = hdr_dccp::access(pkt);

	switch(dccph->type_){
	case DCCP_DATA:
	case DCCP_DATAACK:
		if (q_scheme_ == DCCP_Q_SCHEME_Q_OPT)
			q_packets_wo_opt_ = 0;
		t_last_data_sent_ = now();
		//add data packets to send history
		new_packet = new struct dccp_tcplike_send_hist_entry;
		new_packet->t_sent_ = now();
		new_packet->seq_num_ = dccph->seq_num_;
		result = getNonce(pkt);
		new_packet->ecn_ = (result >= 0 ? result : 0);
		STAILQ_INSERT_HEAD(&(send_hist_), new_packet, linfo_);
		pipe_++;
		if (timer_to_->status() != TIMER_PENDING)
			timer_to_->resched(rto_);
		break;
	default:
		;
	}
}

void DCCPTCPlikeAgent::send_packetRecv(Packet *pkt, int dataSize){
	hdr_dccp *dccph = hdr_dccp::access(pkt);
	hdr_dccpack *dccpah = hdr_dccpack::access(pkt);
	bool lost_ack = false;
	struct dccp_tcplike_send_recv_hist_entry *packet;
	packet = new struct dccp_tcplike_send_recv_hist_entry;
	packet->type_ = dccph->type_;
	packet->seq_num_ = dccph->seq_num_;

	packet->ndp_ = dccph->ndp_;

	//insert in the history over received packets
	if(!insertInSendRecvHistory(packet)){
		debug("%f, DCCP/TCPlike(%s)::send_packetRecv() - Insertion in send recv history failed for seqnum %d\n", now(), name(), packet->seq_num_);
		delete packet;
	} else {
		u_int32_t seq_start;
		u_int32_t seq_end;
		//detect loss
		if(detectLossSendRecv(&seq_start, &seq_end)){
			debug("%f, DCCP/TCPlike(%s)::send_packetRecv() - Ack loss detected (send recv) in %d - %d\n" , now(), name(), seq_start, seq_end);
			lost_ack = true;
		}
	}

	switch(dccph->type_){
	case DCCP_DATA:
		if (!skip_ack_loss_ && lost_ack)
			lostOrMarkedAck();
		break;
	case DCCP_ACK:
	case DCCP_DATAACK:
		skip_ack_loss_ = (dccpah->ack_num_ < ack_win_start_);

		//check if ack is marked
		if(!lost_ack && getECNCodePoint(pkt) == ECN_CE){
			debug("%f, DCCP/TCPlike(%s)::send_packetRecv() - Ack packet is marked %d\n", now(), name(), dccph->seq_num_);
			lost_ack = true;
		}
		
		//check if we got an ack vector (if not, ignore packet)
		if (ackv_recv_ != NULL && ackv_recv_->getSize() > 0){
			
			struct dccp_tcplike_send_hist_entry *elm = findPacketInSendHistory(dccpah->ack_num_,STAILQ_FIRST(&send_hist_));
			if (elm != NULL && elm->seq_num_ == dccpah->ack_num_) {
				//the ack regards a data packet
				//update rtt
				rtt_sample_ = now() - elm->t_sent_ - elapsed_time_recv_*DCCP_OPT_ELAPSED_TIME_UNIT;
				
				if (srtt_ < 0){
					srtt_ = rtt_sample_;
					rttvar_ = rtt_sample_/2.0;
				} else {
					rttvar_ = (1 - beta_) * rttvar_
						+ beta_ * fabs(srtt_ - rtt_sample_);
					srtt_ = (1- alpha_) * srtt_ + alpha_*rtt_sample_;
				}

				rto_ = srtt_ + (k_*rttvar_ > g_ ? k_*rttvar_ : g_);
				if(rto_ < min_rto_)
					rto_ = min_rto_;
				
				debug("%f, DCCP/TCPlike(%s)::send_packetRecv() - RTT measurement updated: elapsed time %d sample %f, rto %f, srtt %f, rttvar %f\n", now(), name(), elapsed_time_recv_, (double) rtt_sample_, (double) rto_, (double) srtt_, (double) rttvar_);
			}
			
			if (dccpah->ack_num_ > high_ack_recv_)
				high_ack_recv_ = dccpah->ack_num_;
			else if (dccpah->ack_num_ == high_ack_recv_)
				debug("%f, DCCP/TCPlike(%s)::send_packetRecv() - Duplicate acknowledgment received ack_num_ %d\n", now(), name(), dccpah->ack_num_);
			else if (!skip_ack_loss_ && lost_ack){
				lostOrMarkedAck();
				break;
			} else
				break;

			if (dccpah->ack_num_ < high_ack_recv_){
				fprintf(stderr,"%f, DCCP/TCPlike(%s)::send_packetRecv() - ack_num %d is less than high_ack_recv_ %d!\n", now(), name(), dccpah->ack_num_, high_ack_recv_);
				fflush(stdout);
				abort();
			}

			//init the stored ack vector if it is empty
			if (stored_ackv_->getSize() == 0){
				stored_ackv_->addPacket(ackv_recv_->getLastSeqNum(), DCCP_PACKET_NOT_RECV);
				stored_ackv_->addPacket(ackv_recv_->getFirstSeqNum(), DCCP_PACKET_NOT_RECV);
				if (seq_all_done_ == 0) //first ack vector recv
					seq_all_done_ = ackv_recv_->getLastSeqNum()-1;
				else //stored ack vector is zero because of a timeout
					debug("%f, DCCP/TCPlike(%s)::send_packetRecv() - seq_all_done_ %d is not zero (stored ackv == 0)\n",now(), name(),seq_all_done_);
			}
			
			debug("%f, DCCP/TCPlike(%s)::send_packetRecv() - high_ack_recv_ %d, hist_last_seq_ %d, seq_all_done_ %d, seq_win_start_ %d\n",now(),name(),high_ack_recv_, hist_last_seq_, seq_all_done_, seq_win_start_);
			
			int num_lost[2];
			int num_marked[2];
			int num_ok[2];
			int num_ok_mark[2];
			int num_ack[2];
			int num_pipe[2];
			for (int i = 0; i < 2; i++){
				num_lost[i] = 0;
				num_marked[i] = 0;
				num_ok[i] = 0;
				num_ok_mark[i] = 0;
				num_ack[i] = 0;
				num_pipe[i] = 0;
			}
			
			int num_packets = 0;
			u_int32_t new_seq_all_done = 0;

			bool ackv_recv_empty = false;
			u_int32_t first_stored = stored_ackv_->getFirstSeqNum();
			u_int32_t seqnum_old;
			u_int32_t seqnum_new;
		        dccp_packet_state state_old;
			dccp_packet_state state_new;

			if (hist_last_seq_ > seq_all_done_){
				fprintf(stderr,"%f, DCCP/TCPlike(%s)::send_packetRecv() - Last packet in history %d is newer than seq_all_done %d\n", now(), name(), hist_last_seq_, seq_all_done_);
				fflush(stdout);
				abort();
			}
			
			//process the ack vector information
			
			ackv_recv_->startPacketWalk();
			stored_ackv_->startPacketWalk();
			
			if (!ackv_recv_->nextPacket(&seqnum_new, &state_new)){
				fprintf(stderr,"%f, DCCP/TCPlike(%s)::send_packetRecv() - ackv_recv_ is empty!\n", now(), name());
				fflush(stdout);
				abort();
			}

			if (seqnum_new > first_stored){
				state_old = DCCP_PACKET_NOT_RECV;
				seqnum_old = seqnum_new;
			} else if (!(stored_ackv_->nextPacket(&seqnum_old, &state_old)) ||
				   seqnum_old != seqnum_new){
				fprintf(stderr,"%f, DCCP/TCPlike(%s)::send_packetRecv() - Stored ackv is empty or seqnum_old != seqnum_new!\n", now(), name());
				fflush(stdout);
				abort();
			}
			
			do {

				if (seqnum_new <= seq_all_done_)
					break;

				if (new_seq_all_done == 0 && num_packets == num_dup_acks_)
					new_seq_all_done = seqnum_new;
				
				elm = findPacketInSendHistory(seqnum_new,elm);
				state_new = stored_ackv_->ackv_cons_[state_old][state_new];
				
				if ((elm != NULL) && (elm->seq_num_ == seqnum_new)){ //if data packet
										
					if (state_new != state_old){
						if (state_old == DCCP_PACKET_NOT_RECV &&
						    state_new == DCCP_PACKET_RECV){
							num_ok[(int) (seqnum_new >= seq_win_start_)]++;
							num_ack[(int) (seqnum_new >= ack_win_start_)]++;
							num_pipe[(int) (seqnum_new >= seq_pipe_start_)]++;
						} else if (state_old == DCCP_PACKET_NOT_RECV &&
							   state_new == DCCP_PACKET_ECN){
							num_marked[(int) (seqnum_new >= seq_win_start_)]++;
							num_ack[(int) (seqnum_new >= ack_win_start_)]++;
							num_pipe[(int) (seqnum_new >= seq_pipe_start_)]++;
						} else if (state_old == DCCP_PACKET_RECV &&
							   state_new == DCCP_PACKET_ECN) {
							num_marked[(int) (seqnum_new >= seq_win_start_)]++;
							num_ok_mark[(int) (seqnum_new >= seq_win_start_)]++;
						}
						
					} else if (state_new == DCCP_PACKET_NOT_RECV){
						if (num_packets >= num_dup_acks_){
							num_lost[(int) (seqnum_new >= seq_win_start_)]++;
							num_ack[(int) (seqnum_new >= ack_win_start_)]++;
							num_pipe[(int) (seqnum_new >= seq_pipe_start_)]++;
						}
					}
				}

				if (state_new != DCCP_PACKET_NOT_RECV)
					num_packets++;
				
				if (!ackv_recv_->nextPacket(&seqnum_new, &state_new)){
					seqnum_new--;
					ackv_recv_empty = true;
				}

				if (seqnum_new == seq_all_done_)
					break;
				else if (seqnum_new > first_stored){
					
					state_old = DCCP_PACKET_NOT_RECV;
					seqnum_old = seqnum_new;
				} else if (!(stored_ackv_->nextPacket(&seqnum_old, &state_old))){
					fprintf(stderr,"%f, DCCP/TCPlike(%s)::send_packetRecv() - Stored ack vector is empty when it shouldn't be!\n", now(), name());
					fflush(stdout);
					abort();
				}
				
				if (ackv_recv_empty){
					state_new = state_old;
				}				

			} while (true);
		       
			debug("%f, DCCP/TCPlike(%s)::send_packetRecv() - in window: ok %d, marked %d, lost %d, ok_mark %d\n",now(), name(), num_ok[1],num_marked[1],num_lost[1],num_ok_mark[1]);
			debug("%f, DCCP/TCPlike(%s)::send_packetRecv() - after window: ok %d, marked %d, lost %d, ok_mark %d\n",now(), name(), num_ok[0],num_marked[0],num_lost[0],num_ok_mark[0]);
			debug("%f, DCCP/TCPlike(%s)::send_packetRecv() - new_seq_all_done %d, seq_all_done_ %d\n",now(), name(), new_seq_all_done,seq_all_done_);
			debug("%f, DCCP/TCPlike(%s)::send_packetRecv() - (ack) in window: %d, not in window %d\n",now(), name(), num_ack[1],num_ack[0]);

			if (new_seq_all_done > seq_all_done_)
				seq_all_done_ = new_seq_all_done;

			stored_ackv_->mergeWith(ackv_recv_);
			stored_ackv_->removePackets(seq_all_done_);

			
			//trim history to seq = min(last in recv ackvector, seq_all_done);
			u_int32_t trim_to = ackv_recv_->getLastSeqNum();
			if (seq_all_done_ < trim_to)
			       trim_to = seq_all_done_;

			trimSendHistory(trim_to);
			
			int num_inc_cwnd =
				num_ok[DCCP_TCPLIKE_IN_WINDOW] + num_marked[DCCP_TCPLIKE_IN_WINDOW] -
				num_ok_mark[DCCP_TCPLIKE_IN_WINDOW] +
				num_ok[DCCP_TCPLIKE_AFTER_WINDOW] + num_marked[DCCP_TCPLIKE_AFTER_WINDOW] -
				num_ok_mark[DCCP_TCPLIKE_AFTER_WINDOW];
			

			debug("%f, DCCP/TCPlike(%s)::send_packetRecv() - Update pipe (before): pipe_ %d\n", now(), name(), (int) pipe_);
			pipe_ = pipe_ - num_pipe[DCCP_TCPLIKE_IN_WINDOW];
			
			debug("%f, DCCP/TCPlike(%s)::send_packetRecv() - Update pipe (after): pipe_ %d\n", now(), name(), (int) pipe_);

			if (pipe_ < 0) {
				fprintf(stderr,"%f, DCCP/TCPlike(%s)::send_packetRecv() - Pipe is less than zero! (%i)\n", now(), name(), (int) pipe_);
				fflush(stdout);
				abort();
			} else if (pipe_ == 0) {
				timer_to_->force_cancel();
				debug("%f, DCCP/TCPlike(%s)::send_packetRecv() - Pipe is zero, cancels timer\n", now(), name());
			} else if (num_inc_cwnd > 0){
				debug("%f, DCCP/TCPlike(%s)::send_packetRecv() - Newly acked packets, rescheduling timer. rto %f\n", now(), name(),(double) rto_);
				timer_to_->resched(rto_);
			}

			updateCwnd(num_inc_cwnd);
			
			if (num_lost[DCCP_TCPLIKE_IN_WINDOW] > 0 ||
			    num_marked[DCCP_TCPLIKE_IN_WINDOW] > 0){
				lostOrMarkedData();
				num_ack_in_win_ = 0;				
				if (checkAckRatio()){
					debug("%f, DCCP/TCPlike(%s)::send_packetRecv() - Ackratio changed because of data loss\n", now(), name());
					num_win_acked_ = 0;
					ack_win_start_ =  seq_num_;
					num_ack[DCCP_TCPLIKE_IN_WINDOW] = 0;
				}
				
				if (detectQuiescence()){
					//output_ = true;
					send_ack_ = true;
					ack_num_ = seq_num_recv_;
				}
			}

			if (!skip_ack_loss_ && lost_ack){
				lostOrMarkedAck();
			} else {
				updateAckRatio(num_ack[DCCP_TCPLIKE_IN_WINDOW]);
			} 

			if (pipe_ < cwnd_)
				output_ = true;

			//check if ackv_size_lim_ is reached
			if (ackv_size_lim_ > 0
			    && ackv_recv_->getSize() > ackv_size_lim_ 
			    && ackv_lim_seq_ < dccpah->ack_num_
			    && detectQuiescence()){
			       debug("%f, DCCP/TCPlike(%s)::send_packet_recv() - Ack vector size limit reached during quiescence. Sending ack of ack. (lim %u, size %u, lim_seq %u, seq_num_ %u\n", now(),name(),ackv_size_lim_,ackv_recv_->getSize(),ackv_lim_seq_,seq_num_);
				send_ack_ = true;
				ack_num_ = seq_num_recv_;
				ackv_lim_seq_ = seq_num_;
			}
			
			
		} else {
			fprintf(stdout,"%f, DCCP/TCPlike(%s)::send_packetRecv() - No ack vector on acknowledgment!\n", now(), name());
			if (lost_ack)
				lostOrMarkedAck();
		}
		break;
	default:
		;
	}

	changeRemoteAckRatio();
	
}

void DCCPTCPlikeAgent::recv_packetRecv(Packet *pkt, int dataSize){
	hdr_dccp *dccph = hdr_dccp::access(pkt);

	if (q_t_data_ == 0.0)
		q_t_data_ = now();
	
	if(dccph->type_ == DCCP_DATA || dccph->type_ == DCCP_DATAACK){
		//start dack timer if idle
		if (timer_dack_->status() == TIMER_IDLE){
			//printf("TIMER is IDLE\n");
			timer_dack_->sched(dack_delay_);
		}
		unacked_++;
		q_t_data_ = now();
		if (dccph->seq_num_ > q_high_data_recv_)
			q_high_data_recv_ = dccph->seq_num_;
		sender_quiescent_ = false;
	} else if (high_ndp_recv_ >= 0 && (int) (dccph->seq_num_ - high_seq_recv_) > 1){
		//some packet is missing
		if ( (int) (dccph->seq_num_ - high_seq_recv_) >
		     ((dccph->ndp_ - high_ndp_recv_ + ndp_limit_) % ndp_limit_)
		     ){ //data packet is "lost"
			debug("%f, DCCP/TCPlike(%s)::recv_packetRecv() - Data loss detected by receiver seq_num %d - ndp %d,  high_seq_recv %d - ndp %d\n",now(),name(), dccph->seq_num_, dccph->ndp_,high_seq_recv_, high_ndp_recv_);
			q_t_data_ = now();
			if (dccph->seq_num_-1 > q_high_data_recv_)
				q_high_data_recv_ = dccph->seq_num_-1;
			sender_quiescent_ = false;
		}
	}

	if (dccph->seq_num_ > high_seq_recv_){
		high_seq_recv_ = dccph->seq_num_;
		high_ndp_recv_ = dccph->ndp_;
		t_high_recv_ = now();
	}
	
	if (unacked_ >= ack_ratio_local_){
		timer_dack_->force_cancel();
		send_ack_ = true;
		ack_num_ = seq_num_recv_;
		unacked_= 0;
		output_ = true;
		output_flag_ = true;
	}

}

/* Variable tracing */

void DCCPTCPlikeAgent::traceAll() {
	char wrk[500];

	sprintf(wrk,"%f %d %d %d %d %f %f %f %f\n",
		now(), (int) cwnd_, (int) cwnd_frac_, (int) ssthresh_,
		(int) pipe_, (double) rto_, (double) srtt_, (double) rttvar_,
		(double) rtt_sample_);

	if (channel_)
		Tcl_Write(channel_, wrk, strlen(wrk));
}


void DCCPTCPlikeAgent::traceVar(TracedVar* v) 
{
	char wrk[500];

	if (!strcmp(v->name(), "cwnd_") ||
	    !strcmp(v->name(), "cwnd_frac_") ||
	    !strcmp(v->name(), "ssthresh_") ||
	    !strcmp(v->name(), "pipe_")){
		sprintf(wrk,"%f %d %s\n", now(), int(*((TracedInt*) v)), v->name());
		if (channel_)
			Tcl_Write(channel_, wrk, strlen(wrk));
	} else if (!strcmp(v->name(), "rto_") ||
		   !strcmp(v->name(), "srtt_") ||
		   !strcmp(v->name(), "rttvar_") ||
		   !strcmp(v->name(), "rtt_sample_")){
		sprintf(wrk,"%f %f %s\n", now(), double(*((TracedDouble*) v)), v->name());
		if (channel_)
			Tcl_Write(channel_, wrk, strlen(wrk));
	} else
		DCCPAgent::traceVar(v);	
}

//public methods

/* Constructor
 * ret: a new DCCPTCPlikeAgent
 */
DCCPTCPlikeAgent::DCCPTCPlikeAgent() : DCCPAgent() {
	ccid_ = DCCP_TCPLIKE_CCID;

	initial_cwnd_ = DCCP_TCPLIKE_INIT_CWND;
	initial_ssthresh_ = DCCP_TCPLIKE_INIT_SSTHRESH;
	initial_rto_ = DCCP_TCPLIKE_INIT_RTO;
	cwnd_timeout_ = DCCP_TCPLIKE_CWND_TO;

	cwnd_ = initial_cwnd_;
	cwnd_frac_ = 0;
	ssthresh_ = initial_ssthresh_;
	pipe_ = 0;

	t_last_data_sent_ = (double) (-DCCP_TCPLIKE_MIN_T)*2;
	q_opt_ratio_ = DCCP_TCPLIKE_QUIESCENCE_OPT_RATIO;
	q_packets_wo_opt_ = 0;
	
	rto_ = initial_rto_;
	srtt_ = -1.0;
	rttvar_ = 0.0;
	rtt_sample_ = 0.0;
	k_ = DCCP_TCPLIKE_K;
	g_ = DCCP_TCPLIKE_G;
	alpha_ = DCCP_TCPLIKE_ALPHA;
	beta_ = DCCP_TCPLIKE_BETA;

	min_rto_ = DCCP_TCPLIKE_MIN_RTO;
	
	num_dup_acks_ = DCCP_NUM_DUP_ACKS;

	STAILQ_INIT(&send_hist_);
	STAILQ_INIT(&send_recv_hist_);
	
	timer_to_ = new DCCPTCPlikeTOTimer(this);
	timer_dack_ = new DCCPTCPlikeDelayedAckTimer(this);
	
	hist_last_seq_ = 0;
	high_ack_recv_ = 0;
	
	high_seq_recv_ = 0;
	high_ndp_recv_ = -1;
	unacked_ = 0;

	stored_ackv_ = new DCCPAckVector(ackv_size_);
	stored_ackv_->ackv_cons_[DCCP_PACKET_RECV][DCCP_PACKET_RECV]         = DCCP_PACKET_RECV;
	stored_ackv_->ackv_cons_[DCCP_PACKET_RECV][DCCP_PACKET_ECN]          = DCCP_PACKET_ECN;
	stored_ackv_->ackv_cons_[DCCP_PACKET_RECV][DCCP_PACKET_RESERVED]     = DCCP_PACKET_RESERVED;
	stored_ackv_->ackv_cons_[DCCP_PACKET_RECV][DCCP_PACKET_NOT_RECV]     = DCCP_PACKET_RECV;
	stored_ackv_->ackv_cons_[DCCP_PACKET_ECN][DCCP_PACKET_RECV]          = DCCP_PACKET_ECN;
	stored_ackv_->ackv_cons_[DCCP_PACKET_ECN][DCCP_PACKET_ECN]           = DCCP_PACKET_ECN;
	stored_ackv_->ackv_cons_[DCCP_PACKET_ECN][DCCP_PACKET_RESERVED]      = DCCP_PACKET_RESERVED;
	stored_ackv_->ackv_cons_[DCCP_PACKET_ECN][DCCP_PACKET_NOT_RECV]      = DCCP_PACKET_ECN;
	stored_ackv_->ackv_cons_[DCCP_PACKET_RESERVED][DCCP_PACKET_RECV]     = DCCP_PACKET_RESERVED;
	stored_ackv_->ackv_cons_[DCCP_PACKET_RESERVED][DCCP_PACKET_ECN]      = DCCP_PACKET_RESERVED;
	stored_ackv_->ackv_cons_[DCCP_PACKET_RESERVED][DCCP_PACKET_RESERVED] = DCCP_PACKET_RESERVED;
	stored_ackv_->ackv_cons_[DCCP_PACKET_RESERVED][DCCP_PACKET_NOT_RECV] = DCCP_PACKET_RESERVED;
	stored_ackv_->ackv_cons_[DCCP_PACKET_NOT_RECV][DCCP_PACKET_RECV]     = DCCP_PACKET_RECV;
	stored_ackv_->ackv_cons_[DCCP_PACKET_NOT_RECV][DCCP_PACKET_ECN]      = DCCP_PACKET_ECN;
	stored_ackv_->ackv_cons_[DCCP_PACKET_NOT_RECV][DCCP_PACKET_RESERVED] = DCCP_PACKET_RESERVED;
	stored_ackv_->ackv_cons_[DCCP_PACKET_NOT_RECV][DCCP_PACKET_NOT_RECV] = DCCP_PACKET_NOT_RECV;
	seq_all_done_ = 0;
	seq_win_start_ = 0;
	seq_pipe_start_ = 0;
	
	num_win_acked_ = 0;
	num_ack_in_win_ = 0;
	ack_win_start_ = 0;
	skip_ack_loss_ = false;
	recv_ack_ratio_ = ack_ratio_remote_;

	q_high_data_recv_ = 0;
	q_t_data_ = 0.0;
	q_min_t_ = DCCP_TCPLIKE_MIN_T;

	ackv_size_lim_ = DCCP_TCPLIKE_ACKV_SIZE_LIM; 
	ackv_lim_seq_ = 0; 

	sender_quiescent_ = false;
	
	dack_delay_ = DCCP_TCPLIKE_DACK_DELAY;
	t_high_recv_ = 0.0;

	//test conversion from bool to int
	if (!((int) (cwnd_ >= cwnd_frac_) == 0 || (int) (cwnd_ >= cwnd_frac_) == 1)){
		fprintf(stderr,"DCCP/TCPlike::DCCPTCPlikeAgent() - The conversion between bool and int is not as expected ((int) (%d >= %d) = %d)!\n", (int) cwnd_, (int) cwnd_frac_, (int) (cwnd_ >= cwnd_frac_));
		fflush(stdout);
		abort();
	} else if (!((int) (cwnd_ < cwnd_frac_) == 0 || (int) (cwnd_ < cwnd_frac_) == 1)){
		fprintf(stderr,"DCCP/TCPlike::DCCPTCPlikeAgent() - The conversion between bool and int is not as expected ((int) (%d < %d) = %d)!\n", (int) cwnd_, (int) cwnd_frac_, (int) (cwnd_ < cwnd_frac_));
		fflush(stdout);
		abort();
	}
		
}

/* Destructor */
DCCPTCPlikeAgent::~DCCPTCPlikeAgent(){
	clearSendHistory();
	clearSendRecvHistory();
	delete stored_ackv_;
	delete timer_to_;
	delete timer_dack_;
}

int DCCPTCPlikeAgent::command(int argc, const char*const* argv){
	return DCCPAgent::command(argc,argv);
}

/* A timeout has occured
 * arg: tno - id of timeout event
 * Handles: Delayed acknowledgement timer - DCCP_TCPLIKE_TIMER_DACK
 *          Timeout timer - DCCP_TCPLIKE_TIMER_TO
 */
void DCCPTCPlikeAgent::timeout(int tno){
	switch (tno){
	case DCCP_TCPLIKE_TIMER_TO:
		debug("%f, DCCP/TCPlike(%s)::timeout() - Timeout\n", now(), name());
		ssthresh_ = cwnd_ / 2;
		if (ssthresh_ == 0)
			ssthresh_ = 1;
		
		cwnd_ = cwnd_timeout_;
		pipe_ = 0;
		
		seq_pipe_start_ = seq_num_;
		
		seq_win_start_ = seq_num_;
		cwnd_frac_ = 0;

		num_ack_in_win_ = 0;
		num_win_acked_ = 0;
		ack_win_start_ = seq_num_;
		recv_ack_ratio_ = 1;
		checkAckRatio();
		changeRemoteAckRatio();
		rto_ = rto_ * 2;
		output();
		break;
	case DCCP_TCPLIKE_TIMER_DACK:
		debug("%f, DCCP/TCPlike(%s)::timeout() - Delayed ack timer expired\n", now(), name());
		send_ack_ = true;
		ack_num_ = seq_num_recv_;
		unacked_ = 0;
		output(true);
		break;
	default:
	        DCCPAgent::timeout(tno);
	}
}

