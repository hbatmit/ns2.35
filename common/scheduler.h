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
 * @(#) $Header: /cvsroot/nsnam/ns-2/common/scheduler.h,v 1.28 2007/12/04 19:59:31 seashadow Exp $ (LBL)
 */

#ifndef ns_scheduler_h
#define ns_scheduler_h

#include "config.h"

// Make use of 64 bit integers if available.
#ifdef HAVE_INT64
typedef int64_t scheduler_uid_t;
#define UID_PRINTF_FORMAT STRTOI64_FMTSTR
#define STRTOUID(S) STRTOI64((S), NULL, 0)
#else
typedef int scheduler_uid_t;
#define UID_PRINTF_FORMAT "%d"
#define STRTOUID(S) atoi((S))
#endif


class Handler;

class Event {
public:
	Event* next_;		/* event list */
	Event* prev_;
	Handler* handler_;	/* handler to call when event ready */
	double time_;		/* time at which event is ready */
	scheduler_uid_t uid_;	/* unique ID */
	Event() : time_(0), uid_(0) {}
};

/*
 * The base class for all event handlers.  When an event's scheduled
 * time arrives, it is passed to handle which must consume it.
 * i.e., if it needs to be freed it, it must be freed by the handler.
 */
class Handler {
 public:
	virtual ~Handler () {}
	virtual void handle(Event* event) = 0;
};

#define	SCHED_START	0.0	/* start time (secs) */

class Scheduler : public TclObject {
public:
	static Scheduler& instance() {
		return (*instance_);		// general access to scheduler
	}
	void schedule(Handler*, Event*, double delay);	// sched later event
	virtual void run();			// execute the simulator
	virtual void cancel(Event*) = 0;	// cancel event
	virtual void insert(Event*) = 0;	// schedule event
	virtual Event* lookup(scheduler_uid_t uid) = 0;	// look for event
	virtual Event* deque() = 0;		// next event (removes from q)
	virtual const Event* head() = 0;	// next event (not removed from q)
	double clock() const {			// simulator virtual time
		return (clock_);
	}
	virtual void sync() {};
	virtual double start() {		// start time
		return SCHED_START;
	}
	virtual void reset();
protected:
	void dumpq();	// for debug: remove + print remaining events
	void dispatch(Event*);	// execute an event
	void dispatch(Event*, double);	// exec event, set clock_
	Scheduler();
	virtual ~Scheduler();
	int command(int argc, const char*const* argv);
	double clock_;
	int halted_;
	static Scheduler* instance_;
	static scheduler_uid_t uid_;
};

class ListScheduler : public Scheduler {
public:
	ListScheduler() : queue_(0) {}
	void cancel(Event*);
	void insert(Event*);
	Event* deque();
	const Event* head() { return queue_; }
	Event* lookup(scheduler_uid_t uid);

protected:
	Event* queue_;
};

#include "heap.h"

class HeapScheduler : public Scheduler {
public:
	HeapScheduler() { hp_ = new Heap; } 
	void cancel(Event* e) {
		if (e->uid_ <= 0)
			return;
		e->uid_ = - e->uid_;
		hp_->heap_delete((void*) e);
	}
	void insert(Event* e) {
		hp_->heap_insert(e->time_, (void*) e);
	}
	Event* lookup(scheduler_uid_t uid);
	Event* deque();
	const Event* head() { return (const Event *)hp_->heap_min(); }
protected:
	Heap* hp_;
};

class CalendarScheduler : public Scheduler {
public:
	CalendarScheduler();
	~CalendarScheduler();
	void cancel(Event*);
	void insert(Event*);
	Event* lookup(scheduler_uid_t uid);
	Event* deque();
	const Event* head();

protected:
	double min_bin_width_;		// minimum bin width for Calendar Queue
	unsigned int adjust_new_width_interval_; // The interval (in unit of resize time) for adjustment of bin width. A zero value disables automatic bin width adjustment
	unsigned time_to_newwidth;	// how many time we failed to adjust the width based on snoopy-queue
	long unsigned head_search_;
	long unsigned insert_search_;
	int round_num_;
	long int gap_num_;		//the number of gap samples in this window (in process of calculation)
	double last_time_;		//the departure time of first event in this window
	double avg_gap_;		//the average gap in last window (finished calculation)

	double width_;
	double diff0_, diff1_, diff2_; /* wrap-around checks */

	int stat_qsize_;		/* # of distinct priorities in queue*/
	int nbuckets_;
	int lastbucket_;
	int top_threshold_;
	int bot_threshold_;

	struct Bucket {
		Event *list_;
		int    count_;
	} *buckets_;
		
	int qsize_;

	virtual void reinit(int nbuck, double bwidth, double start);
	virtual void resize(int newsize, double start);
	virtual double newwidth(int newsize);

private:
	virtual void insert2(Event*);
	double cal_clock_;  // same as clock in sims, may be different in RT-scheduling.

};

class SplayScheduler : public Scheduler 
{
public:
	SplayScheduler() : root_(0), qsize_(0) {}
	void insert(Event *);
	Event *deque();
	const Event *head();
	void cancel(Event *);
	Event *lookup(scheduler_uid_t);

	//void validate() { assert(validate(root_) == qsize_); };
    
protected:
	/* XXX even if debug is enabled, we want these inlined, so
	   XXX they are defined as macros in splay-scheduler.cc
	   Event *&LEFT(Event *e)  { return e->prev_; }
	   Event *&RIGHT(Event *e) { return e->next_; }
	*/
	Event *uid_lookup(Event *);

	Event			*root_;
	scheduler_uid_t 	lookup_uid_;
	int 			qsize_;
private:
	int validate(Event *);
};


#endif
