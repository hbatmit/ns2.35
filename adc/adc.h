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
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/adc/adc.h,v 1.6 2005/08/26 05:05:27 tomh Exp $
 */

#ifndef ns_adc_h
#define ns_adc_h

#include "estimator.h"

#define CLASS 10
class ADC : public NsObject {
public:
	ADC();
	int command(int,const char*const*);
	virtual int admit_flow(int cl,double r, int b)=0;
	virtual void rej_action(int,double,int){}; 
	virtual void teardown_action(int,double,int){}; 
	inline void recv(Packet*, Handler *){} 
	inline void setest(int cl,Estimator *est) {est_[cl]=est;
						 est_[cl]->setactype(type_);}
	double peak_rate(int cl,double r,int b) {return r+b/est_[cl]->period();}
	char *type() {return type_;}
protected:
	Estimator *est_[CLASS];
	double bandwidth_;
	char *type_;
	Tcl_Channel tchan_;
	int src_;
	int dst_;
	int backoff_;
	int dobump_;
};

#endif





