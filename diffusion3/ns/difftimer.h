
/*
 * Copyright (C) 2004-2005 by the University of Southern California
 * $Id: difftimer.h,v 1.4 2005/09/13 20:47:35 johnh Exp $
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
// Diffusion-event handler object, Padma, nov 2001.

#ifdef NS_DIFFUSION

#ifndef diffevent_handler_h
#define diffevent_handler_h


#include <list>
#include "scheduler.h"
#include "timers.hh"


class MapEntry;
class DiffEventQueue;

typedef long d_handle;
typedef list<MapEntry *> UIDMap_t;


class MapEntry {
public:
	d_handle hdl_;
	scheduler_uid_t uid_;
};



/* This handler class keeps track of diffusion specific events. 
   It schedules multiple events and irrespective of the event types, 
   always calls back the callback function of its agent.
*/

class DiffEvent : public Event {
private:
	struct timeval tv_;	
	d_handle handle_;
	void* payload_;
public:
	DiffEvent(d_handle hdl, void *payload, int time);
	~DiffEvent() { }
	int gettime() {
		return ((tv_.tv_sec) + ((tv_.tv_usec)/1000000));
	}
	d_handle getHandle() { return handle_; }
	void* payload() { return payload_; }
};


class DiffEventHandler : public Handler {
public:
	DiffEventHandler(TimerManager *agent, DiffEventQueue *deq) { 
		a_ = agent; 
		queue_ = deq;
	}
	void handle(Event *);

private:
	TimerManager *a_;
	DiffEventQueue *queue_;
};


class DiffEventQueue : public EventQueue { 
public: 
	DiffEventQueue(TimerManager* a) { 
		a_ = a; 
		timerHandler_ = new DiffEventHandler(a_, this);
	} 
	~DiffEventQueue() { delete timerHandler_; }
	// queue functions
	void eqAddAfter(d_handle hdl, void *payload, int delay_msec);
	DiffEvent *eqFindEvent(d_handle hdl);
	bool eqRemove(d_handle hdl);
	
	// mapping functions
	void setUID(d_handle hdl, scheduler_uid_t uid);
	scheduler_uid_t getUID(d_handle hdl);
	
private: 
	TimerManager *a_;
	DiffEventHandler *timerHandler_;
	
	// map for diffusion handle and ns scheduler uid
	UIDMap_t uidmap_;
}; 





#endif //diffusion_timer_h
#endif // NS
