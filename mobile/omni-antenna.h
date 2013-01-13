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
   omni-antenna.h
   omni-directional antenna

*/

#ifndef ns_omniantenna_h
#define ns_omniantenna_h

#include <antenna.h>

class OmniAntenna : public Antenna {

public:
  OmniAntenna();
  
  virtual double getTxGain(double, double, double, double)
  // return the gain for a signal to a node at vector dX, dY, dZ
  // from the transmitter at wavelength lambda
    {
      return Gt_;
    }
  virtual double getRxGain(double, double, double, double)
  // return the gain for a signal from a node at vector dX, dY, dZ
  // from the receiver at wavelength lambda
    {
      return Gr_;
    }
  
  virtual Antenna * copy()
  // return a pointer to a copy of this antenna that will return the 
  // same thing for get{Rx,Tx}Gain that this one would at this point
  // in time.  This is needed b/c if a pkt is sent with a directable
  // antenna, this antenna may be have been redirected by the time we
  // call getTxGain on the copy to determine if the pkt is received.
    {
      // since the Gt and Gr are constant w.r.t time, we can just return
      // this object itself
      return this;
    }

  virtual void release()
  // release a copy created by copy() above
    {
      // don't do anything
    }
 
protected:
  double Gt_;			// gain of transmitter (db)
  double Gr_;			// gain of receiver (db)
};


#endif // ns_omniantenna_h
