/*
 * Copyright (C) 2007 
 * Mercedes-Benz Research & Development North America, Inc.
 * All rights reserved.
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
 * This code was developed by:
 * 
 * Qi Chen                 : qi.chen@daimler.com
 * Heiko Mangold            
 * Daniel Jiang            : daniel.jiang@daimler.com
 * 
 * For further information see: 
 * http://dsn.tm.uni-karlsruhe.de/english/Overhaul_NS-2.php
 */

#ifndef nakagami_h
#define nakagami_h

#include <packet-stamp.h>
#include <wireless-phy.h>
#include <propagation.h>
#include <rng.h>

class Nakagami : public Propagation {
public:
	Nakagami();
	Nakagami(double g0,double g1,double g2,double d0_g,double d1_g,double m_0,double m_1,double m_2,double d0m,double d1m, int use_distribution);
	~Nakagami();

	virtual double Pr(PacketStamp *tx, PacketStamp *rx, WirelessPhy *ifp);
	virtual int command(int argc, const char*const* argv);
	virtual double getDist(double Pr, double Pt, double Gt, double Gr, double hr, double ht, double L, double lambda);
protected:
	RNG *ranVar;	// random number generator for normal distribution
	double gamma0,gamma1, gamma2;
	double d0_gamma,d1_gamma;

	int use_nakagami_dist_;
	double m0,m1,m2;
	double d0_m,d1_m;
};

#endif
