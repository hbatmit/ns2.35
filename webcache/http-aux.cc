/*
 * Copyright (c) Xerox Corporation 1998. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linking this file statically or dynamically with other modules is making
 * a combined work based on this file.  Thus, the terms and conditions of
 * the GNU General Public License cover the whole combination.
 *
 * In addition, as a special exception, the copyright holders of this file
 * give you permission to combine this file with free software programs or
 * libraries that are released under the GNU LGPL and with code included in
 * the standard release of ns-2 under the Apache 2.0 license or under
 * otherwise-compatible licenses with advertising requirements (or modified
 * versions of such code, with unchanged license).  You may copy and
 * distribute such a system following the terms of the GNU GPL for this
 * file and the licenses of the other code concerned, provided that you
 * include the source code of that other code when and as the GNU GPL
 * requires distribution of source code.
 *
 * Note that people who make modified versions of this file are not
 * obligated to grant this special exception for their modified versions;
 * it is their choice whether to do so.  The GNU General Public License
 * gives permission to release a modified version without this exception;
 * this exception also makes it possible to release a modified version
 * which carries forward this exception.
 *
 */

//
// Auxiliary classes for Http multicast invalidation proxy cache
//

#include "http-aux.h"
#include "http.h"

void PushTimer::expire(Event *) 
{
	a_->timeout(HTTP_UPDATE);
}

void HBTimer::expire(Event *) 
{
	a_->timeout(HTTP_INVALIDATION);
}

void LivenessTimer::expire(Event *)
{
	a_->handle_node_failure(nbr_);
}

void HttpHbData::extract(InvalidationRec*& head)
{
	// XXX head must be passed in from outside, because the 
	// InvalidationRec list will obtain the address of head
	head = NULL;
	InvalidationRec *q = NULL;
	// reconstruct a list of invalidation records.
	// We keep the updating record 
	for (int i = 0; i < num_inv(); i++) {
		// Used only to mark that this page will be send in 
		// the next multicast update. The updating field in 
		// this agent will only be set after it gets the real 
		// update. Here we set this field temporarily so that
		// recv_inv can find out the 
		q = inv_rec()[i].copyto();
		q->insert(&head);
	}
}

NeighborCache::~NeighborCache()
{
	timer_->cancel();
	delete timer_;
	ServerEntry *s = sl_.gethead(), *p;
	while (s != NULL) {
		p = s;
		s = s->next();
		delete p;
	}
}

int NeighborCache::is_server_down(int sid)
{
	ServerEntry *s = sl_.gethead();
	while (s != NULL) {
		if (s->server() == sid)
			return s->is_down();
		s = s->next();
	}
	return 0;
}

void NeighborCache::server_down(int sid)
{
	ServerEntry *s = sl_.gethead();
	while (s != NULL) {
		if (s->server() == sid)
			s->down();
		s = s->next();
	}
}

void NeighborCache::server_up(int sid)
{
	ServerEntry *s = sl_.gethead();
	while (s != NULL) {
		if (s->server() == sid)
			s->up();
		s = s->next();
	}
}

void NeighborCache::invalidate(HttpMInvalCache *c)
{
	ServerEntry *s = sl_.gethead();
	while (s != NULL) {
		c->invalidate_server(s->server());
		s = s->next();
	}
}

void NeighborCache::pack_leave(HttpLeaveData& data)
{
	int i;
	ServerEntry *s;
	for (i = 0, s = sl_.gethead(); s != NULL; s = s->next(), i++)
		data.add(i, s->server());
}

void NeighborCache::add_server(int sid)
{
	ServerEntry *s = new ServerEntry(sid);
	sl_.insert(s);
}
