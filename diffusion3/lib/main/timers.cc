//
// timers.cc       : Timer Management Class
// authors         : John Heidemann, Fabio Silva and Alefiya Hussain 
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: timers.cc,v 1.4 2005/09/13 04:53:50 tomh Exp $
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

#include <stdlib.h>
#include <stdio.h>

#include "timers.hh"

// Creates the Timer Management class. Creates the eventqueue The
// eventqueue is used by the Timer class only.  Use the nextTimer and
// executeNextTimer functions to access the event queue

TimerManager::TimerManager()
{
  struct timeval tv;

  // Initialize basic stuff
  next_handle_ = 1;
  GetTime(&tv);
  SetSeed(&tv);

  // Initialize event queue
#ifdef NS_DIFFUSION
  eq_ = new DiffEventQueue(this);
#else
  eq_ = new EventQueue;
#endif // NS_DIFFUSION

#ifdef USE_THREADS
  queue_mtx_ = new pthread_mutex_t;
  pthread_mutex_init(queue_mtx_, NULL);
#endif // USE_THREADS
}

// This function adds a timer to the event queue. The timeout provided
// should be in milliseconds. cb specifies the function that will be
// called.

handle TimerManager::addTimer(int timeout, TimerCallback *cb)
{
  TimerEntry *entry;

#ifdef USE_THREADS
  pthread_mutex_lock(queue_mtx_);
#endif // USE_THREADS
  entry = new TimerEntry(next_handle_, timeout, cb);
  eq_->eqAddAfter(next_handle_, entry, timeout);
  next_handle_++;

#ifdef USE_THREADS
  pthread_mutex_unlock(queue_mtx_);
#endif // USE_THREADS
  return entry->hdl_;
}

// Applications can use this function to remove from the eventqueue a
// previously scheduled timer (the handle should be the one returned
// by the AddTimer function)

bool TimerManager::removeTimer(handle hdl)
{
#ifdef NS_DIFFUSION
  if (eq_->eqRemove(hdl) == false){
    fprintf(stderr, "Error: Can't remove event from queue !\n");
    return false;
  }
  return true;
#else
  QueueEvent *e = NULL;
  TimerEntry *entry;

#ifdef USE_THREADS
  pthread_mutex_lock(queue_mtx_);
#endif // USE_THREADS

  // Find the timer in the queue
  e = eq_->eqFindEvent(hdl);

  // If timer found, remove it from the queue
  if (e){
    entry = (TimerEntry *) e->payload_;
    if (eq_->eqRemove(hdl) == false){
      fprintf(stderr, "Error: Can't remove event from queue !\n");
      exit(-1);
    }

    // Call the application provided delete function

    delete entry;
    delete e;
  }
  else{
#ifdef USE_THREADS
    pthread_mutex_unlock(queue_mtx_);
#endif // USE_THREADS
    return false;
  }

#ifdef USE_THREADS
  pthread_mutex_unlock(queue_mtx_);
#endif // USE_THREADS
  return true;
#endif // NS_DIFFUSION
}

// Returns the expiration value for the first timer on the queue
void TimerManager::nextTimerTime(struct timeval *tv)
{
#ifdef USE_THREADS
  pthread_mutex_lock(queue_mtx_);
#endif // USE_THREADS
  eq_->eqNextTimer(tv);
#ifdef USE_THREADS
  pthread_mutex_unlock(queue_mtx_);
#endif // USE_THREADS
}

// Executes the first timer callback in the queue 
#ifdef NS_DIFFUSION
void TimerManager::diffTimeout(DiffEvent *e)
{
  TimerEntry *entry = (TimerEntry *) e->payload();

  // run it
  int new_timeout = entry->cb_->expire();

  if (new_timeout >= 0){
    if (new_timeout > 0){
      // Change the timer's timeout
      entry->timeout_ = new_timeout;
    }
    eq_->eqAddAfter(entry->hdl_, (TimerEntry *) entry, entry->timeout_);
  }
  else{
    delete entry;
  }
  delete e;
}
#else
void TimerManager::executeNextTimer()
{
#ifdef USE_THREADS
  pthread_mutex_lock(queue_mtx_);
#endif // USE_THREADS
  QueueEvent *e = eq_->eqPop();
  TimerEntry *entry = (TimerEntry *) e->payload_;

#ifdef USE_THREADS
  pthread_mutex_unlock(queue_mtx_);
#endif // USE_THREADS
  // run it
  int new_timeout = entry->cb_->expire();

  if (new_timeout >= 0){
    if (new_timeout > 0){
      // Change the timer's timeout
      entry->timeout_ = new_timeout;
    }
#ifdef USE_THREADS
    pthread_mutex_lock(queue_mtx_);
#endif // USE_THREADS
    eq_->eqAddAfter(entry->hdl_, (TimerEntry *) entry, entry->timeout_);
#ifdef USE_THREADS
    pthread_mutex_unlock(queue_mtx_);
#endif // USE_THREADS
  }
  else{
    delete entry;
  }
  delete e;
}

void TimerManager::executeAllExpiredTimers()
{
  struct timeval tv;

  // Get next timer's timeout
  nextTimerTime(&tv);

  // Remove all expired timers from the top of the queue, calling
  // expire() for each one of them
  while (tv.tv_sec == 0 && tv.tv_usec == 0){
    // Timer at the head of the queue has expired
    executeNextTimer();
    nextTimerTime(&tv);
  }
}
#endif // NS_DIFFUSION
