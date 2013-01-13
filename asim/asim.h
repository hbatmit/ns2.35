
/*
 * asim.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: asim.h,v 1.3 2005/08/25 18:58:01 johnh Exp $
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

// author: Debojyoti Dutta


#ifndef _RED_ROUTER_H_
#define _RED_ROUTER_H_

#include <assert.h>
#include <stdio.h>
#include <math.h>


class RedRouter {
  double MinTh, MaxTh, MaxP;
  double Lambda_L;
  double Lambda_H;

  void Populate();
 public:
  RedRouter(int mTh, int MTh, double MP) {
    MinTh = mTh;
    MaxTh = MTh;
    MaxP = MP; 
    Populate();
  }
  double ComputeProbability(double Lambda, double &Delay);
  short Identical(int mTh, int MTh, double MP) {
    return (mTh == MinTh &&
	    MTh == MaxTh &&
	    MP  == MaxP);
  }
};

void RedRouter::Populate() {
  // rho = Lambda_L: p = 0 => rho/(1-rho) = MinTh
  Lambda_L = ((double)MinTh)/((double)(1+MinTh));

  // rho = Lambda_H: p = Max_p => rho(1-Max_p)/(1-rho(1-Max_p)) = MaxTh;
  if (MaxP < 1)
    Lambda_H = ((double)MaxTh)/((double)(1+MaxTh))/(1-MaxP);
}


double RedRouter::ComputeProbability(double Lambda, double &delay) {
  double p;
  
  if (Lambda <= Lambda_L) {
    delay = Lambda/(1-Lambda);
    return 0;
  }

  if (MaxP < 1 && Lambda > Lambda_H) {
    delay = MaxTh;
    p = (Lambda - Lambda_H*(1 - MaxP))/Lambda;
    return p;
  }

  // Solve the quadratic a.p^2 + b.p + c = 0
  double a, b, c;
  a = Lambda * (MaxTh - MinTh)/(MaxP);
  b = (MaxTh - MinTh)*(1-Lambda)/MaxP + MinTh * Lambda + Lambda;
  c = MinTh*(1-Lambda)-Lambda;

  p = (-b + sqrt(b*b - 4 * a * c))/(2 * a);
  delay = Lambda*(1-p)/(1-(Lambda*(1-p)));
  return p;
}


#endif






