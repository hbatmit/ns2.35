/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1990-1997 Regents of the University of California.
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
 * @(#) $Header: /cvsroot/nsnam/ns-2/tools/flowmon.h,v 1.2 2004/01/18 19:51:20 haldar Exp $ (LBL)
 */

/* File pulled out of flowmon.cc 
 * Hopefully nothing would break as a result
 * -ratul
 */

#ifndef ns_flowmon_h
#define ns_flowmon_h

#include <stdlib.h>
#include "config.h"
#include "queue-monitor.h"
#include "classifier.h"
#include "ip.h"
#include "flags.h"
#include "random.h"

class Flow : public EDQueueMonitor {
public:
	Flow() : src_(-1), dst_(-1), fid_(-1), type_(PT_NTYPE) {  

		bind("src_", (int *) &src_);
		bind("dst_", (int *) &dst_);
		bind("flowid_", (int *) &fid_);
	}
	nsaddr_t src() const { return (src_); }
	nsaddr_t dst() const { return (dst_); }
	int flowid() const { return (fid_); }
	packet_t ptype() const { return (type_); }
	void setfields(Packet *p) {
		hdr_ip* hdr = hdr_ip::access(p);
		hdr_cmn* chdr = hdr_cmn::access(p);
		src_ = hdr->saddr();
		dst_ = hdr->daddr();
		fid_ = hdr->flowid();
		type_ = chdr->ptype();
	}
	virtual void tagging(Packet *) {}

protected:
	nsaddr_t	src_;
	nsaddr_t	dst_;
	int		fid_;
	packet_t	type_;

};

class TaggerTBFlow : public Flow {
public:
	TaggerTBFlow() : target_rate_(0.0), time_last_sent_(0.0),
			total_in(0.0), total_out(0.0)
	{
		bind_bw("target_rate_", &target_rate_);
		bind("bucket_depth_", &bucket_depth_);
		bind("tbucket_", &tbucket_);
		//		bind("off_flags_", &off_flags_);
	}
	void tagging(Packet *p) {
	    hdr_cmn* hdr = hdr_cmn::access(p);
            double now = Scheduler::instance().clock();
            double time_elapsed;

            time_elapsed      = now - time_last_sent_;
            tbucket_ += time_elapsed * target_rate_ / 8.0;
            if (tbucket_ > bucket_depth_)
                tbucket_ = bucket_depth_;         /* never overflow */

      	    if ((double)hdr->size_ < tbucket_ ) {
		    //	((hdr_flags*)p->access(off_flags_))->pri_=1; //Tag the packet as In.
		    (hdr_flags::access(p))->pri_=1;
		    tbucket_ -= hdr->size_;
                total_in += 1;
            }
      	    else {
                total_out += 1;
      	    }
	    time_last_sent_ = now;
	}
protected:
	/* User defined parameters */
	double	target_rate_;		//predefined flow rate in bytes/sec
	double	bucket_depth_;		//depth of the token bucket

	double 	tbucket_;
	/* Dynamic state variables */
	double 	time_last_sent_;
	double	total_in;
	double	total_out;
	//	int 	off_flags_;
};

/* TaggerTSWFlow will use Time Slide Window to check whehter the data flow 
 * stays in the pre-set profile and mark it as In or Out accordingly.
 * By Yun Wang, based on Wenjia's algorithm.
 */
class TaggerTSWFlow : public Flow {
public:
	TaggerTSWFlow() : target_rate_(0.0), avg_rate_(0.0), 
			t_front_(0.0), total_in(0.0), total_out(0.0) 
	{
		bind_bw("target_rate_", &target_rate_);
		bind("win_len_", &win_len_);
		bind_bool("wait_", &wait_);
		//		bind("off_flags_", &off_flags_);
	}
	void tagging(Packet *);
	void run_rate_estimator(Packet *p, double now){

	        hdr_cmn* hdr = hdr_cmn::access(p);
		double bytes_in_tsw = avg_rate_ * win_len_;
		double new_bytes    = bytes_in_tsw + hdr->size_;
		avg_rate_ = new_bytes / (now - t_front_ + win_len_);
		t_front_  = now;
	}
protected:
	/* User-defined parameters. */
	double	target_rate_;		//predefined flow rate in bytes/sec
	double	win_len_;		//length of the slide window
	double	avg_rate_;		//average rate
	double	t_front_;
	int	count;
	int	wait_;

	/* Counters for In/Out packets. */
	double 	total_in;
	double 	total_out;
	//	int	off_flags_;
};

/* Tagger performes like the queue monitor with a classifier
 * to demux by flow and mark the packets based on the flow profile
 */
class Tagger : public EDQueueMonitor {
public:
	Tagger() : classifier_(NULL), channel_(NULL) {}
	void in(Packet *);
	int command(int argc, const char*const* argv);
protected:
        void    dumpflows();
        void    dumpflow(Tcl_Channel, Flow*);
        void    fformat(Flow*);
        char*   flow_list();

        Classifier*     classifier_;
        Tcl_Channel     channel_;

        char    wrk_[2048];     // big enough to hold flow list
};


/*
 * flow monitoring is performed like queue-monitoring with
 * a classifier to demux by flow
 * ---------------------------------------------------------
 * mon_* stuff added to support monitored early drops - ratul
 */

class FlowMon : public EDQueueMonitor {
public:
	FlowMon();
	void in(Packet*);	// arrivals
	void out(Packet*);	// departures
	void drop(Packet*);	// all drops (incl 
	void edrop(Packet*);	// "early" drops
	void mon_edrop(Packet*);	// " monitored early" drops
	int command(int argc, const char*const* argv);

	//added by ratul
	void setClassifier(Classifier * classifier) {
	  classifier_ = classifier;
	}

	Flow * find(Packet* p) {
		return (Flow *)classifier_->find(p);
	}

protected:
	void	dumpflows();
	void	dumpflow(Tcl_Channel, Flow*);
	void	fformat(Flow*);
	char*	flow_list();

	Classifier*	classifier_;
	Tcl_Channel	channel_;

	int enable_in_;		// enable per-flow arrival state
	int enable_out_;	// enable per-flow depart state
	int enable_drop_;	// enable per-flow drop state
	int enable_edrop_;	// enable per-flow edrop state
	int enable_mon_edrop_;  // enable per-flow mon_edrop state 
	
	//an excessive high value for large simulations using flow monitor 
	char	wrk_[65536];	// big enough to hold flow list
};

#endif
