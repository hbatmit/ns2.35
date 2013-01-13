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
	"@(#) $Header: /cvsroot/nsnam/ns-2/adc/hb-adc.cc,v 1.6 2005/08/26 05:05:27 tomh Exp $";
#endif

//Hoeffding Bounds Admission Control

#include "adc.h"
#include <stdlib.h>
#include <math.h>

class HB_ADC : public ADC {
public:
	HB_ADC();
	void teardown_action(int,double,int);
	void rej_action(int,double,int);
protected:
	int admit_flow(int,double,int);
	int rejected_; 
	double epsilon_;
	double sump2_;
};

HB_ADC::HB_ADC() : rejected_(0), sump2_(0)
{
	bind("epsilon_", &epsilon_);
	type_ = new char[3];
	strcpy(type_, "HB");
}


int HB_ADC::admit_flow(int cl,double r,int b)
{
	//get peak rate this class of flow
	double p=peak_rate(cl,r,b);
	if (backoff_) {
		if (rejected_)
			return 0;
	}	
	//printf("Peak rate: %f Avload: %f Rem %f %f %f\n",p,est_[cl]->avload(),sqrt(log(1/epsilon_)*sump2_/2),log(1/epsilon_),sump2_);
	
	if ((p+est_[cl]->avload()+sqrt(log(1/epsilon_)*sump2_/2)) <= bandwidth_) {
		sump2_+= p*p;
		est_[cl]->change_avload(p);
		return 1;
	}
	else {
		rejected_=1;
		return 0;
	}
}


void HB_ADC::rej_action(int cl,double r,int b)
{
	double p=peak_rate(cl,r,b);
	sump2_ -= p*p;
}


void HB_ADC::teardown_action(int cl,double r,int b)
{
	rejected_=0;
	double p=peak_rate(cl,r,b);
	sump2_ -= p*p;
}

static class HB_ADCClass : public TclClass {
public:
	HB_ADCClass() : TclClass("ADC/HB") {}
	TclObject* create(int,const char*const*) {
		return (new HB_ADC());
	}
}class_hb_adc;
