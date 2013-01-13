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
 * @(#) $
 *
 * Contributed by Tom Henderson, UCB Daedalus Research Group, June 1999
 */

#ifndef __ns_sat_geometry_h__
#define __ns_sat_geometry_h__

#include <math.h>
#include <trace.h>
#include "object.h"

// Various constants
#define PI 3.1415926535897
#define MU 398601.2 // Greek Mu (km^3/s^2)
#define LIGHT 299793 // km/s
#define EARTH_PERIOD 86164 // seconds
#define EARTH_RADIUS 6378  // km
#define GEO_ALTITUDE 35786 // km
#define ATMOS_MARGIN 150 // km

#define DEG_TO_RAD(x) ((x) * PI/180)
#define RAD_TO_DEG(x) ((x) * 180/PI)
#define DISTANCE(s_x, s_y, s_z, e_x, e_y, e_z) (sqrt((s_x - e_x) * (s_x - e_x) \
                + (s_y - e_y) * (s_y - e_y) + (s_z - e_z) * (s_z - e_z)))

struct coordinate {
        double r;        // km
        double theta;    // radians
        double phi;      // radians
        // Convert to cartesian as follows:
        // x = rsin(theta)cos(phi)
        // y = rsin(theta)sin(phi)
        // z = rcos(theta)
};

// Library of routines involving satellite geometry
class SatGeometry : public TclObject {
public:
	SatGeometry() { printf("Started\n");}
	static double distance(coordinate, coordinate);              
	static void spherical_to_cartesian(double, double, double,
	    double &, double &, double &);
	static double propdelay(coordinate, coordinate);
	static double get_latitude(coordinate);
	static double get_longitude(coordinate);
	static double get_radius(coordinate a) { return a.r; }
	static double get_altitude(coordinate);
	static double check_elevation(coordinate, coordinate, double);
	static int are_satellites_mutually_visible(coordinate, coordinate);

protected: 
	// Define "command" appropriately if you want OTcl access to this class
        int command(/*int argc, const char*const* argv */) { return 0; }
};

#endif // __ns_sat_geometry_h__
