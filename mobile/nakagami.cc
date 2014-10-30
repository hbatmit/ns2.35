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

#include <math.h>
#include <delay.h>
#include <packet.h>

#include <iostream>
#include <float.h>
#include <packet-stamp.h>
#include <antenna.h>
#include <mobilenode.h>
#include <propagation.h>
#include <wireless-phy.h>
#include <ranvar.h>
#include <nakagami.h>

static class NakagamiClass: public TclClass {
public: 
	NakagamiClass() : TclClass("Propagation/Nakagami") {}
	TclObject* create(int, const char*const*) {
		return (new Nakagami);	
}
} class_nakagami;


Nakagami::Nakagami()
{
	bind("gamma0_", &gamma0);
	bind("gamma1_", &gamma1);
	bind("gamma2_", &gamma2);
	bind("d0_gamma_", &d0_gamma);                           
	bind("d1_gamma_", &d1_gamma);

	bind_bool ("use_nakagami_dist_" ,&use_nakagami_dist_);	// Use random variation superimposed on the mean reception power or not
	bind("m0_",&m0);
	bind("m1_",&m1);
	bind("m2_",&m2);

	bind("d0_m_", &d0_m);
	bind("d1_m_", &d1_m);
}

Nakagami::Nakagami(double g0,double g1,double g2,double d0_g,double d1_g,double m_0,double m_1,double m_2,double d0m,double d1m, int use_distribution)
{
	gamma0 =g0;
	gamma1 =g1;
	gamma2 =g2;
	d0_gamma=d0_g;
	d1_gamma= d1_g;
	m0= m_0;
	m1= m_1;
	m2= m_2;
	d0_m= d0m;
	d1_m= d1m;
	use_nakagami_dist_ = use_distribution;
}

Nakagami::~Nakagami()
{
	//
}


double Nakagami::Pr(PacketStamp *t, PacketStamp *r, WirelessPhy *ifp)
{
	double L = ifp->getL();	 	    // system loss
	double lambda = ifp->getLambda();   // wavelength

	double Xt, Yt, Zt;	    	    // loc of transmitter
	double Xr, Yr, Zr;	 	    // loc of receiver

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
	
	double d_ref = 1.0;

	// calculate receiving power at reference distance
	double Pr0 = Friis(t->getTxPr(), Gt, Gr, lambda, L, d_ref);

	// calculate average power loss predicted by empirical loss model in dB
	// according to measurements, 
	// the default settings of gamma, m and d are stored in tcl/lib/ns-default.tcl
  	double path_loss_dB = 0.0;
  	if (dist > 0 && dist <= d0_gamma) {
		path_loss_dB = 10*gamma0*log10(dist/d_ref); 
  	}
	if (dist > d0_gamma && dist <= d1_gamma) {
		path_loss_dB = 10*gamma0*log10(d0_gamma/d_ref) + 10*gamma1*log10(dist/d0_gamma);
	} 
	if(dist > d1_gamma) { 
		path_loss_dB = 10*gamma0*log10(d0_gamma/d_ref) + 10*gamma1*log10(d1_gamma/d0_gamma)+10*gamma2*log10(dist/d1_gamma);
	}

    // calculate the receiving power at distance dist
 	double Pr = Pr0 * pow(10.0, -path_loss_dB/10.0);
	
 	if (!use_nakagami_dist_) {
 		return Pr; 
 	} else {
 		double m;
 		if ( dist <= d0_m)
 			m = m0;
 		else if ( dist <= d1_m)
 			m = m1; 
 		else
 			m = m2;
 		unsigned int int_m = (unsigned int)(floor (m));
 		
 		double resultPower;
 		
        if (int_m == m) {
 			resultPower = ErlangRandomVariable(Pr/m, int_m).value();
 		} else {
 			resultPower = GammaRandomVariable(m, Pr/m).value();
 		}
 		return resultPower;
	}
}	

int Nakagami::command(int , const char* const* )
{
	return 0;
}


double Nakagami::getDist(double , double , double , double , double , double , double , double ) {
	return DBL_MAX;
}
