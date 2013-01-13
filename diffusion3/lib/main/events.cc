// 
// events.cc     : Provides the EventQueue
// authors       : Lewis Girod John Heidemann and Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: events.cc,v 1.10 2008/03/27 05:24:46 tom_henderson Exp $
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
//
// Linking this file statically or dynamically with other modules is making
// a combined work based on this file.  Thus, the terms and conditions of
// the GNU General Public License cover the whole combination.
//
// In addition, as a special exception, the copyright holders of this file
// give you permission to combine this file with free software programs or
// libraries that are released under the GNU LGPL and with code included in
// the standard release of ns-2 under the Apache 2.0 license or under
// otherwise-compatible licenses with advertising requirements (or modified
// versions of such code, with unchanged license).  You may copy and
// distribute such a system following the terms of the GNU GPL for this
// file and the licenses of the other code concerned, provided that you
// include the source code of that other code when and as the GNU GPL
// requires distribution of source code.
//
// Note that people who make modified versions of this file are not
// obligated to grant this special exception for their modified versions;
// it is their choice whether to do so.  The GNU General Public License
// gives permission to release a modified version without this exception;
// this exception also makes it possible to release a modified version
// which carries forward this exception.

#include <stdio.h>
#include <stdlib.h>

#include "events.hh"

//
//  Timeval utility routines
//

int TimevalCmp(struct timeval *x, struct timeval *y)
{
  if (x->tv_sec > y->tv_sec) return 1;
  if (x->tv_sec < y->tv_sec) return -1;
  if (x->tv_usec > y->tv_usec) return 1;
  if (x->tv_usec < y->tv_usec) return -1;
  return 0;
}

void TimevalSub(struct timeval *x, struct timeval *y, struct timeval *tv)
{
  if (TimevalCmp(x, y) < 0) 
    tv->tv_sec = tv->tv_usec = 0;
  else{
    *tv = *x;
    // borrow..
    if (tv->tv_usec < y->tv_usec){
      tv->tv_usec += 1000000;
      tv->tv_sec--;
    }
    // sub
    tv->tv_usec -= y->tv_usec;
    tv->tv_sec -= y->tv_sec;
  }
}

void TimevalAddusecs(struct timeval *tv, int usecs)
{
  tv->tv_usec += usecs;
  if (tv->tv_usec > 1000000){
    tv->tv_sec += tv->tv_usec / 1000000;
    tv->tv_usec = tv->tv_usec % 1000000;
  }
}

//
//  EventQueue Methods
//

int EventQueue::eventCmp(QueueEvent *x, QueueEvent *y)
{
  return TimevalCmp(&(x->tv_), &(y->tv_));
}

void EventQueue::eqAdd(QueueEvent *e)
{
  QueueEvent *ptr = head_;
  QueueEvent *last = ptr;

  while (ptr && (eventCmp(e, ptr) > 0)){
    last = ptr; 
    ptr = ptr->next_;
  }
  if (last == ptr){
    e->next_ = head_;
    head_ = e;
  }
  else{
    e->next_ = ptr;
    last->next_ = e;
  }
}

QueueEvent * EventQueue::eqPop()
{
  QueueEvent *e = head_;

  if (e){
    head_ = head_->next_;
    e->next_ = NULL;
  }
  return e;
}

QueueEvent * EventQueue::eqFindEvent(handle hdl)
{
  return eqFindNextEvent(hdl, head_);
}

QueueEvent * EventQueue::eqFindNextEvent(handle hdl, QueueEvent *e)
{
  while (e){
    if (e->handle_ == hdl)
      return e;
    e = e->next_;
  }

  return e;
}

void EventQueue::eqAddAfter(handle hdl, void *payload, int delay_msec)
{
  QueueEvent *e = new QueueEvent;

  e->handle_ = hdl;
  e->payload_ = payload;
  setDelay(e, delay_msec);
  eqAdd(e);
}

void EventQueue::setDelay(QueueEvent *e, int delay_msec)
{
  GetTime(&(e->tv_));
  TimevalAddusecs(&(e->tv_), delay_msec * 1000);
}

void EventQueue::eqNextTimer(struct timeval *tv)
{
  struct timeval now;

  if (head_){
    GetTime(&now);
    TimevalSub(&(head_->tv_), &now, tv);
  }
  else{
    tv->tv_sec = MAXVALUE;
    tv->tv_usec = 0;
  }
}

int EventQueue::eqTopInPast()
{
  struct timeval now;

  if (head_){
    GetTime(&now);
    return (TimevalCmp(&(head_->tv_), &now) <= 0);
  }
  return 0;
}

void EventQueue::eqPrint()
{
  QueueEvent *e = head_;

  for (; (e); e = e->next_){
    fprintf(stderr, "time = %ld:%06ld, handle = %ld\n",
	    e->tv_.tv_sec, (long int) e->tv_.tv_usec, e->handle_);
  }
}

bool EventQueue::eqRemoveEvent(QueueEvent *e)
{
  QueueEvent *ce, *pe;

  if (e){
    if (head_ == e){
      head_ = e->next_;
      return true;
    }

    pe = head_;
    ce = head_->next_;

    while (ce){
      if (ce == e){
	pe->next_ = e->next_;
	return true;
      }
      else{
	pe = ce;
	ce = ce->next_;
      }
    }
  }

  return false;
}

bool EventQueue::eqRemove(handle hdl)
{
  QueueEvent *e = NULL;

  // Look in the event queue for an event with handle hdl
  e = eqFindEvent(hdl);

  // If found, delete event
  if (e){
    return (eqRemoveEvent(e));
  }

  return false;
}

// Computes a randomized delay, measured in milliseconds. Note that
// most OS timers have granularities on order of 5-15 ms.
// timer[2] = { base_timer, variance }
int EventQueue::randDelay(int timer[2])
{
  return (int) (timer[0] + ((((float) GetRand()) /
			     ((float) RAND_MAX)) - 0.5) * timer[1]);
}
