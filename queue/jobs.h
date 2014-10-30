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
 * JoBS - ns-2 prototype implementation                                
 *                                                                     
 * Author: Nicolas Christin <nicolas@cs.virginia.edu>, 2000-2002       
 *								      
 * JoBS algorithms originally devised and proposed by		      
 * Nicolas Christin and Jorg Liebeherr                                 
 * Grateful acknowledgments to Tarek Abdelzaher for his help and       
 * comments.                                                           
 *                                                                     
 * $Id: jobs.h,v 1.3 2006/12/17 15:22:42 mweigle Exp $
 */

#ifndef JOBS_H
#define JOBS_H

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "queue.h"
#include "template.h"
#include "random.h"
#include "flags.h"
#include "delay.h"
#include "marker.h"

/* JoBS Queuing */
#ifndef INFINITY
# ifdef __SUNPRO_CC
#  include <values.h>
#  define	INFINITY	MAXDOUBLE
# else
#  define	INFINITY	+1.0e499
# endif
#endif
#define	PRECISION_ERROR +1.0e-10
#define TOL		0.02		// Tolerance in the constraints (2%)
#define MON_WINDOW_SIZE 0.5		// Size of the moving average window
#define RESET_STATS	0
#define UPDATE_STATS	1

/* Dropping strategy */
#define WITH_UPDATE	3
#define WITHOUT_UPDATE	4

/* ADC resolution */
#define ORIGINAL_JOBS	0
#define SHARED_PAIN	1
/* FBS resolution */
#define RESOLVE_OVERFLOW	0
#define RESOLVE_ADC		1

/* Simple macros */
#define min(x, y)   ((x) < (y) ? (x) : (y))
#define max(x, y)   ((x) >= (y) ? (x) : (y))

/* JoBS class */
class JoBS : public Queue {

public: 
	JoBS();
	virtual	int command(int argc, const char*const* argv);
	void	enque(Packet*);
	Packet* deque();
	int	link_id_;
  
protected:

	long	total_backlog_Pkts_;	// Total backlog in packets
	long	total_backlog_Bits_;	// Total backlog in bits
	double	mean_pkt_size_;	        // in bytes... Needs to be *8
	int	drop_front_;		// Drop-from-Front flag
	int	trace_hop_;		// Trace Delays and Drops locally?
	int	adc_resolution_type_;	// Type of algorithm for meeting ADCs
					// 0 = ORIGINAL_JOBS (see techrep)
					// 1 = SHARED_PAIN (drop from all classes)
	int	shared_buffer_;		// 0=separate per-class buffers
					// 1=common buffer
	LinkDelay*	link_;		// outgoing link 
	char*	file_name_;		// Trace files
	int	sampling_period_;
	PacketQueue* 	cls_[NO_CLASSES+1];	// Class queues: do not use class-0 
	int	concerned_RDC_[NO_CLASSES+1];
	int	concerned_RLC_[NO_CLASSES+1];
	int	concerned_ADC_[NO_CLASSES+1];
	int	concerned_ALC_[NO_CLASSES+1];
	int	concerned_ARC_[NO_CLASSES+1];
	double	RDC_[NO_CLASSES+1];	// RDC parameters
	double	RLC_[NO_CLASSES+1];	// RLC parameters
	double	ADC_[NO_CLASSES+1];	// ADC parameters
	double	ALC_[NO_CLASSES+1];	// ALC parameters
	double	ARC_[NO_CLASSES+1];	// ARC parameters

	double	loss_prod_others_[NO_CLASSES+1];
	double	prod_others_  [NO_CLASSES+1];	
	double	service_rate_[NO_CLASSES+1]; // in bps
	double	current_loss_[NO_CLASSES+1]; // in fraction of 1
	double	Rin_	     [NO_CLASSES+1]; // in bits
	double	Rout_	     [NO_CLASSES+1]; // in bits
	double	Rout_th_     [NO_CLASSES+1]; // in bits
	double	Arrival_     [NO_CLASSES+1]; // in bits
	double	last_rate_update_[NO_CLASSES+1];
	
private:

	// Internal functions
	
	void	updateError();
	double  projDelay(int);
	double* assignRateDropsADC();
	double*	adjustRatesRDC();
	int	minRatesNeeded(int);
	void	arvAccounting(Packet*);
	int	pickDroppedRLC(int);
	void	dropTail(int, int);
	void	dropFront(int, int);
	int	enforceWC();
	void	updateStats(Packet*, int);
	
	// Internal variables
	
	int	idle_;				// is the queue idle?
	double	idletime_;			// if so, since when?
	int	pkt_count_;
	double	min_share_;
	double	last_arrival_;
	
	// Statistics
	double	sliding_inter_;
	double	sliding_avg_pkt_size_;
	double	sliding_arv_pkts_;
	double	sliding_arv_pkts_c[NO_CLASSES+1];
	double	sliding_serviced_pkts_[NO_CLASSES+1];
	double	sliding_serviced_bits_[NO_CLASSES+1];
	double	sliding_class_service_rate_[NO_CLASSES+1];
	double	sliding_class_delay_[NO_CLASSES+1];
	
	// Control variables
	
	double	last_xmit_[NO_CLASSES+1];	// last time a packet was sent
	long	backlog_Bits_ [NO_CLASSES+1];
	long	backlog_Pkts_ [NO_CLASSES+1];
	double	error_	      [NO_CLASSES+1];	// in 10e-6*time_unit (us)
	double	min_rate_     [NO_CLASSES+1];	// in Mbps
	double	min_drop_     [NO_CLASSES+1];	// in fraction of 1
	double	max_drop_     [NO_CLASSES+1];	// in fraction of 1
	double	Rout_last_up_ [NO_CLASSES+1];	// last update of Rout's value
	double	avg_elapsed_  [NO_CLASSES+1];	// average time spent in the queue
	double	excess_drops_ [NO_CLASSES+1]; 
	
	double	util_;				// Offered load
	double	Kp_static_; 
	double	Kp_dynamic_;			// Proportional Controller Parameter
	int	ABS_present_; 
	double	monitoring_window_;
	double	last_monitor_update_;
	FILE*	hop_trace_;			// Trace File
};

#endif /* JOBS_H */

