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
 * @(#) $Header: /cvsroot/nsnam/ns-2/xcp/xcpq.h,v 1.12 2006/07/01 19:40:04 tom_henderson Exp $
 */


#ifndef NS_XCPQ_H
#define NS_XCPQ_H

#include "drop-tail.h"
#include "packet.h"
#include "xcp-end-sys.h"

#define  INITIAL_Te_VALUE   0.05       // Was 0.3 Be conservative when 
                                       // we don't kow the RTT

#define TRACE  1                       // when 0, we don't race or write 
                                       // var to disk

class XCPWrapQ;
class XCPQueue;

class XCPTimer : public TimerHandler { 
public:
	XCPTimer(XCPQueue *a, void (XCPQueue::*call_back)() ) 
		: a_(a), call_back_(call_back) {};
protected:
	virtual void expire (Event *e);
	XCPQueue *a_; 
	void (XCPQueue::*call_back_)();
}; 


class XCPQueue : public DropTail {
	friend class XCPTimer;
public:
	XCPQueue();
	void Tq_timeout ();  // timeout every propagation delay 
	void Te_timeout ();  // timeout every avg. rtt
	void everyRTT();     // timeout every highest rtt seen by rtr or some
	                     // preset rtt value
	void setupTimers();  // setup timers for xcp queue only
	void setEffectiveRtt(double rtt) ;
	void routerId(XCPWrapQ* queue, int i);
	int routerId(int id = -1); 
  
	int limit(int len = 0);
	void setBW(double bw);
	void setChannel(Tcl_Channel queue_trace_file);
	double totalDrops() { return total_drops_; }
  
        // Overloaded functions
	void enque(Packet* pkt);
	Packet* deque();
	virtual void drop(Packet* p);
  
	// tracing var
	void setNumMice(int mice) {num_mice_ = mice;}

protected:

	// Utility Functions
	double max(double d1, double d2) { return (d1 > d2) ? d1 : d2; }
	double min(double d1, double d2) { return (d1 < d2) ? d1 : d2; }
        int max(int i1, int i2) { return (i1 > i2) ? i1 : i2; }
	int min(int i1, int i2) { return (i1 < i2) ? i1 : i2; }
	double abs(double d) { return (d < 0) ? -d : d; }

	virtual void trace_var(char * var_name, double var);
  
	// Estimation & Control Helpers
	void init_vars();
	
	// called in enque, but packet may be dropped; used for 
	// updating the estimation helping vars such as
	// counting the offered_load_, sum_rtt_by_cwnd_
	virtual void do_on_packet_arrival(Packet* pkt);

	// called in deque, before packet leaves
	// used for writing the feedback in the packet
	virtual void do_before_packet_departure(Packet* p); 
	
  
	// ---- Variables --------
	unsigned int     routerId_;
	XCPWrapQ*        myQueue_;   //pointer to wrapper queue lying on top
	XCPTimer*        queue_timer_;
	XCPTimer*        estimation_control_timer_;
	XCPTimer*        rtt_timer_;
	double           link_capacity_bps_;

	static const double	ALPHA_;
	static const double	BETA_;
	static const double	GAMMA_;
	static const double	XCP_MAX_INTERVAL;
	static const double	XCP_MIN_INTERVAL;

	double          Te_;       // control interval
	double          Tq_;    
	double          Tr_;
	double          avg_rtt_;       // average rtt of flows
	double          high_rtt_;      // highest rtt seen in flows
	double          effective_rtt_; // pre-set rtt value 
	double          Cp_;
	double          Cn_;
	double          residue_pos_fbk_;
	double          residue_neg_fbk_;
	double          queue_bytes_;   // our estimate of the fluid model queue
	double          input_traffic_bytes_;       // traffic in Te 
	double          sum_rtt_by_throughput_;
	double          sum_inv_throughput_;
	double          running_min_queue_bytes_;
	unsigned int    num_cc_packets_in_Te_;
  
	double		thruput_elep_;
	double		thruput_mice_;
	double		total_thruput_;
	int		num_mice_;
	int		min_queue_ci_;
	int		max_queue_ci_;
	// drops
	int 		drops_;
	double		total_drops_ ;
  
	// ----- For Tracing Vars --------------//
	Tcl_Channel 	queue_trace_file_;
  
};


#endif //NS_XCPQ_H
