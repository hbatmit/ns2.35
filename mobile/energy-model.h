/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- 
 *
 * Copyright (c) 1997, 2000 Regents of the University of California.
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
 * $Header: /cvsroot/nsnam/ns-2/mobile/energy-model.h,v 1.13 2005/06/14 19:43:48 haldar Exp $
 */

// Contributed by Satish Kumar (kkumar@isi.edu)

#ifndef ns_energy_model_h_
#define ns_energy_model_h_

#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "config.h"
#include "trace.h"
#include "rng.h"

const int CHECKFREQ = 1;
const int MAX_WAITING_TIME = 11;

class EnergyModel;

class AdaptiveFidelityEntity : public Handler {
public:  
	AdaptiveFidelityEntity(EnergyModel *nid) : nid_(nid) {} 

	virtual void start();
	virtual void handle(Event *e);

	virtual void adapt_it();
	inline void set_sleeptime(float t) {sleep_time_ = t;}
	inline void set_sleepseed(float t) {sleep_seed_ = t;}

protected:
        EnergyModel *nid_;
	Event intr;
	float  sleep_time_;
	float sleep_seed_;
	float  idle_time_;
};

class SoftNeighborHandler : public Handler {
public:
	SoftNeighborHandler(EnergyModel *nid) {
		nid_ = nid;
	}
	virtual void start();
	virtual void handle(Event *e); 
protected:
	EnergyModel *nid_;
	Event  intr;
};

class MobileNode;
class EnergyModel : public TclObject {
public:
	EnergyModel(MobileNode* n, double energy, double l1, double l2) :
		energy_(energy), er_(0), et_(0),ei_(0), es_(0), 
		initialenergy_(energy), 
		level1_(l1), level2_(l2), node_(n), 
		sleep_mode_(0), total_sleeptime_(0), total_rcvtime_(0), 
		total_sndtime_(0), powersavingflag_(0), 
		last_time_gosleep(0), max_inroute_time_(300), maxttl_(5), 
		adaptivefidelity_(1),  node_on_(true)
	{
		neighbor_list.neighbor_cnt_ = 0;
		neighbor_list.head = NULL;
	}

	inline double energy() const { return energy_; }
//
	inline double et() const { return et_; }
	inline double er() const { return er_; }
	inline double ei() const { return ei_; }
	inline double es() const { return es_; }
//
	inline double initialenergy() const { return initialenergy_; }
	inline double level1() const { return level1_; }
	inline double level2() const { return level2_; }
	inline void setenergy(double e) { energy_ = e; }
   
	virtual void DecrTxEnergy(double txtime, double P_tx);
	virtual void DecrRcvEnergy(double rcvtime, double P_rcv);
	virtual void DecrIdleEnergy(double idletime, double P_idle);
//
	virtual void DecrSleepEnergy(double sleeptime, double P_sleep);
	virtual void DecrTransitionEnergy(double transitiontime, double P_transition);
//	
	inline virtual double MaxTxtime(double P_tx) {
		return(energy_/P_tx);
	}
	inline virtual double MaxRcvtime(double P_rcv) {
		return(energy_/P_rcv);
	}
	inline virtual double MaxIdletime(double P_idle) {
		return(energy_/P_idle);
	}

	void add_neighbor(u_int32_t);      // for adaptive fidelity
	void scan_neighbor();
	inline int getneighbors() { return neighbor_list.neighbor_cnt_; }

	double level1() { return level1_; }
	double level2() { return level2_; }
	inline int sleep() { return sleep_mode_; }
	inline int state() { return state_; }
	inline float state_start_time() { return state_start_time_; }
	inline float& max_inroute_time() { return max_inroute_time_; }
	inline int& adaptivefidelity() { return adaptivefidelity_; }
	inline int& powersavingflag() { return powersavingflag_; }
	inline bool& node_on() { return node_on_; }
	inline float& total_sndtime() { return total_sndtime_; }
	inline float& total_rcvtime() { return total_rcvtime_; }
	inline float& total_sleeptime() { return total_sleeptime_; }
//
	inline float& total_idletime()	{	return total_idletime_;}
//
	inline AdaptiveFidelityEntity* afe() { return afe_; }
	inline int& maxttl() { return maxttl_; }

	virtual void set_node_sleep(int);
	virtual void set_node_state(int);
	virtual void add_rcvtime(float t) {total_rcvtime_ += t;}
	virtual void add_sndtime(float t) {total_sndtime_ += t;}
//
	virtual void add_sleeptime(float t) {total_sleeptime_ += t;};
//
	void start_powersaving();

	// Sleeping state
	enum SleepState { WAITING = 0, POWERSAVING = 1, INROUTE = 2 };

protected:
	double energy_;
//
	double er_; // Total energy consumption in RECV
	double et_; // Total energy consumption in transmission
	double ei_; // Total energy consumption in IDLE mode
	double es_; // Total energy consumption in SLEEP mode
//	
	double initialenergy_;
	double level1_;
	double level2_;

	MobileNode *node_;

	// XXX this structure below can be implemented by ns's LIST
	struct neighbor_list_item {
		u_int32_t id;        		// node id
		int       ttl;    		// time-to-live
		neighbor_list_item *next; 	// pointer to next item
	};

	struct {
		int neighbor_cnt_;   // how many neighbors in this list
		neighbor_list_item *head; 
	} neighbor_list;
	SoftNeighborHandler *snh_;

      	int sleep_mode_;	 // = 1: radio is turned off
	float total_sleeptime_;  // total time of radio in off mode
       	float total_rcvtime_;	 // total time in receiving data
	float total_sndtime_;	 // total time in sending data
//
	float total_idletime_;	// total time in idle mode
//
	int powersavingflag_;    // Is BECA activated ?
	float last_time_gosleep; // time when radio is turned off
	float max_inroute_time_; // maximum time that a node can remaining
				 // active 
	int maxttl_;		 // how long a node can keep its neighbor
				 // list. For AFECA only.
	int state_;		 // used for AFECA state 
	float state_start_time_; // starting time of one AFECA state
	int adaptivefidelity_;   // Is AFECA activated ?
       	AdaptiveFidelityEntity *afe_;

	bool node_on_;   	 // on-off status of this node -- Chalermek
};


#endif // ns_energy_model_h
