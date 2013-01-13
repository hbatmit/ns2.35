/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
 *
 * Copyright (C) 2004 by USC/ISI
 *               2002 by Dina Katabi
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation, advertising
 * materials, and other materials related to such distribution and use
 * acknowledge that the software was developed by the University of
 * Southern California, Information Sciences Institute.  The name of the
 * University may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include "xcpq.h"
#include "xcp.h"
#include "random.h"

const double     XCPQueue::ALPHA_          = 0.4;
const double     XCPQueue::BETA_           = 0.226;
const double     XCPQueue::GAMMA_          = 0.1;
const double     XCPQueue::XCP_MAX_INTERVAL= 1.0;
const double     XCPQueue::XCP_MIN_INTERVAL= .001; 

static class XCPQClass : public TclClass {
public:
	XCPQClass() : TclClass("Queue/DropTail/XCPQ") {}
	TclObject* create(int, const char*const*) {
		return (new XCPQueue);
	}
} class_droptail_xcpq;


XCPQueue::XCPQueue(): queue_timer_(NULL), 
		      estimation_control_timer_(NULL),
		      rtt_timer_(NULL), effective_rtt_(0.0)
{
	init_vars();
}

void XCPQueue::setupTimers()
{
	queue_timer_ = new XCPTimer(this, &XCPQueue::Tq_timeout);
	estimation_control_timer_ = new XCPTimer(this, &XCPQueue::Te_timeout);
	rtt_timer_ = new XCPTimer(this, &XCPQueue::everyRTT);

	// Scheduling timers randomly so routers are not synchronized
	double T;
  
	T = max(0.004, Random::normal(Tq_, 0.2 * Tq_));
	queue_timer_->sched(T);
	
	T = max(0.004, Random::normal(Te_, 0.2 * Te_));
	estimation_control_timer_->sched(T);

	T = max(0.004, Random::normal(Tr_, 0.2 * Tr_));
	rtt_timer_->sched(T);
}


void XCPQueue::routerId(XCPWrapQ* q, int id)
{
	if (id < 0 && q == 0)
		fprintf(stderr, "XCP:invalid routerId and queue\n");
	routerId_ = id;
	myQueue_ = q;
}

int XCPQueue::routerId(int id)
{
	if (id > -1) 
		routerId_ = id;
	return ((int)routerId_); 
}

int XCPQueue::limit(int qlim)
{
	if (qlim > 0) 
		qlim_ = qlim;
	return (qlim_);
}

void XCPQueue::setBW(double bw)
{
	if (bw > 0) 
		link_capacity_bps_ = bw;
}

void XCPQueue::setChannel(Tcl_Channel queue_trace_file)
{
	queue_trace_file_ = queue_trace_file;
}

Packet* XCPQueue::deque()
{
	double inst_queue = byteLength();
/* L 32 */
	if (inst_queue < running_min_queue_bytes_) 
		running_min_queue_bytes_= inst_queue;
  
	Packet* p = DropTail::deque();
	do_before_packet_departure(p);
  
	max_queue_ci_ = max(length(), max_queue_ci_);
	min_queue_ci_ = min(length(), min_queue_ci_);
  
	return (p);
}

void XCPQueue::enque(Packet* pkt)
{
	max_queue_ci_ = max(length(), max_queue_ci_);
	min_queue_ci_ = min(length(), min_queue_ci_);

	do_on_packet_arrival(pkt);
	DropTail::enque(pkt);
}

void XCPQueue::do_on_packet_arrival(Packet* pkt){
  
	double pkt_size = double(hdr_cmn::access(pkt)->size());

	/* L 1 */
	input_traffic_bytes_ += pkt_size;

	hdr_xcp *xh = hdr_xcp::access(pkt); 

	if (xh->xcp_enabled_ != hdr_xcp::XCP_ENABLED)
		return;	// Estimates depend only on Forward XCP Traffic
      
	++num_cc_packets_in_Te_;
  
	if (xh->rtt_ != 0.0) {
		/* L 2 */
		sum_inv_throughput_ += xh->x_;
		/* L 3 */
		if (xh->rtt_ < XCP_MAX_INTERVAL) {
			/* L 4 */
			double y = xh->rtt_ * xh->x_;
			sum_rtt_by_throughput_ += y;
			/* L 5 */
		} else {
			/* L 6 */
			double y = XCP_MAX_INTERVAL * xh->x_;
			sum_rtt_by_throughput_ += y;
		}
	}
}


void XCPQueue::do_before_packet_departure(Packet* p)
{
	if (!p) return;
  
	hdr_xcp *xh = hdr_xcp::access(p);
    
	if (xh->xcp_enabled_ != hdr_xcp::XCP_ENABLED)
		return;
	if (xh->rtt_ == 0.0) {
		xh->delta_throughput_ = 0;
		return;
	}

	double pkt_size = double(hdr_cmn::access(p)->size());

	/* L 20, 21 */
	double pos_fbk = Cp_ * xh->x_;
	double neg_fbk = Cn_ * pkt_size;

	pos_fbk = min(residue_pos_fbk_, pos_fbk);

	neg_fbk = min(residue_neg_fbk_, neg_fbk);

	/* L 22 */
	double feedback = pos_fbk - neg_fbk;
	 
	/* L 23 */	
	if (xh->delta_throughput_ >= feedback) {
		/* L 24 */
		xh->delta_throughput_ = feedback;
		xh->controlling_hop_ = routerId_;
		/* L 25 */
	} else {
		/* L 26 */
		neg_fbk = min(residue_neg_fbk_, neg_fbk + (feedback - xh->delta_throughput_));
		/* L 27 */
		pos_fbk = xh->delta_throughput_ + neg_fbk;
	}
	/* L 28, L 29 */
	residue_pos_fbk_ = max(0.0, residue_pos_fbk_ - pos_fbk);
	residue_neg_fbk_ = max(0.0, residue_neg_fbk_ - neg_fbk);
	/* L 30 */
	if (residue_pos_fbk_ == 0.0)
		Cp_ = 0.0;
	/* L 31 */
	if (residue_neg_fbk_ == 0.0)
		Cn_ = 0.0;
  
	if (TRACE && (queue_trace_file_ != 0 )) {
		trace_var("pos_fbk", pos_fbk);
		trace_var("neg_fbk", neg_fbk);
		trace_var("delta_throughput", xh->delta_throughput_);
		int id = hdr_ip::access(p)->flowid();
		char buf[25];
		sprintf(buf, "X%d",id);
		trace_var(buf, xh->x_);
		
		// tracing measured thruput info
		if (xh->rtt_ > high_rtt_)
			high_rtt_ = xh->rtt_;
		total_thruput_ += pkt_size;
		
		if (num_mice_ != 0) {
			if (id >= num_mice_) 
				thruput_elep_ += pkt_size;
			else 
				thruput_mice_ += pkt_size;
		}
	}
}


void XCPQueue::Tq_timeout()
{
	double inst_queue = byteLength();
	/* L 33 */
	queue_bytes_ = running_min_queue_bytes_;
	/* L 34 */
	running_min_queue_bytes_ = inst_queue;
	/* L 35 */
	Tq_ = max(0.002, (avg_rtt_ - inst_queue/link_capacity_bps_)/2.0); 
	/* L 36 */
	queue_timer_->resched(Tq_);
	
	if (TRACE && (queue_trace_file_ != 0)) {
		trace_var("Tq_", Tq_);
		trace_var("queue_bytes_", queue_bytes_);
		trace_var("routerId_", routerId_);
	}
}

void XCPQueue::Te_timeout()
{

	if (TRACE && (queue_trace_file_ != 0)) {
		trace_var("residue_pos_fbk_not_allocated", residue_pos_fbk_);
		trace_var("residue_neg_fbk_not_allocated", residue_neg_fbk_);
	}

	/* L 8 */
	double input_bw = input_traffic_bytes_ / Te_;
	double phi_bps = 0.0;
	double shuffled_traffic_bps = 0.0;

	if (sum_inv_throughput_ != 0.0) {
/* L 7 */
		avg_rtt_ = sum_rtt_by_throughput_ / sum_inv_throughput_;
	} else
		avg_rtt_ = INITIAL_Te_VALUE;

	
	if (input_traffic_bytes_ > 0) {
/* L 9 */
		phi_bps = ALPHA_ * (link_capacity_bps_- input_bw) 
			- BETA_ * queue_bytes_ / avg_rtt_;
/* L 10 */
		shuffled_traffic_bps = GAMMA_ * input_bw;

		if (shuffled_traffic_bps > abs(phi_bps))
			shuffled_traffic_bps -= abs(phi_bps);
		else
			shuffled_traffic_bps = 0.0;
/* L 10 ends here */
	}
/* L 11, 12 */	
	residue_pos_fbk_ = max(0.0,  phi_bps) + shuffled_traffic_bps;
	residue_neg_fbk_ = max(0.0, -phi_bps) + shuffled_traffic_bps;

	if (sum_inv_throughput_ == 0.0)
		sum_inv_throughput_ = 1.0;
	if (input_traffic_bytes_ > 0) {
/* L 13 */
		Cp_ =  residue_pos_fbk_ / sum_inv_throughput_;
/* L 14 */
		Cn_ =  residue_neg_fbk_ / input_traffic_bytes_;
	} else 
		Cp_ = Cn_ = 0.0;
		
	if (TRACE && (queue_trace_file_ != 0)) {
		trace_var("input_traffic_bytes_", input_traffic_bytes_);
		trace_var("avg_rtt_", avg_rtt_);
		trace_var("residue_pos_fbk_", residue_pos_fbk_);
		trace_var("residue_neg_fbk_", residue_neg_fbk_);
		//trace_var("Qavg", edv_.v_ave);
		trace_var("Qsize", length());
		trace_var("min_queue_ci_", double(min_queue_ci_));
		trace_var("max_queue_ci_", double(max_queue_ci_));

		trace_var("routerId", routerId_);
	} 
	num_cc_packets_in_Te_ = 0;

/* L 15 */  
	input_traffic_bytes_ = 0.0;
/* L 16 */
	sum_inv_throughput_ = 0.0;
/* L 17 */
	sum_rtt_by_throughput_ = 0.0;
/* L 18 */
	Te_ = max(avg_rtt_, XCP_MIN_INTERVAL);

/* L 19 */
	estimation_control_timer_->resched(Te_);

	min_queue_ci_ = max_queue_ci_ = length();
}


void XCPQueue::everyRTT ()
{
	if (effective_rtt_ != 0.0)
		Tr_ = effective_rtt_;
	else 
		if (high_rtt_ != 0.0)
			Tr_ = high_rtt_;

	// measure drops, if any
	trace_var("d", drops_);
	drops_=0;

	// sample the current queue size
	trace_var("q",length());
  
	// Update utilization 
	trace_var("u", total_thruput_/(Tr_*link_capacity_bps_));
	trace_var("u_elep", thruput_elep_/(Tr_*link_capacity_bps_));
	trace_var("u_mice", thruput_mice_/(Tr_*link_capacity_bps_));
	total_thruput_ = 0;
  
	rtt_timer_->resched(Tr_);
}


void  XCPQueue::drop(Packet* p)
{
	drops_++;
	total_drops_++;
  
	Connector::drop(p);
}

void  XCPQueue::setEffectiveRtt(double rtt)
{
	effective_rtt_ = rtt;
	rtt_timer_->resched(effective_rtt_);
}

// Estimation & Control Helpers

void XCPQueue::init_vars() 
{
	link_capacity_bps_	= 0.0;
	avg_rtt_		= INITIAL_Te_VALUE;
	Te_			= INITIAL_Te_VALUE;
  	Tq_			= INITIAL_Te_VALUE; 
	Tr_                     = 0.1;
	high_rtt_               = 0.0;
	Cp_			= 0.0;
	Cn_			= 0.0;     
	residue_pos_fbk_	= 0.0;
	residue_neg_fbk_	= 0.0;     
	queue_bytes_		= 0.0; // our estimate of the fluid model queue
	  
	input_traffic_bytes_	= 0.0; 
	sum_rtt_by_throughput_	= 0.0;
	sum_inv_throughput_	= 0.0;
	running_min_queue_bytes_= 0;
	num_cc_packets_in_Te_   = 0;
  
	queue_trace_file_ = 0;
	myQueue_ = 0;
  
	min_queue_ci_ = max_queue_ci_ = length();
  
	// measuring drops
	drops_ = 0;
	total_drops_ = 0;

	// utilisation
	num_mice_ = 0;
	thruput_elep_ = 0.0;
	thruput_mice_ = 0.0;
	total_thruput_ = 0.0;
}


void XCPTimer::expire(Event *) { 
	(*a_.*call_back_)();
}


void XCPQueue::trace_var(char * var_name, double var)
{
	char wrk[500];
	double now = Scheduler::instance().clock();

	if (queue_trace_file_) {
		int n;
		sprintf(wrk, "%s %g %g",var_name, now, var);
		n = strlen(wrk);
		wrk[n] = '\n'; 
		wrk[n+1] = 0;
		(void)Tcl_Write(queue_trace_file_, wrk, n+1);
	}
	return; 
}


