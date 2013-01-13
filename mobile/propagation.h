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
 */
/* Ported from CMU/Monarch's code, nov'98 -Padma.
   propagation.h

   superclass of all propagation models
*/

#ifndef ns_propagation_h
#define ns_propagation_h

#define SPEED_OF_LIGHT	300000000		// 3 * 10^8 m/s
#define PI		3.1415926535897


#include <topography.h>
#include <phy.h>
#include <wireless-phy.h>
#include <packet-stamp.h>

class PacketStamp;
class WirelessPhy;
/*======================================================================
   Progpagation Models

	Using postion and wireless transmission interface properties,
	propagation models compute the power with which a given
	packet will be received.

	Not all propagation models will be implemented for all interface
	types, since a propagation model may only be appropriate for
	certain types of interfaces.

   ====================================================================== */

class Propagation : public TclObject {

public:
  Propagation() : name(NULL), topo(NULL) {}

  // calculate the Pr by which the receiver will get a packet sent by
  // the node that applied the tx PacketStamp for a given inteface 
  // type
  virtual double Pr(PacketStamp *tx, PacketStamp *rx, Phy *);
  virtual double Pr(PacketStamp *tx, PacketStamp *rx, WirelessPhy *);
  virtual int command(int argc, const char*const* argv);

  // get interference distance
  virtual double getDist(double Pr, double Pt, double Gt, double Gr,
			 double hr, double ht, double L, double lambda);


  // Friis free space equation, likely to be used by other propagation models.
  double Friis(double Pt, double Gt, double Gr, double lambda, double L, double d);
  	// Pt -- transmitted signal power
  	// Gt -- transmitter antenna gain
  	// Gr -- receiver antenna gain
  	// lambda -- wavelength
  	// L -- system loss (L >= 1)
  	// d -- distance between transmitter and receiver
  	// return -- received signal power

protected:
  char *name;
  Topography *topo;
};


// Friis free space propagation model
class FreeSpace : public Propagation {
public:
//	FreeSpace();
	virtual double Pr(PacketStamp *tx, PacketStamp *rx, WirelessPhy *ifp);
	virtual double getDist(double Pr, double Pt, double Gt, double Gr,
			       double ht, double hr, double L, double lambda);
};

#endif /* __propagation_h__ */

