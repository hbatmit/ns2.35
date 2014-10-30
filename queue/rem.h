/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1990-1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *		notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *		notice, this list of conditions and the following disclaimer in the
 *		documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *		must display the following acknowledgement:
 *			This product includes software developed by the Computer Systems
 *			Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *		to endorse or promote products derived from this software without
 *		specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.	IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 * ns-2 code written by: Sanjeewa Athuraliya and Victor Li
 *											 Caltech, Pasadena
 *											 CA.
 */

#ifndef ns_rem_h
#define ns_rem_h

#include "queue.h"
#include <stdlib.h>
#include "agent.h"
#include "template.h"

class LinkDelay;

class REMQueue;

/*
 * REM parameters, supplied by user
 */
struct remp {
	/*
	 * User supplied.
	 */
	double p_inw;	/* queue weight given to cur q size sample */
	double p_gamma;
	double p_phi;
	double p_delta;
	double p_pktsize;
	double p_updtime;
	double p_bo;
	/*
	 * Computed as a function of user supplied parameters.
	*/
	double p_ptc;

};

/*
 * REM variables, maintained by REM 
 */
struct remv {
	TracedDouble v_pl;	/* link price */
	TracedDouble v_prob;	/* prob. of packet marking. */
	double v_in;	/* used in computing the input rate */
	double v_ave;
	double v_count;
	double v_pl1;
	double v_pl2;
	double v_in1;
	double v_in2;

	remv() : v_pl(0.0), v_prob(0.0), v_in(0.0), v_ave(0.0),	v_count(0.0){ }
};

class REMTimer : public TimerHandler {
	public:
		REMTimer (REMQueue *a) : TimerHandler() { a_ = a; }
	protected:
		virtual void expire (Event *e);
		REMQueue *a_;
};			 

class REMQueue : public Queue {
 public:	
	REMQueue();
	void set_update_timer();
	void timeout();

 protected:
	int command(int argc, const char*const* argv);
	void enque(Packet* pkt);
	Packet* deque();
	void reset();
	void run_updaterule();

	LinkDelay* link_;	/* outgoing link */
	PacketQueue *q_; 	/* underlying (usually) FIFO queue */
		
	Tcl_Channel tchan_;		 /* Place to write trace records */
	TracedInt curq_;	/* current qlen seen by arrivals */
	void trace(TracedVar*);	/* routine to write trace records */

	REMTimer rem_timer_;

	/*
	 * Static state.
	 */
	double pmark_;	 //number of packets being marked
	remp remp_;	/* early-drop params */

	/*
	 * Dynamic state.
	 */
	remv remv_;		/* early-drop variables */

	int markpkts_ ; 
	int bcount_;
	int qib_; 

	void print_remp();	// for debugging
	void print_remv();	// for debugging
};


#endif
