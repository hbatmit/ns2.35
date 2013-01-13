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
    "@(#) $Header: /cvsroot/nsnam/ns-2/adc/param-adc.cc,v 1.4 2005/08/26 05:05:27 tomh Exp $";
#endif


/* Parameter-based admission control.  Admission decisions are
 * based on the sum of reserved rates.
 */

#include "adc.h"
#include <stdlib.h>

class Param_ADC : public ADC {
public:
	Param_ADC();
	void teardown_action(int,double,int);
	void rej_action(int,double,int);
	void trace(TracedVar* v);
protected:
	int admit_flow(int,double,int);
	double utilization_;
	TracedDouble resv_rate_;
	double oresv_rate_;
};

Param_ADC::Param_ADC() : resv_rate_(0), oresv_rate_(0)
{
	bind("utilization_",&utilization_);
	type_ = new char[5];
	strcpy(type_, "PBAC");

	resv_rate_.tracer(this);
	resv_rate_.name("\"Reserved Rate\"");
}

void Param_ADC::rej_action(int /*cl*/,double p,int /*r*/)
{
	resv_rate_-=p;
}


void Param_ADC::teardown_action(int /*cl*/,double p,int /*r*/)
{
	resv_rate_-=p;
}

int Param_ADC::admit_flow(int /*cl*/,double r,int /*b*/)
{
	if (resv_rate_ + r <= utilization_ * bandwidth_) {
		resv_rate_ +=r;
		return 1;
	}
	return 0;
}

static class Param_ADCClass : public TclClass {
public:
	Param_ADCClass() : TclClass("ADC/Param") {}
	TclObject* create(int,const char*const*) {
		return (new Param_ADC());
	}
}class_param_adc;

void Param_ADC::trace(TracedVar* v)
{
	char wrk[500];
	double *p, newval;

	/* check for right variable */
	if (strcmp(v->name(), "\"Reserved Rate\"") == 0) {
		p = &oresv_rate_;
	}
	else {
		fprintf(stderr, "PBAC: unknown trace var %s\n", v->name());
		return;
	}

	newval = double(*((TracedDouble*)v));

	if (tchan_) {
		int n;
		double t = Scheduler::instance().clock();
		/* f -t 0.0 -s 1 -a SA -T v -n Num -v 0 -o 0 */
		sprintf(wrk, "f -t %g -s %d -a %s:%d-%d -T v -n %s -v %g -o %g",
			t, src_, type_, src_, dst_, v->name(), newval, *p);
		n = strlen(wrk);
		wrk[n] = '\n';
		wrk[n+1] = 0;
		(void)Tcl_Write(tchan_, wrk, n+1);
		
	}

	*p = newval;

	return;



}

