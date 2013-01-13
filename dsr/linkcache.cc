
/*
 * linkcache.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: linkcache.cc,v 1.2 2005/08/25 18:58:05 johnh Exp $
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
// Ported from CMU/Monarch's code, appropriate copyright applies.  
#ifdef DSR_LINKCACHE

#include <god.h>
#include "path.h"
#include "routecache.h"
#include <list.h>
#ifdef DSR_CACHE_STATS
#include "cache_stats.h"
#endif

#define CURRENT_TIME Scheduler::instance().clock()
#define MAX_SIMTIME  86400

bool cache_ignore_hints = false;     // ignore all hints?
bool cache_use_overheard_routes = true; 

// Specifies whether or not to retain links after they go bad
// I don't think this is implemented.
static const bool lc_cache_negative_links = false;
static const double lc_neg_cache_life = 1.0;

// When inserting/deleting link A->B, also insert/delete B->A.
static const bool lc_bidirectional_links = true;

// Evict "unreachable" links from the cache.
static const bool lc_evict_unreachable_links = false;

// do we expire in realtime? or do we wait for use?
//#define REALTIME_EXPIRE

// The maximum number of nodes supported by the cache.
#define LC_MAX_NODES 200

// General generational stuff...
static const double lc_gen_use_bonus = 300;

// Static stuff
static const double lc_static_discover_life = 5.0;
static const double lc_static_overhear_life = 5.0;

// Table stuff
static const double lc_table_init_val = 50.0;
static const double lc_table_mult_by  = 0.5;
static const double lc_linear_alpha   = 1.0/8.0;
static const double lc_exp_add_amt    = 2.0;
static const double lc_exp_div_amt    = 2.0;
static const double lc_minlifetime    = 2.0;

// types of linkgens
#define EXPPOLICY_INFINITE  0
#define EXPPOLICY_STATIC    1
#define EXPPOLICY_LINEAR    2
#define EXPPOLICY_EXP       3
#ifdef GOD_STABILITY
#define EXPPOLICY_GOD_OMNI  4
#define EXPPOLICY_GOD_TABLE 5
#endif
#define LONGEST_LIVED_ROUTE

static const int lc_exppolicy = EXPPOLICY_EXP;

#ifdef GOD_STABILITY
extern double *godavglife;
#endif

#ifndef REALTIME_EXPIRE
// statistics...
#define IS_GOOD     0
#define IS_BAD      1
#define CALLED_GOOD 0
#define CALLED_BAD  2
static int expirestats[4];
#endif

static int verbose_debug = 0;

// *****************************************************

class Link {
public:
	Link(nsaddr_t dst) {
		ln_dst = dst;
		ln_flags = ln_cost = 0;
		ln_timeout = 0.0;
		ln_insert = 0.0;
		ln_t = 0.0;
		ln_linktype = LT_NONE;
		ln_logstat = LS_UNLOGGED;
	}

	inline bool expired() {
		return (ln_timeout <= CURRENT_TIME);
	}

	LIST_ENTRY(Link) ln_link;
	nsaddr_t   ln_dst;
	int        ln_flags;       // link status information
#define LINK_FLAG_UP 0x01

	/*
	 * Right now all links have the same cost (1). - josh
	 */
	int        ln_cost;        // cost of using the link

	/*
	 * the use of ln_timeout to expire links from the
	 * cache has not yet been implemented. - josh
	 * Implemented. - yihchun
	 */
	double     ln_timeout;     // when the link expires
	double     ln_insert;      // when the link expires

#define LINK_TIMEOUT MAX_SIMTIME

	// mirror the values kept in class ID
	Time	   ln_t;
	Link_Type  ln_linktype;
	Log_Status ln_logstat;
};

LIST_HEAD(dsrLinkHead, Link);

class LinkCache : public RouteCache {
friend class MobiHandler;

public:
	LinkCache();

	void noticeDeadLink(const ID&from, const ID& to, Time t);
	// the link from->to isn't working anymore, purge routes containing
	// it from the cache

	void noticeRouteUsed(const Path& route, Time t,
			     const ID& who_from);
	// tell the cache about a route we saw being used

	void addRoute(const Path& route, Time t, const ID& who_from);
	// add this route to the cache (presumably we did a route request
	// to find this route and don't want to lose it)

	bool findRoute(ID dest, Path& route, int for_me = 0);
	// if there is a cached path from us to dest returns true and fills in
	// the route accordingly. returns false otherwise
	// if for_use, then we assume that the node really wants to keep 
	// the returned route so it will be promoted to primary storage
	// if not there already
	int command(int argc, const char*const* argv);

protected:
	dsrLinkHead lcache[LC_MAX_NODES + 1];
	// note the zeroth index is not used

	int addLink(const ID& from, const ID& to,
		    int flags, double timeout = LINK_TIMEOUT, int cost = 1);
	int delLink(const ID& from, const ID& to);
	Link* findLink(int from, int to);
	void purgeLink(void);
	void dumpLink(void);

	void Link_to_ID(Link* l, ID& id);

#ifdef DSR_CACHE_STATS
	void checkLink_logall(const ID& from, const ID& to, int action);
	void checkLink(const ID& from, const ID& to, int action);
	void periodic_checkCache();

	struct cache_stats stat;
#endif
private:
	////////////////////////////////////////////////////////////
	// Dijkstra's Algorithm

	double dirty; // the next time it gets dirty
#ifdef LONGEST_LIVED_ROUTE
	double dl[LC_MAX_NODES + 1];
#endif
	u_int32_t d[LC_MAX_NODES + 1];
	u_int32_t pi[LC_MAX_NODES + 1];
	bool S[LC_MAX_NODES + 1];
	double exptable[LC_MAX_NODES + 1];
#define INFINITY 0x7fffffff

	void init_single_source(int s);
#ifdef LONGEST_LIVED_ROUTE
	void relax(u_int32_t u, u_int32_t v, u_int32_t w, double);
#else
	void relax(u_int32_t u, u_int32_t v, u_int32_t w);
#endif
	int  extract_min_q(void);
	void dijkstra(void);
	void dump_dijkstra(int dst);
	double find_timeout(ID a, ID b, bool discovered);
};

///////////////////////////////////////////////////////////////////////////

double LinkCache::find_timeout(ID a, ID b, bool discovered) {
	double lifetime = 0.0;
	switch (lc_exppolicy) {
	case EXPPOLICY_INFINITE:
#ifdef GOD_STABILITY
        case EXPPOLICY_GOD_OMNI:
#endif
		return MAX_SIMTIME;
	case EXPPOLICY_LINEAR: case EXPPOLICY_EXP:
		lifetime = exptable[a.addr] > exptable[b.addr] ? 
		  exptable[b.addr] : exptable[a.addr];
		lifetime *= lc_table_mult_by;
		break;
#ifdef GOD_STABILITY
	case EXPPOLICY_GOD_TABLE:
		lifetime = godavglife[a.addr] > godavglife[b.addr] ? 
		  godavglife[b.addr] : godavglife[a.addr];
		lifetime *= lc_table_mult_by;
		break;
#endif
	case EXPPOLICY_STATIC:
		lifetime = discovered ? lc_static_discover_life :
		  lc_static_overhear_life;
		break;
	default:
		assert(0);
	}

	if (lifetime < lc_minlifetime)
		lifetime = lc_minlifetime;

	return CURRENT_TIME + lifetime;
}

RouteCache *
makeRouteCache()
{
  return new LinkCache();
}

LinkCache::LinkCache() : RouteCache()
{
	int i = 0;

	for(i = 0; i <= LC_MAX_NODES; i++) {
		LIST_INIT(&lcache[i]);
		exptable[i] = lc_table_init_val;
	}

#ifdef DSR_CACHE_STATS
	stat.reset();
#endif
	dirty = -1;
}


int
LinkCache::command(int argc, const char*const* argv)
{
  if(argc == 2 && strcasecmp(argv[1], "startdsr") == 0)
    { 
      if (ID(1,::IP) == net_id) 
	trace("Sconfig %.5f using LINKCACHE %d", Scheduler::instance().clock(),
		lc_exppolicy);
      // FALL-THROUGH
    }

  if(argc == 3)
    {   
      if (strcasecmp(argv[1], "ip-addr") == 0)
        {
          assert(atoi(argv[2]) <= LC_MAX_NODES);

#ifndef REALTIME_EXPIRE
	  if (atoi(argv[2]) == 1)
	    bzero(expirestats, sizeof(expirestats));
#endif

          // don't return
        } 
    }
  return RouteCache::command(argc, argv);
}


void
LinkCache::noticeDeadLink(const ID& from, const ID& to, Time)
{
	Link *l = findLink(from.addr, to.addr);

	if (l && (l->ln_flags & LINK_FLAG_UP) &&
	    (lc_exppolicy == EXPPOLICY_LINEAR || 
	     lc_exppolicy == EXPPOLICY_EXP)) {
		if (lc_exppolicy == EXPPOLICY_LINEAR) {
			exptable[from.addr]=lc_linear_alpha*exptable[from.addr]
			  +(CURRENT_TIME-l->ln_insert)*(1-lc_linear_alpha);
			exptable[to.addr]=lc_linear_alpha*exptable[to.addr]
			  +(CURRENT_TIME-l->ln_insert)*(1-lc_linear_alpha);
		} else {
			exptable[from.addr] /= lc_exp_div_amt;
			exptable[to.addr] /= lc_exp_div_amt;
		}
	}

	if(lc_cache_negative_links == true) {
		if(l) {
			l->ln_flags &= ~LINK_FLAG_UP;
			l->ln_insert = CURRENT_TIME;
			l->ln_timeout = CURRENT_TIME + lc_neg_cache_life;
			dirty = CURRENT_TIME;
		} else {
			addLink(from, to, 0, CURRENT_TIME + lc_neg_cache_life);
		}
	}
	else {
		if(l) {
#ifdef DSR_CACHE_STATS
			checkLink(from, to, ACTION_DEAD_LINK);
#endif
			(void) delLink(from, to);
		}
	}
}

void
LinkCache::noticeRouteUsed(const Path& p, Time t, const ID& who_from)
{
  Path stub;
  int c;

  if(pre_noticeRouteUsed(p, stub, t, who_from) == 0)
	  return;
  
#ifdef DSR_CACHE_STATS
  int link_notice_count = stat.link_notice_count; 
  int link_notice_bad_count = stat.link_notice_bad_count;   
#endif

  /*
   * Add each link in the "path" to the route cache.
   */
  for(c = 0; c < stub.length() - 1; c++) {
	  if(addLink(stub[c], stub[c+1], LINK_FLAG_UP, 
		     find_timeout(stub[c], stub[c+1], false))) {
#ifdef DSR_CACHE_STATS
		checkLink(stub[c], stub[c+1], ACTION_NOTICE_ROUTE);
#endif
	  }
  }

#ifdef DSR_CACHE_STATS
  if(stat.link_notice_count > link_notice_count)
	  stat.route_notice_count++;
  if(stat.link_notice_bad_count > link_notice_bad_count)
	  stat.route_notice_bad_count++;
#endif
}

void
LinkCache::addRoute(const Path& route, Time t, const ID& who_from)
{
  Path rt;
  int c;

  if(pre_addRoute(route, rt, t, who_from) == 0)
	  return;
  
#ifdef DSR_CACHE_STATS
  int link_add_count = stat.link_add_count; 
  int link_add_bad_count = stat.link_add_bad_count;   
#endif

  for(c = 0; c < rt.length() - 1; c++) {
	  if(addLink(rt[c], rt[c+1], LINK_FLAG_UP,
		     find_timeout(rt[c], rt[c+1], true))) {
#ifdef DSR_CACHE_STATS
		  checkLink(rt[c], rt[c+1], ACTION_ADD_ROUTE);
#endif
	  }
  }

#ifdef DSR_CACHE_STATS
  if(stat.link_add_count > link_add_count)
	  stat.route_add_count++;
  if(stat.link_add_bad_count > link_add_bad_count)
	  stat.route_add_bad_count++;
#endif
}

bool
LinkCache::findRoute(ID dest, Path& route, int for_me = 0)
{
	u_int32_t v;
	int rpath[MAX_SR_LEN];
	u_int32_t roff = 0;

	if(verbose_debug) dumpLink();

	/*
	 * Compute all of the shortest paths...
	 */
	if(dirty <= CURRENT_TIME) {
		dijkstra();

		// remove all of the links for nodes that are unreachable
		if(lc_evict_unreachable_links == true) {
			purgeLink();
		}
	}
	if(verbose_debug) dump_dijkstra(dest.addr);

	/*
	 * Trace backwards to figure out the path to DEST
	 */
	for(v = dest.addr; d[v] < INFINITY; v = pi[v]) {
	  //assert(v >= 1 && v <= LC_MAX_NODES);
		if (roff >= MAX_SR_LEN) {
 		        // path between us and dest is too long to be useful
		        break;
		}

		rpath[roff] = v;
		roff++;

		if(v == net_id.addr)
			break;
	}

	if(roff < 2 || d[v] == INFINITY || roff >= MAX_SR_LEN) {
#ifdef DSR_CACHE_STATS
		stat.route_find_count += 1;
		if (for_me) stat.route_find_for_me += 1; 
		stat.route_find_miss_count += 1;
#endif
		return false; // no path
	}


#ifdef GOD_STABILITY
	/* The logic for God's omniscient expiration policy:
	 *   if we expire in realtime, kill dead links immediately, and
	 *     goto top. XXX this could create unnecessary overhead.
	 *   if we expire on the fly, we set the timeout to an appropriate
	 *     value, depending on whether or not the link is valid.
	 */
	if (lc_exppolicy == EXPPOLICY_GOD_OMNI) {
		int last = net_id.addr;
		for (v=1;v<roff;v++) {
			Link *l = findLink(last, rpath[roff-v-1]);
#ifdef REALTIME_EXPIRE
			if (God::instance()->hops(last, rpath[roff-v-1])!=1) {
				LIST_REMOVE(l, ln_link);
				delete l;
				dirty = CURRENT_TIME;
				return findRoute(dest, route, for_me);
			}
#else
			if (God::instance()->hops(last, rpath[roff-v-1])!=1) {
				l->ln_timeout = CURRENT_TIME;
			} else {
				l->ln_timeout = MAX_SIMTIME;
			}
#endif
			last = rpath[roff-v-1];
		}
	}
#endif //GOD_STABILITY 


#ifndef REALTIME_EXPIRE
	int last = net_id.addr;
	for (v=1;v<roff;v++) {
		Link *l = findLink(last, rpath[roff-v-1]);
		bool godsays = God::instance()->hops(last, rpath[roff-v-1])==1;
		last = rpath[roff-v-1];
		assert(l);
		if (l->expired()) {
			expirestats[CALLED_BAD + (godsays?IS_GOOD:IS_BAD)]++;
			LIST_REMOVE(l, ln_link);
			delete l;
			dirty = CURRENT_TIME;
			return findRoute(dest, route, for_me);
		} else {
			expirestats[CALLED_GOOD + (godsays?IS_GOOD:IS_BAD)]++;
		}
	}
#endif

	/*
	 * Build the path that we need...
	 */
	ID id;

	route.reset();

	id.addr = net_id.addr;  // source route is rooted at "us"
	id.type = ::IP;
	id.link_type = LT_NONE;
	id.log_stat = LS_NONE;
	route.appendToPath(id);

	for(v = 1; v < roff ; v++) {
		assert((int) (roff - v - 1) >= 0 && (roff - v - 1) < MAX_SR_LEN);

		Link *l = findLink(route[v-1].addr, rpath[roff - v - 1]);

		assert(l && !l->expired());

		if (for_me)
			l->ln_timeout += lc_gen_use_bonus;

		if (lc_exppolicy == EXPPOLICY_EXP) {
			exptable[route[v-1].addr] += lc_exp_add_amt;
			exptable[rpath[roff - v - 1]] += lc_exp_add_amt;
		}

		id.addr = rpath[roff - v - 1];
		id.type = ::IP;
		id.link_type = LT_NONE;
		id.log_stat = LS_NONE;
		route.appendToPath(id);

		route[v-1].t = l->ln_t;
		route[v-1].link_type = l->ln_linktype;
		route[v-1].log_stat = l->ln_logstat;
	}
	
#ifdef DSR_CACHE_STATS
	int bad = checkRoute_logall(&route, ACTION_FIND_ROUTE, 0);
	stat.route_find_count += 1; 
	if (for_me) stat.route_find_for_me += 1;     
	stat.route_find_bad_count += bad ? 1 : 0;
	stat.link_find_count += route.length() - 1;
	stat.link_find_bad_count +=  bad;
#endif
	return true;
}


void
LinkCache::Link_to_ID(Link *l, ID& id)
{
	id.addr = l->ln_dst;
	id.type = ::IP;
	id.t = l->ln_t;
	id.link_type = l->ln_linktype;
	id.log_stat = l->ln_logstat;
}

///////////////////////////////////////////////////////////////////////////

/*
 * Dijkstra's algorithm taken from CLR.
 */

void
LinkCache::init_single_source(int s)
{
	int v;

	for(v = 1; v <= LC_MAX_NODES; v++) {
		d[v] = INFINITY;
#ifdef LONGEST_LIVED_ROUTE
		dl[v] = 0; // dies immediately
#endif
		pi[v] = 0; // invalid node ID

		S[v] = false;
	}
	d[s] = 0;
#ifdef LONGEST_LIVED_ROUTE
	dl[s] = MAX_SIMTIME;
#endif
}

void
#ifdef LONGEST_LIVED_ROUTE
LinkCache::relax(u_int32_t u, u_int32_t v, u_int32_t w, double timeout)
#else
LinkCache::relax(u_int32_t u, u_int32_t v, u_int32_t w)
#endif
{
	assert(d[u] < INFINITY);
	assert(w == 1); /* make sure everything's working how I expect */

	if(d[v] > d[u] + w
#ifdef LONGEST_LIVED_ROUTE
	   || (d[v] == d[u] + w && dl[v] < dl[u] && dl[v] < timeout)
#endif
	   ) {
		d[v] = d[u] + w;
#ifdef LONGEST_LIVED_ROUTE
		dl[v] = (dl[u] > timeout) ? timeout : dl[u];
#endif
		pi[v] = u;
	}
}


int
LinkCache::extract_min_q()
{
	int u;
	int min_u = 0;

	for(u = 1; u <= LC_MAX_NODES; u++) {
		if(S[u] == false && d[u] < INFINITY && 
		   (min_u == 0 || d[u] < d[min_u] 
#ifdef LONGEST_LIVED_ROUTE
		    || (d[u] == d[min_u] && dl[u] > dl[min_u])
#endif
		   )) {
			min_u = u;
		}
	}
	return min_u; // (min_u == 0) ==> no valid link
}

void
LinkCache::dijkstra()
{
	int u;	
	Link *v;

	dirty = MAX_SIMTIME;  // all of the info is up-to-date

#ifdef REALTIME_EXPIRE
	for (u=1; u <= LC_MAX_NODES; u++) {
		v = lcache[u].lh_first;
		while (v) {
			if(v->ln_timeout <= CURRENT_TIME) {
				Link *tmp;
				tmp = v->ln_link.le_next;
				LIST_REMOVE(v, ln_link);
				delete v;
				v = tmp;
			} else {
				if (v->ln_timeout < dirty)
					dirty = v->ln_timeout;
				v = v->ln_link.le_next;
			}
		}
	}
#endif

	init_single_source(net_id.addr);

	while((u = extract_min_q()) != 0) {
		S[u] = true;
		v = lcache[u].lh_first;
		for( ; v; v = v->ln_link.le_next) {
			if(v->ln_flags & LINK_FLAG_UP) {
#ifdef LONGEST_LIVED_ROUTE
				relax(u, v->ln_dst, v->ln_cost, v->ln_timeout);
#else
				relax(u, v->ln_dst, v->ln_cost);
#endif
			}
		}
	}
}

void
LinkCache::dump_dijkstra(int dst)
{
	static char tbuf[512];
	u_int32_t u, toff = 0;

	bzero(tbuf, sizeof(tbuf));
	sprintf(tbuf, "SRC %.9f _%s_ dijkstra *%d* ",
		CURRENT_TIME, net_id.dump(), dst);
	toff = strlen(tbuf);

	for(u = 1; u <= LC_MAX_NODES; u++) {
		if(d[u] < INFINITY) {
			sprintf(tbuf+toff, "%d,%d,%d ", u, d[u], pi[u]);
			toff = strlen(tbuf);
			assert(toff < sizeof(tbuf));
		}
	}

	trace("%s", tbuf);
}
	

///////////////////////////////////////////////////////////////////////////

int
LinkCache::addLink(const ID& from, const ID& to,
		   int flags, double timeout, int cost)
{
	Link *l;
	int rc = 0;

	if((l = findLink(from.addr, to.addr)) == 0) {
		l = new Link(to.addr);
		assert(l);
		LIST_INSERT_HEAD(&lcache[from.addr], l, ln_link);
		dirty = CURRENT_TIME;
		l->ln_insert = CURRENT_TIME;
		rc = 1;
	} else if ((l->ln_flags & flags) != flags) {
		// we want to set flags
		dirty = CURRENT_TIME;
		l->ln_insert = CURRENT_TIME;
	}

	l->ln_t = CURRENT_TIME;
	l->ln_linktype = from.link_type;
	l->ln_flags |= flags;
	l->ln_cost = cost;
	l->ln_timeout = timeout;

	if(lc_bidirectional_links == false)
		return rc;

	/*
	 * Add the "other" direction of the link...
	 */
	if((l = findLink(to.addr, from.addr)) == 0) {
		l = new Link(from.addr);
		assert(l);
		l->ln_insert = CURRENT_TIME;
		LIST_INSERT_HEAD(&lcache[to.addr], l, ln_link);
		dirty = CURRENT_TIME;
	} else if ((l->ln_flags & flags) != flags) {
		// we want to set flags
		dirty = CURRENT_TIME;
		l->ln_insert = CURRENT_TIME;
	}
	l->ln_t = CURRENT_TIME;
	l->ln_linktype = from.link_type;
	l->ln_flags |= flags;
	l->ln_cost = cost;
	l->ln_timeout = timeout;

	return rc;
}

int
LinkCache::delLink(const ID& from, const ID& to)
{
	Link *l;
	int rc = 0;

	if((l = findLink(from.addr, to.addr))) {
		LIST_REMOVE(l, ln_link);
		delete l;
		dirty = CURRENT_TIME;
		rc = 1;
	}

	if(lc_bidirectional_links == false)
		return rc;

	/*
	 * Remove the "other" direction of the link...
	 */
	if((l = findLink(to.addr, from.addr))) {
		LIST_REMOVE(l, ln_link);
		delete l;
		dirty = CURRENT_TIME;
	}

	return rc;
}

Link*
LinkCache::findLink(int from, int to)
{
	Link *l;

	for(l = lcache[from].lh_first; l; l = l->ln_link.le_next) {
		if(l->ln_dst == to) {
			return l;
		}
	}
	return 0;
}

void
LinkCache::purgeLink()
{
	int u;
	Link *l;

	for(u = 1; u <= LC_MAX_NODES; u++) {
		if(d[u] == INFINITY) {
			ID from, to;

			from.addr = u;
			from.type = ::IP;
			from.link_type = LT_NONE;
			from.log_stat = LS_NONE;

			l = lcache[u].lh_first;
			for( ; l; l = l->ln_link.le_next) {
				Link_to_ID(l, to);
#ifdef DSR_CACHE_STATS
				checkLink_logall(from, to, ACTION_EVICT);
#endif
				(void) delLink(from, to);
			}
		}
	}
}


void
LinkCache::dumpLink()
{
	Link *l;
	int i;
	static char tbuf[512];
	u_int32_t toff = 0;

	bzero(tbuf, sizeof(tbuf));
	sprintf(tbuf, "SRC %.9f _%s_ dump-link ", CURRENT_TIME, net_id.dump());
	toff = strlen(tbuf);

	for(i = 1; i <= LC_MAX_NODES; i++) {
		for(l = lcache[i].lh_first; l; l = l->ln_link.le_next) {
			sprintf(tbuf+toff, "%d->%d, ", i, l->ln_dst);
			toff = strlen(tbuf);
			assert(toff < sizeof(tbuf));
		}
	}

	trace("%s", tbuf);
}

#ifdef DSR_CACHE_STATS

/*
 * Called only from "purgeLink"
 */
void
LinkCache::checkLink_logall(const ID& from, const ID& to, int action)
{
	Link *l = findLink(from.addr, to.addr);

	assert(action == ACTION_EVICT);
	assert(l);

	if(God::instance()->hops(from.addr, to.addr) != 1) {
		trace("SRC %.9f _%s_ %s [%d %d] %s->%s dead %d %.9f",
		      CURRENT_TIME, net_id.dump(),
		      action_name[action], 0, 0,
		      from.dump(), to.dump(),
		      l->ln_linktype, l->ln_t);
	}
}

void
LinkCache::checkLink(const ID& from, const ID& to, int action)
{
	Link *l = findLink(from.addr, to.addr);
	int alive = 0;

	assert(l);

	if(God::instance()->hops(from.addr, to.addr) != 1) {
		if(l->ln_logstat == LS_UNLOGGED) {
			trace("SRC %.9f _%s_ %s [%d %d] %s->%s dead %d %.9f",
				CURRENT_TIME, net_id.dump(),
				action_name[action], 0, 0,
				from.dump(), to.dump(),
				l->ln_linktype, l->ln_t);
			l->ln_logstat = LS_LOGGED;
		}
	} else {
		alive = 1;
		if(l->ln_logstat == LS_LOGGED) {
			trace("SRC %.9f _%s_ resurrected-link [%d %d] %s->%s dead %d %.9f",
				CURRENT_TIME, net_id.dump(), 0, 0,
				from.dump(), to.dump(),
				l->ln_linktype, l->ln_t);
			l->ln_logstat = LS_UNLOGGED;
		}
        }

	switch(action) {
	case ACTION_ADD_ROUTE:
		stat.link_add_count += 1;
		if(alive == 0){
		        stat.link_add_bad_count += 1;
		}
		if(l->ln_linktype == LT_TESTED)
			stat.link_add_tested += 1;
		break;

	case ACTION_NOTICE_ROUTE:
		stat.link_notice_count += 1;
		if(alive == 0){
			stat.link_notice_bad_count += 1;
		}
		if(l->ln_linktype == LT_TESTED)
			stat.link_notice_tested += 1;
		break;
	}
}

void
LinkCache::periodic_checkCache()
{
  int c;
  int link_count = 0;
  int link_bad_count = 0;
  double link_bad_time = 0.0;
  int link_bad_tested = 0;
  int link_good_tested = 0;

#ifndef REALTIME_EXPIRE
  if (ID(1,::IP) == net_id) 
    trace("SRC %.9f _%s_ cache-expire-bits %d %d %d %d",
	  CURRENT_TIME, net_id.dump(), expirestats[0], expirestats[1], 
	  expirestats[2], expirestats[3]);
#endif

  for(c = 1; c <= LC_MAX_NODES; c++) {
	Link *v = lcache[c].lh_first;
	for( ; v; v = v->ln_link.le_next) {
		link_count += 1;
		if(God::instance()->hops(c, v->ln_dst) != 1) {
			link_bad_count += 1;
			link_bad_time += CURRENT_TIME - v->ln_t;
			if(v->ln_linktype == LT_TESTED)
				link_bad_tested += 1;
		} else {
			if(v->ln_linktype == LT_TESTED)
				link_good_tested += 1;
		}
	}
  }

  trace("SRC %.9f _%s_ cache-summary %d %d %d %d | %d %.9f %d %d | %d %d %d %d %d | %d %d %d %d %d | %d %d %d %d %d %d",
	CURRENT_TIME, net_id.dump(),
        0, //route_count,
        0, // route_bad_count,
        link_count, // subroute_count,
        0, // subroute_bad_count,

        link_bad_count,
        link_bad_count ? link_bad_time/link_bad_count : 0.0,
        link_bad_tested,
        link_good_tested,

        stat.route_add_count,     // always 0 -- not increment
        stat.route_add_bad_count, // always 0 -- not increment
        stat.link_add_count, // stat.subroute_add_count,
        stat.link_add_bad_count, // stat.subroute_add_bad_count,
        stat.link_add_tested,
             
        stat.route_notice_count,
        stat.route_notice_bad_count,
        stat.link_notice_count, // stat.subroute_notice_count,
        stat.link_notice_bad_count, // stat.subroute_notice_bad_count,
        stat.link_notice_tested,
             
        stat.route_find_count,
	stat.route_find_for_me,
        stat.route_find_bad_count,
        stat.route_find_miss_count,
        stat.subroute_find_count,
        stat.subroute_find_bad_count);
  stat.reset();
}

#endif
#endif //DSR_LINKCACHE
