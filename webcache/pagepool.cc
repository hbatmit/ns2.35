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
 * $Header: /cvsroot/nsnam/ns-2/webcache/pagepool.cc,v 1.19 2011/10/06 00:23:38 tom_henderson Exp $
 */

#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#ifdef WIN32
#include <windows.h>
#include <io.h>
#else 
#include <unistd.h>
#include <sys/file.h>
#endif
#include <sys/stat.h>

#include <stdio.h>
#include <limits.h>
#include <ctype.h>

extern "C" {
#include <otcl.h>
}
#include "pagepool.h"
#include "http.h"

// Static/global variables
int ClientPage::PUSHALL_ = 0;	// Initialized to selective push

void ServerPage::set_mtime(int *mt, int n)
{
	if (mtime_ != NULL) 
		delete []mtime_;
	mtime_ = new int[n];
	memcpy(mtime_, mt, sizeof(int)*n);
}

ClientPage::ClientPage(const char *n, int s, double mt, double et, double a) :
		Page(s), age_(a), mtime_(mt), etime_(et), 
		status_(HTTP_VALID_PAGE), counter_(0), 
		mpushTime_(0)
{
	// Parse name to get server and page id
	char *buf = new char[strlen(n) + 1];
	strcpy(buf, n);
	char *tmp = strtok(buf, ":");
	server_ = (HttpApp*)TclObject::lookup(tmp);
	if (server_ == NULL) {
		fprintf(stderr, "Non-exitent server name for page %s", n);
		abort();
	}
	tmp = strtok(NULL, ":");
	id_ = atol(tmp);
	delete []buf;
}

void ClientPage::print_name(char* name, PageID& id)
{
	sprintf(name, "%s:%-d", id.s_->name(), id.id_);
}

void ClientPage::split_name(const char* name, PageID& id)
{
	char *buf = new char[strlen(name)+1];
	strcpy(buf, name);
	char *tmp = strtok(buf, ":");
	id.s_ = (HttpApp*)TclObject::lookup(tmp);
	if (id.s_ == NULL) {
		fprintf(stderr, "Non-exitent server name for page %s\n", name);
		abort();
	}
	tmp = strtok(NULL, ":");
	id.id_ = atol(tmp);
	delete []buf;
}

void ClientPage::print_info(char *buf)
{
	sprintf(buf, "size %d modtime %.17g time %.17g age %.17g",
		size(), mtime(), etime(), age());
	if (is_uncacheable())
		strcat(buf, " noc 1");
}

void ClientPage::name(char* buf) 
{
	sprintf(buf, "%s:%d", server_->name(), id());
}


static class PagePoolClass : public TclClass {
public:
        PagePoolClass() : TclClass("PagePool") {}
        TclObject* create(int, const char*const*) {
		return (new PagePool());
	}
} class_pagepool_agent;

int PagePool::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		// XXX Should be static class variables... 
		if (strcmp(argv[1], "set-allpush") == 0) {
			ClientPage::PUSHALL_ = 1;
			return (TCL_OK);
		}
		if (strcmp(argv[1], "set-selpush") == 0) {
			ClientPage::PUSHALL_ = 0;
			return (TCL_OK);
		}
	}
	return TclObject::command(argc, argv);
}


// TracePagePool
// Used for Worrell's filtered server traces only. For handling general 
// web server traces and proxy traces, have a look at ProxyTracePagePool below.
//
// Load a trace statistics file, and randomly generate requests and 
// page lifetimes from the trace.
//
// Trace statistics file format:
// <URL> <size> {<modification time>}

static class TracePagePoolClass : public TclClass {
public:
        TracePagePoolClass() : TclClass("PagePool/Trace") {}
        TclObject* create(int argc, const char*const* argv) {
		if (argc >= 5)
			return (new TracePagePool(argv[4]));
		return 0;
	}
} class_tracepagepool_agent;

TracePagePool::TracePagePool(const char *fn) : 
	PagePool(), ranvar_(0)
{
	FILE *fp = fopen(fn, "r");
	if (fp == NULL) {
		fprintf(stderr, 
			"TracePagePool: couldn't open trace file %s\n", fn);
		abort();	// What else can we do?
	}

	namemap_ = new Tcl_HashTable;
	Tcl_InitHashTable(namemap_, TCL_STRING_KEYS);
	idmap_ = new Tcl_HashTable;
	Tcl_InitHashTable(idmap_, TCL_ONE_WORD_KEYS);

	while (load_page(fp));
	change_time();
}

TracePagePool::~TracePagePool()
{
	if (namemap_ != NULL) {
		Tcl_DeleteHashTable(namemap_);
		delete namemap_;
	}
	if (idmap_ != NULL) {
		Tcl_DeleteHashTable(idmap_);
		delete idmap_;
	}
}

void TracePagePool::change_time()
{
	Tcl_HashEntry *he;
	Tcl_HashSearch hs;
	ServerPage *pg;
	int i, j;

	for (i = 0, he = Tcl_FirstHashEntry(idmap_, &hs);
	     he != NULL;
	     he = Tcl_NextHashEntry(&hs), i++) {
		pg = (ServerPage *) Tcl_GetHashValue(he);
		for (j = 0; j < pg->num_mtime(); j++) 
			pg->mtime(j) -= (int)start_time_;
	}
	end_time_ -= start_time_;
	start_time_ = 0;
	duration_ = (int)end_time_;
}

ServerPage* TracePagePool::load_page(FILE *fp)
{
	static char buf[TRACEPAGEPOOL_MAXBUF];
	char *delim = " \t\n";
	char *tmp1, *tmp2;
	ServerPage *pg;

	// XXX Use internal variables of struct Page
	if (!fgets(buf, TRACEPAGEPOOL_MAXBUF, fp))
		return NULL;

	// URL
	tmp1 = strtok(buf, delim);
	// Size
	tmp2 = strtok(NULL, delim);
	pg = new ServerPage(atoi(tmp2), num_pages_++);

	if (add_page(tmp1, pg)) {
		delete pg;
		return NULL;
	}

	// Modtimes, assuming they are in ascending time order
	int num = 0;
	int *nmd = new int[5];
	while ((tmp1 = strtok(NULL, delim)) != NULL) {
		if (num >= 5) {
			int *tt = new int[num+5];
			memcpy(tt, nmd, sizeof(int)*num);
			delete []nmd;
			nmd = tt;
		}
		nmd[num] = atoi(tmp1);
		if (nmd[num] < start_time_)
			start_time_ = nmd[num];
		if (nmd[num] > end_time_)
			end_time_ = nmd[num];
		num++;
	}
	pg->num_mtime() = num;
	pg->set_mtime(nmd, num);
	delete []nmd;
	return pg;
}

int TracePagePool::add_page(const char* name, ServerPage *pg)
{
	int newEntry = 1;
	Tcl_HashEntry *he = Tcl_CreateHashEntry(namemap_, 
						(const char *)name,
						&newEntry);
	if (he == NULL)
		return -1;
	if (newEntry)
		Tcl_SetHashValue(he, (ClientData)pg);
	else 
		fprintf(stderr, "TracePagePool: Duplicate entry %s\n", 
			name);

	long key = pg->id();
	Tcl_HashEntry *hf = 
		Tcl_CreateHashEntry(idmap_, (const char *)key, &newEntry);
	if (hf == NULL) {
		Tcl_DeleteHashEntry(he);
		return -1;
	}
	if (newEntry)
		Tcl_SetHashValue(hf, (ClientData)pg);
	else 
		fprintf(stderr, "TracePagePool: Duplicate entry %d\n", 
			pg->id());

	return 0;
}

ServerPage* TracePagePool::get_page(int id)
{
	if ((id < 0) || (id >= num_pages_))
		return NULL;
	long key = id;
	Tcl_HashEntry *he = Tcl_FindHashEntry(idmap_, (const char *)key);
	if (he == NULL)
		return NULL;
	return (ServerPage *)Tcl_GetHashValue(he);
}

int TracePagePool::command(int argc, const char *const* argv)
{
	Tcl &tcl = Tcl::instance();

	if (argc == 2) {
		if (strcmp(argv[1], "get-poolsize") == 0) {
			/* 
			 * <pgpool> get-poolsize
			 * Get the number of pages currently in pool
			 */
			tcl.resultf("%d", num_pages_);
			return TCL_OK;
		} else if (strcmp(argv[1], "get-start-time") == 0) {
			tcl.resultf("%.17g", start_time_);
			return TCL_OK;
		} else if (strcmp(argv[1], "get-duration") == 0) {
			tcl.resultf("%d", duration_);
			return TCL_OK;
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "gen-pageid") == 0) {
			/* 
			 * <pgpool> gen-pageid <client_id>
			 * Randomly generate a page id from page pool
			 */
			double tmp = ranvar_ ? ranvar_->value() : 
				Random::uniform();
			// tmp should be in [0, num_pages_-1]
			tmp = (tmp < 0) ? 0 : (tmp >= num_pages_) ? 
				(num_pages_-1):tmp;
			if ((int)tmp >= num_pages_) abort();
			tcl.resultf("%d", (int)tmp);
			return TCL_OK;
		} else if (strcmp(argv[1], "gen-size") == 0) {
			/*
			 * <pgpool> gen-size <pageid>
			 */
			int id = atoi(argv[2]);
			ServerPage *pg = get_page(id);
			if (pg == NULL) {
				tcl.add_errorf("TracePagePool %s: page %d doesn't exists.\n",
					       name_, id);
				return TCL_ERROR;
			}
			tcl.resultf("%d", pg->size());
			return TCL_OK;
		} else if (strcmp(argv[1], "ranvar") == 0) {
			/* 
			 * <pgpool> ranvar <ranvar> 
			 * Set a random var which is used to randomly pick 
			 * a page from the page pool.
			 */
			ranvar_ = (RandomVariable *)TclObject::lookup(argv[2]);
			return TCL_OK;
		} else if (strcmp(argv[1], "set-start-time") == 0) {
			double st = strtod(argv[2], NULL);
			start_time_ = st;
			end_time_ += st;
		} else if (strcmp(argv[1], "gen-init-modtime") == 0) {
			tcl.resultf("%.17g", Scheduler::instance().clock());
			return TCL_OK;
		}
	} else {
		if (strcmp(argv[1], "gen-modtime") == 0) {
			/* 
			 * <pgpool> get-modtime <pageid> <modtime>
			 * 
			 * Return next modtime that is larger than modtime
			 * To retrieve the first modtime (creation time), set 
			 * <modtime> to -1 in the request.
			 */
			int id = atoi(argv[2]);
			double mt = strtod(argv[3], NULL);
			ServerPage *pg = get_page(id);
			if (pg == NULL) {
				tcl.add_errorf("TracePagePool %s: page %d doesn't exists.\n",
					       name_, id);
				return TCL_ERROR;
			}
			for (int i = 0; i < pg->num_mtime(); i++) 
				if (pg->mtime(i) > mt) {
					tcl.resultf("%.17g", 
						    pg->mtime(i)+start_time_);
					return TCL_OK;
				}
			// When get to the last modtime, return -1
			tcl.resultf("%d", INT_MAX);
			return TCL_OK;
		}
	}
	return PagePool::command(argc, argv);
}



static class MathPagePoolClass : public TclClass {
public:
        MathPagePoolClass() : TclClass("PagePool/Math") {}
        TclObject* create(int, const char*const*) {
		return (new MathPagePool());
	}
} class_mathpagepool_agent;

// Use 3 ranvars to generate requests, mod times and page size

int MathPagePool::command(int argc, const char *const* argv)
{
	Tcl& tcl = Tcl::instance();

	// Keep the same tcl interface as PagePool/Trace
	if (argc == 2) {
		if (strcmp(argv[1], "get-poolsize") == 0) { 
			tcl.result("1");
			return TCL_OK;
		} else if (strcmp(argv[1], "get-start-time") == 0) {
			tcl.resultf("%.17g", start_time_);
			return TCL_OK;
		} else if (strcmp(argv[1], "get-duration") == 0) {
			tcl.resultf("%d", duration_);
			return TCL_OK;
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "gen-pageid") == 0) {
			// Single page
			tcl.result("0");
			return TCL_OK;
		} else if (strcmp(argv[1], "gen-size") == 0) {
			if (rvSize_ == 0) {
				tcl.add_errorf("%s: no page size generator", 
					       name_);
				return TCL_ERROR;
			}
			int size = (int) rvSize_->value();
			if (size == 0)
				// XXX do not allow page size 0, because TcpApp
				// doesn't behave correctly when sending 0 byte
				size = 1;
			tcl.resultf("%d", size);
			return TCL_OK;
		} else if (strcmp(argv[1], "ranvar-size") == 0) {
			rvSize_ = (RandomVariable*)TclObject::lookup(argv[2]);
			return TCL_OK;
		} else if (strcmp(argv[1], "ranvar-age") == 0) {
			rvAge_ = (RandomVariable*)TclObject::lookup(argv[2]);
			return TCL_OK;
		} else if (strcmp(argv[1], "set-start-time") == 0) {
			double st = strtod(argv[2], NULL);
			start_time_ = st;
			end_time_ += st;
			return TCL_OK;
		} else if (strcmp(argv[1], "gen-init-modtime") == 0) {
			tcl.resultf("%.17g", Scheduler::instance().clock());
			return TCL_OK;
		}
	} else {
		if (strcmp(argv[1], "gen-modtime") == 0) {
			if (rvAge_ == 0) {
				tcl.add_errorf("%s: no page age generator", 
					       name_);
				return TCL_ERROR;
			}
			double mt = strtod(argv[3], NULL);
			tcl.resultf("%.17g", mt + rvAge_->value());
			return TCL_OK;
		}
	}

	return PagePool::command(argc, argv);
}


// Assume one main page, which changes often, and multiple component pages
static class CompMathPagePoolClass : public TclClass {
public:
        CompMathPagePoolClass() : TclClass("PagePool/CompMath") {}
        TclObject* create(int, const char*const*) {
		return (new CompMathPagePool());
	}
} class_compmathpagepool_agent;


CompMathPagePool::CompMathPagePool()
{
	bind("num_pages_", &num_pages_);
	bind("main_size_", &main_size_);
	bind("comp_size_", &comp_size_);
}

int CompMathPagePool::command(int argc, const char *const* argv)
{
	Tcl& tcl = Tcl::instance();

	// Keep the same tcl interface as PagePool/Trace
	if (argc == 2) {
		if (strcmp(argv[1], "get-poolsize") == 0) { 
			tcl.result("1");
			return TCL_OK;
		} else if (strcmp(argv[1], "get-start-time") == 0) {
			tcl.resultf("%.17g", start_time_);
			return TCL_OK;
		} else if (strcmp(argv[1], "get-duration") == 0) {
			tcl.resultf("%d", duration_);
			return TCL_OK;
		}

	} else if (argc == 3) {
		if (strcmp(argv[1], "gen-pageid") == 0) {
			// Main pageid, never return id of component pages
			tcl.result("0");
			return TCL_OK;
		} else if (strcmp(argv[1], "gen-size") == 0) {
			int id = atoi(argv[2]);
			if (id == 0) 
				tcl.resultf("%d", main_size_);
			else 
				tcl.resultf("%d", comp_size_);
			return TCL_OK;
		} else if (strcmp(argv[1], "gen-obj-size") == 0) {
			tcl.resultf("%d", comp_size_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "get-next-objs") == 0) {
			PageID id;
			ClientPage::split_name(argv[2], id);
			// If we want simultaneous requests of multiple
			// objects, return a list; otherwise return a single
			// pageid. 
			for (int i = id.id_+1; i < num_pages_; i++) {
				tcl.resultf("%s %s:%d", tcl.result(), 
					    id.s_->name(), i);
			}
			return TCL_OK;
		} else if (strcmp(argv[1], "ranvar-main-age") == 0) {
			rvMainAge_ = 
				(RandomVariable*)TclObject::lookup(argv[2]);
			return TCL_OK;
		} else if (strcmp(argv[1], "ranvar-obj-age") == 0) {
			rvCompAge_ =
				(RandomVariable*)TclObject::lookup(argv[2]);
			return TCL_OK;
		} else if (strcmp(argv[1], "set-start-time") == 0) {
			double st = strtod(argv[2], NULL);
			start_time_ = st;
			end_time_ += st;
			return TCL_OK;
		} else if (strcmp(argv[1], "gen-init-modtime") == 0) {
			tcl.resultf("%.17g", Scheduler::instance().clock());
			return TCL_OK;
		} else if (strcmp(argv[1], "is-mainpage") == 0) {
			// Tell if the given page is a main page or an 
			// embedded object. 
			// XXX Here because we have only one page, so only 
			// page id 0 is the main page. If we have multiple 
			// pages, we need something else to do this.
			PageID t1;
			ClientPage::split_name(argv[2], t1);
			if (t1.id_ == 0)
				tcl.result("1");
			else 
				tcl.result("0");
			return TCL_OK;
		} else if (strcmp(argv[1], "get-mainpage") == 0) {
			// Get the main page of an embedded object
			// XXX Should maintain a mapping between embedded
			// objects and main pages. It can be an algorithmic
			// one, e.g., using page id intervals. It's simple 
			// here because we have only one page.
			PageID t1;
			ClientPage::split_name(argv[2], t1);
			tcl.resultf("%s:0", t1.s_->name());
			return TCL_OK;
		} else if (strcmp(argv[1], "get-obj-num") == 0) {
			// Returns the number of embedded objects of the page
			// given in argv[1]. Here because we have only one 
			// page, we return a fixed value.
			tcl.resultf("%d", num_pages_-1);
			return TCL_OK;
		}

	} else {
		// argc > 3
		if (strcmp(argv[1], "gen-modtime") == 0) {
			int id = atoi(argv[2]);
			if (id == 0) {
				if (rvMainAge_ == 0) {
				  tcl.add_errorf("%s: no page age generator", 
						 name_);
					return TCL_ERROR;
				}
				double mt = strtod(argv[3], NULL);
				tcl.resultf("%.17g", mt + rvMainAge_->value());
			} else {
				if (rvCompAge_ == 0) {
				   tcl.add_errorf("%s: no page age generator", 
						  name_);
				   return TCL_ERROR;
				}
				double mt = atoi(argv[3]);
				tcl.resultf("%.17g", mt + rvCompAge_->value());
			}
			return TCL_OK;
		} else if (strcmp(argv[1], "gen-obj-modtime") == 0) {
			if (rvCompAge_ == 0) {
				tcl.add_errorf("%s: no page age generator", 
					       name_);
				return TCL_ERROR;
			}
			double mt = atoi(argv[3]);
			tcl.resultf("%.17g", mt + rvCompAge_->value());
			return TCL_OK;
		}
	}

	return PagePool::command(argc, argv);
}


static class ClientPagePoolClass : public TclClass {
public:
        ClientPagePoolClass() : TclClass("PagePool/Client") {}
        TclObject* create(int, const char*const*) {
		return (new ClientPagePool());
	}
} class_clientpagepool_agent;

ClientPagePool::ClientPagePool()
{
	namemap_ = new Tcl_HashTable;
	Tcl_InitHashTable(namemap_, TCL_STRING_KEYS);
}

ClientPagePool::~ClientPagePool()
{
	if (namemap_ != NULL) {
		Tcl_DeleteHashTable(namemap_);
		delete namemap_;
	}
}

// In case client/cache/server needs details, e.g., page listing
int ClientPagePool::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "list-pages") == 0) {
			Tcl_HashEntry *he;
			Tcl_HashSearch hs;
			char *buf = new char[num_pages_*20];
			char *p = buf;
			for (he = Tcl_FirstHashEntry(namemap_, &hs); 
			     he != NULL;
			     he = Tcl_NextHashEntry(&hs)) {
				char* retVal = Tcl_GetHashKey(namemap_, he);
				// Convert name to a PageID
				PageID t1;
				ClientPage::split_name (retVal, t1);
#ifdef NEED_SUNOS_PROTOS
				sprintf(p, "%s:%-d ", t1.s_->name(),t1.id_);
				p += strlen(p);
#else
				p += sprintf(p,"%s:%-d ",t1.s_->name(),t1.id_);
#endif
			}
			tcl.resultf("%s", buf);
			delete []buf;
			return TCL_OK;
		}
	}
	return PagePool::command(argc, argv);
}

ClientPage* ClientPagePool::get_page(const char *name)
{
	PageID t1;
	ClientPage::split_name(name, t1);

	Tcl_HashEntry *he = Tcl_FindHashEntry(namemap_, name);
	if (he == NULL)
		return NULL;
	return (ClientPage *)Tcl_GetHashValue(he);
}

int ClientPagePool::get_pageinfo(const char *name, char *buf)
{
	ClientPage *pg = get_page(name);
	if (pg == NULL) 
		return -1;
	pg->print_info(buf);
	return 0;
}

ClientPage* ClientPagePool::enter_page(int argc, const char*const* argv)
{
	double mt = -1, et, age = -1, noc = 0;
	int size = -1;
	for (int i = 3; i < argc; i+=2) {
		if (strcmp(argv[i], "modtime") == 0)
			mt = strtod(argv[i+1], NULL);
		else if (strcmp(argv[i], "size") == 0) 
			size = atoi(argv[i+1]);
		else if (strcmp(argv[i], "age") == 0)
			age = strtod(argv[i+1], NULL);
		else if (strcmp(argv[i], "noc") == 0)
				// non-cacheable flag
			noc = 1;
	}
	// XXX allow mod time < 0 and age < 0!!
	if (size < 0) {
		fprintf(stderr, "PagePool %s: wrong information for page %s\n",
			name_, argv[2]);
		return NULL;
	}
	et = Scheduler::instance().clock();
	ClientPage* pg = new ClientPage(argv[2], size, mt, et, age);
	if (add_page(pg) < 0) {
		delete pg; 
		return NULL;
	}
	if (noc) 
		pg->set_uncacheable();
	return pg;
}

ClientPage* ClientPagePool::enter_page(const char *name, int size, double mt, 
				       double et, double age)
{
	ClientPage* pg = new ClientPage(name, size, mt, et, age);
	if (add_page(pg) < 0) {
		delete pg; 
		return NULL;
	}
	return pg;
}

// XXX We don't need parsing "noc" here because a non-cacheable
// page won't be processed by a cache.
ClientPage* ClientPagePool::enter_metadata(int argc, const char*const* argv)
{
	ClientPage *pg = enter_page(argc, argv);
	if (pg != NULL)
		pg->set_valid_hdr();
	return pg;
}

ClientPage* ClientPagePool::enter_metadata(const char *name, int size, 
					   double mt, double et, double age)
{
	ClientPage *pg = enter_page(name, size, mt, et, age);
	if (pg != NULL) 
		pg->set_valid_hdr();
	return pg;
}

int ClientPagePool::add_page(ClientPage* pg)
{
	if (pg == NULL)
		return -1;

	char buf[HTTP_MAXURLLEN];
	memset (buf, 0, sizeof(buf));
	pg->name(buf);

	int newEntry = 1;
	Tcl_HashEntry *he = Tcl_CreateHashEntry(namemap_, 
						buf,
						&newEntry);
	if (he == NULL)
		return -1;

	// XXX If cache replacement algorithm is added, should change 
	// cache size here!!
	if (newEntry) {
		Tcl_SetHashValue(he, (ClientData)pg);
		num_pages_++;
 	} else {
		// Replace the old one
		ClientPage *q = (ClientPage *)Tcl_GetHashValue(he);
		// XXX must copy the counter value
		pg->counter() = q->counter();
		// XXX must copy the mpush values
		if (q->is_mpush())
			pg->set_mpush(q->mpush_time());
		Tcl_SetHashValue(he, (ClientData)pg);
		delete q;
	}
	return 0;
}

int ClientPagePool::remove_page(const char *name)
{
	// Find out which client we are seeking
	Tcl_HashEntry *he = Tcl_FindHashEntry(namemap_, name);
	if (he == NULL)
		return -1;
	ClientPage *pg = (ClientPage *)Tcl_GetHashValue(he);
	Tcl_DeleteHashEntry(he);
	delete pg;
	num_pages_--;
	// XXX If cache replacement algorithm is added, should change
	// cache size here!!
	return 0;
}

int ClientPagePool::set_mtime(const char *name, double mt)
{
	ClientPage *pg = (ClientPage *)get_page(name);
	if (pg == NULL) 
		return -1;
	pg->mtime() = mt;
	return 0;
}

int ClientPagePool::get_mtime(const char *name, double& mt)
{
	ClientPage *pg = (ClientPage *)get_page(name);
	if (pg == NULL) 
		return -1;
	mt = pg->mtime();
	return 0;
}

int ClientPagePool::set_etime(const char *name, double et)
{
	ClientPage *pg = (ClientPage *)get_page(name);
	if (pg == NULL) 
		return -1;
	pg->etime() = et;
	return 0;
}

int ClientPagePool::get_etime(const char *name, double& et)
{
	ClientPage *pg = (ClientPage *)get_page(name);
	if (pg == NULL) 
		return -1;
	et = pg->etime();
	return 0;
}

int ClientPagePool::get_size(const char *name, int& size) 
{
	ClientPage *pg = (ClientPage *)get_page(name);
	if (pg == NULL) 
		return -1;
	size = pg->size();
	return 0;
}

int ClientPagePool::get_age(const char *name, double& age) 
{
	ClientPage *pg = (ClientPage *)get_page(name);
	if (pg == NULL) 
		return -1;
	age = pg->age();
	return 0;
}

void ClientPagePool::invalidate_server(int sid)
{
	Tcl_HashEntry *he;
	Tcl_HashSearch hs;
	ClientPage *pg;
	int i;

	for (i = 0, he = Tcl_FirstHashEntry(namemap_, &hs);
	     he != NULL;
	     he = Tcl_NextHashEntry(&hs), i++) {
		pg = (ClientPage *) Tcl_GetHashValue(he);
		if (pg->server()->id() == sid)
			pg->server_down();
	}
}


// Proxy traces. Request file format:
//
// [<time> <clientID> <serverID> <URL_ID>]
// i <Duration> <Number_of_unique_URLs>
//
// <time> is guaranteed to start from 0. It needs to be adjusted
//
// Page file format (sorted by access counts)
// 
// <serverID> <URL_ID> <PageSize> <AccessCount>

static class ProxyTracePagePoolClass : public TclClass {
public:
        ProxyTracePagePoolClass() : TclClass("PagePool/ProxyTrace") {}
        TclObject* create(int, const char*const*) {
		return (new ProxyTracePagePool());
	}
} class_ProxyTracepagepool_agent;

ProxyTracePagePool::ProxyTracePagePool() : 
	rvDyn_(NULL), rvStatic_(NULL), br_(0), 
	size_(NULL), reqfile_(NULL), req_(NULL), lastseq_(0)
{
}

ProxyTracePagePool::~ProxyTracePagePool()
{
	if (size_ != NULL) 
		delete []size_;
	if (reqfile_ != NULL) 
		fclose(reqfile_);
	if (req_ != NULL) {
		Tcl_DeleteHashTable(req_);
		delete req_;
	}
}

int ProxyTracePagePool::init_req(const char *fn) 
{
	reqfile_ = fopen(fn, "r");
	if (reqfile_ == NULL) {
		fprintf(stderr, 
		  "ProxyTracePagePool: couldn't open trace file %s\n", fn);
		return TCL_ERROR;
	}

	// Discover information about the trace, e.g., number of pages,
	// start time, end time, etc. They should be available at the 
	// first line of the trace file.
	return find_info();
}

int ProxyTracePagePool::find_info()
{
	// Read the last line of the file
	fseek(reqfile_, -128, SEEK_END);
	char buf[129];
	if (fread(buf, 1, 128, reqfile_) != 128) {
		fprintf(stderr,
			"ProxyTracePagePool: cannot read file information\n");
		return TCL_ERROR;
	}
	int i;
	// ignore the last RETURN
	buf[128] = 0;
	if (buf[127] == '\n')
		buf[127] = 0; 
	for (i = 127; i >= 0; i--)
		if (buf[i] == '\n') {
			i++; 
			break;
		}
	if (buf[i] != 'i') {
		fprintf(stderr, 
	"ProxyTracePagePool: trace file doesn't contain statistics.\n");
		abort();
	}
	double len;
	sscanf(buf+i+1, "%lf %i", &len, &num_pages_);
	duration_ = (int)ceil(len);
#if 0
	printf("ProxyTracePagePool: duration %d pages %u\n",
	       duration_, num_pages_);
#endif
	rewind(reqfile_);
	return TCL_OK;
}

// Load page size info. Assuming request stream has already been loaded
int ProxyTracePagePool::init_page(const char *fn)
{
	FILE *fp = fopen(fn, "r");
	if (fp == NULL) {
		fprintf(stderr, 
		  "ProxyTracePagePool: couldn't open trace file %s\n", fn);
		return TCL_ERROR;
	}
	if (size_ != NULL) 
		delete []size_;
	int* p = new int[num_pages_];
	size_ = p;
	for (int i = 0; i < num_pages_; i++, p++)
		fscanf(fp, "%*d %*d %d %*u\n", p);
	fclose(fp);
	return TCL_OK;
}

ProxyTracePagePool::ClientRequest* ProxyTracePagePool::load_req(int cid)
{
	// Find out which client we are seeking
	Tcl_HashEntry *he;
	ClientRequest *p;
	int dummy; 
	long key = cid;
	if ((he = Tcl_FindHashEntry(req_, (const char*)key)) == NULL) {
		// New entry
		p = new ClientRequest();
		p->seq_ = lastseq_++;
		he = Tcl_CreateHashEntry(req_, (const char*)key, &dummy);
		Tcl_SetHashValue(he, (const char*)p);
		// Search from the beginning of file for this new client
		fseek(reqfile_, 0, SEEK_SET);
	} else {
		p = (ClientRequest*)Tcl_GetHashValue(he);
		if (p->nrt_ == -1)
			// No more requests for this client
			return p;
		// Clear EOF status
		fseek(reqfile_, p->fpos_, SEEK_SET);
	}

	// Looking for the next available request for this client
	double nrt;
	int ncid = -1, nurl;
	char buf[256];
	while (fgets(buf, 256, reqfile_)) {
		if (isalpha(buf[0])) {
			// Last line, break;
			ncid = -1;
			break;
		}
		sscanf(buf, "%lf %d %*d %d\n", &nrt, &ncid, &nurl);
		if ((ncid % nclient_) == p->seq_)
			break;
	}
	if ((ncid % nclient_) != p->seq_)
		// Didn't find the next request for this client
		p->nrt_ = -1;
	else {
		p->nrt_ = nrt, p->nurl_ = nurl;
		p->nrt_ += start_time_;
	}
	p->fpos_ = ftell(reqfile_);
	return p;
}

// Provide a tcl interface compatible with MathPagePool
int ProxyTracePagePool::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();

	if (argc == 2) {
		if (strcmp(argv[1], "get-poolsize") == 0) { 
			tcl.resultf("%u", num_pages_);
			return TCL_OK;
		} else if (strcmp(argv[1], "get-start-time") == 0) {
			tcl.resultf("%.17g", start_time_);
			return TCL_OK;
		} else if (strcmp(argv[1], "get-duration") == 0) {
			tcl.resultf("%d", duration_);
			return TCL_OK;
		} else if (strcmp(argv[1], "bimodal-ratio") == 0) {
			tcl.resultf("%g", br_ / 10);
			return TCL_OK;
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "set-client-num") == 0) {
			// Set the number of clients it'll access
			// Cannot be changed once set
			if (req_ != NULL)
				return TCL_ERROR;
			int num = atoi(argv[2]);
			req_ = new Tcl_HashTable;
			Tcl_InitHashTable(req_, TCL_ONE_WORD_KEYS);
			nclient_ = num;
			return TCL_OK;
		} else if (strcmp(argv[1], "gen-request") == 0) {
			// Use client id to get a corresponding request
			int id = atoi(argv[2]);
			ClientRequest *p = load_req(id);
			if ((p->nrt_ >= 0) && 
			    (p->nrt_ < Scheduler::instance().clock())) {
				// XXX Do NOT treat this as an error, also
				// do NOT disable further requests from this 
				// client.
				fprintf(stderr,
					"%.17g: Wrong request time %g.\n",
					Scheduler::instance().clock(),
					p->nrt_);
				// XXX If it's a little bit older than current 
				// time, let it be a little bit later than now
				p->nrt_ = Scheduler::instance().clock()+0.001;
			}
			tcl.resultf("%lf %d", 
				    p->nrt_ - Scheduler::instance().clock(), 
				    p->nurl_);
			return TCL_OK;
		} else if (strcmp(argv[1], "gen-size") == 0) {
			int id = atoi(argv[2]);
			if ((id < 0) || (id > num_pages_)) {
				tcl.result("PagePool: id out of range.\n");
				return TCL_ERROR;
			}
			tcl.resultf("%d", size_[id]);
			return TCL_OK;
		} else if (strcmp(argv[1], "set-start-time") == 0) {
			start_time_ = strtod(argv[2], NULL);
			return TCL_OK;
		} else if (strcmp(argv[1], "bimodal-ratio") == 0) {
			// XXX Codes in Http/Server::gen-page{} also depends
			// on this dyn/static page algorithm. If this is 
			// changed, that instproc must be changed too.
			//
			// percentage of dynamic pages. E.g., 
			// if this ratio is 5, then page 0-4 is 
			// dynamic, and page 4-99 is static, and so on.
			double ratio = strtod(argv[2], NULL);
			//br_ = (int)ceil(ratio*100);
			br_ = (int)ceil(ratio*10);
			return TCL_OK;
		} else if (strcmp(argv[1], "ranvar-dp") == 0) {
			// Page mod ranvar for dynamic pages
			rvDyn_ = (RandomVariable*)TclObject::lookup(argv[2]);
			return TCL_OK;
		} else if (strcmp(argv[1], "ranvar-sp") == 0) {
			// page mod ranvar for static pages
			rvStatic_= (RandomVariable*)TclObject::lookup(argv[2]);
			return TCL_OK;
		} else if (strcmp(argv[1], "set-reqfile") == 0) {
			return init_req(argv[2]);
		} else if (strcmp(argv[1], "set-pagefile") == 0) {
			return init_page(argv[2]);
		} else if (strcmp(argv[1], "gen-init-modtime") == 0) {
			int id = atoi(argv[2]) % 10;
			if (id >= br_)
				// Static page
				tcl.result("0");
			else
				// Dynamic page
				tcl.resultf("%.17g", 
					    Scheduler::instance().clock());
			return TCL_OK;
		}
	} else {
		if (strcmp(argv[1], "gen-modtime") == 0) {
			if ((rvDyn_ == 0) || (rvStatic_ == 0)) {
				tcl.add_errorf("%s: no page age generator", 
					       name_);
				return TCL_ERROR;
			}
			// int id = atoi(argv[2]) % 100;
			int id = atoi(argv[2]) % 10;
			double mt = strtod(argv[3], NULL);
			if (id >= br_) 
				tcl.resultf("%.17g", mt + rvStatic_->value());
			else 
				tcl.resultf("%.17g", mt + rvDyn_->value());
			return TCL_OK;
		}
	}

	return PagePool::command(argc, argv);
}


// Proxy trace with special method for page modification
static class EPAPagePoolClass : public TclClass {
public:
	EPAPagePoolClass() : TclClass("PagePool/ProxyTrace/epa") {}
	TclObject* create(int, const char*const*) {
		return (new EPATracePagePool());
	}
} class_epapagepool_agent;

int EPATracePagePool::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "pick-pagemod") == 0) {
			if (rvDyn_ == 0) {
				tcl.add_errorf("%s: no page age generator",
					       name_);
				return (TCL_ERROR);
			}
			int j = (int)floor(rvDyn_->value());
			//fprintf(stderr, "mod id = %d\n", j/br_*10 + j % br_);
			tcl.resultf("%d", j/br_*10 + j % br_);
			return TCL_OK;
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "ranvar-dp") == 0) {
			rvDyn_ = (RandomVariable*)TclObject::lookup(argv[2]);
			if (rvDyn_ == 0) {
				tcl.add_errorf("%s: no page age generator",
					       name_);
				return (TCL_ERROR);
			}
			((UniformRandomVariable*)rvDyn_)->setmin(0);
			((UniformRandomVariable*)rvDyn_)->setmax(num_pages_/10*br_ + num_pages_%br_ - 1);
			return TCL_OK;
		}
	} else {
		if (strcmp(argv[1], "gen-modtime") == 0) {
			// Return a very large number
			tcl.resultf("%d", INT_MAX);
			return TCL_OK;
		}
	}
	return ProxyTracePagePool::command(argc, argv);
}

