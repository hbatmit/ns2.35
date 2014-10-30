/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * media-app.h
 * Copyright (C) 1997 by the University of Southern California
 * $Id: media-app.h,v 1.12 2005/08/25 18:58:10 johnh Exp $
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
// Media applications: wrappers of transport agents for multimedia objects
// 
// Their tasks are to receive 'data' from underlying transport agents, 
// and to notify "above" applications that such data has arrived. 
//
// When required (by RapAgent), it also provides callbacks to the 
// transport agent, and contact the above application on behalf of the 
// transport agent.
//
// $Header: /cvsroot/nsnam/ns-2/rap/media-app.h,v 1.12 2005/08/25 18:58:10 johnh Exp $

#ifndef ns_media_app_h
#define ns_media_app_h

#include <stdlib.h>
#include <tcl.h>
#include "config.h"
#include "agent.h"
#include "app.h"
#include "webcache/http-aux.h"
#include "rap/rap.h"
#include "rap/utilities.h"


class HttpMediaData;

// Fixed length segment in a multimedia stream
class MediaSegment : public DoubleListElem {
public: 
	MediaSegment() : start_(0), end_(0), flags_(0) {}
	MediaSegment(int start, int end) : start_(start),end_(end),flags_(0) {}
	MediaSegment(const HttpMediaData& d);
	MediaSegment(const MediaSegment& s) {
		// XXX Don't copy pointers (prev_,next_)?
		start_ = s.start_;
		end_ = s.end_;
		flags_ = s.flags_;
	}

	int start() const { return start_; }
	int end() const { return end_; }
	int datasize() const { return end_ - start_; }
	MediaSegment* next() const { 
		return (MediaSegment *)DoubleListElem::next(); 
	}
	MediaSegment* prev() const { 
		return (MediaSegment *)DoubleListElem::prev(); 
	}

	void set_start(int d) { start_ = d; }
	void set_end(int d) { end_ = d; }
	void set_datasize(int d) { end_ = start_ + d; }
	// Advance the segment by size
	void advance(int size) { start_ += size, end_ = start_ + size; }

	int in(const MediaSegment& s) const {
		return ((start_ >= s.start_) && (end_ <= s.end_));
	}
	int before(const MediaSegment& s) const {
		return (end_ <= s.start_);
	}
	int overlap(const MediaSegment& s) const {
		assert(s.start_ <= s.end_);
		return ((s.end_ >= start_) && (s.start_ <= end_));
	}
	// Return the overlapping size between the two
	int merge(const MediaSegment& s) {
		if ((s.end_ < start_) || (s.start_ > end_))
			// No overlap
			return 0;
		int size = datasize() + s.datasize();
		if (s.start_ < start_) start_ = s.start_;
		if (s.end_ > end_) end_ = s.end_;
		assert(size >= datasize());
		return (size - datasize());
	}
	// Return the amount of data evicted
	int evict_tail(int sz) {
		// Adjust both the size and playout time
#ifdef MCACHE_DEBUG
		fprintf(stderr, "evicted (%d, %d) ", end_-sz, end_);
#endif
		if (datasize() >= sz) 
			end_ -= sz;
		else {
			sz = datasize();
			end_ = start_;
		}
		return sz;
	}
	// Return the amount of data evicted
	int evict_head(int sz) {
		// Adjust both the size and playout time
		if (datasize() >= sz) 
			start_ += sz;
		else {
			sz = datasize();
			end_ = start_;
		}
		return sz;
	}
	int is_empty() const { return end_ == start_; }

	// Whether this is the last segment of the available data
	enum { MS_LAST = 1, MS_PREF = 2 };
	int is_last() const { return (flags_ & MS_LAST); }
	void set_last() { flags_ |= MS_LAST; }
	int is_pref() const { return (flags_ & MS_PREF); }
	void set_pref() { flags_ |= MS_PREF; }

private: 
	int start_;
	int end_;
	int flags_;
};

// Maintains received segments of every layer
class MediaSegmentList : public DoubleList {
public:
	MediaSegmentList() : DoubleList(), length_(0) {}

	int length() const { return length_; }
	void add(const MediaSegment& s);
	int in(const MediaSegment& s);
	MediaSegment get_nextseg(const MediaSegment& s);
	int evict_tail(int size);
	int evict_head(int size);
	int evict_head_offset(int offset);
	int overlap_size(const MediaSegment& s) const;
	MediaSegmentList check_holes(const MediaSegment& s);

	char* dump2buf();
	virtual void destroy() {
		DoubleList::destroy();
		length_ = 0;
	}

	// Debug only
	void print(void);
	int getsize();
	void check_integrity();

protected:
	void merge_seg(MediaSegment* seg);

private:
	int length_;
};


// Represents a multimedia segment transmitted through the network
class HttpMediaData : public HttpData {
private:
	char page_[HTTP_MAXURLLEN];	// Page ID
	char sender_[HTTP_MAXURLLEN];	// Sender name
	int layer_;			// Layer id. 0 if no layered encoding
	int st_;			// Segment start time
	int et_; 			// Segment end time
	int flags_; 			// flags: end of all data, etc.
	Application* conid_;		// Connection ID. Used for statistics
public:
	struct hdr {
		char sender_[HTTP_MAXURLLEN];
		char page_[HTTP_MAXURLLEN];
		int layer_;
		int st_, et_;
		int flags_;
	};

public:
	HttpMediaData(const char* sender, const char* name, int layer, 
		      int st, int et);
	HttpMediaData(HttpMediaData& d) : HttpData(d) {
		layer_ = d.layer_;
		st_ = d.st_;
		et_ = d.et_;
		flags_ = d.flags_;
		strcpy(page_, d.page_);
		strcpy(sender_, d.sender_);
	}

	virtual int size() const { return HttpData::size() + sizeof(hdr); }
	virtual AppData* copy() { return (new HttpMediaData(*this)); }

	int st() const { return st_; }
	int et() const { return et_; }
	int datasize() const { return et_ - st_; }
	int layer() const { return layer_; }
	const char* page() const { return page_; }
	const char* sender() const { return sender_; }

	Application* conid() { return conid_; }
	void set_conid(Application* c) { conid_ = c; }

	// flags
	// MD_LAST: last segment of this layre
	// MD_FINISH: completed the entire stream
	enum {
		MD_LAST = 1, 
		MD_FINISH = 2,
		MD_PREFETCH = 4
	}; 

	// Whether this is the last data segment of the layer
	int is_last() const { return (flags_ & MD_LAST); }
	void set_last() { flags_ |= MD_LAST; }
	int is_finished() const { return (flags_ & MD_FINISH); }
	void set_finish() { flags_ |= MD_FINISH; }
	int is_pref() const { return (flags_ & MD_PREFETCH); }
	void set_pref() { flags_ |= MD_PREFETCH; }
};


const int MEDIAREQ_GETSEG   	= 1;
const int MEDIAREQ_CHECKSEG 	= 2;

const int MEDIAREQ_SEGAVAIL 	= 3;
const int MEDIAREQ_SEGUNAVAIL 	= 4;

// It provides a MediaApp two types of requests: check data availability and 
// request data. It won't be sent in a packet so we don't need copy(). 
class MediaRequest : public AppData {
private:
	int request_; 			// Request code
	char name_[HTTP_MAXURLLEN];	// Page URL
	int layer_;			// Layer ID
	u_int st_;			// Start playout time
	u_int et_;			// End playout time
	Application* app_;		// Calling application
public:
	MediaRequest(int rc) : AppData(MEDIA_REQUEST), request_(rc) {}
	MediaRequest(const MediaRequest& r) : AppData(MEDIA_REQUEST) {
		request_ = r.request();
		st_ = r.st();
		et_ = r.et();
		layer_ = r.layer();
		strcpy(name_, r.name());
	}
	// We don't need it, but have to declare.
	virtual AppData* copy() { abort(); return NULL; }

	int request() const { return request_; }
	int st() const { return st_; }
	int et() const { return et_; }
	int datasize() const { return et_ - st_; }
	int layer() const { return layer_; }
	const char* name() const { return name_; }
	Application* app() const { return app_; }

	// These functions allow the caller to fill in only the 
	// necessary fields
	void set_st(int d) { st_ = d; }
	void set_et(int d) { et_ = d; }
	void set_datasize(int d) { et_ = st_ + d; }
	void set_name(const char *s) { strcpy(name_, s); }
	void set_layer(int d) { layer_ = d; }
	void set_app(Application* a) { app_ = a; }
};



// Maximum number of layers for all streams in the simulation
const int MAX_LAYER = 10;

// XXX A media app is only responsible for transmission of a single 
// media stream!! Once the transmission is done, the media app will be 
// deleted.
class MediaApp : public Application {
public:
	MediaApp(const char* page);

	virtual AppData* get_data(int& size, AppData* req_data = 0);
	virtual void process_data(int size, AppData* data) {
		send_data(size, data);
	}
	void set_page(const char* pg) { strcpy(page_, pg); }

	void log(const char* fmt, ...);
	int command(int argc, const char*const* argv);

protected:
	virtual void start();
	virtual void stop();

	// Helper function
	RapAgent* rap() { return (RapAgent*)agent_; }

	int seg_size_; 			// data segment size
	char page_[HTTP_MAXURLLEN];
	MediaSegment data_[MAX_LAYER];	// Pointer to next data to be sent
	Tcl_Channel log_; 		// log file

	// XXX assuming all streams in the simulation have the same
	// number of layers.
	int num_layer_;
private:
	int last_layer_;
};



class QA; 

class QATimer : public TimerHandler {
public:
	QATimer(QA *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	QA *a_;
};

const double QA_EPSILON = 1e-6;

class QA : public MediaApp { 
public:
	QA(const char *page);
	virtual ~QA();

	virtual AppData* get_data(int& size, AppData* req_data = 0);
	void UpdateState();
	double UpdateInterval() { return rap()->srtt(); } 

protected:
	virtual int command(int argc, const char*const* argv);
	virtual void stop();

	// Helper functions
	void check_availability(int layer, const MediaSegment& s);
	RapAgent* rap() { return (RapAgent*)agent_; }

	// Misc helpers
	inline double MWM(double srtt) {
		return 2 * LAYERBW_ * srtt;
	}
	inline double rate() { 
		return (double)seg_size_ / rap()->ipg();
	}

	// Calculate area of a triangle for a given side and slope
	inline double BufNeed(double side, double slope) {
		return (side <= 0) ? 0.0 : ((side*side) / (2.0*slope));
	}
	int AllZero(double *arr, int len);
	double TotalBuf(int n, double *buffer);
	AppData* output(int& size, int layer);
	void DumpInfo(double t, double last_t, double rate, 
		      double avgrate, double srtt);
	double bufOptScen1(int layer, int layers, double currrate, 
			   double slope, int backoffs);
	double bufOptScen2(int layer, int layers, double currrate, 
			   double slope, int backoffs);
	void drain_buf(double* DrainArr, double bufToDrain, 
		       double* FinalDrainArray, double* bufAvail, 
		       int layers, double rate, double srtt);
	void DrainPacket(double bufToDrain, double* FinalDrainArray, 
			 int layers, double rate, double srtt, 
			 double* FinalBuffer);

	// Ack feedback information
	void DrainBuffers();

	// Debugging output
	void debug(const char* fmt, ...);
	void panic(const char* fmt, ...);
	void check_layers(int layer, MediaSegment& tmp);

	// Data members
	int layer_;
	double    playTime_; // playout time of the receiver
	double    startTime_; // Absoloute time when playout started

	// Internal state info for QA
	double   buffer_[MAX_LAYER];
	double   drained_[MAX_LAYER]; 
	double   bw_[MAX_LAYER];
	int	 playing_[MAX_LAYER]; 
	int	 sending_[MAX_LAYER];
	QATimer* updTimer_;

	// Average transmission rate and its moving average weight
	// Measured whenever a segment is sent out (XXX why??)
	double   avgrate_; 
	double   rate_weight_;

	// Variables related to playout buffer management
	int      poffset_;  /* The playout offset: estimation of client 
			       playout time */
	// Linked list keeping track of holes between poffset_ and current
	// transmission pointer
	MediaSegmentList outlist_[MAX_LAYER];
	// The end offset of the prefetch requests. Used to avoid redundant
	// prefetching requests
	int pref_[MAX_LAYER];

	// OTcl-bound variables
	int debug_;  		// Turn on/off debug output
	double pref_srtt_;	// Prefetching SRTT
	int LAYERBW_;
	int MAXACTIVELAYERS_;
	double SRTTWEIGHT_;
	int SMOOTHFACTOR_;
	int MAXBKOFF_;
}; 

#endif // ns_media_app_h
