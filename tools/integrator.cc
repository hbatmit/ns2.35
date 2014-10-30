/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1996 Regents of the University of California.
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
 * 	This product includes software developed by the MASH Research
 * 	Group at the University of California Berkeley.
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
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/tools/integrator.cc,v 1.8 1998/06/27 01:23:57 gnguyen Exp $";
#endif

#include <stdlib.h>
#include "integrator.h"

static class IntegratorClass : public TclClass {
 public:
	IntegratorClass() : TclClass("Integrator") {}
	TclObject* create(int, const char*const*) {
		return (new Integrator);
	}
} integrator_class;

Integrator::Integrator() : lastx_(0.), lasty_(0.), sum_(0.)
{
	bind("lastx_", &lastx_);
	bind("lasty_", &lasty_);
	bind("sum_", &sum_);
}

void Integrator::set(double x, double y)
{
	lastx_ = x;
	lasty_ = y;
	sum_ = 0.;
}

void Integrator::newPoint(double x, double y)
{
	sum_ += (x - lastx_) * lasty_;
	lastx_ = x;
	lasty_ = y;
}

int Integrator::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "reset") == 0) {
			set(0., 0.);
			return (TCL_OK);
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "newpoint") == 0) {
			double x = atof(argv[2]);
			double y = atof(argv[3]);
			newPoint(x, y);
			return (TCL_OK);
		}
	}
	return (TclObject::command(argc, argv));
}

/*
 * interface for the 'Samples' class, will probably want to move
 * to some sort of "stats" file at some point
 */
static class SamplesClass : public TclClass {
 public:
	SamplesClass() : TclClass("Samples") {}
	TclObject* create(int, const char*const*) {
		return (new Samples);
	}
} samples_class;

int Samples::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "mean") == 0) {
			if (cnt_ > 0) {
				Tcl::instance().resultf("%g", mean());
				return (TCL_OK);
			}
			Tcl::instance().resultf("tried to take mean with no sample points");
			return (TCL_ERROR);
		}
		if (strcmp(argv[1], "cnt") == 0) {
			Tcl::instance().resultf("%u", cnt());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "variance") == 0) {
			if (cnt_ == 1) {
				Tcl::instance().resultf("0.0");
				return (TCL_OK);
			}
			if (cnt_ > 2) {
				Tcl::instance().resultf("%g", variance());
				return (TCL_OK);
			}
			return (TCL_ERROR);
		}
		if (strcmp(argv[1], "reset") == 0) {
			reset();
			return (TCL_OK);
		}
	} else if ( argc == 3 ) {
		if ( strcmp(argv[1],"newpoint") == 0 ) {
			double x = atof(argv[2]);
			newPoint(x);
			return (TCL_OK);
		}
	}
	return (TclObject::command(argc, argv));
}
