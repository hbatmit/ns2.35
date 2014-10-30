
/*
 * routecache.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: routecache.cc,v 1.8 2006/02/21 15:20:18 mahrenho Exp $
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

/*
 * Ported from CMU/Monarch's code, appropriate copyright applies.  
 * routecache.cc
 *   handles routes
 */

extern "C" {
#include <stdio.h>
#include <stdarg.h>
}



#undef DEBUG

#include "path.h"
#include "routecache.h"
#include <god.h>

static const int verbose_debug = 0;

/*===============================================================
  OTcl definition
----------------------------------------------------------------*/
static class RouteCacheClass : public TclClass {
public:
        RouteCacheClass() : TclClass("RouteCache") {}
        TclObject* create(int, const char*const*) {
                return makeRouteCache();
        }
} class_RouteCache;

/*===============================================================
 Constructors
----------------------------------------------------------------*/
RouteCache::RouteCache():
#ifdef DSR_CACHE_STATS
	mh(this),
#endif
	MAC_id(invalid_addr), net_id(invalid_addr)
{
#ifdef DSR_CACHE_STATS
	stat.reset();
#endif
}

RouteCache::~RouteCache()
{
}

int
RouteCache::command(int argc, const char*const* argv)
{
  if(argc == 2)
    {
      if(strcasecmp(argv[1], "startdsr") == 0)
	{
#ifdef DSR_CACHE_STATS
	  mh.start();
#endif
	  return TCL_OK;
	}
    }
  else if(argc == 3) 
    {    
      if (strcasecmp(argv[1], "ip-addr") == 0) {
	net_id = ID(atoi(argv[2]), ::IP);
	return TCL_OK;
      }
      else if(strcasecmp(argv[1], "mac-addr") == 0) {
	MAC_id = ID(atoi(argv[2]), ::MAC);
      return TCL_OK;
      }
      
      /* must be a looker up */
      TclObject *obj = (TclObject*) TclObject::lookup(argv[2]);
      if(obj == 0)
	return TCL_ERROR;
      
      if(strcasecmp(argv[1], "log-target") == 0 || 
	 strcasecmp(argv[1], "tracetarget") == 0) {
	logtarget = (Trace*) obj;
	return TCL_OK;
      }

      return TCL_ERROR;
    }
  
  return TclObject::command(argc, argv);
}

void
RouteCache::trace(char* fmt, ...)
{
  va_list ap;
  
  if (!logtarget) return;

  va_start(ap, fmt);
  vsprintf(logtarget->pt_->buffer(), fmt, ap);
  logtarget->pt_->dump();
  va_end(ap);
}


void
RouteCache::dump(FILE *out)
{
  fprintf(out, "Route cache dump unimplemented\n");
  fflush(out);
}

///////////////////////////////////////////////////////////////////////////

extern bool cache_ignore_hints;
extern bool cache_use_overheard_routes;

#define STOP_PROCESSING 0
#define CONT_PROCESSING 1

int
RouteCache::pre_addRoute(const Path& route, Path& rt,
			 Time t, const ID& who_from)
{
  assert(!(net_id == invalid_addr));
  
  if (route.length() < 2)
    return STOP_PROCESSING; // we laugh in your face

  if(verbose_debug)
    trace("SRC %.9f _%s_ adding rt %s from %s",
	  Scheduler::instance().clock(), net_id.dump(),
	  route.dump(), who_from.dump());

  if (route[0] != net_id && route[0] != MAC_id) 
    {
      fprintf(stderr,"%.9f _%s_ adding bad route to cache %s %s\n",
	      t, net_id.dump(), who_from.dump(), route.dump());
      return STOP_PROCESSING;
    }
	     
  rt = (Path&) route;	// cast away const Path&
  rt.owner() = who_from;

  Link_Type kind = LT_TESTED;
  for (int c = 0; c < rt.length(); c++)
    {
      rt[c].log_stat = LS_UNLOGGED;
      if (rt[c] == who_from) kind = LT_UNTESTED; // remaining ids came from $
      rt[c].link_type = kind;
      rt[c].t = t;
    }

  return CONT_PROCESSING;
}

int
RouteCache::pre_noticeRouteUsed(const Path& p, Path& stub,
				Time t, const ID& who_from)
{
  int c;
  bool first_tested = true;

  if (p.length() < 2)
	  return STOP_PROCESSING;
  if (cache_ignore_hints == true)
	  return STOP_PROCESSING;

  for (c = 0; c < p.length() ; c++) {
	  if (p[c] == net_id || p[c] == MAC_id) break;
  }

  if (c == p.length() - 1)
	  return STOP_PROCESSING; // path contains only us

  if (c == p.length()) { // we aren't in the path...
	  if (cache_use_overheard_routes) {
		  // assume a link from us to the person who
		  // transmitted the packet
		  if (p.index() == 0) {
			   /* must be a route request */
			  return STOP_PROCESSING;
		  }

		  stub.reset();
		  stub.appendToPath(net_id);
		  int i = p.index() - 1;
		  for ( ; i < p.length() && !stub.full() ; i++) {
			  stub.appendToPath(p[i]);
		  }
		  // link to xmiter might be unidirectional
		  first_tested = false;
	  }
	  else {
		  return STOP_PROCESSING;
	  }
  }
  else { // we are in the path, extract the subpath
	  CopyIntoPath(stub, p, c, p.length() - 1);
  }

  Link_Type kind = LT_TESTED;
  for (c = 0; c < stub.length(); c++) {
	  stub[c].log_stat = LS_UNLOGGED;

	   // remaining ids came from $
	  if (stub[c] == who_from)
		  kind = LT_UNTESTED;
	  stub[c].link_type = kind;
	  stub[c].t = t;
  }
  if (first_tested == false)
	  stub[0].link_type = LT_UNTESTED;

  return CONT_PROCESSING;
}

#undef STOP_PROCESSING
#undef CONT_PROCESSING

//////////////////////////////////////////////////////////////////////

#ifdef DSR_CACHE_STATS

void
MobiHandler::handle(Event *) {
        cache->periodic_checkCache();
        Scheduler::instance().schedule(this, &intr, interval);
}

int
RouteCache::checkRoute_logall(Path *p, int action, int start)
{
  int c;
  int subroute_bad_count = 0;

  if(p->length() == 0)
    return 0;
  assert(p->length() >= 2);

  assert(action == ACTION_DEAD_LINK ||
         action == ACTION_EVICT ||
         action == ACTION_FIND_ROUTE);

  for (c = start; c < p->length() - 1; c++)
    {
      if (God::instance()->hops((*p)[c].getNSAddr_t(), (*p)[c+1].getNSAddr_t()) != 1)
	{
          trace("SRC %.9f _%s_ %s [%d %d] %s->%s dead %d %.9f",
                Scheduler::instance().clock(), net_id.dump(),
                action_name[action], p->length(), c,
                (*p)[c].dump(), (*p)[c+1].dump(),
                (*p)[c].link_type, (*p)[c].t);

          if(subroute_bad_count == 0)
            subroute_bad_count = p->length() - c - 1;
	}
    }
  return subroute_bad_count;
}

#endif /* DSR_CACHE_STAT */

