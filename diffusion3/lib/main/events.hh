//
// events.hh     : Implements a queue of timer-based events
// Authors       : Lewis Girod, John Heidemann and Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: events.hh,v 1.8 2005/09/13 04:53:49 tomh Exp $
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
//

// This file defines lower level events (which contain a type, a
// payload and an expiration time) and an event queue, kept in sorted
// order by expiration time. This is used by both the Diffusion
// Routing Library API and the Diffusion Core module for keeping track
// of events. Application (or Filter) developers should use the Timer
// API instead.

#ifndef _EVENT_H_
#define _EVENT_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "tools.hh"

typedef long handle;
#define MAXVALUE 0x7ffffff // No events in the queue

class QueueEvent {
public:
  struct timeval tv_;
  handle handle_;
  void *payload_;

  QueueEvent *next_;
};

class EventQueue {

//
//  Methods
//
//  eqAdd         inserts an event into the queue
//  eqAddAfter    creates a new event and inserts it
//  eqPop         extracts the first event and returns it
//  eqNextTimer   returns the time of expiration for the next event
//  eqTopInPast   returns true if the head of the timer queue is in the past
//  eqRemoveEvent removes an event from the queue
//  eqRemove      removes the event with the given handle from the queue
//

public:
  EventQueue(){
    // Empty
    head_ = NULL;
  };
  virtual ~EventQueue(){
    // Empty
  };

  virtual void eqAddAfter(handle hdl, void *payload, int delay_msec);
  QueueEvent * eqPop();
  QueueEvent * eqFindEvent(handle hdl);
  void eqNextTimer(struct timeval *tv);
  virtual bool eqRemove(handle hdl);

private:

  int eqTopInPast();
  void eqPrint();
  bool eqRemoveEvent(QueueEvent *e);
  QueueEvent * eqFindNextEvent(handle hdl, QueueEvent *e);
  void eqAdd(QueueEvent *e);

//
//  QueueEvent methods
//
//  setDelay   sets the expiration time to a delay after present time
//  randDelay  computes a delay given a vector {base delay, variance}
//             delay times are chosen uniformly within the variance.
//  eventCmp   compares the expiration times from two events and returns
//             -1, 0 1 for <, == and >
//

  void setDelay(QueueEvent *e, int delay_msec);
  int randDelay(int timer[2]);
  int eventCmp(QueueEvent *x, QueueEvent *y);

  QueueEvent *head_;

};

// Extra utility functions for managing struct timeval

// Compares two timeval structures, returning -1, 0 or 1 for <, == or >
int TimevalCmp(struct timeval *x, struct timeval *y);

// tv will be x - y (or 0,0 if x < y)
void TimevalSub(struct timeval *x, struct timeval *y, struct timeval *tv);

// Add usecs to tv
void TimevalAddusecs(struct timeval *tv, int usecs);

#ifdef NS_DIFFUSION
#ifdef RAND_MAX
#undef RAND_MAX
#endif // RAND_MAX
#define RAND_MAX 2147483647
#else
#ifndef RAND_MAX
#define RAND_MAX 2147483647
#endif // !RAND_MAX
#endif // NS_DIFFUSION

#endif // !_EVENT_H_
