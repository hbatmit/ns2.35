/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * Copyright (C) 1997 by the University of Southern California
 * $Id: timer-handler.cc,v 1.9 2006/02/21 15:20:18 mahrenho Exp $
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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/common/timer-handler.cc,v 1.9 2006/02/21 15:20:18 mahrenho Exp $ (USC/ISI)";
#endif

#include <stdlib.h>  // abort()
#include "timer-handler.h"

void
TimerHandler::cancel()
{
	if (status_ != TIMER_PENDING) {
		fprintf(stderr,
		  "Attempting to cancel timer at %p which is not scheduled\n",
		  reinterpret_cast<void *>(this));
		abort();
	}
	_cancel();
	status_ = TIMER_IDLE;
}

/* sched checks the state of the timer before shceduling the
 * event. It the timer is already set, abort is called.
 * This is different than the OTcl timers in tcl/ex/timer.tcl,
 * where sched is the same as reshced, and no timer state is kept.
 */
void
TimerHandler::sched(double delay)
{
	if (status_ != TIMER_IDLE) {
		fprintf(stderr,"Couldn't schedule timer");
		abort();
	}
	_sched(delay);
	status_ = TIMER_PENDING;
}

void
TimerHandler::resched(double delay)
{
	if (status_ == TIMER_PENDING)
		_cancel();
	_sched(delay);
	status_ = TIMER_PENDING;
}

void
TimerHandler::handle(Event *e)
{
	if (status_ != TIMER_PENDING)   // sanity check
		abort();
	status_ = TIMER_HANDLING;
	expire(e);
	// if it wasn't rescheduled, it's done
	if (status_ == TIMER_HANDLING)
		status_ = TIMER_IDLE;
}
