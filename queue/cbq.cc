/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 The Regents of the University of California.
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
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
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
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/queue/cbq.cc,v 1.28 2005/07/27 01:13:44 tomh Exp $ (LBL)";
#endif


//
// new version of cbq using the ns-2 fine-grain
// objects.  Also, re-orginaize CBQ to look more like how
// its description reads in ToN v3n4 and simplify extraneous stuff -KF
//
// there is a 1-1 relationship between classes and queues, except
// that internal nodes in the LS tree don't have queues
//
// Definitions:
//	overlimit:
//		recently used more than allocated link-sharing bandwidth
//			(in bytes/sec averaged over specified interval)
//	
//	level:
//		all leaves are at level 1
//		interior nodes are at a level 1 greater than
//		    the highest level number of any of its children
//
//	unsatisfied:
//		(leaf): underlimit and has demand
//		(interior): underlimit and has some descendant w/demand
//			[either a leaf or interior descendant]
//
//	formal link-sharing:
//		class may continue unregulated if either:
//		1> class is underlimit or at-limit
//		2> class has a under(at)-limit ancestor at level i
//			and no unsatisfied classes at any levels < i
//
//	ancestors-only link-sharing:
//		class may continue unregulated if either:
//		1> class is under/at limit
//		2> class has an UNDER-limit ancestor [at-limit not ok]
//
//	top-level link-sharing:
//		class may continue unregulated if either:
//		1> class is under/at limit
//		2> class has an UNDER-limit ancestor with level
//			<= the value of "top-level"

#include "queue-monitor.h"
#include "queue.h"
#include "delay.h"

#define	MAXPRIO		10	/* # priorities in scheduler */
#define	MAXLEVEL	32	/* max depth of link-share tree(s) */
#define	LEAF_LEVEL	1	/* level# for leaves */
#define	POWEROFTWO	16

class CBQueue;

class CBQClass : public Connector {
public:
	friend class CBQueue;
	friend class WRR_CBQueue;

	CBQClass();
	int	command(int argc, const char*const* argv);
	void	recv(Packet*, Handler*);	// from upstream classifier

protected:

	void	newallot(double);		// change an allotment
	void	update(Packet*, double);	// update when sending pkt
	void	delayed(double);		// when overlim/can't borrow

	int	satisfied(double);		// satisfied?
	int 	demand();			// do I have demand?
	int 	leaf();				// am I a leaf class?

	int	ancestor(CBQClass*p);		// are we an ancestor of p?
	int	desc_with_demand();		// any desc has demand?

	CBQueue*	cbq_;			// the CBQueue I'm part of

	CBQClass*	peer_;			// peer at same sched prio level
	CBQClass*	level_peer_;		// peer at same LS level
	CBQClass*	lender_;		// parent I can borrow from

	Queue*		q_;			// underlying queue
	QueueMonitor*	qmon_;			// monitor for the queue

	double		allotment_;		// frac of link bw
	double		maxidle_;		// bound on idle time
	double		maxrate_;		// bound on bytes/sec rate
	double		extradelay_;		// adjustment to delay
	double		last_time_;		// last xmit time this class
	double		undertime_;		// will become unsat/eligible
	double		avgidle_;		// EWMA of idle
	int		pri_;			// priority for scheduler
	int		level_;			// depth in link-sharing tree
	int		delayed_;		// boolean-was I delayed
	int		bytes_alloc_;		// for wrr only
	int		permit_borrowing_;	// ok to borrow?

};

class CBQueue : public Queue {
public:
	CBQueue();
	void		reset();
	void		enque(Packet*) { abort(); }
	void		recv(Packet*, Handler*);
	LinkDelay*	link() const { return (link_); }
	CBQClass*	level(int n) const { return levels_[n]; }
	Packet*		deque();
	virtual int	command(int argc, const char*const* argv);
	virtual void	addallot(int, double) { }
	Packet*	pending_pkt() const { return (pending_pkt_); }
	void		sched();
	int		toplevel() {	// are we using toplevel?
// 		return (eligible_ == &eligible_toplevel);
		return (eligible_ == TOPLEVEL);
	}
	void		toplevel_arrival(CBQClass*, double);
protected:
	Event		intr_;
	int		algorithm(const char *);
	virtual int	insert_class(CBQClass*);
	int		send_permitted(CBQClass*, double);
	CBQClass*	find_lender(CBQClass*, double);
	void		toplevel_departure(CBQClass*, double);

	CBQClass*	last_lender_;
	Packet*		pending_pkt_;		// queued packet
	LinkDelay*	link_;			// managed link

	CBQClass*	active_[MAXPRIO];	// classes at prio of index
	CBQClass*	levels_[MAXLEVEL+1];	// LL of classes per level
	int		maxprio_;		// highest prio# seen
	int		maxpkt_;		// largest pkt (used by WRR)
	int		maxlevel_;		// highest level# seen
	int		toplevel_;		// for top-level LS

// 	typedef int (CBQueue::*eligible_type_)(CBQClass*, double);
// 	eligible_type_ eligible_;		// eligible function
	enum eligible_type_ { NONE, FORMAL, ANCESTORS, TOPLEVEL };
	eligible_type_ eligible_;
	int	eligible_formal(CBQClass*, double);
	int	eligible_ancestors(CBQClass*, double) { return (1); }
	int	eligible_toplevel(CBQClass* cl, double) {
		return(cl->level_ <= toplevel_);
	}
};

static class CBQQueueClass : public TclClass {
public:
	CBQQueueClass() : TclClass("Queue/CBQ") { }
	TclObject* create(int, const char*const*) {
		return (new CBQueue);
	}
} class_cbq;

static class CBQClassClass : public TclClass {
public:
	CBQClassClass() : TclClass("CBQClass") { }
	TclObject* create(int, const char*const*) {
		return (new CBQClass);
	}
} class_cbqclass;

CBQueue::CBQueue() : last_lender_(NULL), pending_pkt_(NULL), link_(NULL),
	maxprio_(-1), maxpkt_(-1), maxlevel_(-1), toplevel_(MAXLEVEL),
// 	eligible_((eligible_type_)NULL)
	eligible_(NONE)
{
	bind("maxpkt_", &maxpkt_);
	memset(active_, '\0', sizeof(active_));
	memset(levels_, '\0', sizeof(levels_));
}

/*
 * schedule ourselves, used by CBQClass::recv
 */

void
CBQueue::sched()
{
	Scheduler& s = Scheduler::instance();
	blocked_ = 1;
	s.schedule(&qh_, &intr_, 0);
}

/*
 * invoked by passing a packet from one of our managed queues
 * basically provides a queue of one packet
 */

void
CBQueue::recv(Packet* p, Handler*)
{

	if (pending_pkt_ != NULL)
		abort();

	blocked_ = 1;
	pending_pkt_ = p;
}

void
CBQueue::reset()
{
	// don't do anything
	// in particular, don't let Queue::reset() call
	// our deque() method
}

int
CBQueue::algorithm(const char *arg)
{

	if (*arg == '0' || (strcmp(arg, "ancestor-only") == 0)) {
// 		eligible_ = &eligible_ancestors;
		eligible_ = ANCESTORS;
		return (1);
	} else if (*arg == '1' || (strcmp(arg, "top-level") == 0)) {
// 		eligible_ = &eligible_toplevel;
		eligible_ = TOPLEVEL;
		return (1);
	} else if (*arg == '2' || (strcmp(arg, "formal") == 0)) {
// 		eligible_ = &eligible_formal;
		eligible_ = FORMAL;
		return (1);
	} else if (*arg == '3' || (strcmp(arg, "old-formal") == 0)) {
		fprintf(stderr, "CBQ: old-formal LS not supported\n");
		return (-1);
	}
	return (-1);
}

/*
 *
 * toplevel_arrival:	called only using TL link sharing on arrival
 * toplevel_departure:	called only using TL link sharing on departure
 */

void
CBQueue::toplevel_departure(CBQClass *cl, double now)
{
	if (toplevel_ >= last_lender_->level_) {
		if ((cl->qmon_->pkts() <= 1) ||
		    last_lender_->undertime_ > now) {
			toplevel_ = MAXLEVEL;
		} else {
			toplevel_ = last_lender_->level_;
		}
	}
}

void
CBQueue::toplevel_arrival(CBQClass *cl, double now)
{
	if (toplevel_ > 1) {
		if (cl->undertime_ < now)
			toplevel_ = 1;
		else if (toplevel_ > 2 && cl->permit_borrowing_ && cl->lender_ != NULL) {
			if (cl->lender_->undertime_ < now)
				toplevel_ = 2;
		}
	}
}

/*
 * deque: this gets invoked by way of our downstream
 * (i.e. linkdelay) neighbor doing a 'resume' on us
 * via our handler (by Queue::resume()), or by our upstream
 * neighbor when it gives us a packet when we were
 * idle
 */

Packet *
CBQueue::deque()
{

	Scheduler& s = Scheduler::instance();
	double now = s.clock();

	CBQClass* first = NULL;
	CBQClass* eligible = NULL;
	CBQClass* cl;
	register int prio;
	Packet* rval;

	int none_found = 0;

	/*
	 * prio runs from 0 .. maxprio_
	 *
	 * round-robin through all the classes at priority 'prio'
	 * 	if any class is ok to send, resume it's queue
	 * go on to next lowest priority (higher prio nuber) and repeat
	 * [lowest priority number is the highest priority]
	 */

	for (prio = 0; prio <= maxprio_; prio++) {
		// see if there is any class at this prio
		if ((cl = active_[prio]) == NULL) {
			// nobody at this prio level
			continue;
		}

		// look for underlimit peer with something to send
		do {
			// anything to send?
			if (cl->demand()) {
				if (first == NULL && cl->permit_borrowing_ && cl->lender_ != NULL)
					first = cl;
				if (send_permitted(cl, now)) {
					// ok to send
					eligible = cl;
					goto found;
				} else {
					// not ok right now
					cl->delayed(now);
				}
			}
			cl = cl->peer_;	// move to next at same prio
		} while (cl != active_[prio]);
	}
	// did not find anyone so let first go
	// eligible will be NULL at this point
	if (first != NULL) {
		none_found = 1;
		eligible = first;
	}

found:
	if (eligible != NULL) {
		active_[eligible->pri_] = eligible->peer_;
		// eligible->q_->unblock();
		eligible->q_->resume();	// fills in pending
		if (pending_pkt_ && !none_found) {
			eligible->update(pending_pkt_, now);
			if (toplevel())
				toplevel_departure(eligible, now);
		}
	}
	rval = pending_pkt_;
	pending_pkt_ = NULL;

	return (rval);
}

/*
 * we are permitted to send if either
 *	1> we are not overlimit (ie we are underlimit or at limit)
 *	2> one of the varios algorithm-dependent conditions is met
 *
 * if we are permitted, who did we borrow from? [could be ourselves
 * if we were not overlimit]
 */
int CBQueue::send_permitted(CBQClass* cl, double now)
{
	if (cl->undertime_ < now) {
		cl->delayed_ = 0;
		last_lender_ = cl;
		return (1);
	} else if (cl->permit_borrowing_ &&
		   (((cl = find_lender(cl, now)) != NULL))) {
		last_lender_ = cl;
		return (1);
	}
	return (0);
}

/*
 * find_lender(class, time)
 *
 *	find a lender for the provided class according to the
 *	various algorithms
 *
 */

CBQClass*
CBQueue::find_lender(CBQClass* cl, double now)
{
	if ((!cl->permit_borrowing_) || ((cl = cl->lender_) == NULL))
		return (NULL);		// no ancestor to borrow from

	while (cl != NULL) {
		// skip past overlimit ancestors
		//	if using TL and we're above the TL limit
		//	do early out
		if (cl->undertime_ > now) {
			if (toplevel() && cl->level_ > toplevel_)
				return (NULL);
			cl = cl->lender_;
			continue;
		}

		// found what may be an eligible
		// lender, check using per-algorithm eligibility
		// criteria
		// XXX we explicitly invoke this indirect method with
		// the "this" pointer because MS VC++ can't parse it
		// without it...
// 		if ((this->*eligible_)(cl, now))
// 			return (cl);
		switch (eligible_) {
		case TOPLEVEL:
			if (eligible_toplevel(cl, now))
				return (cl);
			break;
		case ANCESTORS:
			if (eligible_ancestors(cl, now))
				return (cl);
			break;
		case FORMAL:
			if (eligible_formal(cl, now))
				return (cl);
			break;
		default:
			fprintf(stderr, "Wrong eligible_\n");
			abort();
		}
		cl = cl->lender_;
	}
	return (cl);
}

/*
 * rule #2 for formal link-sharing
 *	class must have no unsatisfied classes below it
 */
int
CBQueue::eligible_formal(CBQClass *cl, double now)
{
	int level;
	CBQClass *p;

	// check from leaf level to (cl->level - 1)
	for (level = LEAF_LEVEL; level < cl->level_; level++) {
		p = levels_[level];
		while (p != NULL) {
			if (!p->satisfied(now))
				return (0);
			p = p->level_peer_;
		}
	}
	return (1);
}

/*
 * insert a class into the cbq object
 */

int
CBQueue::insert_class(CBQClass *p)
{
	p->cbq_ = this;

	/*
	 * Add to circularly-linked list "active_"
	 *    of peers for the given priority.
	 */

	if (p->pri_ < 0 || p->pri_ > (MAXPRIO-1)) {
		fprintf(stderr, "CBQ class %s has invalid pri %d\n",
			p->name(), p->pri_);
		return (-1);
	}

	if (p->q_ != NULL) {
		// only leaf nodes (which have associated queues)
		// are scheduled
		if (active_[p->pri_] != NULL) {
			p->peer_ = active_[p->pri_]->peer_;
			active_[p->pri_]->peer_ = p;
		} else {
			p->peer_ = p;
			active_[p->pri_] = p;
		}
		if (p->pri_ > maxprio_)
			maxprio_ = p->pri_;
	}

	/*
	 * Compute maxrate from allotment.
	 * convert to bytes/sec
	 *	and store the highest prio# we've seen
	 */

	if (p->allotment_ < 0.0 || p->allotment_ > 1.0) {
		fprintf(stderr, "CBQ class %s has invalid allot %f\n",
			p->name(), p->allotment_);
		return (-1);
	}

	if (link_ == NULL) {
		fprintf(stderr, "CBQ obj %s has no link!\n", name());
		return (-1);
	}
	if (link_->bandwidth() <= 0.0) {
		fprintf(stderr, "CBQ obj %s has invalid link bw %f on link %s\n",
			name(), link_->bandwidth(), link_->name());
		return (-1);
	}

	p->maxrate_ = p->allotment_ * (link_->bandwidth() / 8.0);
	addallot(p->pri_, p->allotment_);

	/*
	 * Add to per-level list
	 *	and store the highest level# we've seen
	 */

	if (p->level_ <= 0 || p->level_ > MAXLEVEL) {
		fprintf(stderr, "CBQ class %s has invalid level %d\n",
			p->name(), p->level_);
		return (-1);
	}

	p->level_peer_ = levels_[p->level_];
	levels_[p->level_] = p;
	if (p->level_ > maxlevel_)
		maxlevel_ = p->level_;

	/*
	 * Check that parent and borrow linkage are acyclic.
	 */
#ifdef notdef
	check_for_cycles(CBQClass::parent);
	check_for_cycles(CBQClass::borrow);
#endif
	return 0;
}

int CBQueue::command(int argc, const char*const* argv)
{

	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "insert-class") == 0) {
			CBQClass *cl = (CBQClass*)TclObject::lookup(argv[2]);
			if (cl == 0) {
				tcl.resultf("CBQ: no class object %s",
					argv[2]);
				return (TCL_ERROR);
			}
			if (insert_class(cl) < 0) {
				tcl.resultf("CBQ: trouble inserting class %s",
					argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "link") == 0) {
			LinkDelay* del = (LinkDelay*)TclObject::lookup(argv[2]);
			if (del == 0) {
				tcl.resultf("CBQ: no LinkDelay object %s",
					argv[2]);
				return(TCL_ERROR);
			}
			link_ = del;
			return (TCL_OK);
		}
		if (strcmp(argv[1], "algorithm") == 0) {
			if (algorithm(argv[2]) < 0)
				return (TCL_ERROR);
			return (TCL_OK);
		}
	}
	return (Queue::command(argc, argv));
}

class WRR_CBQueue : public CBQueue {
public:
	WRR_CBQueue() {
		memset(M_, '\0', sizeof(M_));
		memset(alloc_, '\0', sizeof(alloc_));
		memset(cnt_, '\0', sizeof(cnt_));
	}
	void	addallot(int prio, double diff) {
		alloc_[prio] += diff;
		setM();
	}
protected:
	Packet *deque();
	int	insert_class(CBQClass*);
	void	setM();
	double	alloc_[MAXPRIO];
	double	M_[MAXPRIO];
	int	cnt_[MAXPRIO];		// # classes at prio of index
	int	command(int argc, const char*const* argv);
};

static class WRR_CBQQueueClass : public TclClass {
public:
	WRR_CBQQueueClass() : TclClass("Queue/CBQ/WRR") { }
	TclObject* create(int, const char*const*) {
		return (new WRR_CBQueue);
	}
} class_wrr_cbq;

int WRR_CBQueue::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (strcmp(argv[1], "insert-class") == 0) {
		CBQClass *cl = (CBQClass*)TclObject::lookup(argv[2]);
		if (cl == 0) {
			tcl.resultf("WRR-CBQ: no class object %s",
				argv[2]);
			return (TCL_ERROR);
		}
		if (insert_class(cl) < 0) {
			tcl.resultf("WRR-CBQ: trouble inserting class %s",
				argv[2]);
			return (TCL_ERROR);
		}
		return (TCL_OK);
	}
	return (CBQueue::command(argc, argv));
}

Packet *
WRR_CBQueue::deque()
{

	double now = Scheduler::instance().clock();

	CBQClass* first = NULL;
	CBQClass* eligible = NULL;
	CBQClass* next_eligible = NULL;
	CBQClass* cl;

	register int prio;
	int deficit, done;
	int none_found = 0;

	Packet* rval;

	/*
	 * prio runs from 0 .. maxprio_
	 *
	 * round-robin through all the classes at priority 'prio'
	 *      if any class is ok to send, resume it's queue
	 * go on to next lowest priority (higher prio nuber) and repeat
	 * [lowest priority number is the highest priority]
	 */

	for (prio = 0; prio <= maxprio_; prio++) {
		// see if there is any class at this prio
		if ((cl = active_[prio]) == NULL) {
			// nobody at this prio level
			continue;
		}
                /* 
                 * The WRR round for this priority level starts at deficit 0.
                 *  The round ends if some class is found that is ready
                 *  to send and has positive "bytes_alloc_".
                 * Status advances to deficit 1 if some class was found  
                 *  that was able to send except for insufficient
                 *  "bytes_alloc_".
                 * If status was deficit 1 at the end of the first round,
                 *  then status advances to deficit 2.
                 * Another round of WRR is then begun at deficit 2, looking
                 *  for a class to send even with insufficient "bytes_alloc_".
                 */
		deficit = done = 0;
		while (!done) {
                        // look for class at this priority level ok to send
			do {
                                // set up "weight" for WRR
				if (deficit < 2 && cl->bytes_alloc_ <= 0)
					cl->bytes_alloc_ +=
						(int)(cl->allotment_ * M_[cl->pri_]);
                                // anything to send?
				if (cl->demand()) {
					if (first == NULL && cl->permit_borrowing_ && cl->lender_ != NULL)
						first = cl;
					if (!send_permitted(cl, now)) {
                                                // not ok to send right now
						cl->delayed(now);
					} else {
                                                // ok to send, if class has
                                                //  enough "weight" for WRR.
						int bytes = cl->bytes_alloc_;
						if (bytes > 0 || deficit > 1) {
							eligible = cl;
							goto found;
						} else
							deficit = 1;
					}
				}
				cl->bytes_alloc_ = 0;
				cl = cl->peer_;
			} while (cl != active_[prio] && cl != 0);
			if (deficit == 1)
				deficit = 2;
			else
				done = 1;
		}
	}
        // did not find anyone so let first go
	if ((eligible == NULL) && first != NULL) {
		none_found = 1;
		eligible = first;
	}

found:
	// do accounting
	if (eligible != NULL) {
		next_eligible = eligible->peer_;
		eligible->q_->resume();
		if (pending_pkt_ != NULL && !none_found) {
			// reduce our alloc
			// by the packet size.  If we're
			// still positive, we get to go again
			int bytes = eligible->bytes_alloc_;
			hdr_cmn* hdr = hdr_cmn::access(pending_pkt_);
			if (bytes > 0) {
				eligible->bytes_alloc_ -= hdr->size();
			}
			bytes = eligible->bytes_alloc_;
			if (bytes > 0) {
				next_eligible = eligible;
			}
			eligible->update(pending_pkt_, now);
			if (toplevel())
				toplevel_departure(eligible, now);
		}
		active_[eligible->pri_] = next_eligible;
	}
	rval = pending_pkt_;
	pending_pkt_ = NULL;

	return (rval);
}

int
WRR_CBQueue::insert_class(CBQClass *p)
{
	if (CBQueue::insert_class(p) < 0)
		return (-1);
	++cnt_[p->pri_];
	setM();
	return (0);
}

void
WRR_CBQueue::setM()
{
	int i;
	for (i = 0; i <= maxprio_; i++) {
		if (alloc_[i] > 0.0)
                        // allocate "cnt_[i] * maxpkt_" bytes to each
                        //  priority level:
                        M_[i] = cnt_[i] * maxpkt_ * 1.0 / alloc_[i];
                        // allocate "alloc_[i] * 2.0 * cnt_[i] * maxpkt_"
                        //  bytes to each priority level:
                        //  M_[i] = 2.0 * cnt_[i] * maxpkt_;
		else
			M_[i] = 0.0;

		if (M_[i] < 0.0) {
			fprintf(stderr, "M_[i]: %f, cnt_[i]: %d, maxpkt_: %d, alloc_[i]: %f\n",
				M_[i], cnt_[i], maxpkt_, alloc_[i]);
			abort();
		}

	}
	return;
}

/******************** CBQClass definitions **********************/

CBQClass::CBQClass() : cbq_(0), peer_(0), level_peer_(0), lender_(0),
	q_(0), qmon_(0), allotment_(0.0), maxidle_(-1.0), maxrate_(0.0),
	extradelay_(0.0), last_time_(0.0), undertime_(0.0), avgidle_(0.0),
	pri_(-1), level_(-1), delayed_(0), bytes_alloc_(0),
	permit_borrowing_(1)
{
	/* maxidle_ is no longer bound; it is now a method interface */
	bind("priority_", &pri_);
	bind("level_", &level_);
	bind("extradelay_", &extradelay_);
	bind_bool("okborrow_", &permit_borrowing_);

	if (pri_ < 0 || pri_ > (MAXPRIO-1))
		abort();

	if (level_ <= 0 || level_ > MAXLEVEL)
		abort();
}

// why can't these two be inline (?)
int
CBQClass::demand()
{
	return (qmon_->pkts() > 0);
}

int
CBQClass::leaf()
{
	return (level_ == LEAF_LEVEL);
}

/*
 * we are upstream from the queue
 * the queue should be unblocked if the downstream
 * cbq is not busy and blocked otherwise
 *
 * we get our packet from the classifier, because of
 * this the handler is NULL.  Besides the queue downstream
 * from us (Queue::recv) ignores the handler anyhow
 * 
 */
void
CBQClass::recv(Packet *pkt, Handler *h)
{
	if (cbq_->toplevel()) {
		Scheduler* s;
		if ((s = &Scheduler::instance()) != NULL)
			cbq_->toplevel_arrival(this, s->clock());
	}
	send(pkt, h);	// queue packet downstream
	if (!cbq_->blocked()) {
		cbq_->sched();
	}
	return;
}

/*
 * update a class' statistics and all parent classes
 * up to the root
 */
void CBQClass::update(Packet* p, double now)
{
	double idle, avgidle;

	hdr_cmn* hdr = hdr_cmn::access(p);
	int pktsize = hdr->size();

	double tx_time = cbq_->link()->txtime(p);
	double fin_time = now + tx_time;

	idle = (fin_time - last_time_) - (pktsize / maxrate_);
	avgidle = avgidle_;
	avgidle += (idle - avgidle) / POWEROFTWO;
	if (maxidle_ < 0) {
		fprintf(stderr,
			"CBQClass: warning: maxidle_ not configured!\n");
	} else if (avgidle > maxidle_)
		avgidle = maxidle_;
	avgidle_ = avgidle;

	if (avgidle <= 0) {
		undertime_ = fin_time + tx_time *
			(1.0 / allotment_ - 1.0);
		undertime_ += (1-POWEROFTWO) * avgidle;
	}
	last_time_ = fin_time;
	// tail-recurse up to root of tree performing updates
	if (lender_)
		lender_->update(p, now);

	return;
}

/*
 * satisfied: is this class satisfied?
 */

int
CBQClass::satisfied(double now)
{
	if (leaf()) {
		/* leaf is unsat if underlimit with backlog */
		if (undertime_ < now && demand())
			return (0);
		else
			return (1);
	}
	if (undertime_ < now && desc_with_demand())
		return (0);

	return (1);
}

/*
 * desc_with_demand: is there a descendant of this class with demand
 *	really, is there a leaf which is a descendant of me with
 *	a backlog
 */

int
CBQClass::desc_with_demand()
{
	CBQClass *p = cbq_->level(LEAF_LEVEL);
	for (; p != NULL; p = p->level_peer_) {
		if (p->demand() && ancestor(p))
			return (1);
	}
	return (0);
}

/*
 * happens when a class is unable to send because it
 * is being regulated
 */

void CBQClass::delayed(double now)
{
	double delay = undertime_ - now + extradelay_;

	if (delay > 0 && !delayed_) {
		undertime_ += extradelay_;
		undertime_ -= (1-POWEROFTWO) * avgidle_;
		delayed_ = 1;
	}
}

/*
 * return 1 if we are an ancestor of p, 0 otherwise
 */
int
CBQClass::ancestor(CBQClass *p)
{
	if (!p->permit_borrowing_ || p->lender_ == NULL)
		return (0);
	else if (p->lender_ == this)
		return (1);
	return (ancestor(p->lender_));
}

/*
 * change an allotment
 */
void
CBQClass::newallot(double bw)
{
        if (allotment_ < 0)
                allotment_ = 0;
        if (bw < 0)
                bw = 0;
	maxrate_ = bw * ( cbq_->link()->bandwidth() / 8.0 );
	double diff = bw - allotment_;
	allotment_ = bw;
	cbq_->addallot(pri_, diff);
	return;
}


/*
 * OTcl Interface
 */
/* 
 * $class1 parent $class2
 * $class1 borrow $class2
 * $class1 qdisc $queue
 * $class1 allot
 * $class1 allot new-bw
 */
int CBQClass::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "allot") == 0) {
			tcl.resultf("%g", allotment_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "cbq") == 0) {
			if (cbq_ != NULL)
				tcl.resultf("%s", cbq_->name());
			else
				tcl.resultf("");
			return(TCL_OK);
		}
		if (strcmp(argv[1], "qdisc") == 0) {
			if (q_ != NULL)
				tcl.resultf("%s", q_->name());
			else
				tcl.resultf("");
			return (TCL_OK);
		}
		if (strcmp(argv[1], "qmon") == 0) {
			if (qmon_ != NULL)
				tcl.resultf("%s", qmon_->name());
			else
				tcl.resultf("");
			return (TCL_OK);
		}
	} else if (argc == 3) {
		// for now these are the same
		if ((strcmp(argv[1], "parent") == 0)) {

			if (strcmp(argv[2], "none") == 0) {
				lender_ = NULL;
				return (TCL_OK);
			}
			lender_ = (CBQClass*)TclObject::lookup(argv[2]);
			if (lender_ != NULL)
				return (TCL_OK);

			return (TCL_ERROR);
		}
		if (strcmp(argv[1], "qdisc") == 0) {
			q_ = (Queue*) TclObject::lookup(argv[2]);
			if (q_ != NULL)
				return (TCL_OK);
			tcl.resultf("couldn't find object %s",
				argv[2]);
			return (TCL_ERROR);
		}
		if (strcmp(argv[1], "qmon") == 0) {
			qmon_ = (QueueMonitor*) TclObject::lookup(argv[2]);
			if (qmon_ != NULL)
				return (TCL_OK);
			return (TCL_ERROR);
		}
		if (strcmp(argv[1], "allot") == 0) {
			double bw = atof(argv[2]);
			if (bw < 0.0)
				return (TCL_ERROR);
			if (allotment_ != 0.0) {
				tcl.resultf(" class %s already has allotment of %f!",
					    name(), allotment_);
				return (TCL_ERROR);
			}
			allotment_ = bw;
			return (TCL_OK);
		}
		if (strcmp(argv[1], "newallot") == 0) {
			double bw = atof(argv[2]);
			if (bw < 0.0)
				return (TCL_ERROR);
			newallot(bw);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "maxidle") == 0) {
			double m = atof(argv[2]);
			if (m < 0.0) {
				tcl.resultf("invalid maxidle value %s (must be non-negative)",
					    argv[2]);
				return (TCL_ERROR);
			}
			maxidle_ = m;
			return (TCL_OK);
		}
	}
	return (Connector::command(argc, argv));
}
