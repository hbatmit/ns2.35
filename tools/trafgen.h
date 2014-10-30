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
 * Changes by the Daedalus group, http://daedalus.cs.berkeley.edu
 *	Add Application interface
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/tools/trafgen.h,v 1.7 2005/08/26 05:05:31 tomh Exp $ (Xerox)
 */

#ifndef ns_trafgen_h
#define ns_trafgen_h

#include "app.h"
#include "timer-handler.h"

class TrafficGenerator;

class TrafficTimer : public TimerHandler {
public:
	TrafficTimer(TrafficGenerator* tg) : tgen_(tg) {}
protected:
	void expire(Event*);
	TrafficGenerator* tgen_;
};


/* abstract class for traffic generation modules.  derived classes
 * must define  the next_interval() function.  the traffic generation 
 * module schedules an event for a UDP_Agent when it is time to 
 * generate a new packet.  it passes the packet size with the event 
 * (to accommodate traffic generation modules that may not use fixed 
 * size packets).
 */

/* abstract class for traffic generation modules.  derived classes
 * must define the next_interva() function that is invoked by
 * UDP_Agent.  This function returns the time to the next packet
 * is generated and sets the size of the packet (which is passed
 * by reference).  The init() method is called from the start()
 * method of the UDP_Agent.  It can do any one-time initialization
 * needed by the traffic generation process.
 */

class TrafficGenerator : public Application {
public:
	TrafficGenerator();
	virtual double next_interval(int &) = 0;
	virtual void init() {}
	virtual double interval() { return 0; }
	virtual int on() { return 0; }
	virtual void timeout();

	virtual void recv() {}
	virtual void resume() {}

protected:
	virtual void start();
	virtual void stop();

	double nextPkttime_;
	int size_;
	int running_;
	TrafficTimer timer_;
};

#endif
