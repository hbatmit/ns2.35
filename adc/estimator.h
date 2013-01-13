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
 * @(#) $Header: /cvsroot/nsnam/ns-2/adc/estimator.h,v 1.8 2005/08/26 05:05:27 tomh Exp $
 */

//Estimator unit estimates average load every period interval of time

#ifndef ns_estimator_h
#define ns_estimator_h

#include "connector.h"
#include "measuremod.h"
#include "timer-handler.h"

class Estimator;

class Estimator_Timer : public TimerHandler {
public:
	Estimator_Timer(Estimator *est) : TimerHandler() { est_ = est;}
	
protected:
	virtual void expire(Event *e);
	Estimator *est_;
};

class Estimator : public NsObject {
public:
	Estimator();
	inline double avload() { return double(avload_);};

	inline virtual void change_avload(double incr) { avload_ += incr;}
	inline virtual void newflow(double) {};
	int command(int argc, const char*const* argv); 
	virtual void timeout(int);
	inline void recv(Packet *,Handler *){}
	virtual void start();
	void stop();
	void setmeasmod(MeasureMod *);
	void setactype(const char*);
	inline double &period(){ return period_;}
	void trace(TracedVar* v);
protected:
	MeasureMod *meas_mod_;
	TracedDouble avload_;
	double period_; 
	virtual void estimate()=0;
	Estimator_Timer est_timer_;
	TracedDouble measload_;
	Tcl_Channel tchan_;
	int src_;
	int dst_;
	double omeasload_;
	double oavload_;
	char *actype_;
};

#endif
