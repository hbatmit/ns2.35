
/*
 * mobicache.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: mobicache.cc,v 1.7 2006/02/21 15:20:18 mahrenho Exp $
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
// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
/* mobicache.cc
   cache used in the mobicom 98 submission.  see the paper for a description
   Ported from CMU/Monarch's code, appropriate copyright applies.  
*/

//#ifdef DSR_MOBICACHE

extern "C" {
#include <stdio.h>
#include <stdarg.h>
}

#undef DEBUG

#include <god.h>
#include "path.h"
#include "routecache.h"
#ifdef DSR_CACHE_STATS
#include "cache_stats.h"
#endif


/* invariants

   - no path contains an id more than once
   - all paths start with the id of our node (MAC_id or net_id)
*/

#define fout stdout

static const int verbose = 0;
static const int verbose_debug = 0;

/*===============================================================
  function selectors
----------------------------------------------------------------*/
bool cache_ignore_hints = false;     // ignore all hints?
bool cache_use_overheard_routes = true; 
// if we are A, and we over hear a rt Z Y (X) W V U, do we add route
// A X W V U to the cache?


/*===============================================================
  Class declaration
----------------------------------------------------------------*/
class MobiCache;

class Cache {
friend class MobiCache;

public:
  Cache(char *name, int size, MobiCache *rtcache);
  ~Cache();

  int pickVictim(int exclude = -1);
  // returns the index of a suitable victim in the cache
  // will spare the life of exclude
  bool searchRoute(const ID& dest, int& i, Path &path, int &index);
  // look for dest in cache, starting at index, 
  //if found, rtn true with path s.t. cache[index] == path && path[i] == dest
  Path* addRoute(Path &route, int &prefix_len);
  // rtns a pointer the path in the cache that we added
  void noticeDeadLink(const ID&from, const ID& to);
  // the link from->to isn't working anymore, purge routes containing
  // it from the cache

private:
  Path *cache;
  int size;
  int victim_ptr;		// next victim for eviction
  MobiCache *routecache;
  char *name;
};

///////////////////////////////////////////////////////////////////////////

class MobiCache : public RouteCache {
friend class Cache;
friend class MobiHandler;

public:
  MobiCache(const ID& MAC_id, const ID& net_id,int psize = 30,int ssize = 34 );
  MobiCache();
  ~MobiCache();
  void noticeDeadLink(const ID&from, const ID& to, Time t);
  // the link from->to isn't working anymore, purge routes containing
  // it from the cache
  void noticeRouteUsed(const Path& route, Time t,
		       const ID& who_from);
  // tell the cache about a route we saw being used
  void addRoute(const Path& route, Time t, const ID& who_from);
  // add this route to the cache (presumably we did a route request
  // to find this route and don't want to lose it)
  bool findRoute(ID dest, Path& route, int for_use = 0);
  // if there is a cached path from us to dest returns true and fills in
  // the route accordingly. returns false otherwise
  // if for_use, then we assume that the node really wants to keep 
  // the returned route so it will be promoted to primary storage if not there
  // already
  int command(int argc, const char*const* argv);

protected:
  Cache *primary_cache;   /* routes that we are using, or that we have reason
			     to believe we really want to hold on to */
  Cache *secondary_cache; /* routes we've learned via a speculative process
			     that might not pan out */

#ifdef DSR_CACHE_STATS
  void periodic_checkCache(void);
  void checkRoute(Path *p, int action, int prefix_len);
  void checkRoute(Path &p, int&, int&, double&, int&, int&, double &);
#endif
};


RouteCache *
makeRouteCache()
{
  return new MobiCache();
}


/*===============================================================
  OTcl definition
----------------------------------------------------------------*/
static class MobiCacheClass : public TclClass {
public:
        MobiCacheClass() : TclClass("MobiCache") {}
        TclObject* create(int, const char*const*) {
                return (new MobiCache);
        }
} class_MobiCache;

/*===============================================================
 Constructors
----------------------------------------------------------------*/
MobiCache::MobiCache(): RouteCache()
{
  primary_cache = new Cache("primary", 30, this);
  secondary_cache = new Cache("secondary", 64, this);
  //secondary_cache = new Cache("secondary", 10000, this);
  assert(primary_cache != NULL && secondary_cache != NULL);
#ifdef DSR_CACHE_STATS
	stat.reset();
#endif
}

MobiCache::~MobiCache()
{
  delete primary_cache;
  delete secondary_cache;
}

int
MobiCache::command(int argc, const char*const* argv)
{
  if(argc == 2 && strcasecmp(argv[1], "startdsr") == 0)
    { 
      if (ID(1,::IP) == net_id) 
	trace("Sconfig %.5f using MOBICACHE", Scheduler::instance().clock());
      // FALL-THROUGH
    }
  return RouteCache::command(argc, argv);
}

#ifdef DSR_CACHE_STATS

void
MobiCache::periodic_checkCache()
{
  int c;
  int route_count = 0;
  int route_bad_count = 0;
  int subroute_count = 0;
  int subroute_bad_count = 0;
  int link_bad_count = 0;
  double link_bad_time = 0.0;
  int link_bad_tested = 0;
  int link_good_tested = 0;

  for(c = 0; c < primary_cache->size; c++)
    {
      int x = 0;

      if (primary_cache->cache[c].length() == 0) continue;

      checkRoute(primary_cache->cache[c],
                 x,
                 link_bad_count,
                 link_bad_time,
                 link_bad_tested,
                 link_good_tested,
		 stat.link_good_time);

      route_count += 1;
      route_bad_count += x ? 1 : 0;
      
      subroute_count += primary_cache->cache[c].length() - 1;
      subroute_bad_count += x;
    }
  for(c = 0; c < secondary_cache->size; c++)
    {
      int x = 0;

      if (secondary_cache->cache[c].length() == 0) continue;

      checkRoute(secondary_cache->cache[c],
                 x,
                 link_bad_count,
                 link_bad_time,
                 link_bad_tested,
                 link_good_tested,
		 stat.link_good_time);

      route_count += 1;
      route_bad_count += x ? 1 : 0;

      subroute_count += secondary_cache->cache[c].length() - 1;
      subroute_bad_count += x;
    }

  // lifetime of good link is (total time) / (total num links - num bad links)
  stat.link_good_time = stat.link_good_time / (subroute_count - link_bad_count);

  trace("SRC %.9f _%s_ cache-summary %d %d %d %d | %d %.9f %d %d | %d %d %d %d %d | %d %d %d %d %d | %d %d %d %d %d %d %.9f",
	Scheduler::instance().clock(), net_id.dump(),
        route_count,
        route_bad_count,
        subroute_count,
        subroute_bad_count,

        link_bad_count,
        link_bad_count ? link_bad_time/link_bad_count : 0.0,
        link_bad_tested,
        link_good_tested,

        stat.route_add_count,
        stat.route_add_bad_count,
        stat.subroute_add_count,
        stat.subroute_add_bad_count,
        stat.link_add_tested,
             
        stat.route_notice_count,
        stat.route_notice_bad_count,
        stat.subroute_notice_count,
        stat.subroute_notice_bad_count,
        stat.link_notice_tested,
             
        stat.route_find_count,
	stat.route_find_for_me,
        stat.route_find_bad_count,
        stat.route_find_miss_count,
        stat.subroute_find_count,
        stat.subroute_find_bad_count,

	stat.link_good_time);
  stat.reset();
}

#endif /* DSR_CACHE_STATS */

/*===============================================================
  member functions
----------------------------------------------------------------*/

void
MobiCache::addRoute(const Path& route, Time t, const ID& who_from)
// add this route to the cache (presumably we did a route request
// to find this route and don't want to lose it)
// who_from is the id of the routes provider
{
  Path rt;

  if(pre_addRoute(route, rt, t, who_from) == 0)
    return;

  // must call addRoute before checkRoute
  int prefix_len = 0;

#ifdef DSR_CACHE_STATS
  Path *p = primary_cache->addRoute(rt, prefix_len);
  checkRoute(p, ACTION_ADD_ROUTE, prefix_len);
#else
  (void) primary_cache->addRoute(rt, prefix_len);
#endif
}

void
MobiCache::noticeDeadLink(const ID&from, const ID& to, Time)
  // the link from->to isn't working anymore, purge routes containing
  // it from the cache
{
  if(verbose_debug)
    trace("SRC %.9f _%s_ dead link %s->%s",
	  Scheduler::instance().clock(), net_id.dump(),
	  from.dump(), to.dump());
  
  primary_cache->noticeDeadLink(from, to);
  secondary_cache->noticeDeadLink(from, to);
  return;
}


void
MobiCache::noticeRouteUsed(const Path& p, Time t, const ID& who_from)
// tell the cache about a route we saw being used
{
  Path stub;
  if(pre_noticeRouteUsed(p, stub, t, who_from) == 0)
    return;

  int prefix_len = 0;

#ifdef DSR_CACHE_STATS
  Path *p0 = secondary_cache->addRoute(stub, prefix_len);
  checkRoute(p0, ACTION_NOTICE_ROUTE, prefix_len);
#else
  (void) secondary_cache->addRoute(stub, prefix_len);
#endif
}

bool
MobiCache::findRoute(ID dest, Path& route, int for_me)
// if there is a cached path from us to dest returns true and fills in
// the route accordingly. returns false otherwise
// if for_me, then we assume that the node really wants to keep 
// the returned route so it will be promoted to primary storage if not there
// already
{
  Path path;
  int min_index = -1;
  int min_length = MAX_SR_LEN + 1;
  int min_cache = 0;		// 2 == primary, 1 = secondary
  int index;
  int len;

  assert(!(net_id == invalid_addr));

  index = 0;
  while (primary_cache->searchRoute(dest, len, path, index))
    {
      min_cache = 2;
      if (len < min_length)
	{
	  min_length = len;
	  route = path;
	}
      index++;
    }
  
  index = 0;
  while (secondary_cache->searchRoute(dest, len, path, index))
    {
      if (len < min_length)
	{
	  min_index = index;
	  min_cache = 1;
	  min_length = len;
	  route = path;
	}
      index++;
    }

  if (min_cache == 1 && for_me)
    { // promote the found route to the primary cache
      int prefix_len;
 
      primary_cache->addRoute(secondary_cache->cache[min_index], prefix_len);

      // no need to run checkRoute over the Path* returned from
      // addRoute() because whatever was added was already in
      // the cache.

      //   prefix_len = 0
      //        - victim was selected in primary cache
      //        - data should be "silently" migrated from primary to the
      //          secondary cache
      //   prefix_len > 0
      //        - there were two copies of the first prefix_len routes
      //          in the cache, but after the migration, there will be
      //          only one.
      //        - log the first prefix_len bytes of the secondary cache
      //          entry as "evicted"
      if(prefix_len > 0)
        {
          secondary_cache->cache[min_index].setLength(prefix_len);
#ifdef DSR_CACHE_STATS
          checkRoute_logall(&secondary_cache->cache[min_index], 
                            ACTION_EVICT, 0);
#endif
        }
      secondary_cache->cache[min_index].setLength(0); // kill route
    }

  if (min_cache) 
    {
      route.setLength(min_length + 1);
      if (verbose_debug)
	trace("SRC %.9f _%s_ $hit for %s in %s %s",
	      Scheduler::instance().clock(), net_id.dump(),
	      dest.dump(), min_cache == 1 ? "secondary" : "primary",
	      route.dump());	
#ifdef DSR_CACHE_STATS
      int bad = checkRoute_logall(&route, ACTION_FIND_ROUTE, 0);      
      stat.route_find_count += 1;
      if (for_me) stat.route_find_for_me += 1;
      stat.route_find_bad_count += bad ? 1 : 0;
      stat.subroute_find_count += route.length() - 1;
      stat.subroute_find_bad_count += bad;
#endif
      return true;
    }
  else
    {
      if (verbose_debug)
        trace("SRC %.9f _%s_ find-route [%d] %s->%s miss %d %.9f",
              Scheduler::instance().clock(), net_id.dump(),
              0, net_id.dump(), dest.dump(), 0, 0.0);
#ifdef DSR_CACHE_STATS
      stat.route_find_count += 1;
      if (for_me) stat.route_find_for_me += 1;
      stat.route_find_miss_count += 1;
#endif
      return false;
    }
}

/*===========================================================================
  class Cache routines
---------------------------------------------------------------------------*/

Cache::Cache(char *name, int size, MobiCache *rtcache)
{
  this->name = name;
  this->size = size;
  cache = new Path[size];
  routecache = rtcache;
  victim_ptr = 0;
}

Cache::~Cache() 
{
  delete[] cache;
}

bool 
Cache::searchRoute(const ID& dest, int& i, Path &path, int &index)
  // look for dest in cache, starting at index, 
  //if found, return true with path s.t. cache[index] == path && path[i] == dest
{
  for (; index < size; index++)
    for (int n = 0 ; n < cache[index].length(); n++)
      if (cache[index][n] == dest) 
	{
	  i = n;
	  path = cache[index];
	  return true;
	}
  return false;
}

Path*
Cache::addRoute(Path & path, int &common_prefix_len)
{
  int index, m, n;
  int victim;

  // see if this route is already in the cache
  for (index = 0 ; index < size ; index++)
    { // for all paths in the cache
      for (n = 0 ; n < cache[index].length() ; n ++)
	{ // for all nodes in the path
	  if (n >= path.length()) break;
	  if (cache[index][n] != path[n]) break;
	}
      if (n == cache[index].length()) 
	{ // new rt completely contains cache[index] (or cache[index] is empty)
          common_prefix_len = n;
          for ( ; n < path.length() ; n++)
            cache[index].appendToPath(path[n]);
	  if (verbose_debug)
	    routecache->trace("SRC %.9f _%s_ %s suffix-rule (len %d/%d) %s",
   	      Scheduler::instance().clock(), routecache->net_id.dump(),
              name, n, path.length(), path.dump());	
	  goto done;
	}
      else if (n == path.length())
	{ // new route already contained in the cache
          common_prefix_len = n;
	  if (verbose_debug)
	    routecache->trace("SRC %.9f _%s_ %s prefix-rule (len %d/%d) %s",
   	      Scheduler::instance().clock(), routecache->net_id.dump(),
	      name, n, cache[index].length(), cache[index].dump());	
	  goto done;
	}
      else 
	{ // keep looking at the rest of the cache 
	}
    } 

  // there are some new goodies in the new route
  victim = pickVictim();
  if(verbose_debug) {
    routecache->trace("SRC %.9f _%s_ %s evicting %s",
		      Scheduler::instance().clock(), routecache->net_id.dump(),
		      name, cache[victim].dump());	
    routecache->trace("SRC %.9f _%s_ while adding %s",
		      Scheduler::instance().clock(), routecache->net_id.dump(),
		      path.dump());	
  }
  cache[victim].reset();
  CopyIntoPath(cache[victim], path, 0, path.length() - 1);
  common_prefix_len = 0;
  index = victim; // remember which cache line we stuck the path into

done:

#ifdef DEBUG
  {
    Path &p = path;
    int c;
    char buf[1000];
    char *ptr = buf;
    ptr += sprintf(buf,"Sdebug %.9f _%s_ adding ", 
		   Scheduler::instance().clock(), routecache->net_id.dump());
    for (c = 0 ; c < p.length(); c++)
      ptr += sprintf(ptr,"%s [%d %.9f] ",p[c].dump(), p[c].link_type, p[c].t);
    routecache->trace(buf);
  }
#endif //DEBUG

  // freshen all the timestamps on the links in the cache
  for (m = 0 ; m < size ; m++)
    { // for all paths in the cache

#ifdef DEBUG
  {
    if (cache[m].length() == 0) continue;

    Path &p = cache[m];
    int c;
    char buf[1000];
    char *ptr = buf;
    ptr += sprintf(buf,"Sdebug %.9f _%s_ checking ", 
		   Scheduler::instance().clock(), routecache->net_id.dump());
    for (c = 0 ; c < p.length(); c++)
      ptr += sprintf(ptr,"%s [%d %.9f] ",p[c].dump(), p[c].link_type, p[c].t);
    routecache->trace(buf);
  }
#endif //DEBUG
      
      for (n = 0 ; n < cache[m].length() - 1 ; n ++)
	{ // for all nodes in the path
	  if (n >= path.length() - 1) break;
	  if (cache[m][n] != path[n]) break;
	  if (cache[m][n+1] == path[n+1])
	    { // freshen the timestamps and type of the link	      

#ifdef DEBUG
routecache->trace("Sdebug %.9f _%s_ freshening %s->%s to %d %.9f",
		  Scheduler::instance().clock(), routecache->net_id.dump(),
		  path[n].dump(), path[n+1].dump(), path[n].link_type,
		  path[n].t);
#endif //DEBUG

	      cache[m][n].t = path[n].t;
	      cache[m][n].link_type = path[n].link_type;
	      /* NOTE: we don't check to see if we're turning a TESTED
		 into an UNTESTED link.  Last change made rules -dam 5/19/98 */
	    }
	}
    }
  return &cache[index];
}


void
Cache::noticeDeadLink(const ID&from, const ID& to)
  // the link from->to isn't working anymore, purge routes containing
  // it from the cache
{  
  for (int p = 0 ; p < size ; p++)
    { // for all paths in the cache
      for (int n = 0 ; n < (cache[p].length()-1) ; n ++)
	{ // for all nodes in the path
	  if (cache[p][n] == from && cache[p][n+1] == to)
	    {
	      if(verbose_debug)
		routecache->trace("SRC %.9f _%s_ %s truncating %s %s",
                                  Scheduler::instance().clock(),
                                  routecache->net_id.dump(),
                                  name, cache[p].dump(),
                                  cache[p].owner().dump());
#ifdef DSR_CACHE_STATS
              routecache->checkRoute(&cache[p], ACTION_CHECK_CACHE, 0);
              routecache->checkRoute_logall(&cache[p], ACTION_DEAD_LINK, n);
#endif	      
	      if (n == 0)
		cache[p].reset();        // kill the whole path
	      else {
		cache[p].setLength(n+1); // truncate the path here
                cache[p][n].log_stat = LS_UNLOGGED;
              }

	      if(verbose_debug)
		routecache->trace("SRC %.9f _%s_ to %s %s",
		      Scheduler::instance().clock(), routecache->net_id.dump(),
		      cache[p].dump(), cache[p].owner().dump());

	      break;
	    } // end if this is a dead link
	} // end for all nodes
    } // end for all paths
  return;
}

int
Cache::pickVictim(int exclude)
// returns the index of a suitable victim in the cache
// never return exclude as the victim, but rather spare their life
{
  for(int c = 0; c < size ; c++)
    if (cache[c].length() == 0) return c;
  
  int victim = victim_ptr;
  while (victim == exclude)
    {
      victim_ptr = (victim_ptr+1 == size) ? 0 : victim_ptr+1;
      victim = victim_ptr;
    }
  victim_ptr = (victim_ptr+1 == size) ? 0 : victim_ptr+1;

#ifdef DSR_CACHE_STATS
  routecache->checkRoute(&cache[victim], ACTION_CHECK_CACHE, 0);
  int bad = routecache->checkRoute_logall(&cache[victim], ACTION_EVICT, 0);
  routecache->trace("SRC %.9f _%s_ evicting %d %d %s",
                    Scheduler::instance().clock(), routecache->net_id.dump(),
                    cache[victim].length() - 1, bad, name);
#endif
  return victim;
}

#ifdef DSR_CACHE_STATS

/*
 * Called only for the once-per-second cache check.
 */
void
MobiCache::checkRoute(Path & p,
                      int & subroute_bad_count,
                      int & link_bad_count,
                      double & link_bad_time,
                      int & link_bad_tested,
                      int & link_good_tested,
		      double & link_good_time)
{
  int c;
  int flag = 0;

  if(p.length() == 0)
    return;
  assert(p.length() >= 2);

  for (c = 0; c < p.length() - 1; c++)
    {
      assert(LS_UNLOGGED == p[c].log_stat || LS_LOGGED == p[c].log_stat );
      if (God::instance()->hops(p[c].getNSAddr_t(), p[c+1].getNSAddr_t()) != 1)
	{ // the link's dead
          if(p[c].log_stat == LS_UNLOGGED)
	    {
	      trace("SRC %.9f _%s_ check-cache [%d %d] %s->%s dead %d %.9f",
		    Scheduler::instance().clock(), net_id.dump(),
		    p.length(), c, p[c].dump(), p[c+1].dump(),
                    p[c].link_type, p[c].t);
	      p[c].log_stat = LS_LOGGED;
	    }
          if(flag == 0)
            {
              subroute_bad_count += p.length() - c - 1;
              flag = 1;
            }
          link_bad_count += 1;
          link_bad_time += Scheduler::instance().clock() - p[c].t;
          link_bad_tested += (p[c].link_type == LT_TESTED) ? 1 : 0;
	}
      else
        {
	  
	  link_good_time += Scheduler::instance().clock() - p[c].t;
	  
          if(p[c].log_stat == LS_LOGGED)
            {
              trace("SRC %.9f _%s_ resurrected-link [%d %d] %s->%s dead %d %.9f",
                    Scheduler::instance().clock(), net_id.dump(),
                    p.length(), c, p[c].dump(), p[c+1].dump(),
                    p[c].link_type, p[c].t);
              p[c].log_stat = LS_UNLOGGED;
            }
          link_good_tested += (p[c].link_type == LT_TESTED) ? 1 : 0;
        }
    }
}

void
MobiCache::checkRoute(Path *p, int action, int prefix_len)
{
  int c;
  int subroute_bad_count = 0;
  int tested = 0;

  if(p->length() == 0)
    return;
  assert(p->length() >= 2);

  assert(action == ACTION_ADD_ROUTE ||
         action == ACTION_CHECK_CACHE ||
         action == ACTION_NOTICE_ROUTE);

  for (c = 0; c < p->length() - 1; c++)
    {
      if (God::instance()->hops((*p)[c].getNSAddr_t(),
                                (*p)[c+1].getNSAddr_t()) != 1)
	{ // the link's dead
          if((*p)[c].log_stat == LS_UNLOGGED)
            {
              trace("SRC %.9f _%s_ %s [%d %d] %s->%s dead %d %.9f",
                    Scheduler::instance().clock(), net_id.dump(),
                    action_name[action], p->length(), c,
                    (*p)[c].dump(), (*p)[c+1].dump(),
                    (*p)[c].link_type, (*p)[c].t);

              (*p)[c].log_stat = LS_LOGGED;
            }

          if(subroute_bad_count == 0)
            subroute_bad_count = p->length() - c - 1;
	}
      else
        {
          if((*p)[c].log_stat == LS_LOGGED)
            {
              trace("SRC %.9f _%s_ resurrected-link [%d %d] %s->%s dead %d %.9f",
                    Scheduler::instance().clock(), net_id.dump(),
                    p->length(), c, (*p)[c].dump(), (*p)[c+1].dump(),
                    (*p)[c].link_type, (*p)[c].t);
              (*p)[c].log_stat = LS_UNLOGGED;
            }
        }
      tested += (*p)[c].link_type == LT_TESTED ? 1 : 0;
    }

  /*
   * Add Route or Notice Route actually did something
   */
  if(prefix_len < p->length())
    {
      switch(action)
        {
        case ACTION_ADD_ROUTE:
          stat.route_add_count += 1;
          stat.route_add_bad_count += subroute_bad_count ? 1 : 0;
          stat.subroute_add_count += p->length() - prefix_len - 1;
          stat.subroute_add_bad_count += subroute_bad_count;
          stat.link_add_tested += tested;
          break;

        case ACTION_NOTICE_ROUTE:
          stat.route_notice_count += 1;
          stat.route_notice_bad_count += subroute_bad_count ? 1 : 0;
          stat.subroute_notice_count += p->length() - prefix_len - 1;
          stat.subroute_notice_bad_count += subroute_bad_count;
          stat.link_notice_tested += tested;
          break;
        }
    }
}
#endif /* DSR_CACHE_STATS */

//#endif /* DSR_MOBICACHE */
