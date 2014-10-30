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
 * $Id: dccp_tfrc.cc,v 1.1 2010/04/06 05:09:01 tom_henderson Exp $ */

#include "ip.h"
#include "dccp_tfrc.h"
#include "flags.h"


//OTcl linkage for DCCPTFRC agent
static class DCCPTFRCClass : public TclClass {
public:
	DCCPTFRCClass() : TclClass("Agent/DCCP/TFRC") {};
	TclObject* create(int argc, const char*const* argv){
		return (new DCCPTFRCAgent());
	}
} class_dccptfrc;

//methods for timer classes

/* Constructor
 * arg: agent - the owning agent (to notify about timeout)
 * ret: A new DCCPTFRCSendTimer
 */
DCCPTFRCSendTimer::DCCPTFRCSendTimer(DCCPTFRCAgent* agent) : TimerHandler(){
	agent_ = agent;
}

/* Called when the timer has expired
 * arg: e - The event that happened (i.e. the timer expired)
 */
void DCCPTFRCSendTimer::expire(Event *e){
	agent_->timeout(DCCP_TFRC_TIMER_SEND);
}

/* Constructor
 * arg: agent - the owning agent (to notify about timeout)
 * ret: A new DCCPTFRCNoFeedbackTimer
 */
DCCPTFRCNoFeedbackTimer::DCCPTFRCNoFeedbackTimer(DCCPTFRCAgent* agent) : TimerHandler(){
	agent_ = agent;
}

/* Called when the timer has expired
 * arg: e - The event that happened (i.e. the timer expired)
 */
void DCCPTFRCNoFeedbackTimer::expire(Event *e){
	agent_->timeout(DCCP_TFRC_TIMER_NO_FEEDBACK);
}

//private methods

/* Clear the send history */
inline void DCCPTFRCAgent::clearSendHistory(){
	struct s_hist_entry *elm, *elm2;

	/* Empty packet history */
	elm = STAILQ_FIRST(&s_hist_);
	while (elm != NULL) {
		elm2 = STAILQ_NEXT(elm, linfo_);
		delete elm;
		elm = elm2;
	}
	
	STAILQ_INIT(&s_hist_);
}

/* Find a specific packet in the send history
 * arg: seq_num - sequence number of packet to find
 *      start_elm - start searching from this element
 * ret: pointer to the correct element if the element is found
 *      otherwise the closest packet with lower seq num is returned
 *      which can be NULL.
 */
inline struct s_hist_entry* DCCPTFRCAgent::findPacketInSendHistory(u_int32_t seq_num, s_hist_entry* start_elm){
	struct s_hist_entry *elm = start_elm;
	while (elm != NULL && elm->seq_num_ > seq_num)
		elm = STAILQ_NEXT(elm, linfo_);
	return elm;
}

/* Remove packets from send history
 * arg: trim_to - remove packets with lower sequence numbers than this
 * Note: if trim_to does not exist in the history, trim_to is set to the next
 *       lower sequence number found before trimming. 
 */
void DCCPTFRCAgent::trimSendHistory(u_int32_t trim_to){
	if (trim_to > s_hist_last_seq_){
		struct s_hist_entry *elm, *elm2;
		//find packet corresponding to trim_to
		elm = findPacketInSendHistory(trim_to, STAILQ_FIRST(&s_hist_));
		if (elm != NULL){
			//remove older packets
			elm2 = STAILQ_NEXT(elm, linfo_);	
			while (elm2 != NULL){
				STAILQ_REMOVE(&s_hist_,elm2,s_hist_entry,linfo_);
				delete elm2;
				elm2 = STAILQ_NEXT(elm, linfo_);	
			}
		}
		s_hist_last_seq_ = trim_to;
	}
}

/* Prints the contents of the send history */
void DCCPTFRCAgent::printSendHistory(){
	struct s_hist_entry *elm = STAILQ_FIRST(&s_hist_);
	if (elm == NULL)
		fprintf(stdout, "Packet history is empty (send)\n");
	else {
		fprintf(stdout, "Packet history (send):\n");
		while(elm != NULL){
			fprintf(stdout,"Packet: seq %d, t_sent %f, ecn %d, wincount %u\n", elm->seq_num_, elm->t_sent_, elm->ecn_, elm->win_count_);
			elm = STAILQ_NEXT(elm, linfo_);
		}
	}
}

/* Clear the history of received packets */
inline void DCCPTFRCAgent::clearRecvHistory(){
	struct r_hist_entry *elm, *elm2;

	/* Empty packet history */
	elm = STAILQ_FIRST(&r_hist_);
	while (elm != NULL) {
		elm2 = STAILQ_NEXT(elm, linfo_);
		delete elm;
		elm = elm2;
	}
	
	STAILQ_INIT(&r_hist_);
}

/* Insert a packet in the receive history
 * Will not insert "lost" packets or duplicates.
 * arg: packet - entry representing the packet to insert
 * ret: true if packet was inserted, otherwise false.
 */
bool DCCPTFRCAgent::insertInRecvHistory(r_hist_entry *packet){
	struct r_hist_entry *elm, *elm2;
	int num_later = 0;
	fflush(stdout);
	if (STAILQ_EMPTY(&r_hist_)){  //history is empty
		STAILQ_INSERT_HEAD(&r_hist_, packet, linfo_);
		if (packet->type_ != DCCP_ACK)
				r_last_data_pkt_ = packet;
	} else {  //history contains at least one entry
		elm = STAILQ_FIRST(&r_hist_);
		if (packet->seq_num_ > elm->seq_num_){  //insert first in history
			STAILQ_INSERT_HEAD(&r_hist_, packet, linfo_);
			if (packet->type_ != DCCP_ACK)
				r_last_data_pkt_ = packet;
		} else if (packet->seq_num_ == elm->seq_num_) //duplicate
			return false;
		else {  //packet should be inserted somewhere after the head
			num_later = 1;

			//walk through the history to find the correct place
			elm2 = STAILQ_NEXT(elm,linfo_);
			while(elm2 != NULL){
				if (packet->seq_num_ > elm2->seq_num_){
					STAILQ_INSERT_AFTER(&r_hist_, elm, packet, linfo_);
					if (packet->type_ != DCCP_ACK)
						r_last_data_pkt_ = packet;
					
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
				STAILQ_INSERT_TAIL(&r_hist_, packet, linfo_);
				if (packet->type_ != DCCP_ACK)
					r_last_data_pkt_ = packet;
			}
			
		}
	}

	removeAcksRecvHistory();
	
	return true;
}

/* Prints the contents of the receive history */
void DCCPTFRCAgent::printRecvHistory(){
	struct r_hist_entry *elm = STAILQ_FIRST(&r_hist_);
	if (elm == NULL)
		fprintf(stdout, "Packet history is empty (recv)\n");
	else {
		fprintf(stdout, "Packet history (recv):\n");
		while(elm != NULL){
			fprintf(stdout,"Packet: seq %d, type %s, ndp %d, size %d, win_count %d, t_recv %f\n", elm->seq_num_, packetTypeAsStr(elm->type_), elm->ndp_, elm->size_, elm->win_count_, elm->t_recv_);
			elm = STAILQ_NEXT(elm, linfo_);
		}
	}	
}

/* Trim receive history
 * arg: time - remove packet with recv time less than this
 *      seq_num - remove packet with seq num less than this
 * Note: both of the above condition must be true for a packet
 *       to be removed. Furthermore, the function always keeps
 *       at least num_dup_acks_ packets in the history.
 */
void DCCPTFRCAgent::trimRecvHistory(double time, u_int32_t seq_num){
	struct r_hist_entry *elm, *elm2;
	int num_later = 1;
	elm = STAILQ_FIRST(&r_hist_);

	//find the packet after the num_dup_acks_ limit
	while (elm != NULL && num_later <= num_dup_acks_){
		num_later++;
		elm = STAILQ_NEXT(elm, linfo_);
	}

        if (elm != NULL){
		//ensure that there exist atleast one data packet after num_dup_acks+1 limit (for loss event detection)
		elm = findDataPacketInRecvHistory(STAILQ_NEXT(elm,linfo_));

		if (elm != NULL){
			elm2 = STAILQ_NEXT(elm, linfo_);
			while(elm2 != NULL){
				if (elm2->seq_num_ < seq_num && elm2->t_recv_ < time){
					STAILQ_REMOVE(&r_hist_,elm2,r_hist_entry,linfo_);
				        delete elm2;
				} else 
					elm = elm2;
				elm2 = STAILQ_NEXT(elm, linfo_);
			}
		}
	}
}

/* Remove all acks after the num_dup_acks_ limit */
void DCCPTFRCAgent::removeAcksRecvHistory(){
	struct r_hist_entry *elm1 = STAILQ_FIRST(&r_hist_);
	struct r_hist_entry *elm2;
	
	int num_later = 1;
	//find the packet after the num_dup_acks_ limit
	while (elm1 != NULL && num_later <= num_dup_acks_){
		num_later++;
		elm1 = STAILQ_NEXT(elm1, linfo_);
	}

	if(elm1 == NULL)
		return;
	
	elm2 = STAILQ_NEXT(elm1, linfo_);
	while(elm2 != NULL){
		if (elm2->type_ == DCCP_ACK){
			STAILQ_REMOVE(&r_hist_,elm2,r_hist_entry,linfo_);
			delete elm2;
		} else {
			elm1 = elm2;
		}
		elm2 = STAILQ_NEXT(elm1, linfo_);
	}
}

/* Find a data packet in the receive history
 * arg: start - pointer to the first packet to search from
 * ret: pointer to the found data packet or
 *      or NULL if none is found.
 */
inline r_hist_entry *DCCPTFRCAgent::findDataPacketInRecvHistory(r_hist_entry *start){
	while(start != NULL && start->type_ == DCCP_ACK)
		start = STAILQ_NEXT(start,linfo_);
	return start;
}

/* Detects packet loss in recv history.
 * arg: seq_start, seq_end - the lost packet is in [start,end]
 *      win_count          - window counter of packet before the loss
 * ret: true if a packet loss was detected, otherwise false.
 */
bool DCCPTFRCAgent::detectLossRecv(u_int32_t *seq_start, u_int32_t *seq_end, u_int8_t *win_count){
	bool result = false;
	struct r_hist_entry *before = STAILQ_FIRST(&r_hist_);
	int num_later = 1;
	//find the packet before the num_dup_acks_ limit
	while (before != NULL && num_later < num_dup_acks_){
		num_later++;
		before = STAILQ_NEXT(before, linfo_);
	}
	
	struct r_hist_entry *after = NULL;
	//find the packet after the limit
	if (before != NULL)
		after = STAILQ_NEXT(before, linfo_);
	
	if (before == NULL || after == NULL)
		return false;

	u_int32_t dist = before->seq_num_ - after->seq_num_;
	
	if (dist == 1)  //no loss
		return false;

	//we have loss, check if it includes a data packet by comparing ndp values
	/* check no data packets */
	if(before->type_ == DCCP_DATA || before->type_ == DCCP_DATAACK)
		dist -= 1;

	if(dist % DCCP_NDP_LIMIT != (u_int32_t) ((int) before->ndp_ - (int) after->ndp_+DCCP_NDP_LIMIT) % DCCP_NDP_LIMIT){
		result = true;
		*seq_start = before->seq_num_ - 1;
		*seq_end = after->seq_num_ + 1;
		after = findDataPacketInRecvHistory(after);
		if (after == NULL)
			*win_count = 0;
		else
			*win_count = after->win_count_;
	}

	return result;
}

/* Detect packet marks in recv history before num_dup_ack limit.
 * arg: seq_num - sequence number of marked packet (if any)
 *      win_count - win_count of marked packet (if any)
 * ret: true if a packet mark was found, otherwise false.
 */
bool DCCPTFRCAgent::detectECNRecv(u_int32_t *seq_num, u_int8_t *win_count){
	struct r_hist_entry *elm;
	int num_later = 1;
	elm = STAILQ_FIRST(&r_hist_);

	//walk through all packets <= num_dup_acks_
	while (elm != NULL && num_later <= num_dup_acks_){
		if ((elm->type_ == DCCP_DATA || elm->type_ == DCCP_DATAACK)
		    && elm->ecn_ == ECN_CE){
			elm->ecn_ = ECN_NOT_ECT;
			*seq_num = elm->seq_num_;
			*win_count = elm->win_count_;
			return true;
		}
		num_later++;
		elm = STAILQ_NEXT(elm, linfo_);
	}
	return false;
}

/* Calculate the total amount of data in recent packets.
 * arg: time - sum data in packets received later than this time
 * ret: total amount of data
 */
u_int32_t DCCPTFRCAgent::sumPktSizes(double time){
	struct r_hist_entry *walker = STAILQ_FIRST(&r_hist_);
	u_int32_t sum = 0;
	walker = findDataPacketInRecvHistory(walker);

	while(walker != NULL){
		if (walker->t_recv_ >= time)
			sum += walker->size_;
		walker = findDataPacketInRecvHistory(STAILQ_NEXT(walker,linfo_));
	}
	return sum;
}

/* Sample the round trip time
 * arg: rtt - the obtained rtt
 *      last_seq - sequence number of oldest packet used
 * ret: true if the rtt could succesfully be estimated from wc
 *      false otherwise. Note that rtt above is always valid.
 * Note: If not enough packets exist to use wc to estimate rtt,
 *       the rtt measured on handshake packets are used.
 *       The function will disregard packets with wc >= max_wc_inc_. 
 */
bool DCCPTFRCAgent::sampleRTT(double *rtt, u_int32_t *last_seq){
	*last_seq = 0;
	if (r_last_data_pkt_ == NULL){
		*rtt = rtt_conn_est_;
		return false;
	}

	struct r_hist_entry *elm, *elm2;
	struct r_hist_entry *last = NULL;

	elm = findDataPacketInRecvHistory(STAILQ_NEXT(r_last_data_pkt_, linfo_));
	int prev_wc = r_last_data_pkt_->win_count_;
       
	while (elm != NULL) {
		last = elm;
		if ((prev_wc - (int)(elm->win_count_) + ccval_limit_) % ccval_limit_ >= max_wc_inc_) {
			debug("%f, DCCP/TFRC(%s)::sampleRTT() - Win count distance of %d between to packets is too large (r_rtt_ %f)\n", now(), name(),(prev_wc - (int)(elm->win_count_) + ccval_limit_) % ccval_limit_, (double) r_rtt_);
			//printRecvHistory();
			if (r_rtt_ > 0.0){  //we already have an rtt estimate
				*rtt = r_rtt_;
				*last_seq = last->seq_num_;
			} else {  //first estimation
				*rtt = rtt_conn_est_;
			}
			return false;
		}
		prev_wc = (int)(elm->win_count_);
		
		if (((int) r_last_data_pkt_->win_count_ - (int)(elm->win_count_) + ccval_limit_) % ccval_limit_ > win_count_per_rtt_){
			
			elm2 = findDataPacketInRecvHistory(STAILQ_NEXT(elm,linfo_));
			while(elm2 != NULL){
				if(elm2->win_count_ == last->win_count_ && elm2->t_recv_ > last->t_recv_)
					last = elm2;
				else if (elm2->win_count_ != last->win_count_)
					break;
				elm2 = findDataPacketInRecvHistory(STAILQ_NEXT(elm2,linfo_));
			}
			break;
		}

		elm = findDataPacketInRecvHistory(STAILQ_NEXT(elm, linfo_));
	}

	if (last == NULL){
		*rtt = rtt_conn_est_;
		return false;
	}

	int d = ((int) r_last_data_pkt_->win_count_ - (int) (last->win_count_) + ccval_limit_) % ccval_limit_;
	if (d <= win_count_per_rtt_){
		debug("%f, DCCP/TFRC(%s)::sampleRTT() - Win count distance of %d is too small (r_rtt_ %f)\n", now(), name(), d, (double) r_rtt_);
		if (r_rtt_ > 0.0){  //we already have an rtt estimate
			*rtt = r_rtt_;
			*last_seq = last->seq_num_;
		} else {  //first estimation
			*rtt = rtt_conn_est_;
		}
		return false;
	}
	
	*last_seq = last->seq_num_;
	*rtt = ((double) win_count_per_rtt_)*(r_last_data_pkt_->t_recv_ - last->t_recv_)/(double) (d);

	return (elm != NULL);
}

/* Calculate the receive rate to send on feedbacks
 * ret: the receive rate
 */
double DCCPTFRCAgent::calcXrecv(){
	u_int32_t size = 0;
	double t = now() - r_t_last_feedback_;
	if (t < r_rtt_){
		size = sumPktSizes(now()-r_rtt_);
		t = r_rtt_;
		debug("%f, DCCP/TFRC(%s)::calcXrecv() - Used rtt %f as t -> xrecv = %f\n", now(), name(), t, ((double) size) / t);
	} else {
		size = r_bytes_recv_;
		debug("%f, DCCP/TFRC(%s)::calcXrecv() - Used time since last feedback %f as t -> xrecv = %f\n", now(), name(), t, ((double) size) / t);
	}
	return ( ((double) size) / t);
}

/* Clear the loss event history */
void DCCPTFRCAgent::clearLIHistory(){
	struct li_hist_entry *li_elm,*li_elm2;
	/* Empty loss interval history */
	li_elm = TAILQ_FIRST(&(r_li_hist_));
	while (li_elm != NULL) {
		li_elm2 = TAILQ_NEXT(li_elm, linfo_);
		delete li_elm;
		li_elm = li_elm2;
	}
	TAILQ_INIT(&(r_li_hist_));
}

/* Print the loss event history */
void DCCPTFRCAgent::printLIHistory(){
	struct li_hist_entry *li_elm;

	if (TAILQ_EMPTY(&r_li_hist_))
		printf("Loss interval history is empty\n");
	else {
		printf("Loss interval history:\n");
		li_elm = TAILQ_FIRST(&(r_li_hist_));
		while (li_elm != NULL) {
			printf("Length %d, start %d, win_count %d\n",li_elm->interval_,li_elm->seq_num_,li_elm->win_count_);
			li_elm = TAILQ_NEXT(li_elm, linfo_);
		}
	}
}

/* Detects quiescence of the corresponding sender
 * ret: true if sender is quiescent
 *      false otherwise
 */
bool DCCPTFRCAgent::detectQuiescence(){
	debug("%f, DCCP/TFRC(%s)::detectQuiescence() - scheme %d, time now %f, time high data %f, rtt %f\n", now(), name(), q_scheme_, now(), r_q_t_data_,(double) r_rtt_);

	if (q_scheme_ == DCCP_Q_SCHEME_Q_OPT){
		if(r_sender_quiescent_)
			debug("%f, DCCP/TFRC(%s)::detectQuiescence() - Receiver detected that the corresponding sender is quiescent (Q_OPT)\n", now(),name());
		return r_sender_quiescent_;
	} else if (q_scheme_ == DCCP_Q_SCHEME_Q_FEAT){
		if (q_local_)
			debug("%f, DCCP/TFRC(%s)::detectQuiescence() - Receiver detected that the corresponding sender is quiescent (Q_FEAT)\n", now(),name());
		return q_local_;
	}
	
	if (r_rtt_ <= 0.0)
		return false;
	
	double t = q_min_t_;
	if (t < 2*r_rtt_)  
		t = 2*r_rtt_; 
	if (now()-r_q_t_data_ >= t)
		debug("%f, DCCP/TFRC(%s)::detectQuiescence() - Receiver detected that the corresponding sender is quiescent (NORMAL)\n", now(), name());
	return (now()-r_q_t_data_ >= t);
}

//protected methods

/* OTcl binding of variables */
void DCCPTFRCAgent::delay_bind_init_all(){
	delay_bind_init_one("use_loss_rate_local_");
	delay_bind_init_one("use_loss_rate_remote_");
	delay_bind_init_one("rtt_scheme_local_");
	delay_bind_init_one("rtt_scheme_remote_");
	delay_bind_init_one("num_dup_acks_");
	delay_bind_init_one("p_tol_");
	delay_bind_init_one("win_count_per_rtt_");
	delay_bind_init_one("max_wc_inc_");
	delay_bind_init_one("s_use_osc_prev_");
	delay_bind_init_one("s_smallest_p_");
	delay_bind_init_one("s_rtt_q_");
	delay_bind_init_one("s_rtt_q2_");
	delay_bind_init_one("s_t_mbi_");
	delay_bind_init_one("s_os_time_gran_");
	delay_bind_init_one("s_s_");
	delay_bind_init_one("s_initial_x_");
	delay_bind_init_one("s_initial_rto_");
	delay_bind_init_one("s_x_");
	delay_bind_init_one("s_x_inst_");
	delay_bind_init_one("s_x_recv_");
	delay_bind_init_one("s_r_sample_");
	delay_bind_init_one("s_rtt_");
	delay_bind_init_one("s_r_sqmean_");
	delay_bind_init_one("s_p_");
	delay_bind_init_one("s_q_opt_ratio_");
	delay_bind_init_one("r_s_");
	delay_bind_init_one("r_rtt_");
	delay_bind_init_one("r_p_");
	delay_bind_init_one("q_min_t_");
	DCCPAgent::delay_bind_init_all();
}

int DCCPTFRCAgent::delay_bind_dispatch(const char *varName, const char *localName, TclObject *tracer){
	if (delay_bind(varName, localName, "use_loss_rate_local_", &use_loss_rate_local_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "use_loss_rate_remote_", &use_loss_rate_remote_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "rtt_scheme_local_", &rtt_scheme_local_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "rtt_scheme_remote_", &rtt_scheme_remote_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "num_dup_acks_", &num_dup_acks_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "p_tol_", &p_tol_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "win_count_per_rtt_", &win_count_per_rtt_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "max_wc_inc_", &max_wc_inc_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "s_use_osc_prev_", &s_use_osc_prev_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "s_smallest_p_", &s_smallest_p_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "s_rtt_q_", &s_rtt_q_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "s_rtt_q2_", &s_rtt_q2_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "s_t_mbi_", &s_t_mbi_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "s_os_time_gran_", &s_os_time_gran_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "s_s_", &s_s_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "s_initial_x_", &s_initial_x_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "s_initial_rto_", &s_initial_rto_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "s_x_", &s_x_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "s_x_inst_", &s_x_inst_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "s_x_recv_", &s_x_recv_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "s_r_sample_", &s_r_sample_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "s_rtt_", &s_rtt_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "s_p_", &s_p_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "s_r_sqmean_", &s_r_sqmean_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "s_q_opt_ratio_", &s_q_opt_ratio_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "r_s_", &r_s_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "r_rtt_", &r_rtt_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "r_p_", &r_p_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "q_min_t_", &q_min_t_, tracer)) return TCL_OK;
	return DCCPAgent::delay_bind_dispatch(varName, localName, tracer);
}

void DCCPTFRCAgent::reset(){
	DCCPAgent::reset();
	cancelTimers();

	s_t_last_data_sent_ = (-q_min_t_)*2;
	s_q_packets_wo_opt_ = 0;

	//sender
	s_hist_last_seq_ = 0;
	
	clearSendHistory();
	
	s_x_ = s_initial_x_;
	s_x_inst_ = s_initial_x_;  
	s_x_recv_ = 0.0;
	s_x_calc_ = 0.0;
	
	s_r_sample_ = 0.0;
	s_rtt_ = 0.0;
	s_r_sqmean_ = 0.0;
	s_p_ = 0.0;
	
	s_t_ld_ = -1.0;

	s_last_win_count_ = 0;
	s_t_last_win_count_ = -1.0;
	s_wc_ = 0;
	s_recv_wc_ = 0;
		
	s_idle_ = 0;
	s_t_rto_ = 0.0;

	s_t_nom_ = 0.0;
	s_t_ipi_ = 0.0;
	s_delta_ = 0.0;
	
	/* init packet history */
	STAILQ_INIT(&(s_hist_));
	
	s_state_ = TFRC_S_STATE_NO_SENT;

	//receiver

	clearRecvHistory();
	clearLIHistory();

	r_high_seq_recv_ = 0;
	r_high_ndp_recv_ = -1;
	r_t_high_recv_ = 0.0;
	
	r_rtt_ = 0.0;
	r_p_ = 0.0;
	
	r_last_counter_ = 0;
	r_seq_last_counter_ = 0;
	
	r_t_last_feedback_ = 0.0;
	r_bytes_recv_ = 0;	

	r_q_t_data_ = 0.0;
	r_sender_quiescent_ = false;
	
	/* init packet history */
	STAILQ_INIT(&(r_hist_));
	
	/* init loss interval history */
	TAILQ_INIT(&(r_li_hist_));
	
	r_state_ = TFRC_R_STATE_NO_DATA;
	r_last_data_pkt_ = NULL;
}

void DCCPTFRCAgent::cancelTimers(){
	s_timer_send_->force_cancel();
	s_timer_no_feedback_->force_cancel();
	DCCPAgent::cancelTimers();
}

bool DCCPTFRCAgent::processOption(u_int8_t type, u_char* data, u_int8_t size, Packet *pkt){
	bool result = DCCPAgent::processOption(type, data, size, pkt);
	u_int32_t ui32;
	if (result){
		switch(type){
		case DCCP_OPT_ACK_VECTOR_N0:
		case DCCP_OPT_ACK_VECTOR_N1:
			//check if we received a new ack vector
			if (ackv_recv_ != NULL){//&& ackv_recv_->getFirstSeqNum() >= s_high_ack_recv_){
				//check if we got all needed packets in the history
				if(s_hist_last_seq_ <= ackv_recv_->getLastSeqNum()){
					
					u_int8_t ene = 0;
					u_int32_t seqnum;
					dccp_packet_state state;
					struct s_hist_entry *elm = STAILQ_FIRST(&s_hist_);

					//walk through the ack vector and compute the ECN Nonce Echo
					ackv_recv_->startPacketWalk();
					while(ackv_recv_->nextPacket(&seqnum, &state)){
						if (state == DCCP_PACKET_RECV){
							elm = findPacketInSendHistory(seqnum,elm);
							if (elm != NULL && elm->seq_num_ == seqnum){
								ene = ene ^ (elm->ecn_);
							} else if (elm == NULL)
								break;
						}
					}
					//compare with the received echo
					if (ene != ackv_recv_->getENE()){
						printSendHistory();
						ackv_recv_->printPackets();
						  
						fprintf(stdout,"%f, DCCP/TFRC(%s)::processOption() - ECN check failed! \n", now(), name());
						sendReset(DCCP_RST_AGG_PEN,0,0,0);
						result = false;
					}
					
					//trim send history
					trimSendHistory(ackv_recv_->getLastSeqNum());
				} else
					debug("%f, DCCP/TFRC(%s)::processOption() - Ack vector includes packets not in history. Skipping ecn check for now...\n", now(), name());
			} 
			break;
		case DCCP_OPT_QUIESCENCE:
			r_sender_quiescent_ = true;
			break;
		case DCCP_TFRC_OPT_RTT:
			if (size == 4) {
				((u_char*) &ui32)[0] = data[0];
				((u_char*) &ui32)[1] = data[1];
				((u_char*) &ui32)[2] = data[2];
				((u_char*) &ui32)[3] = data[3];
			        r_rtt_ = ((double) (ui32))*DCCP_TFRC_OPT_RTT_UNIT;
			} else {
				fprintf(stdout,"%f, DCCP/TFRC(%s)::processOption() - RTT option with wrong size %d received\n",now(), name(), size);
			}
			break;
		}
	}
	return result;
}

void DCCPTFRCAgent::buildInitialFeatureList(){
	DCCPAgent::buildInitialFeatureList();
	changeFeature(DCCP_TFRC_FEAT_LOSS_EVENT_RATE, DCCP_FEAT_LOC_LOCAL);
	changeFeature(DCCP_TFRC_FEAT_LOSS_EVENT_RATE, DCCP_FEAT_LOC_REMOTE);
	changeFeature(DCCP_TFRC_FEAT_RTT_SCHEME, DCCP_FEAT_LOC_LOCAL);
	changeFeature(DCCP_TFRC_FEAT_RTT_SCHEME, DCCP_FEAT_LOC_REMOTE);
}

int DCCPTFRCAgent::setFeature(u_int8_t num, dccp_feature_location location,
	       u_char* data, u_int8_t size, bool testSet){
	switch(num){
	case DCCP_TFRC_FEAT_LOSS_EVENT_RATE:
		if (size == 1){
			if (location == DCCP_FEAT_LOC_LOCAL){
				if (use_loss_rate_local_ && data[0] ||
				    !(use_loss_rate_local_ || data[0]))
					return DCCP_FEAT_OK;
				else
					return DCCP_FEAT_NOT_PREFERED;
			
			} else {
				if (use_loss_rate_remote_ && data[0] ||
				    !(use_loss_rate_remote_ || data[0]))
					return DCCP_FEAT_OK;
				else
					return DCCP_FEAT_NOT_PREFERED;
			}	
		} else
			return DCCP_FEAT_ERR_SIZE;
		break;
	case DCCP_TFRC_FEAT_RTT_SCHEME:
		if (size == 1){
			if (location == DCCP_FEAT_LOC_LOCAL){
				if (rtt_scheme_local_ == data[0])
					return DCCP_FEAT_OK;
				else
					return DCCP_FEAT_NOT_PREFERED;
			} else {
				if (rtt_scheme_remote_ == data[0])
					return DCCP_FEAT_OK;
				else
					return DCCP_FEAT_NOT_PREFERED;
			}
		} else
			return DCCP_FEAT_ERR_SIZE;
		break;
	default:
		return DCCPAgent::setFeature(num, location, data, size); 
	}
}

int DCCPTFRCAgent::getFeature(u_int8_t num, dccp_feature_location location,
	       u_char* data, u_int8_t maxSize){

	switch(num){
	case DCCP_TFRC_FEAT_LOSS_EVENT_RATE:
		if (maxSize > 0){
			if (location == DCCP_FEAT_LOC_LOCAL)
				data[0] = (u_int8_t) use_loss_rate_local_;
			else
				data[0] = (u_int8_t) use_loss_rate_remote_;
				
			return 1;
		} else
			return DCCP_FEAT_ERR_SIZE;
		break;
	case DCCP_TFRC_FEAT_RTT_SCHEME:
		if (maxSize > 0){
			if (location == DCCP_FEAT_LOC_LOCAL)
				data[0] = (u_int8_t) rtt_scheme_local_;
			else
				data[0] = (u_int8_t) rtt_scheme_remote_;
			return 1;
		} else
			return DCCP_FEAT_ERR_SIZE;
		break;
	default:
		return DCCPAgent::getFeature(num, location, data, maxSize);
	}
}

dccp_feature_type DCCPTFRCAgent::getFeatureType(u_int8_t num){
	switch(num){
	case DCCP_TFRC_FEAT_LOSS_EVENT_RATE:
	case DCCP_TFRC_FEAT_RTT_SCHEME:
		return DCCP_FEAT_TYPE_SP;
		break;
	default:
		return DCCPAgent::getFeatureType(num);
	}
}

bool DCCPTFRCAgent::send_askPermToSend(int dataSize, Packet *pkt){
	ack_num_ = seq_num_recv_;
	bool result = ( tfrc_send_packet(dataSize) > 0 );

	if (result && dataSize == 0){  //no ecn on pure acks 
		hdr_flags* flagsh = hdr_flags::access(pkt);
		flagsh->ce() = 0;
		flagsh->ect() = 0;
	}
	
	u_int32_t el_time32 = (u_int32_t) ((now() - r_t_high_recv_)/DCCP_OPT_ELAPSED_TIME_UNIT);
	double t;
	if (q_scheme_ == DCCP_Q_SCHEME_Q_OPT && !infinite_send_ && dataSize == 0){
		t = q_min_t_;
		if (t < 2*s_rtt_)  
			t = 2*s_rtt_;
		if (now() - s_t_last_data_sent_ > t && sb_->empty()){
			s_q_packets_wo_opt_++;
			if (s_q_packets_wo_opt_ >= s_q_opt_ratio_){
				opt_->addOption(DCCP_OPT_QUIESCENCE,NULL,0);
				s_q_packets_wo_opt_ = 0;
			}
		}
	} else if (q_scheme_ == DCCP_Q_SCHEME_Q_FEAT && !infinite_send_){
		if(dataSize > 0 && q_remote_){
			if (changeFeature(DCCP_FEAT_Q,DCCP_FEAT_LOC_REMOTE))
				q_remote_ = 0;
		} else if (dataSize == 0) {
			t = q_min_t_;
			if (t < 2*s_rtt_)  
				t = 2*s_rtt_;
			if (now() - s_t_last_data_sent_ > t && sb_->empty() && !q_remote_){
				if (changeFeature(DCCP_FEAT_Q,DCCP_FEAT_LOC_REMOTE))
					q_remote_ = 1;
					
			}
		}
		
	}
	
	//add elapsed time on acknowledgments if needed
	if(result && send_ack_ && el_time32 > 0){
		debug("%f, DCCP/TFRC(%s)::send_askPermToSend() - Ack is delayed by %d (t_high_recv_ %f)\n", now(), name(),el_time32,r_t_high_recv_);
		if (el_time32 > 0 && el_time32 <= 0xFFFF){
			u_int16_t el_time16 = (u_int16_t) el_time32;
			opt_->addOption(DCCP_OPT_ELAPSED_TIME,((u_char*) &el_time16), 2);
		} else if (el_time32 > 0)
			opt_->addOption(DCCP_OPT_ELAPSED_TIME,((u_char*) &el_time32), 4); 
	}
	return result;
}

void DCCPTFRCAgent::send_packetSent(Packet *pkt, bool moreToSend, int dataSize){
	hdr_dccp *dccph = hdr_dccp::access(pkt);

	switch(dccph->type_){
	case DCCP_DATA:
	case DCCP_DATAACK:
		if (q_scheme_ == DCCP_Q_SCHEME_Q_OPT)
			s_q_packets_wo_opt_ = 0;
		s_t_last_data_sent_ = now();
		break;
	default:
		;
	}
	
	tfrc_send_packet_sent(pkt,(int) moreToSend, dataSize);
}

void DCCPTFRCAgent::send_packetRecv(Packet *pkt, int dataSize){
	tfrc_send_packet_recv(pkt);
}

void DCCPTFRCAgent::recv_packetRecv(Packet *pkt, int dataSize){
	hdr_dccp *dccph = hdr_dccp::access(pkt);

	if (r_q_t_data_ == 0.0)
		r_q_t_data_ = now();

	if(dccph->type_ == DCCP_DATA || dccph->type_ == DCCP_DATAACK){
		r_q_t_data_ = now();
		r_sender_quiescent_ = false;
	} else if (r_high_ndp_recv_ >= 0 && (int) (dccph->seq_num_ - r_high_seq_recv_) > 1){
		//some packet is missing
		if ( (int) (dccph->seq_num_ - r_high_seq_recv_) >
		     ((dccph->ndp_ - r_high_ndp_recv_ + DCCP_NDP_LIMIT) % DCCP_NDP_LIMIT)
		     ){ //data packet is "lost"
			debug("%f, DCCP/TFRC(%s)::recv_packetRecv() - Data loss detected by receiver seq_num %d - ndp %d,  high_seq_recv %d - ndp %d\n",now(),name(), dccph->seq_num_, dccph->ndp_,r_high_seq_recv_, r_high_ndp_recv_);
			r_q_t_data_ = now();
			r_sender_quiescent_ = false;
		}
	}
	
	if (dccph->seq_num_ > r_high_seq_recv_){
		r_high_seq_recv_ = dccph->seq_num_;
		r_high_ndp_recv_ = dccph->ndp_;
		r_t_high_recv_ = now();
	}
	tfrc_recv_packet_recv(pkt, dataSize);
}

/* Variable tracing */

void DCCPTFRCAgent::traceAll() {
	char wrk[500];

	sprintf(wrk,"%f %f %f %f %f %f %f %f %f %f\n",
		now(), (double) s_x_, (double) s_x_inst_,(double) s_x_recv_,
		(double) s_r_sample_, (double) s_rtt_, (double) s_r_sqmean_,
		(double) s_p_, (double) r_rtt_, (double) r_p_);

	if (channel_)
		Tcl_Write(channel_, wrk, strlen(wrk));
}


void DCCPTFRCAgent::traceVar(TracedVar* v) 
{
	char wrk[500];
	
	if (!strcmp(v->name(), "s_x_") ||
	    !strcmp(v->name(), "s_x_inst_") ||
	    !strcmp(v->name(), "s_x_recv_") ||
	    !strcmp(v->name(), "s_r_sample_") ||
	    !strcmp(v->name(), "s_rtt_") ||
	    !strcmp(v->name(), "s_r_sample_") ||
	    !strcmp(v->name(), "s_rtt_") ||
	    !strcmp(v->name(), "s_r_sqmean_") ||
	    !strcmp(v->name(), "s_p_") ||
	    !strcmp(v->name(), "r_rtt_") ||
	    !strcmp(v->name(), "r_p_")){
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
DCCPTFRCAgent::DCCPTFRCAgent() : DCCPAgent() {

	ccid_ = DCCP_TFRC_CCID;
	use_ecn_local_ = 1;
	use_ecn_remote_ = 1;
	use_ackv_local_ = 1;
	use_ackv_remote_ = 1;
	use_loss_rate_local_ = DCCP_TFRC_FEAT_DEF_LOSS_EVENT_RATE;
	use_loss_rate_remote_ = DCCP_TFRC_FEAT_DEF_LOSS_EVENT_RATE;
	rtt_scheme_local_ = DCCP_TFRC_FEAT_DEF_RTT_SCHEME;
	rtt_scheme_remote_ = DCCP_TFRC_FEAT_DEF_RTT_SCHEME;
	win_count_per_rtt_ = DCCP_TFRC_WIN_COUNT_PER_RTT;

	q_min_t_ = DCCP_TFRC_MIN_T;

	p_tol_ = DCCP_TFRC_P_TOLERANCE;
	
	s_t_last_data_sent_ = (-q_min_t_)*2;
	s_q_opt_ratio_ = DCCP_TFRC_QUIESCENCE_OPT_RATIO;
	s_q_packets_wo_opt_ = 0;
	
	s_timer_send_ = new DCCPTFRCSendTimer(this);
	s_timer_no_feedback_ = new DCCPTFRCNoFeedbackTimer(this);

	s_initial_x_ = DCCP_TFRC_INIT_SEND_RATE;
	s_initial_rto_ = DCCP_TFRC_INIT_RTO;

	s_rtt_q_ = DCCP_TFRC_RTT_Q;
	s_rtt_q2_ = DCCP_TFRC_RTT_Q2;
	s_t_mbi_ = DCCP_TFRC_MBI;
	
	s_s_ = DCCP_TFRC_STD_PACKET_SIZE;
	s_x_ = s_initial_x_;
	s_x_inst_ = s_initial_x_;
	
	s_use_osc_prev_ = 1;  
	
	s_x_recv_ = 0.0;
	s_x_calc_ = 0.0;
	
	s_r_sample_ = 0.0;
	s_rtt_ = 0.0;
	s_r_sqmean_ = 0.0;
	s_p_ = 0.0;
	s_smallest_p_ = DCCP_TFRC_SMALLEST_P;
	
	s_t_ld_ = -1.0;

	s_last_win_count_ = 0;
	s_t_last_win_count_ = -1.0;
	s_wc_ = 0;
	s_recv_wc_ = 0;
	
	s_idle_ = 0;
	s_t_rto_ = 0.0;

	s_t_nom_ = 0.0;
	s_t_ipi_ = 0.0;
	s_delta_ = 0.0;
	s_os_time_gran_ = DCCP_TFRC_OPSYS_TIME_GRAN;
	
	/* init packet history */
	STAILQ_INIT(&(s_hist_));
	
	s_state_ = TFRC_S_STATE_NO_SENT;

	max_wc_inc_ = DCCP_TFRC_MAX_WC_INC;

	s_hist_last_seq_ = 0;
	
	//receiver
	 
	r_rtt_ = 0.0;
	r_p_ = 0.0;
	
	r_last_counter_ = 0;
	r_seq_last_counter_ = 0;
	
	r_t_last_feedback_ = 0.0;
	r_bytes_recv_ = 0;	

	r_s_ = DCCP_TFRC_STD_PACKET_SIZE;

	r_num_w_ = DCCP_TFRC_NUM_W; 

	try {
		r_w_ = new double[r_num_w_];
	} catch (...) {
		fprintf(stderr, "DCCP/TFRC::DCCPTFRCAgent() - Out of memory\n");
		fflush(stdout);
		abort();
	}
	int r_num_w_half = r_num_w_/2;

	/* Init weights */
	for (int i = 0; i < r_num_w_; i++) {
		if (i < r_num_w_half)
			r_w_[i] = 1.0;
		else
			r_w_[i] = 1.0 - ((double) (i-(r_num_w_half - 1)))/(r_num_w_half + 1.0);
	}

	/* init packet history */
	STAILQ_INIT(&(r_hist_));
	
	/* init loss interval history */
	TAILQ_INIT(&(r_li_hist_));
	
	r_state_ = TFRC_R_STATE_NO_DATA;
	r_last_data_pkt_ = NULL;
	
	r_high_seq_recv_ = 0;
	r_high_ndp_recv_ = -1;
	r_t_high_recv_ = 0.0;

	r_q_t_data_ = 0.0;
	r_sender_quiescent_ = false;
}
	
/* Destructor */
DCCPTFRCAgent::~DCCPTFRCAgent(){
	/* uninit sender */
	
	/* unschedule timers */
	cancelTimers();

	/* Empty packet history */

	clearSendHistory();

	clearRecvHistory();
	clearLIHistory();

	delete [] r_w_;
	delete s_timer_send_;
	delete s_timer_no_feedback_;
}

/* Process a "function call" from OTCl
 * 
 * Supported function call handled by this agent:
 * weights <count>+<w1>+<w2>....  - set the weights for avg li calc.
 * example  $dccp weights 8+1.0+1.0+1.0+1.0+0.8+0.6+0.4+0.2
 */
int DCCPTFRCAgent::command(int argc, const char*const* argv){
	if (argc == 3) {
		if (strcmp(argv[1], "weights") == 0) {
			int len = strlen(argv[2])+1;
			if (len < 4)
				return TCL_ERROR;
			char *temp;
			try {
				temp = new char[len];
				strcpy(temp, argv[2]);
				r_num_w_ = atoi(strtok(temp, "+"));
				if (r_num_w_ <= 0){
					fprintf(stderr, "%f, DCCP/TFRC(%s)::command()/weights - Number of weights is invalid (num weights %i)\n",now(),name(),r_num_w_);
					return TCL_ERROR;
				}
				delete r_w_;
				r_w_ = new double[r_num_w_];
			} catch (...) {
				fprintf(stderr, "%f, DCCP/TFRC(%s)::command()/weights - Out of memory (num weights %i)\n",now(), name(), r_num_w_);
				return TCL_ERROR;
			}
			
			char *weight;
			for (int i = 0; i < r_num_w_; i++){
				weight = strtok(NULL, "+");
				if (weight == NULL){
					fprintf(stderr, "%f, DCCP/TFRC(%s)::command()/weights - Missing weight %i\n",now(),name(), i+1);
					return TCL_ERROR;
				}
				r_w_[i] = atof(weight);
			}
			delete [] temp;

			printf("%f, DCCP/TFRC(%s)::command() - Weights:",now(), name());
			for (int i = 0; i < r_num_w_; i++) {
				printf(" %f", r_w_[i]);
			}
			
			printf(" (tot %i)\n", r_num_w_);
			fflush(stdout);
			return TCL_OK;
		}
	}
	return (DCCPAgent::command(argc, argv));
}

/* A timeout has occured
 * arg: tno - id of timeout event
 * Handles: Send timer - DCCP_TFRC_TIMER_SEND
 *          No feedback timer - DCCP_TFRC_TIMER_NO_FEEDBACK
 */
void DCCPTFRCAgent::timeout(int tno){
	switch (tno){
	case DCCP_TFRC_TIMER_SEND:
		debug("%f, DCCP/TFRC(%s)::timeout() - Send timer expired\n", now(), name());
		output(false);
		//make sure we schedule next send time
		tfrc_send_packet_sent(NULL,0,-1); 
		break;
	case DCCP_TFRC_TIMER_NO_FEEDBACK:
		debug("%f, DCCP/TFRC(%s)::timeout() - No feedback timer expired\n", now(), name());
		tfrc_time_no_feedback();
		break;
	default:
	        DCCPAgent::timeout(tno);
	}
}

/* Sender side */

/* Calculate new t_ipi (inter packet interval) by
 *    t_ipi = s/X_inst; 
 * Note: No check for x=0 -> t_ipi ={0xFFF...,0xFFF}
 */
#define CALCNEWTIPI(ccbp)                              \
        do{                                            \
           s_t_ipi_ = ((double) s_s_) / s_x_inst_;      \
        }while (0)

/* Calculate new delta by 
 *    delta = min(t_ipi/2, t_gran/2);
 */
#define CALCNEWDELTA(ccbp)                             \
         do {                                          \
            s_delta_ = s_os_time_gran_ / 2.0;                  \
            if (s_t_ipi_ < s_os_time_gran_){     \
	       s_delta_ = s_t_ipi_/2.0;              \
	    }                                          \
	 } while (0)


/* Calculate sending rate from the throughput equation
 * arg: s - packet size (in bytes)
 *      R - round trip time (int seconds)
 *      p - loss event rate
 * ret: maximum allowed sending rate
 */
inline double DCCPTFRCAgent::tfrc_calcX(u_int16_t s, double R, double p){
	double temp = R*sqrt(2*p/3) + (4*R*(3*sqrt(3*p/8))*p*(1+32*p*p));
	
	return  (((double) s) / temp);
}

/* Find the loss event rate corresponding to a send rate
 * in the TCP throughput eq.
 * args: s - packet size (in bytes)
 *       R - Round trip time  (in seconds)
 *       x - send rate (in bytes/s)
 * returns:  the loss eventrate accurate to 5% with resp. to x
 */
double DCCPTFRCAgent::tfrc_calcP(u_int16_t s, double R, double x){
	double p0 = 0.5;
	double dp = 0.25;
	double p, res;
	int i = 0;
	p = p0;
	if (x <= tfrc_calcX(s,R,1.0)){
	  return 1.0;
	}
	
	while (i < 50){
		res = tfrc_calcX(s,R,p);
		if (res > (1.0-p_tol_)*x && res < (1.0+p_tol_)*x)
			return p;
		else if (res > x)
			p += dp;
		else
			p -= dp;
		dp = dp/2;
		i++;
	}
	debug("%f, DCCP/TFRC(%s)::tfrc_calcP() - Max iterations reached\n", now(), name());
	return p;
}

/* Set the send timer
 * arg: t_now - time now
 */
void DCCPTFRCAgent::tfrc_set_send_timer(double t_now){
	double t_temp;
	
	/* set send timer to expire in t_ipi - (t_now-t_nom_old) 
	 * or in other words after t_nom - t_now */
	t_temp = s_t_nom_-t_now;
	
	if(t_temp < 0) {
		fprintf(stderr,"%f, DCCP/TFRC(%s)::tfrc_set_send_timer() - Scheduled a negative time!\n", now(), name());
		fflush(stdout);
		abort();
	}
	debug("%f, DCCP/TFRC(%s)::tfrc_set_send_timer() - Send timer is scheduled to expire in %f s\n", now(), name(), t_temp);

	s_timer_send_->resched(t_temp);
}

/*
 * Update X by
 *    If (p > 0)
 *       x_calc = calcX(s,R,p);
 *       X = max(min(X_calc, 2*X_recv), s/t_mbi);
 *    Else
 *       If (t_now - tld >= R)
 *          X = max(min("2*X, 2*X_recv),s/R);
 *          tld = t_now;
 *   t_now - time now
 */ 
void DCCPTFRCAgent::tfrc_updateX(double t_now){
	double temp = 0;
	double t_temp = 0;
	
	if (s_p_ >= s_smallest_p_){     /* to avoid large error in calcX */
		s_x_calc_ = tfrc_calcX(s_s_, s_rtt_, s_p_);
		temp = 2*s_x_recv_;
		if (s_x_calc_ < temp)
			temp = s_x_calc_;
		s_x_ = temp;
		if(temp < ((double) s_s_)/s_t_mbi_)
			s_x_ = ((double) s_s_)/s_t_mbi_;
		
		debug("%f, DCCP/TFRC(%s)::tfrc_updateX() - Updated send rate to %f bytes/s (p > 0, %f)\n", now(), name(), (double) s_x_, (double) s_p_);

		if (s_use_osc_prev_){
			s_x_inst_ = s_x_ * s_r_sqmean_ / sqrt(s_r_sample_);
			debug("%f, DCCP/TFRC(%s)::tfrc_updateX() - Oscillation prevention adjusted send rate to %f bytes/s (p > 0, %f)\n", now(), name(), (double) s_x_inst_, (double) s_p_);
		} else
			s_x_inst_ = s_x_;
	} else {
		t_temp = t_now-s_t_ld_;
		if (t_temp >= s_rtt_) {
			temp = 2*s_x_recv_;
			if (2*s_x_ < temp)
				temp = 2*s_x_;
			s_x_ = ((double)(s_s_))/s_rtt_;
			if(temp > s_x_)
				s_x_ = temp;
			s_t_ld_ = t_now;
			s_x_inst_ = s_x_;
			debug("%f, DCCP/TFRC(%s)::tfrc_updateX() - Updated send rate to %f bytes/s (p == 0, %f)\n", now(), name(), (double) s_x_, (double) s_p_);
		} else
			debug("%f, DCCP/TFRC(%s)::tfrc_updateX() - Did not update send rate (p == 0, %f)\n", now(), name(), (double) s_p_);
	} 
}

/* Halve the sending rate when no feedback timer expires */
void DCCPTFRCAgent::tfrc_time_no_feedback(){
	double next_time_out = -1.0;
	
	switch(s_state_){
	case TFRC_S_STATE_NO_FBACK:
		debug("%f, DCCP/TFRC(%s)::tfrc_time_no_feedback() - No feedback timer expired state NO_FBACK\n", now(), name());
		
		/* half send rate */
		s_x_ = s_x_ / 2.0;
		if (s_x_ < ((double) s_s_) / s_t_mbi_)
			s_x_ = ((double) s_s_) / s_t_mbi_;

		s_x_inst_ = s_x_;
		debug("%f, DCCP/TFRC(%s)::tfrc_time_no_feedback() - Updated send rate to %f bytes/s\n", now(), name(), (double) s_x_);
    
		/* calculate next time out */
		
		next_time_out = 2.0*((double) (s_s_))/ s_x_;
				 
		if (next_time_out < s_initial_rto_)
			next_time_out = s_initial_rto_;
		break;
	case TFRC_S_STATE_FBACK:
		/* Check if IDLE since last timeout and recv rate is less than 4 packets per RTT */ 
		
		if(!(s_idle_) || (s_x_recv_ >= 4.0 * ((double) s_s_) / s_rtt_)){
			debug("%f, DCCP/TFRC(%s)::tfrc_time_no_feedback() - No feedback timer expired, state FBACK, not idle\n", now(), name());

			/* Half sending rate */
			
			/*  If (X_calc > 2* X_recv)
			 *    X_recv = max(X_recv/2, s/(2*t_mbi));
			 *  Else
			 *    X_recv = X_calc/4;
			 */
			if(s_p_ >= s_smallest_p_ && s_x_calc_ == 0.0){
				fprintf(stderr,"%f, DCCP/TFRC(%s)::tfrc_time_no_feedback() - X_calc is zero while p = %f > 0\n", now(), name(), (double) s_p_);
				fflush(stdout);
				abort();
			}
			
			
			/* check also if p i zero -> x_calc is infinity ?? */
			if(s_p_ < s_smallest_p_ || s_x_calc_ > 2*s_x_recv_){
				s_x_recv_ = s_x_recv_ / 2;
				if (s_x_recv_ < ((double)s_s_)/(2*s_t_mbi_))
					s_x_recv_ = ((double)s_s_)/(2*s_t_mbi_);
			} else {
				s_x_recv_ = s_x_calc_ / 4.0;
			}

			/* Update sending rate */
			tfrc_updateX(now());
		} 
		
		/* Schedule no feedback timer to
		 * expire in max(4*R, 2*s/X) 
		 */
		next_time_out = 2.0*((double) s_s_)/ s_x_;
		if (next_time_out < s_t_rto_)
			next_time_out = s_t_rto_;
    
		break;
	default:
		fprintf(stderr,"%f, DCCP/TFRC(%s)::tfrc_time_no_feedback() - Illegal state!\n", now(), name());
		fflush(stdout);
		abort();
		break;
	}
	
	/* Set timer */
	
	if (next_time_out <= 0.0){
		fprintf(stderr,"%f, DCCP/TFRC(%s)::tfrc_time_no_feedback() - About to schedule no feedback timer in %f s.\n", now(), name(), next_time_out);
		fflush(stdout);
		abort();
	}
	
	debug("%f, DCCP/TFRC(%s)::tfrc_time_no_feedback() - Scheduled no feedback timer to expire in %f s\n", now(), name(), next_time_out);

	s_timer_no_feedback_->resched(next_time_out);
	
	/* set idle flag */
	s_idle_ = 1;
}



/* Similar to send_askPermToSend() */
int DCCPTFRCAgent::tfrc_send_packet(int datasize){
	u_int8_t answer = 0;
	u_int8_t win_count = 0;
	u_int32_t uw_win_count = 0;
	double t_temp;
	
	/* check if pure ACK */
	if(datasize == 0){
		return 1;
	}
	
	switch (s_state_){
	case TFRC_S_STATE_NO_SENT :
		debug("%f, DCCP/TFRC(%s)::tfrc_send_packet() - DCCP asks for permission to send the first data packet\n", now(), name());
		
		s_t_nom_ = now();  /* set nominal send time for initial packet */
		
		/* init feedback timer */
		
		s_timer_no_feedback_->sched((double) s_initial_rto_);

		s_t_last_win_count_ = now();
		debug("%f, DCCP/TFRC(%s)::tfrc_send_packet() - TFRC - Permission granted. Scheduled no feedback timer (initial) to expire in %f s\n", now(), name(),(double) s_initial_rto_); 

		/* start send timer */
	
		/* Calculate new t_ipi */
		CALCNEWTIPI(cb);
		s_t_nom_ += s_t_ipi_;  /* t_nom += t_ipi */

		/* Calculate new delta */
		CALCNEWDELTA(cb);
		tfrc_set_send_timer(now());   /* schedule sendtimer */
		s_state_ = TFRC_S_STATE_NO_FBACK;
		answer = 1;
		break;
	case TFRC_S_STATE_NO_FBACK :
	case TFRC_S_STATE_FBACK :
		//if(!(s_ch_stimer.callout)){
		if (s_timer_send_->status() != TIMER_PENDING){
			t_temp = now() + s_delta_;
			
			//if (( timevalcmp(&(t_temp),&(s_t_nom_),>))){
			if (t_temp > s_t_nom_){
				/* Packet can be sent */

				if (s_t_last_win_count_ == -1.0){
					fprintf(stderr,"%f, DCCP/TFRC(%s)::tfrc_send_packet() - t_last_win_count unitialized!\n", now(), name());
					fflush(stdout);
					abort();
				}

				t_temp = now()-s_t_last_win_count_;
	
				/* calculate win_count option */
				if(s_state_ == TFRC_S_STATE_NO_FBACK){
					/* Assume RTT= t_rto(initial)/4 */
					uw_win_count = (u_int32_t) (t_temp / ((double) s_initial_rto_/(4.0*((double)win_count_per_rtt_))));
				}else{
					uw_win_count = (u_int32_t) (t_temp / (s_rtt_/((double)win_count_per_rtt_)));
				}
				/* don't increse wc with more than max_wc_inc_*/
				debug("%f, DCCP/TFRC(%s)::tfrc_send_packet() - window counter update:  time since last wc %f, rtt %f, inc %d\n", now(), name(), t_temp, (double) s_rtt_, uw_win_count);
				if(uw_win_count > (u_int32_t) max_wc_inc_)
					uw_win_count = max_wc_inc_;
				uw_win_count += s_last_win_count_;

				if (s_rtt_ > 0.0 && uw_win_count < s_recv_wc_ + win_count_per_rtt_) {
					debug("%f, DCCP/TFRC(%s)::tfrc_send_packet() - window counter changed due to last acked wc restriction: wc before %d (%d) last wc %d (%d)\n", now(), name(), uw_win_count, uw_win_count % ccval_limit_, s_recv_wc_, s_recv_wc_ % ccval_limit_);
					uw_win_count = s_recv_wc_ + win_count_per_rtt_;
				}
				s_wc_ = uw_win_count;

				win_count = uw_win_count % ccval_limit_;
				answer = 1;
			} else {
				answer = 0;
			}
		} else {
			answer = 0;
		}
		break;
	default:
		fprintf(stderr,"%f, DCCP/TFRC(%s)::tfrc_send_packet() - Illegal state!\n", now(), name());
		fflush(stdout);
		abort();
		break;
	}
	
	/* can we send? if so add options and add to packet history */
	if(answer){
		ccval_ = win_count;
		if (rtt_scheme_local_ == DCCP_TFRC_RTT_SCHEME_OPTION && s_rtt_ > 0){
			u_int32_t temp_rtt = (u_int32_t) (s_rtt_/DCCP_TFRC_OPT_RTT_UNIT);
			opt_->addOption(DCCP_TFRC_OPT_RTT,((u_char*) &temp_rtt), 4);
		}
	}
	return answer;
}

/* Similar to send_packetSent() */
void DCCPTFRCAgent::tfrc_send_packet_sent(Packet *pkt, int moreToSend, int datasize){
	hdr_dccp *dccph = (pkt == NULL ? NULL : hdr_dccp::access(pkt));
	double t_temp;
	struct s_hist_entry *packet;
	int result;

	/* check if we have sent a data packet */
	if(datasize > 0){
		/* add packet to history */
		packet = new struct s_hist_entry;

		packet->t_sent_ = now();
		packet->seq_num_ = dccph->seq_num_;
		result = getNonce(pkt);
		
		packet->ecn_ = (result >= 0 ? result : 0);
		packet->win_count_ = s_wc_;

		STAILQ_INSERT_HEAD(&(s_hist_), packet, linfo_);
		
		/* check if win_count have changed */
		if(s_wc_ != s_last_win_count_){
			s_t_last_win_count_ = now();
			s_last_win_count_ = s_wc_;
		}
		debug("%f, DCCP/TFRC(%s)::tfrc_send_packet_sent() - Packet sent (seq: %u,win_count: %u (%u),t_sent %f )\n", now(), name(), packet->seq_num_,packet->win_count_, packet->win_count_ % ccval_limit_,packet->t_sent_);
		s_idle_ = 0;
	}

	/* if timer is running, do nothing */

	if(s_timer_send_->status() == TIMER_PENDING)
		return;

	switch (s_state_){
	case TFRC_S_STATE_NO_SENT :
		/* if first was pure ack */ 
		if(datasize == 0){
			return; //goto sps_release;
		} else {
			fprintf(stderr,"%f, DCCP/TFRC(%s)::tfrc_send_packet_sent() - Packet sent in state NO_SENT included data!\n", now(), name());
			fflush(stdout);
			abort();
		}
		break;
	case TFRC_S_STATE_NO_FBACK :
	case TFRC_S_STATE_FBACK :
		if(datasize <= 0){ /* we have ack (which never can have more to send?!?!?!?) (or simulate a sent packet to schedule send timer) */
			moreToSend = 0;
		} else {
			/* Calculate new t_ipi */
			CALCNEWTIPI(cb);
			s_t_nom_ += s_t_ipi_;  /* t_nom += t_ipi */
			/* Calculate new delta */
			CALCNEWDELTA(cb);
		}

		if(!moreToSend){
			/* loop until we find a send time in the future */

			t_temp = now()+s_delta_; 

			//while((timevalcmp(&(t_temp),&(s_t_nom_),>))){
			while(t_temp > s_t_nom_){
				/* Calculate new t_ipi */
				CALCNEWTIPI(cb);
				s_t_nom_ += s_t_ipi_; 
				/* Calculate new delta */
				CALCNEWDELTA(cb);
				
				t_temp = now() + s_delta_; 
			}
			tfrc_set_send_timer(now());
		} else {
			t_temp = now() + s_delta_; 
			/* Check if next packet can not be sent immediately */

			if(!(t_temp > s_t_nom_)){
				tfrc_set_send_timer(now());   /* if so schedule sendtimer */
			}
		}
		break;
	default:
		fprintf(stderr,"%f, DCCP/TFRC(%s)::tfrc_send_packet_sent() - Illegal state!\n", now(), name());
		fflush(stdout);
		abort();
		break;
	}
}

/* Similar to send_packetRecv() */
void DCCPTFRCAgent::tfrc_send_packet_recv(Packet *pkt){
	hdr_dccp *dccph = hdr_dccp::access(pkt);
	hdr_dccpack *dccpah = hdr_dccpack::access(pkt);
	double next_time_out; 
	int res;
	u_int32_t pinv = 0;
	u_int32_t x_recv = 0;
	
	struct s_hist_entry *elm; //,*elm2;
	bool gotP;
	bool gotXrecv;
	
	/* we are only interested in ACKs */
	if (!(dccph->type_ == DCCP_ACK || dccph->type_ == DCCP_DATAACK))
		return;
	
	res = dccph->options_->getOption(DCCP_TFRC_OPT_LOSS_EVENT_RATE,(u_char *) &pinv,4);
	gotP = (res == 4);
        res = dccph->options_->getOption(DCCP_TFRC_OPT_RECV_RATE,(u_char *) &x_recv,4);
	gotXrecv = (res == 4);
	
	if (!gotP || !gotXrecv){
		debug("%f, DCCP/TFRC(%s)::tfrc_send_packet_recv() - Missing options: gotP %u, gotXrecv %u\n", now(), name(), (int) gotP, (int) gotXrecv);
		return;
	}

	debug("%f, DCCP/TFRC(%s)::tfrc_send_packet_recv() - Received options on seq %u, ack %u: pinv=%u, t_elapsed=%u, x_recv=%u\n",now(),name(),dccph->seq_num_, dccpah->ack_num_,pinv,elapsed_time_recv_,x_recv);
  

	switch (s_state_){
	case TFRC_S_STATE_NO_FBACK :
	case TFRC_S_STATE_FBACK :
		/* Calculate new round trip sample by
		 * R_sample = (t_now - t_recvdata)-t_delay;
		 */

		/* get t_recvdata from history */
		elm = STAILQ_FIRST(&(s_hist_));
		while (elm != NULL) {
			if (elm->seq_num_ == dccpah->ack_num_)
				break;
			elm = STAILQ_NEXT(elm, linfo_);
		}
		
		if(elm == NULL){  //probably an ack-of-ack (but with feedback)
			debug("%f, DCCP/TFRC(%s)::tfrc_send_packet_recv() - Acked packet (ack: %d) does not exist in history.\n", now(), name(),dccpah->ack_num_);

		} else {
			//store wc on acked packet

			s_recv_wc_ = elm->win_count_;
			
			/* Update RTT */
			
			s_r_sample_ = now() - (elm->t_sent_) - ((double) elapsed_time_recv_)*DCCP_OPT_ELAPSED_TIME_UNIT;
			
			/* Update RTT estimate by 
			 * If (No feedback recv)
			 *    R = R_sample;
			 * Else
			 *    R = q*R+(1-q)*R_sample;
			 */
			if (s_state_ == TFRC_S_STATE_NO_FBACK){
				s_state_ = TFRC_S_STATE_FBACK;
				s_rtt_ = s_r_sample_;

				if (s_use_osc_prev_)
					s_r_sqmean_ = sqrt(s_r_sample_);
			} else {
				s_rtt_ = s_rtt_q_ * s_rtt_ + (1-s_rtt_q_)* s_r_sample_;
				if (s_use_osc_prev_)
					s_r_sqmean_ = s_rtt_q2_*s_r_sqmean_ + (1-s_rtt_q2_)*sqrt(s_r_sample_);
			}
			
			debug("%f, DCCP/TFRC(%s)::tfrc_send_packet_recv() - New RTT estimate %f (last sample %f)\n", now(), name(), (double) s_rtt_, (double) s_r_sample_);

			if (s_use_osc_prev_)
				debug("%f, DCCP/TFRC(%s)::tfrc_send_packet_recv() - New Long term RTT estimate %f (last sample %f)\n", now(), name(), (double) s_r_sqmean_, (double) s_r_sample_);
			/* Update timeout interval */
			s_t_rto_ = 4*s_rtt_;
		}

		/* Update receive rate */
		s_x_recv_ = (double) x_recv;   /* x_recv in bytes per second */

		/* Update loss event rate */
		if (pinv == 0)
			s_p_ = 0;
		else {
			s_p_ = 1.0 / ((double) pinv);
			
			if(s_p_ < s_smallest_p_){
				s_p_ = s_smallest_p_;
				
				debug("%f, DCCP/TFRC(%s)::tfrc_send_packet_recv() - Smallest value of p is used!\n", now(), name());
			}
		}
    
		/* unschedule no feedback timer */
		s_timer_no_feedback_->cancel();

		/* Update sending rate */
		tfrc_updateX(now());

		/* Update next send time */
		s_t_nom_ -= s_t_ipi_;   /* revert to previous send time */
		
		/* Calculate new t_ipi */
		CALCNEWTIPI(cb);
		s_t_nom_ += s_t_ipi_; /* a new next send time */
		/* Calculate new delta */
		CALCNEWDELTA(cb);

    
		s_timer_send_->cancel();

		if(detectQuiescence()){
			send_ack_ = true;
		}
		output(false);
		tfrc_send_packet_sent(NULL,0,-1); /* make sure we schedule next send time */
  
		/* remove all packets older than the one acked from history */
		/* elm points to acked package! */

		if(!use_ackv_remote_)
			trimSendHistory(elm->seq_num_);
		
		/* Schedule no feedback timer to
		 * expire in max(4*R, 2*s/X) 
		 */
		next_time_out = 2*((double) s_s_) / s_x_;

		if (next_time_out < s_t_rto_)
			next_time_out = s_t_rto_;

		if (next_time_out <= 0.0){
			fprintf(stderr,"%f, DCCP/TFRC(%s)::tfrc_send_packet_recv() - About to schedule no feedback timer in %f s.\n", now(), name(), next_time_out);
			fflush(stdout);
			abort();
		}

		s_timer_no_feedback_->sched(next_time_out);

		debug("%f, DCCP/TFRC(%s)::tfrc_send_packet_recv() - Scheduled no feedback timer to expire in %f s\n", now(), name(), next_time_out);

		/* set idle flag */
		s_idle_ = 1;

		break;
	default:
		fprintf(stderr,"%f, DCCP/TFRC(%s)::tfrc_send_packet_recv() - Illegal state!\n", now(), name());
		fflush(stdout);
		abort();
		break;
  }
}

/* Receiver side */

/* Find a data packet in history
 * args:  cb - ccb of receiver
 *        elm - pointer to element (variable)
 *        num - number in history (variable)
 * returns:  elm points to found packet, otherwise NULL
 */
#define TFRC_RECV_FINDDATAPACKET(cb,elm,num) \
  do{ \
    elm = STAILQ_FIRST(&(r_hist_)); \
      while((elm) != NULL){ \
        if((elm)->type_ == DCCP_DATA || (elm)->type_ == DCCP_DATAACK) \
          (num)--; \
        if(num == 0) \
          break; \
        elm = STAILQ_NEXT((elm), linfo_); \
      } \
  } while (0)

/* Calculate the average loss interval
 * ret: average loss interval (1/p)
 */
double DCCPTFRCAgent::tfrc_calcImean(){
	struct li_hist_entry *elm;
	double I_tot;
	double I_tot0 = 0.0;
	double I_tot1 = 0.0;
	double W_tot = 0.0;
	int i;
	elm = TAILQ_FIRST(&(r_li_hist_));

	for (i = 0; i< r_num_w_; i++) {
		if (elm == NULL)
			goto I_panic;

		I_tot0 = I_tot0 + (elm->interval_ * r_w_[i]);
		W_tot = W_tot + r_w_[i];
		elm = TAILQ_NEXT(elm, linfo_);
	}
	
	elm = TAILQ_FIRST(&(r_li_hist_));
	elm = TAILQ_NEXT(elm, linfo_);

	for (i = 1; i <= r_num_w_; i++) {
		if (elm == NULL)
			goto I_panic;

		I_tot1 = I_tot1 + (elm->interval_ * r_w_[i-1]);
		elm = TAILQ_NEXT(elm, linfo_);
	}

	I_tot = I_tot0;       /* I_tot = max(I_tot0, I_tot1) */
	if (I_tot0 < I_tot1)
		I_tot = I_tot1;
	
	debug("%f, DCCP/TFRC(%s)::tfrc_calcImean() - I_tot0 %f, I_tot1 %f -> I_tot %f\n", now(), name(), I_tot0, I_tot1, I_tot);

	if(I_tot < W_tot) 
		I_tot = W_tot;
	
	return (I_tot/W_tot);

 I_panic: fprintf(stderr,"%f, DCCP/TFRC(%s)::tfrc_calcImean() - Loss interval history is corrupt!\n", now(), name());
	fflush(stdout);
	abort();
	return 0;
}

/* Prepare and send a feedback packet */
void DCCPTFRCAgent::tfrc_recv_send_feedback(){
	u_int32_t x_recv, pinv;
	//u_int16_t t_elapsed;
	struct r_hist_entry *elm;
	int num;

	if (r_p_ < 0.00000000025)  /* -> 1/p > 4 000 000 000 */
		pinv = 0;
	else
		pinv = (u_int32_t) ((double) 1.0 / r_p_);

	switch (r_state_){
	case TFRC_R_STATE_NO_DATA :
		x_recv = 0;
		break;
	case TFRC_R_STATE_DATA :
		/* Calculate x_recv */
		x_recv = (u_int32_t) calcXrecv();
		break;
	default:
		fprintf(stderr,"%f, DCCP/TFRC(%s)::tfrc_recv_send_feedback() - Illegal state!\n", now(), name());
		fflush(stdout);
		abort();	
		break;
	}
 
	/* Find largest win_count so far (data packet with highest seqnum so far) */
	num = 1;
	TFRC_RECV_FINDDATAPACKET(cb,elm,num);
  
	if(elm == NULL){
		fprintf(stderr,"%f, DCCP/TFRC(%s)::tfrc_recv_send_feedback() - No data packet in history!\n", now(), name());
		fflush(stdout);
		abort();
	}
	
	/* add options from variables above */
	if(opt_->addOption(DCCP_TFRC_OPT_LOSS_EVENT_RATE, (u_char*) &pinv, 4) != DCCP_OPT_NO_ERR 
	   || opt_->addOption(DCCP_TFRC_OPT_RECV_RATE, (u_char*) &x_recv, 4) != DCCP_OPT_NO_ERR){
		debug("%f, DCCP/TFRC(%s)::tfrc_recv_send_feedback() - Can't add options. No feedback sent\n", now(), name());
		//todo: remove option
		return;
	}

	send_ack_ = true;

	r_last_counter_ = elm->win_count_;  
	r_seq_last_counter_ = elm->seq_num_;
	r_t_last_feedback_ = now();   
	r_bytes_recv_ = 0;

	debug("%f, DCCP/TFRC(%s)::tfrc_recv_send_feedback() - Sending a feedback packet with pinv %u, x_recv %u, ack=%u (elm %u)\n",now(),name(),pinv, x_recv,seq_num_recv_, elm->seq_num_);

	output_ = true;
	output_flag_ = true;
}


/* Approximate the length of the first loss interval
 * ret: the length of the first loss interval
 */
u_int32_t DCCPTFRCAgent::tfrc_recv_calcFirstLI(){
	double x_recv, p;
	u_int32_t temp, pinv;

	//if we haven't got an r_rtt_ approximation yet
	if (r_rtt_ == 0.0){
		if (rtt_scheme_remote_ == DCCP_TFRC_RTT_SCHEME_WIN_COUNT){
			double temp_rtt = r_rtt_;
			sampleRTT(&temp_rtt, &temp);
			r_rtt_ = temp_rtt;
		} else
			r_rtt_ = rtt_conn_est_;
	}
	
	x_recv = calcXrecv();

	debug("%f, DCCP/TFRC(%s)::tfrc_recv_calcFirstLI() - Receive rate: %f bytes/s\n", now(), name(), x_recv);

	p = tfrc_calcP(r_s_,r_rtt_, x_recv);
	pinv = (u_int32_t) ceil(1/p);

	debug("%f, DCCP/TFRC(%s)::tfrc_recv_calcFirstLI() - Approximated p to %f -> pinv = interval length = %u  (xcalc/xrecv = %f)\n", now(), name(), p, pinv, tfrc_calcX(r_s_,r_rtt_,p) / x_recv);
	
	return pinv;
}

/* Update the loss interval history */
void DCCPTFRCAgent::tfrc_recv_updateLI(){
	struct r_hist_entry *elm;
	struct li_hist_entry *li_elm = NULL;
	u_int32_t seq_temp = 0;
	u_int8_t win_start;
	u_int8_t win_loss;
	int debug_info = 0;
	u_int32_t seq_start;
	u_int32_t seq_end;
	u_int32_t seq_loss;
	int i = 0;
	while(i < 2){
		i++;
		if(i == 1 && detectLossRecv(&seq_start, &seq_end, &win_loss)){
			/* we have found a packet loss! */
			seq_loss = seq_end;
			debug("%f, DCCP/TFRC(%s)::tfrc_recv_updateLI() - Loss found: seqloss=%d, winloss=%d\n", now(), name(), seq_loss, win_loss);
		} else if (i == 1) {
			continue;
		} else if (i == 2 && detectECNRecv(&seq_end, &win_loss)) {
			seq_loss = seq_end;
			debug("%f, DCCP/TFRC(%s)::tfrc_recv_updateLI() - ECN mark found: seqloss=%d, winloss=%d\n", now(), name(), seq_loss, win_loss);
		} else
			break;
	
		if(TAILQ_EMPTY(&(r_li_hist_))){
			debug_info = 1;
			/* first loss detected */
			debug("%f, DCCP/TFRC(%s)::tfrc_recv_updateLI() - First loss event detected\n", now(), name());
			
			/* create history */
			try{
				for(i=0; i< r_num_w_+1; i++){
					li_elm = new struct li_hist_entry;
					
					li_elm->interval_ = 0;
					li_elm->seq_num_ = 0;
					li_elm->win_count_ = 0;
					TAILQ_INSERT_HEAD(&(r_li_hist_),li_elm,linfo_);
				}
			} catch (...) {
				debug("%f, DCCP/TFRC(%s)::tfrc_recv_updateLI() - Not enough memory for loss interval history!\n", now(), name());
				clearLIHistory();
				return;
			}
			
			li_elm->seq_num_ = seq_loss;
			li_elm->win_count_ = win_loss;
			
			li_elm = TAILQ_NEXT(li_elm,linfo_);
			/* add approx interval */
			li_elm->interval_ = tfrc_recv_calcFirstLI();
			
		} else {  /* we have a loss interval history */
			debug_info = 2;
			/* Check if the loss is in the same loss event as interval start */
			if ((TAILQ_FIRST(&(r_li_hist_)))->seq_num_ > seq_loss)
				continue;
			
			win_start = (TAILQ_FIRST(&(r_li_hist_)))->win_count_;
			if ((win_loss > win_start 
			     && win_loss - win_start > win_count_per_rtt_) ||
			    (win_loss < win_start
			     && win_start - win_loss < ccval_limit_-win_count_per_rtt_)){
				/* new loss event detected */
				/* calculate last interval length */
				if (seq_loss <= TAILQ_FIRST(&(r_li_hist_))->seq_num_){
					fprintf(stderr, "seq_loss is less than last interval start\n");
					fflush(stdout);
					abort();
				}
				
				seq_temp = seq_loss - TAILQ_FIRST(&(r_li_hist_))->seq_num_;

				(TAILQ_FIRST(&(r_li_hist_)))->interval_ = seq_temp;
				
				debug("%f, DCCP/TFRC(%s)::tfrc_recv_updateLI() - New loss event detected\n", now(), name());
				
				/* Remove oldest interval */
				li_elm = TAILQ_LAST(&(r_li_hist_),li_hist_head);
				TAILQ_REMOVE(&(r_li_hist_),li_elm,linfo_);
				
				/* Create the newest interval */
				li_elm->seq_num_ = seq_loss;
				li_elm->win_count_ = win_loss;
				
				/* insert it into history */
				TAILQ_INSERT_HEAD(&(r_li_hist_),li_elm,linfo_);	
			} else
				debug("%f, DCCP/TFRC(%s)::tfrc_recv_updateLI() - Loss belongs to previous loss event\n", now(), name());
		}
	}
	
	if(TAILQ_FIRST(&(r_li_hist_)) != NULL){
		/* calculate interval to last loss event */

		elm = STAILQ_FIRST(&r_hist_);
		if (elm->seq_num_ < TAILQ_FIRST(&(r_li_hist_))->seq_num_){
			fprintf(stderr, "updating the recent li : seq_num_%d < last_loss_start %d\n",elm->seq_num_, TAILQ_FIRST(&(r_li_hist_))->seq_num_);
			fflush(stdout);
			abort();
		}
		seq_temp = elm->seq_num_-
			TAILQ_FIRST(&(r_li_hist_))->seq_num_;

		(TAILQ_FIRST(&(r_li_hist_)))->interval_ = seq_temp;
		if (debug_info > 0){
			debug("%f, DCCP/TFRC(%s)::tfrc_recv_updateLI() - Highest data packet received %u\n", now(), name(), elm->seq_num_);

		}

	}
}


/* Similar to recv_packetRecv */
void DCCPTFRCAgent::tfrc_recv_packet_recv(Packet* pkt, int dataSize){
	hdr_dccp *dccph = hdr_dccp::access(pkt);
	//hdr_dccpack *dccpah = hdr_dccpack::access(pkt);
	struct r_hist_entry *packet;
	u_int8_t win_count = 0;
	double p_prev; 
	bool ins = false;
	u_int32_t trim_to;
	double temp_rtt;
	if(!(r_state_ == TFRC_R_STATE_NO_DATA || r_state_ == TFRC_R_STATE_DATA)){
		fprintf(stderr,"%f, DCCP/TFRC(%s)::tfrc_recv_packet_recv() - Illegal state!\n", now(), name());
		fflush(stdout);
		abort();	
		return;
	}

	if (r_rtt_ <= 0.0){  //initiate the rtt estimate
		if (rtt_scheme_remote_ == DCCP_TFRC_RTT_SCHEME_WIN_COUNT){
			temp_rtt = r_rtt_;
			sampleRTT(&temp_rtt, &trim_to);
			r_rtt_ = temp_rtt;
		} else {
			r_rtt_ = rtt_conn_est_;
			trim_to = dccph->seq_num_;
		}
	}
	
	/* Check which type */
	switch(dccph->type_){
	case DCCP_ACK:
		if(r_state_ == TFRC_R_STATE_NO_DATA)
			return;
		break;
	case DCCP_DATA:
	case DCCP_DATAACK:
		break;
	default:
		fprintf(stderr,"%f, DCCP/TFRC(%s)::tfrc_recv_packet_recv() - Received wrong packet type!\n", now(), name());
		fflush(stdout);
		abort();	
		return;
	}
	
	/* Add packet to history */
	
	packet = new struct r_hist_entry;
	
	if (packet == NULL){
		debug("%f, DCCP/TFRC(%s)::tfrc_recv_packet_recv() - Not enough memory to add received packet to history (consider it lost)!\n", now(), name());
		return;
	}
	
	packet->t_recv_ = now();
	packet->seq_num_ = dccph->seq_num_;
	packet->type_ = dccph->type_;
	packet->ndp_ = dccph->ndp_;
	packet->size_ = dataSize;
	packet->ecn_ = getECNCodePoint(pkt);
	win_count = dccph->ccval_;
	packet->win_count_ = dccph->ccval_; 

	ins = insertInRecvHistory(packet);

	if (!ins){  //packet insertion failed
		delete packet;
		return;
	}
       
		
	if (dccph->type_ != DCCP_ACK){
		if (rtt_scheme_remote_ == DCCP_TFRC_RTT_SCHEME_WIN_COUNT){
			temp_rtt = r_rtt_;
			sampleRTT(&temp_rtt, &trim_to);
			r_rtt_ = temp_rtt;
		} else {
			r_rtt_ = rtt_conn_est_;
			trim_to = dccph->seq_num_;
		}
		debug("%f, DCCP/TFRC(%s)::tfrc_recv_packet_recv() - RTT estimated to %f (last seq %d)\n", now(), name(), (double) r_rtt_, trim_to);
	}
	        switch (r_state_){
		case TFRC_R_STATE_NO_DATA :
			if (dccph->type_ != DCCP_ACK){
				debug("%f, DCCP/TFRC(%s)::tfrc_recv_packet_recv() - Sending the initial feedback packet\n", now(), name());
				tfrc_recv_send_feedback();
				r_state_ = TFRC_R_STATE_DATA;
			}
			break;
		case TFRC_R_STATE_DATA : 
			r_bytes_recv_ = r_bytes_recv_ + dataSize;

			/* find loss events */
			tfrc_recv_updateLI();
			p_prev = r_p_;
			
			/* Calculate loss event rate */
			if(!TAILQ_EMPTY(&(r_li_hist_))){
				r_p_ = 1/tfrc_calcImean();
			}  
			/* check send conditions then send */
			if(r_p_ > p_prev){
				debug("%f, DCCP/TFRC(%s)::tfrc_recv_packet_recv() - Sending a feedback packet because p>p_prev\n", now(), name());
				tfrc_recv_send_feedback();
				trimRecvHistory(now() - r_rtt_,trim_to);
			} else if (dccph->type_ != DCCP_ACK) {
				if (dccph->seq_num_ > r_seq_last_counter_){ 
					/* the sequence number is newer than seq_last_count */
					if ((win_count > r_last_counter_ 
					     && win_count-r_last_counter_ >= win_count_per_rtt_) ||
					    (win_count < r_last_counter_
					     && r_last_counter_ - win_count <= ccval_limit_-win_count_per_rtt_)){
						debug("%f, DCCP/TFRC(%s)::tfrc_recv_packet_recv() - Sending a feedback packet because one rtt has passed since last packet (diff in win_count %i)\n", now(), name(),(win_count-r_last_counter_+ccval_limit_) % ccval_limit_);
						
						tfrc_recv_send_feedback();
						trimRecvHistory(now() - r_rtt_,trim_to);
					}
				}  
			}  
			break;
		default:
			fprintf(stderr,"%f, DCCP/TFRC(%s)::tfrc_recv_packet_recv() - Illegal state!\n", now(), name());
			fflush(stdout);
			abort();
			break;
		}
}

