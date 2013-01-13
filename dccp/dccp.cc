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
 * $Id: dccp.cc,v 1.1 2010/04/06 05:09:01 tom_henderson Exp $ */

#include "ip.h"
#include "dccp.h"
#include "flags.h"
#include "random.h"

//string representation of types
char* DCCPAgent::state_str_[DCCP_NUM_STATES] =
{"CLOSED", "LISTEN", "RESPOND", "REQUEST", "OPEN", "CLOSEREQ", "CLOSING"};

int DCCPAgent::hdr_size_[DCCP_NUM_PTYPES] =
{ DCCP_REQ_HDR_SIZE, DCCP_RESP_HDR_SIZE, DCCP_DATA_HDR_SIZE,
  DCCP_ACK_HDR_SIZE, DCCP_DACK_HDR_SIZE, DCCP_CREQ_HDR_SIZE,
  DCCP_CLOSE_HDR_SIZE, DCCP_RESET_HDR_SIZE };

char* DCCPAgent::ptype_str_[DCCP_NUM_PTYPES] =
{ "REQUEST", "RESPONSE", "DATA", "ACK", "DATAACK", "CLOSEREQ", "CLOSE", "RESET" };

char* DCCPAgent::reset_reason_str_[DCCP_NUM_RESET_REASONS]=
{ "Unspecified" , "Closed", "Invalid Packet", "Option Error", "Feature Error",
  "Connection Refused", "Bad Service Name", "Too Busy", "Bad Init Cookie",
  "Unknown", "Unanswered Challenge", "Fruitless Negotiation",
  "Agression Penalty", "No Connection", "Aborted", "Extended Seqnos",
  "Mandatory Failure"
};

char* DCCPAgent::feature_location_str_[DCCP_NUM_FEAT_LOCS] =
{ "LOCAL", "REMOTE" };

//OTcl linkage for DCCP agent
static class DCCPClass : public TclClass {
public:
	DCCPClass() : TclClass("Agent/DCCP") {};
	TclObject* create(int argc, const char*const* argv){
		return (new DCCPAgent());
	}
} class_dccp;

//methods for timer classes

/* Constructor
 * arg: agent - the owning agent (to notify about timeout)
 * ret: A new DCCPRetransmitTimer
 */
DCCPRetransmitTimer::DCCPRetransmitTimer(DCCPAgent* agent) : TimerHandler(){
	agent_ = agent;
}

/* Initialize the timer.
 * arg: delay       - initial delay
 *      maxToTDelay - total maximum delay
 */
void DCCPRetransmitTimer::init(double delay, double maxTotDelay){
	force_cancel();
	backoff_failed_ = false;
	delay_ = delay;
	tot_delay_ = 0;
	max_tot_delay_ = maxTotDelay;
}

/* Called when the timer has expired
 * arg: e - The event that happened (i.e. the timer expired)
 */
void DCCPRetransmitTimer::expire(Event *e){
	agent_->timeout(DCCP_TIMER_RTX);
}

/* Back off the timer (by a factor of 2)
 * The last back off will reach the max_tot_delay.
 */
void DCCPRetransmitTimer::backOff(){
	if (tot_delay_ == 0){  //first scheduled
	} else 	if (tot_delay_ + delay_ * 2 <= max_tot_delay_){
		//we have not reached max_tot_delay
		delay_ *= 2;    //exp back-off
	} else {  //if we double, we would go beyond max_tot_delay
		delay_ = max_tot_delay_-tot_delay_;
		backoff_failed_ = true;
	}
	tot_delay_ += delay_;
	
	if(delay_ > 0.0)
		resched(delay_);	
}

/* Check if the back-off failed (that is if max_tot_delay is reached)
 * ret: true if back-off failed, otherwise false. 
 */
bool DCCPRetransmitTimer::backOffFailed(){
	return backoff_failed_;
}

/* Constructor
 * arg: agent - the owning agent (to notify about timeout)
 * ret: A new DCCPSendTimer
 */
DCCPSendTimer::DCCPSendTimer(DCCPAgent* agent) : TimerHandler(){
	agent_ = agent;
}

/* Called when the timer has expired
 * arg: e - The event that happened (i.e. the timer expired)
 */
void DCCPSendTimer::expire(Event *e){
	agent_->timeout(DCCP_TIMER_SND);
}

//private methods

/* Creates a new packet and fills in some header fields
 * ret: a new packet
 */
Packet* DCCPAgent::newPacket(){
	Packet* pkt = allocpkt();
	hdr_dccp* dccph = hdr_dccp::access(pkt);
	hdr_dccpack* dccpah = hdr_dccpack::access(pkt);
	hdr_flags* flagsh = hdr_flags::access(pkt);

	//fill in seq number and (possibly) ack number
	dccph->seq_num_ = seq_num_;
	dccpah->ack_num_ = seq_num_recv_;

	dccph->options_ = NULL;
	dccph->ccval_ = 0;
	dccph->cscov_ = cscov_;
	
	if (use_ecn_local_){  //mark packet as ecn capable
		if (nonces_->uniform(-0.5,0.5) > 0){
			//set ect(1)
			flagsh->ect() = 0;
			flagsh->ce() = 1;
		} else {
			//set ect(0)
			flagsh->ect() = 1;
			flagsh->ce() = 0;
		}
	} else {
		flagsh->ect() = 0;
		flagsh->ce() = 0;
	}

	return pkt;
}

/* Check if an incoming packet is valid
 * arg: pkt - incoming packet
 * ret: true if the packet is valid, otherwise false
 */
bool DCCPAgent::checkPacket(Packet* pkt){
	bool new_pkt;
	bool result = true;
	hdr_cmn* cmnh = hdr_cmn::access(pkt);
	hdr_dccp* dccph = hdr_dccp::access(pkt);
	hdr_dccpack* dccpah = hdr_dccpack::access(pkt);

	if (cmnh->error()){  //e.g. "checksum failed"
		debug("%f, DCCP(%s)::checkPacket() - Packet is corrupt (%d)\n",
			      now(), name(), dccph->seq_num_);
		return false;
	}
	
	bool ack_recv = (dccph->type_ != DCCP_DATA) && (dccph->type_ != DCCP_REQUEST);
	if (ack_recv && packet_sent_ && dccpah->ack_num_ >= seq_num_){
		//invalid ack_num_
		debug("%f, DCCP(%s)::checkPacket() - Ack num not valid (%d)\n",
			      now(), name(), dccpah->ack_num_);
		return false;
	} else if (ack_recv && !packet_sent_){
		//if no packet has been sent, allow only resets with acknum 0
		if (!(dccph->type_ == DCCP_RESET && dccpah->ack_num_ == 0)){
			debug("%f, DCCP(%s)::checkPacket() - Ack num not valid (No packet sent!) (%d)\n",
			      now(), name(), dccpah->ack_num_);
			return false;
		}
	}
		
	new_pkt = (dccph->seq_num_ > seq_num_recv_);

	if (dccph->type_ == DCCP_RESET) //reset is always valid 
		result = true;
	else
		switch (state_){
		case DCCP_STATE_CLOSED:
			result = false;
			break;
		case DCCP_STATE_LISTEN:
			result = (dccph->type_ == DCCP_REQUEST);
		break;
		case DCCP_STATE_RESPOND:
			result = (new_pkt && dccph->type_ == DCCP_REQUEST ||
				  dccph->type_ == DCCP_CLOSE ||
				  new_pkt && dccph->type_ == DCCP_ACK ||
				  new_pkt && dccph->type_ == DCCP_DATAACK);
			break;
		case DCCP_STATE_OPEN:
			result = (dccph->type_ == DCCP_CLOSE ||
				  dccph->type_ == DCCP_CLOSEREQ && !server_ ||
				  dccph->type_ == DCCP_ACK ||
				  dccph->type_ == DCCP_DATA ||
				  dccph->type_ == DCCP_DATAACK);
			break;
		case DCCP_STATE_REQUEST:
			result = (dccph->type_ == DCCP_RESPONSE
				  && dccpah->ack_num_ == seq_num_-1 ||
				  dccph->type_ == DCCP_CLOSE);
			break;
		case DCCP_STATE_CLOSEREQ:
			result = (dccph->type_ == DCCP_CLOSE ||
				  dccph->type_ == DCCP_ACK ||
				  dccph->type_ == DCCP_DATA ||
				  dccph->type_ == DCCP_DATAACK);
		break;
		case DCCP_STATE_CLOSING:
			result = (dccph->type_ == DCCP_CLOSEREQ ||
				  dccph->type_ == DCCP_ACK ||
				  dccph->type_ == DCCP_DATA ||
				  dccph->type_ == DCCP_DATAACK);
		break;

		default:
			fprintf(stderr, "%f, DCCP(%s):checkPacket() - Illegal state (%d)!\n",
			now(), name(),state_);
			fflush(stdout);
			abort();
		}
	
	if (result){
		//set highest seqnum and acknum recv so far
		if (seq_num_recv_ < dccph->seq_num_)
			seq_num_recv_ = dccph->seq_num_;
		if (ack_recv && ack_num_recv_ < dccpah->ack_num_)
			ack_num_recv_ = dccpah->ack_num_;
	}
	return result;
}


/* Add feature negotiation options on packets for features
 * currently under negotiation. Will use getFeature() to obtain values.
 */
void DCCPAgent::addFeatureOptions(){
	int result;
	u_int8_t type;
	u_char* data = new u_char[DCCP_OPT_MAX_LENGTH];
	int i;
	
	for(i = 0; i < feat_list_used_; i++){
		if (!packet_recv_ || feat_list_first_[i] || feat_list_seq_num_[i] <= ack_num_recv_){
			//this is the first time, or last change for this
			//feature should have been confirmed by now 

			if (feat_list_loc_[i] == DCCP_FEAT_LOC_LOCAL)
				type = DCCP_OPT_CHANGEL;
			else
				type = DCCP_OPT_CHANGER;

			result	= getFeature(feat_list_num_[i], feat_list_loc_[i], data, DCCP_OPT_MAX_LENGTH);
			if (result > 0){
				feat_list_seq_num_[i] = seq_num_;
				debug("%f, DCCP(%s)::addFeatureOptions() - Adding option type %d for feat %d, location %s, seq %d\n",
				      now(), name(), type, feat_list_num_[i], featureLocationAsStr(feat_list_loc_[i]),feat_list_seq_num_[i]);
				opt_->addFeatureOption(type,feat_list_num_[i], data, result);
				feat_list_first_[i] = false;
			} else
				debug("%f, DCCP(%s)::addFeatureOptions() - getFeature() failed for feature %d, location %s. Error: %d \n",
					now(), name(), feat_list_num_[i], featureLocationAsStr(feat_list_loc_[i]),result);
			
		} else
			debug("%f, DCCP(%s)::addFeatureOptions() - Old change still pending for feat %d, location %s (seq_sent: %d ack_recv: %d)\n",
				      now(), name(), feat_list_num_[i], featureLocationAsStr(feat_list_loc_[i]),feat_list_seq_num_[i],ack_num_recv_);
	}
		
	for(i = 0; i < feat_conf_used_; i++){
		if (feat_conf_loc_[i] == DCCP_FEAT_LOC_LOCAL)
			type = DCCP_OPT_CONFIRML;
		else
			type = DCCP_OPT_CONFIRMR;
		
		result	= getFeature(feat_conf_num_[i], feat_conf_loc_[i], data, DCCP_OPT_MAX_LENGTH);
		if (result > 0){
			debug("%f, DCCP(%s)::addFeatureOptions() - Adding option type %d for feat %d, location %s\n", now(), name(), type, feat_conf_num_[i], featureLocationAsStr(feat_conf_loc_[i]));
			opt_->addFeatureOption(type,feat_conf_num_[i], data, result);
		} else
		    debug("%f, DCCP(%s)::addFeatureOptions() - getFeature() failed for feature %d, location %s. Error: %d \n", now(), name(), feat_conf_num_[i], featureLocationAsStr(feat_conf_loc_[i]),result); 
	}
	feat_conf_used_ = 0;
	delete [] data;
}

/* Finish feature negotiation for a feature abd remove it from
 * the list of ongoing feature negotiations.
 * arg: num      - feature number
 *      location - feature location
 * ret: true if the feature is removed
 *      false if it doesn't exist in the list
 */
bool DCCPAgent::finishFeatureNegotiation(u_int8_t num, dccp_feature_location location){
	int walker = 0;
	while (walker < feat_list_used_){ //walk through the list
		if (feat_list_num_[walker] == num && feat_list_loc_[walker] == location){
			//we found the feature
			//move the rest of the list one step up
			if (walker + 1 < feat_list_used_){
				for(int i = walker; i < feat_list_used_-1; i++){
					feat_list_num_[i] = feat_list_num_[i+1];
					feat_list_loc_[i] = feat_list_loc_[i+1];
				}
			}
			feat_list_used_--;
			return true;
				
		}
		walker++;
	}
	return false;
}

/* Add a feature to the list of features to confirm
 * Will only add one entry for each (feat,loc) pair
 * arg: num      - feature number
 *      location - feature location
 */
void DCCPAgent::confirmFeature(u_int8_t num, dccp_feature_location location){
	int walker = 0;
	while (walker < feat_conf_used_){ //walk through the list
		if (feat_conf_num_[walker] == num && feat_conf_loc_[walker] == location){
			//we have found the feature
			break;
		}
		walker++;
	}

	if (walker == feat_conf_used_ && feat_conf_used_ < feat_size_){
		//add to the end of list
		feat_conf_num_[walker] = num;
		feat_conf_loc_[walker] = location;
		feat_conf_used_++;
	}
}

/* Find a feature in the list of ongoing negotiations
 * arg: num      - feature number
 *      location - feature location
 * ret: position in the feature list if successfull
 *      otherwise -1
 */
int DCCPAgent::findFeatureInList(u_int8_t num, dccp_feature_location location){
	int walker = 0;
	while (walker < feat_list_used_){ //walk through the list
		if (feat_list_num_[walker] == num && feat_list_loc_[walker] == location){
			//we have found the feature
			return walker;
		}
		walker++;
	}
	return -1;
}

//protected methods

/* OTcl binding of variables */
void DCCPAgent::delay_bind_init_all(){
	delay_bind_init_one("packetSize_");
	delay_bind_init_one("initial_rtx_to_");
	delay_bind_init_one("max_rtx_to_");
	delay_bind_init_one("resp_to_");
	delay_bind_init_one("sb_size_");
	delay_bind_init_one("opt_size_");
	delay_bind_init_one("feat_size_");
	delay_bind_init_one("ackv_size_");
	delay_bind_init_one("ccid_");
	delay_bind_init_one("use_ecn_local_");
	delay_bind_init_one("use_ecn_remote_");
	delay_bind_init_one("ack_ratio_local_");
	delay_bind_init_one("ack_ratio_remote_");
	delay_bind_init_one("use_ackv_local_");
	delay_bind_init_one("use_ackv_remote_");
	delay_bind_init_one("q_scheme_");
	delay_bind_init_one("q_local_");
	delay_bind_init_one("q_remote_");
	delay_bind_init_one("snd_delay_");
	delay_bind_init_one("nam_tracevar_");
	delay_bind_init_one("trace_all_oneline_");
	delay_bind_init_one("allow_mult_neg_");
	delay_bind_init_one("ndp_limit_");
	delay_bind_init_one("ccval_limit_");
	delay_bind_init_one("num_data_pkt_");
	delay_bind_init_one("num_ack_pkt_");
	delay_bind_init_one("num_dataack_pkt_");
	delay_bind_init_one("cscov_");
	
	Agent::delay_bind_init_all();

        reset();
}

int DCCPAgent::delay_bind_dispatch(const char *varName, const char *localName, TclObject *tracer){
        if (delay_bind(varName, localName, "packetSize_", &size_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "initial_rtx_to_", &initial_rtx_to_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "max_rtx_to_", &max_rtx_to_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "resp_to_", &resp_to_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "sb_size_", &sb_size_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "opt_size_", &opt_size_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "feat_size_", &feat_size_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "ackv_size_", &ackv_size_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "ccid_", &ccid_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "use_ecn_local_", &use_ecn_local_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "use_ecn_remote_", &use_ecn_remote_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "ack_ratio_local_", &ack_ratio_local_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "ack_ratio_remote_", &ack_ratio_remote_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "use_ackv_local_", &use_ackv_local_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "use_ackv_remote_", &use_ackv_remote_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "q_scheme_", &q_scheme_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "q_local_", &q_local_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "q_remote_", &q_remote_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "snd_delay_", &snd_delay_, tracer)) return TCL_OK;
	if (delay_bind_bool(varName, localName, "trace_all_oneline_", &trace_all_oneline_ , tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "nam_tracevar_", &nam_tracevar_ , tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "allow_mult_neg_", &allow_mult_neg_ , tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "ndp_limit_", &ndp_limit_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "ccval_limit_", &ccval_limit_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "num_data_pkt_", &num_data_pkt_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "num_ack_pkt_", &num_ack_pkt_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "num_dataack_pkt_", &num_dataack_pkt_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "cscov_", &cscov_, tracer)) return TCL_OK;
	return Agent::delay_bind_dispatch(varName, localName, tracer);
}

/* Return a string representation of a type */
const char* DCCPAgent::stateAsStr(dccp_state state){
        if (state < DCCP_NUM_STATES && state >= 0)
		return state_str_[state];
	else
		return "UNKNOWN";
}

const char* DCCPAgent::packetTypeAsStr(dccp_packet_type type){
	if (type < DCCP_NUM_PTYPES && type >= 0)
		return ptype_str_[type];
	else
		return "UNKNOWN";
}

const char* DCCPAgent::resetReasonAsStr(dccp_reset_reason reason){
	if (reason < DCCP_NUM_RESET_REASONS && reason >= 0)
		return reset_reason_str_[reason];
	else
		return "Unknown";
}

const char* DCCPAgent::featureLocationAsStr(dccp_feature_location location){
	if (location < DCCP_NUM_FEAT_LOCS && location >= 0)
		return feature_location_str_[location];
	else
		return "UNKNOWN";
}

/* Return the header size (without options) of a packet type
 * arg: type - packet type
 * ret: header size
 */
int DCCPAgent::headerSize(dccp_packet_type type){
	if (type < DCCP_NUM_PTYPES && type >= 0)
		return hdr_size_[type];
	else
		return 0;
}

/* Extract the ecn codepoint
 * arg: pkt - packet
 * ret: ecn codepoint of packet
 */
dccp_ecn_codepoint DCCPAgent::getECNCodePoint(Packet* pkt){
	hdr_flags* flagsh = hdr_flags::access(pkt);
	if (flagsh->ect() == 1 && flagsh->ce() == 1)
		return ECN_CE;
	else if (flagsh->ect() == 0 && flagsh->ce() == 1)
		return ECN_ECT1;
	else if (flagsh->ect() == 1 && flagsh->ce() == 0)
		return ECN_ECT0;
	else
		return ECN_NOT_ECT;
}

/* Get the packet nonce
 * arg: pkt - packet
 * ret: 0 if ECT(0) is set
 *      1 if ECT(1) is set
 *      otherwise -1
 */
int DCCPAgent::getNonce(Packet* pkt){
	dccp_ecn_codepoint ecn = DCCPAgent::getECNCodePoint(pkt);
	if (ecn == ECN_ECT0 || ecn == ECN_ECT1)
		return ecn;
	else
		return -1;
}

/* Changes state (and cancel/init/sched timers)
 * arg: new_state - new state
 */
inline void DCCPAgent::changeState(dccp_state new_state){
	debug("%f, DCCP(%s)::changeState() - State changed from %s (%d) to %s (%d)\n",
	      now(), name(), stateAsStr(state_), state_, stateAsStr(new_state), new_state);
	state_ = new_state;
	switch (new_state){
	case DCCP_STATE_RESPOND:
		timer_rtx_->force_cancel();
		timer_rtx_->sched(resp_to_);
		break;
	case DCCP_STATE_OPEN:
		timer_rtx_->force_cancel();
		break;
	case DCCP_STATE_REQUEST:
	case DCCP_STATE_CLOSEREQ:
	case DCCP_STATE_CLOSING:
		cancelTimers();
		timer_rtx_->init(initial_rtx_to_,max_rtx_to_);
		break;
	default:
		;
	}
}

/* Reinitialize the agent */
void DCCPAgent::reset(){
	debug("%f, DCCP(%s)::reset() - Reset called (seq_num_ %d)\n", now(), name(), seq_num_);
	
	cancelTimers();
	if (server_)
		changeState(DCCP_STATE_LISTEN);
	
	if (state_ != DCCP_STATE_LISTEN && state_ != DCCP_STATE_CLOSED)
		changeState(DCCP_STATE_CLOSED);
	
	delete opt_;
	opt_ = new DCCPOptions(opt_size_);
	delete sb_;
	sb_ = new DCCPSendBuffer(sb_size_);
	delete ackv_;
	ackv_ = new DCCPAckVector(ackv_size_);
        elapsed_time_recv_ = 0;
	send_ackv_ = true;
	manage_ackv_ = true;
	seq_num_ = 0;
	seq_num_recv_ = 0;
	ack_num_recv_ = 0;
	ackv_recv_ = NULL;
	ack_num_ = 0;
	ccval_ = 0;
	send_ack_ = false;
	conn_est_ = false;
	output_ = false;
	output_flag_ = false;
	infinite_send_ = false;
	close_on_empty_ = false;
	server_ = false;
	ndp_ = 0;
	ack_ratio_local_ = DCCP_FEAT_DEF_ACK_RATIO;
	ack_ratio_remote_ = DCCP_FEAT_DEF_ACK_RATIO;
	q_local_ = DCCP_FEAT_DEF_Q;
	q_remote_ = DCCP_FEAT_DEF_Q;
	delete [] feat_list_num_;
	delete [] feat_list_loc_;
	delete [] feat_list_seq_num_;
	delete [] feat_list_first_;
	feat_list_num_ = new u_int8_t[feat_size_];
	feat_list_loc_ = new dccp_feature_location[feat_size_];
	feat_list_seq_num_ = new u_int32_t[feat_size_];
	feat_list_first_ = new bool[feat_size_];
	feat_list_used_ = 0;

	delete [] feat_conf_num_;
	delete [] feat_conf_loc_;
	feat_conf_num_ = new u_int8_t[feat_size_];
	feat_conf_loc_ = new dccp_feature_location[feat_size_];
	feat_conf_used_ = 0;
	
	seq_last_feat_neg_ = 0;
	feat_first_in_pkt_ = true;

	packet_sent_ = false;
	packet_recv_ = false;
	ar_unacked_ = 0;

	rtt_conn_est_ = 0.0;
	
	num_data_pkt_ = 0;
	num_ack_pkt_ = 0;
	num_dataack_pkt_ = 0;
}

/* Send a packet.
 * arg: try_pure_ack - if true, try to send a pure ack if cc
 *                     refuses to send a dataack
 */
void DCCPAgent::output(bool try_pure_ack){
	int data_size;
	Packet* pkt;
	hdr_dccp *dccph;
	hdr_dccpack *dccpah;
	hdr_cmn *cmnh;
	bool moreToSend;
	bool tell_cc;
	bool tell_app;
	
 again: 
	data_size = 0;
	moreToSend = false;
	tell_cc = false;
	tell_app = false;
	pkt = newPacket();
	dccph = hdr_dccp::access(pkt);
	dccpah = hdr_dccpack::access(pkt);
	cmnh = hdr_cmn::access(pkt);
       
	switch(state_){
	case DCCP_STATE_REQUEST:
		dccph->type_ = DCCP_REQUEST;
		cmnh->ptype() = PT_DCCP_REQ;
		packet_sent_ = true;
		timer_rtx_->backOff();
		rtt_conn_est_ = now();
		goto send;
		break;
	case DCCP_STATE_RESPOND:
		dccph->type_ = DCCP_RESPONSE;
		cmnh->ptype() = PT_DCCP_RESP;
		packet_sent_ = true;
		rtt_conn_est_ = now();
		goto send;
		break;
	case DCCP_STATE_OPEN: 
		if(!conn_est_){ //the last ack in handshake
			dccph->type_ = DCCP_ACK;
			cmnh->ptype() = PT_DCCP_ACK;
			send_ack_ = false;
			conn_est_ = true;
			goto send;
		}
			
		if (infinite_send_ || !(sb_->empty())){
			//we have data to send
			dccph->type_ = DCCP_DATA;
			cmnh->ptype() = PT_DCCP_DATA;
			if (!infinite_send_){
				cmnh->size() = sb_->top();
			}
			data_size = cmnh->size();
		}

		if (data_size > 0 && send_ack_){
			dccph->type_ = DCCP_DATAACK;
			cmnh->ptype() = PT_DCCP_DATAACK;
		} else if (send_ack_) {
			dccph->type_ = DCCP_ACK;
			cmnh->ptype() = PT_DCCP_ACK;
		} else if (data_size == 0)
			goto free;
		//ask cc of permission to send
		if (!send_askPermToSend(data_size,pkt)) {
			if (try_pure_ack && data_size > 0 && send_ack_){
				//we had a DCCP-DataAck packet, try to send a pure ack
				data_size = 0;
				cmnh->size() = 0;
				dccph->type_ = DCCP_ACK;
				cmnh->ptype() = PT_DCCP_ACK;
				if (!send_askPermToSend(data_size,pkt)) {
					debug("%f, DCCP(%s)::output() - CC refused sending a pure ack packet\n",
					      now(), name());
					goto free;
				}
			} else 
				goto free;
		}
		//remove from sendbuffer (if applicable)
		if (data_size > 0 && !infinite_send_){
			sb_->remove();
			tell_app = sb_->empty();
		}
		
		tell_cc = true;
		
		if (send_ack_){  //we are going to send an ack
			dccpah->ack_num_ = ack_num_;
			//add ackvector if applicable
			if (use_ackv_local_ && send_ackv_) {
				ackv_->sendAckVector(dccph->seq_num_, dccpah->ack_num_,opt_);
			}
		}
		dccph->ccval_ = ccval_;
		send_ack_ = false;
		goto send;
		break;
	case DCCP_STATE_CLOSEREQ:
		dccph->type_ = DCCP_CLOSEREQ;
		cmnh->ptype() = PT_DCCP_CLOSEREQ;
		timer_rtx_->backOff();
		break;
	case DCCP_STATE_CLOSING:
		dccph->type_ = DCCP_CLOSE;
		cmnh->ptype() = PT_DCCP_CLOSE;
		timer_rtx_->backOff();
		break;
	default:
		;
	}
       
 send:
	assert(opt_ != NULL);
	addFeatureOptions();
	
	//calculate packet size and data offset
	cmnh->size() = data_size + headerSize(dccph->type_)+opt_->getSize();
	dccph->data_offset_ = (headerSize(dccph->type_)+opt_->getSize()) / 4;

	//attach options
	dccph->options_ = opt_;
	opt_ = new DCCPOptions(opt_size_);
	
	//set ndp
	if (data_size == 0)
		ndp_ = (ndp_ + 1) % ndp_limit_;
	dccph->ndp_ = ndp_;
	
	debug("%f, DCCP(%s)::output() - type %s (%d), seq: %d, ack: %d, size: %d, data_size %d, data_offset_ %d, ndp: %d, ecn: %d\n",
	      now(), name(), packetTypeAsStr(dccph->type_), dccph->type_, dccph->seq_num_, dccpah->ack_num_, cmnh->size(), data_size, dccph->data_offset_,
	      dccph->ndp_,getECNCodePoint(pkt));

	moreToSend = (state_ == DCCP_STATE_OPEN && (infinite_send_ || !sb_->empty()));
	seq_num_++;
	if(tell_cc) //inform cc
		send_packetSent(pkt, moreToSend, data_size);

	switch(cmnh->ptype()){
	case PT_DCCP_DATA:
		num_data_pkt_++;
		break;
	case PT_DCCP_ACK:
		num_ack_pkt_++;
		break;
	case PT_DCCP_DATAACK:
		num_dataack_pkt_++;
		break;
	default:
		;
	}
	
	//send packet
	send(pkt,0);

	if (tell_app){ //inform application
		assert(!moreToSend);
		idle();

		if (close_on_empty_)
			close();
		return;
	}

	ccval_ = 0;
		
	if (moreToSend)  //try to send more data if available
		goto again;
	return;
 free:
	Packet::free(pkt);
}

/* Send a reset packet.
 * arg: reason  - reason for reset
 *      data<X> - data to include on reset packet
 */
void DCCPAgent::sendReset(dccp_reset_reason reason, u_int8_t data1,
	       u_int8_t data2, u_int8_t data3){
	Packet *pkt = newPacket();
	hdr_dccp *dccph = hdr_dccp::access(pkt);
	hdr_dccpack *dccpah = hdr_dccpack::access(pkt);
	hdr_dccpreset *dccpresh = hdr_dccpreset::access(pkt);
	hdr_cmn *cmnh = hdr_cmn::access(pkt);

	//fill in header info
	dccph->type_ = DCCP_RESET;
	cmnh->ptype() = PT_DCCP_RESET;
	dccpresh->rst_reason_ = reason;
	dccpresh->rst_data1_ = data1;
	dccpresh->rst_data2_ = data2;
	dccpresh->rst_data3_ = data3;
	cmnh->size() = headerSize(dccph->type_);
	dccph->data_offset_ = cmnh->size() / 4;
	assert(cmnh->size() % 4 != 0);
	dccph->options_ = new DCCPOptions(opt_size_);
	if (state_ != DCCP_STATE_CLOSED && state_ != DCCP_STATE_LISTEN){
		//we have valid sequence number
		ndp_ = (ndp_ + 1) % ndp_limit_;
		dccph->ndp_ = ndp_;
		seq_num_++;
	} else {
		dccph->ndp_ = 1;
		dccph->seq_num_ = 0;
	}
	dccpah->ack_num_ = seq_num_recv_;

	debug("%f, DCCP(%s)::sendReset() - Sent a RESET packet (Reason %s (%d), data (%d,%d,%d), seq: %d, ack: %d, size: %d, data_offset_ %d, ndp: %d)\n",
	      now(), name(), resetReasonAsStr(dccpresh->rst_reason_), dccpresh->rst_reason_, dccpresh->rst_data1_, dccpresh->rst_data2_, dccpresh->rst_data3_, dccph->seq_num_, dccpah->ack_num_, cmnh->size(), dccph->data_offset_,dccph->ndp_);
	send(pkt,0);
}

/* Parse options in a packet.
 * Will call parseOption() on every option found.
 * Fails if parseOption() fails.
 * arg: pkt - packet
 * ret: true if parse is successful
 *      false if the parse failed and the connection should be reset
 */
bool DCCPAgent::parseOptions(Packet *pkt){
	int result = 0;
	u_int8_t *type = new u_int8_t;
	u_char *data = new u_char[DCCP_OPT_MAX_LENGTH];
	hdr_dccp *dccph = hdr_dccp::access(pkt);
	DCCPOptions* options = dccph->options_;

	if(options == NULL){
		delete type;
		delete [] data;
		return false;
	}
	
	feat_first_in_pkt_ = true;

	options->startOptionWalk();
	result = options->nextOption(type, data, DCCP_OPT_MAX_LENGTH);

	while (result >= 0){  //walk through and process opt until done or fail
		if(!processOption(*type, data, result, pkt)){
			delete type;
			delete [] data;
			return false;
		}
			
		result = options->nextOption(type, data, DCCP_OPT_MAX_LENGTH);	
	}
	
	delete type;
	delete [] data;
	return true;
}

/* Process an incoming option.
 * arg: type - option type
 *      data - option data
 *      size - size of opption data
 *      pkt  - the packet the option was received on
 * ret: true if option processing was successful
 *      false if it fails
 */
bool DCCPAgent::processOption(u_int8_t type, u_char* data, u_int8_t size, Packet *pkt){
	debug("%f, DCCP(%s)::processOption() - Type %d, data %d, %d, %d ... size %d\n",
	      now(), name(), type, (size > 0 ? data[0] : 0),(size > 1 ? data[1] : 0),
	      (size > 2 ? data[2] : 0), size);
	int result = 0;
	dccp_feature_location loc;
	hdr_dccp *dccph = hdr_dccp::access(pkt);
	hdr_dccpack *dccpah = hdr_dccpack::access(pkt);

	//allow only changeR(ack_ratio) when connection is established
	if (state_ == DCCP_STATE_OPEN && (type == DCCP_OPT_CHANGER || type == DCCP_OPT_CONFIRML)){
		if (size > 0 && getFeatureType((u_int8_t) data[0]) == DCCP_FEAT_TYPE_SP){
			fprintf(stdout,"%f, DCCP(%s)::processOption() - Feature neg of feat %d is not allowed after connection establishment\n",
				now(), name(), data[0]);
			return true;
		}
	} else if (state_ == DCCP_STATE_OPEN && (type == DCCP_OPT_CHANGEL || type == DCCP_OPT_CONFIRMR) && size > 0 && getFeatureType((u_int8_t) data[0]) == DCCP_FEAT_TYPE_NN) {
		fprintf(stdout,"%f, DCCP(%s)::processOption() - ChangeL or ConfirmR are not allowed for non-negotiable features (feat %u)\n",
				now(), name(), data[0]);
			return true;
	}
	
	u_int16_t ui16 = 0;
	u_int32_t ui32 = 0;

	//check that feat neg options are processed in seq num order
	if (type == DCCP_OPT_CHANGEL || type == DCCP_OPT_CHANGER
	    || type == DCCP_OPT_CONFIRML || type == DCCP_OPT_CONFIRMR){
		if (packet_recv_ && feat_first_in_pkt_ && dccph->seq_num_ <= seq_last_feat_neg_){
			debug("%f, DCCP(%s)::processOption() - Type %d: feature neg option is out of order (last seq %u, seq now %u)\n", now(), name(), type, seq_last_feat_neg_,dccph->seq_num_);
			return true;
		} else {
			seq_last_feat_neg_ = dccph->seq_num_;
			feat_first_in_pkt_ = false;
		}
	}
	
	switch(type){
	case DCCP_OPT_PADDING:
		break;
	case DCCP_OPT_QUIESCENCE:
		break;
	case DCCP_OPT_CHANGEL:
		if (size > 1){
			result = setFeature(data[0], DCCP_FEAT_LOC_REMOTE, &(data[1]), size-1);
			if (result != DCCP_FEAT_OK)
				goto reset;
			confirmFeature(data[0], DCCP_FEAT_LOC_REMOTE);

			if (allow_mult_neg_ > 0 && feat_conf_used_ > 0 && state_ == DCCP_STATE_OPEN){
				//add an ack number to the packet with confirm options
				send_ack_ = true;
				ack_num_ = seq_num_recv_;
			}

		} else {
			result = DCCP_FEAT_ERR_SIZE;
			goto reset;
		}
		break;
	case DCCP_OPT_CHANGER:
		if (size > 1){
			result = setFeature(data[0], DCCP_FEAT_LOC_LOCAL, &(data[1]), size-1);
			if (result != DCCP_FEAT_OK)
				goto reset;
			confirmFeature(data[0], DCCP_FEAT_LOC_LOCAL);

			if (allow_mult_neg_ > 0 && feat_conf_used_ > 0 && state_ == DCCP_STATE_OPEN){
				//add an ack number to the packet with confirm options
				send_ack_ = true;
				ack_num_ = seq_num_recv_;
			}

		} else {
			result = DCCP_FEAT_ERR_SIZE;
			goto reset;
		}
		break;
	case DCCP_OPT_CONFIRML:
	case DCCP_OPT_CONFIRMR:
		loc = DCCP_FEAT_LOC_REMOTE;
		if (type == DCCP_OPT_CONFIRMR)
			loc = DCCP_FEAT_LOC_LOCAL;
		
		if (size > 1){
			if (featureIsChanging(data[0], loc)){
				if (allow_mult_neg_ > 0){
					if (dccph->type_ == DCCP_ACK ||
					    dccph->type_ == DCCP_DATAACK ||
					    dccph->type_ == DCCP_RESPONSE){
						
						if (dccpah->ack_num_ < feat_list_seq_num_[findFeatureInList(data[0], loc)]){
							debug("%f, DCCP(%s)::processOption() - Confirm acknowledged an old Change. Ignoring confirm...\n", now(), name());
							return true;
						}
					} else {
						debug("%f, DCCP(%s)::processOption() - Missing ack num together with confirm option.\n", now(), name());
					}
				} 
				if (setFeature(data[0], loc, &(data[1]), size-1,true) != DCCP_FEAT_ERR_TEST){
					result = setFeature(data[0], loc, &(data[1]), size-1);
					if (result != DCCP_FEAT_OK)
						goto reset;
					finishFeatureNegotiation(data[0], loc);
				} else
					debug("%f, DCCP(%s)::processOption() - Test of setFeature() failed for feature type %u. Ignoring confirm...\n", now(), name(), data[0]);
				
			} else {
				//ignore
			}
		} else {
			result = DCCP_FEAT_ERR_SIZE;
			goto reset;
		}
		break;
	case DCCP_OPT_ACK_VECTOR_N0:
	case DCCP_OPT_ACK_VECTOR_N1:
		debug("%f, DCCP(%s)::processOption() - Received ackvector (ene %d)\n",
		      now(), name(), 1-(DCCP_OPT_ACK_VECTOR_N1-type));
		if (dccph->type_ == DCCP_DATA) {
			fprintf(stdout,"%f, DCCP(%s)::processOption() - Ackvector received on DCCP-Data packet!\n",now(), name());
			break;
		}
			
		if (ackv_recv_ != NULL){
			fprintf(stderr,"%f, DCCP(%s)::processOption() - ackv_recv_ not null\n",now(), name());
			fflush(stdout);
			abort();
		}
		//store received ack vector
		ackv_recv_ = new DCCPAckVector(ackv_size_);
		if (!ackv_recv_->setAckVector(data,size,dccpah->ack_num_,1-(DCCP_OPT_ACK_VECTOR_N1-type))){
			fprintf(stdout,"%f, DCCP(%s)::processOption() - Failed to set ack vector\n",now(), name());
			delete ackv_recv_;
			ackv_recv_ = NULL;
		}
		break;
	case DCCP_OPT_ELAPSED_TIME:
		if (size == 2){
			((u_char*) &ui16)[0] = data[0];
			((u_char*) &ui16)[1] = data[1];
			elapsed_time_recv_ = ui16;
		} else if (size == 4) {
			((u_char*) &ui32)[0] = data[0];
			((u_char*) &ui32)[1] = data[1];
			((u_char*) &ui32)[2] = data[2];
			((u_char*) &ui32)[3] = data[3];
			elapsed_time_recv_ = ui32;
		} else {
			fprintf(stdout,"%f, DCCP(%s)::processOption() - Elapsed time with wrong size %d received\n",now(), name(), size);
		}
		break;
	default:
		if (type_ < DCCP_OPT_CC_START)
			debug("%f, DCCP(%s)::processOption() - Unknown option (type %d)\n",
		      now(), name(), type);
	}

	return true;
 reset:
	switch (result){
	case DCCP_FEAT_NOT_PREFERED:
		sendReset(DCCP_RST_FLESS_NEG,data[0],data[1],0);
		break;
	case DCCP_FEAT_UNKNOWN:
		fprintf(stdout,"%f, DCCP(%s)::processOption() - Unknown feature (type %d)\n",
		      now(), name(), data[0]);
		break;
	case DCCP_FEAT_ERR_SIZE:
		sendReset(DCCP_RST_FEATURE_ERR,
			  (size > 0 ? data[0] : 0),
			  (size > 1 ? data[1] : 0),
			  (size > 2 ? data[2] : 0));
		break;				
	}
	return false;
}

/* Change a feature (i.e. initiate a feature negotiation).
 * arg: num      - feature number
 *      location - feature location
 * ret: true if the feature is added to the list of ongoing feat neg
 *      false if the list is full or the feature is already present
 *            and multiple negotiations are not allowed.             
 */
bool DCCPAgent::changeFeature(u_int8_t num, dccp_feature_location location){
	if (allow_mult_neg_ > 0){
		int pos = findFeatureInList(num,location);

		if (pos >= 0){
			feat_list_seq_num_[pos] = seq_num_;
			feat_list_first_[pos] = true;
			return true;
		}
		//does not exist, check if full
		if (feat_list_used_ == feat_size_)
			return false;		
	} else {
		//check if the list is full, or if this feature is already in neg
		if (feat_list_used_ == feat_size_ || featureIsChanging(num, location))
			return false;
	}

	feat_list_num_[feat_list_used_] = num;
	feat_list_loc_[feat_list_used_] = location;
	feat_list_seq_num_[feat_list_used_] = 0;
	feat_list_first_[feat_list_used_] = true;
	debug("%f, DCCP(%s)::changeFeature() - Added feature %d, location %s\n",
	      now(), name(), num, featureLocationAsStr(location));
	feat_list_used_++;
	return true;
}

/* Check if a feature is currently under negotiation.
 * arg: num      - feature number
 *      location - feature location
 * ret: true if the feature is under negotiation
 *      false otherwise
 */
bool DCCPAgent::featureIsChanging(u_int8_t num, dccp_feature_location location){
	for (int i = 0; i< feat_list_used_; i++)
		if(feat_list_num_[i] == num && feat_list_loc_[i] == location)
			return true;
	return false;
}

/* Build the list of features to neg on DCCP-Request packet */
void DCCPAgent::buildInitialFeatureList(){
	debug("%f, DCCP(%s)::buildInitialFeatureList() - values cc %d, ecnl %d, ecnr %d, ackrl %d, ackrr %d, ackvl %d, ackvr %d\n",
			      now(), name(), ccid_, use_ecn_local_,use_ecn_remote_, ack_ratio_local_,ack_ratio_remote_,use_ackv_local_,use_ackv_remote_);
	changeFeature(DCCP_FEAT_CC, DCCP_FEAT_LOC_LOCAL);
	changeFeature(DCCP_FEAT_CC, DCCP_FEAT_LOC_REMOTE);
	changeFeature(DCCP_FEAT_ECN, DCCP_FEAT_LOC_LOCAL);
	changeFeature(DCCP_FEAT_ECN, DCCP_FEAT_LOC_REMOTE);
	changeFeature(DCCP_FEAT_ACK_RATIO, DCCP_FEAT_LOC_LOCAL);
	changeFeature(DCCP_FEAT_ACK_RATIO, DCCP_FEAT_LOC_REMOTE);
	changeFeature(DCCP_FEAT_ACKV, DCCP_FEAT_LOC_LOCAL);
	changeFeature(DCCP_FEAT_ACKV, DCCP_FEAT_LOC_REMOTE);
	changeFeature(DCCP_FEAT_Q_SCHEME, DCCP_FEAT_LOC_LOCAL);
	changeFeature(DCCP_FEAT_Q_SCHEME, DCCP_FEAT_LOC_REMOTE);
	changeFeature(DCCP_FEAT_Q, DCCP_FEAT_LOC_LOCAL);
	changeFeature(DCCP_FEAT_Q, DCCP_FEAT_LOC_REMOTE);
}	

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
int DCCPAgent::setFeature(u_int8_t num, dccp_feature_location location,
	       u_char* data, u_int8_t size, bool testSet){
	u_int16_t ui16;
	
	switch(num){
	case DCCP_FEAT_CC:
		if (size == 1){
			if (ccid_ == data[0])
				return DCCP_FEAT_OK;
			else
				return DCCP_FEAT_NOT_PREFERED;
		} else
			return DCCP_FEAT_ERR_SIZE;
		break;
	case DCCP_FEAT_ECN:
		if (size == 1){
			if (location == DCCP_FEAT_LOC_LOCAL){
				if (use_ecn_local_ && data[0] ||
				    !(use_ecn_local_ || data[0]))
					return DCCP_FEAT_OK;
				else
					return DCCP_FEAT_NOT_PREFERED;
			
			} else {
				if (use_ecn_remote_ && data[0] ||
				    !(use_ecn_remote_ || data[0]))
					return DCCP_FEAT_OK;
				else
					return DCCP_FEAT_NOT_PREFERED;
			}	
		} else
			return DCCP_FEAT_ERR_SIZE;
		break;
	case DCCP_FEAT_ACK_RATIO:
		if (size == 2){
			((u_char*) &ui16)[0] = data[0];
			((u_char*) &ui16)[1] = data[1];
			if (testSet){
				if (location == DCCP_FEAT_LOC_LOCAL){
					return ((ack_ratio_local_ != (u_int16_t) ui16) ? DCCP_FEAT_ERR_TEST : DCCP_FEAT_OK);
				} else {
					return ((ack_ratio_remote_ != (u_int16_t) ui16) ? DCCP_FEAT_ERR_TEST : DCCP_FEAT_OK);
				}
			} else {
				if (location == DCCP_FEAT_LOC_LOCAL){
					ack_ratio_local_ = (u_int16_t) ui16;
					
				} else
					ack_ratio_remote_ = (u_int16_t) ui16;
			}
			return DCCP_FEAT_OK;
		} else
			return DCCP_FEAT_ERR_SIZE;
		break;
	case DCCP_FEAT_ACKV:
		if (size == 1){
			if (location == DCCP_FEAT_LOC_LOCAL){
				if (use_ackv_local_ && data[0] ||
				    !(use_ackv_local_ || data[0]))
					return DCCP_FEAT_OK;
				else
					return DCCP_FEAT_NOT_PREFERED;
			
			} else {
				if (use_ackv_remote_ == data[0]||
				    !(use_ackv_remote_ || data[0]))
					return DCCP_FEAT_OK;
				else
					return DCCP_FEAT_NOT_PREFERED;
			}
		} else
			return DCCP_FEAT_ERR_SIZE;
		break;
	case DCCP_FEAT_Q_SCHEME:
		if (size == 1){
			if (q_scheme_ == data[0])
				return DCCP_FEAT_OK;
			else
				return DCCP_FEAT_NOT_PREFERED;
		} else
			return DCCP_FEAT_ERR_SIZE;
		break;
	case DCCP_FEAT_Q:
		if (size == 1){
			if (location == DCCP_FEAT_LOC_LOCAL){
				q_local_ = (data[0] > 0 ? 1 : 0);
			} else {
				q_remote_ = (data[0] > 0 ? 1 : 0);
			}
			return DCCP_FEAT_OK;
		} else
			return DCCP_FEAT_ERR_SIZE;
		break;
	default:
		return DCCP_FEAT_UNKNOWN;
	}
}

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
int DCCPAgent::getFeature(u_int8_t num, dccp_feature_location location,
	       u_char* data, u_int8_t maxSize){
	u_int16_t ui16;
	
	switch(num){
	case DCCP_FEAT_CC:
		if (maxSize > 0){
			data[0] = (u_int8_t) ccid_;
			return 1;
		} else
			return DCCP_FEAT_ERR_SIZE;
		break;
	case DCCP_FEAT_ECN:
		if (maxSize > 0){
			if (location == DCCP_FEAT_LOC_LOCAL)
				data[0] = (u_int8_t) use_ecn_local_;
			else
				data[0] = (u_int8_t) use_ecn_remote_;
				
			return 1;
		} else
			return DCCP_FEAT_ERR_SIZE;
		break;
	case DCCP_FEAT_ACK_RATIO:
		if (maxSize > 1){
			if (location == DCCP_FEAT_LOC_LOCAL)
				ui16 = (u_int16_t) ack_ratio_local_;
			else
				ui16 = (u_int16_t) ack_ratio_remote_;
			data[0] = ((u_char*) &ui16)[0];
			data[1] = ((u_char*) &ui16)[1];
			
			return 2;
		} else
			return DCCP_FEAT_ERR_SIZE;
		break;
	case DCCP_FEAT_ACKV:
		if (maxSize > 0){
			if (location == DCCP_FEAT_LOC_LOCAL)
				data[0] = (u_int8_t) use_ackv_local_;
			else
				data[0] = (u_int8_t) use_ackv_remote_;
				
			return 1;
		} else
			return DCCP_FEAT_ERR_SIZE;
		break;
	case DCCP_FEAT_Q_SCHEME:
		if (maxSize > 0){
			data[0] = (u_int8_t) q_scheme_;
			return 1;
		} else
			return DCCP_FEAT_ERR_SIZE;
		break;
	case DCCP_FEAT_Q:
		if (maxSize > 0){
			if (location == DCCP_FEAT_LOC_LOCAL)
				data[0] = (u_int8_t) q_local_;
			else
				data[0] = (u_int8_t) q_remote_;
				
			return 1;
		} else
			return DCCP_FEAT_ERR_SIZE;
		break;
	default:
		return DCCP_FEAT_UNKNOWN;
	}
}

/* Obtain the feature type
 * arg: num      - feature number
 * ret: the feature type, or DCCP_FEAT_TYPE_UKNOWN if unknown
 */
dccp_feature_type DCCPAgent::getFeatureType(u_int8_t num){
	switch(num){
	case DCCP_FEAT_CC:
	case DCCP_FEAT_ECN:
	case DCCP_FEAT_ACKV:
	case DCCP_FEAT_Q_SCHEME:
		return DCCP_FEAT_TYPE_SP;
		break;
	case DCCP_FEAT_ACK_RATIO:
	case DCCP_FEAT_Q:
		return DCCP_FEAT_TYPE_NN;
	default:
		return DCCP_FEAT_TYPE_UNKNOWN;
	}
}

/* Cancel all timers */
void DCCPAgent::cancelTimers(){
	timer_snd_->force_cancel();
	timer_rtx_->force_cancel();
}


/* Ask sender permission to send a packet
 * arg: dataSize - size of data in packet (0 = pure ACK)
 *      pkt      - the packet to send
 * ret: true if permission is granted, otherwise false
 * Note: Packet should not be used for other than manipulating
 *       ecn marks!
 */
bool DCCPAgent::send_askPermToSend(int dataSize, Packet *pkt){
	return (dataSize == 0 || timer_snd_->status() != TIMER_PENDING);
}

/* A(n) ACK/DATA/DATAACK packet has been sent (sender)
 * arg: pkt        - packet sent
 *      moreToSend - true if there exist more data to send
 *      dataSize   - size of data sent
 */
void DCCPAgent::send_packetSent(Packet *pkt, bool moreToSend, int dataSize) {
	if (snd_delay_ > 0)
		timer_snd_->resched(snd_delay_);
	else {
		fprintf(stderr,"%f, DCCP(%s)::send_packetSend() - snd_delay_ is 0!\n", now(), name());
		fflush(stdout);
		abort();
	}
};

/* A(n) ACK/DATA/DATAACK packet has been received (sender)
 * arg: pkt        - packet received
 *      dataSize   - size of data in packet
 * If this function would like to send a packet, set output_ = true
 * and output_flag_ if appropriate.
 */
void DCCPAgent::send_packetRecv(Packet *pkt, int dataSize) {	
};

/* A ACK/DATA/DATAACK packet has been received (receiver)
 * arg: pkt        - packet received
 *      dataSize   - size of data in packet
 * If this function would like to send a packet, set output_ = true
 * and output_flag_ if appropriate.
 */
void DCCPAgent::recv_packetRecv(Packet *pkt, int dataSize) {
	hdr_dccp *dccph = hdr_dccp::access(pkt);

	if (dccph->type_ != DCCP_DATA && dccph->type_ != DCCP_DATAACK
	    && dccph->type_ != DCCP_ACK){
		fprintf(stderr,"%f, DCCP(%s)::recv_packetRecv() - Got a packet of type %s!\n", now(), name(), packetTypeAsStr(dccph->type_));
		fflush(stdout);
		abort();
	}

	//ack ackording to ack ratio
	if(dccph->type_ == DCCP_DATA || dccph->type_ == DCCP_DATAACK){
		ar_unacked_++;
		if (ar_unacked_ >= ack_ratio_local_){
			send_ack_ = true;
			ack_num_ = seq_num_recv_;
			ar_unacked_= 0;
			output_ = true;
			output_flag_ = true;
		}
	}
};

/* Tracing functions */
void DCCPAgent::traceAll() {
}

void DCCPAgent::traceVar(TracedVar* v) {
}


//public methods

/* Constructor
 * ret: A new DCCPAgent
 */
DCCPAgent::DCCPAgent() : Agent(PT_DCCP){
	sb_size_ = DCCP_SB_SIZE;
	opt_size_ = DCCP_OPT_SIZE; 
	feat_size_ = DCCP_FEAT_SIZE;
	ackv_size_ = DCCP_ACKV_SIZE;

	ndp_limit_ = DCCP_NDP_LIMIT;
	ccval_limit_ = DCCP_CCVAL_LIMIT;
	
	server_ = false;
	seq_num_ = 0;
	seq_num_recv_ = 0;
	ack_num_recv_ = 0;
	ackv_recv_ = NULL;
	state_ = DCCP_STATE_CLOSED;
	ack_num_ = 0;
	ccval_ = 0;
	cscov_ = DCCP_CSCOV_ALL;
	send_ack_ = 0;
	conn_est_ = false;
	output_ = false;
	output_flag_ = false;
	infinite_send_ = false;
	close_on_empty_ = false;
	ndp_ = 0;
	ccid_ = DCCP_CCID;
	use_ecn_local_ = DCCP_FEAT_DEF_ECN;
	use_ecn_remote_ = DCCP_FEAT_DEF_ECN;
	ack_ratio_local_ = DCCP_FEAT_DEF_ACK_RATIO;
	ack_ratio_remote_ = DCCP_FEAT_DEF_ACK_RATIO;
	use_ackv_local_ = DCCP_FEAT_DEF_ACKV;
	use_ackv_remote_ = DCCP_FEAT_DEF_ACKV;
	q_scheme_ = DCCP_FEAT_DEF_Q_SCHEME;
	q_local_ = DCCP_FEAT_DEF_Q;
	q_remote_ = DCCP_FEAT_DEF_Q;
	
	feat_list_num_ = new u_int8_t[feat_size_];
	feat_list_loc_ = new dccp_feature_location[feat_size_];
	feat_list_seq_num_ = new u_int32_t[feat_size_];
	feat_list_first_ = new bool[feat_size_];
	feat_list_used_ = 0;

	feat_conf_num_ = new u_int8_t[feat_size_];
	feat_conf_loc_ = new dccp_feature_location[feat_size_];
	feat_conf_used_ = 0;

	seq_last_feat_neg_ = 0;
	feat_first_in_pkt_ = true;

	allow_mult_neg_ = 0;
	
	packet_sent_ = false;
	packet_recv_ = false;
	
	sb_ = new DCCPSendBuffer(sb_size_);
	opt_ = new DCCPOptions(opt_size_);
	ackv_ = new DCCPAckVector(ackv_size_);
	
	send_ackv_ = true;
	manage_ackv_ = true;

	elapsed_time_recv_ = 0;
	nonces_ = new RNG();
	nonces_->set_seed((long int) 0);

	timer_rtx_ = new DCCPRetransmitTimer(this);
	timer_snd_ = new DCCPSendTimer(this);
	
	snd_delay_ = DCCP_SND_DELAY;

	nam_tracevar_ = false;
	trace_all_oneline_ = false;

	ar_unacked_ = 0;

	rtt_conn_est_ = 0.0;
	
	initial_rtx_to_ = DCCP_INITIAL_RTX_TO;
	max_rtx_to_ = DCCP_MAX_RTX_TO;
	resp_to_ = DCCP_RESP_TO;

	num_data_pkt_ = 0;
	num_ack_pkt_ = 0;
	num_dataack_pkt_ = 0;
}

/* Destructor */
DCCPAgent::~DCCPAgent(){
	delete timer_rtx_;
	delete timer_snd_;
	delete sb_;
	delete opt_;
	delete nonces_;
	delete [] feat_list_num_;
	delete [] feat_list_loc_;
	delete [] feat_list_seq_num_;
	delete [] feat_list_first_;
	delete [] feat_conf_num_;
	delete [] feat_conf_loc_;
}

/* Process a "function call" from OTCl
 * arg: argc - number of arguments
 *      argv - arguments
 * ret: TCL_OK if successful, TCL_ERROR otherwise
 */
int DCCPAgent::command(int argc, const char*const* argv){
	if (argc == 3) {
		if (strcmp(argv[1], "advance") == 0) {
			advanceby(atoi(argv[2]));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "advanceby") == 0) {
			advanceby(atoi(argv[2]));
			return (TCL_OK);
		}
	}
	return (Agent::command(argc, argv));
}

/* Receive a packet
 * arg: pkt     - Packet received
 *      handler - handler
 */
void DCCPAgent::recv(Packet* pkt, Handler* handler){
	int data_size = 0;
	bool ack_received = false;
	bool data_or_ack_pkt = false;
	bool processOptions = true;
	bool tell_cc = false;
	bool tell_app = false;
	hdr_dccp *dccph = hdr_dccp::access(pkt);
	hdr_dccpack *dccpah = hdr_dccpack::access(pkt);
	hdr_dccpreset *dccpresh = hdr_dccpreset::access(pkt);
	hdr_cmn *cmnh = hdr_cmn::access(pkt);
	output_ = false;
	output_flag_ = false;

	//check if packet is valid
	if (!checkPacket(pkt))
		goto free;

	//preprocess packet (if reset, reset the agent)
	switch (dccph->type_){
	case DCCP_REQUEST:
	case DCCP_RESPONSE:
		break;
	case DCCP_DATA:
	case DCCP_DATAACK:
		data_or_ack_pkt = true;
		data_size = cmnh->size() -  headerSize(dccph->type_)
			- dccph->options_->getSize();
		ack_received = (dccph->type_ == DCCP_DATAACK);
		break;
	case DCCP_ACK:
		data_or_ack_pkt = true;
		ack_received = true;
		break;
	case DCCP_CLOSEREQ:
	case DCCP_CLOSE:
		processOptions = false;
		break;
	case DCCP_RESET:
		debug("%f, DCCP(%s)::recv() - Received a reset packet, reason %s (%d), data (%d,%d,%d))\n",
		      now(), name(), resetReasonAsStr(dccpresh->rst_reason_), dccpresh->rst_reason_,
		      dccpresh->rst_data1_,dccpresh->rst_data2_,dccpresh->rst_data3_);
		ack_received = true;
		reset();
		goto free;
		break;
	default:
		fprintf(stdout, "%f, DCCP(%s)::recv() - Received an unknown packet (type %d) in state %s (%d)!\n",
			now(), name(), dccph->type_, stateAsStr(state_), state_);
		goto free;
	}
	debug("%f, DCCP(%s)::recv() - type %s, seq %d, ack %d, packet size %d, data_size is %d, cscov_ %d\n",now(), name(), packetTypeAsStr(dccph->type_),dccph->seq_num_, dccpah->ack_num_,cmnh->size(),data_size, dccph->cscov_);
	
	//process options (not on close packets)
	if (processOptions && !parseOptions(pkt)){
		debug("%f, DCCP(%s)::recv() - Parse option failed!)\n", now(), name());
		reset();
		goto free;
	}
	
	switch (state_){
	case DCCP_STATE_CLOSED:
		fprintf(stdout, "%f, DCCP(%s)::recv() - Received a %s (%d) packet in state %s!\n",
			now(), name(), packetTypeAsStr(dccph->type_),
			dccph->type_, stateAsStr(state_));
		if (dccph->type_ != DCCP_RESET){
			//ignore
		}
		goto free;
		break;
	case DCCP_STATE_LISTEN:
		if(dccph->type_ == DCCP_REQUEST){
			packet_recv_ = true;
			changeState(DCCP_STATE_RESPOND);
			//ack_num_ = seq_num_recv_;
			output_ = true;
			goto free_send;
		} else if (dccph->type_ != DCCP_RESET){
			//ignore
		}
		goto free;
		break;
	case DCCP_STATE_RESPOND:
		if(dccph->type_ == DCCP_REQUEST){
			//resend the RESPONSE
			output_ = true;
			goto free_send;
		} else if (dccph->type_ == DCCP_CLOSE) {
			sendReset(DCCP_RST_CLOSED,0,0,0);
			reset();
		} else if (ack_received && dccpah->ack_num_ == seq_num_-1) {
			rtt_conn_est_ = now() - rtt_conn_est_;
			debug("%f, DCCP(%s)::recv() - RTT estimated to %f s (server)!\n",now(), name(), rtt_conn_est_);
			if (data_size > 0){
				//the last ack in handshake contained data
				tell_cc = true;
				dccph->type_ = DCCP_DATA;
				if (use_ackv_local_ && manage_ackv_)
					ackv_->addPacket(dccph->seq_num_, getECNCodePoint(pkt), true);	
			} else if (use_ackv_local_ && manage_ackv_)
				ackv_->addPacket(dccph->seq_num_+1, DCCP_PACKET_NOT_RECV);
			tell_app = (!infinite_send_) && sb_->empty();
			conn_est_ = true;
			changeState(DCCP_STATE_OPEN);
			goto data;
		} else if (ack_received)
			debug("%f, DCCP(%s)::recv() - Invalid ack num %d to finalise the handshake\n",now(),name(), dccpah->ack_num_);
		goto free;
		break;
	case DCCP_STATE_OPEN:
		if (dccph->type_ == DCCP_CLOSE){
			sendReset(DCCP_RST_CLOSED, 0 ,0 ,0);
			reset();
		} else if (dccph->type_ == DCCP_CLOSEREQ){
			changeState(DCCP_STATE_CLOSING);
			output_ = true;
			goto free_send;
		} else if (data_or_ack_pkt) {
			if (use_ackv_local_ && manage_ackv_){
				//add packet to local ackvector
				if(!(ackv_->addPacket(dccph->seq_num_, getECNCodePoint(pkt), dccph->type_ != DCCP_ACK)))
					fprintf(stdout, "%f, DCCP(%s)::recv() - Add packet to ack vector failed!\n",now(), name());

				if (ack_received){
					ackv_->ackRecv(dccpah->ack_num_, ackv_recv_);
				}
			}
			tell_cc = true;
			goto data;
		}
		goto free;
		break;
	case DCCP_STATE_REQUEST:
		if (dccph->type_ == DCCP_RESPONSE){
			packet_recv_ = true;
			changeState(DCCP_STATE_OPEN);
			send_ack_ = true;
			ack_num_ = seq_num_recv_;
			output_ = true;
			rtt_conn_est_ = now() - rtt_conn_est_;
			debug("%f, DCCP(%s)::recv() - RTT estimated to %f s (client)!\n",now(), name(), rtt_conn_est_);
			if (use_ackv_local_ && manage_ackv_)
				ackv_->addPacket(dccph->seq_num_+1,DCCP_PACKET_NOT_RECV);
			goto free_send;
		} else if (dccph->type_ == DCCP_CLOSE){
			sendReset(DCCP_RST_CLOSED, 0 ,0 ,0);
			reset();
		} else if (dccph->type_ != DCCP_RESET){
			//ignore
		}
		goto free;
		break;
	case DCCP_STATE_CLOSEREQ:
	        if (dccph->type_ == DCCP_CLOSE) {
			sendReset(DCCP_RST_CLOSED,0,0,0);
			reset();
		} else if (data_or_ack_pkt)
			goto data;
		goto free;
		break;
	case DCCP_STATE_CLOSING:
		if (dccph->type_ == DCCP_CLOSEREQ){
			changeState(DCCP_STATE_CLOSING);  //i.e. reset timer
			output_ = true;
			goto free_send;
		} else if (data_or_ack_pkt)
			goto data;
		goto free;
		break;
	default:
		fprintf(stderr, "%f, DCCP(%s):recv() - Illegal state (%d)!\n",
			now(), name(),state_);
		fflush(stdout);
		abort();
	}
	
 data:
	if (data_or_ack_pkt) {
		if (tell_cc){
			recv_packetRecv(pkt,data_size);
			send_packetRecv(pkt,data_size);
		}
		if(data_size > 0){
			recvBytes(data_size);  //inform the application
		}
	}

	if (tell_app)
		idle();
 free_send:
	if (output_)
		output(output_flag_);
 free:
	//remove received options
	if (ackv_recv_ != NULL){
		delete ackv_recv_;
		ackv_recv_ = NULL;
	}
	elapsed_time_recv_ = 0;
		
	delete (dccph->options_);
	dccph->options_ = NULL;
	
	Packet::free(pkt);
}

/* Send a packet,
 * If this is the first packet to send, initiate a connection.
 * arg: nbytes - number of bytes in packet (-1 -> infinite send)
 *      flags  - Flags:
 *                 "MSG_EOF" will close the connection when all data
 *                  has been flag
 */
void DCCPAgent::sendmsg(int nbytes, const char *flags){
	if (infinite_send_)
		return;
	
	switch (state_){
	case DCCP_STATE_CLOSED:
	case DCCP_STATE_RESPOND:
	case DCCP_STATE_REQUEST:
	case DCCP_STATE_OPEN:
		if (nbytes > 0) {
			if (!(sb_->add(nbytes)))
				debug("%f, DCCP(%s)::sendmsg() - Sendbuffer is full\n", now(), name());	
		} else if (nbytes == 0) {
			fprintf(stdout, "%f, DCCP(%s)::sendmsg() - Application tries to send 0 byte of data\n",
			now(), name());
			return;
		} else {
			debug("%f, DCCP(%s)::sendmsg() - Infinite send activated\n",
			      now(), name());
			infinite_send_ = true;
		}

		if (flags != NULL && strcmp(flags, "MSG_EOF") == 0) 
			close_on_empty_ = TRUE;	
		
		if (state_ == DCCP_STATE_CLOSED) {
			//initiate a connection
			buildInitialFeatureList();
			changeState(DCCP_STATE_REQUEST);
			output();
		} else if (state_ == DCCP_STATE_OPEN)
			output();
		break;
	case DCCP_STATE_LISTEN:
	case DCCP_STATE_CLOSEREQ:
	case DCCP_STATE_CLOSING:
		break;
	default:
		fprintf(stderr, "%f, DCCP(%s)::sendmsg() - Illegal state (%d)!\n",
			now(), name(),state_);
		fflush(stdout);
		abort();
	}
}

/* Close the connection */
void DCCPAgent::close(){
	debug("%f, DCCP(%s)::close() - The application wants to close the connection (state %s)\n", now(), name(),stateAsStr(state_));
	switch(state_){
	case DCCP_STATE_CLOSED:
		fprintf(stdout, "%f, DCCP(%s)::close() - Already closed!\n",
			now(), name());	
		break;
	case DCCP_STATE_RESPOND:
		changeState(DCCP_STATE_CLOSEREQ);
		output();
		break;
	case DCCP_STATE_REQUEST:
		sendReset(DCCP_RST_CLOSED,0,0,0);
		reset();
		break;
	case DCCP_STATE_OPEN:
		if (server_)
			changeState(DCCP_STATE_CLOSEREQ);
		else
			changeState(DCCP_STATE_CLOSING);
		
		output();
		break;
	case DCCP_STATE_LISTEN:
		reset();
		changeState(DCCP_STATE_CLOSED);
		break;
	case DCCP_STATE_CLOSEREQ:
	case DCCP_STATE_CLOSING:
		fprintf(stdout, "%f, DCCP(%s)::close() - Application tried to close when in state %s\n",
			now(), name(), stateAsStr(state_));	
		break;
	default:
		fprintf(stderr, "%f, DCCP(%s)::sendmsg() - Illegal state (%d)!\n",
			now(), name(),state_);
		fflush(stdout);
		abort();
	}

}

/* Listen for incoming connections
 * Use close() to stop listen
 */
void DCCPAgent::listen(){
	if (state_ == DCCP_STATE_CLOSED){
		server_ = true;
		changeState(DCCP_STATE_LISTEN);
	} else
		debug("%f, DCCP(%s)::listen() - Listen called while in state %s (%d)\n",
		      now(), name(), stateAsStr(state_),state_);
		
}

/* A timeout has occured
 * arg: tno - id of timeout event
 */
void DCCPAgent::timeout(int tno){
	debug("%f, DCCP(%s)::timeout() - Timeout %d occured!\n",
		      now(), name(), tno);
	switch (tno){
	case DCCP_TIMER_RTX:
		switch(state_){
		case DCCP_STATE_RESPOND:
			sendReset(DCCP_RST_CLOSED,0,0,0);
			reset();
			break;
		case DCCP_STATE_REQUEST:
			if (timer_rtx_->backOffFailed()){
				sendReset(DCCP_RST_CLOSED,0,0,0);
				reset(); 
			} else
				output();
			break;
		case DCCP_STATE_CLOSING:
			if (timer_rtx_->backOffFailed()){
				reset();
			} else
				output();
			break;
		case DCCP_STATE_CLOSEREQ:
			if (timer_rtx_->backOffFailed()){
				sendReset(DCCP_RST_CLOSED,0,0,0);
				reset();
			} else
				output();
			break;
		default:
			fprintf(stderr,"%f, DCCP(%s)::timeout() - Timer (%d) should not be running in state %s!\n",
		      now(), name(), tno,stateAsStr(state_));
			fflush(stdout);
			abort();
		}
		break;
	case DCCP_TIMER_SND:
		switch(state_){
		case DCCP_STATE_OPEN:
			output();
			break;
		default:
			fprintf(stderr,"%f, DCCP(%s)::timeout() - Timer (%d) should not be running in state %s!\n",
				now(), name(), tno,stateAsStr(state_));
			fflush(stdout);
			abort();
		}
		break;
	default:
		fprintf(stdout,"%f, DCCP(%s)::timeout() - Unknown timeout occured (%d)!\n",
		      now(), name(), tno);
	}
}

/* Trace a variable.
 * arg: v  - traced variable
 */
void DCCPAgent::trace(TracedVar* v) {
	if (nam_tracevar_) {
		Agent::trace(v);
	} else if (trace_all_oneline_){
		traceAll();
	}else{ 
		traceVar(v);
	}
}

/* Send delta packets of packetSize_ size
 * arg: delta - number of packets to send
 */
void DCCPAgent::advanceby(int delta){
	if (delta <= 0)
		return;
	
	if (!infinite_send_){
		
		switch (state_){
		case DCCP_STATE_CLOSED:
		case DCCP_STATE_RESPOND:
		case DCCP_STATE_REQUEST:
		case DCCP_STATE_OPEN:
			
			while (!sb_->full() && delta > 0){
				sb_->add(size_);
				delta--;
			}
			if (sb_->full()){
				fprintf(stdout,"%f, DCCP(%s)::advanceby() - Sendbuffer is full (%d packets dropped)\n",
					now(), name(), delta);
			}
			if (state_ == DCCP_STATE_CLOSED) {
				//initiate a connection
				buildInitialFeatureList();
				changeState(DCCP_STATE_REQUEST);
				output();
			} else if (state_ == DCCP_STATE_OPEN)
				output();
		case DCCP_STATE_LISTEN:
		case DCCP_STATE_CLOSEREQ:
		case DCCP_STATE_CLOSING:
			break;
		default:
			fprintf(stderr, "%f, DCCP(%s)::advanceby() - Illegal state (%d)!\n",
				now(), name(),state_);
			fflush(stdout);
			abort();
			
		}
	}
}

