
/*
 * routecache.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: routecache.h,v 1.8 2006/02/21 15:20:18 mahrenho Exp $
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
/* -*- c++ -*- 
   routecache.h

   Interface all route caches must support

*/

#ifndef _routecache_h
#define _routecache_h

#include <cstdarg>

#include <object.h>
#include <trace.h>

#include "path.h"

#ifdef DSR_CACHE_STATS
class RouteCache;
#include <dsr/cache_stats.h>
#endif

class RouteCache : public TclObject {

public:
  RouteCache();
  ~RouteCache();

  virtual void noticeDeadLink(const ID&from, const ID& to, Time t) = 0;
  // the link from->to isn't working anymore, purge routes containing
  // it from the cache

  virtual void noticeRouteUsed(const Path& route, Time t, 
			       const ID& who_from) = 0;
  // tell the cache about a route we saw being used
  // if first tested is set, then we assume the first link was recently
  // known to work

  virtual void addRoute(const Path& route, Time t, const ID& who_from) = 0;
  // add this route to the cache (presumably we did a route request
  // to find this route and don't want to lose it)

  virtual bool findRoute(ID dest, Path& route, int for_use) = 0;
  // if there is a cached path from us to dest returns true and fills in
  // the route accordingly. returns false otherwise
  // if for_use, then we assume that the node really wants to keep 
  // the returned route

  virtual int command(int argc, const char*const* argv);
  void trace(char* fmt, ...);

  // *******************************************************

  int pre_addRoute(const Path& route, Path &rt,
		   Time t, const ID& who_from);
  // returns 1 if the Route should be added to the cache, 0 otherwise.

  int pre_noticeRouteUsed(const Path& p, Path& stub,
			  Time t, const ID& who_from);
  // returns 1 if the Route should be added to the cache, 0 otherwise.

#ifdef DSR_CACHE_STATS
  MobiHandler mh;
  struct cache_stats stat;

  virtual void periodic_checkCache() = 0;
  int checkRoute_logall(Path *p, int action, int start);
#endif


  //**********************************************************

  ID MAC_id, net_id;
  virtual void dump(FILE *out);

protected:
  Trace *logtarget;
};

RouteCache * makeRouteCache();
// return a ref to the route cache that should be used in this sim

#endif // _routecache_h
