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
 * @(#) $Header: /cvsroot/nsnam/ns-2/common/scheduler.cc,v 1.76 2009/01/01 03:42:13 tom_henderson Exp $
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/common/scheduler.cc,v 1.76 2009/01/01 03:42:13 tom_henderson Exp $ (LBL)";
#endif

#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include "config.h"
#include "scheduler.h"
#include "packet.h"


#ifdef MEMDEBUG_SIMULATIONS
#include "mem-trace.h"
#endif

Scheduler* Scheduler::instance_;
scheduler_uid_t Scheduler::uid_ = 1;

// class AtEvent : public Event {
// public:
// 	char* proc_;
// };

Scheduler::Scheduler() : clock_(SCHED_START), halted_(0)
{
}

Scheduler::~Scheduler(){
	instance_ = NULL ;
}

/*
 * Schedule an event delay time units into the future.
 * The event will be dispatched to the specified handler.
 * We use a relative time to avoid the problem of scheduling
 * something in the past.
 *
 * Scheduler::schedule does a fair amount of error checking
 * because debugging problems when events are triggered
 * is much harder (because we've lost all context about who did
 * the scheduling).
 */
void 
Scheduler::schedule(Handler* h, Event* e, double delay)
{
	// handler should ALWAYS be set... if it's not, it's a bug in the caller
	if (!h) {
		fprintf(stderr,
			"Scheduler: attempt to schedule an event with a NULL handler."
			"  Don't DO that at time %f\n", clock_);
		abort();
	};
	
	if (e->uid_ > 0) {
		printf("Scheduler: Event UID not valid!\n\n");
		abort();
	}
	
	if (delay < 0) {
		// You probably don't want to do this
		// (it probably represents a bug in your simulation).
		fprintf(stderr, 
			"warning: ns Scheduler::schedule: scheduling event\n\t"
			"with negative delay (%f) at time %f.\n", delay, clock_);
	}

	if (uid_ < 0) {
		fprintf(stderr, "Scheduler: UID space exhausted!\n");
		abort();
	}
	e->uid_ = uid_++;
	e->handler_ = h;
	double t = clock_ + delay;

	e->time_ = t;
	insert(e);
}

void
Scheduler::run()
{
	instance_ = this;
	Event *p;
	/*
	 * The order is significant: if halted_ is checked later,
	 * event p could be lost when the simulator resumes.
	 * Patch by Thomas Kaemer <Thomas.Kaemer@eas.iis.fhg.de>.
	 */
	while (!halted_ && (p = deque())) {
		dispatch(p, p->time_);
	}
}

/*
 * dispatch a single simulator event by setting the system
 * virtul clock to the event's timestamp and calling its handler.
 * Note that the event may have side effects of placing other items
 * in the scheduling queue
 */

void
Scheduler::dispatch(Event* p, double t)
{
	if (t < clock_) {
		fprintf(stderr, "ns: scheduler going backwards in time from %f to %f.\n", clock_, t);
		abort();
	}

	clock_ = t;
	p->uid_ = -p->uid_;	// being dispatched
	p->handler_->handle(p);	// dispatch
}

void
Scheduler::dispatch(Event* p)
{
	dispatch(p, p->time_);
}

class AtEvent : public Event {
public:
	AtEvent() : proc_(0) {
	}
	~AtEvent() {
		if (proc_) delete [] proc_;
	}
	char* proc_;
};

class AtHandler : public Handler {
public:
	void handle(Event* event);
} at_handler;

void 
AtHandler::handle(Event* e)
{
	AtEvent* at = (AtEvent*)e;
	Tcl::instance().eval(at->proc_);
	delete at;
}

void
Scheduler::reset()
{
	clock_ = SCHED_START;
}

int 
Scheduler::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (instance_ == 0)
		instance_ = this;
	if (argc == 2) {
		if (strcmp(argv[1], "run") == 0) {
			/* set global to 0 before calling object reset methods */
			reset();	// sets clock to zero
			run();
			return (TCL_OK);
		} else if (strcmp(argv[1], "now") == 0) {
			sprintf(tcl.buffer(), "%.17g", clock());
			tcl.result(tcl.buffer());
			return (TCL_OK);
		} else if (strcmp(argv[1], "resume") == 0) {
			halted_ = 0;
			run();
			return (TCL_OK);
		} else if (strcmp(argv[1], "halt") == 0) {
			halted_ = 1;
			return (TCL_OK);

		} else if (strcmp(argv[1], "clearMemTrace") == 0) {
#ifdef MEMDEBUG_SIMULATIONS
			extern MemTrace *globalMemTrace;
			if (globalMemTrace)
				globalMemTrace->diff("Sim.");
#endif
			return (TCL_OK);
		} else if (strcmp(argv[1], "is-running") == 0) {
			sprintf(tcl.buffer(), "%d", !halted_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "dumpq") == 0) {
			if (!halted_) {
				fprintf(stderr, "Scheduler: dumpq only allowed while halted\n");
				tcl.result("0");
				return (TCL_ERROR);
			}
			dumpq();
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "at") == 0 ||
		    strcmp(argv[1], "cancel") == 0) {
			Event* p = lookup(STRTOUID(argv[2]));
			if (p != 0) {
				/*XXX make sure it really is an atevent*/
				cancel(p);
				AtEvent* ae = (AtEvent*)p;
				delete ae;
			}
		} else if (strcmp(argv[1], "at-now") == 0) {
			const char* proc = argv[2];

			// "at [$ns now]" may not work because of tcl's 
			// string number resolution
			AtEvent* e = new AtEvent;
			int n = strlen(proc);
			e->proc_ = new char[n + 1];
			strcpy(e->proc_, proc);
			schedule(&at_handler, e, 0);
			sprintf(tcl.buffer(), UID_PRINTF_FORMAT, e->uid_);
			tcl.result(tcl.buffer());
		}
		return (TCL_OK);
	} else if (argc == 4) {
		if (strcmp(argv[1], "at") == 0) {
			/* t < 0 means relative time: delay = -t */
			double delay, t = atof(argv[2]);
			const char* proc = argv[3];

			AtEvent* e = new AtEvent;
			int n = strlen(proc);
			e->proc_ = new char[n + 1];
			strcpy(e->proc_, proc);
			delay = (t < 0) ? -t : t - clock();
			if (delay < 0) {
				tcl.result("can't schedule command in past");
				return (TCL_ERROR);
			}
			schedule(&at_handler, e, delay);
			sprintf(tcl.buffer(), UID_PRINTF_FORMAT, e->uid_);
			tcl.result(tcl.buffer());
			return (TCL_OK);
		}
	}
	return (TclObject::command(argc, argv));
}

void
Scheduler::dumpq()
{
	Event *p;

	printf("Contents of scheduler queue (events) [cur time: %f]---\n",
		clock());
	while ((p = deque()) != NULL) {
		printf("t:%f uid: ", p->time_);
		printf(UID_PRINTF_FORMAT, p->uid_);
		printf(" handler: %p\n", reinterpret_cast<void *>(p->handler_) );
	}
}

static class ListSchedulerClass : public TclClass {
public:
	ListSchedulerClass() : TclClass("Scheduler/List") {}
	TclObject* create(int /* argc */, const char*const* /* argv */) {
		return (new ListScheduler);
	}
} class_list_sched;

void 
ListScheduler::insert(Event* e)
{
	double t = e->time_;
	Event** p;
	for (p = &queue_; *p != 0; p = &(*p)->next_)
		if (t < (*p)->time_)
			break;
	e->next_ = *p;
	*p = e;
}

/*
 * Cancel an event.  It is an error to call this routine
 * when the event is not actually in the queue.  The caller
 * must free the event if necessary; this routine only removes
 * it from the scheduler queue.
 */
void 
ListScheduler::cancel(Event* e)
{
	Event** p;
	if (e->uid_ <= 0)	// event not in queue
		return;
	for (p = &queue_; *p != e; p = &(*p)->next_)
		if (*p == 0)
			abort();

	*p = (*p)->next_;
	e->uid_ = - e->uid_;
}

Event* 
ListScheduler::lookup(scheduler_uid_t uid)
{
	Event* e;
	for (e = queue_; e != 0; e = e->next_)
		if (e->uid_ == uid)
			break;
	return (e);
}


Event*
ListScheduler::deque()
{ 
	Event* e = queue_;
	if (e)
		queue_ = e->next_;
	return (e);
}

#include "heap.h"

Heap::Heap(int size)
		: h_s_key(0), h_size(0), h_maxsize(size), h_iter(0)
{
	h_elems = new Heap_elem[h_maxsize];
	memset(h_elems, 0, h_maxsize*sizeof(Heap_elem));
	//for (unsigned int i = 0; i < h_maxsize; i++)
	//	h_elems[i].he_elem = 0;
}

Heap::~Heap()
{
	delete [] h_elems;
}

/*
 * int	heap_member(Heap *h, void *elem):		O(n) algorithm.
 *
 *	Returns index(elem \in h->he_elems[]) + 1,
 *			if elem \in h->he_elems[],
 *		0,	otherwise.
 *	External callers should just test for zero, or non-zero.
 *	heap_delete() uses this routine to find an element in the heap.
 */
int
Heap::heap_member(void* elem)
{
	unsigned int i;
	Heap::Heap_elem* he;
	for (i = 0, he = h_elems; i < h_size; i++, he++)
		if (he->he_elem == elem)
			return ++i;
	return 0;
}

/*
 * int	heap_delete(Heap *h, void *elem):		O(n) algorithm
 *
 *	Returns 1 for success, 0 otherwise.
 *
 * find elem in h->h_elems[] using heap_member()
 *
 * To actually remove the element from the tree, first swap it to the
 * root (similar to the procedure applied when inserting a new
 * element, but no key comparisons--just get it to the root).
 *
 * Then call heap_extract_min() to remove it & fix the tree.
 * 	This process is not the most efficient, but we do not
 *	particularily care about how fast heap_delete() is.
 *	Besides, heap_member() is already O(n), 
 *	and is the dominating cost.
 *
 * Actually remove the element by calling heap_extract_min().
 * 	The key that is now at the root is not necessarily the
 *	minimum, but heap_extract_min() does not care--it just
 *	removes the root.
 */
int
Heap::heap_delete(void* elem)
{
	int	i;
	if ((i = heap_member(elem)) == 0)
		return 0;
	for (--i; i; i = parent(i)) {
		swap(i, parent(i));
	}
	(void) heap_extract_min();
	return 1;
}

/*
 * void	heap_insert(Heap *h, heap_key_t *key, void *elem)
 *
 * Insert <key, elem> into heap h.
 * Adjust heap_size if we hit the limit.
 * 
 *	i := heap_size(h)
 *	heap_size := heap_size + 1
 *	while (i > 0 and key < h[Parent(i)])
 *	do	h[i] := h[Parent(i)]
 *		i := Parent(i)
 *	h[i] := key
 */
void
Heap::heap_insert(heap_key_t key, void* elem) 
{
	unsigned int	i, par;
	if (h_maxsize == h_size) {	/* Adjust heap_size */
		unsigned int osize = h_maxsize;
		Heap::Heap_elem *he_old = h_elems;
		h_maxsize *= 2;
		h_elems = new Heap::Heap_elem[h_maxsize];
		memcpy(h_elems, he_old, osize*sizeof(Heap::Heap_elem));
		delete []he_old;
	}

	i = h_size++;
	par = parent(i);
	while ((i > 0) && 
	       (KEY_LESS_THAN(key, h_s_key,
			      h_elems[par].he_key, h_elems[par].he_s_key))) {
		h_elems[i] = h_elems[par];
		i = par;
		par = parent(i);
	}
	h_elems[i].he_key  = key;
	h_elems[i].he_s_key= h_s_key++;
	h_elems[i].he_elem = elem;
	return;
}
		
/*
 * void *heap_extract_min(Heap *h)
 *
 *	Returns the smallest element in the heap, if it exists.
 *	NULL otherwise.
 *
 *	if heap_size[h] == 0
 *		return NULL
 *	min := h[0]
 *	heap_size[h] := heap_size[h] - 1   # C array indices start at 0
 *	h[0] := h[heap_size[h]]
 *	Heapify:
 *		i := 0
 *		while (i < heap_size[h])
 *		do	l = HEAP_LEFT(i)
 *			r = HEAP_RIGHT(i)
 *			if (r < heap_size[h])
 *				# right child exists =>
 *				#       left child exists
 *				then	if (h[l] < h[r])
 *						then	try := l
 *						else	try := r
 *				else
 *			if (l < heap_size[h])
 *						then	try := l
 *						else	try := i
 *			if (h[try] < h[i])
 *				then	HEAP_SWAP(h[i], h[try])
 *					i := try
 *				else	break
 *		done
 */
void*
Heap::heap_extract_min()
{
	void*	min;				  /* return value */
	unsigned int	i;
	unsigned int	l, r, x;

	if (h_size == 0)
		return 0;
	min = h_elems[0].he_elem;
	h_elems[0] = h_elems[--h_size];
// Heapify:
	i = 0;
	while (i < h_size) {
		l = left(i);
		r = right(i);
		if (r < h_size) {
			if (KEY_LESS_THAN(h_elems[l].he_key, h_elems[l].he_s_key,
					  h_elems[r].he_key, h_elems[r].he_s_key))
				x= l;
			else
				x= r;
		} else
			x = (l < h_size ? l : i);
		if ((x != i) && 
		    (KEY_LESS_THAN(h_elems[x].he_key, h_elems[x].he_s_key,
				   h_elems[i].he_key, h_elems[i].he_s_key))) {
			swap(i, x);
			i = x;
		} else {
			break;
		}
	}
	return min;
}


static class HeapSchedulerClass : public TclClass {
public:
	HeapSchedulerClass() : TclClass("Scheduler/Heap") {}
	TclObject* create(int /* argc */, const char*const* /* argv */) {
		return (new HeapScheduler);
	}
} class_heap_sched;

Event* 
HeapScheduler::lookup(scheduler_uid_t uid)
{
	Event* e;
	
	for (e = (Event*) hp_->heap_iter_init(); e;
	     e = (Event*) hp_->heap_iter())
		if (e->uid_ == uid)
			break;
	return e;
}

Event*
HeapScheduler::deque()
{
	return ((Event*) hp_->heap_extract_min());
}

/*
 * Calendar queue scheduler contributed by
 * David Wetherall <djw@juniper.lcs.mit.edu>
 * March 18, 1997.
 *
 * A calendar queue basically hashes events into buckets based on
 * arrival time.
 *
 * See R.Brown. "Calendar queues: A fast O(1) priority queue implementation 
 *  for the simulation event set problem." 
 *  Comm. of ACM, 31(10):1220-1227, Oct 1988
 */

#define CALENDAR_HASH(t) ((int)fmod((t)/width_, nbuckets_))

static class CalendarSchedulerClass : public TclClass {
public:
	CalendarSchedulerClass() : TclClass("Scheduler/Calendar") {}
	TclObject* create(int /* argc */, const char*const* /* argv */) {
		return (new CalendarScheduler);
	}
} class_calendar_sched;

CalendarScheduler::CalendarScheduler() : cal_clock_(clock_) {
	bind("adjust_new_width_interval_", &adjust_new_width_interval_);
	bind("min_bin_width_", &min_bin_width_);
	if (adjust_new_width_interval_) {
		avg_gap_ = -2;
		last_time_ = -2;
		gap_num_ = 0;
		head_search_ = 0;
		insert_search_ = 0; 
		round_num_ = 0; 
		time_to_newwidth = adjust_new_width_interval_;
	}
	reinit(4, 1.0, cal_clock_);
}

CalendarScheduler::~CalendarScheduler() {
	// XXX free events?
	delete [] buckets_;
	qsize_ = 0;
	stat_qsize_ = 0;
}

void 
CalendarScheduler::insert(Event* e)
{
	int i;
	double newtime = e->time_;
	if (cal_clock_ > newtime) {
		// may happen in RT scheduler
		cal_clock_ = newtime;
		i = lastbucket_ = CALENDAR_HASH(cal_clock_);
	} else
		i = CALENDAR_HASH(newtime);

	Bucket* current=(&buckets_[i]);
	Event *head = current->list_;
	Event *after=0;

	if (!head) {
		current->list_ = e;
		e->next_ = e->prev_ = e;
		++stat_qsize_; 
		++(current->count_);
	} else {
		insert_search_++;
		if (newtime < head->time_) {
			//  e-> head -> ...
			e->next_ = head;
			e->prev_ = head->prev_;
			e->prev_->next_ = e;
			head->prev_ = e;
			current->list_ = e;
                        ++stat_qsize_;
                        ++(current->count_);
		} else {
                        for (after = head->prev_; newtime < after->time_; after = after->prev_) { insert_search_++; };
			//...-> after -> e -> ...
			e->next_ = after->next_;
			e->prev_ = after;
			e->next_->prev_ = e;
			after->next_ = e;
			if (after->time_ < newtime) {
				//unique timing
				++stat_qsize_; 
				++(current->count_);
			}
		}
	}
	++qsize_;
	//assert(e == buckets_[i].list_ ||  e->prev_->time_ <= e->time_);
	//assert(e == buckets_[i].list_->prev_ || e->next_->time_ >= e->time_);

  	if (stat_qsize_ > top_threshold_) {
  		resize(nbuckets_ << 1, cal_clock_);
		return;
	}
}

void 
CalendarScheduler::insert2(Event* e)
{
	// Same as insert, but for inserts e *before* any same-time-events, if
	//   there should be any.  Since it is used only by CalendarScheduler::newwidth(),
	//   some important checks present in insert() need not be performed.

	int i = CALENDAR_HASH(e->time_);
	Event *head = buckets_[i].list_;
	Event *before=0;
	if (!head) {
		buckets_[i].list_ = e;
		e->next_ = e->prev_ = e;
		++stat_qsize_; 
		++buckets_[i].count_;
	} else {
		bool newhead;
		if (e->time_ > head->prev_->time_) { //strict LIFO, so > and not >=
			// insert at the tail
			before = head;
			newhead = false;
		} else {
			// insert event in time sorted order, LIFO for sim-time events
			for (before = head; e->time_ > before->time_; before = before->next_)
				;
			newhead = (before == head);
		}

		e->next_ = before;
		e->prev_ = before->prev_;
		before->prev_ = e;
		e->prev_->next_ = e;
		if (newhead) {
			buckets_[i].list_ = e;
			//assert(e->time_ <= e->next_->time_);
		}

		if (e != e->next_ && e->next_->time_ != e->time_) {
			// unique timing
			++stat_qsize_; 
			++buckets_[i].count_;
		}
	}
	//assert(e == buckets_[i].list_ ||  e->prev_->time_ <= e->time_);
	//assert(e == buckets_[i].list_->prev_ || e->next_->time_ >= e->time_);

	++qsize_;
	// no need to check resizing
}

const Event*
CalendarScheduler::head()
{
	if (qsize_ == 0)
		return NULL;

	int l = -1, i = lastbucket_;
	int lastbucket_dec = (lastbucket_) ? lastbucket_ - 1 : nbuckets_ - 1;
	double diff;
	Event *e, *min_e = NULL;
#define CAL_DEQUEUE(x) 						\
do { 								\
	head_search_++;						\
	if ((e = buckets_[i].list_) != NULL) {			\
		diff = e->time_ - cal_clock_;			\
		if (diff < diff##x##_)	{			\
			l = i;					\
			goto found_l;				\
		}						\
		if (min_e == NULL || min_e->time_ > e->time_) {	\
			min_e = e;				\
			l = i;					\
		}						\
	}							\
	if (++i == nbuckets_) i = 0;				\
} while (0)
		 
	// CAL_DEQUEUE applied successively will find the event to
	// dequeue (within one year) and keep track of the
	// minimum-priority event seen so far; the argument controls
	// the criteria used to decide whether the event should be
	// considered `within one year'.  Importantly, the number of
	// buckets should not be less than 4.
	CAL_DEQUEUE(0); 
	CAL_DEQUEUE(1); 
	for (; i != lastbucket_dec; ) {
		CAL_DEQUEUE(2);
	}
	// one last bucket is left unchecked - take the minimum
	// [could have done CAL_DEQUEUE(3) with diff3_ = bwidth*(nbuck*3/2-1)]
	e = buckets_[i].list_;
	if (min_e != NULL && 
	    (e == NULL || min_e->time_ < e->time_))
		e = min_e;
	else {
		//assert(e);
		l = i;
	}
 found_l:
	//assert(buckets_[l].count_ >= 0);
	//assert(buckets_[l].list_ == e);

	/* l is the index of the bucket to dequeue, e is the event */
	/* 
	 * still want to advance lastbucket_ and cal_clock_ to save
	 * time when deque() follows. 
	 */
	assert (l != -1);
	lastbucket_ = l;
 	cal_clock_  = e->time_;
	
	return e;
}

Event* 
CalendarScheduler::deque()
{
	Event *e = const_cast<Event *>(head());

	if (!e)
		return 0;

	if (adjust_new_width_interval_) {
		if (last_time_< 0) last_time_ = e->time_;
		else 
		{
			gap_num_ ++;
			if (gap_num_ >= qsize_ ) {
	                	double tt_gap_ = e->time_ - last_time_;
				avg_gap_ = tt_gap_ / gap_num_;
        	                gap_num_ = 0;
                	        last_time_ = e->time_;
				round_num_ ++;
				if ((round_num_ > 20) &&
					   (( head_search_> (insert_search_<<1))
					  ||( insert_search_> (head_search_<<1)) )) 
				{
					resize(nbuckets_, cal_clock_);
					round_num_ = 0;
				} else {
                        	        if (round_num_ > 100) {
                                	        round_num_ = 0;
                                        	head_search_ = 0;
	                                        insert_search_ = 0;
        	                        }
				}
			}
		}
	};

	int l = lastbucket_;

	// update stats and unlink
	if (e->next_ == e || e->next_->time_ != e->time_) {
		--stat_qsize_;
		//assert(stat_qsize_ >= 0);
		--buckets_[l].count_;
		//assert(buckets_[l].count_ >= 0);

	}
	--qsize_;

	if (e->next_ == e)
		buckets_[l].list_ = 0;
	else {
		e->next_->prev_ = e->prev_;
		e->prev_->next_ = e->next_;
		buckets_[l].list_ = e->next_;
	}

	e->next_ = e->prev_ = NULL;


	//if (buckets_[l].count_ == 0)
	//	assert(buckets_[l].list_ == 0);

 	if (stat_qsize_ < bot_threshold_) {
		resize(nbuckets_ >> 1, cal_clock_);
	}

	return e;
}

void 
CalendarScheduler::reinit(int nbuck, double bwidth, double start)
{
	buckets_ = new Bucket[nbuck];

	memset(buckets_, 0, sizeof(Bucket)*nbuck); //faster than ctor

	width_ = bwidth;
	nbuckets_ = nbuck;
	qsize_ = 0;
	stat_qsize_ = 0;

	lastbucket_ =  CALENDAR_HASH(start);

	diff0_ = bwidth*nbuck/2;
	diff1_ = diff0_ + bwidth;
	diff2_ = bwidth*nbuck;
	//diff3_ = bwidth*(nbuck*3/2-1);

	bot_threshold_ = (nbuck >> 1) - 2;
	top_threshold_ = (nbuck << 1);
}

void 
CalendarScheduler::resize(int newsize, double start)
{
	double bwidth;
	if (newsize == nbuckets_) {
		/* we resize for bwidth*/
		if (head_search_) bwidth = head_search_; else bwidth = 1;
		if (insert_search_) bwidth = bwidth / insert_search_;
		bwidth = sqrt (bwidth) * width_;
 		if (bwidth < min_bin_width_) {
 			if (time_to_newwidth>0) {
 				time_to_newwidth --;
 			        head_search_ = 0;
 			        insert_search_ = 0;
 				round_num_ = 0;
 				return; //failed to adjust bwidth
 			} else {
				// We have many (adjust_new_width_interval_) times failure in adjusting bwidth.
				// should do a reshuffle with newwidth 
 				bwidth = newwidth(newsize);
 			}
 		};
		//snoopy queue calculation
	} else {
		/* we resize for size */
		bwidth = newwidth(newsize);
		if (newsize < 4)
			newsize = 4;
	}

	Bucket *oldb = buckets_;
	int oldn = nbuckets_;

	reinit(newsize, bwidth, start);

	// copy events to new buckets
	int i;

	for (i = 0; i < oldn; ++i) {
		// we can do inserts faster, if we use insert2, but to
		// preserve FIFO, we have to start from the end of
		// each bucket and use insert2
		if  (oldb[i].list_) {
			Event *tail = oldb[i].list_->prev_;
			Event *e = tail; 
			do {
				Event* ep = e->prev_;
				e->next_ = e->prev_ = 0;
				insert2(e);
				e = ep;
			} while (e != tail);
		}
	}
        head_search_ = 0;
        insert_search_ = 0;
	round_num_ = 0;
        delete [] oldb;
}

// take samples from the most populated bucket.
double
CalendarScheduler::newwidth(int newsize)
{
	if (adjust_new_width_interval_) {
		time_to_newwidth = adjust_new_width_interval_;
		if (avg_gap_ > 0) return avg_gap_*4.0;
	}
	int i;
	int max_bucket = 0; // index of the fullest bucket
	for (i = 1; i < nbuckets_; ++i) {
		if (buckets_[i].count_ > buckets_[max_bucket].count_)
			max_bucket = i;
	}
	int nsamples = buckets_[max_bucket].count_;

	if (nsamples <= 4) return width_;
	
	double nw = buckets_[max_bucket].list_->prev_->time_ 
		- buckets_[max_bucket].list_->time_;
	assert(nw > 0);
	
	nw /= ((newsize < nsamples) ? newsize : nsamples); // min (newsize, nsamples)
	nw *= 4.0;
	
	return nw;
}

/*
 * Cancel an event.  It is an error to call this routine
 * when the event is not actually in the queue.  The caller
 * must free the event if necessary; this routine only removes
 * it from the scheduler queue.
 *
 */
void 
CalendarScheduler::cancel(Event* e)
{
	if (e->uid_ <= 0)	// event not in queue
		return;

	int i = CALENDAR_HASH(e->time_);

	assert(e->prev_->next_ == e);
	assert(e->next_->prev_ == e);

	if (e->next_ == e || 
	    (e->next_->time_ != e->time_ &&
	    (e->prev_->time_ != e->time_))) { 
		--stat_qsize_;
		assert(stat_qsize_ >= 0);
		--buckets_[i].count_;
		assert(buckets_[i].count_ >= 0);
	}

	if (e->next_ == e) {
		assert(buckets_[i].list_ == e);
		buckets_[i].list_ = 0;
	} else {
		e->next_->prev_ = e->prev_;
		e->prev_->next_ = e->next_;
		if (buckets_[i].list_ == e)
			buckets_[i].list_ = e->next_;
	}

	if (buckets_[i].count_ == 0)
		assert(buckets_[i].list_ == 0);

	e->uid_ = -e->uid_;
	e->next_ = e->prev_ = NULL;

	--qsize_;

	return;
}

Event * 
CalendarScheduler::lookup(scheduler_uid_t uid)
{
	for (int i = 0; i < nbuckets_; i++) {
		Event* head =  buckets_[i].list_;
		Event* p = head;
		if (p) {
			do {
				if (p->uid_== uid) 
					return p;
				p = p->next_;
			} while (p != head);
		}
	}
	return NULL;
}

#ifndef WIN32
#include <sys/time.h>
#endif

class RealTimeScheduler : public CalendarScheduler {
public:
	RealTimeScheduler();
	virtual void run();
	double start() const { return start_; }
	virtual void reset();
protected:
	void sync() { clock_ = tod(); }
	double tod();
	double slop_;	// allowed drift between real-time and virt time
	double start_;	// starting time
};

static class RealTimeSchedulerClass : public TclClass {
public:
	RealTimeSchedulerClass() : TclClass("Scheduler/RealTime") {}
	TclObject* create(int /* argc */, const char*const* /* argv */) {
		return (new RealTimeScheduler);
	}
} class_realtime_sched;

RealTimeScheduler::RealTimeScheduler() : start_(0.0)
{
	bind("maxslop_", &slop_);
}

double
RealTimeScheduler::tod()
{
	timeval tv;
	gettimeofday(&tv, 0);
	double s = tv.tv_sec;
	s += (1e-6 * tv.tv_usec);
	return (s - start_);
}

void
RealTimeScheduler::reset()
{
	clock_ = SCHED_START;
	start_ = tod();
}

void 
RealTimeScheduler::run()
{ 
	static const double RTSCHEDULER_MINWAIT = 1.0e-3; // don't wait for less
	const Event *p;

	/*XXX*/
	instance_ = this;

	while (!halted_) {
		clock_ = tod();
		p = head();
		if (p && (clock_ - p->time_) > slop_) {
			fprintf(stderr,
				"RealTimeScheduler: warning: slop "
				"%f exceeded limit %f [clock_:%f, p->time_:%f]\n",
				clock_ - p->time_, slop_, clock_, p->time_);
		}
		// handle "old events"
		while (p && p->time_ <= clock_) {

			dispatch(deque(), clock_);
			if (halted_)
				return;
			p = head();
			clock_ = tod();
		}
		
		if (!p) {
			// blocking wait for TCL events
			Tcl_WaitForEvent(0); // no sim events, wait forever
			clock_ = tod();
		} else {
			double diff = p->time_ - clock_;
			// blocking wait only if there is enough time
			if (diff > RTSCHEDULER_MINWAIT) {
				Tcl_Time to;
				to.sec = long(diff);
				to.usec = long(1e6*(diff - to.sec));
				Tcl_WaitForEvent(&to);    // block
				clock_ = tod();
			}
		}
		Tcl_DoOneEvent(TCL_DONT_WAIT);
	}
	// we reach here only if halted
}
