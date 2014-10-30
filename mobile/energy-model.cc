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
 * $Header: /cvsroot/nsnam/ns-2/mobile/energy-model.cc,v 1.6 2005/06/13 17:50:41 haldar Exp $
 */

// Contributed by Satish Kumar <kkumar@isi.edu>

#include <stdarg.h>
#include <float.h>

#include "random.h"
#include "energy-model.h"
#include "mobilenode.h"
#include "god.h"

static class EnergyModelClass : public TclClass
{
public:
	EnergyModelClass ():TclClass ("EnergyModel") {}
	TclObject *create (int argc, const char *const *argv) {
		if (argc == 8) {
			MobileNode *n=(MobileNode*)TclObject::lookup(argv[4]);
			return (new EnergyModel(n, atof(argv[5]), 
						atof(argv[6]), atof(argv[7])));
		} else {
			Tcl::instance().add_error("Wrong arguments to ErrorModel");
			return 0;
		}
	}
} class_energy_model;

void EnergyModel::DecrTxEnergy(double txtime, double P_tx) 
{
	double dEng = P_tx * txtime;
	if (energy_ <= dEng)
		energy_ = 0.0;
	else
		energy_ = energy_ - dEng;
	if (energy_ <= 0.0)
		God::instance()->ComputeRoute();
//
	// This variable keeps track of total energy consumption in Transmission..
	et_=et_+dEng;
//
}


void EnergyModel::DecrRcvEnergy(double rcvtime, double P_rcv) 
{
	double dEng = P_rcv * rcvtime;
	if (energy_ <= dEng)
		energy_ = 0.0;
	else
		energy_ = energy_ - dEng;
	if (energy_ <= 0.0)
		God::instance()->ComputeRoute();
//
	// This variable keeps track of total energy consumption in RECV mode..
	er_=er_+dEng;
//
}

void EnergyModel::DecrIdleEnergy(double idletime, double P_idle) 
{
	double dEng = P_idle * idletime;
	if (energy_ <= dEng)
		energy_ = 0.0;
	else
		energy_ = energy_ - dEng;
	if (energy_ <= 0.0)
		God::instance()->ComputeRoute();
//
	// This variable keeps track of total energy consumption in IDLE mode..
	ei_=ei_+dEng;
//
}

//
void EnergyModel::DecrSleepEnergy(double sleeptime, double P_sleep) 
{
	double dEng = P_sleep * sleeptime;
	if (energy_ <= dEng)
		energy_ = 0.0;
	else
		energy_ = energy_ - dEng;
	if (energy_ <= 0.0)
		God::instance()->ComputeRoute();

	// This variable keeps track of total energy consumption in SLEEP mode..
	es_=es_+dEng;
}

void EnergyModel::DecrTransitionEnergy(double transitiontime, double P_transition) 
{
	double dEng = P_transition * transitiontime;
	if (energy_ <= dEng)
		energy_ = 0.0;
	else
		energy_ = energy_ - dEng;
	if (energy_ <= 0.0)
		God::instance()->ComputeRoute();
}
//

// XXX Moved from node.cc. These wireless stuff should NOT stay in the 
// base node.
void EnergyModel::start_powersaving()
{
	snh_ = new SoftNeighborHandler(this);
	snh_->start();
	
	afe_ = new AdaptiveFidelityEntity(this);
	afe_->start();

	state_ = EnergyModel::POWERSAVING;
	state_start_time_ = Scheduler::instance().clock();
}

void EnergyModel::set_node_sleep(int status)
{
	Tcl& tcl=Tcl::instance();
	//static float last_time_gosleep;
	// status = 1 to set node into sleep mode
	// status = 0 to put node back to idle mode.
	// time in the sleep mode should be used as credit to idle 
	// time energy consumption
	if (status) {
		last_time_gosleep = Scheduler::instance().clock();
		//printf("id=%d : put node into sleep at %f\n",
		// address_,last_time_gosleep);
		sleep_mode_ = status;
		if (node_->exist_namchan()) 
			tcl.evalf("%s add-mark m1 blue hexagon",node_->name());
	} else {
		sleep_mode_ = status;
		if (node_->exist_namchan()) 
			tcl.evalf("%s delete-mark m1", node_->name()); 
		//printf("id= %d last_time_sleep = %f\n",
		// address_, last_time_gosleep);
		if (last_time_gosleep) {
			total_sleeptime_ += Scheduler::instance().clock() -
				last_time_gosleep;
			last_time_gosleep = 0;
		}
	}	
}

void EnergyModel::set_node_state(int state)
{
	switch (state_) { 
	case POWERSAVING:
	case WAITING:
		state_ = state;
		state_start_time_ = Scheduler::instance().clock();
		break;
	case INROUTE:
		if (state == POWERSAVING) {
			state_ = state;
		} else if (state == INROUTE) {
			// a data packet is forwarded, needs to reset 
			// state_start_time_
			state_start_time_= Scheduler::instance().clock();
		}
		break;
	default:
		printf("Wrong state, quit...\n");
		abort();
	}
}

void EnergyModel::add_neighbor(u_int32_t nodeid)
{
	neighbor_list_item *np;
	np = neighbor_list.head;
	for (; np; np = np->next) {
		if (np->id == nodeid) {
			np->ttl = maxttl_;
			break;
		}
	}
	if (!np) {      // insert this new entry
		np = new neighbor_list_item;
		np->id = nodeid;
		np->ttl = maxttl_;
		np->next = neighbor_list.head;
		neighbor_list.head = np;
		neighbor_list.neighbor_cnt_++;
	}
}

void EnergyModel::scan_neighbor()
{
	neighbor_list_item *np, *lp;
	if (neighbor_list.neighbor_cnt_ > 0) {
		lp = neighbor_list.head;
		np = lp->next;
		for (; np; np = np->next) {
			np->ttl--;
			if (np->ttl <= 0){
				lp->next = np->next;
				delete np;
				np = lp;
				neighbor_list.neighbor_cnt_--;
			} 
			lp = np;
		}
		// process the first element
		np = neighbor_list.head;
		np->ttl--;
		if (np->ttl <= 0) {
			neighbor_list.head = np->next;
			delete np;
			neighbor_list.neighbor_cnt_--;
		}
	}
}


void SoftNeighborHandler::start()
{
	Scheduler::instance().schedule(this, &intr, CHECKFREQ);
}

void SoftNeighborHandler::handle(Event *)
{
	Scheduler &s = Scheduler::instance();
	nid_->scan_neighbor();
	s.schedule(this, &intr, CHECKFREQ);
}

void AdaptiveFidelityEntity::start()
{
	sleep_time_ = 2;
	sleep_seed_ = 2;
	idle_time_ = 10;
	nid_->set_node_sleep(0);
	Scheduler::instance().schedule(this, &intr, 
				       Random::uniform(0, idle_time_));
}

void AdaptiveFidelityEntity::handle(Event *)
{
	Scheduler &s = Scheduler::instance();
	int node_state = nid_->state();
	switch (node_state) {
	case EnergyModel::POWERSAVING:
		if (nid_->sleep()) {
			// node is in sleep mode, wake it up
			nid_->set_node_sleep(0);
			adapt_it();
			s.schedule(this, &intr, idle_time_);
		} else {
			// node is in idle mode, put it into sleep
			nid_->set_node_sleep(1);
			adapt_it();
			s.schedule(this, &intr, sleep_time_);
		}
		break;
	case EnergyModel::INROUTE:
		// 100s is the maximum INROUTE time.
		if (s.clock()-(nid_->state_start_time()) < 
		    nid_->max_inroute_time()) {
			s.schedule(this, &intr, idle_time_);
		} else {
			nid_->set_node_state(EnergyModel::POWERSAVING);
			adapt_it();
			nid_->set_node_sleep(1);
			s.schedule(this, &intr, sleep_time_); 
		}
		break;
	case EnergyModel::WAITING:
		// 10s is the maximum WAITING time
		if (s.clock()-(nid_->state_start_time()) < MAX_WAITING_TIME) {
			s.schedule(this, &intr, idle_time_);
		} else {
			nid_->set_node_state(EnergyModel::POWERSAVING);
			adapt_it();
			nid_->set_node_sleep(1);
			s.schedule(this, &intr, sleep_time_); 
		}
		break;
	default:
		fprintf(stderr, "Illegal Node State!");
		abort();
	}
}

void AdaptiveFidelityEntity::adapt_it()
{
	float delay;
	// use adaptive fidelity
	if (nid_->adaptivefidelity()) {
		int neighbors = nid_->getneighbors();
		if (!neighbors) 
			neighbors = 1;
		delay = sleep_seed_ * Random::uniform(1,neighbors); 
	      	set_sleeptime(delay);
	}
}

