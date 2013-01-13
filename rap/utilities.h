/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * utilities.h
 * Copyright (C) 1997 by the University of Southern California
 * $Id: utilities.h,v 1.7 2005/08/25 18:58:11 johnh Exp $
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

//
// utilities.h 
//      Miscellaneous useful definitions, including debugging routines.
// 
// Author:
//   Mohit Talwar (mohit@catarina.usc.edu)
//
// $Header: /cvsroot/nsnam/ns-2/rap/utilities.h,v 1.7 2005/08/25 18:58:11 johnh Exp $

#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <memory.h>

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "address.h"
#include "ip.h"
#include "random.h"
#include "raplist.h"

// Functions...
extern FILE * DebugEnable(unsigned int nodeid);

// Print debug message if flag is enabled
extern void Debug (int debugFlag, FILE *log, char* format, ...); 



// Data structures
class DoubleListElem {
public:
	DoubleListElem() : prev_(0), next_(0) {}

	virtual ~DoubleListElem () {}

	DoubleListElem* next() const { return next_; }
	DoubleListElem* prev() const { return prev_; }

	virtual void detach() {
		if (prev_ != 0) prev_->next_ = next_;
		if (next_ != 0) next_->prev_ = prev_;
		prev_ = next_ = 0;
	}
	// Add new element s before this one
	virtual void insert(DoubleListElem *s) {
		s->next_ = this;
		s->prev_ = prev_;
		if (prev_ != 0) prev_->next_ = s;
		prev_ = s;
	}
	// Add new element s after this one
	virtual void append(DoubleListElem *s) {
		s->next_ = next_;
		s->prev_ = this;
		if (next_ != 0) next_->prev_ = s;
		next_ = s;
	}

private:
	DoubleListElem *prev_, *next_;
};

class DoubleList {
public:
	DoubleList() : head_(0), tail_(0) {}
	virtual ~DoubleList() {}
	virtual void destroy();
	DoubleListElem* head() { return head_; }
	DoubleListElem* tail() { return tail_; }

	void detach(DoubleListElem *e) {
		if (head_ == e)
			head_ = e->next();
		if (tail_ == e)
			tail_ = e->prev();
		e->detach();
	}
	void insert(DoubleListElem *src, DoubleListElem *dst) {
		dst->insert(src);
		if (dst == head_)
			head_ = src;
	}
	void append(DoubleListElem *src, DoubleListElem *dst) {
		dst->append(src);
		if (dst == tail_)
			tail_ = src;
	}
protected:
	DoubleListElem *head_, *tail_;
};


#endif // UTILITIES_H
