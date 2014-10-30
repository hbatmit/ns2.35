
/*
 * qsagent.h
 * Copyright (C) 2001 by the University of Southern California
 * $Id: qsagent.h,v 1.5 2006/02/22 20:35:37 sallyfloyd Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */

/*
 * Quick Start for TCP and IP.
 * A scheme for transport protocols to dynamically determine initial 
 * congestion window size.
 *
 * http://www.ietf.org/internet-drafts/draft-amit-quick-start-02.ps
 *
 * This defines the Class for the Quick Start agent. "Agent/QSAgent"
 * 
 * qsagent.h
 *
 * Srikanth Sundarrajan, 2002
 * sundarra@usc.edu
 */

#ifndef _QSAGENT_H
#define _QSAGENT_H

#include <stdarg.h>

#include "object.h"
#include "agent.h"
#include "packet.h"
#include "hdr_qs.h"
#include "timer-handler.h"
#include "lib/bsd-list.h"
#include "queue.h"
#include "delay.h"

class QSAgent;

class QSTimer: public TimerHandler {
public:
	QSTimer(QSAgent * qs_handle) : TimerHandler() { qs_handle_ = qs_handle; }
	virtual void expire(Event * e);

protected:
	QSAgent * qs_handle_;
};

class Agent;
class QSAgent : public Agent{
public:

	virtual int command(int argc, const char*const* argv);
	virtual void recv(Packet*, Handler* callback = 0);

	TclObject * old_classifier_;

	int qs_enabled_;
	int algorithm_; // QS request processing algorithm
			// 3 for the recommended algorithm.
	double state_delay_;
	double alloc_rate_;
	double threshold_;  // for algorithm_ 3, utilized bandwidth
		            // must be less than threshold.
	int max_rate_;	// maximum rate for one rate request, for some
			// quickstart algorithms (but not for #3)
	int mss_;

	/*
	 * 1: Bps = rate * 10 KB
	 * 2: Bps = 2 ^ rate * 1024
	 */
	static int rate_function_;

	QSTimer qs_timer_;

	QSAgent();
	~QSAgent();

	double aggr_approval_;
	double prev_int_aggr_;

protected:
	double process(LinkDelay *link, Queue *queue, double ratereq);
};

#endif // _QSAGENT_H
