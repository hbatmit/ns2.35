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
 * $Header: /cvsroot/nsnam/ns-2/webcache/http-aux.h,v 1.17 2005/08/26 05:05:31 tomh Exp $
 */

//
// Auxiliary classes for HTTP multicast invalidation proxy cache
//

#ifndef ns_http_aux_h
#define ns_http_aux_h

#include <tclcl.h>

#include "random.h"
#include "ns-process.h"
#include "pagepool.h"
#include "timer-handler.h"

const int HTTP_HBEXPIRE_COUNT	= 5; // How many lost HBs mean disconnection?
const int HTTP_HBINTERVAL	= 60;// Heartbeat intervals (seconds)
const int HTTP_UPDATEINTERVAL	= 5; // Delays to push/v1 (seconds)

class HttpMInvalCache;

// Used for caches to keep track of invalidations
class InvalidationRec {
public:
	InvalidationRec(const char *pid, double mtime, char updating = 0) {
		pg_ = new char[strlen(pid) + 1];
		strcpy(pg_, pid);
		mtime_ = mtime;
		scount_ = HTTP_HBEXPIRE_COUNT;
		updating_ = updating;
		next_ = 0, prev_ = 0;
	}
	virtual ~InvalidationRec() {
		delete []pg_;
	}

	const char* pg() const { return pg_; }
	double mtime() const { return mtime_; }
	char updating() const { return updating_; }
	int scount() const { return scount_; }
	InvalidationRec* next() { return next_; } 

	void reset(double mtime) {
		scount_ = HTTP_HBEXPIRE_COUNT;
		mtime_ = mtime;
	}
	int dec_scount() { return --scount_; }
	void set_updating() { updating_ = 1; }
	void clear_updating() { updating_ = 0; }

	void insert(InvalidationRec **head) {
		next_ = *head;
		if (next_ != 0)
			next_->prev_ = &next_;
		prev_ = head;
		*head = this;
	}
	void detach() {
		if (prev_ != 0)
			*prev_ = next_;
		if (next_ != 0)
			next_->prev_ = prev_;
	}

	friend class HttpMInvalCache;

protected:
	char *pg_;
	double mtime_;
	char updating_;	// 1 if an update is going to be sent soon
	int scount_;	// Times that an invalidation needs to be multicast
	InvalidationRec *next_;
	InvalidationRec **prev_;
};

class HBTimer : public TimerHandler {
public: 
	HBTimer(HttpMInvalCache *a, double interval) : TimerHandler() { 
		a_ = a, interval_ = interval; 
	}
	void set_interval(double interval) { interval_ = interval; }
	double get_interval() const { return interval_; }
	double next_interval() {
		return interval_ * (1 + Random::uniform(-0.1,0.1));
	}
	void sched() { TimerHandler::sched(interval_); }
	void resched() {
		TimerHandler::resched(next_interval());
	}
protected: 
	virtual void expire(Event *e);
	virtual void handle(Event *e) {
		TimerHandler::handle(e);
		resched();
	}
	HttpMInvalCache *a_;
	double interval_;
};

class PushTimer : public TimerHandler {
public:
	PushTimer(HttpMInvalCache *a, double itv) : 
		TimerHandler(), a_(a), interval_(itv) {}
	void set_interval(double itv) { interval_ = itv; } 
	double get_interval() const { return interval_; }
	void sched() {
		TimerHandler::sched(interval_);
	}
protected:
	virtual void expire(Event *e);
	HttpMInvalCache *a_;
	double interval_;
};

class LivenessTimer : public TimerHandler {
public:
	LivenessTimer(HttpMInvalCache *a, double itv, int nbr) :
		TimerHandler(), a_(a), nbr_(nbr), interval_(itv) {}
	void sched() { TimerHandler::sched(interval_); }
	void resched() { TimerHandler::resched(interval_); }
protected:
	virtual void expire(Event *e);
	HttpMInvalCache *a_;	// The cache to be alerted
	int nbr_;		// Neighbor cache id
	double interval_;
};


// XXX ADU defined in app-connector.h

const int HTTPDATA_COST		= 8;

// User-level packets
class HttpData : public AppData {
private:
	int id_;	// ID of the sender

public:
	HttpData() : AppData(HTTP_DATA) {}
	HttpData(AppDataType t, int d) : AppData(t) { id_ = d; }
	HttpData(HttpData& d) : AppData(d) { id_ = d.id_; }

	inline int& id() { return id_; }
	virtual int size() const { return sizeof(HttpData); }
	virtual int cost() const { return HTTPDATA_COST; }
	virtual AppData* copy() { return (new HttpData(*this)); }
};



// HTTP data during normal request and response: containing a tcl command
class HttpNormalData : public HttpData {
private: 
	char *str_;
	int strlen_;
	int cost_;
public:
	// XXX Whoever sends data should allocate memory to store the string.
	// Whoever receives this object should delete it to free the string.
	HttpNormalData(int id, int cost, const char *str) : 
		HttpData(HTTP_NORMAL, id) {
		if ((str == NULL) || (*str == 0))
			strlen_ = 0;
		else
			strlen_ = strlen(str) + 1;
		if (strlen_ > 0) {
			str_ = new char[strlen_];
			strcpy(str_, str);
		} else
			str_ = NULL;
		cost_ = cost;
	}
	HttpNormalData(HttpNormalData& d) : HttpData(d) {
		cost_ = d.cost_;
		strlen_ = d.strlen_;
		if (strlen_ > 0) {
			str_ = new char[strlen_];
			strcpy(str_, d.str_);
		} else
			str_ = NULL;
	}
	virtual ~HttpNormalData() {
		if (str_ != NULL)
			delete []str_;
	}

	virtual int size() const {
		// Intentially use sizeof(int) instead of 2*sizeof(int)
		return (sizeof(HttpData)+sizeof(int)+strlen_);
	}
	virtual int cost() const { return cost_; }
	char* str() const { return str_; }
	virtual AppData* copy() {
		return (new HttpNormalData(*this));
	}
};



// XXX assign cost to a constant so as to be more portable
const int HTTPHBDATA_COST = 32;
const int HTTP_MAXURLLEN = 20;

// Struct used to pack invalidation records into packets
class HttpHbData : public HttpData {
protected:
	struct InvalRec {
		char pg_[HTTP_MAXURLLEN];	// Maximum page id length
		double mtime_;
		// Used only to mark that this page will be send in the 
		// next multicast update. The updating field in this agent 
		// will only be set after it gets the real update.
		int updating_; 
		void copy(InvalidationRec *v) {
			strcpy(pg_, v->pg());
			mtime_ = v->mtime();
			updating_ = v->updating();
		}
		InvalidationRec* copyto() {
			return (new InvalidationRec(pg_, mtime_, updating_));
		}
	};

private:
	int num_inv_;
	InvalRec* inv_rec_;
	inline InvalRec* inv_rec() { return inv_rec_; }

public:
	HttpHbData(int id, int n) : 
		HttpData(HTTP_INVALIDATION, id), num_inv_(n) {
		if (num_inv_ > 0)
			inv_rec_ = new InvalRec[num_inv_];
		else
			inv_rec_ = NULL;
	}
	HttpHbData(HttpHbData& d) : HttpData(d) {
		num_inv_ = d.num_inv_;
		if (num_inv_ > 0) {
			inv_rec_ = new InvalRec[num_inv_];
			memcpy(inv_rec_, d.inv_rec_,num_inv_*sizeof(InvalRec));
		} else
			inv_rec_ = NULL;
	}
	virtual ~HttpHbData() { 
		if (inv_rec_ != NULL)
			delete []inv_rec_; 
	}

	virtual int size() const {
		return HttpData::size() + num_inv_*sizeof(InvalRec);
	}
	// XXX byte cost to appear in trace file
	virtual int cost() const { 
		// Minimum size 1 so that TCP will send a packet
		return (num_inv_ == 0) ? 1 : (num_inv_*HTTPHBDATA_COST); 
	}
	virtual AppData* copy() {
		return (new HttpHbData(*this));
	}

	inline int& num_inv() { return num_inv_; }
	inline void add(int i, InvalidationRec *r) {
		inv_rec()[i].copy(r);
	}
	inline char* rec_pg(int i) { return inv_rec()[i].pg_; }
	inline double& rec_mtime(int i) { return inv_rec()[i].mtime_; }
	void extract(InvalidationRec*& ivlist);
};



class HttpUpdateData : public HttpData {
protected:
	// Pack page updates to be pushed to caches
	struct PageRec {
		char pg_[HTTP_MAXURLLEN];
		double mtime_;
		double age_;
		int size_;
		void copy(ClientPage *p) {
			p->name(pg_);
			mtime_ = p->mtime();
			age_ = p->age();
			size_ = p->size();
		}
	};
private:
	int num_;
	int pgsize_;
	PageRec *rec_;
	inline PageRec* rec() { return rec_; }
public:
	HttpUpdateData(int id, int n) : HttpData(HTTP_UPDATE, id) {
		num_ = n;
		pgsize_ = 0;
		if (num_ > 0)
			rec_ = new PageRec[num_];
		else
			rec_ = NULL;
	}
	HttpUpdateData(HttpUpdateData& d) : HttpData(d) {
		num_ = d.num_;
		pgsize_ = d.pgsize_;
		if (num_ > 0) {
			rec_ = new PageRec[num_];
			memcpy(rec_, d.rec_, num_*sizeof(PageRec));
		} else
			rec_ = NULL;
	}
	virtual ~HttpUpdateData() {
		if (rec_ != NULL)
			delete []rec_;
	}

	virtual int size() const { 
		return HttpData::size() + 2*sizeof(int)+num_*sizeof(PageRec); 
	}
	virtual int cost() const { return pgsize_; }
	virtual AppData* copy() {
		return (new HttpUpdateData(*this));
	}

	inline int num() const { return num_; }
	inline int pgsize() const { return pgsize_; }

	inline void set_pgsize(int s) { pgsize_ = s; }
	inline void add(int i, ClientPage *p) {
		rec()[i].copy(p);
		pgsize_ += p->size();
	}

	inline char* rec_page(int i) { return rec()[i].pg_; }
	inline int& rec_size(int i) { return rec()[i].size_; }
	inline double& rec_age(int i) { return rec()[i].age_; }
	inline double& rec_mtime(int i) { return rec()[i].mtime_; }
};



const int HTTPLEAVE_COST = 4;

// Message: server leave
class HttpLeaveData : public HttpData {
private:
	int num_;
	int* rec_; // array of server ids which are out of contact
	inline int* rec() { return rec_; }
public:
	HttpLeaveData(int id, int n) : HttpData(HTTP_LEAVE, id) {
		num_ = n;
		if (num_ > 0)
			rec_ = new int[num_];
		else
			rec_ = NULL;
	}
	HttpLeaveData(HttpLeaveData& d) : HttpData(d) {
		num_ = d.num_;
		if (num_ > 0) {
			rec_ = new int[num_];
			memcpy(rec_, d.rec_, num_*sizeof(int));
		} else
			rec_ = NULL;
	}
	virtual ~HttpLeaveData() { 
		if (rec_ != NULL)
			delete []rec_; 
	}

	virtual int size() const { 
		return HttpData::size() + (num_+1)*sizeof(int);
	}
	virtual int cost() const { return num_*HTTPLEAVE_COST; }
	virtual AppData* copy() {
		return (new HttpLeaveData(*this));
	}

	inline int num() const { return num_; }
	inline void add(int i, int id) {
		rec()[i] = id;
	}
	inline int rec_id(int i) { return rec()[i]; }
};


// Auxliary class
class NeighborCache {
public:
	NeighborCache(HttpMInvalCache*c, double t, LivenessTimer *timer) : 
		cache_(c), time_(t), down_(0), timer_(timer) {}
	~NeighborCache();

	double time() { return time_; }
	void reset_timer(double time) { 
		time_ = time, timer_->resched(); 
	}
	int is_down() { return down_; }
	void down() { down_ = 1; }
	void up() { down_ = 0; }
	int num() const { return sl_.num(); }
	HttpMInvalCache* cache() { return cache_; }
	void pack_leave(HttpLeaveData&);
	int is_server_down(int sid);
	void server_down(int sid);
	void server_up(int sid);
	void invalidate(HttpMInvalCache *c);
	void add_server(int sid);

	// Maintaining neighbor cache timeout entries
	struct ServerEntry {
		ServerEntry(int sid) : server_(sid), next_(NULL), down_(0) {}
		int server() { return server_; }
		ServerEntry* next() { return next_; }
		int is_down() { return down_; }
		void down() { down_ = 1; }
		void up() { down_ = 0; }

		int server_;
		ServerEntry *next_;
		int down_;
	};
	// We cannot use template. :(((
	struct ServerList {
		ServerList() : head_(NULL), num_(0) {}
		void insert(ServerEntry *s) {
			s->next_ = head_;
			head_ = s;
			num_++;
		}
		// We don't need a detach()
		ServerEntry* gethead() { return head_; } // For iterations
		int num() const { return num_; }
		ServerEntry *head_;
		int num_;
	};

protected:
	HttpMInvalCache* cache_;
	double time_;
	int down_;
	ServerList sl_;
	LivenessTimer *timer_;
};

#endif // ns_http_aux_h
