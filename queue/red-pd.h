/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 2000  International Computer Science Institute
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
 *	This product includes software developed by ACIRI, the AT&T 
 *      Center for Internet Research at ICSI (the International Computer
 *      Science Institute).
 * 4. Neither the name of ACIRI nor of ICSI may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICSI AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ICSI OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/queue/red-pd.h,v 1.4 2001/01/10 23:30:14 sfloyd Exp $ (ACIRI)
 */


#ifndef ns_red_pd_h
#define ns_red_pd_h

#include "red.h"
#include "flowmon.h"

class REDQueue;
class RedPDFlow;

class RedPDQueue : public REDQueue {
 public:	
	RedPDQueue (const char * = "Drop", const char * = "Drop");

	int auto_;              // boolean to decide if automatic updates to rate estimates 
	                                         // are required
	int global_target_;     // boolean to decide if we have the same targetBW_ 
	                                         // for all the monitored flows
	double targetBW_;       // the global targetBW_ in bps
	int noMonitored_;       // number of monitored flows
	double unresponsive_penalty_;  //multiplicative penalty factor for flows marked unresponsive
	                               // they get dropped with probability $prob*unresponsive_penalty_
	
	double P_testFRp_;      // to test the FRP thing
	int noidle_;		// boolean to decide if unresponsive flows
				//   should be dropped when queue is idle
 
	void setFlowMon(FlowMon * flowMon) {
		flowMonitor_ = flowMon;
	}

protected:
	int command(int argc, const char*const* argv);
	void reset();
	void enque(Packet* pkt);

	int off_ip_;

	FlowMon* flowMonitor_;        // the flowMonitor_ associated with the queue
	char medTraceType[20];        //the type of trace object for mon early drops
	NsObject * MEDTrace;          //the trace object for mon early drops

	double getP_monFlow(double current, double target) {
		if (current <= 0 || current < target) 
			return 0; //means don't drop
		// the surviving probability is target/current
		return 1 - (target/current);
	}		
};
 
class RedPDFlow : public Flow {

public:
	//default values - no drops
	double targetBW_;                  // the target BW of this flow in bps
	double currentBW_;                 // the current BW of the flow  in bps
	int monitored_;                    // boolean: whether this flow is being monitored
	int unresponsive_;                 // boolean: whether this flow is responsive
	
	double monitorStartTime_;          // time when we started monitoring this flow
	double unresponsiveStartTime_;     // time when we declared this flow as unresponsive 
	double lastDropTime_;              // time when the last packet from this flow was dropped 
	                                   // actually the time when the currentBW_ exceeded targetBW_

	int count;                         // number of packets since last drop
	int auto_;                         // boolean: if rate estmation is going on

	RedPDFlow() : targetBW_(0), currentBW_(0),  
		monitored_(0), unresponsive_(0),
		monitorStartTime_(0), unresponsiveStartTime_(0), lastDropTime_(0), 
		count(0), auto_(0) {
		
		bind_bw("currentBW_", &currentBW_);
		bind_bw("targetBW_", &targetBW_);

		bind_bool("monitored_", &monitored_);
		bind_bool("unresponsive_", &unresponsive_);

		bind("lastDropTime_", &lastDropTime_);
		bind("monitorStartTime_", &monitorStartTime_);
		bind("unresponsiveStartTime_", &unresponsiveStartTime_);

		bind_bool("auto_", &auto_);
	}

	int monitored() {return monitored_; };
	int unresponsive() {return unresponsive_; };
	
	RedPDFlow(double target, double current) {
		targetBW_ = target;
		currentBW_ = current;
	};

	void set(double target, double current) {
		targetBW_ = target;
		currentBW_ = current;
		monitored_ = 1;
	};
	
	double getP_monFLow() {
		if (currentBW_ <= 0 || currentBW_ < targetBW_) 
			return 0; //means don't drop
		
		// the surviving probability is target/current
		return 1 - (targetBW_/currentBW_);
	};

};


#endif
