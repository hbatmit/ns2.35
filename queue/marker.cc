/*
 * Copyright (c) 2000-2002, by the Rector and Board of Visitors of the 
 * University of Virginia.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, 
 * with or without modification, are permitted provided 
 * that the following conditions are met:
 *
 * Redistributions of source code must retain the above 
 * copyright notice, this list of conditions and the following 
 * disclaimer. 
 *
 * Redistributions in binary form must reproduce the above 
 * copyright notice, this list of conditions and the following 
 * disclaimer in the documentation and/or other materials provided 
 * with the distribution. 
 *
 * Neither the name of the University of Virginia nor the names 
 * of its contributors may be used to endorse or promote products 
 * derived from this software without specific prior written 
 * permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, 
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *                                                                     
 * Marker module for JoBS (and WTP).
 *                                                                     
 * Authors: Constantinos Dovrolis <dovrolis@mail.eecis.udel.edu>, 
 *          Nicolas Christin <nicolas@cs.virginia.edu>, 2000-2002       
 *								      
 * $Id: marker.cc,v 1.1 2003/02/02 22:18:22 xuanc Exp $
 */

#include <string.h>
#include <queue.h>
#include "random.h"
#include "marker.h"

static class MarkerClass : public TclClass {
 public:
	MarkerClass() : TclClass("Queue/Marker") {}
	TclObject* create(int, const char*const*) {
		return (new Marker);
	}
} class_marker;



Marker::Marker() {
	q_ = new PacketQueue; 
	for (int i=0; i<=NO_CLASSES; i++) marker_arrvs_[i]=0;
	// How can we bind arrays between cc and tcl????
	if (NO_CLASSES!=4) {
		printf("Change Marker's code!!!\n\n");
		abort();
	} 
	bind("marker_arrvs1_",	&(marker_arrvs_[1]));
	bind("marker_arrvs2_",	&(marker_arrvs_[2]));
	bind("marker_arrvs3_",	&(marker_arrvs_[3]));
	bind("marker_arrvs4_",	&(marker_arrvs_[4]));
	
	// Some initial values for the random marking fractions
	marker_frc_[0]=0.0; // class-0 is not used
	marker_frc_[1]=0.4; 
	marker_frc_[2]=0.7;
	marker_frc_[3]=0.9; 
	marker_frc_[4]=1.0;
}



int Marker::command(int argc, const char*const* argv) {
	if (argc == 3) {
		if (!strcmp(argv[1], "marker_type")) {
			marker_type_ = atoi(argv[2]);	
			if ((marker_type_ != DETERM) && (marker_type_ != STATIS)) {
				printf("Wrong Marker Type\n");
				abort();
			}
			return (TCL_OK);
		}
		if (!strcmp(argv[1], "marker_class")) {
			marker_class_ = atoi(argv[2]);	
			if (marker_class_<1 || marker_class_>NO_CLASSES) {
				printf("Wrong Marker Class:%d\n", marker_class_);
				abort();
			}
			return (TCL_OK);
		}
		if (!strcmp(argv[1], "init-seed")) {
			rn_seed_ = atoi(argv[2]);	
			Random::seed(rn_seed_);
			srand48((long)(rn_seed_));
			return (TCL_OK);
		}
	}
	if (argc == NO_CLASSES+2) {
		if (!strcmp(argv[1], "marker_frc")) {
			double sum = 0.0;
			for (int i=1; i<=NO_CLASSES; i++) {
				marker_frc_[i] = sum + atof(argv[1+i]);	
				sum = marker_frc_[i];
				// printf("Fraction of class-%d traffic: %.3f\n", 
				//	i, marker_frc_[i]-marker_frc_[i-1]);
			}
			if (sum >  1.0) {
			   printf("Class marking thresholds should add to 1.0 \n");
			   abort();
			} 
			return (TCL_OK);
		}
	}
	return Queue::command(argc, argv);
}



void Marker::enque(Packet* p) {
	hdr_ip*	  iph = hdr_ip::access(p);
	hdr_cmn* cm_h = hdr_cmn::access(p);
	
	// Timestamp the packet's arrival in a header field
	// used for measuring the e2e delay of the packet
	// (for monitoring purposes)
	double  cur_time  = Scheduler::instance().clock();
	cm_h->ts_arr_ = cur_time;

	if (marker_type_ == DETERM) {
		// Mark with fixed class 
		iph->prio_ = marker_class_; 
	} else { 
		// (marker_type_ == STATIS) 
		// Determine probabilistically the class of this packet
		double rn = drand48(); 
		int i=0;
		do i++; while (rn >= marker_frc_[i]);
		iph->prio_ = i; 
	}


	// Count the packets arrived in this class
	marker_arrvs_[iph->prio_] += 1.;

	q_->enque(p);
	if (q_->length() >= qlim_) {
		q_->remove(p);
		drop(p);
		printf("Packet drops in Marker of type:%d\n", marker_type_);
	}
}

// Nothing interesting here
Packet* Marker::deque() {
	return q_->deque();
}
