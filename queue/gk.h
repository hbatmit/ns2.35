
/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
 *
 */
 
#ifndef ns_gk_h
#define ns_gk_h
 
#include <string.h>
#include "queue.h"

class LinkDelay; 

class GK : public Queue {
 public:
	GK(const char *);
 protected:
  int command(int argc, const char*const* argv);
  void enque(Packet*);
  Packet* deque();
  void deque_vq();    /* Deques from the virtual queue_ */
	LinkDelay* link_; /* outgoing link */
  PacketQueue *q_;    /* underlying FIFO queue */
  int drop_front_;    /* drop-from-front (rather than from tail) */
  double ecnlim_;     /* Factor by which capacity and buffer is acled down*/
  double vq_len;      /* Virtual Queue length */
  double c_;          /* Capacity of the link */
  int mark_flag;      /* Indicates that all outgoing packets shd be marked */
  double prev_time;
  double curr_time;
	int mean_pktsize_;
	TracedInt curq_;	/* current qlen seen by arrivals */

	// added to be able to trace EDrop Objects
	// the other events - forced drop, enque and deque are traced by a 
	// different mechanism.
	NsObject * EDTrace;    //early drop trace
	char traceType[20];    /* the preferred type for early drop trace. 
														better be less than 19 chars long */
	Tcl_Channel tchan_;	/* place to write trace records */
	void trace(TracedVar*);	/* routine to write trace records */
};

#endif
