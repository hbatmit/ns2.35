/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1990-1997 Regents of the University of California.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
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
 *
 * Here is one set of parameters from one of my simulations:
 * 
 * ed [ q_weight=0.002 thresh=5 linterm=30 maxthresh=15
 *         mean_pktsize=500 dropmech=random-drop queue-size=60
 *         plot-file=none bytes=false doubleq=false dqthresh=50 
 *	   wait=true ]
 * 
 * 1/"linterm" is the max probability of dropping a packet. 
 * There are different options that make the code
 * more messy that it would otherwise be.  For example,
 * "doubleq" and "dqthresh" are for a queue that gives priority to
 *   small (control) packets, 
 * "bytes" indicates whether the queue should be measured in bytes 
 *   or in packets, 
 * "dropmech" indicates whether the drop function should be random-drop 
 *   or drop-tail when/if the queue overflows, and 
 *   the commented-out Holt-Winters method for computing the average queue 
 *   size can be ignored.
 * "wait" indicates whether the gateway should wait between dropping
 *   packets.
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/queue/red.h,v 1.46 2010/03/08 05:54:53 tom_henderson Exp $ (LBL)
 */

#ifndef ns_red_h
#define ns_red_h

#include "queue.h"

#include "trace.h"
class LinkDelay;

/*
 * Early drop parameters, supplied by user
 */
struct edp {
	/*
	 * User supplied.
	 */
	int mean_pktsize;	/* avg pkt size, linked into Tcl */
	int idle_pktsize;	/* avg pkt size used during idle times */
	int bytes;		/* true if queue in bytes, false if packets */
	int wait;		/* true for waiting between dropped packets */
	int setbit;		/* true to set congestion indication bit */
	int gentle;		/* true to increases dropping prob. slowly *
				 * when ave queue exceeds maxthresh. */
	double th_min;		/* minimum threshold of average queue size */
	double th_min_pkts;	/* always maintained in packets */
	double th_max;		/* maximum threshold of average queue size */
	double th_max_pkts;     /* always maintained in packets */
	double max_p_inv;       /* 1/max_p, for max_p = maximum prob.  */
	                        /* adaptive RED: the initial max_p_inv     */	
	double mark_p;		/* when p < mark_p, mark chosen packets */
				/* when p > mark_p, drop chosen packets */
        int use_mark_p;		/* use mark_p only for deciding when to drop, */
				/*   if queue is not full */
				/* Set use_mark_p true and set mark_p to 2.0 */
				/*   to always mark instead of drop */
				/*   when queue is not full */
	double q_w;		/* queue weight given to cur q size sample */
	int adaptive;		/* 0 for default RED */
				/* 1 for adaptive RED, adapting max_p */
	int cautious;           /* 0 for default RED */
                                /* 1 for not dropping/marking when the */
                                /*  instantaneous queue is much below the */
                                /*  average */
	double alpha;           /* adaptive RED: additive param for max_p */
	double beta;            /* adaptive RED: multip param for max_p */
	double interval;	/* adaptive RED: interval for adaptations */
	double targetdelay;     /* adaptive RED: target queue size */
	double top;		/* adaptive RED: upper bound for max_p */
	double bottom;		/* adaptive RED: lower bound for max_p */
				/* 0 for automatic setting */
	int feng_adaptive;	/* adaptive RED: Use the Feng et al. version */
			
	/*
	 * Computed as a function of user supplied paramters.
	 */
	double ptc;		/* packet time constant in packets/second */
	double delay;		/* link delay */
};

/*
 * Early drop variables, maintained by RED
 */
struct edv {
	TracedDouble v_ave;	/* average queue size */
	TracedDouble v_prob1;	/* prob. of packet drop before "count". */
	double v_slope;		/* used in computing average queue size */
				/* obsolete */
	double v_prob;		/* prob. of packet drop */
	double v_a;		/* v_prob = v_a * v_ave + v_b */
	double v_b;
	double v_c;		/* used for "gentle" mode */
	double v_d;		/* used for "gentle" mode */
	int count;		/* # of packets since last drop */
	int count_bytes;	/* # of bytes since last drop */
	int old;		/* 0 when average queue first exceeds thresh */
	TracedDouble cur_max_p;	//current max_p
	double lastset;		/* adaptive RED: last time adapted */
	enum Status {Above, Below, Between}; // for use in Feng's Adaptive RED
	Status status;
	//edv() : v_ave(0.0), v_prob1(0.0), v_slope(0.0), v_prob(0.0),
	//v_a(0.0), v_b(0.0), v_c(0.0), v_d(0.0), count(0), 
	//	count_bytes(0), old(0), cur_max_p(1.0) { }
};

class REDQueue : public Queue {
 public:	
	/*	REDQueue();*/
	REDQueue(const char * = "Drop");
 protected:
	void initParams();
	int command(int argc, const char*const* argv);
	void enque(Packet* pkt);
	virtual Packet *pickPacketForECN(Packet* pkt);
	virtual Packet *pickPacketToDrop();
	Packet* deque();
	void initialize_params();
	void reset();
	void run_estimator(int nqueued, int m);	/* Obsolete */
	double estimator(int nqueued, int m, double ave, double q_w);
	void updateMaxP(double new_ave, double now);
	void updateMaxPFeng(double new_ave);
	int drop_early(Packet* pkt);
	double modify_p(double p, int count, int count_bytes, int bytes,
	   int mean_pktsize, int wait, int size);
 	double calculate_p_new(double v_ave, double th_max, int gentle, 
	  double v_a, double v_b, double v_c, double v_d, double max_p);
 	double calculate_p(double v_ave, double th_max, int gentle, 
	  double v_a, double v_b, double v_c, double v_d, double max_p_inv);
        virtual void reportDrop(Packet *pkt); //pushback
	void print_summarystats();

	
	int summarystats_;	/* true to print true average queue size */

	LinkDelay* link_;	/* outgoing link */
	int fifo_;		/* fifo queue? */
        PacketQueue *q_; 	/* underlying (usually) FIFO queue */
		
	int bcount_() { return q_->byteLength(); };		
			/* OBSOLETED - USE q_->byteLength() INSTEAD */
	int qib_;		/* bool: queue measured in bytes? */
	NsObject* de_drop_;	/* drop_early target */

	//added to be able to trace EDrop Objects - ratul
	//the other events - forced drop, enque and deque are traced by a different mechanism.
	NsObject * EDTrace;    //early drop trace
	char traceType[20];    //the preferred type for early drop trace. 
	                       //better be less than 19 chars long

	Tcl_Channel tchan_;	/* place to write trace records */
	TracedInt curq_;	/* current qlen seen by arrivals */
	void trace(TracedVar*);	/* routine to write trace records */

	/*
	 * Static state.
	 */
	int drop_tail_;		/* drop-tail */
	int drop_front_;	/* drop-from-front */
	int drop_rand_;		/* drop-tail, or drop random? */
	int ns1_compat_;	/* for ns-1 compatibility, bypass a */
				/*   small bugfix */

	edp edp_;	/* early-drop params */
	int doubleq_;	/* for experiments with priority for small packets */
	int dqthresh_;	/* for experiments with priority for small packets */

	/*
	 * Dynamic state.
	 */
	int idle_;		/* queue is idle? */
	double idletime_;	/* if so, since this time */
	edv edv_;		/* early-drop variables */
	int first_reset_;       /* first time reset() is called */

	void print_edp();	// for debugging
	void print_edv();	// for debugging

};

#endif
