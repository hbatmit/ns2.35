/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * shadowing.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: shadowing.cc,v 1.6 2010/03/08 05:54:52 tom_henderson Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */

/*
 * Shadowing propation model, including the path-loss model.
 * This statistcal model is applicable for both outdoor and indoor.
 * Wei Ye, weiye@isi.edu, 2000
 */


#include <math.h>

#include <delay.h>
#include <packet.h>

#include <packet-stamp.h>
#include <antenna.h>
#include <mobilenode.h>
#include <propagation.h>
#include <wireless-phy.h>
#include <shadowing.h>


static class ShadowingClass: public TclClass {
public:
	ShadowingClass() : TclClass("Propagation/Shadowing") {}
	TclObject* create(int, const char*const*) {
		return (new Shadowing);
	}
} class_shadowing;


Shadowing::Shadowing()
{
	bind("pathlossExp_", &pathlossExp_);
	bind("std_db_", &std_db_);
	bind("dist0_", &dist0_);
	bind("seed_", &seed_);
	
	ranVar = new RNG;
	ranVar->set_seed(RNG::PREDEF_SEED_SOURCE, seed_);
}


Shadowing::~Shadowing()
{
	delete ranVar;
}


double Shadowing::Pr(PacketStamp *t, PacketStamp *r, WirelessPhy *ifp)
{
	double L = ifp->getL();		// system loss
	double lambda = ifp->getLambda();   // wavelength

	double Xt, Yt, Zt;		// loc of transmitter
	double Xr, Yr, Zr;		// loc of receiver

	t->getNode()->getLoc(&Xt, &Yt, &Zt);
	r->getNode()->getLoc(&Xr, &Yr, &Zr);

	// Is antenna position relative to node position?
	Xr += r->getAntenna()->getX();
	Yr += r->getAntenna()->getY();
	Zr += r->getAntenna()->getZ();
	Xt += t->getAntenna()->getX();
	Yt += t->getAntenna()->getY();
	Zt += t->getAntenna()->getZ();

	double dX = Xr - Xt;
	double dY = Yr - Yt;
	double dZ = Zr - Zt;
	double dist = sqrt(dX * dX + dY * dY + dZ * dZ);

	// get antenna gain
	double Gt = t->getAntenna()->getTxGain(dX, dY, dZ, lambda);
	double Gr = r->getAntenna()->getRxGain(dX, dY, dZ, lambda);

	// calculate receiving power at reference distance
	double Pr0 = Friis(t->getTxPr(), Gt, Gr, lambda, L, dist0_);

	// calculate average power loss predicted by path loss model
	double avg_db;
        if (dist > dist0_) {
            avg_db = -10.0 * pathlossExp_ * log10(dist/dist0_);
        } else {
            avg_db = 0.0;
        }
   
	// get power loss by adding a log-normal random variable (shadowing)
	// the power loss is relative to that at reference distance dist0_
	double powerLoss_db = avg_db + ranVar->normal(0.0, std_db_);

	// calculate the receiving power at dist
	double Pr = Pr0 * pow(10.0, powerLoss_db/10.0);
	
	return Pr;
}


int Shadowing::command(int argc, const char* const* argv)
{
	if (argc == 4) {
		if (strcmp(argv[1], "seed") == 0) {
			int s = atoi(argv[3]);
			if (strcmp(argv[2], "raw") == 0) {
				ranVar->set_seed(RNG::RAW_SEED_SOURCE, s);
			} else if (strcmp(argv[2], "predef") == 0) {
				ranVar->set_seed(RNG::PREDEF_SEED_SOURCE, s);
				// s is the index in predefined seed array
				// 0 <= s < 64
			} else if (strcmp(argv[2], "heuristic") == 0) {
				ranVar->set_seed(RNG::HEURISTIC_SEED_SOURCE, 0);
			}
			return(TCL_OK);
		}
	}
	
	return Propagation::command(argc, argv);
}


double Shadowing::getDist(double , double , double , double, double , double , double , double )
{
	return DBL_MAX;
}

