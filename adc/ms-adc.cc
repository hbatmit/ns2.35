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
	"@(#) $Header: /cvsroot/nsnam/ns-2/adc/ms-adc.cc,v 1.7 2005/08/26 05:05:27 tomh Exp $";
#endif

//Measured Sum Admission control

#include "adc.h"
#include <stdlib.h>

class MS_ADC : public ADC {
public:
	MS_ADC();
	void rej_action(int, double,int);
protected:
	int admit_flow(int,double,int);
	inline virtual double get_rate(int /*cl*/, double r,int /*b*/) 
		{ return r ; };
	double utilization_;
};

MS_ADC::MS_ADC() 
{
	bind("utilization_",&utilization_);
	type_ = new char[5];
	strcpy(type_, "MSAC");
}

void MS_ADC::rej_action(int cl,double r,int b)
{
	double rate = get_rate(cl,r,b);
	est_[cl]->change_avload(-rate);
}


int MS_ADC::admit_flow(int cl,double r,int b)
{
	double rate = get_rate(cl,r,b);
	if (rate+est_[cl]->avload() < utilization_* bandwidth_) {
		est_[cl]->change_avload(rate);
		return 1;
	}
	return 0;
}


static class MS_ADCClass : public TclClass {
public:
	MS_ADCClass() : TclClass("ADC/MS") {}
	TclObject* create(int,const char*const*) {
		return (new MS_ADC());
	}
}class_ms_adc;


/* a measured sum algorithm that uses peak rather than token rate
 */

class MSPK_ADC : public MS_ADC {
public:
	MSPK_ADC() { };
protected:
	inline virtual double get_rate(int cl,double r,int b){
		return(peak_rate(cl,r,b));};
};

static class MSPK_ADCClass : public TclClass {
public:
	MSPK_ADCClass() : TclClass("ADC/MSPK") {}
	TclObject* create(int,const char*const*) {
		return (new MSPK_ADC());
	}
} class_mspk_adc;
