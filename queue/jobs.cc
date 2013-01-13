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
 * $Id: jobs.cc,v 1.5 2011/10/02 22:32:34 tom_henderson Exp $                   
 * 							              
 */

#include "jobs.h"

static class JoBSClass : public TclClass {
public:
  JoBSClass() : TclClass("Queue/JoBS") {}
  TclObject* create(int, const char*const*) {
    return (new JoBS);
  }
} class_jobs;

JoBS::JoBS() : link_(NULL) {
	for (int i=1; i<=NO_CLASSES; i++) 
		cls_[i]	= new PacketQueue;

	bind_bool("drop_front_", &drop_front_);
	bind_bool("trace_hop_", &trace_hop_);
	bind("mean_pkt_size_", &mean_pkt_size_);  // avg pkt size
	bind("adc_resolution_type_", &adc_resolution_type_);
	bind_bool("shared_buffer_", &shared_buffer_);
	total_backlog_Pkts_ 	= 0;
	total_backlog_Bits_ 	= 0;
	pkt_count_ = 0;
	idle_ = 1;
	ABS_present_ = FALSE;
	monitoring_window_ = 0;
	last_arrival_ = 0;
	last_monitor_update_ = 0;
	sliding_arv_pkts_=0;
	util_ = 0;

	for (int i=1; i<=NO_CLASSES; i++) {
		RDC_[i] = (double)i; 
		RLC_[i] = (double)i;
		ADC_[i] = INFINITY;
		ALC_[i] = INFINITY;
		sliding_serviced_pkts_[i] = 0;
		sliding_serviced_bits_[i] = 0;
		sliding_arv_pkts_c[i] = 0;
		sliding_class_delay_[i] = 0;
		sliding_class_service_rate_[i] = 0;
		last_xmit_[i] = 0;
		
		avg_elapsed_[i] = 0.0;
		excess_drops_[i] = 0.0;
		Rout_last_up_[i] = 0.0;
		min_drop_[i] = 0.0;
		max_drop_[i] = 1.0;
		error_[i] = 0.0;
		min_rate_[i] = 0.0;
		backlog_Pkts_[i] = 0;
		backlog_Bits_[i] = 0;
		service_rate_[i] = 0;
		current_loss_[i] = 0;
		Rout_[i] = 0;
		Rin_[i] = 0;
		Rout_th_[i] = 0;
		last_rate_update_[i] = 0;
	}  
}

int JoBS::command(int argc, const char*const* argv) {
	Tcl& tcl = Tcl::instance();
	double tmp[NO_CLASSES+1];

	if (argc >= NO_CLASSES+2) { /* input RDCs/RLCs/ADCs/ALCs */
		if (strcmp(argv[1], "init-rdcs") == 0) {
			RDC_[0] = -1;
			for (int i=1; i<=NO_CLASSES; i++) {
				RDC_[i] = atof(argv[i+1]);
				if (RDC_[i] == -1)
					concerned_RDC_[i] = FALSE;
				else
				  concerned_RDC_[i] = TRUE;
			}
			for (int i=1; i<=NO_CLASSES;i++) {
				tmp[i] = 1.0;
				for (int j = 1; j <= i-1; j++) 
				  if (concerned_RDC_[j]) tmp[i] *= RDC_[j];
			}

			for (int i=1; i<=NO_CLASSES; i++) {
				prod_others_[i] = 1.0;
				for (int j = 1; j <= NO_CLASSES; j++) 
					if ((j != i)&&(concerned_RDC_[j]))
					  prod_others_[i] *= tmp[j];
			}
			if (argc == NO_CLASSES+2) {
				fprintf(stdout, "\nConfigured RDC, with:\n");
				for (int i=1; i<=NO_CLASSES; i++) {
					fprintf(stdout, "\tClass %d: ",i);
					if (concerned_RDC_[i]) 
						fprintf(stdout, "\t%f\t(%f)\n", (double)RDC_[i], (double) prod_others_[i]);
					else 
						fprintf(stdout, "\tNot concerned\n");
				}
			}
			return (TCL_OK);
		}

		if (strcmp(argv[1], "init-rlcs") == 0) {
			RLC_[0] = -1;
			for (int i=1; i<=NO_CLASSES; i++) {
				RLC_[i] = atof(argv[i+1]);
				if (RLC_[i] == -1)
					concerned_RLC_[i] = FALSE;
				else
					concerned_RLC_[i] = TRUE;
			}

			for (int i=1; i<=NO_CLASSES;i++) {
				tmp[i] = 1.0;
				for (int j = 1; j <= i-1; j++) 
					if (concerned_RLC_[j]) 
						tmp[i] *= RLC_[j];
			}

			for (int i=1; i<=NO_CLASSES; i++) {
				loss_prod_others_[i] = 1.0;
				for (int j = 1; j <= NO_CLASSES; j++) 
					if ((j != i)&&(concerned_RLC_[j]))
						loss_prod_others_[i] *= tmp[j];
			}
			if (argc == NO_CLASSES+2) {
				fprintf(stdout, "\nConfigured RLC, with:\n");
				for (int i=1; i<=NO_CLASSES; i++) {
					fprintf(stdout, "\tClass %d: ",i);
					if (concerned_RLC_[i]) 
						fprintf(stdout, "\t%f\t(%f)\n", (double)RLC_[i], (double)loss_prod_others_[i]);
					else 
						fprintf(stdout, "\tNot concerned\n");
				}
			}
			
			return (TCL_OK);
		}
		if (strcmp(argv[1], "init-adcs") == 0) {
			ADC_[0] = -1;
			for (int i=1; i<=NO_CLASSES; i++) {
				ADC_[i] = atof(argv[i+1]);
				if (ADC_[i] == -1) {
					concerned_ADC_[i] = FALSE;
					ADC_[i] = INFINITY;
				} else {
					concerned_ADC_[i] = TRUE;
					ABS_present_ = TRUE;
				}
			}
			if (argc == NO_CLASSES+2) {
				fprintf(stdout, "\nConfigured ADC, with:\n");
				for (int i=1; i<=NO_CLASSES; i++) {
					fprintf(stdout, "\tClass %d: ", i);
					if (concerned_ADC_[i]) 
						fprintf(stdout, "\t%f secs\n", (double)ADC_[i]);
					else 
						fprintf(stdout, "\tNot concerned\n");
				}
			}

			return (TCL_OK);
		}
		if (strcmp(argv[1], "init-alcs") == 0) {
			ALC_[0] = -1;
			for (int i=1; i<=NO_CLASSES; i++) {
				ALC_[i] = atof(argv[i+1]);
				if (ALC_[i] == -1) {
					concerned_ALC_[i] = FALSE;
					ALC_[i] = INFINITY;
				} else
					concerned_ALC_[i] = TRUE;
			}
			if (argc == NO_CLASSES+2) {
				fprintf(stdout, "\nConfigured ALC, with:\n");
				for (int i=1; i<=NO_CLASSES; i++) {
					fprintf(stdout, "\tClass %d: ",i);
					if (concerned_ALC_[i])  
						fprintf(stdout, "\t%f\n", (double)ALC_[i]);
					else 
						fprintf(stdout, "\tNot concerned\n");	
				}
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "init-arcs") == 0) {
			ARC_[0] = -1;
			for (int i=1; i<=NO_CLASSES; i++) {
				ARC_[i] = atof(argv[i+1]);
				if (ARC_[i] == -1) {
					concerned_ARC_[i] = FALSE;
					ARC_[i] = 0;
				} else {
					concerned_ARC_[i] = TRUE;
					ABS_present_ = TRUE;
				}
			}
			if (argc == NO_CLASSES+2) {
				fprintf(stdout, "\nConfigured ARC, with:\n");
				for (int i=1; i<=NO_CLASSES; i++) {
					fprintf(stdout, "\tClass %d: ",i);
					if (concerned_ARC_[i])  
						fprintf(stdout, "\t%f\n", (double)ARC_[i]);
					else 
						fprintf(stdout, "\tNot concerned\n");	
				}
			}
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "link") == 0) {
			LinkDelay* del = (LinkDelay*)TclObject::lookup(argv[2]);
			if (del == 0) {
				tcl.resultf("JoBS: no LinkDelay object %s",
					    argv[2]);
				return(TCL_ERROR);
			}
			link_ = del;
			for (int i=1; i <= NO_CLASSES; i++)
				service_rate_[i] = 0;
			return (TCL_OK);
		}
		if (strcmp(argv[1], "sampling-period") == 0) {
			sampling_period_ = atoi(argv[2]);
			if (sampling_period_ < 0) {
				fprintf(stderr, "JoBS: sampling period must be positive!!\n");
				abort();
				return (TCL_ERROR); // __NOT REACHED__
			} else 
				return (TCL_OK);      
		}
		if (strcmp(argv[1], "id") == 0) {
			link_id_ = (int)atof(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "trace-file") == 0) {
			file_name_ = new(char[500]);
			strcpy(file_name_, argv[2]);
			if (strcmp(file_name_, "null")) {
				trace_hop_ = true;
			} else {
				trace_hop_ = false;
			}
			return (TCL_OK);
		}
	} else if (argc == 2) {
		if (strcmp(argv[1], "initialize") == 0) {
			double myMaxPi=0;
			for (int i = 1; i <= NO_CLASSES; i++) 
				if (prod_others_[i] > myMaxPi) 
					myMaxPi = prod_others_[i];

			Kp_static_ = -2/(double)(myMaxPi*mean_pkt_size_*sampling_period_);

      // Open files
			if (trace_hop_) {
				if ((hop_trace_ = fopen(file_name_, "w"))==NULL) {
					fprintf(stderr, "Problem with opening the per-hop trace file: %s\n", file_name_);
					abort();	  
				}
			}    
			return (TCL_OK);
		}
		if (strcmp(argv[1], "copyright-info") == 0) {
			fprintf(stdout, "\n----------------------------------------------------------\n\n");
			fprintf(stdout, "JoBS scheduler/dropper [prototype ns-2 implementation]\n");
			fprintf(stdout, "Version 1.0 (CVS Revision: $Id: jobs.cc,v 1.5 2011/10/02 22:32:34 tom_henderson Exp $)\n\n");
			fprintf(stdout, "ns-2 implementation by Nicolas Christin <nicolas@cs.virginia.edu>\n");
			fprintf(stdout, "JoBS algorithms proposed by Nicolas Christin and Jorg Liebeherr.\n");
			fprintf(stdout, "Grateful acknowledgments to Tarek Abdelzaher for his help and comments.\n");
			fprintf(stdout, "Visit http://qosbox.cs.virginia.edu for more\n");
			fprintf(stdout, "information.\n\n");
			fprintf(stdout, "Copyright (c) 2000-2002 by the Rector and Board of Visitors of the\n");
			fprintf(stdout, "University of Virginia.\n");
			fprintf(stdout, "All Rights Reserved.\n");
			fprintf(stdout, "----------------------------------------------------------\n\n");
			return (TCL_OK);
		}
	}
	return (Queue::command(argc, argv));
}

// *******************************************************************
// JoBS: Simplified Packetization of the Fluid Flow Model.
// *******************************************************************

Packet* JoBS::deque() { // Lower classes are better. 
	double 	cur_time  = Scheduler::instance().clock(); 
	double	error;
	double	maxError;
	int	svc_class = 0;
  
	error = 0;

	// Fast translation of service rates into packet forwarding decisions

	maxError = -INFINITY;
	for (int i=1; i<=NO_CLASSES; i++) {
		if (cls_[i]->head()) {
			error = (double)Rout_th_[i] - Rout_[i];
			if (error > maxError) {
				maxError = error;
				svc_class = i;
			}
		}
	}

	if (svc_class == 0) {
		idle_ = 1;
		if (&Scheduler::instance() != NULL)
			idletime_ = cur_time;
		else
			idletime_ = 0.0;
		return NULL; /* Idle system */
	} else {
		idle_ = 0;
		Packet* p = cls_[svc_class]->deque();
		hdr_cmn* cm_h = hdr_cmn::access(p);
    
		total_backlog_Pkts_--;
		total_backlog_Bits_ -= 8*cm_h->size();
		backlog_Pkts_[svc_class]--;
		backlog_Bits_[svc_class] -= 8*cm_h->size();
		Rout_[svc_class] += 8*cm_h->size();
		Rout_last_up_[svc_class] = cur_time;
		
		sliding_serviced_pkts_[svc_class] ++;
		sliding_serviced_bits_[svc_class] += 8*cm_h->size();
		sliding_class_delay_[svc_class] 
		  = ((sliding_serviced_pkts_[svc_class]-1)*(sliding_class_delay_[svc_class])
		     +(cur_time-cm_h->ts_))/(sliding_serviced_pkts_[svc_class]);
		last_xmit_[svc_class] = cur_time+(8.*cm_h->size()/(double)link_->bandwidth());
		return(p);
	}
}

// *******************************************************************
// JoBS: Enqueue packet 
// *******************************************************************

void JoBS::enque(Packet* pkt) {
	double* DeltaR;
	int i;
	hdr_ip*  ip_h = hdr_ip::access(pkt);
	hdr_cmn* cm_h = hdr_cmn::access(pkt);
	
	int prio = ip_h->prio_;
	if ((prio < 1) || (prio > NO_CLASSES)) {
		printf("Bad priority value\n");
		abort();
	}
  
	double cur_time = Scheduler::instance().clock(); 
	cm_h->ts_ = cur_time; // timestamp the packet
	cls_[prio]->enque(pkt);
	
	monitoring_window_ -= cur_time-last_monitor_update_;
	last_monitor_update_ = cur_time;
	if (monitoring_window_ <= 0) {
		updateStats(pkt, RESET_STATS);
		monitoring_window_ = MON_WINDOW_SIZE;
	} else {
		updateStats(pkt, UPDATE_STATS);
	}
	
	// Reset arrival, input, and service curves if the scheduler is 
	// not busy

	if (idle_) {
		for (i=1; i<= NO_CLASSES; i++) {
		  Arrival_[i]=0.0;
		  Rin_[i]=0.0;
		  Rout_[i]=0.0;
		  Rout_th_[i]=0.0;
		  last_rate_update_[i] = 0;
		  Rout_last_up_[i] = cur_time;
		  idle_ = 0;
		}
	}
	
	arvAccounting(pkt);
	
	/* Is the buffer full? */  
	if (!shared_buffer_ && backlog_Pkts_[prio] > qlim_/NO_CLASSES) {
		/* separate buffers: no guarantees on packet drops 
		 * can be offered
		 * thus we drop the incoming packet
		 */
		if (drop_front_)
			dropFront(prio, WITH_UPDATE);
		else
			dropTail(prio, WITH_UPDATE);    
	} else {
	  /* shared buffer */
		if (total_backlog_Pkts_ > qlim_) {
			if (!concerned_RLC_[prio]) {
				if (!concerned_ALC_[prio]) {
					// No ALC, no RLC on that class. Drop the incoming packet.
					if (drop_front_)
						dropFront(prio, WITH_UPDATE);
					else
						dropTail(prio, WITH_UPDATE);
				} else {
					// No RLC on that class. Drop the incoming packet if you can...
					if (current_loss_[prio]+(double)8*cm_h -> size()/(double)Arrival_[prio] <= ALC_[prio]) {
						if (drop_front_)
							dropFront(prio, WITH_UPDATE);
						else	    
							dropTail(prio, WITH_UPDATE);
					} else {
						// If it doesn't work, pick another class
						if (drop_front_)
							dropFront(pickDroppedRLC(RESOLVE_OVERFLOW), WITH_UPDATE);
						else	    
							dropTail(pickDroppedRLC(RESOLVE_OVERFLOW), WITH_UPDATE);
					}
				}
			} else {
				// RLC on that class (w/ or w/o ALC), pick a class according to the RLC
			  if (drop_front_)
				dropFront(pickDroppedRLC(RESOLVE_OVERFLOW), WITH_UPDATE);
			  else	    
				dropTail(pickDroppedRLC(RESOLVE_OVERFLOW), WITH_UPDATE);
			}
		}
	}
	pkt_count_ ++;
	enforceWC(); // Change rates if some classes have entered/left the system
	// since the last update

	if ((ABS_present_)&&(!minRatesNeeded(ip_h -> prio()))) {
		DeltaR = assignRateDropsADC();   

		if (DeltaR != NULL) {
			for (i = 1; i <= NO_CLASSES; i++) 
				service_rate_[i] += DeltaR[i];
			delete []DeltaR; 
		}

		DeltaR = adjustRatesRDC();
		
		if (DeltaR) {
			for (i = 1; i <= NO_CLASSES; i++) 
				service_rate_[i] += DeltaR[i];
			delete []DeltaR;   
		}

		pkt_count_ = 0;

	} else if (pkt_count_ >= sampling_period_) {
		/* Is it time for a new evaluation of the system? */
		DeltaR = adjustRatesRDC();
		if (DeltaR != NULL) {
			for (i = 1; i <= NO_CLASSES; i++)
				service_rate_[i] += DeltaR[i];
			delete []DeltaR;
		} 
		pkt_count_ = 0;
	}
	return;
}

// *******************************************************************
// Auxiliary functions (helpers) follow
// *******************************************************************

// 1) Scheduling Primitives

// *******************************************************************
// JoBS: Enforce Work-Conserving Property
// *******************************************************************

int JoBS::enforceWC() {
	int i;
	int activeClasses;
	int updated;
	
	updated = FALSE;
	activeClasses = 0;
	
	for (i = 1; i <= NO_CLASSES; i++) {
		if (cls_[i]->head()) 
			activeClasses++;
		if ((cls_[i] -> head() && service_rate_[i] <= PRECISION_ERROR)
		    ||(cls_[i] -> head() == NULL && service_rate_[i] > 0)) 
		  updated = TRUE;
	}

	if (updated) {
		for (i = 1; i <= NO_CLASSES; i++) {
			if (cls_[i]->head() == NULL) {
				service_rate_[i] = 0;
			} else {
				if (activeClasses > 0)
					service_rate_[i] = link_->bandwidth()/(double)activeClasses;
				else 
					service_rate_[i] = 0 ;
			}
		}
	}
	
	return (updated);
}

// *******************************************************************
// JoBS: Adjusts the rates for meeting the RDCs
// *******************************************************************

double* JoBS::adjustRatesRDC() {
	int i, j;
	int RDC_Classes, activeClasses;
	double* result;
	double credit, available, lower_bound, upper_bound;
	double bk;
	
	activeClasses = 0;
	RDC_Classes = 0;

	upper_bound = link_ -> bandwidth();

	for (i = 1; i <= NO_CLASSES; i++)
		if (cls_[i]->head() != NULL) {
			activeClasses++;
			if (concerned_RDC_[i]) 
				RDC_Classes++;
			else 
				upper_bound -= service_rate_[i];
		}

	result = new double[NO_CLASSES+1];

	if (result == NULL) 
		return NULL;

	for (i = 0; i <= NO_CLASSES; i++) 
		result[i] = 0;

	if (upper_bound < PRECISION_ERROR || activeClasses == 0) return result;
	
	credit = 0;
	lower_bound = 0;
	
	updateError();
	min_share_ = 1.0;
	bk = 0;
	
	for (i = 1; i <= NO_CLASSES; i++) 
		if (concerned_RDC_[i])
			bk += Rin_[i];

	for (i = 1; i <= NO_CLASSES; i++) 
		if ((double)Rin_[i]/(double)bk < min_share_)
			min_share_ = (double)Rin_[i]/(double)bk;
  
	/*
	 * note that the formula for Kp is slightly different 
	 * from the formula derived in CS-2001-21. 
	 * the formula used here provides better results 
	 * at the expense of more complicated computations.
	 */
	
	Kp_dynamic_ = Kp_static_*pow(upper_bound, 2.)*min_share_*util_*max(util_, 1.0);

	credit = 0;
	for (i = 1; i <= NO_CLASSES; i++) {
		if ((cls_[i]->head() != NULL)&&(concerned_RDC_[i])) {
			result[i] = Kp_dynamic_*(error_[i]); 
		}
	}

	// saturation 

	for (i = 1; i <= NO_CLASSES; i++) 
		if (cls_[i]->head() != NULL) {
			if (concerned_RDC_[i]) {
				lower_bound += max(0, min_rate_[i]);
			} 
		}

	for (i = 1; i <= NO_CLASSES; i++) {
		if ((concerned_RDC_[i])&&(result[i] + service_rate_[i] > upper_bound)) {
			for (j = 0; j <= NO_CLASSES; j++) {
				if (concerned_RDC_[j]) {
					if (j == i) 
						result[j] = (upper_bound-service_rate_[j])  
						    + min_rate_[j] - lower_bound;
					else
						result[j] = -service_rate_[j]+min_rate_[j];
				}
			}
			return result;
		}
		if (concerned_RDC_[i] && result[i] + service_rate_[i] < min_rate_[i]) {
			credit += service_rate_[i]+result[i]-min_rate_[i]; 
			// "credit" is in fact a negative number
			result[i] = -service_rate_[i]+min_rate_[i];
		}
	}

	for (i = NO_CLASSES; (i > 0)&&(credit < 0); i--) {
		if ((cls_[i]->head() != NULL)&&(concerned_RDC_[i])) {
			available = result[i] + service_rate_[i]-min_rate_[i];
			if (available >= -credit) {
				result[i] += credit;
				credit = 0;
			} else {
				result[i] -= available;
				credit += available;
			}      
		}
	}
	return result;
}

// *******************************************************************
// JoBS: Assigns the rates (and possibly drops some traffic) 
// needed for meeting the ADCs
// *******************************************************************

double* JoBS::assignRateDropsADC() {
	double* x;
	//double myRatios[NO_CLASSES+1];
	double c[NO_CLASSES+1], n[NO_CLASSES+1];
	double k[NO_CLASSES+1], target[NO_CLASSES+1];
	double available[NO_CLASSES+1];
	double toDrop;
	int lowest, highest;
	int victim_class, keep_going;
	int i;
	Packet* p; 
	hdr_cmn* cm_h; 
	double cur_time = Scheduler::instance().clock(); 
	
	x = new double[NO_CLASSES+1];
	
	if (x == NULL) return NULL;
	
	for (i = 0;i <= NO_CLASSES; i++) 
		x[i] = 0;
	
	keep_going = TRUE;
	
	for (i = 1; i <= NO_CLASSES; i++) {
		if (cls_[i]->head() != NULL) {
			p = cls_[i]->head();
			cm_h = hdr_cmn::access(p);
			n[i] = (double)service_rate_[i];
			k[i] = (double)backlog_Bits_[i];
			available[i] = service_rate_[i];
			
			if (concerned_ADC_[i]) { 
				c[i] = (double)max((double)ADC_[i]-(cur_time-cm_h->ts_),1e-10);
				target[i] = (double)k[i]/(double)c[i];
				available[i] = -(target[i] - n[i]); 
			} 
			if (concerned_ARC_[i]) { 
				if (n[i] - ARC_[i] < available[i])
					available[i] = n[i] - ARC_[i];
			}
		} else {
			available[i] = service_rate_[i];
			n[i] = 0;
			k[i] = 0;
			c[i] = 0;
		}
	}
	
	// Step 1: Adjust rates 
	
	highest = 1;
	lowest  = NO_CLASSES;
	
	while ((highest < NO_CLASSES+1)&&(available[highest] >= 0))
		highest++; // which is the highest class that needs more service? 
	while ((lowest > 0)&&(available[lowest] <= 0))
		lowest--;  // which is the lowest class that needs less service? 
	
	
	while ((highest != NO_CLASSES+1)&&(lowest != 0)) {
		// give the excess service from lowest to highest 
		if (available[lowest]+available[highest] > 0) {
			// Still some "credit" left 
			// Give all that is needed by "highest" 
			n[lowest]  -= -available[highest];
			n[highest] += -available[highest];
			
			available[lowest]  -= -available[highest];
			available[highest] = 0;
			
			while ((highest < NO_CLASSES+1)&&(available[highest] >= 0))
				highest++;  // which is the highest class that needs more service? 
			
		} else if (available[lowest]+available[highest] == 0) {
			// No more credit left but it's fine 
			n[lowest]  -= -available[highest];
			n[highest] += -available[highest];
			
			available[highest] = 0;
			available[lowest]  = 0;
			
			while ((highest < NO_CLASSES+1)&&(available[highest] >= 0))
				highest++;  // which is the highest class that needs more service? 
			while ((lowest > 0)&&(available[lowest] <= 0))
				lowest--;   // which is the lowest class that needs less service? 

		} else if (available[lowest]+available[highest] < 0) {
			// No more credit left and we need to switch to another class 
			n[lowest]  -= available[lowest];
			n[highest] += available[lowest];

			available[highest] += available[lowest];
			available[lowest]  = 0;
			
			while ((lowest > 0)&&(available[lowest] <= 0))
				lowest--;  // which is the lowest class that needs less service? 
		}
	}
	
	for (i = 1; i <= NO_CLASSES; i++) {
		if (cls_[i]->head() != NULL)
			x[i] = n[i] - (double)service_rate_[i];
		else 
			x[i] = - (double)service_rate_[i] ;
	}
	
	// Step 2: Adjust drops 
	
	if (highest != NO_CLASSES+1) {
		// Some class still needs additional service 
		if (adc_resolution_type_ == SHARED_PAIN) {
			// Drop from all classes
			toDrop = 0;
			for (i = 1; i <= NO_CLASSES; i++) 
				if (available[i] < 0 && concerned_ADC_[i] && cls_[i]->head()) 
					toDrop += k[i] - c[i]*n[i];
			
			while ((toDrop > 0)&&(keep_going)) {
				victim_class = pickDroppedRLC(RESOLVE_ADC);
				if (drop_front_) 
					p = cls_[victim_class] -> head();
				else
					p = cls_[victim_class] -> tail();
	      
				cm_h = hdr_cmn::access(p);

				if (current_loss_[victim_class]+(double)8*cm_h -> size()/(double)Arrival_[victim_class] > ALC_[victim_class]) {
					keep_going = FALSE;
				} else {
					toDrop -= (double)8*cm_h -> size();

					if (drop_front_) 
						dropFront(victim_class, WITH_UPDATE);
					else
						dropTail(victim_class, WITH_UPDATE);
				}
			}	
		} else {
			// Drop solely from the class(es) that need(s) more service
			for (i = 1; i <= NO_CLASSES; i++) {
				if (available[i] < 0 && cls_[i] -> head()) {
					k[i] = c[i]*n[i];	

					while ((backlog_Bits_[i] > k[i])&&(keep_going)) {
						if (drop_front_) {
							p = cls_[i] -> head();
							cm_h = hdr_cmn::access(p);
							if (current_loss_[i]+(double)8*cm_h -> size()/(double)Arrival_[i] > ALC_[i]) {
								keep_going = FALSE;
							} else 
								dropFront(i, WITH_UPDATE);
						} else {
							p = cls_[i] -> tail();
							cm_h = hdr_cmn::access(p);
							if (current_loss_[i]+(double)8*cm_h -> size()/(double)Arrival_[i] > ALC_[i]) {
								keep_going = FALSE;
							} else 
								dropTail(i, WITH_UPDATE);
						}
					}
					k[i] = backlog_Bits_[i];
				}
			}      
		}
	} 

	for (i = 1; i <= NO_CLASSES; i++) 
		if (cls_[i]->head() && concerned_ADC_[i]) {
			if (c[i] > 0) {
				if (concerned_ADC_[i] && !concerned_ARC_[i]) {
					min_rate_[i] = (double)k[i]/(double)c[i];
				} else 
					min_rate_[i] = (double)n[i];
			} else {
				min_rate_[i] = INFINITY;
			}
		} else  if (cls_[i]->head() && concerned_ARC_[i]) {
			min_rate_[i] = n[i];
		} else {
		  min_rate_[i] = 0.0;    
		}
	return (x);
}

// 1bis) Scheduling Helpers

// *******************************************************************
// JoBS: Update Error - used by the RDC feedback loop controller
// *******************************************************************

void JoBS::updateError() {
	int i;
	int activeClasses, backloggedClasses;
	double meanWeightedDelay;

	meanWeightedDelay = 0;
	activeClasses = 0 ; 
	backloggedClasses = 0;
	
	for (i = 1; i <= NO_CLASSES; i++) 
		if (cls_[i]->head() != NULL) {
			backloggedClasses++;
			if (concerned_RDC_[i]) {
				meanWeightedDelay += prod_others_[i]*projDelay(i);
				activeClasses ++;
			}    
		}

	if (activeClasses > 0) 
		meanWeightedDelay /= (double)activeClasses;
	else if (backloggedClasses == 0) {
		fprintf(stderr, "JoBS::updateError() called but there's no backlog!\n");
		abort();
	}
	
	for (i = 1; i <= NO_CLASSES; i++) 
		if ((cls_[i]->head() != NULL)&&(concerned_RDC_[i])) {
			error_[i] = meanWeightedDelay-prod_others_[i]*projDelay(i);
		} else {
			error_[i] = 0.0; // either the class isn't concerned, or it's not backlogged
			// in any case, the rate shouldn't be adjusted.
		}
	return;
}
// *******************************************************************
// JoBS: Derives the rates needed for meeting the ADC.
// Returns true if the sum of the rates needed for meeting the ADC
// is less than the link's bandwidth, false otherwise.
// It's also the place where the "RED" algorithm is triggered.
// Side effect (desired): assigns a value to min_rate_[i]
// *******************************************************************

int JoBS::minRatesNeeded(int /*priority*/) {
	int result;
	int i;
	double cur_time;
	Packet* p;
	hdr_cmn* cm_h; 

	cur_time = Scheduler::instance().clock();
	result = TRUE;
  
	for (i = 1; i <= NO_CLASSES; i++) {
		if (cls_[i]->head() != 0 && (concerned_ADC_[i] || concerned_ARC_[i])) {
			p = cls_[i]->head();
			cm_h = hdr_cmn::access(p);

			if (concerned_ADC_[i]) { 
				if ((ADC_[i] - (cur_time-cm_h->ts_)) > 0 ) {
					// min rate needed for ADC
					min_rate_[i] = (double)(backlog_Bits_[i])
					    /(double)(ADC_[i] - (cur_time-cm_h->ts_));
					if (concerned_ARC_[i] && ARC_[i] > min_rate_[i]) {
						// min rate needed for ADC+ARC
						min_rate_[i] = ARC_[i];
					}
				} else 
					min_rate_[i] = INFINITY; 	
			} else if (concerned_ARC_[i]) {
				// no ADC, an ARC
				min_rate_[i] = ARC_[i];
			}
			if (min_rate_[i] > service_rate_[i]) 
				result = FALSE;
		} else 
			min_rate_[i] = 0.0;
	}
	return result;
}

// *******************************************************************
// JoBS: Returns the value of the delay metric for class i
// *******************************************************************

double JoBS::projDelay(int i) {
	double cur_time  = Scheduler::instance().clock(); 
	if (cls_[i]->head() != NULL) {
		Packet* p = cls_[i]->head();
		hdr_cmn* cm_h = hdr_cmn::access(p);
		return (cur_time - cm_h -> ts_);
	} else return 0.0; // __NOT REACHED__
}

// 2) Dropping Primitives

// *******************************************************************
// JoBS: Choose which class to drop from in case of a buffer overflow
// *******************************************************************

int JoBS::pickDroppedRLC(int mode) {
	double Mean;
	double loss_error[NO_CLASSES+1];
	int i, activeClasses, backloggedClasses;
	int class_dropped;
	double maxError;
	double maxALC;
	double cur_time  = Scheduler::instance().clock(); 

	hdr_cmn* cm_h; 
	
	class_dropped = -1;
	maxError = 0;
	Mean = 0;
	activeClasses = 0;
	backloggedClasses = 0;
	
	for (i = 1; i <= NO_CLASSES; i++) 
		if (cls_[i]->head() != NULL) {
			backloggedClasses ++;
			if (concerned_RLC_[i]) {
				Mean += loss_prod_others_[i]*current_loss_[i];
				activeClasses ++;
			}    
		}
	
	if (activeClasses > 0) 
		Mean /= (double)activeClasses;
	else if (backloggedClasses == 0) {
		fprintf(stderr, "JoBS::pickDroppedRLC() called but there's no backlog!\n");
		abort();
	}

	if (activeClasses == 0) 
		class_dropped = NO_CLASSES+1; // no classes are concerned by RLCs (NO_CLASSES+1 means "ignore RLC")
	else {
		for (i = 1; i <= NO_CLASSES; i++) 
			if ((cls_[i]->head() != NULL)&&(concerned_RLC_[i])) 
				loss_error[i]=loss_prod_others_[i]*current_loss_[i]-Mean;
			else 
				loss_error[i] = INFINITY; 
    
		for (i = 1; i <= NO_CLASSES; i++)
			if ((cls_[i]->head() != NULL)&&(loss_error[i] <= maxError)) {
				maxError = loss_error[i]; // Find out which class is the most below the mean
				class_dropped = i;   // It's the one that needs to be dropped
				// Ties are broken in favor of the higher priority classes (i.e., if
				// two classes present the same deviation, the lower priority class 
				// will get dropped).
			} 
		
		if (class_dropped != -1) {
			if (drop_front_)
				cm_h = hdr_cmn::access(cls_[class_dropped] -> head());
			else
				cm_h = hdr_cmn::access(cls_[class_dropped] -> tail());

			if (current_loss_[class_dropped]+(double)8*cm_h -> size()/(double)Arrival_[class_dropped] > ALC_[class_dropped])
				class_dropped = NO_CLASSES+1; // the class to drop for meeting the RLC will defeat the ALC: ignore RLC.
		} else 
			class_dropped = NO_CLASSES+1;
	}

	if (class_dropped != -1) { // this test is here only for "safety purposes" 
		// it should always return true at this point.
		if (class_dropped == NO_CLASSES+1) {
			maxALC = -INFINITY;
			for (i = 1; i <= NO_CLASSES; i++) {
				if (cls_[i] -> tail() != NULL) {
					if (ALC_[i]-current_loss_[i] > maxALC)
					{
						;
					}
					maxALC = ALC_[i]-current_loss_[i]; // pick the class which is the furthest from its ALC
					class_dropped = i;
				}
			}
			if (drop_front_)
			  cm_h = hdr_cmn::access(cls_[class_dropped]->head());
			else
			  cm_h = hdr_cmn::access(cls_[class_dropped]->tail());
			if ((mode == RESOLVE_OVERFLOW)
			    &&(current_loss_[class_dropped]+(double)8*cm_h -> size()/(double)Arrival_[class_dropped] > ALC_[class_dropped])) {

				fprintf(stderr, "*** Warning at time t=%f: ALC violated in class %d on link %d\n(reason: buffer overflow impossible to resolve otherwise)\nPkt size=%d bits\told_loss[%d]=%f\tnew_loss[%d]=%f\tArrival_[%d]=%f\tALC[%d]=%f\n",
					cur_time, link_id_,
					class_dropped, 8*cm_h -> size(),
					class_dropped, current_loss_[class_dropped],
					class_dropped, current_loss_[class_dropped]+(double)8*cm_h -> size()/(double)Arrival_[class_dropped],
					class_dropped, (double)Arrival_[class_dropped],
					class_dropped, ALC_[class_dropped]);
			}
		}		
	} else {
		fprintf(stderr, "Trying to drop a packet, but there's nothing in any queue!\n");
		abort();
	}
	return class_dropped;
}

// *******************************************************************
// JoBS: Drops traffic from the tail of the specified class, and 
// update the statistics.
// *******************************************************************

void JoBS::dropTail(int prio, int strategy) {
	Packet *p;
	hdr_cmn *cm_h;
	
	p = cls_[prio] -> tail();
	cm_h = hdr_cmn::access(p);
	
	total_backlog_Pkts_ --;
	total_backlog_Bits_ -= 8*cm_h->size();
	backlog_Pkts_[prio] --;
	backlog_Bits_[prio] -= 8*cm_h->size();
	Rin_[prio] -= 8*cm_h->size();
	if (strategy == WITH_UPDATE) {
		for (int i = 1; i <= NO_CLASSES; i++) 
			current_loss_[i] = min_drop_[i];
		current_loss_[prio] += (double)8*cm_h -> size()	
		    /(double)Arrival_[prio];
	}
	cls_[prio] -> remove(p);
	drop(p);
	return;
}

// *******************************************************************
// JoBS: Drops traffic from the head of the specified class, and 
// update the statistics.
// *******************************************************************

void JoBS::dropFront(int prio, int strategy) {
	Packet *p;
	hdr_cmn *cm_h;
	
	p = cls_[prio] -> head();
	cm_h = hdr_cmn::access(p);
	total_backlog_Pkts_ --;
	total_backlog_Bits_ -= 8*cm_h->size();
	backlog_Pkts_[prio] --;
	backlog_Bits_[prio] -= 8*cm_h->size();
	Rin_[prio] -= 8*cm_h->size();
	if (strategy == WITH_UPDATE) {
		for (int i = 1; i <= NO_CLASSES; i++) 
			current_loss_[i] = min_drop_[i];
		current_loss_[prio] += (double)8*cm_h -> size()	
		  /(double)Arrival_[prio];
	}
	cls_[prio] -> remove(p);
	drop(p);
	return;
}  


// 3) Other Helpers

// *******************************************************************
// JoBS: Update internal variables upon a packet arrival.
// *******************************************************************

void JoBS::arvAccounting(Packet* thePacket) {
	int i;
	double cur_time;
	hdr_cmn* cm_h;
	hdr_cmn* cm_h1;
	hdr_cmn* cm_h2; 
	hdr_ip*  ip_h; 
	
	cur_time = Scheduler::instance().clock();
	cm_h = hdr_cmn::access(thePacket);
	ip_h = hdr_ip::access(thePacket);
	
	/* Update curves */
	
	Arrival_[ip_h -> prio()]      += 8*cm_h -> size();
	Rin_    [ip_h -> prio()]      += 8*cm_h -> size();
	if (last_rate_update_[ip_h -> prio()] == 0) {
		last_rate_update_[ip_h -> prio()] = cur_time;
	} else {
		Rout_th_ [ip_h -> prio()]      += (cur_time-last_rate_update_[ip_h -> prio()])*service_rate_[ip_h -> prio()];
		last_rate_update_[ip_h -> prio()] = cur_time;
	}
	backlog_Pkts_[ip_h -> prio()] ++;
	backlog_Bits_[ip_h -> prio()] += 8*cm_h -> size();
	total_backlog_Bits_ += 8*cm_h -> size();
	total_backlog_Pkts_ ++;
	
	for (i = 1; i <= NO_CLASSES; i++) {
		if (Arrival_[i] > 0) {
			current_loss_[i] = (double)(Arrival_[i] - Rin_[i])/(double)(Arrival_[i]);
			min_drop_[i] = (double)(max(0.0, (1.0-(double)(Rin_[i])/(double)Arrival_[i])));
			max_drop_[i] = (double)(min(1.0, (1.0-(double)((double)Rout_[i]/(double)Arrival_[i]))));	
		} else {
			current_loss_[i] = 0;
			min_drop_[i] = 0.0;
			max_drop_[i] = 0.0;
		}
		
		if (cls_[i]->head() != NULL) {
			cm_h1 = hdr_cmn::access(cls_[i]->head());
			cm_h2 = hdr_cmn::access(cls_[i]->tail());
			avg_elapsed_[i] = (2*cur_time - cm_h1 -> ts_ - cm_h2 -> ts_)/((backlog_Pkts_[i]>1)? 2.0 :1.0);
		}
	}
	return;
}

// 4) Debugging Tools

void JoBS::updateStats(Packet* p, int action) {
	double cur_time = Scheduler::instance().clock();
	hdr_cmn* cm_h;
	hdr_ip* ip_h;
	
	cm_h = hdr_cmn::access(p);
	ip_h = hdr_ip::access(p);
	if (action == UPDATE_STATS) {
		sliding_arv_pkts_++;
		sliding_arv_pkts_c[ip_h->prio()]++;
		sliding_inter_ = (cur_time - last_arrival_ + sliding_inter_ * (sliding_arv_pkts_-1))
		    /(double)sliding_arv_pkts_;
		sliding_avg_pkt_size_ = (sliding_avg_pkt_size_*(sliding_arv_pkts_ - 1) + 8*cm_h->size())
		    /(double)sliding_arv_pkts_; 
		last_arrival_ = cur_time;
	} else if (action == RESET_STATS) {
		if (trace_hop_) {
			fprintf(hop_trace_, "%f\t", cur_time);
			for (int i=1;i<=NO_CLASSES;i++) {
				fprintf(hop_trace_, "%.8f\t", max(current_loss_[i],0.00000001));
			}
			for (int i=1;i<=NO_CLASSES;i++) {
				fprintf(hop_trace_, "%.8f\t", max(sliding_class_delay_[i],0.00000001));
			}
			for (int i=1;i<=NO_CLASSES;i++) {
				sliding_class_service_rate_[i] = sliding_serviced_bits_[i]/(double)MON_WINDOW_SIZE;
				fprintf(hop_trace_, "%.0f\t", max(sliding_class_service_rate_[i], 1));
				sliding_serviced_pkts_[i] = 0;
				sliding_serviced_bits_[i] = 0;
				sliding_class_delay_[i] = 0;
				sliding_class_service_rate_[i] = 0;
			}
			for (int i=1;i<=NO_CLASSES;i++) {
				fprintf(hop_trace_, "%.0f\t", (double)cls_[i]->length());
			}
			fprintf(hop_trace_, "\n");
		}
		sliding_arv_pkts_ = 1;
		sliding_arv_pkts_c[ip_h->prio()] = 1;
		sliding_inter_ = cur_time - last_arrival_;
		sliding_avg_pkt_size_ = 8*cm_h->size();
		last_arrival_ = cur_time;
	}
	util_ = sliding_avg_pkt_size_ / (sliding_inter_*link_->bandwidth());
	return;
}
