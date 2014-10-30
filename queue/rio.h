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
 * @(#) $Header: /cvsroot/nsnam/ns-2/queue/rio.h,v 1.5 2000/07/04 01:59:31 sfloyd Exp $ (LBL)
 */

#ifndef ns_rio_h
#define ns_rio_h

#include "tclcl.h"
#include "packet.h"
#include "random.h"
#include "flags.h"
#include "delay.h"
#include "template.h"
#include "red.h"

/*
 * Early drop parameters, supplied by user, for a subqueue.
 */
struct edp_rio {
	/*
	 * User supplied.
	 */
	int gentle;	        /* when ave queue exceeds maxthresh. */
	double th_min;	/* minimum threshold of average queue size */
	double th_max;	/* maximum threshold of average queue size */
	double max_p_inv;	/* 1/max_p, for max_p = maximum prob.  */
};

/*
 * Early drop variables, maintained by RIO, for a subqueue.
 */
struct edv_rio {
        /* added by Wenjia modified by Yun: a new set In packets */
        double v_ave;         /* average In queue size */
	double v_prob1;	/* prob. of packet drop before "count". */
        double v_slope;       /* used in computing average queue size */
        double v_r;
        double v_prob;        /* prob. of packet drop */
        double v_a;           /* v_prob = v_a * v_ave + v_b */
        double v_b;
	double v_c;
	double v_d;
        int count;           /* # of packets since last drop */
        int count_bytes;     /* # of bytes since last drop */
        int old;             /* 0 when average queue first exceeds thresh */
        struct dlist* drops;

	edv_rio() : v_ave(0.0), v_prob1(0.0), v_slope(0.0), v_prob(0.0),
		v_a(0.0), v_b(0.0), count(0), count_bytes(0), old(0) { }
};

class REDQueue;

class RIOQueue : public virtual REDQueue {
 public:	
	RIOQueue();
 protected:
	void enque(Packet* pkt);
	Packet* deque();
	void reset();

	void run_out_estimator(int out, int total, int m);

	// int drop_early(Packet* pkt);
	int drop_in_early(Packet* pkt);
	int drop_out_early(Packet* pkt);

	/* added by Yun: In packets byte count */
	int in_len_;	/* In Packets count */
	int in_bcount_;	/* In packets byte count */

	void trace(TracedVar*);	/* routine to write trace records */

	/*
	 * Static state.
	 */
	int priority_method_;	/* 0 to leave priority field in header, */
				/*  1 to use flowid as priority.  */

	edp_rio edp_in_; 	/* early-drop params for IN traffic */
	edp_rio edp_out_;        /* early-drop params for OUT traffic */

	/*
	 * Dynamic state.
	 */
	/* added by Wenjia noticed by Yun: to trace the idle */
	int in_idle_;
	double in_idletime_;

	edv_rio edv_in_;	/* early-drop variables for IN traffic */
	edv_rio edv_out_;        /* early-drop variables for OUT traffic */ 

	void print_edp();	// for debugging
	void print_edv();	// for debugging
};

#endif
