/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- 
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
 * @(#) $Header: /cvsroot/nsnam/ns-2/common/mobilenode.h,v 1.22 2006/02/21 15:20:18 mahrenho Exp $
 *
 */

/*
 * XXX
 * Eventually energe model and location stuff in this file will be cleaned
 * up and moved to separate file to improve modularity. BUT before that is 
 * finished, they should stay in this file rather than bothering the base 
 * node.
 */

class MobileNode;

#ifndef __ns_mobilenode_h__
#define __ns_mobilenode_h__

#define MN_POSITION_UPDATE_INTERVAL	30.0   // seconds
#define MAX_SPEED			5.0    // meters per second (33.55 mph)
#define MIN_SPEED			0.0


#include "object.h"
#include "trace.h"
#include "lib/bsd-list.h"
#include "phy.h"
#include "topography.h"
#include "arp.h"
#include "node.h"
#include "gridkeeper.h"
#include "energy-model.h"
#include "location.h"



#if COMMENT_ONLY
		 -----------------------
		|			|
		|	Upper Layers	|
		|			|
		 -----------------------
		    |		    |
		    |		    |
		 -------	 -------
		|	|	|	|
		|  LL	|	|  LL	|
		|	|	|	|
		 -------	 -------
		    |		    |
		    |		    |
		 -------	 -------
		|	|	|	|
		| Queue	|	| Queue	|
		|	|	|	|
		 -------	 -------
		    |		    |
		    |		    |
		 -------	 -------
		|	|	|	|
		|  Mac	|	|  Mac	|
		|	|	|	|
		 -------	 -------
		    |		    |
		    |		    |
		 -------	 -------	 -----------------------
		|	|	|	|	|			|
		| Netif	| <---	| Netif | <---	|	Mobile Node	|
		|	|	|	|	|			|
		 -------	 -------	 -----------------------
		    |		    |
		    |		    |
		 -----------------------
		|			|
		|	Channel(s) 	|
		|			|
		 -----------------------
#endif
class MobileNode;

class PositionHandler : public Handler {
public:
	PositionHandler(MobileNode* n) : node(n) {}
	void handle(Event*);
private:
	MobileNode *node;
};

class MobileNode : public Node 
{
	friend class PositionHandler;
public:
	MobileNode();
	virtual int command(int argc, const char*const* argv);

	double	distance(MobileNode*);
	double	propdelay(MobileNode*);
	void	start(void);
        inline void getLoc(double *x, double *y, double *z) {
		update_position();  *x = X_; *y = Y_; *z = Z_;
	}
        inline void getVelo(double *dx, double *dy, double *dz) {
		*dx = dX_ * speed_; *dy = dY_ * speed_; *dz = 0.0;
	}
	inline MobileNode* nextnode() { return link_.le_next; }
	inline int base_stn() { return base_stn_;}
	inline void set_base_stn(int addr) { base_stn_ = addr; }

	void dump(void);

	inline MobileNode*& next() { return next_; }
	inline double X() { return X_; }
	inline double Y() { return Y_; }
	inline double Z() { return Z_; }
	inline double speed() { return speed_; }
	inline double dX() { return dX_; }
	inline double dY() { return dY_; }
	inline double dZ() { return dZ_; }
	inline double destX() { return destX_; }
	inline double destY() { return destY_; }
	inline double radius() { return radius_; }
	inline double getUpdateTime() { return position_update_time_; }
	//inline double last_routingtime() { return last_rt_time_;}

	void update_position();
	void log_energy(int);
	//void logrttime(double);
	virtual void idle_energy_patch(float, float);

	/* For list-keeper */
	MobileNode* nextX_;
	MobileNode* prevX_;
	
protected:
	/*
	 * Last time the position of this node was updated.
	 */
	double position_update_time_;
        double position_update_interval_;

	/*
         *  The following indicate the (x,y,z) position of the node on
         *  the "terrain" of the simulation.
         */
	double X_;
	double Y_;
	double Z_;
	double speed_;	// meters per second

	/*
         *  The following is a unit vector that specifies the
         *  direction of the mobile node.  It is used to update
         *  position
         */
	double dX_;
	double dY_;
	double dZ_;

        /* where are we going? */
	double destX_;
	double destY_;

	/*
	 * for gridkeeper use only
 	 */
	MobileNode*	next_;
	double          radius_;

	// Used to generate position updates
	PositionHandler pos_handle_;
	Event pos_intr_;

	void	log_movement();
	void	random_direction();
	void	random_speed();
        void    random_destination();
        int	set_destination(double x, double y, double speed);
	  
private:
	inline int initialized() {
		return (T_ && log_target_ &&
			X_ >= T_->lowerX() && X_ <= T_->upperX() &&
			Y_ >= T_->lowerY() && Y_ <= T_->upperY());
	}
	void		random_position();
	void		bound_position();
	int		random_motion_;	// is mobile

	/*
	 * A global list of mobile nodes
	 */
	LIST_ENTRY(MobileNode) link_;


	/*
	 * The topography over which the mobile node moves.
	 */
	Topography *T_;
	/*
	 * Trace Target
	 */
	Trace* log_target_;
        /* 
	 * base_stn for mobilenodes communicating with wired nodes
         */
	int base_stn_;


	//int last_rt_time_;
};

#endif // ns_mobilenode_h











