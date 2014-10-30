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
 * $Header: /cvsroot/nsnam/ns-2/webcache/http.cc,v 1.22 2009/12/30 22:06:34 tom_henderson Exp $
 *
 */
//
// Implementation of the HTTP agent. We want a separate agent for HTTP because
// we are interested in (detailed) HTTP headers, instead of just request and 
// response patterns.
//

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>

#include "tclcl.h"
#include "agent.h"
#include "app.h"
#include "tcp-simple.h"
#include "http.h"
#include "http-aux.h"
#include "trace.h"
#include "tcpapp.h"
#include "mcache.h"

//----------------------------------------------------------------------
// Http Application
//
// Allows multiple concurrent HTTP connections
//----------------------------------------------------------------------
static class HttpAppClass : public TclClass {
public:
        HttpAppClass() : TclClass("Http") {}
        TclObject* create(int, const char*const*) {
		return (new HttpApp());
	}
} class_http_app;

// What states should be in a http agent?
HttpApp::HttpApp() : log_(0)
{
	bind("id_", &id_);
	// Map a client address to a particular TCP agent
	tpa_ = new Tcl_HashTable;
	Tcl_InitHashTable(tpa_, TCL_ONE_WORD_KEYS);
}

HttpApp::~HttpApp()
{
	if (tpa_ != NULL) {
		Tcl_DeleteHashTable(tpa_);
		delete tpa_;
	}
}

int HttpApp::add_cnc(HttpApp* client, TcpApp *agt)
{
	int newEntry = 1;
	long key = client->id();
	Tcl_HashEntry *he = Tcl_CreateHashEntry(tpa_, 
						(const char *) key,
						&newEntry);
	if (he == NULL) 
		return -1;
	if (newEntry)
		Tcl_SetHashValue(he, (ClientData)agt);
	return 0;
}

void HttpApp::delete_cnc(HttpApp* client)
{
        long key = client->id();
	Tcl_HashEntry *he = Tcl_FindHashEntry(tpa_,(const char *)key);
	if (he != NULL) {
		TcpApp *cnc = (TcpApp *)Tcl_GetHashValue(he);
		Tcl_DeleteHashEntry(he);
		delete cnc;
	}
}

TcpApp* HttpApp::lookup_cnc(HttpApp* client)
{
        long key = client->id();
	Tcl_HashEntry *he = 
		Tcl_FindHashEntry(tpa_, (const char *)key);
	if (he == NULL)
		return NULL;
	return (TcpApp *)Tcl_GetHashValue(he);
}

// Basic functionalities: 
int HttpApp::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();

	if (argc == 2) {
		if (strcmp(argv[1], "id") == 0) {
			if (argc == 3) {
				id_ = atoi(argv[2]);
				tcl.resultf("%d", id_);
			} else
				tcl.resultf("%d", id_);
			return TCL_OK;
		} else if (strcmp(argv[1], "log") == 0) {
			// Return the name of the log channel
			if (log_ != NULL)
				tcl.resultf("%s", Tcl_GetChannelName(log_));
			else
				tcl.result("");
			return TCL_OK;
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "get-modtime") == 0) {
			double mt;
			if (pool_->get_mtime(argv[2], mt) != -1) {
				tcl.resultf("%.17g", mt);
				return TCL_OK;
			} else 
				return TCL_ERROR;
		} else if (strcmp(argv[1], "exist-page") == 0) { 
			tcl.resultf("%d", pool_->exist_page(argv[2]));
			return TCL_OK;
		} else if (strcmp(argv[1], "get-size") == 0) {
			int size;
			if (pool_->get_size(argv[2], size) != -1) {
				tcl.resultf("%d", size);
				return TCL_OK;
			} else 
				return TCL_ERROR;
		} else if (strcmp(argv[1], "get-age") == 0) {
			double age;
			if (pool_->get_age(argv[2], age) != -1) {
				tcl.resultf("%.17g", age);
				return TCL_OK;
			} else 
				return TCL_ERROR;
		} else if (strcmp(argv[1], "get-cachetime") == 0) {
			double et;
			if (pool_->get_etime(argv[2], et) != -1) {
				tcl.resultf("%.17g", et);
				return TCL_OK;
			} else 
				return TCL_ERROR;
		} else if (strcmp(argv[1], "get-page") == 0) {
			char buf[4096];
			if (pool_->get_pageinfo(argv[2], buf) != -1) {
				tcl.resultf("%s", buf);
				return TCL_OK;
			} else 
				return TCL_ERROR;
		} else if (strcmp(argv[1], "get-cnc") == 0) {
			/*
			 * <http> get-cnc <client>
			 *
			 * Given the communication party, get the tcp agent 
			 * connected to it.
			 */
			HttpApp *client = 
				(HttpApp *)TclObject::lookup(argv[2]);
			TcpApp *cnc = (TcpApp *)lookup_cnc(client);
			if (cnc == NULL)
				tcl.result("");
			else 
				tcl.resultf("%s", cnc->name());
			return TCL_OK;

		} else if (strcmp(argv[1], "set-pagepool") == 0) {
			pool_ = (ClientPagePool*)TclObject::lookup(argv[2]);
			if (pool_ != NULL) 
				return TCL_OK;
			else 
				return TCL_ERROR;
		} else if (strcmp(argv[1], "is-connected") == 0) {
			/*
			 * <http> is-connected <server>
			 */
			HttpApp *a = (HttpApp*)TclObject::lookup(argv[2]);
			TcpApp *cnc = (TcpApp*)lookup_cnc(a);
			if (cnc == NULL) 
				tcl.result("0");
			else 
				tcl.result("1");
			return TCL_OK;
		} else if (strcmp(argv[1], "is-valid") == 0) {
			ClientPage *pg = 
				(ClientPage *)pool_->get_page(argv[2]);
			if (pg == NULL) {
				tcl.resultf("%d is-valid: No page %s", 
					    id_, argv[2]);
				return TCL_ERROR;
			}
			tcl.resultf("%d", pg->is_valid());
			return TCL_OK;
		} else if (strcmp(argv[1], "log") == 0) {
			int mode;
			log_ = Tcl_GetChannel(tcl.interp(), 
					      (char*)argv[2], &mode);
			if (log_ == 0) {
				tcl.resultf("%d: invalid log file handle %s\n",
					    id_, argv[2]);
				return TCL_ERROR;
			}
			return TCL_OK;
		} else if (strcmp(argv[1], "disconnect") == 0) {
			/*
			 * <http> disconnect <client> 
			 * Delete the association of source and sink TCP.
			 */
			HttpApp *client = 
				(HttpApp *)TclObject::lookup(argv[2]);
			delete_cnc(client);
			return TCL_OK;
		} else if (strcmp(argv[1], "get-pagetype") == 0) {
			/*
			 * <http> get-pagetype <pageid>
			 * return the page type
			 */
			ClientPage *pg = 
				(ClientPage*)pool_->get_page(argv[2]);
			if (pg == NULL) {
				tcl.resultf("%d get-pagetype: No page %s", 
					    id_, argv[2]);
				return TCL_ERROR;
			}
			switch (pg->type()) {
			case HTML:
				tcl.result("HTML");
				break;
			case MEDIA:
				tcl.result("MEDIA");
				break;
			default:
				fprintf(stderr, "Unknown page type %d", 
					pg->type());
				return TCL_ERROR;
			}
			return TCL_OK;
		} else if (strcmp(argv[1], "get-layer") == 0) {
			// Assume the page is a MediaPage
			MediaPage *pg = (MediaPage *)pool_->get_page(argv[2]);
			if (pg == NULL) {
				tcl.resultf("%d get-layer: No page %s", 
					    id_, argv[2]);
				return TCL_ERROR;
			}
			if (pg->type() != MEDIA) {
				tcl.resultf("%d get-layer %s not a media page",
					    id_, argv[2]);
				return TCL_ERROR;
			}
			tcl.resultf("%d", pg->num_layer());
			return TCL_OK;
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "connect") == 0) {
			/*
			 * <http> connect <client> <ts>
			 *
			 * Associate a TCP agent with the given client. 
			 * <ts> is the agent used to send packets out.
			 * We assume two-way TCP connection, therefore we 
			 * only need one agent.
			 */
			HttpApp *client = 
				(HttpApp *)TclObject::lookup(argv[2]);
			TcpApp *cnc = (TcpApp *)TclObject::lookup(argv[3]);
			if (add_cnc(client, cnc)) {
				tcl.resultf("%s: failed to connect to %s", 
					    name_, argv[2]);
				return TCL_ERROR;
			}
			// Set data delivery target
			cnc->target() = (Process*)this;
			return TCL_OK;
		} else if (strcmp(argv[1], "set-modtime") == 0) {
			double mt = strtod(argv[3], NULL);
			if (pool_->set_mtime(argv[2], mt) != -1)
				return TCL_OK;
			else 
				return TCL_ERROR;
		} else if (strcmp(argv[1], "set-cachetime") == 0) {
			double et = Scheduler::instance().clock();
			if (pool_->set_etime(argv[2], et) != -1)
				return TCL_OK;
			else 
				return TCL_ERROR;
		}
	} else {
		if (strcmp(argv[1], "send") == 0) {
			/*
			 * <http> send <client> <bytes> <callback> 
			 */
			HttpApp *client = 
				(HttpApp *)TclObject::lookup(argv[2]);
			if (client == NULL) {
				tcl.add_errorf("%s: bad client name %s",
					       name_, argv[2]);
				return TCL_ERROR;
			}
			int bytes = atoi(argv[3]);
			TcpApp *cnc = (TcpApp *)lookup_cnc(client);
			if (cnc == NULL) {
				//tcl.resultf("%s: no connection to client %s",
				//	    name_, argv[2]);
				// Tolerate it
				return TCL_OK;
			}
			char *buf = strdup(argv[4]);
			HttpNormalData *d = 
				new HttpNormalData(id_, bytes, buf);
			cnc->send(bytes, d);
			// delete d;
			free(buf);
			return TCL_OK;
		
		} else if (strcmp(argv[1], "enter-page") == 0) {
			ClientPage* pg = pool_->enter_page(argc, argv);
			if (pg == NULL)
				return TCL_ERROR;
			else 
				return TCL_OK;

		} else if (strcmp(argv[1], "evTrace") == 0) { 
			char buf[1024], *p;
			if (log_ != 0) {
				sprintf(buf, TIME_FORMAT" i %d ", 
				  BaseTrace::round(Scheduler::instance().clock()), 
					id_);
				p = &(buf[strlen(buf)]);
				for (int i = 2; i < argc; i++) {
					strcpy(p, argv[i]);
					p += strlen(argv[i]);
					*(p++) = ' ';
				}
				// Stick in a newline.
				*(p++) = '\n', *p = 0;
				Tcl_Write(log_, buf, p-buf);
			}
			return TCL_OK;
		}
	}

	return TclObject::command(argc, argv);
}

void HttpApp::log(const char* fmt, ...)
{
	// Don't do anything if we don't have a log file.
	if (log_ == 0) 
		return;

	char buf[10240], *p;
	sprintf(buf, TIME_FORMAT" i %d ", 
		BaseTrace::round(Scheduler::instance().clock()), id_);
	p = &(buf[strlen(buf)]);
	va_list ap;
	va_start(ap, fmt);
	vsprintf(p, fmt, ap);
	Tcl_Write(log_, buf, strlen(buf));
}

void HttpApp::process_data(int, AppData* data)
{
	if (data == NULL) 
		return;

	switch (data->type()) {
	case HTTP_NORMAL: {
		HttpNormalData *tmp = (HttpNormalData*)data;
		Tcl::instance().eval(tmp->str());
		break;
	}
	default:
		fprintf(stderr, "Bad http invalidation data type %d\n", 
			data->type());
		abort();
		break;
	}
}



//----------------------------------------------------------------------
// Clients
//----------------------------------------------------------------------
static class HttpClientClass : public TclClass {
public:
	HttpClientClass() : TclClass("Http/Client") {}
        TclObject* create(int, const char*const*) {
		return (new HttpClient());
	}
} class_httpclient_app;



//----------------------------------------------------------------------
// Servers
//----------------------------------------------------------------------
static class HttpServerClass : public TclClass {
public:
        HttpServerClass() : TclClass("Http/Server") {}
        TclObject* create(int, const char*const*) {
		return (new HttpServer());
	}
} class_httpserver_app;

static class HttpInvalServerClass : public TclClass {
public:
        HttpInvalServerClass() : TclClass("Http/Server/Inval") {}
        TclObject* create(int, const char*const*) {
		return (new HttpInvalServer());
	}
} class_httpinvalserver_app;

static class HttpYucInvalServerClass : public TclClass {
public:
        HttpYucInvalServerClass() : TclClass("Http/Server/Inval/Yuc") {}
        TclObject* create(int, const char*const*) {
		return (new HttpYucInvalServer());
	}
} class_httpyucinvalserver_app;

HttpYucInvalServer::HttpYucInvalServer() :
	inv_sender_(0), invlist_(0), num_inv_(0)
{
	bind("hb_interval_", &hb_interval_);
	bind("enable_upd_", &enable_upd_);
	bind("Ca_", &Ca_);
	bind("Cb_", &Cb_);
	bind("push_thresh_", &push_thresh_);
	bind("push_high_bound_", &push_high_bound_);
	bind("push_low_bound_", &push_low_bound_);
}

int HttpYucInvalServer::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();

	switch (argv[1][0]) {
	case 'a': 
		if (strcmp(argv[1], "add-inval-sender") == 0) {
			HttpUInvalAgent *tmp = 
				(HttpUInvalAgent *)TclObject::lookup(argv[2]);
			if (tmp == NULL) {
				tcl.resultf("Non-existent agent %s", argv[2]);
				return TCL_ERROR;
			}
			inv_sender_ = tmp;
			return TCL_OK;
		} if (strcmp(argv[1], "add-inv") == 0) {
			/*
			 * <server> add-inv <pageid> <modtime>
			 */
			double mtime = strtod(argv[3], NULL);
			add_inv(argv[2], mtime);
			return TCL_OK;
		}
		break;
	case 'c': 
		if (strcmp(argv[1], "count-request") == 0) {
			ClientPage *pg = 
				(ClientPage *)pool_->get_page(argv[2]);
			if (pg == NULL) {
				tcl.resultf("%d count-request: No page %s", 
					    id_, argv[2]);
				return TCL_ERROR;
			}
			pg->count_request(Cb_, push_high_bound_);
			log("S NTF p %s v %d\n", argv[2], pg->counter());
			return TCL_OK;
		} else if (strcmp(argv[1], "count-inval") == 0) {
			ClientPage *pg = 
				(ClientPage *)pool_->get_page(argv[2]);
			if (pg == NULL) {
				tcl.resultf("%d count-inval: No page %s", 
					    id_, argv[2]);
				return TCL_ERROR;
			}
			pg->count_inval(Ca_, push_low_bound_);
			log("S NTF p %s v %d\n", argv[2], pg->counter());
			return TCL_OK;
		} 
		break;
	case 'i': 
		if (strcmp(argv[1], "is-pushable") == 0) {
			ClientPage *pg = 
				(ClientPage *)pool_->get_page(argv[2]);
			if (pg == NULL) {
				tcl.resultf("%d is-pushable: No page %s", 
					    id_, argv[2]);
				return TCL_ERROR;
			}
			if (pg->is_mpush() && 
			    (Scheduler::instance().clock() - pg->mpush_time() >
			     HTTP_HBEXPIRE_COUNT*hb_interval_)) {
				// If mandatory push timer expires, stop push
				pg->clear_mpush();
				fprintf(stderr, 
					"server %d timeout mpush\n", id_);
			}
			tcl.resultf("%d", ((enable_upd_ && 
					   (pg->counter() >= push_thresh_)) ||
					   pg->is_mpush()));
			return TCL_OK;
		}
		break;
	case 'r': 
		if ((strcmp(argv[1], "request-mpush") == 0) ||
		    (strcmp(argv[1], "refresh-mpush") == 0)) {
			ClientPage *pg = 
				(ClientPage *)pool_->get_page(argv[2]);
			if (pg == NULL) {
				tcl.resultf("%d is-valid: No page %s", 
					    id_, argv[2]);
			return TCL_ERROR;
			}
			pg->set_mpush(Scheduler::instance().clock());
			return TCL_OK;
		} 
		break;
	case 's': 
		if (strcmp(argv[1], "send-hb") == 0) {
			send_heartbeat();
			return TCL_OK;
		} else if (strcmp(argv[1], "stop-mpush") == 0) {
			ClientPage *pg = 
				(ClientPage *)pool_->get_page(argv[2]);
			if (pg == NULL) {
				tcl.resultf("%d is-valid: No page %s", 
					    id_, argv[2]);
				return TCL_ERROR;
			}
		pg->clear_mpush();
		fprintf(stderr, "server %d stopped mpush\n", id_);
		return TCL_OK;
		}
		break;
	}

	return HttpApp::command(argc, argv);
}

void HttpYucInvalServer::add_inv(const char *name, double mtime)
{
	InvalidationRec *p = get_invrec(name);
	if ((p != NULL) && (p->mtime() < mtime)) {
		p->detach();
		delete p;
		p = NULL;
		num_inv_--;
	} 
	if (p == NULL) {
		p = new InvalidationRec(name, mtime);
		p->insert(&invlist_);
		num_inv_++;
	}
}

InvalidationRec* HttpYucInvalServer::get_invrec(const char *name)
{
	// XXX What should we do if we already have an
	// invalidation record of this page in our 
	// invlist_? --> We should replace it with the new one
	InvalidationRec *r = invlist_;
	for (r = invlist_; r != NULL; r = r->next())
		if (strcmp(name, r->pg()) == 0)
			return r;
	return NULL;
}

HttpHbData* HttpYucInvalServer::pack_heartbeat()
{
	HttpHbData *data = new HttpHbData(id_, num_inv_);
	InvalidationRec *p = invlist_, *q;
	int i = 0;
	while (p != NULL) {
		data->add(i++, p);
		// Clearing up invalidation sending list
		if (!p->dec_scount()) {
			// Each invalidation is sent to its children
			// for at most HTTP_HBEXPIRE times. After that 
			// the invalidation record is removed from 
			// the list
			q = p;
			p = p->next();
			q->detach();
			delete q;
			num_inv_--;
		} else 
			p = p->next();
	}
	return data;
}

void HttpYucInvalServer::send_hb_helper(int size, AppData *data)
{
	inv_sender_->send(size, data);
}

void HttpYucInvalServer::send_heartbeat()
{
	if (inv_sender_ == NULL)
		return;

	HttpHbData* d = pack_heartbeat();
	send_hb_helper(d->cost(), d);
}




//----------------------------------------------------------------------
// Http cache with invalidation protocols. Http/Cache and Http/Cache/Inval
// are used as base classes and provide common TCL methods. Http/Cache 
// derives Http/Cache/TTL and Http/Cache/TTL/Old. Http/Cache/Inval derives
// unicast invalidation and multicast invalidation.
//----------------------------------------------------------------------

static class HttpCacheClass : public TclClass {
public:
        HttpCacheClass() : TclClass("Http/Cache") {}
        TclObject* create(int, const char*const*) {
		return (new HttpCache());
	}
} class_httpcache_app;

static class HttpInvalCacheClass : public TclClass {
public:
        HttpInvalCacheClass() : TclClass("Http/Cache/Inval") {}
        TclObject* create(int, const char*const*) {
		return (new HttpInvalCache());
	}
} class_httpinvalcache_app;

static class HttpMInvalCacheClass : public TclClass {
public:
        HttpMInvalCacheClass() : TclClass("Http/Cache/Inval/Mcast") {}
        TclObject* create(int, const char*const*) {
		return (new HttpMInvalCache());
	}
} class_HttpMInvalCache_app;

// Static members and functions
HttpMInvalCache** HttpMInvalCache::CacheRepository_ = NULL;
int HttpMInvalCache::NumCache_ = 0;

void HttpMInvalCache::add_cache(HttpMInvalCache *c)
{
	if (CacheRepository_ == NULL) {
		CacheRepository_ = new HttpMInvalCache* [c->id() + 1];
		CacheRepository_[c->id()] = c;
		NumCache_ = c->id();
	} else if (NumCache_ < c->id()) {
		HttpMInvalCache** p = new HttpMInvalCache* [c->id()+1];
		memcpy(p, CacheRepository_, 
		       (c->id()+1)*sizeof(HttpMInvalCache*));
		delete[]CacheRepository_;
		CacheRepository_ = p;
		NumCache_ = c->id();
		p[c->id()] = c;
	} else
		CacheRepository_[c->id()] = c;
}

HttpMInvalCache::HttpMInvalCache() : 
	hb_timer_(this, HTTP_HBINTERVAL),
	inv_sender_(0), num_sender_(0), size_sender_(0), 
	invlist_(0), num_inv_(0), inv_parent_(NULL),
	upd_sender_(NULL), num_updater_(0), size_updater_(0)
{
	bind("hb_interval_", &hb_interval_);
	bind("enable_upd_", &enable_upd_);	// If we allow push
	bind("Ca_", &Ca_);
	bind("Cb_", &Cb_);
	bind("push_thresh_", &push_thresh_);
	bind("push_high_bound_", &push_high_bound_);
	bind("push_low_bound_", &push_low_bound_);

	hb_timer_.set_interval(hb_interval_);
	Tcl_InitHashTable(&sstate_, TCL_ONE_WORD_KEYS);
	Tcl_InitHashTable(&nbr_, TCL_ONE_WORD_KEYS);
}

HttpMInvalCache::~HttpMInvalCache() 
{
	if (num_sender_ > 0) 
		delete []inv_sender_;
	Tcl_DeleteHashTable(&sstate_);
	Tcl_DeleteHashTable(&nbr_);
}

int HttpMInvalCache::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc < 2) 
		return HttpInvalCache::command(argc, argv);

	switch (argv[1][0]) {
	case 'a':
		if ((strcmp(argv[1], "add-inval-listener") == 0) ||
		    (strcmp(argv[1], "add-upd-listener") == 0)) {
			HttpInvalAgent *tmp = 
				(HttpInvalAgent *)TclObject::lookup(argv[2]);
			tmp->attachApp((Application *)this);
			return TCL_OK;
		} else if (strcmp(argv[1], "add-inval-sender") == 0) {
			HttpInvalAgent *tmp = 
				(HttpInvalAgent *)TclObject::lookup(argv[2]);
			if (tmp == NULL) {
				tcl.resultf("Non-existent agent %s", argv[2]);
				return TCL_ERROR;
			}
			if (num_sender_ == size_sender_) {
				HttpInvalAgent **tt = 
					new HttpInvalAgent*[size_sender_+5];
				memcpy(tt, inv_sender_, 
				       sizeof(HttpInvalAgent*)*size_sender_);
				delete []inv_sender_;
				size_sender_ += 5;
				inv_sender_ = tt;
			}
			inv_sender_[num_sender_++] = tmp;
			return TCL_OK;
		} else if (strcmp(argv[1], "add-to-map") == 0) {
			add_cache(this);
			return TCL_OK;
		} else if (strcmp(argv[1], "add-upd-sender") == 0) {
			HttpInvalAgent *tmp = 
				(HttpInvalAgent *)TclObject::lookup(argv[2]);
			if (tmp == NULL) {
				tcl.resultf("Non-existent agent %s", argv[2]);
				return TCL_ERROR;
			}
			if (num_updater_ == size_updater_) {
				HttpInvalAgent **tt = 
					new HttpInvalAgent*[size_updater_+5];
				memcpy(tt, upd_sender_, 
				       sizeof(HttpInvalAgent*)*size_updater_);
				delete []upd_sender_;
				size_updater_ += 5;
				upd_sender_ = tt;
			}
			upd_sender_[num_updater_++] = tmp;
			return TCL_OK;
		}
		break;

	case 'c':
		if (strcmp(argv[1], "count-request") == 0) {
			ClientPage *pg = 
				(ClientPage *)pool_->get_page(argv[2]);
			if (pg == NULL) {
				tcl.resultf("%d count-request: No page %s", 
					    id_, argv[2]);
				return TCL_ERROR;
			}
			pg->count_request(Cb_, push_high_bound_);
			log("E NTF p %s v %d\n", argv[2], pg->counter());
			return TCL_OK;
		} else if (strcmp(argv[1], "check-sstate") == 0) {
			/*
			 * <cache> check-sstate <sid> <cid>
			 * If server is re-connected, reinstate it
			 */
			int sid = atoi(argv[2]);
			int cid = atoi(argv[3]);
			check_sstate(sid, cid);
			return TCL_OK;
		}
		break;

	case 'i':
		// XXX We don't need a "is-pushable" for cache!
		if (strcmp(argv[1], "is-unread") == 0) {
			ClientPage *pg = 
				(ClientPage *)pool_->get_page(argv[2]);
			if (pg == NULL) {
				tcl.resultf("%d is-unread: No page %s", 
					    id_, argv[2]);
				return TCL_ERROR;
			}
			tcl.resultf("%d", pg->is_unread());
			return TCL_OK;
		}
		break;

	case 'j':
		if (strcmp(argv[1], "join") == 0) {
			/*
			 * <cache> join <server_id> <cache>
			 *
			 * <server> join via <cache>. If they are the same,
			 * it means we are the primary cache for <server>.
			 */
			int sid = atoi(argv[2]);
			HttpMInvalCache *cache = 
				(HttpMInvalCache*)TclObject::lookup(argv[3]);
			if (cache == NULL) {
			    tcl.add_errorf("Non-existent cache %s", argv[3]);
			    return TCL_ERROR;
			}
			// Add neighbor cache if necessary
			NeighborCache *c = lookup_nbr(cache->id());
			if (c == NULL)
				add_nbr(cache);
			// Establish server invalidation contract
			check_sstate(sid, cache->id());
			return TCL_OK;
		}
		break;

	case 'p':
		if (strcmp(argv[1], "parent-cache") == 0) {
			/*
			 * <cache> parent-cache <web_server_id>
			 * Return the parent cache of <web_server_id> in the 
			 * virtual distribution tree. 
			 */
			int sid = atoi(argv[2]);
			SState *sst = lookup_sstate(sid);
			if (sst == NULL)
				tcl.result("");
			else {
				// Bad hack... :(
		NeighborCache *c = lookup_nbr(sst->cache()->cache()->id());
				tcl.resultf("%s", c->cache()->name());
			}
			return TCL_OK;
		} else if (strcmp(argv[1], "push-children") == 0) {
			// Multicast the pushed page to all children
			ClientPage *pg = 
				(ClientPage *)pool_->get_page(argv[2]);
			if (pg == NULL) {
				tcl.resultf("%d is-valid: No page %s", 
					    id_, argv[2]);
				return TCL_ERROR;
			}
			send_upd(pg);
			return TCL_OK;
		}
		break;

	case 'r':
		if (strcmp(argv[1], "recv-inv") == 0) {
			/*
			 * <cache> recv-inv <pageid> <modtime>
			 * This should be called only by a web server, 
			 * therefore we do not check the validity of the 
			 * invalidation
			 */
			// Pack it into a HttpHbData, and process it
			HttpHbData *d = new HttpHbData(id_, 1);
			strcpy(d->rec_pg(0), argv[2]);
			d->rec_mtime(0) = strtod(argv[3], NULL);
			//int old_inv = num_inv_;
			tcl.resultf("%d", recv_inv(d));
			delete d;
			return TCL_OK;
		} else if (strcmp(argv[1], "recv-push") == 0) {
			/* 
			 * <cache> recv-push <pageid> args
			 */
			HttpUpdateData *d = new HttpUpdateData(id_, 1);
			strcpy(d->rec_page(0), argv[2]);
			for (int i = 3; i < argc; i+=2) {
				if (strcmp(argv[i], "modtime") == 0)
				  d->rec_mtime(0) = strtod(argv[i+1], NULL);
				else if (strcmp(argv[i], "size") == 0) {
				  d->rec_size(0) = atoi(argv[i+1]);
				  // XXX need to set total update page size
				  d->set_pgsize(d->rec_size(0));
				} else if (strcmp(argv[i], "age") == 0)
				  d->rec_age(0) = strtod(argv[i+1], NULL);
			}
			tcl.resultf("%d", recv_upd(d));
			delete d;
			return TCL_OK;
		} else if (strcmp(argv[1], "register-server") == 0) {
			/*
			 * <self> register-server <cache_id> <server_id>
			 * We get a GET response about a page from <server>, 
			 * which we hear from <cache> 
			 */
			int cid = atoi(argv[2]);
			int sid = atoi(argv[3]);
			// Assuming we've already known the cache
			check_sstate(sid, cid);
			return TCL_OK;
		}
		break;

	case 's':
		if (strcmp(argv[1], "start-hbtimer") == 0) {
			if (hb_timer_.status() == TIMER_IDLE)
				hb_timer_.sched();
			return TCL_OK;
		} else if (strcmp(argv[1], "server-hb") == 0) {
			int id = atoi(argv[2]);
			recv_heartbeat(id);
			return TCL_OK;
		} else if (strcmp(argv[1], "set-pinv-agent") == 0) {
			inv_parent_ = 
				(HttpUInvalAgent*)TclObject::lookup(argv[2]);
			return TCL_OK;
		} else if (strcmp(argv[1], "set-parent") == 0) {
			HttpMInvalCache *c = 
				(HttpMInvalCache*)TclObject::lookup(argv[2]);
			if (c == NULL) {
			    tcl.add_errorf("Non-existent cache %s", argv[2]);
			    return TCL_ERROR;
			}
			// Add parent cache into known cache list
			add_nbr(c);
			return TCL_OK;
		} else if (strcmp(argv[1], "set-unread") == 0) {
			ClientPage *pg = 
				(ClientPage *)pool_->get_page(argv[2]);
			if (pg == NULL) {
				tcl.resultf("%d is-valid: No page %s", 
					    id_, argv[2]);
				return TCL_ERROR;
			}
			pg->set_unread();
			return TCL_OK;
		} else if (strcmp(argv[1], "set-read") == 0) {
			ClientPage *pg = 
				(ClientPage *)pool_->get_page(argv[2]);
			if (pg == NULL) {
				tcl.resultf("%d is-valid: No page %s", 
					    id_, argv[2]);
				return TCL_ERROR;
			}
			pg->set_read();
			return TCL_OK;
		} else if (strcmp(argv[1], "set-mandatory-push") == 0) { 
			ClientPage *pg = 
				(ClientPage *)pool_->get_page(argv[2]);
			if (pg == NULL) {
				tcl.resultf("%d is-valid: No page %s", 
					    id_, argv[2]);
				return TCL_ERROR;
			}
			pg->set_mpush(Scheduler::instance().clock());
			return TCL_OK;
		} else if (strcmp(argv[1], "stop-mpush") == 0) {
			ClientPage *pg = 
				(ClientPage *)pool_->get_page(argv[2]);
			if (pg == NULL) {
				tcl.resultf("%d is-valid: No page %s", 
					    id_, argv[2]);
				return TCL_ERROR;
			}
			pg->clear_mpush();
			return TCL_OK;
		}
		break;

	default:
		break;
	}
	return HttpInvalCache::command(argc, argv);
}

void HttpMInvalCache::check_sstate(int sid, int cid)
{
	if ((sid == cid) && (cid == id_))
		// How come?
		return;
	SState *sst = lookup_sstate(sid);
	NeighborCache *c = lookup_nbr(cid);
	if (sst == NULL) {
		if (c == NULL) {
			fprintf(stderr, 
"%g: cache %d: No neighbor cache for received invalidation from %d via %d\n", 
				Scheduler::instance().clock(), id_, sid, cid);
			abort();
		}
#ifdef WEBCACHE_DEBUG
		fprintf(stderr,
			"%g: cache %d: registered server %d via cache %d\n",
			Scheduler::instance().clock(), id_, sid, cid);
#endif
		sst = new SState(c);
		add_sstate(sid, sst);
		c->add_server(sid);
	} else if (sst->is_down()) {
		sst->up();
		if (cid != id_) {
			if (c == NULL) {
				fprintf(stderr, 
 "[%g]: Cache %d has an invalid neighbor cache %d\n",
 Scheduler::instance().clock(), id_, cid);
				abort();
			}
			c->server_up(sid);
		}
#ifdef WEBCACHE_DEBUG
		fprintf(stderr, 
		"[%g] Cache %d reconnected to server %d via cache %d\n", 
			Scheduler::instance().clock(), id_, 
			sid, cid);
#endif
		Tcl::instance().evalf("%s mark-rejoin", name_);
	}
}

void HttpMInvalCache::add_sstate(int sid, SState *sst)
{
	int newEntry = 1;
	long key = sid;
	Tcl_HashEntry *he = 
		Tcl_CreateHashEntry(&sstate_, (const char *)key, &newEntry);
	if (he == NULL) 
		return;
	if (newEntry)
		Tcl_SetHashValue(he, (ClientData)sst);
}

HttpMInvalCache::SState* HttpMInvalCache::lookup_sstate(int sid)
{
        long key = sid;
	Tcl_HashEntry *he = Tcl_FindHashEntry(&sstate_, (const char *)key);
	if (he == NULL)
		return NULL;
	return (SState *)Tcl_GetHashValue(he);
}

NeighborCache* HttpMInvalCache::lookup_nbr(int id)
{
        long key = id;
	Tcl_HashEntry *he = Tcl_FindHashEntry(&nbr_, (const char *)key);
	if (he == NULL)
		return NULL;
	return (NeighborCache *)Tcl_GetHashValue(he);
}

// Add a new neighbor cache
void HttpMInvalCache::add_nbr(HttpMInvalCache *cache)
{
	int newEntry = 1;
	long key = cache->id ();
	Tcl_HashEntry *he = 
		Tcl_CreateHashEntry(&nbr_, (const char *)key, 
				    &newEntry);
	if (he == NULL) 
		return;
	// If this cache already exists, don't do anything
	if (!newEntry)
		return;

	// Start a timer for the neighbor
	LivenessTimer *timer = 
		new LivenessTimer(this,HTTP_HBEXPIRE_COUNT*hb_interval_,
				  cache->id());

	double time = Scheduler::instance().clock();
	NeighborCache *c = new NeighborCache(cache, time, timer);
	Tcl_SetHashValue(he, (ClientData)c);
}

// Two ways to receive a heartbeat: (1) via HttpInvalAgent; (2) via TCP 
// connection between a server and a primary cache. (See "server-hb" handling
// in command().
void HttpMInvalCache::recv_heartbeat(int id)
{
	// Receive time of the heartbeat
	double time = Scheduler::instance().clock();

	NeighborCache *c = lookup_nbr(id);
	if (c == NULL) {
		// XXX
		// The only possible place for this to happen is in the TLC
		// group, where no JOIN could ever reach. Moreover, 
		// we don't even have an entry for that cache yet, so here
		// we add that cache into our entry, and later on we'll add
		// corresponding servers there.
		if (id == id_) 
			return;
		add_nbr(map_cache(id));
#ifdef WEBCACHE_DEBUG
		fprintf(stderr, "TLC %d discovered TLC %d\n", id_, id);
#endif
		return;
	} else if (c->is_down()) {
		// Neighbor cache recovers. Don't do anything special and
		// let invalid entries recover themselves
		c->up();
#ifdef WEBCACHE_DEBUG
		fprintf(stderr, "[%g] Cache %d reconnected to cache %d\n", 
			Scheduler::instance().clock(), id_, id);
#endif
		Tcl::instance().evalf("%s mark-rejoin", name_);
	} else
		// Update heartbeat time
		c->reset_timer(time);
}

void HttpMInvalCache::invalidate_server(int sid)
{
	SState *sst = lookup_sstate(sid);
	if (sst->is_down())
		// If this server is already marked down, return
		return;
	sst->down();
	pool_->invalidate_server(sid);
}

void HttpMInvalCache::handle_node_failure(int cid)
{
#ifdef WEBCACHE_DEBUG
	fprintf(stderr, "[%g] Cache %d disconnected from cache %d\n", 
		Scheduler::instance().clock(), id_, cid);
#endif
	Tcl::instance().evalf("%s mark-leave", name_);

	NeighborCache *c = lookup_nbr(cid);
	if (c == NULL) {
		fprintf(stderr, "%s: An unknown neighbor cache %d failed.\n",
			name_, cid);
	}
	// Mark the cache down
	c->down();
	// Invalidate entries of all servers related to that cache
	// XXX We don't have an iterator for all servers in NeighborCache!
	c->invalidate(this);

	// Send leave message to all children
	HttpLeaveData* data = new HttpLeaveData(id_, c->num());
	c->pack_leave(*data);
	send_leave(data);
}

void HttpMInvalCache::recv_leave(HttpLeaveData *d)
{
#ifdef WEBCACHE_DEBUG
	fprintf(stderr, "[%g] Cache %d gets a LEAVE from cache %d\n", 
		Scheduler::instance().clock(), id_, d->id());
#endif

	if (d->num() == 0) {
		fprintf(stderr, 
		    "%s (%g) gets a leave from cache without server!\n", 
			name_, Scheduler::instance().clock());
		return;
	}

	SState *sst;
	HttpLeaveData* data = new HttpLeaveData(id_, d->num());
	NeighborCache *c = lookup_nbr(d->id());
	int i, j;
	for (i = 0, j = 0; i < d->num(); i++) {
		sst = lookup_sstate(d->rec_id(i));

		// If we haven't heard of that server, which means we don't 
		// have any page of that server, ignore the leave message.
		if (sst == NULL) 
			continue;
		// If it's already marked down, don't bother again.
		if (sst->is_down()) 
			continue;
		// If we hear a LEAVE about a server from one of 
		// our child in the virtual distribution tree 
		// of the server, ignore it.
		if (c != sst->cache()) 
			continue;

		// We have the page, and we hold inval contract. Invalidate 
		// the page and inform our children of it.
		sst->down();
		data->add(j++, d->rec_id(i));
		pool_->invalidate_server(d->rec_id(i));
		Tcl::instance().evalf("%s mark-leave", name_);
	}
	// Delete it if it's not sent out 
	if (j > 0)
		send_leave(data);
	delete data;
}

void HttpMInvalCache::send_leave(HttpLeaveData *d)
{
	send_hb_helper(d->cost(), d);
}

void HttpMInvalCache::timeout(int reason)
{
	switch (reason) {
	case HTTP_INVALIDATION:
		// Send an invalidation message
		send_heartbeat();
		break;
	case HTTP_UPDATE:
		// XXX do nothing. May put client selective joining update
		// group here.
		break;
	default:
		fprintf(stderr, "%s: Unknown reason %d", name_, reason);
		break;
	}
}

void HttpMInvalCache::process_data(int size, AppData* data)
{
	if (data == NULL)
		return;

	switch (data->type()) {
	case HTTP_INVALIDATION: {
		// Update timer for the source of the heartbeat
		HttpHbData *inv = (HttpHbData*)data;
		recv_heartbeat(inv->id());
		recv_inv(inv);
		break;
	}
	case HTTP_UPDATE: {
		// Replace all updated pages
		HttpUpdateData *pg = (HttpUpdateData*)data;
		recv_upd(pg);
		break;
	}
	// JOIN messages are sent via TCP and direct TCL callback.
	case HTTP_LEAVE: {
		HttpLeaveData *l = (HttpLeaveData*)data;
		recv_leave(l);
		break;
	}
	default:
		HttpApp::process_data(size, data);
		return;
	}
}

void HttpMInvalCache::add_inv(const char *name, double mtime)
{
	InvalidationRec *p = get_invrec(name);
	if ((p != NULL) && (p->mtime() < mtime)) {
		p->detach();
		delete p;
		p = NULL;
		num_inv_--;
	} 
	if (p == NULL) {
		p = new InvalidationRec(name, mtime);
		p->insert(&invlist_);
		num_inv_++;
	}
}

InvalidationRec* HttpMInvalCache::get_invrec(const char *name)
{
	// XXX What should we do if we already have an
	// invalidation record of this page in our 
	// invlist_? --> We should replace it with the new one
	InvalidationRec *r = invlist_;
	for (r = invlist_; r != NULL; r = r->next())
		if (strcmp(name, r->pg()) == 0)
			return r;
	return NULL;
}

HttpHbData* HttpMInvalCache::pack_heartbeat()
{
	HttpHbData *data = new HttpHbData(id_, num_inv_);
	InvalidationRec *p = invlist_, *q;
	int i = 0;
	while (p != NULL) {
		data->add(i++, p);
		// Clearing up invalidation sending list
		if (!p->dec_scount()) {
			// Each invalidation is sent to its children
			// for at most HTTP_HBEXPIRE times. After that 
			// the invalidation record is removed from 
			// the list
			q = p;
			p = p->next();
			q->detach();
			delete q;
			num_inv_--;
		} else 
			p = p->next();
	}
	return data;
}

int HttpMInvalCache::recv_inv(HttpHbData *data)
{
	if (data->num_inv() == 0)
		return 0;

	InvalidationRec *head;
	data->extract(head);
	int old_inv = num_inv_;
	process_inv(data->num_inv(), head, data->id());
	//log("E GINV z %d\n", data->size());
	if (old_inv < num_inv_) 
		// This invalidation is valid
		return 1;
	else 
		return 0;
}

// Get an invalidation, check invalidation modtimes, then setup 
// invalidation forwarding entries
// The input invalidation record list is destroyed.
void HttpMInvalCache::process_inv(int, InvalidationRec *ivlist, int cache)
{
	InvalidationRec *p = ivlist, *q, *r;
	//int upd = 0;
	while (p != NULL) {
		ClientPage* pg = (ClientPage *)pool_->get_page(p->pg());

		// XXX Establish server states. Server states only gets 
		// established when we have a page (no matter if we have its
		// content), and we have got an invalidation for the page. 
		// Then we know we've got an invalidation contract for the 
		// page.
		if (pg != NULL) {
			check_sstate(pg->server()->id(), cache);
			// Count this invalidation no matter whether we're
			// going to drop it. But if we doesn't get it 
			// from our virtual parent, don't count it
			SState *sst = lookup_sstate(pg->server()->id());
			if (sst == NULL) {
				// How come we doesn't know the server???
				fprintf(stderr, 
					"%s %d: couldn't find the server.\n", 
					__FILE__, __LINE__);
				abort();
			}
			if ((sst->cache()->cache()->id() == cache) && 
			    (pg->mtime() > p->mtime())) {
				// Don't count repeated invalidations.
				pg->count_inval(Ca_, push_low_bound_);
				log("E NTF p %s v %d\n",p->pg(),pg->counter());
			}
		}

		// Hook for filters of derived classes
		if (recv_inv_filter(pg, p) == HTTP_INVALCACHE_FILTERED) {
			// If we do not have the page, or we have (or know 
			// about) a newer page, ignore this invalidation 
			// record and keep going.
			//
			// If we have this version of the page, and it's 
			// already invalid, ignore this extra invalidation
			q = p;
			p = p->next();
			q->detach();
			delete q;
		} else {
			// Otherwise we invalidate our page and setup a 
			// invalidation sending record for the page
			pg->invalidate(p->mtime());
			// Delete existing record for that page if any
			q = get_invrec(p->pg());
			if ((q != NULL) && (q->mtime() < p->mtime())) {
				q->detach();
				delete q;
				q = NULL;
				num_inv_--;
			}
			r = p; 
			p = p->next();
			r->detach();
			// Insert it if necessary
			if (q == NULL) {
				r->insert(&invlist_);
				num_inv_++;
				// XXX
				Tcl::instance().evalf("%s mark-invalid",name_);
				log("E GINV p %s m %.17g\n", r->pg(), r->mtime());
			} else
				delete r;
		}
	}
}

void HttpMInvalCache::send_hb_helper(int size, AppData *data)
{
	if (inv_parent_ != NULL) 
		inv_parent_->send(size, data->copy());
	for (int i = 0; i < num_sender_; i++)
		inv_sender_[i]->send(size, data->copy());
}

void HttpMInvalCache::send_heartbeat()
{
	if ((num_sender_ == 0) && (inv_parent_ == NULL))
		return;

	HttpHbData* d = pack_heartbeat();
	send_hb_helper(d->cost(), d);
	delete d;
}

int HttpMInvalCache::recv_upd(HttpUpdateData *d)
{
	if (d->num() != 1) {
		fprintf(stderr, 
			"%d gets an update which contain !=1 pages.\n", id_);
		abort();
	}

	ClientPage *pg = pool_->get_page(d->rec_page(0));
	if (pg != NULL)
	{
		if (pg->mtime() >= d->rec_mtime(0)) {
			// If we've already had this version, or a newer 
			// version, ignore this old push
//			fprintf(stderr, "[%g] %d gets an old push\n", 
// 				Scheduler::instance().clock(), id_);
//			log("E OLD m %g p %g\n", d->rec_mtime(0), pg->mtime());
			return 0;
		} else {
			// Our old page is invalidated by this new push,
			// set up invalidation records for our children
			add_inv(d->rec_page(0), d->rec_mtime(0));
			pg->count_inval(Ca_, push_low_bound_);
			log("E NTF p %s v %d\n", d->rec_page(0),pg->counter());
		}
	}

	// Add the new page into our pool
	ClientPage *q = pool_->enter_page(d->rec_page(0), d->rec_size(0), 
					  d->rec_mtime(0),
					  Scheduler::instance().clock(),
					  d->rec_age(0));
	// By default the page is valid and read. Set it as unread
	q->set_unread();

	log("E GUPD m %.17g z %d\n", d->rec_mtime(0), d->pgsize());
	Tcl::instance().evalf("%s mark-valid", name_);

	// XXX If the page was previously marked as MandatoryPush, then
	// we need to check if it's timed out
	if (q->is_mpush() && (Scheduler::instance().clock() - q->mpush_time()
			      > HTTP_HBEXPIRE_COUNT*hb_interval_)) {
		// If mandatory push timer expires, stop push
		q->clear_mpush();
		Tcl::instance().evalf("%s cancel-mpush-refresh %s", 
				      name_, d->rec_page(0));
	}

	if ((enable_upd_ && (q->counter() >= push_thresh_)) || q->is_mpush())
	{
		// XXX Continue pushing if we either select to push, or 
		// were instructed to do so.
		return 1;
	}
	else 
	{
		return 0;
	}
}

HttpUpdateData* HttpMInvalCache::pack_upd(ClientPage* page)
{
	HttpUpdateData *data = new HttpUpdateData(id_, 1);
	data->add(0, page);
	return data;
}

void HttpMInvalCache::send_upd_helper(int pgsize, AppData* data)
{
	for (int i = 0; i < num_updater_; i++)
		upd_sender_[i]->send(pgsize, data->copy());
}

void HttpMInvalCache::send_upd(ClientPage *page)
{
	if ((num_updater_ == 0) || !enable_upd_) 
		return;

	HttpUpdateData* d = pack_upd(page);
	send_upd_helper(d->pgsize(), d);
	delete d;
}


//----------------------------------------------------------------------
// Multicast invalidation + two way liveness messages + 
// invalidation filtering. 
//----------------------------------------------------------------------
static class HttpPercInvalCacheClass : public TclClass {
public:
        HttpPercInvalCacheClass() : TclClass("Http/Cache/Inval/Mcast/Perc") {}
        TclObject* create(int, const char*const*) {
		return (new HttpPercInvalCache());
	}
} class_HttpPercInvalCache_app;

HttpPercInvalCache::HttpPercInvalCache() 
{
	bind("direct_request_", &direct_request_);
}

int HttpPercInvalCache::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();

	if (strcmp(argv[1], "is-header-valid") == 0) {
		ClientPage *pg = 
			(ClientPage *)pool_->get_page(argv[2]);
		if (pg == NULL) {
			tcl.resultf("%d is-valid: No page %s", 
				    id_, argv[2]);
			return TCL_ERROR;
		}
		tcl.resultf("%d", pg->is_header_valid());
		return TCL_OK;
	} else if (strcmp(argv[1], "enter-metadata") == 0) {
		/* 
		 * <cache> enter-metadata <args...>
		 * The same arguments as enter-page, but set the page status
		 * as HTTP_VALID_HEADER, i.e., if we get a request, we need 
		 * to fetch the actual valid page content
		 */
		ClientPage *pg = pool_->enter_metadata(argc, argv);
		if (pg == NULL)
			return TCL_ERROR;
		else
			return TCL_OK;
	}

	return HttpMInvalCache::command(argc, argv);
}
