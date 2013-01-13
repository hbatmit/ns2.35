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
 * Contributed by Tom Henderson, UCB Daedalus Research Group, June 1999
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/satellite/satgeometry.cc,v 1.6 2001/05/21 19:27:31 haldar Exp $";
#endif

#include "satgeometry.h"
#include "satposition.h"


static class SatGeometryClass : public TclClass {
public:
        SatGeometryClass() : TclClass("SatGeometry") {}
        TclObject* create(int, const char*const*) {
                return (new SatGeometry());
        }
} class_sat_geometry;


// Returns the distance in km between points a and b
double SatGeometry::distance(coordinate a, coordinate b)
{
        double a_x, a_y, a_z, b_x, b_y, b_z;     // cartesian
	spherical_to_cartesian(a.r, a.theta, a.phi, a_x, a_y, a_z);
	spherical_to_cartesian(b.r, b.theta, b.phi, b_x, b_y, b_z);
        return (BaseTrace::round(DISTANCE(a_x, a_y, a_z, b_x, b_y, b_z), 1.0E+8));


}

void SatGeometry::spherical_to_cartesian(double R, double Theta,
    double Phi, double &X, double &Y, double &Z)
{      
	X = R * sin(Theta) * cos (Phi);
	Y = R * sin(Theta) * sin (Phi);
	Z = R * cos(Theta);
}

// Propagation delay is the distance divided by the speed of light
double SatGeometry::propdelay(coordinate a, coordinate b)
{
	double delay = distance(a, b)/LIGHT;
	return (BaseTrace::round(delay, 1.0E+8));
}

double SatGeometry::get_altitude(coordinate a)
{
        return (a.r - EARTH_RADIUS);
}

// Returns latitude in radians, in the range from -PI/2 to PI/2
double SatGeometry::get_latitude(coordinate a)
{
        return (PI/2 - a.theta);
}

// Returns (earth-centric) longitude corresponding to the position of the node 
// (the input coordinate corresponds to fixed coordinate system, through
// which the Earth rotates, so we have to scale back the effects of rotation).
// The return value ranges from -PI to PI.
double SatGeometry::get_longitude(coordinate coord_)
{
        double period = EARTH_PERIOD; // period of earth in seconds
        // adjust longitude so that it is earth-centric (i.e., account
        // for earth rotating beneath).   
        double earth_longitude = fmod((coord_.phi -
           (fmod(NOW + SatPosition::time_advance_,period)/period) * 2*PI), 
	    2*PI);
	// Bring earth_longitude to be within (-PI, PI)
        if (earth_longitude < (-1*PI))
		earth_longitude = 2*PI + earth_longitude;
        if (earth_longitude > PI)
		earth_longitude = (-(2*PI - earth_longitude));
	if (fabs(earth_longitude) < 0.0001)
		return 0;   // To avoid trace output of "-0.00"
	else
		return (earth_longitude);
}       

// If the satellite is above the elevation mask of the terminal, returns 
// the elevation mask in radians; otherwise, returns 0.
double SatGeometry::check_elevation(coordinate satellite,
    coordinate terminal, double elev_mask_)
{
	double S = satellite.r;  // satellite radius
	double S_2 = satellite.r * satellite.r;  // satellite radius^2
	double E = EARTH_RADIUS;
	double E_2 = E * E;
	double d, theta, alpha;

	d = distance(satellite, terminal);
	if (d < sqrt(S_2 - E_2)) {
		// elevation angle > 0
		theta = acos((E_2+S_2-(d*d))/(2*E*S));
		alpha = acos(sin(theta) * S/d);
		return ( (alpha > elev_mask_) ? alpha : 0);
	} else
		return 0;
}

// This function determines whether two satellites are too far apart
// to establish an ISL between them, due to Earth atmospheric grazing
// (or shadowing by the Earth itself).  Assumes that both satellites nodes
// are at the same altitude.  The line between the two satellites can be
// bisected, and a perpendicular from that point to the Earth's center will
// form a right triangle.  If the length of this perpendicular is less than
// EARTH_RADIUS + ATMOS_MARGIN, the link cannot be established.
//
int SatGeometry::are_satellites_mutually_visible(coordinate first, coordinate second)
{
	// if we drop a perpendicular from the ISL to the Earth's surface,
	// we have a right triangle.  The atmospheric margin is the minimum
	// ISL grazing altitude.
	double c, d, min_radius, grazing_radius;
	double radius = get_radius(first); // could just use first.r here.
	double distance_ = distance(first, second);
	c = radius * radius;
	d = (distance_/2) * (distance_/2);
	grazing_radius = (EARTH_RADIUS + ATMOS_MARGIN);
	min_radius = sqrt(c - d);
	if (min_radius >= grazing_radius) {
		return TRUE;
	} else {
		return FALSE;
	}
}

