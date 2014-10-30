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
 * $Id: demarker.cc,v 1.1 2003/02/02 22:18:22 xuanc Exp $
 */

#include <string.h>
#include <queue.h>
#include "demarker.h"
#include "tcp.h"

static class DemarkerClass : public TclClass {
public:
	DemarkerClass() : TclClass("Queue/Demarker") {}
	TclObject* create(int, const char*const*) {
		return (new Demarker);
	}
} class_demarker;

Demarker::Demarker() {
	q_ = new PacketQueue; 
  
	last_monitor_update_=0.0;
	monitoring_window_ = 0.1;
	
	for (int i=0; i<=NO_CLASSES; i++) {
		demarker_arrvs_[i]=0;
		arrived_Bits_[i] = 0;
	}
  // Binding arrays between cc and tcl still not supported by tclcl...
  if (NO_CLASSES != 4) {
	printf("Change Demarker's code!!!\n\n");
	abort();
  }
  bind("demarker_arrvs1_",    &(demarker_arrvs_[1]));
  bind("demarker_arrvs2_",    &(demarker_arrvs_[2]));
  bind("demarker_arrvs3_",    &(demarker_arrvs_[3]));
  bind("demarker_arrvs4_",    &(demarker_arrvs_[4]));
}



int Demarker::command(int argc, const char*const* argv) {
	if (argc == 3) {
		if (strcmp(argv[1], "trace-file") == 0) {
			file_name_ = new(char[500]);
			strcpy(file_name_,argv[2]);
			if (strcmp(file_name_,"null") != 0) {
				demarker_type_ = VERBOSE;
				for (int i=1; i<=NO_CLASSES; i++) {
					char filename[500]; 
					sprintf(filename,"%s.%d", file_name_,i);
					if ((delay_tr_[i] = fopen(filename,"w"))==NULL) {
						printf("Problem with opening the trace files\n");
						abort();
					}
				}
			} else {
				demarker_type_ = QUIET;
			}
			return (TCL_OK);
		} else if (strcmp(argv[1], "id") == 0) {
			link_id_ = (int)atof(argv[2]);
			return (TCL_OK);
		} 
	}
	return Queue::command(argc, argv);
}



void Demarker::enque(Packet* p) {
	q_->enque(p);
	if (q_->length() >= qlim_) {
		q_->remove(p);
		drop(p);
		printf("Packet drops in Demarker of type:%d\n", demarker_type_);
	}
}



Packet* Demarker::deque() {
  Packet* p= q_->deque();
  if (p==NULL) return p;
  
  hdr_ip*  iph = hdr_ip::access(p);
  hdr_cmn* cm_h	= hdr_cmn::access(p);
  double  cur_time  = Scheduler::instance().clock();

  int cls = iph->prio_;
  if ((cls<1) || (cls>NO_CLASSES)) {
    printf("Wrong class type in Demarker-deque (S=%d, D=%d, FID=%d, Class=%d)\n",
	   (int)(iph->src().addr_), (int)(iph->dst().addr_),
	   iph->fid_, iph->prio_);
    
    fflush(stdout);
    abort();
  }
  demarker_arrvs_[cls] += 1.;

 
  if (demarker_type_ == VERBOSE) {
    // Write end-to-end delay of packet in per class trace file
    if (cur_time > START_STATISTICS) {
      double pack_del = cur_time - cm_h->ts_arr_;
      cm_h->ts_arr_=0;    // This stupid thing is required..
      // print arrival time and delay
      fprintf(delay_tr_[cls], "%.5f %.5f\n", 
	      cur_time, pack_del);
    }    
 
    return p;
  }
  
  return p;
}
