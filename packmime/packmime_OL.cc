/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/* 
 * Copyright 2002, Statistics Research, Bell Labs, Lucent Technologies and
 * The University of North Carolina at Chapel Hill
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright 
 * notice, this list of conditions and the following disclaimer in the 
 * documentation and/or other materials provided with the distribution.
 *    3. The name of the author may not be used to endorse or promote 
 * products derived from this software without specific prior written 
 * permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Reference
 *     Stochastic Models for Generating Synthetic HTTP Source Traffic 
 *     J. Cao, W.S. Cleveland, Y. Gao, K. Jeffay, F.D. Smith, and M.C. Weigle 
 *     IEEE INFOCOM 2004.
 *
 * Documentation available at http://dirt.cs.unc.edu/packmime/
 * 
 * Contacts: Michele Weigle (mcweigle@cs.unc.edu),
 *           Kevin Jeffay (jeffay@cs.unc.edu)
 */

/*
#include <sys/types.h>
#include <sys/stat.h> 
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "config.h"
#include "random.h"
#include "trafgen.h"
#include "ranvar.h"
*/
#include "packmime_OL.h"
#include "packmime_ranvar.h"

/* 
 * Constant bit rate traffic source.   Parameterized by interval, (optional)
 * random noise in the interval, and packet size.  
 */

/*
class PackMimeOpenLoop : public TrafficGenerator {
 public:
	PackMimeOpenLoop();
	virtual double next_interval(int&);
	//HACK so that udp agent knows interpacket arrival time within a burst	
	int command(int argc, const char*const* argv);
 
	
 protected:
	virtual void start();		
	void init();
	int size_;
	int seqno_;
	int64_t maxpkts_;	
	
	// statistics objects
	RandomVariable* pm_pac_ia_;
	RandomVariable* pm_pac_size_;	

	// helper methods
	TclObject* lookup_obj(const char* name) {
                TclObject* obj = Tcl::instance().lookup(name);
                if (obj == NULL) 
                        fprintf(stderr, "Bad object name %s\n", name);
                return obj;
        }

	inline int lookup_rv (RandomVariable*& rv, const char* name) {
		if (rv != NULL)
			Tcl::instance().evalf ("delete %s", rv->name());
		rv = (RandomVariable*) lookup_obj (name);
		return rv ? (TCL_OK) : (TCL_ERROR);
	}
};
*/

static class PackMimeOpenLoopClass : public TclClass {
 public:
	PackMimeOpenLoopClass() : TclClass("Application/Traffic/PackMimeOpenLoop") {}
	TclObject* create(int, const char*const*) {
		return (new PackMimeOpenLoop());
	}
} class_bl_traffic;

PackMimeOpenLoop::PackMimeOpenLoop() : size_(40), seqno_(0), maxpkts_(0), 
				       pm_pac_ia_(NULL), pm_pac_size_(NULL)
{	
	bind("maxpkts_", &maxpkts_);
}

void PackMimeOpenLoop::init()
{
	if (agent_)
		agent_->set_pkttype(PT_BLTRACE);
	else 
		printf("no agent_\n");
	
	return;
}

void PackMimeOpenLoop::start()
{		
	init();
	
	if (!pm_pac_ia_ || !pm_pac_size_) {
		fprintf(stderr, "Random variables are not set up yet!\n");
		return;
	}
        running_ = 1;	
	nextPkttime_ = next_interval(size_);	
	timer_.resched(nextPkttime_);       	
	
	return;
}

double PackMimeOpenLoop::next_interval(int& size)
{		
	double t = 1;
	
	size = (int)pm_pac_size_->value(); // current packet size
	assert(size>=40);
	size -= 28;	// length of IP + UDP header	
	t = pm_pac_ia_->value();   // time to next packet arrival    
	
	if ((++seqno_ < maxpkts_) || (maxpkts_ <= 0))
		return(t);
	else
		return(-1); 
}

int PackMimeOpenLoop::command(int argc, const char*const* argv)
{	 
	if (argc == 3) {
		if (strcmp (argv[1], "set-pac-size") == 0) {
			int res = lookup_rv (pm_pac_size_, argv[2]);
			if (res == TCL_ERROR) {
				delete pm_pac_size_;
				fprintf (stderr, "Invalid packet size random variable\n");
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
		else if (strcmp (argv[1], "set-pac-ia") == 0) {
			int res = lookup_rv (pm_pac_ia_, argv[2]);
			if (res == TCL_ERROR) {
				delete pm_pac_ia_;
				fprintf (stderr,"Invalid packet arrive random variable\n");
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
	} else if (argc == 2) {
		if (strcmp(argv[1], "start") == 0) {			
			start();
			return(TCL_OK);
		}
	}
	return (Application::command(argc, argv));
}
