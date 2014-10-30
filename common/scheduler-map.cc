/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1994 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "config.h"
#ifdef HAVE_STL

// Event Scheduler using the standard library std::set
// Contributed by George F. Riley, Georgia Tech.  Spring 2002

#include <stdio.h>
#include <set>

#include "scheduler.h"


class MapScheduler : public Scheduler {
public:
	MapScheduler();
	~MapScheduler();
public:
	void cancel(Event*);
	void insert(Event*);
	Event* lookup(scheduler_uid_t uid);
	Event* deque();
	const Event *head() { return *EventQueue_.begin(); }
private:
	struct event_less_adapter {
		bool operator()(const Event *e1, const Event *e2) const
		{
			return e1->time_ < e2->time_ ||
				(e1->time_ == e2->time_	&& e1->uid_ < e2->uid_); // for FIFO
		}
	};
	typedef set<Event *, event_less_adapter> EventQueue_t;
	EventQueue_t EventQueue_;	// The actual event list
};

static class MapSchedulerClass : public TclClass {
public:
	MapSchedulerClass() : TclClass("Scheduler/Map") {}
	TclObject* create(int /* argc */, const char*const* /* argv */) {
		return (new MapScheduler);
	}
} class_stl_sched;

MapScheduler::MapScheduler()
{
}

MapScheduler::~MapScheduler()
{
}

void MapScheduler::cancel(Event* p)
{
	EventQueue_t::iterator eIT = EventQueue_.find(p);
	if (eIT != EventQueue_.end()) {
		EventQueue_.erase(eIT);
		p->uid_ = -p->uid_; // Negate the uid for reuse
	}
}

void MapScheduler::insert(Event* p)
{
	EventQueue_.insert(p);
}

Event* MapScheduler::lookup(scheduler_uid_t uid) // look for event
{
	for (EventQueue_t::iterator eIT = EventQueue_.begin();
	     eIT != EventQueue_.end(); 
	     ++eIT) {
		if ((*eIT)->uid_ == uid) 
			return (*eIT);
	}

	return 0;
}

Event* MapScheduler::deque()
{
	EventQueue_t::iterator eIT = EventQueue_.begin();
	if (eIT == EventQueue_.end()) 
		return 0;

	EventQueue_.erase(eIT);

	return *eIT;
}

#endif // HAVE_STL
