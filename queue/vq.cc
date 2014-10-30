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

/* Marking scheme proposed by Kunniyur and Srikant in "Decentralized Adaptive
*    ECN Algorithms" published in the Proceedings of Infocom2000, Tel-Aviv, 
*    Israel, March 2000.
* 
* Basic Idea of the AVQ (Adaptive Virtual Queue) Algorithm
* --------------------------------------------------------
* 
* The scheme involves having a virtual queue at each link with the same
* arrival rate as the original queue, but with the service rate scaled
* down by some factor (ecnlim_). The buffer capacity remains the same,
* but if needed we can also scale the buffer capacity by come constant
* (buflim_). When a packet is dropped from the virtual queue, mark a
* packet at the front of the real queue. Although in this implemetation,
* a "drop-tail" function is used in the VIRTUAL QUEUE to mark packets in
* the original queue, any algorithm (like RED) can be used in the
* virtual queue to signal congestion. The capacity of the virtual queue
* is adapted periodically to guarantee loss-free service. 
* 
* The update of the virtual capacity (capacity of the virtual queue) is
* done using a token-bucket. For implentation details, please refer to
* the Sigcomm 2001 paper titled "Analysis and Design of an Adaptive
* Virtual Queue (AVQ) algorithm for Active Queue Management (AQM). 
* 
* The virtual queue length can be taken in packets or in bytes. If the
* packet sizes are not fixed, then it is recommended to use bytes
* instead of packets as units of length. If the variable (qib_) is set,
* then the virtual queue is measured in bytes, else it is measured in
* packets and the mean packet size is set to 1000. 
* 
*                          -Srisankar
* 
* ********************************************************************  
* Options in the AVQ algorithm code:
* ----------------------------------
* 
* * ecnlim_ -> This parameter specifies the capacity of the virtual 
*              queue as a fraction of the capcity of the original queue.
* 
* * buflim_ -> This parameter specifies the buffer capacity of the
*              virtual queue as a fraction of the buffer capacity of the
*              original queue.
* 
* * queue_in_bytes_ -> Indicates whether you want to measure the queue
*                      in bytes or packets. 
* 
* * markpkts_ -> Set to 1 if router wants to emply marking for that
*                link. Set to 0, if dropping is preferred. 
* 
* * markfront_ -> Set to 1 if mark from front policy is employed. Usually
*                 set to zero.   
* 
* * mean_pktsize- -> The mean packet size is set to 1000.
* 
* * gamma -> This parameter specifies the desired arrival rate at the link.
* 
* ********************************************************************  
*/

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/queue/vq.cc,v 1.7 2010/03/08 05:54:53 tom_henderson Exp $ (LBL)";
#endif
#include "flags.h"
#include "delay.h"
#include "vq.h"
#include "math.h"

static class VqClass : public TclClass {
public:
	VqClass() : TclClass("Queue/Vq") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc==5) 
			return (new Vq(argv[4]));
		else
			return (new Vq("Drop"));
	}
} class_vq;

Vq::Vq(const char * ) : link_(NULL), EDTrace(NULL), tchan_(0){
	q_ = new PacketQueue;
	pq_ = q_;
	bind_bool("drop_front_", &drop_front_);
	bind("ecnlim_", &ecnlim_); //  = ctilde/c ; Initial value set to 0.8
	bind("buflim_", &buflim_); /* Fraction of the original buffer that the VQ buffer has. Default is 1.0 */
	bind_bool("queue_in_bytes_", &qib_);	 // boolean: q in bytes?
	bind("gamma_", &gamma_); // Defines the utilization. Default is 0.98
	bind_bool("markpkts_", &markpkts_); /* Whether to mark or drop?  Default is drop */
	bind_bool("markfront_", &markfront_); /* Mark from front?  Deafult is false */
	bind("mean_pktsize_", &mean_pktsize_);  // avg pkt size
	bind("curq_", &curq_);          // current queue size
	
	vq_len = 0.0;
	prev_time = 0.0;
	vqprev_time = 0.0;
	alpha2 = 0.15; // Determines how fast we adapt at the link
	Pktdrp = 0; /* Useful if we are dropping pkts instead of marking */
	firstpkt = 1;
	qlength = 0; /* Tracks the queue length */
}

int Vq::command(int argc, const char*const* argv) {
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

void Vq::enque(Packet* p)
{
	q_->enque(p);
	hdr_cmn* ch = hdr_cmn::access(p);
	qlength = qlength + (qib_ * ch->size()) + (1 - qib_);
	
	if(firstpkt){
		/* Changing c_ so that it is measured in packets per second */
		/* Assuming packets are fixed size with 1000 bytes */
		
		if(qib_) c_ = c_ / (8.0);
		else c_ = c_ / (8.0 * mean_pktsize_);
		firstpkt = 0;
		ctilde = c_ * ecnlim_;
		prev_time = Scheduler::instance().clock();
		vqprev_time = Scheduler::instance().clock();
	}

	/* Update the Virtual Queue length */ 	
	curr_time = Scheduler::instance().clock();
	vq_len = vq_len - (ctilde * (curr_time - vqprev_time));
	vqprev_time = curr_time;
	if(vq_len < 0.0) vq_len = 0.0;
	vq_len = vq_len + qib_ * ch->size() + (1 - qib_);
	
	/* checkPacketForECN() returns 1 if we need to mark or drop a packet*/
	
	if(checkPacketForECN()){
		if(markpkts_)
			markPacketForECN(p);
		else
			dropPacketForECN(p);
	}
	
	/* Adaptation of the virtual capacity, tilde(C) */
	/* Use the token bucket system  */
	/* Scale alpha appropriately if qib_ is set */
	
	if(Pktdrp == 0){ // Do the adaptation
		/* Pktdrp = 0 always when marking */
		ctilde = ctilde + alpha2*gamma_*c_*(curr_time - prev_time) - alpha2*(1.0 - qib_) - alpha2*qib_*ch->size();
		if(ctilde > c_) ctilde = c_;
		if(ctilde <0.0) ctilde = 0.0;
		prev_time = curr_time;
	}
	else{ // No adaptation and reset Pktdrp
		Pktdrp = 0;
	}
	
	if (qlength > qlim_*( 1 - qib_ + qib_*mean_pktsize_)) {
		if (drop_front_) { /* remove from head of queue */
			if(q_->length() > 0){
				Packet *pp = q_->head();
				qlength = qlength - qib_ * ch->size() - (1 - qib_);
				q_->remove(pp); 
				drop(pp);
			}
		} 
		else {
			q_->remove(p);
			qlength = qlength - qib_ * ch->size() - (1 - qib_);
			drop(p);
		}
	}
	curq_ = qlength;
}

/* This is a simple DropTail on the VQ. However, one Can add 
   any AQM scheme on VQ here */
int Vq::checkPacketForECN(){
	if(vq_len > (buflim_ * qlim_ * ( 1 - qib_ + qib_*mean_pktsize_))){
		return 1;
	}
	else{
		return 0;
	}
}

/* Implements a simple mark-tail/mark-front here. If needed other
   mechanism (like mark-random ) can also be implemented */ 
void  Vq::markPacketForECN(Packet* pkt)
{
	/* Update the VQ length */
	hdr_cmn* ch = hdr_cmn::access(pkt);
	vq_len = vq_len - qib_ * ch->size() - (1.0 - qib_);
	if(vq_len < 0.0) vq_len = 0.0;
	
	if(markfront_){ 
		Packet *pp = q_->head();
		hdr_flags* hf = hdr_flags::access(pp);
		if(hf->ect() == 1)  // ECN capable flow
			hf->ce() = 1; // Mark the TCP Flow;
	}
	else{ 
		/* Mark the current packet and forget about it */
		hdr_flags* hdr = hdr_flags::access(pkt);
		if(hdr->ect() == 1)  // ECN capable flow
			hdr->ce() = 1; // For TCP Flows
	}
}	

/* Implements a simple drop-tail/drop-front here. If needed other
   mechanism (like drop-random ) can also be implemented */ 
void Vq::dropPacketForECN(Packet* pkt) 
{
	/* Update the VQ length */
	hdr_cmn* ch = hdr_cmn::access(pkt);
	vq_len = vq_len - qib_ * ch->size() - (1.0 - qib_);
	if(vq_len < 0.0) vq_len = 0.0;

	if(drop_front_){
		/* If drop from front is enabled, then deque a 
		   packet from the front of the queue and drop it ...
		   Usually not recommended */ 
		if(q_->length() > 0 ){
			Packet *pp = q_->head();
			qlength = qlength - qib_ * ch->size() - (1 - qib_);
			q_->remove(pp); /* The queue length is taken care of in
										 in the deque program */
			drop(pp);

		}
	}
	else{
		q_->remove(pkt);
		qlength = qlength - qib_ * ch->size() - (1 - qib_);
		drop(pkt);
	}
	
	/* If one is dropping packets, one needs to be careful about 
	   measuring the total arrival rate to calculate the virtual
	   capacity, tilde(C). In this case, the arrival rate that is
	   taken into the adaptation algorithm is the accepted rate and
	   not the offered rate. */
	Pktdrp = 1; // Do not update tilde(C)
	
}	

Packet* Vq::deque()
{
	if((q_->length() > 0)){
		Packet *ppp = q_->deque();
		hdr_cmn* ch = hdr_cmn::access(ppp);
		qlength = qlength - qib_ * ch->size() - (1 - qib_);
		curq_ = qlength;
		return ppp;
	}
	else return q_->deque();
}

/*
 * Routine called by TracedVar facility when variables change values.
 * Currently used to trace value of 
 * the instantaneous queue size seen by arriving packets.
 * Note that the tracing of each var must be enabled in tcl to work.
 */

void Vq::trace(TracedVar* v)
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








