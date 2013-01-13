/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * Copyright (C) 1997 by the University of Southern California
 * $Id: timer-handler.h,v 1.9 2005/08/25 18:58:02 johnh Exp $
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
 * @(#) $Header: /cvsroot/nsnam/ns-2/common/timer-handler.h,v 1.9 2005/08/25 18:58:02 johnh Exp $ (USC/ISI)
 */

#ifndef timer_handler_h
#define timer_handler_h

#include "scheduler.h"

/*
 * Abstract base class to deal with timer-style handlers.
 *
 *
 * To define a new timer, subclass this function and define handle:
 *
 * class MyTimer : public TimerHandler {
 * public:
 *	MyTimer(MyAgentClass *a) : AgentTimerHandler() { a_ = a; }
 *	virtual double expire(Event *e);
 * protected:
 *	MyAgentClass *a_;
 * };
 *
 * Then define expire.
 *
 * Often MyTimer will be a friend of MyAgentClass,
 * or expire() will only call a function of MyAgentClass.
 *
 * See tcp-rbp.{cc,h} for a real example.
 */
#define TIMER_HANDLED -1.0	// xxx: should be const double in class?

class TimerHandler : public Handler {
public:
	TimerHandler() : status_(TIMER_IDLE) { }

	void sched(double delay);	// cannot be pending
	void resched(double delay);	// may or may not be pending
					// if you don't know the pending status
					// call resched()
	void cancel();			// must be pending
	inline void force_cancel() {	// cancel!
		if (status_ == TIMER_PENDING) {
			_cancel();
			status_ = TIMER_IDLE;
		}
	}
	enum TimerStatus { TIMER_IDLE, TIMER_PENDING, TIMER_HANDLING };
	int status() { return status_; };

protected:
	virtual void expire(Event *) = 0;  // must be filled in by client
	// Should call resched() if it wants to reschedule the interface.

	virtual void handle(Event *);
	int status_;
	Event event_;

private:
	inline void _sched(double delay) {
		(void)Scheduler::instance().schedule(this, &event_, delay);
	}
	inline void _cancel() {
		(void)Scheduler::instance().cancel(&event_);
		// no need to free event_ since it's statically allocated
	}
};

// Local Variables:
// mode:c++
// End:

#endif /* timer_handler_h */
