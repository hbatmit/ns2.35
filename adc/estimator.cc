/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linking this file statically or dynamically with other modules is making
 * a combined work based on this file.  Thus, the terms and conditions of
 * the GNU General Public License cover the whole combination.
 *
 * In addition, as a special exception, the copyright holders of this file
 * give you permission to combine this file with free software programs or
 * libraries that are released under the GNU LGPL and with code included in
 * the standard release of ns-2 under the Apache 2.0 license or under
 * otherwise-compatible licenses with advertising requirements (or modified
 * versions of such code, with unchanged license).  You may copy and
 * distribute such a system following the terms of the GNU GPL for this
 * file and the licenses of the other code concerned, provided that you
 * include the source code of that other code when and as the GNU GPL
 * requires distribution of source code.
 *
 * Note that people who make modified versions of this file are not
 * obligated to grant this special exception for their modified versions;
 * it is their choice whether to do so.  The GNU General Public License
 * gives permission to release a modified version without this exception;
 * this exception also makes it possible to release a modified version
 * which carries forward this exception.
 */
#ifndef lint
static const char rcsid[] =
	"@(#) $Header: /cvsroot/nsnam/ns-2/adc/estimator.cc,v 1.8 2005/08/26 05:05:27 tomh Exp $";
#endif

#include "estimator.h"

Estimator::Estimator() : meas_mod_(0),avload_(0.0),est_timer_(this), measload_(0.0), tchan_(0), omeasload_(0), oavload_(0)
{
	bind("period_",&period_);
	bind("src_", &src_);
	bind("dst_", &dst_);

	avload_.tracer(this);
	avload_.name("\"Estimated Util.\"");
	measload_.tracer(this);
	measload_.name("\"Measured Util.\"");
}

int Estimator::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc==2) {
		if (strcmp(argv[1],"load-est") == 0) {
			tcl.resultf("%.3f",double(avload_));
			return(TCL_OK);
		} else if (strcmp(argv[1],"link-utlzn") == 0) {
			tcl.resultf("%.3f",meas_mod_->bitcnt()/period_);
			return(TCL_OK);
		}
	}
	if (argc == 3) {
		if (strcmp(argv[1], "attach") == 0) {
			int mode;
			const char* id = argv[2];
			tchan_ = Tcl_GetChannel(tcl.interp(), (char*)id, &mode);
			if (tchan_ == 0) {
				tcl.resultf("Estimator: trace: can't attach %s for writing", id);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "setbuf") == 0) {
			/* some sub classes actually do something here */
			return(TCL_OK);
		}
	}
	return NsObject::command(argc,argv);
}

void Estimator::setmeasmod (MeasureMod *measmod)
{
	meas_mod_=measmod;
}

void Estimator::start()
{
	avload_=0;
	measload_ = 0;
	est_timer_.resched(period_);
}

void Estimator::stop()
{
	est_timer_.cancel();
}

void Estimator::timeout(int)
{
	estimate();
	est_timer_.resched(period_);
}

void Estimator_Timer::expire(Event* /*e*/) 
{
	est_->timeout(0);
}

void Estimator::trace(TracedVar* v)
{
	char wrk[500];
	double *p, newval;

	/* check for right variable */
	if (strcmp(v->name(), "\"Estimated Util.\"") == 0) {
		p = &oavload_;
	}
	else if (strcmp(v->name(), "\"Measured Util.\"") == 0) {
		p = &omeasload_;
	}
	else {
		fprintf(stderr, "Estimator: unknown trace var %s\n", v->name());
		return;
	}

	newval = double(*((TracedDouble*)v));

	if (tchan_) {
		int n;
		double t = Scheduler::instance().clock();
		/* f -t 0.0 -s 1 -a SA -T v -n Num -v 0 -o 0 */
		sprintf(wrk, "f -t %g -s %d -a %s:%d-%d -T v -n %s -v %g -o %g",
			t, src_, actype_, src_, dst_, v->name(), newval, *p);
		n = strlen(wrk);
		wrk[n] = '\n';
		wrk[n+1] = 0;
		(void)Tcl_Write(tchan_, wrk, n+1);
		
	}

	*p = newval;

	return;



}

void Estimator::setactype(const char* type)
{
	actype_ = new char[strlen(type)+1];
	strcpy(actype_, type);
	return;
}
