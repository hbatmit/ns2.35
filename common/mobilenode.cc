/*-*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- 
 *
 * Copyright (c) 1997 Regents of the University of California.
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
 * $Header: /cvsroot/nsnam/ns-2/common/mobilenode.cc,v 1.36 2006/02/22 13:21:52 mahrenho Exp $
 *
 * Code in this file will be changed in the near future. From now on it 
 * should be treated as for backward compatibility only, although it is in
 * active use by many other code in ns. - Aug 29, 2000
 */

/* 
 * CMU-Monarch project's Mobility extensions ported by Padma Haldar, 
 * 11/98.
 */


#include <math.h>
#include <stdlib.h>

#include "connector.h"
#include "delay.h"
#include "packet.h"
#include "random.h"
#include "trace.h"
#include "address.h"

#include "arp.h"
#include "topography.h"
#include "ll.h"
#include "mac.h"
#include "propagation.h"
#include "mobilenode.h"
#include "phy.h"
#include "wired-phy.h"
#include "god.h"

// XXX Must supply the first parameter in the macro otherwise msvc
// is unhappy. 
static LIST_HEAD(_dummy_MobileNodeList, MobileNode) nodehead = { 0 };

static class MobileNodeClass : public TclClass {
public:
        MobileNodeClass() : TclClass("Node/MobileNode") {}
        TclObject* create(int, const char*const*) {
                return (new MobileNode);
        }
} class_mobilenode;

/*
 *  PositionHandler()
 *
 *  Updates the position of a mobile node every N seconds, where N is
 *  based upon the speed of the mobile and the resolution of the topography.
 *
 */
void
PositionHandler::handle(Event*)
{
	Scheduler& s = Scheduler::instance();

#if 0
	fprintf(stderr, "*** POSITION HANDLER for node %d (time: %f) ***\n",
		node->address(), s.clock());
#endif
	/*
	 * Update current location
	 */
	node->update_position();

	/*
	 * Choose a new random speed and direction
	 */
#ifdef DEBUG
        fprintf(stderr, "%d - %s: calling random_destination()\n",
                node->address_, __PRETTY_FUNCTION__);
#endif
	node->random_destination();

	s.schedule(&node->pos_handle_, &node->pos_intr_,
		   node->position_update_interval_);
}


/* ======================================================================
   Mobile Node
   ====================================================================== */

MobileNode::MobileNode(void) : 
	pos_handle_(this)
{
	X_ = Y_ = Z_ = speed_ = 0.0;
	dX_ = dY_ = dZ_ = 0.0;
	destX_ = destY_ = 0.0;

	random_motion_ = 0;
	base_stn_ = -1;
	T_ = 0;

	log_target_ = 0;
	next_ = 0;
	radius_ = 0;

	position_update_interval_ = MN_POSITION_UPDATE_INTERVAL;
	position_update_time_ = 0.0;
	

	LIST_INSERT_HEAD(&nodehead, this, link_);	// node list
	LIST_INIT(&ifhead_);				// interface list
	bind("X_", &X_);
	bind("Y_", &Y_);
	bind("Z_", &Z_);
	bind("speed_", &speed_);
	
}

int
MobileNode::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if(argc == 2) {
		if(strcmp(argv[1], "start") == 0) {
		        start();
			return TCL_OK;
		} else if(strcmp(argv[1], "log-movement") == 0) {
#ifdef DEBUG
                        fprintf(stderr,
                                "%d - %s: calling update_position()\n",
                                address_, __PRETTY_FUNCTION__);
#endif
		        update_position();
		        log_movement();
			return TCL_OK;
		} else if(strcmp(argv[1], "log-energy") == 0) {
			log_energy(1);
			return TCL_OK;
		} else if(strcmp(argv[1], "powersaving") == 0) {
			energy_model()->powersavingflag() = 1;
			energy_model()->start_powersaving();
			return TCL_OK;
		} else if(strcmp(argv[1], "adaptivefidelity") == 0) {
			energy_model()->adaptivefidelity() = 1;
			energy_model()->powersavingflag() = 1;
			energy_model()->start_powersaving();
			return TCL_OK;
		} else if (strcmp(argv[1], "energy") == 0) {
			Tcl& tcl = Tcl::instance();
			tcl.resultf("%f", energy_model()->energy());
			return TCL_OK;
		} else if (strcmp(argv[1], "adjustenergy") == 0) {
			// assume every 10 sec schedule and 1.15 W 
			// idle energy consumption. needs to be
			// parameterized.
			idle_energy_patch(10, 1.15);
			energy_model()->total_sndtime() = 0;
			energy_model()->total_rcvtime() = 0;
			energy_model()->total_sleeptime() = 0;
			return TCL_OK;
		} else if (strcmp(argv[1], "on") == 0) {
			energy_model()->node_on() = true;
			tcl.evalf("%s set netif_(0)", name_);
			const char *str = tcl.result();
			tcl.evalf("%s NodeOn", str);
			God::instance()->ComputeRoute();
			return TCL_OK;
		} else if (strcmp(argv[1], "off") == 0) {
			energy_model()->node_on() = false;
			tcl.evalf("%s set netif_(0)", name_);
			const char *str = tcl.result();
			tcl.evalf("%s NodeOff", str);
			tcl.evalf("%s set ragent_", name_);
			str = tcl.result();
			tcl.evalf("%s reset-state", str);
			God::instance()->ComputeRoute();
		     	return TCL_OK;
		} else if (strcmp(argv[1], "shutdown") == 0) {
			// set node state
			//Phy *p;
			energy_model()->node_on() = false;
			
			//p = ifhead().lh_first;
			//if (p) ((WirelessPhy *)p)->node_off();
			return TCL_OK;
		} else if (strcmp(argv[1], "startup") == 0) {
			energy_model()->node_on() = true;
			return TCL_OK;
		}
	
	} else if(argc == 3) {
		if(strcmp(argv[1], "addif") == 0) {
			WiredPhy* phyp = (WiredPhy*)TclObject::lookup(argv[2]);
			if(phyp == 0)
				return TCL_ERROR;
			phyp->insertnode(&ifhead_);
			phyp->setnode(this);
			return TCL_OK;
		} else if (strcmp(argv[1], "setsleeptime") == 0) {
			energy_model()->afe()->set_sleeptime(atof(argv[2]));
			energy_model()->afe()->set_sleepseed(atof(argv[2]));
			return TCL_OK;
		} else if (strcmp(argv[1], "setenergy") == 0) {
			energy_model()->setenergy(atof(argv[2]));
			return TCL_OK;
		} else if (strcmp(argv[1], "settalive") == 0) {
			energy_model()->max_inroute_time() = atof(argv[2]);
			return TCL_OK;
		} else if (strcmp(argv[1], "maxttl") == 0) {
			energy_model()->maxttl() = atoi(argv[2]);
			return TCL_OK;
		} else if(strcmp(argv[1], "radius") == 0) {
                        radius_ = strtod(argv[2],NULL);
                        return TCL_OK;
                } else if(strcmp(argv[1], "random-motion") == 0) {
			random_motion_ = atoi(argv[2]);
			return TCL_OK;
		} else if(strcmp(argv[1], "addif") == 0) {
			WirelessPhy *n = (WirelessPhy*)
				TclObject::lookup(argv[2]);
			if(n == 0)
				return TCL_ERROR;
			n->insertnode(&ifhead_);
			n->setnode(this);
			return TCL_OK;
		} else if(strcmp(argv[1], "topography") == 0) {
			T_ = (Topography*) TclObject::lookup(argv[2]);
			if (T_ == 0)
				return TCL_ERROR;
			return TCL_OK;
		} else if(strcmp(argv[1], "log-target") == 0) {
			log_target_ = (Trace*) TclObject::lookup(argv[2]);
			if (log_target_ == 0)
				return TCL_ERROR;
			return TCL_OK;
		} else if (strcmp(argv[1],"base-station") == 0) {
			base_stn_ = atoi(argv[2]);
			if(base_stn_ == -1)
				return TCL_ERROR;
			return TCL_OK;
		} 
	} else if (argc == 4) {
		if (strcmp(argv[1], "idleenergy") == 0) {
			idle_energy_patch(atof(argv[2]),atof(argv[3]));
			return TCL_OK;
		}
	} else if (argc == 5) {
		if (strcmp(argv[1], "setdest") == 0) { 
			/* <mobilenode> setdest <X> <Y> <speed> */
#ifdef DEBUG
			fprintf(stderr, "%d - %s: calling set_destination()\n",
				address_, __FUNCTION__);
#endif
  
			if (set_destination(atof(argv[2]), atof(argv[3]), 
					    atof(argv[4])) < 0)
				return TCL_ERROR;
			return TCL_OK;
		}
	}
	return Node::command(argc, argv);
}


/* ======================================================================
   Other class functions
   ====================================================================== */
void
MobileNode::dump(void)
{
	Phy *n;
	fprintf(stdout, "Index: %d\n", address_);
	fprintf(stdout, "Network Interface List\n");
 	for(n = ifhead_.lh_first; n; n = n->nextnode() )
		n->dump();	
	fprintf(stdout, "--------------------------------------------------\n");
}

/* ======================================================================
   Position Functions
   ====================================================================== */
void 
MobileNode::start()
{
	Scheduler& s = Scheduler::instance();

	if(random_motion_ == 0) {
		log_movement();
		return;
	}

	assert(initialized());

	random_position();
#ifdef DEBUG
        fprintf(stderr, "%d - %s: calling random_destination()\n",
                address_, __PRETTY_FUNCTION__);
#endif
	random_destination();
	s.schedule(&pos_handle_, &pos_intr_, position_update_interval_);
}

void 
MobileNode::log_movement()
{
        if (!log_target_) 
		return;

	Scheduler& s = Scheduler::instance();
	sprintf(log_target_->pt_->buffer(),
		"M %.5f %d (%.2f, %.2f, %.2f), (%.2f, %.2f), %.2f",
		s.clock(), address_, X_, Y_, Z_, destX_, destY_, speed_);
	log_target_->pt_->dump();
}


void
MobileNode::log_energy(int flag)
{
	if (!log_target_) 
		return;
	Scheduler &s = Scheduler::instance();
	if (flag) {
		sprintf(log_target_->pt_->buffer(),"N -t %f -n %d -e %f", s.clock(),
			address_, energy_model_->energy()); 
	} else {
		sprintf(log_target_->pt_->buffer(),"N -t %f -n %d -e 0 ", s.clock(),
			address_); 
	}
	log_target_->pt_->dump();
}

//void
//MobileNode::logrttime(double t)
//{
//	last_rt_time_ = (int)t;
//}

void
MobileNode::bound_position()
{
	double minX;
	double maxX;
	double minY;
	double maxY;
	int recheck = 1;

	assert(T_ != 0);

	minX = T_->lowerX();
	maxX = T_->upperX();
	minY = T_->lowerY();
	maxY = T_->upperY();

	while (recheck) {
		recheck = 0;
		if (X_ < minX) {
			X_ = minX + (minX - X_);
			recheck = 1;
		}
		if (X_ > maxX) {
			X_ = maxX - (X_ - maxX);
			recheck = 1;
		}
		if (Y_ < minY) {
			Y_ = minY + (minY - Y_);
			recheck = 1;
		}
		if (Y_ > maxY) {
			Y_ = maxY- (Y_ - maxY);
			recheck = 1;
		}
		if (recheck) {
			fprintf(stderr, "Adjust position of node %d\n",address_);
		}
	}
}

int
MobileNode::set_destination(double x, double y, double s)
{
	assert(initialized());

	if(x >= T_->upperX() || x <= T_->lowerX())
		return -1;
	if(y >= T_->upperY() || y <= T_->lowerY())
		return -1;
	
	update_position();	// figure out where we are now
	
	destX_ = x;
	destY_ = y;
	speed_ = s;
	
	dX_ = destX_ - X_;
	dY_ = destY_ - Y_;
	dZ_ = 0.0;		// this isn't used, since flying isn't allowed

	double len;
	
	if (destX_ != X_ || destY_ != Y_) {
		// normalize dx, dy to unit len
		len = sqrt( (dX_ * dX_) + (dY_ * dY_) );
		dX_ /= len;
		dY_ /= len;
	}
  
	position_update_time_ = Scheduler::instance().clock();

#ifdef DEBUG
	fprintf(stderr, "%d - %s: calling log_movement()\n", 
		address_, __FUNCTION__);
#endif
	log_movement();

	/* update gridkeeper */
	if (GridKeeper::instance()){
		GridKeeper* gp =  GridKeeper::instance();
		gp-> new_moves(this);
	}                     

	if (namChan_ != 0) {
		
		double v = speed_ * sqrt( (dX_ * dX_) + (dY_ * dY_)); 
		
		sprintf(nwrk_,     
			"n -t %f -s %d -x %f -y %f -U %f -V %f -T %f",
			Scheduler::instance().clock(),
			nodeid_,
			X_, Y_,
			speed_ * dX_, speed_ * dY_,
			( v != 0) ? len / v : 0. );   
		namdump();         
	}
	return 0;
}



void 
MobileNode::update_position()
{
	double now = Scheduler::instance().clock();
	double interval = now - position_update_time_;
	double oldX = X_;
	//double oldY = Y_;

	if ((interval == 0.0)&&(position_update_time_!=0))
		return;         // ^^^ for list-based imprvmnt 


	// CHECK, IF THE SPEED IS 0, THEN SKIP, but usually it's not 0
	X_ += dX_ * (speed_ * interval);
	Y_ += dY_ * (speed_ * interval);

	if ((dX_ > 0 && X_ > destX_) || (dX_ < 0 && X_ < destX_))
	  X_ = destX_;		// correct overshoot (slow? XXX)
	if ((dY_ > 0 && Y_ > destY_) || (dY_ < 0 && Y_ < destY_))
	  Y_ = destY_;		// correct overshoot (slow? XXX)
	
	/* list based improvement */
	if(oldX != X_)// || oldY != Y_)
		T_->updateNodesList(this, oldX);//, oldY);
	// COMMENTED BY -VAL- // bound_position();

	// COMMENTED BY -VAL- // Z_ = T_->height(X_, Y_);

#if 0
	fprintf(stderr, "Node: %d, X: %6.2f, Y: %6.2f, Z: %6.2f, time: %f\n",
		address_, X_, Y_, Z_, now);
#endif
	position_update_time_ = now;
}


void
MobileNode::random_position()
{
	if (T_ == 0) {
		fprintf(stderr, "No TOPOLOGY assigned\n");
		exit(1);
	}

	X_ = Random::uniform() * T_->upperX();
	Y_ = Random::uniform() * T_->upperY();
	Z_ = T_->height(X_, Y_);

	position_update_time_ = 0.0;
}

void
MobileNode::random_destination()
{
	if (T_ == 0) {
		fprintf(stderr, "No TOPOLOGY assigned\n");
		exit(1);
	}

	random_speed();
#ifdef DEBUG
        fprintf(stderr, "%d - %s: calling set_destination()\n",
                address_, __FUNCTION__);
#endif
	(void) set_destination(Random::uniform() * T_->upperX(),
                               Random::uniform() * T_->upperY(),
                               speed_);
}

void
MobileNode::random_direction()
{
	/* this code isn't used anymore -dam 1/22/98 */
	double len;

	dX_ = (double) Random::random();
	dY_ = (double) Random::random();

	len = sqrt( (dX_ * dX_) + (dY_ * dY_) );

	dX_ /= len;
	dY_ /= len;
	dZ_ = 0.0;				// we're not flying...

	/*
	 * Determine the sign of each component of the
	 * direction vector.
	 */
	if (X_ > (T_->upperX() - 2*T_->resol())) {
		if (dX_ > 0) 
			dX_ = -dX_;
	} else if (X_ < (T_->lowerX() + 2*T_->resol())) {
		if (dX_ < 0) 
			dX_ = -dX_;
	} else if (Random::uniform() <= 0.5) {
		dX_ = -dX_;
	}

	if (Y_ > (T_->upperY() - 2*T_->resol())) {
		if (dY_ > 0) 
			dY_ = -dY_;
	} else if (Y_ < (T_->lowerY() + 2*T_->resol())) {
		if (dY_ < 0) 
			dY_ = -dY_;
	} else if(Random::uniform() <= 0.5) {
		dY_ = -dY_;
	}
#if 0
	fprintf(stderr, "Location: (%f, %f), Direction: (%f, %f)\n",
		X_, Y_, dX_, dY_);
#endif
}

void
MobileNode::random_speed()
{
	speed_ = Random::uniform() * MAX_SPEED;
}

double
MobileNode::distance(MobileNode *m)
{
	update_position();		// update my position
	m->update_position();		// update m's position

        double Xpos = (X_ - m->X_) * (X_ - m->X_);
        double Ypos = (Y_ - m->Y_) * (Y_ - m->Y_);
	double Zpos = (Z_ - m->Z_) * (Z_ - m->Z_);

        return sqrt(Xpos + Ypos + Zpos);
}

double
MobileNode::propdelay(MobileNode *m)
{
	return distance(m) / SPEED_OF_LIGHT;
}

void 
MobileNode::idle_energy_patch(float /*total*/, float /*P_idle*/)
{
}
