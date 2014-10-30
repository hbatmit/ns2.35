/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/tools/expoo.cc,v 1.15 2005/08/26 05:05:30 tomh Exp $ (Xerox)";
#endif

#include <stdlib.h>
 
#include "random.h"
#include "trafgen.h"
#include "ranvar.h"


/* implement an on/off source with exponentially distributed on and
 * off times.  parameterized by average burst time, average idle time,
 * burst rate and packet size.
 */

class EXPOO_Traffic : public TrafficGenerator {
 public:
	EXPOO_Traffic();
	virtual double next_interval(int&);
        virtual void timeout();
	// Added by Debojyoti Dutta October 12th 2000
	int command(int argc, const char*const* argv);
 protected:
	void init();
	double ontime_;   /* average length of burst (sec) */
	double offtime_;  /* average length of idle time (sec) */
	double rate_;     /* send rate during on time (bps) */
	double interval_; /* packet inter-arrival time during burst (sec) */
	unsigned int rem_; /* number of packets left in current burst */

	/* new stuff using RandomVariable */
	ExponentialRandomVariable burstlen_;
	ExponentialRandomVariable Offtime_;

};


static class EXPTrafficClass : public TclClass {
 public:
	EXPTrafficClass() : TclClass("Application/Traffic/Exponential") {}
	TclObject* create(int, const char*const*) {
		return (new EXPOO_Traffic());
	}
} class_expoo_traffic;

// Added by Debojyoti Dutta October 12th 2000
// This is a new command that allows us to use 
// our own RNG object for random number generation
// when generating application traffic

int EXPOO_Traffic::command(int argc, const char*const* argv){
        
        if(argc==3){
                if (strcmp(argv[1], "use-rng") == 0) {
                        burstlen_.seed((char *)argv[2]);
                        Offtime_.seed((char *)argv[2]);
                        return (TCL_OK);
                }
        }
        return Application::command(argc,argv);
}


EXPOO_Traffic::EXPOO_Traffic() : burstlen_(0.0), Offtime_(0.0)
{
	bind_time("burst_time_", &ontime_);
	bind_time("idle_time_", Offtime_.avgp());
	bind_bw("rate_", &rate_);
	bind("packetSize_", &size_);
}

void EXPOO_Traffic::init()
{
        /* compute inter-packet interval during bursts based on
	 * packet size and burst rate.  then compute average number
	 * of packets in a burst.
	 */
	interval_ = (double)(size_ << 3)/(double)rate_;
	burstlen_.setavg(ontime_/interval_);
	rem_ = 0;
	if (agent_)
		agent_->set_pkttype(PT_EXP);
}

double EXPOO_Traffic::next_interval(int& size)
{
	double t = interval_;

	if (rem_ == 0) {
		/* compute number of packets in next burst */
		rem_ = int(burstlen_.value() + .5);
		/* make sure we got at least 1 */
		if (rem_ == 0)
			rem_ = 1;
		/* start of an idle period, compute idle time */
		t += Offtime_.value();
	}	
	rem_--;

	size = size_;
	return(t);
}

void EXPOO_Traffic::timeout()
{
	if (! running_)
		return;

	/* send a packet */
	// The test tcl/ex/test-rcvr.tcl relies on the "NEW_BURST" flag being 
	// set at the start of any exponential burst ("talkspurt").  
	if (nextPkttime_ != interval_ || nextPkttime_ == -1) 
		agent_->sendmsg(size_, "NEW_BURST");
	else 
		agent_->sendmsg(size_);
	/* figure out when to send the next one */
	nextPkttime_ = next_interval(size_);
	/* schedule it */
	if (nextPkttime_ > 0)
		timer_.resched(nextPkttime_);
}



