/* -*-  Mode:C++; c-basic-offset:2; tab-width:2; indent-tabs-mode:t -*- */
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
 * @(#) $Header: /usr/src/mash/repository/vint/ns-2/drop-tail.h,v 1.8 1998/06/27 01
:23:45 gnguyen Exp $ (LBL)
 */
/* This version of this program contains teh token-bucket formulation.
   For a non token-bucket formulation and for one that is tested, please
   look at vq_plain.cc and vq_plain.h
                         - Srisankar
												   01/07/2001.
*/
 
#ifndef ns_vq_h
#define ns_vq_h

#include <string.h>
#include "queue.h"
#include "assert.h"

class Vq;

class LinkDelay; 

class Vq : public Queue {
public:
  Vq(const char *); 
protected:
  int command(int argc, const char*const* argv);
  void enque(Packet*);
  Packet* deque();
	int checkPacketForECN();
	void markPacketForECN(Packet* pkt);
	void dropPacketForECN(Packet* pkt); 

	LinkDelay* link_;	/* outgoing link */
	PacketQueue *q_;    /* underlying FIFO queue */
  int drop_front_;    /* drop-from-front (rather than from tail) */
  double ecnlim_;     /* Limit when ecn marking comes into effect*/
	double buflim_;
  double vq_len;
  double c_;
	double prev_time;
	double vqprev_time;
  double curr_time;
	double alpha2;
	double gamma_;
	int qib_;	/* bool: queue measured in bytes? */
	double ctilde; // Virtual Capacity
	int markpkts_; // Whether to mark or drop packets
	int markfront_; // Mark Front?
	int firstpkt;
	int Pktdrp;
	int pkt_cnt;
	long int qlength;
	int mean_pktsize_;

	FILE *fp;

	// added to be able to trace EDrop Objects
	// the other events - forced drop, enque and deque are traced by a 
	// different mechanism.
	NsObject * EDTrace;    //early drop trace
	char traceType[20];    /* the preferred type for early drop trace. 
														better be less than 19 chars long */
	Tcl_Channel tchan_;	/* place to write trace records */
	TracedInt curq_;	/* current qlen seen by arrivals */
	void trace(TracedVar*);	/* routine to write trace records */
};

#endif
