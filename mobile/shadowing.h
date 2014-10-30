/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * shadowing.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: shadowing.h,v 1.5 2010/03/08 05:54:52 tom_henderson Exp $
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


#ifndef shadowing_h
#define shadowing_h

#include <packet-stamp.h>
#include <wireless-phy.h>
#include <propagation.h>
#include <rng.h>
#include <float.h>

class Shadowing : public Propagation {
public:
	Shadowing();
	~Shadowing();
	virtual double Pr(PacketStamp *tx, PacketStamp *rx, WirelessPhy *ifp);
	virtual double getDist(double Pr, double Pt, double Gt, double Gr,
			       double hr, double ht, double L, double lambda);
	virtual int command(int argc, const char*const* argv);

protected:
	RNG *ranVar;	// random number generator for normal distribution
	
	double pathlossExp_;	// path-loss exponent
	double std_db_;		// shadowing deviation (dB),
	double dist0_;	// close-in reference distance
	int seed_;	// seed for random number generator
};

#endif

