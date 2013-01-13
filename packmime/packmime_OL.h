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

#ifndef ns_packmimeOL_h
#define ns_packmimeOL_h

#include <sys/types.h>
#include <sys/stat.h> 
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "config.h"
#include "random.h"
#include "trafgen.h"
#include "ranvar.h"

/* 
 * Constant bit rate traffic source.   Parameterized by interval, (optional)
 * random noise in the interval, and packet size.  
 */

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

#endif
