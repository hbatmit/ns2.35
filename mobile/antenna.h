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
   
   antenna.h
   super class for all antenna types
*/

#ifndef ns_antenna_h
#define ns_antenna_h

#include "object.h"
#include "lib/bsd-list.h"

class Antenna;
LIST_HEAD(an_head, Antenna);

class Antenna : public TclObject {

public:
  Antenna();
  
  virtual double getTxGain(double /*dX*/, double /*dY*/, double /*dZ*/,
			   double /*lambda*/);
  // return the gain for a signal to a node at vector dX, dY, dZ
  // from the transmitter at wavelength lambda

  virtual double getRxGain(double /*dX*/, double /*dY*/, double /*dZ*/,
			   double /*lambda*/);
  // return the gain for a signal from a node at vector dX, dY, dZ
  // from the receiver at wavelength lambda
  
  virtual Antenna * copy();
  // return a pointer to a copy of this antenna that will return the 
  // same thing for get{Rx,Tx}Gain that this one would at this point
  // in time.  This is needed b/c if a pkt is sent with a directable
  // antenna, this antenna may be have been redirected by the time we
  // call getTxGain on the copy to determine if the pkt is received.

  virtual void release();
  // release a copy created by copy() above

  inline void insert(struct an_head* head) {
    LIST_INSERT_HEAD(head, this, link);
  }

  virtual inline double getX() {return X_;}
  virtual inline double getY() {return Y_;}
  virtual inline double getZ() {return Z_;}

private:
  LIST_ENTRY(Antenna) link;
  
protected:  
  double X_;			// position w.r.t. the node
  double Y_;
  double Z_;
};


#endif // ns_antenna_h
