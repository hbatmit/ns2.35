
/*
 * Copyright (C) 2004-2005 by the University of Southern California
 * $Id: difftimer.cc,v 1.4 2005/09/13 20:47:34 johnh Exp $
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
// Diffusion-event handler class, Padma, nov 2001. 

#ifdef NS_DIFFUSION

#include "difftimer.h"
#include "diffagent.h"
#include "diffrtg.h"

DiffEvent::DiffEvent(d_handle hdl, void *payload, int time) {
	handle_ = hdl;
	payload_ = payload;
	GetTime(&tv_);
	TimevalAddusecs(&tv_, time*1000);
}

void DiffEventQueue::eqAddAfter(d_handle hdl, void *payload, int delay_msec) {
	DiffEvent* de;
	
	de = new DiffEvent(hdl, payload, delay_msec);
	DiffEventHandler *dh = timerHandler_;
	double delay = ((double)delay_msec)/1000;   //convert msec to sec
	
	(void)Scheduler::instance().schedule(dh, de, delay);
	scheduler_uid_t uid = ((Event *)de)->uid_;
	
	setUID(hdl, uid);
}


DiffEvent *DiffEventQueue::eqFindEvent(d_handle hdl) {
	scheduler_uid_t uid = getUID(hdl);
	if (uid != -1) {
		Event *p = Scheduler::instance().lookup(uid);
		if (p != 0) {
			return ((DiffEvent *)p);
		} else {
			fprintf(stderr, "Error: Can't find event in scheduler queue\n");
			return NULL;
		}
	} else {
		fprintf(stderr, "Error: Can't find event in uidmap!\n");
		return NULL;
	}
}

bool DiffEventQueue::eqRemove(d_handle hdl) {
	// first lookup UID
	scheduler_uid_t uid = getUID(hdl);
	if (uid != -1) {
		// next look for event in scheduler queue
		Event *p = Scheduler::instance().lookup(uid);
		if (p != 0) {
			// remove event from scheduler
			Scheduler::instance().cancel(p);
			TimerEntry* te = (TimerEntry *)((DiffEvent *)p)->payload();
			delete te;
			delete p;
			return 1;
		} else {
			fprintf(stderr, "Error: Can't find event in scheduler queue\n");
			return 0;
		}
	} else {
		fprintf(stderr, "Error: Can't find event in uidmap!\n");
		return 0;
	}
}

// sets value of UID and matching handle into uidmap
void DiffEventQueue::setUID(d_handle hdl, scheduler_uid_t uid) {
	MapEntry *me = new MapEntry;
	me->hdl_ = hdl;
	me->uid_ = uid;
	uidmap_.push_back(me);
}

// finds UID for matching handle; removes entry from uidmap and returns uid
scheduler_uid_t DiffEventQueue::getUID(d_handle hdl) {
	UIDMap_t::iterator itr;
	MapEntry* entry;
	for (itr = uidmap_.begin(); itr != uidmap_.end(); ++itr) {
		entry = *itr;
		if (entry->hdl_ == hdl) { // found handle
			// don't need this entry, take it out of list
			scheduler_uid_t uid = entry->uid_;
			itr = uidmap_.erase(itr);
			delete entry;
			return uid;
		}
	}
	return (-1);
}

// event handler that gets called at expiry time of event
void DiffEventHandler::handle(Event *e) {
	DiffEvent *de = (DiffEvent *)e;
	
	d_handle hdl = de->getHandle();
	queue_->getUID(hdl); // only removes entry from uidmap
	a_->diffTimeout(de);
}


#endif // NS
