
/*
 * cache_stats.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: cache_stats.h,v 1.4 2005/08/25 18:58:04 johnh Exp $
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

// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
// Ported from CMU/Monarch's code, appropriate copyright applies.  

#ifndef __cache_stats__
#define __cache_stats__

#include "routecache.h"

#define ACTION_ADD_ROUTE        1
#define ACTION_NOTICE_ROUTE     2
#define ACTION_FIND_ROUTE       3
#define ACTION_DEAD_LINK        4
#define ACTION_EVICT            5
#define ACTION_CHECK_CACHE      6

static char *action_name [] = {"",
                              "add-route", "notice-route", "find-route",
                              "dead-link", "evicting-route", "check-cache"};

class cache_stats {
public:
        void reset() {
                route_add_count = 0;
                route_add_bad_count = 0;
                subroute_add_count = 0;
                subroute_add_bad_count = 0;
                link_add_tested = 0;

                route_notice_count = 0;
                route_notice_bad_count = 0;
                subroute_notice_count = 0;
                subroute_notice_bad_count = 0;
                link_notice_tested = 0;

                route_find_count = 0;
		route_find_for_me = 0;
                route_find_bad_count = 0;
                route_find_miss_count = 0;
                subroute_find_count = 0;
                subroute_find_bad_count = 0;

		link_add_count = 0;
		link_add_bad_count = 0;
		link_notice_count = 0;
		link_notice_bad_count = 0;
		link_find_count = 0;
		link_find_bad_count = 0;
		
		link_good_time = 0.0;

        }
                
        int     route_add_count;
        int     route_add_bad_count;
        int     subroute_add_count;
        int     subroute_add_bad_count;
        int     link_add_tested;

        int     route_notice_count;
        int     route_notice_bad_count;
        int     subroute_notice_count;
        int     subroute_notice_bad_count;
        int     link_notice_tested;

        int     route_find_count;
        int     route_find_for_me;
        int     route_find_bad_count;
        int     route_find_miss_count;
        int     subroute_find_count;
        int     subroute_find_bad_count;

	// extras for the Link-State Cache
	int	link_add_count;
	int	link_add_bad_count;
	int	link_notice_count;
	int	link_notice_bad_count;
	int	link_find_count;
	int	link_find_bad_count;

	double     link_good_time;
};


/*===============================================================
  Handler to collect statistics once per second.
----------------------------------------------------------------*/

class MobiHandler : public Handler {
public:
        MobiHandler(RouteCache *C) {
                interval = 1.0;
                cache = C;
        }
        void start() {
                Scheduler::instance().schedule(this, &intr, 1.0);
        }
        void handle(Event *);
private:
        double interval;
        Event intr;
        RouteCache *cache;
};

#endif /* __cache_stats_h__ */
