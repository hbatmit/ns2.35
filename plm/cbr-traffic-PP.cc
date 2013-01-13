/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * This code is a contribution of Arnaud Legout, Institut Eurecom, France.
 * This code is highly inspired from the code of the CBR sources.
 * The following copyright is the original copyright included in the 
 * cbr_traffic.cc file.
 *
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *  
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linking this file statically or dynamically with other modules is making
 * a combined work based on this file.  Thus, the terms and conditions of
 * the GNU General Public License cover the whole combination.
 *
 * In addition, as a special exception, the copyright holders of this file
 * give you permission to combine this file with free software programs or
 * libraries that are released under the GNU LGPL and with code included in
 * the standard release of ns-2 under the Apache 2.0 license or under
 * otherwise-compatible licenses with advertising requirements (or modified
 * versions of such code, with unchanged license).  You may copy and
 * distribute such a system following the terms of the GNU GPL for this
 * file and the licenses of the other code concerned, provided that you
 * include the source code of that other code when and as the GNU GPL
 * requires distribution of source code.
 *
 * Note that people who make modified versions of this file are not
 * obligated to grant this special exception for their modified versions;
 * it is their choice whether to do so.  The GNU General Public License
 * gives permission to release a modified version without this exception;
 * this exception also makes it possible to release a modified version
 * which carries forward this exception.
 */

#include <stdlib.h>
 
#include "random.h"
#include "trafgen.h"
#include "ranvar.h"


/* 
 * Constant bit rate traffic source.   Parameterized by interval, (optional)
 * random noise in the interval, and packet size.  
 */

class CBR_PP_Traffic : public TrafficGenerator {
 public:
	CBR_PP_Traffic();
	virtual double next_interval(int&);
	//HACK so that udp agent knows interpacket arrival time within a burst
	inline double interval() { return (interval_); }
 protected:
	virtual void start();
	void init();
	void timeout();
	double rate_;     /* send rate during on time (bps) */
	double interval_; /* packet inter-arrival time during burst (sec) */
	double random_;
	int seqno_;
	int maxpkts_;
	int PP_;
	int PBM_;         /*size of the packets bunch*/
};


static class CBR_PP_TrafficClass : public TclClass {
 public:
	CBR_PP_TrafficClass() : TclClass("Application/Traffic/CBR_PP") {}
	TclObject* create(int, const char*const*) {
		return (new CBR_PP_Traffic());
	}
} class_cbr_PP_traffic;

CBR_PP_Traffic::CBR_PP_Traffic() : seqno_(0)
{
	bind_bw("rate_", &rate_);
	bind("random_", &random_);
	bind("packetSize_", &size_);
	bind("maxpkts_", &maxpkts_);
	bind("PBM_", &PBM_);
}

void CBR_PP_Traffic::init()
{
        // compute inter-packet interval 
	 interval_ = PBM_*(double)(size_ << 3)/(double)rate_;
	//interval_ = 1e-100;
	PP_ = 0;
	if (agent_)
		agent_->set_pkttype(PT_CBR);
}

void CBR_PP_Traffic::start()
{
        init();
        running_ = 1;
        timeout();
}

double CBR_PP_Traffic::next_interval(int& size)
{
	// Recompute interval in case rate_ or size_ has changes
	if (PP_ >= (PBM_ - 1)){		
		interval_ = PBM_*(double)(size_ << 3)/(double)rate_;
		PP_ = 0;
	}
	else {
		interval_ = 1e-100;
		PP_ += 1 ;
	}
	double t = interval_;
	if (random_==1)
		t += interval_ * Random::uniform(-0.5, 0.5);	
	if (random_==2)
		t += interval_ * Random::uniform(-0.000001, 0.000001);	
	size = size_;
	if (++seqno_ < maxpkts_)
		return(t);
	else
		return(-1); 
}

void CBR_PP_Traffic::timeout()
{
        if (! running_)
                return;

        /* send a packet */
        // The test tcl/ex/test-rcvr.tcl relies on the "NEW_BURST" flag being 
        // set at the start of any exponential burst ("talkspurt").  
        if (PP_ == 0) 
                agent_->sendmsg(size_, "NEW_BURST");
        else 
                agent_->sendmsg(size_);

        /* figure out when to send the next one */
        nextPkttime_ = next_interval(size_);
        /* schedule it */
        if (nextPkttime_ > 0)
                timer_.resched(nextPkttime_);
}

