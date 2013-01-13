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
 * Modified to use period_ and offer isascending(), Lloyd Wood, March 2000. */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/satellite/satposition.cc,v 1.9 2005/08/22 05:08:34 tomh Exp $";
#endif

#include "satposition.h"
#include "satgeometry.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static class TermSatPositionClass : public TclClass {
public:
	TermSatPositionClass() : TclClass("Position/Sat/Term") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc == 5) {
			float a, b;
			sscanf(argv[4], "%f %f", &a, &b);
			return (new TermSatPosition(a, b));
		} else
			return (new TermSatPosition);
	}
} class_term_sat_position;

static class PolarSatPositionClass : public TclClass {
public:
	PolarSatPositionClass() : TclClass("Position/Sat/Polar") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc == 5) {
			float a = 0, b = 0, c = 0, d = 0, e = 0;
			sscanf(argv[4], "%f %f %f %f %f", &a, &b, &c, &d, &e);
			return (new PolarSatPosition(a, b, c, d, e));
		}
		else {
			return (new PolarSatPosition);
		}
	}
} class_polarsatposition;

static class GeoSatPositionClass : public TclClass {
public:
	GeoSatPositionClass() : TclClass("Position/Sat/Geo") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc == 5) 
			return (new GeoSatPosition(double(atof(argv[4]))));
		else 
			return (new GeoSatPosition);
	}
} class_geosatposition;

static class SatPositionClass : public TclClass {
public:
	SatPositionClass() : TclClass("Position/Sat") {}
	TclObject* create(int, const char*const*) {
			printf("Error:  do not instantiate Position/Sat\n");
			return (0);
	}
} class_satposition;

double SatPosition::time_advance_ = 0;

SatPosition::SatPosition() : node_(0)  
{
        bind("time_advance_", &time_advance_);
}

int SatPosition::command(int argc, const char*const* argv) {     
	//Tcl& tcl = Tcl::instance();
	if (argc == 2) {
	}
	if (argc == 3) {
		if(strcmp(argv[1], "setnode") == 0) {
			node_ = (Node*) TclObject::lookup(argv[2]);
			if (node_ == 0)
				return TCL_ERROR;
			return TCL_OK;
		}
	}
	return (TclObject::command(argc, argv));
}

/////////////////////////////////////////////////////////////////////
// class TermSatPosition
/////////////////////////////////////////////////////////////////////

// Specify initial coordinates.  Default coordinates place the terminal
// on the Earth's surface at 0 deg lat, 0 deg long.
TermSatPosition::TermSatPosition(double Theta, double Phi)  {
	initial_.r = EARTH_RADIUS;
	period_ = EARTH_PERIOD; // seconds
	set(Theta, Phi);
	type_ = POSITION_SAT_TERM;
}

//
// Convert user specified latitude and longitude to our spherical coordinates
// Latitude is in the range (-90, 90) with neg. values -> south
// Initial_.theta is stored from 0 to PI (spherical)
// Longitude is in the range (-180, 180) with neg. values -> west
// Initial_.phi is stored from 0 to 2*PI (spherical)
//
void TermSatPosition::set(double latitude, double longitude)
{
	if (latitude < -90 || latitude > 90)
		fprintf(stderr, "TermSatPosition:  latitude out of bounds %f\n",
		   latitude);
	if (longitude < -180 || longitude > 180)
		fprintf(stderr, "TermSatPosition: longitude out of bounds %f\n",
		    longitude);
	initial_.theta = DEG_TO_RAD(90 - latitude);
	if (longitude < 0)
		initial_.phi = DEG_TO_RAD(360 + longitude);
	else
		initial_.phi = DEG_TO_RAD(longitude);
}

coordinate TermSatPosition::coord()
{
	coordinate current;

	current.r = initial_.r;
	current.theta = initial_.theta;
	current.phi = fmod((initial_.phi + 
	    (fmod(NOW + time_advance_,period_)/period_) * 2*PI), 2*PI);

#ifdef POINT_TEST
	current = initial_; // debug option to stop earth's rotation
#endif
	return current;
}

/////////////////////////////////////////////////////////////////////
// class PolarSatPosition
/////////////////////////////////////////////////////////////////////

PolarSatPosition::PolarSatPosition(double altitude, double Inc, double Lon, 
    double Alpha, double Plane) : next_(0), plane_(0) {
	set(altitude, Lon, Alpha, Inc);
        bind("plane_", &plane_);
        if (Plane) 
		plane_ = int(Plane);
	type_ = POSITION_SAT_POLAR;
}

//
// Since it is easier to compute instantaneous orbit position based on a
// coordinate system centered on the orbit itself, we keep initial coordinates
// specified in terms of the satellite orbit, and convert to true spherical 
// coordinates in coord().
// Initial satellite position is specified as follows:
// initial_.theta specifies initial angle with respect to "ascending node"
// i.e., zero is at equator (ascending)-- this is the $alpha parameter in Otcl
// initial_.phi specifies East longitude of "ascending node"  
// -- this is the $lon parameter in OTcl
// Note that with this system, only theta changes with time
//
void PolarSatPosition::set(double Altitude, double Lon, double Alpha, double Incl)
{
	if (Altitude < 0) {
		fprintf(stderr, "PolarSatPosition:  altitude out of \
		   bounds: %f\n", Altitude);
		exit(1);
	}
	initial_.r = Altitude + EARTH_RADIUS; // Altitude in km above the earth
	if (Alpha < 0 || Alpha >= 360) {
		fprintf(stderr, "PolarSatPosition:  alpha out of bounds: %f\n", 
		    Alpha);
		exit(1);
	}
	initial_.theta = DEG_TO_RAD(Alpha);
	if (Lon < -180 || Lon > 180) {
		fprintf(stderr, "PolarSatPosition:  lon out of bounds: %f\n", 
		    Lon);
		exit(1);
	}
	if (Lon < 0)
		initial_.phi = DEG_TO_RAD(360 + Lon);
	else
		initial_.phi = DEG_TO_RAD(Lon);
	if (Incl < 0 || Incl > 180) {
		// retrograde orbits = (90 < Inclination < 180)
		fprintf(stderr, "PolarSatPosition:  inclination out of \
		    bounds: %f\n", Incl); 
		exit(1);
	}
	inclination_ = DEG_TO_RAD(Incl);
	// XXX: can't use "num = pow(initial_.r,3)" here because of linux lib
	double num = initial_.r * initial_.r * initial_.r;
	period_ = 2 * PI * sqrt(num/MU); // seconds
}


//
// The initial coordinate has the following properties:
// theta: 0 < theta < 2 * PI (essentially, this specifies initial position)  
// phi:  0 < phi < 2 * PI (longitude of ascending node)
// Return a coordinate with the following properties (i.e. convert to a true
// spherical coordinate):
// theta:  0 < theta < PI
// phi:  0 < phi < 2 * PI
//
coordinate PolarSatPosition::coord()
{
	coordinate current;
	double partial;  // fraction of orbit period completed
	partial = 
	    (fmod(NOW + time_advance_, period_)/period_) * 2*PI; //rad
	double theta_cur, phi_cur, theta_new, phi_new;

	// Compute current orbit-centric coordinates:
	// theta_cur adds effects of time (orbital rotation) to initial_.theta
	theta_cur = fmod(initial_.theta + partial, 2*PI);
	phi_cur = initial_.phi;
	// Reminder:  theta_cur and phi_cur are temporal translations of 
	// initial parameters and are NOT true spherical coordinates.
	//
	// Now generate actual spherical coordinates,
	// with 0 < theta_new < PI and 0 < phi_new < 360

	assert (inclination_ < PI);

	// asin returns value between -PI/2 and PI/2, so 
	// theta_new guaranteed to be between 0 and PI
	theta_new = PI/2 - asin(sin(inclination_) * sin(theta_cur));
	// if theta_new is between PI/2 and 3*PI/2, must correct
	// for return value of atan()
	if (theta_cur > PI/2 && theta_cur < 3*PI/2)
		phi_new = atan(cos(inclination_) * tan(theta_cur)) + 
			phi_cur + PI;
	else
		phi_new = atan(cos(inclination_) * tan(theta_cur)) + 
			phi_cur;
	phi_new = fmod(phi_new + 2*PI, 2*PI);
	
	current.r = initial_.r;
	current.theta = theta_new;
	current.phi = phi_new;
	return current;
}


//
// added by Lloyd Wood, 27 March 2000.
// allows us to distinguish between satellites that are ascending (heading north)
// and descending (heading south).
//
bool PolarSatPosition::isascending()
{	
	double partial = (fmod(NOW + time_advance_, period_)/period_) * 2*PI; //rad
	double theta_cur = fmod(initial_.theta + partial, 2*PI);
	if ((theta_cur > PI/2)&&(theta_cur < 3*PI/2)) {
		return 0;
	} else {
		return 1;
	}
}

int PolarSatPosition::command(int argc, const char*const* argv) {     
	Tcl& tcl = Tcl::instance();
        if (argc == 2) {
	}
	if (argc == 3) {
		if (strcmp(argv[1], "set_next") == 0) {
			next_ = (PolarSatPosition *) TclObject::lookup(argv[2]);
			if (next_ == 0) {
				tcl.resultf("no such object %s", argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
	}
	return (SatPosition::command(argc, argv));
}

/////////////////////////////////////////////////////////////////////
// class GeoSatPosition
/////////////////////////////////////////////////////////////////////

GeoSatPosition::GeoSatPosition(double longitude) 
{
	initial_.r = EARTH_RADIUS + GEO_ALTITUDE;
	initial_.theta = PI/2;
	set(longitude);
	type_ = POSITION_SAT_GEO;
	period_ = EARTH_PERIOD;
}

coordinate GeoSatPosition::coord()
{
	coordinate current;
	current.r = initial_.r;
	current.theta = initial_.theta;
	double fractional = 
	    (fmod(NOW + time_advance_, period_)/period_) *2*PI; // rad
	current.phi = fmod(initial_.phi + fractional, 2*PI);
	return current;
}

//
// Longitude is in the range (0, 180) with negative values -> west
//
void GeoSatPosition::set(double longitude)
{
	if (longitude < -180 || longitude > 180)
		fprintf(stderr, "GeoSatPosition:  longitude out of bounds %f\n",
		    longitude);
	if (longitude < 0)
		initial_.phi = DEG_TO_RAD(360 + longitude);
	else
		initial_.phi = DEG_TO_RAD(longitude);
}
