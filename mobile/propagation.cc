/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
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
 propagation.cc
 $Id: propagation.cc,v 1.9 2010/03/08 05:54:52 tom_henderson Exp $
*/

#include <stdio.h>

#include <topography.h>
#include <propagation.h>
#include <wireless-phy.h>

class PacketStamp;
int
Propagation::command(int argc, const char*const* argv)
{
  TclObject *obj;  

  if(argc == 3) 
    {
      if( (obj = TclObject::lookup(argv[2])) == 0) 
	{
	  fprintf(stderr, "Propagation: %s lookup of %s failed\n", argv[1],
		  argv[2]);
	  return TCL_ERROR;
	}

      if (strcasecmp(argv[1], "topography") == 0) 
	{
	  topo = (Topography*) obj;
	  return TCL_OK;
	}
    }
  return TclObject::command(argc,argv);
}
 

/* As new network-intefaces are added, add a default method here */

double
Propagation::Pr(PacketStamp *, PacketStamp *, Phy *)
{
	fprintf(stderr,"Propagation model %s not implemented for generic NetIF\n",
	  name);
	abort();
	return 0; // Make msvc happy
}


double
Propagation::Pr(PacketStamp *, PacketStamp *, WirelessPhy *)
{
	fprintf(stderr,
		"Propagation model %s not implemented for SharedMedia interface\n",
		name);
	abort();
	return 0; // Make msvc happy
}

double
Propagation::getDist(double , double , double , double , double , double , double , double )
{
	fprintf(stderr,
		"Propagtion model %s not implemented for generic use\n", name);
	abort();
	return 0;
}

double
Propagation::Friis(double Pt, double Gt, double Gr, double lambda, double L, double d)
{
        /*
         * Friis free space equation:
         *
         *       Pt * Gt * Gr * (lambda^2)
         *   P = --------------------------
         *       (4 * pi * d)^2 * L
         */
	if (d == 0.0) //XXX probably better check < MIN_DISTANCE or some such
		return Pt;
  double M = lambda / (4 * PI * d);
  return (Pt * Gt * Gr * (M * M)) / L;
}


// methods for free space model
static class FreeSpaceClass: public TclClass {
public:
	FreeSpaceClass() : TclClass("Propagation/FreeSpace") {}
	TclObject* create(int, const char*const*) {
		return (new FreeSpace);
	}
} class_freespace;


double FreeSpace::Pr(PacketStamp *t, PacketStamp *r, WirelessPhy *ifp)
{
	double L = ifp->getL();		// system loss
	double lambda = ifp->getLambda();   // wavelength

	double Xt, Yt, Zt;		// location of transmitter
	double Xr, Yr, Zr;		// location of receiver

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
	double d = sqrt(dX * dX + dY * dY + dZ * dZ);

	// get antenna gain
	double Gt = t->getAntenna()->getTxGain(dX, dY, dZ, lambda);
	double Gr = r->getAntenna()->getRxGain(dX, dY, dZ, lambda);

	// calculate receiving power at distance
	double Pr = Friis(t->getTxPr(), Gt, Gr, lambda, L, d);
	// warning: use of `l' length character with `f' type character
	//  - Sally Floyd, FreeBSD.
	printf("%lf: d: %lf, Pr: %e\n", Scheduler::instance().clock(), d, Pr);

	return Pr;
}

double
FreeSpace::getDist(double Pr, double Pt, double Gt, double Gr, double , double , double L, double lambda)
{
        return sqrt((Pt * Gt * Gr * lambda * lambda) / (L * Pr)) /
                (4 * PI);
}

