/* -*-  Mode:C++; c-basic-offset:4; tab-width:4; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1994 Regents of the University of California.
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
 *  This product includes software developed by the Computer Systems
 *  Engineering Group at Lawrence Berkeley Laboratory.
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
 */

/* Marking scheme proposed by Gibbens and Kelly in "Resource pricing
   and the evolution of Internet"

   Central Idea:
   ------------

   The link maintains a virtual queue with the same arrivals as the
   real queue. However, the capacity of the virtual queue is smaller
   than the capacity of the real queue, i.e., set the capacity of the
   virtual queue, ctilde = ecnlim_ * c_. The buffer is also scaled by
   the same factor, i,e B_{vq} = ecnlim_ * qlim_. 

   When the VQ drops a packet, mark ALL the packets in the real queue
   as well as all the INCOMING packets till the VQ becomes empty
   again.

   Variables:
   ----------

   ecnlim_ : Fraction of the buffer size and the capacity that the VQ
             has.
	 
   mark_flag: Indicates that the VQ overflowed and that all outgoing
              packets has to be marked.

*/ 


#include "flags.h"
#include "delay.h"
#include "gk.h"
#include "math.h"

static class GKClass : public TclClass {
public:
  	GKClass() : TclClass("Queue/GK") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc==5)
			return (new GK(argv[4]));
		else
			return (new GK("Drop"));
	}	
} class_gk;

GK::GK(const char * ):link_(NULL), EDTrace(NULL), tchan_(0)
{
	q_ = new PacketQueue;
	pq_ = q_;
	bind_bool("drop_front_", &drop_front_);
	bind("ecnlim_", &ecnlim_); 
	bind("mean_pktsize_", &mean_pktsize_); 
	bind("curq_", &curq_); 
	vq_len = 0.0;
	prev_time = 0.0;
	mark_flag = 0;
}

int GK::command(int argc, const char*const* argv) {
	Tcl& tcl = Tcl::instance();
  if (argc == 3) {
	  if (strcmp(argv[1], "link") == 0) {
		  LinkDelay* del = (LinkDelay*)TclObject::lookup(argv[2]);
		  if (del == 0) {
			  return(TCL_ERROR);
		  }
		  // set capacity now
			link_ = del;
		  c_ = del->bandwidth();
			c_ = c_ / (8.0 * mean_pktsize_);
		  return (TCL_OK);
	  }
	  if (!strcmp(argv[1], "packetqueue-attach")) {
		  delete q_;
		  if (!(q_ = (PacketQueue*) TclObject::lookup(argv[2])))
			  return (TCL_ERROR);
		  else {
			  pq_ = q_;
			  return (TCL_OK);
		  }
	  }
		// attach a file for variable tracing
		if (strcmp(argv[1], "attach") == 0) {
			int mode;
			const char* id = argv[2];
			tchan_ = Tcl_GetChannel(tcl.interp(), (char*)id, &mode);
			if (tchan_ == 0) {
				tcl.resultf("Vq: trace: can't attach %s for writing", id);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
  }
  return Queue::command(argc, argv);
}

void GK::enque(Packet* p)
{
  q_->enque(p);
 	 
  curr_time = Scheduler::instance().clock();
  /*Whenever a packet is enqueued, the actual length of the
    virtual queue is determined */
 
  if(curr_time > prev_time){
	  deque_vq();
  }
  /* Add the packet to the VQ */
  vq_len = vq_len + 1.0;

  /* If the VQ overflows, set flag so that all packets may be marked
	 till the VQ hits zero again. */
  if(vq_len > (ecnlim_ * qlim_)){
	  mark_flag = 1; // Indicates that all outgoing packets has to be marked
	  vq_len = vq_len - 1.0;
  }
  
  if (q_->length() >= qlim_) {
		if (drop_front_) { /* remove from head of queue */
	  	Packet *pp = q_->deque();
	  	drop(pp);
		} else {
	  	q_->remove(p);
	  	drop(p);
		}
  }
 	curq_ = q_->length(); 
}

Packet* GK::deque()
{
  /* Check the status of the virtual queue. We do this to update the
	 mark_flag.  */
  curr_time = Scheduler::instance().clock();
  deque_vq();

  /* If the Real queue has packets and the mark_flag is set, mark the
	 outgoing packet. */
  if((q_->length() > 0) && (mark_flag == 1)){
	Packet *pp = q_->deque();
	hdr_flags* hf = hdr_flags::access(pp);
	if(hf->ect() == 1)  // ECN capable flow
		hf->ce() = 1; // Mark the TCP Flow;
	return pp;
  }
  else return q_->deque();
}

/* This procedure updates the VQ */
void GK::deque_vq(){
  if(vq_len > 0.0){ 
	vq_len = vq_len - (ecnlim_ * c_ * (curr_time - prev_time));
	prev_time = curr_time;
	
	/* If the VQ hits zero, unset mark_flag */
	if(vq_len <= 0.0){
	  vq_len = 0.0;
	  mark_flag = 0;
	}
  }
}

/*
 * Routine called by TracedVar facility when variables change values.
 * Currently used to trace value of 
 * the instantaneous queue size seen by arriving packets.
 * Note that the tracing of each var must be enabled in tcl to work.
 */

void GK::trace(TracedVar* v)
{
	char wrk[500];
	const char *p;

	if ((p = strstr(v->name(), "curq")) == NULL) {
		fprintf(stderr, "Vq:unknown trace var %s\n", v->name());
		return;
	}

	if (tchan_) {
		int n;
		double t = Scheduler::instance().clock();
		// XXX: be compatible with nsv1 RED trace entries
		if (*p == 'c') {
			sprintf(wrk, "Q %g %d", t, int(*((TracedInt*) v)));
		} else {
			sprintf(wrk, "%c %g %g", *p, t, double(*((TracedDouble*) v)));
		}
		n = strlen(wrk);
		wrk[n] = '\n'; 
		wrk[n+1] = 0;
		(void)Tcl_Write(tchan_, wrk, n+1);
	}
	return; 
}
