/*
 *
 * Copyright (c) Xerox Corporation 1998. All rights reserved.
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
 * $Header: /cvsroot/nsnam/ns-2/webcache/http.h,v 1.14 2005/08/26 05:05:31 tomh Exp $
 *
 */
//
// Definition of the HTTP agent
// 

#ifndef ns_http_h
#define ns_http_h

#include <stdlib.h>
#include <tcl.h>
#include "config.h"
#include "agent.h"
#include "ns-process.h"
#include "app.h"

#include "pagepool.h"
#include "inval-agent.h"
#include "tcpapp.h"
#include "http-aux.h"

class HttpApp : public Process {
public:
	HttpApp();
	virtual ~HttpApp();

	virtual int command(int argc, const char*const* argv);
	void log(const char *fmt, ...);
	int id() const { return id_; }

	virtual void process_data(int size, AppData* d);
	virtual AppData* get_data(int&, AppData*) {
		// Do not support it
		abort();
		return NULL;
	}

protected:
	int add_cnc(HttpApp *client, TcpApp *agt);
	void delete_cnc(HttpApp *client);
	TcpApp* lookup_cnc(HttpApp *client);

	void set_pagepool(ClientPagePool* pp) { pool_ = pp; }

	Tcl_HashTable *tpa_;	// TcpApp hash table
	int id_;		// Node id
	ClientPagePool *pool_;	// Page repository
	Tcl_Channel log_;	// Log file descriptor
};


//----------------------------------------------------------------------
// Servers
//----------------------------------------------------------------------
class HttpServer : public HttpApp {
public: 
	// All methods are in TCL
};

class HttpInvalServer : public HttpServer {
public:
};

// Http server with periodic unicast heartbeat invalidations
class HttpYucInvalServer : public HttpInvalServer {
public:
	HttpYucInvalServer();

	virtual int command(int argc, const char*const* argv);
	void add_inv(const char *name, double mtime);

protected:
	// heartbeat methods
	HttpUInvalAgent *inv_sender_; // Heartbeat/invalidation sender
	InvalidationRec *invlist_; 
	int num_inv_;

	void send_heartbeat();
	HttpHbData* pack_heartbeat();
	virtual void send_hb_helper(int size, AppData *data);
	InvalidationRec* get_invrec(const char *name);

	int Ca_, Cb_, push_thresh_, enable_upd_;
	int push_high_bound_, push_low_bound_;
	double hb_interval_;		// Heartbeat interval (second)
};



//----------------------------------------------------------------------
// Clients
//----------------------------------------------------------------------

// Place holder: everything is in OTcl. We declare it as a split object
// in case that its derived classes need some C++ handling.
class HttpClient : public HttpApp {
};



//----------------------------------------------------------------------
// Caches
//----------------------------------------------------------------------

class HttpCache : public HttpApp {
};

class HttpInvalCache : public HttpCache {
};

// Invalidations embedded in periodic heartbeats
// Used by recv_inv() and recv_inv_filter() to filter invalidations.
const int HTTP_INVALCACHE_FILTERED 	= 0;
const int HTTP_INVALCACHE_UNFILTERED 	= 1;

// Http cache with periodic multicast heartbeat invalidation
class HttpMInvalCache : public HttpInvalCache {
public:
	HttpMInvalCache();
	virtual ~HttpMInvalCache();

	virtual int command(int argc, const char*const* argv);
	virtual void process_data(int size, AppData* data);
	virtual void timeout(int reason);

	void handle_node_failure(int cid);
	void invalidate_server(int sid);
	void add_inv(const char *name, double mtime);

protected:
	HBTimer hb_timer_;		// Heartbeat/Inval timer
	HttpInvalAgent **inv_sender_;	// Heartbeat/Inval sender agents
	int num_sender_;		// # of heartbeat sender agents
	int size_sender_;		// Maximum size of array inv_sender_
	InvalidationRec *invlist_;	// All invalidations to be sent
	int num_inv_;			// # of invalidations in invlist_

	void send_heartbeat();
	HttpHbData* pack_heartbeat();
	virtual void send_hb_helper(int size, AppData *data);

	int recv_inv(HttpHbData *d);
	virtual void process_inv(int n, InvalidationRec *ivlist, int cache);
	virtual int recv_inv_filter(ClientPage* pg, InvalidationRec *p) {
		return ((pg == NULL) || (pg->mtime() >= p->mtime()) ||
			!pg->is_valid()) ? 
			HTTP_INVALCACHE_FILTERED : HTTP_INVALCACHE_UNFILTERED;
	}
	InvalidationRec* get_invrec(const char *name);

	// Maintaining SState(Server, NextCache)
	// Use the shadow names of server and cache as id
	struct SState {
		SState(NeighborCache* c) : down_(0), cache_(c) {}
		int is_down() { return down_; }
		void down() { down_ = 1; }
		void up() { down_ = 0; }
		NeighborCache* cache() { return cache_; }
		int down_;		// If the server is disconnected
		NeighborCache *cache_;	// NextCache
	};

	Tcl_HashTable sstate_;
	void add_sstate(int sid, SState* sst);
	SState* lookup_sstate(int sid);
	// check & establish sstate
	void check_sstate(int sid, int cid); 

	// Maintaining liveness of neighbor caches
	Tcl_HashTable nbr_;
	void add_nbr(HttpMInvalCache* c);
	NeighborCache* lookup_nbr(int id);

	HttpUInvalAgent *inv_parent_;	// Heartbeat/Inval to parent cache
	
	void recv_heartbeat(int id);
	void recv_leave(HttpLeaveData *d);
	void send_leave(HttpLeaveData *d);

	double hb_interval_;		// Heartbeat interval (second)
	int enable_upd_;		// Whether enable push
	int Ca_, Cb_, push_thresh_;
	int push_high_bound_, push_low_bound_;

	HttpInvalAgent **upd_sender_;	// Agents to push updates to
	int num_updater_;		// # number of update agents
	int size_updater_;		// Size of array upd_sender_

	void add_update(const char *name, double mtime);
	void send_upd(ClientPage *pg);
	int recv_upd(HttpUpdateData *d);
	virtual void send_upd_helper(int pgsize, AppData* data);
	HttpUpdateData* pack_upd(ClientPage *pg);

	// Use a static mapping to convert cache id to cache pointers
	static HttpMInvalCache** CacheRepository_;
	static int NumCache_;
	static void add_cache(HttpMInvalCache* c);
	static HttpMInvalCache* map_cache(int id) {
		return CacheRepository_[id];
	}
};

//----------------------------------------------------------------------
// Multicast invalidation + two way liveness messages + 
// invalidation filtering. 
//----------------------------------------------------------------------
class HttpPercInvalCache : virtual public HttpMInvalCache {
public:
	HttpPercInvalCache();
	int command(int argc, const char*const* argv);

protected: 
	virtual int recv_inv_filter(ClientPage *pg, InvalidationRec *ir) {
		// If we already have an invalid page, don't forward the
		// invalidation any more.
		return ((pg == NULL) || (pg->mtime() >= ir->mtime()) ||
			!pg->is_header_valid()) ? 
			HTTP_INVALCACHE_FILTERED : HTTP_INVALCACHE_UNFILTERED;
	}

	// Flag: if we allow direct request, and hence pro formas
	int direct_request_;
};

#endif // ns_http_h
