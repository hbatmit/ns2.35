/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * splay-scheduler.cc
 * Copyright (C) 2002 by the University of Southern California
 * $Id: splay-scheduler.cc,v 1.6 2005/08/25 18:58:02 johnh Exp $
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

// @(#) $Header: /cvsroot/nsnam/ns-2/common/splay-scheduler.cc,v 1.6 2005/08/25 18:58:02 johnh Exp $

#ifndef lint
static const char rcsid[] =
"@(#) $Header: /cvsroot/nsnam/ns-2/common/splay-scheduler.cc,v 1.6 2005/08/25 18:58:02 johnh Exp $";
#endif
/**
 *
 * Scheduler based on a splay tree.
 *
 * Daniel D. Sleator and Robert E. Tarjan. Self-adjusting binary
 * search trees. Journal of the ACM, 32(3):652--686, 1985. 118.
 *
 * Basic idea of this scheduler: it organizes the event queue into a
 * binary search tree.  For every insert and deque operation,
 * "splaying" is performed aimed at shortening the search path for
 * "similar" priorities (time_).  This should give O(log(N)) amortized
 * time for basic operations, may be even better for correlated inserts.
 *
 * Implementation notes: Event::next_ and Event::prev_ are used as
 * right and left pointers.  insert() and deque() use the "top-down"
 * splaying algorithm taken almost verbatim from the paper and in some
 * cases optimized for particular operations.  lookup() is extremely
 * inefficient, and should be avoided whenever possible.  cancel()
 * would be better off if we had a pointer to the parent, then there
 * wouldn't be any need to search for it (and use Event::uid_ to
 * resolve conflicts when same-priority events are both on the left
 * and right).  cancel() does not perform any splaying, while it
 * perhaps should.
 *
 * Memory used by this scheduler is O(1) (not counting the events).
 *
 * The original vefsion of this code was by Yuri Pryadkin.
 **/
#include <scheduler.h>
#include <assert.h>


static class SplaySchedulerClass : public TclClass 
{
public:
        SplaySchedulerClass() : TclClass("Scheduler/Splay") {}
        TclObject* create(int /* argc */, const char*const* /* argv */) {
                return (new SplayScheduler);
        }
} class_splay_sched;

#undef LEFT
#undef RIGHT
#define LEFT(e)				((e)->prev_)	
#define RIGHT(e)			((e)->next_)

#define ROTATE_RIGHT(t, x)		\
    do {				\
    	LEFT(t) = RIGHT(x);	 	\
    	RIGHT(x) = (t);			\
    	(t) = (x);			\
    } while(0)

#define ROTATE_LEFT(t, x)		\
    do {				\
    	RIGHT(t)  = LEFT(x);	 	\
    	LEFT(x) = (t);			\
    	(t) = (x);			\
    } while(0)
#define LINK_LEFT(l, t)			\
    do {				\
	RIGHT(l) = (t);			\
	(l) = (t);			\
	(t) = RIGHT(t);			\
    } while (0)
#define LINK_RIGHT(r, t)		\
    do {				\
	LEFT(r) = (t);			\
	(r) = (t);			\
	(t) = LEFT(t);			\
    } while (0)

void
SplayScheduler::insert(Event *n) 
{
	Event *l;	// bottom right in the left tree
	Event *r;	// bottom left in the right tree
	Event *t;	// root of the remaining tree
	Event *x;   	// current node
    
	++qsize_;

	double time = n->time_;
    
	if (root_ == 0) {
		LEFT(n) = RIGHT(n) = 0;
		root_ = n;
		//validate();
		return;
	}
	t = root_;
	root_ = n;	// n is the new root
	l = n;
	r = n;
	for (;;) {
		if (time < t->time_) {
			x = LEFT(t);
			if (x == 0) {
				LEFT(r) = t;
				RIGHT(l) = 0;
				break;
			}
			if (time < x->time_) {
				ROTATE_RIGHT(t, x);
			}
			LINK_RIGHT(r, t);
			if (t == 0) {
				RIGHT(l) = 0;
				break;
			}
		} else {
			x = RIGHT(t);
			if (x == 0) {
				RIGHT(l) = t; 
				LEFT(r) = 0;
				break;	
			}
			if (time >= x->time_) {
				ROTATE_LEFT(t, x);
			}
			LINK_LEFT(l, t);
			if (t == 0) {
				LEFT(r) = 0;
				break;
			}
			
		}
	} /* for (;;) */

	// assemble:
	//   swap left and right children
	x = LEFT(n);
	LEFT(n) = RIGHT(n);
	RIGHT(n) = x;
	//validate();
}

const Event *
SplayScheduler::head()
{
	Event *t;
	Event *l;
#if 1
	if (root_ == 0)
		return 0;
	for (t = root_; (l = LEFT(t)); t = l)
		;

	return t;
#else
	Event *ll;
	Event *lll;

	if (root_ == 0)
		return 0;

	t = root_;
	l = LEFT(t);

	if (l == 0) {
		return t;
	}
	for (;;) { 
		ll = LEFT(l);
		if (ll == 0) {
			return l;
		}

		lll = LEFT(ll);
		if (lll == 0) {
			return ll;
		}

		// zig-zig: rotate l with ll
		LEFT(t) = ll;
		LEFT(l) = RIGHT(ll);
		RIGHT(ll) = l;

		t = ll;
		l = lll;
	}
#endif /* 1/0 */
}

Event *
SplayScheduler::deque()
{
	Event *t;
	Event *l;
	Event *ll;
	Event *lll;

	if (root_ == 0)
		return 0;

	--qsize_;

	t = root_;
	l = LEFT(t);

	if (l == 0) {			// root is the element to dequeue
		root_ = RIGHT(t);	// right branch becomes the root
		//validate();
		return t;
	}
	for (;;) { 
		ll = LEFT(l);
		if (ll == 0) {
			LEFT(t) = RIGHT(l);
			//validate();
			return l;
		}

		lll = LEFT(ll);
		if (lll == 0) {
			LEFT(l) = RIGHT(ll);
			//validate();
			return ll;
		}

		// zig-zig: rotate l with ll
		LEFT(t) = ll;
		LEFT(l) = RIGHT(ll);
		RIGHT(ll) = l;

		t = ll;
		l = lll;
	}
} 

void 
SplayScheduler::cancel(Event *e)
{

	if (e->uid_ <= 0) 
		return; // event not in queue

	Event **t;
	//validate();
	if (e == root_) {
		t = &root_;
	} else {
		// searching among same-time events is a real bugger,
		// all because we don't have a parent pointer; use
		// uid_ to resolve conflicts.
		for (t = &root_; *t;) {
			t = ((e->time_ > (*t)->time_) || 
			     ((e->time_ == (*t)->time_) && e->uid_ > (*t)->uid_))
				? &RIGHT(*t) : &LEFT(*t);
			if (*t == e)
				break;
		}
		if (*t == 0) {
			fprintf(stderr, "did not find it\n");
			abort(); // not found
		}
	}
	// t is the pointer to e in the parent or to root_ if e is root_
	e->uid_ = -e->uid_;
	--qsize_;

	if (RIGHT(e) == 0) {
		*t = LEFT(e);
		LEFT(e) = 0;
		//validate();
		return;
	} 
	if (LEFT(e) == 0) {
		*t = RIGHT(e);
		RIGHT(e) = 0;
		//validate();
		return;
	}

	// find successor
	Event *p = RIGHT(e), *q = LEFT(p);

	if (q == 0) {
		// p is the sucessor
		*t = p;
		LEFT(p) = LEFT(e);
		//validate();
		return;
	}
	for (; LEFT(q); p = q, q = LEFT(q)) 
		;
	// q is the successor
	// p is q's parent
	*t = q;
	LEFT(p) = RIGHT(q);
	LEFT(q) = LEFT(e);
	RIGHT(q) = RIGHT(e);
	RIGHT(e) = LEFT(e) = 0;
	//validate();
}


Event *
SplayScheduler::lookup(scheduler_uid_t uid) 
{
	lookup_uid_ = uid;
	return uid_lookup(root_);
}

Event *
SplayScheduler::uid_lookup(Event *root)
{
	if (root == 0)
		return 0;
	if (root->uid_ == lookup_uid_)
		return root;

	Event *res = uid_lookup(LEFT(root));
 
	if (res) 
		return res;

	return uid_lookup(RIGHT(root));
}

int
SplayScheduler::validate(Event *root) 
{
	int size = 0;
	if (root) {
		++size;
		assert(LEFT(root) == 0 || root->time_ >= LEFT(root)->time_);
		assert(RIGHT(root) == 0 || root->time_ <= RIGHT(root)->time_);
		size += validate(LEFT(root));
		size += validate(RIGHT(root));
		return size;
	}
	return 0;
}
