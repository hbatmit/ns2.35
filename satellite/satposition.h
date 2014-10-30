/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1999 Regents of the University of California.
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
 *      This product includes software developed by the MASH Research
 *      Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
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
 * @(#) $Header: /cvsroot/nsnam/ns-2/satellite/satposition.h,v 1.10 2000/08/18 18:34:01 haoboy Exp $
 *
 * Contributed by Tom Henderson, UCB Daedalus Research Group, June 1999
 * Modified to use period_ and offer isascending(), Lloyd Wood, March 2000. */

#ifndef __satposition_h__
#define __satposition_h__

#include <stdlib.h>

#include "trace.h"
#include "lib/bsd-list.h"
#include "phy.h"
#include "node.h"
#include "object.h"
#include "satgeometry.h"

// Position types
#define POSITION_SAT 1
#define POSITION_SAT_POLAR 2
#define POSITION_SAT_GEO 3
#define POSITION_SAT_TERM 4

class SatPosition : public TclObject {
 public:
	SatPosition();
	int type() { return type_; }
	double period() { return period_; }
	Node* node() { return node_; }
	virtual coordinate coord() = 0; 

	// configuration parameters
	static double time_advance_;
 protected:
        int command(int argc, const char*const* argv);
	coordinate initial_;
	double period_;
	int type_;
	Node* node_;
};

class PolarSatPosition : public SatPosition {
 public:
	PolarSatPosition(double = 1000, double = 90, double = 0, double = 0, 
            double = 0);
	virtual coordinate coord();
	void set(double Altitude, double Lon, double Alpha, double inclination=90); 
	bool isascending();
	PolarSatPosition* next() { return next_; }
	int plane() { return plane_; }

 protected:
        int command(int argc, const char*const* argv);
        PolarSatPosition* next_;    // Next intraplane satellite
	int plane_;  // Orbital plane that this satellite resides in
	double inclination_; // radians

	
};

class GeoSatPosition : public SatPosition {
 public:
	GeoSatPosition(double longitude = 0);
	virtual coordinate coord();
	void set(double longitude); 
 protected:
};

class TermSatPosition : public SatPosition {
 public:
	TermSatPosition(double = 0, double = 0);
	virtual coordinate coord();
	void set(double latitude, double longitude);
 protected:
};

#endif // __satposition_h__
