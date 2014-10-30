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

#ifndef ns_sa_h
#define ns_sa_h

#include "resv.h"
#include "agent.h"
#include "timer-handler.h"
#include "trafgen.h"
#include "rtp.h"
#include "random.h"

class SA_Agent;

class SA_Timer : public TimerHandler {
public:
        SA_Timer(SA_Agent *a) : TimerHandler() { a_ = a; }
protected:
        virtual void expire(Event *e);
        SA_Agent *a_;
};


class SA_Agent : public Agent {
public:
	SA_Agent();
	~SA_Agent();
	int command(int, const char*const*);
	virtual void timeout(int);
	virtual void sendmsg(int nbytes, const char *flags = 0);
protected:
	void start(); 
	void stop(); 
	void sendreq();
	void sendteardown();
	void recv(Packet *, Handler *);
        void stoponidle(const char *);
	virtual void sendpkt();
	double rate_;
	int bucket_;
	NsObject* ctrl_target_;
        TrafficGenerator *trafgen_;
        int rtd_;  /* ready-to-die: waiting for last burst to end */
	char* callback_;

	SA_Timer sa_timer_;
	double nextPkttime_;
	int running_;
	int seqno_;
	int size_;
};


#endif
