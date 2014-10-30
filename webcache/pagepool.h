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
 * $Header: /cvsroot/nsnam/ns-2/webcache/pagepool.h,v 1.17 2005/09/18 23:33:35 tomh Exp $
 *
 */
//
// Definitions for class PagePool
//

#ifndef ns_pagepool_h
#define ns_pagepool_h

#include <stdio.h>
#include <limits.h>
#include <tcl.h>
#include <ranvar.h>
#include <tclcl.h>
#include "config.h"

enum WebPageType { HTML, MEDIA };

class Page {
public:
	Page(int size) : size_(size) {}
	virtual ~Page () {}
	int size() const { return size_; }
	int& id() { return id_; }
	virtual WebPageType type() const = 0; 	// Page type: HTML or MEDIA

protected:
	int size_;
	int id_;
};

class ServerPage : public Page {
public:
	ServerPage(int size, int id) : Page(size) {
		id_ = id, mtime_ = NULL, num_mtime_ = 0;
	}
	virtual ~ServerPage() {
		if (mtime_ != NULL) 
			delete []mtime_;
	}

	virtual WebPageType type() const { return HTML; }

	int& size() { return size_; }
	int& mtime(int n) { return mtime_[n]; }
	int& num_mtime() { return num_mtime_; }
	void set_mtime(int *mt, int n);

protected:
	int *mtime_;
	int num_mtime_;
};

class HttpApp;

// Page states
const int HTTP_PAGE_STATE_MASK	= 0x00FF;
const int HTTP_ALL_PAGE_STATES	= 0x00ff; // all page states
const int HTTP_VALID_PAGE	= 0x01;	// Valid page
const int HTTP_SERVER_DOWN	= 0x02;	// Server is down. Don't know if 
					// page is valid
const int HTTP_VALID_HEADER	= 0x04;	// Only meta-data is valid
const int HTTP_UNREAD_PAGE	= 0x08;	// Unread valid page

// Used only for server to manage its pages. A none-cacheable page won't be 
// stored by caches and clients.
const int HTTP_UNCACHEABLE	= 0x10; // Non-cacheable page

// Page actions
const int HTTP_PAGE_ACTION_MASK = 0xFF00; // Page action bit mask
const int HTTP_MANDATORY_PUSH	= 0x1000; // If the page is mandatory pushed

struct PageID {
	PageID() : s_(NULL), id_(0) {}
        PageID(int *buffer) {
	        PageID *realBuffer = reinterpret_cast <PageID *> (buffer);
		s_ = realBuffer->s_;
		id_ = realBuffer->id_;
	}
	PageID(HttpApp *app, int id) {
		s_ = app;
		id_ = id;
	}
	HttpApp* s_;
	int id_;
};

class ClientPage : public Page {
public:
	ClientPage(const char *n, int s, double mt, double et, double a);
	virtual ~ClientPage() {}

	virtual WebPageType type() const { return HTML; }
	virtual void print_info(char* buf);

	void name(char* buf);
	double& mtime() { return mtime_; }
	double& etime() { return etime_; }
	double& age() { return age_; }
	HttpApp* server() { return server_; }

	// Page becomes valid. Clear all other possible invalid bits
	void validate(double mtime) { 
		if (mtime_ >= mtime)
			abort(); // This shouldn't happen!
		// Clear server down bit
		clear_page_state(HTTP_SERVER_DOWN);
		set_page_state(HTTP_VALID_PAGE);
		mtime_ = mtime;
	}
	void invalidate(double mtime) { 
		if (mtime_ >= mtime)
			return;
		clear_page_state(HTTP_VALID_PAGE);
		clear_page_state(HTTP_VALID_HEADER);
		mtime_ = mtime;
	}
	int is_valid() const { 
		return (status_ & HTTP_VALID_PAGE);
	}
	int is_header_valid() const {
		return ((status_ & HTTP_VALID_PAGE) || 
			(status_ & HTTP_VALID_HEADER));
	}
	inline void set_valid_hdr() { 
		// XXX page invalid, but only header valid
		clear_page_state(HTTP_SERVER_DOWN);
		clear_page_state(HTTP_VALID_PAGE);
		set_page_state(HTTP_VALID_HEADER); 
	}

	inline void set_uncacheable() { 
		set_page_state(HTTP_UNCACHEABLE);
	}
	inline int is_uncacheable() {
		return (status_ & HTTP_UNCACHEABLE);
	}

	// Has nothing to do with valid/invalid/server_down etc. It can 
	// be combined with all other page status
	inline void set_unread() { 
		set_page_state(HTTP_UNREAD_PAGE); 
	}
	inline void set_read() { 
		clear_page_state(HTTP_UNREAD_PAGE);
	}
	inline int is_unread() { return (status_ & HTTP_UNREAD_PAGE); }

	inline int is_server_down() { return (status_ & HTTP_SERVER_DOWN); }
	inline void server_down() {
		// Set page as invalid
		// Don't change mtime, only change page status
		clear_page_state(HTTP_VALID_PAGE);
		clear_page_state(HTTP_VALID_HEADER);
		set_page_state(HTTP_SERVER_DOWN);
	}

	// Flags to indicate whether we want to do all push or selective push
	// If 0: selective push, otherwise all push
	static int PUSHALL_; 
	inline int& counter() { 
		if (PUSHALL_) counter_ = INT_MAX;
		return counter_;
	}
	inline int count_inval(int a, int th) { 
		if (PUSHALL_) 
			return INT_MAX;
		else {
			counter_ -= a; 
			if (counter_ < th) 
				counter_ = th;
			return counter_; 
		}
	}
	inline int count_request(int b, int th) { 
		if (PUSHALL_) 
			return INT_MAX;
		else {
			counter_ += b; 
			if (counter_ > th) 
				counter_ = th;
			return counter_; 
		}
	}
	inline void set_mpush(double time) { 
		set_page_action(HTTP_MANDATORY_PUSH), mpushTime_ = time; 
	}
	inline void clear_mpush() { clear_page_action(HTTP_MANDATORY_PUSH); }
	inline int is_mpush() { return status_ & HTTP_MANDATORY_PUSH; }
	inline double mpush_time() { return mpushTime_; }

	// Used to split page names into page identifiers
	static void split_name(const char* name, PageID& id);
	static void print_name(char* name, PageID& id);

protected:
	void set_page_state(int state) {
		status_ |= state;
	}
	void clear_page_state(int state) {
		status_ = status_ & ~state;
	}
	void set_page_action(int action) {
		status_ |= action;
	}
	void clear_page_action(int action) {
		status_ = status_ & ~action;
	}

	HttpApp* server_;
	double age_;
	double mtime_;	// modification time
	double etime_;	// entry time
	int status_;	// VALID or INVALID
	int counter_;	// counter for invalidation & request
	double mpushTime_;
};


// Abstract page pool, used for interface only
class PagePool : public TclObject {
public: 
	PagePool() : num_pages_(0), start_time_(INT_MAX), end_time_(INT_MIN) {}
	int num_pages() const { return num_pages_; }
protected:
	virtual int command(int argc, const char*const* argv);
	int num_pages_;
	double start_time_;
	double end_time_;
	int duration_;

	// Helper functions
	TclObject* lookup_obj(const char* name) {
		TclObject* obj = Tcl::instance().lookup(name);
		if (obj == NULL) 
			fprintf(stderr, "Bad object name %s\n", name);
		return obj;
	}
};

// Page pool based on real server traces

const int TRACEPAGEPOOL_MAXBUF = 4096;

// This trace must contain web page names and all of its modification times
class TracePagePool : public PagePool {
public:
	TracePagePool(const char *fn);
	virtual ~TracePagePool();
	virtual int command(int argc, const char*const* argv);

protected:
	Tcl_HashTable *namemap_, *idmap_;
	RandomVariable *ranvar_;

	ServerPage* load_page(FILE *fp);
	void change_time();
	int add_page(const char* pgname, ServerPage *pg);

	ServerPage* get_page(int id);
};

// Page pool based on mathematical models of request and page 
// modification patterns
class MathPagePool : public PagePool {
public:
	// XXX TBA: what should be here???
	MathPagePool() : rvSize_(0), rvAge_(0) { num_pages_ = 1; }

protected:
	virtual int command(int argc, const char*const* argv);
	// Single page
	RandomVariable *rvSize_;
	RandomVariable *rvAge_;
};

// Assume one main page, which changes often, and multiple component pages
class CompMathPagePool : public PagePool {
public:
	CompMathPagePool();

protected:
	virtual int command(int argc, const char*const* argv);
	RandomVariable *rvMainAge_;  // modtime for main page
	RandomVariable *rvCompAge_;  // modtime for component pages
	int main_size_, comp_size_;
};

class ClientPagePool : public PagePool {
public:
	ClientPagePool();
	virtual ~ClientPagePool();

	virtual ClientPage* enter_page(int argc, const char*const* argv);
	virtual ClientPage* enter_metadata(int argc, const char*const* argv);
	virtual ClientPage* enter_page(const char *name, int size, double mt, 
				       double et, double age);
	virtual ClientPage* enter_metadata(const char *name, int size, 
					   double mt, double et, double age);
	virtual int remove_page(const char *name);

	void invalidate_server(int server_id);

	ClientPage* get_page(const char *name);
	int get_mtime(const char *name, double &mt);
	int set_mtime(const char *name, double mt);
	int exist_page(const char *name) { return (get_page(name) != NULL); }
	int get_size(const char *name, int &size);
	int get_age(const char *name, double &age);
	int get_etime(const char *name, double &et);
	int set_etime(const char *name, double et);
	int get_pageinfo(const char *name, char *buf);

	virtual int command(int argc, const char*const* argv);

protected:

	int add_page(ClientPage *pg);
	Tcl_HashTable *namemap_;
};

// This is *not* designed for BU trace files. We should write a script to 
// transform BU traces to a single trace file with the following format:
//
// <client id> <page id> <time> <size>
//
// Q: How would we deal with page size changes? 
// What if simulated response time
// is longer and a real client request for the same page happened before the 
// simulated request completes? 

class ProxyTracePagePool : public PagePool {
public:
	ProxyTracePagePool();
// : rvDyn_(NULL), rvStatic_(NULL), br_(0), 
//		size_(NULL), reqfile_(NULL), req_(NULL), lastseq_(0)
//		{}
	virtual ~ProxyTracePagePool();
	virtual int command(int argc, const char*const* argv);

protected:
	// How would we handle different types of page modifications? How 
	// to integrate bimodal, and multi-modal distributions?
	int init_req(const char *fn);
	int init_page(const char *fn);
	int find_info();

	RandomVariable *rvDyn_, *rvStatic_;
	int br_; 		// bimodal ratio
	int *size_; 		// page sizes
	FILE *reqfile_;		// request stream of proxy trace

	struct ClientRequest {
		ClientRequest() : seq_(0), nrt_(0), nurl_(0), fpos_(0)
			{}
		int seq_;	// client sequence number, used to match 
				// client ids in the trace file
		double nrt_;	// next request time
		int nurl_; 	// next request url
		long fpos_;	// position in file of its next request
	};
	Tcl_HashTable *req_;	// Requests table
	int nclient_, lastseq_;
	ClientRequest* load_req(int cid);
};

class EPATracePagePool : public ProxyTracePagePool {
public:
	virtual int command(int argc, const char*const* argv);
};

#endif //ns_pagepool_h
