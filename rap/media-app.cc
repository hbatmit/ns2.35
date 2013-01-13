/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * media-app.cc
 * Copyright (C) 1997 by the University of Southern California
 * $Id: media-app.cc,v 1.15 2011/10/02 22:32:34 tom_henderson Exp $
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
// Implementation of media application
//
// $Header: /cvsroot/nsnam/ns-2/rap/media-app.cc,v 1.15 2011/10/02 22:32:34 tom_henderson Exp $

#include <stdarg.h>

#include "template.h"
#include "media-app.h"
#include "utilities.h"


//----------------------------------------------------------------------
// Classes related to a multimedia object
//
// MediaSegment
// MediaSegmentList: segments in a layer
// MediaPage: a stored multimedia object (stream)
//----------------------------------------------------------------------

MediaSegment::MediaSegment(const HttpMediaData& d) : flags_(0)
{
	start_ = d.st();
	end_ = d.et();
	if (d.is_last())
		set_last();
	if (d.is_pref())
		set_pref();
}

void MediaSegmentList::add(const MediaSegment& s) 
{
	MediaSegment* tmp = (MediaSegment *)head_;
	while ((tmp != NULL) && (tmp->before(s))) {
		tmp = tmp->next();
	}

	// Append at the tail, or the first element in list
	if (tmp == NULL) {
		length_ += s.datasize();
		if ((tail_ != NULL) && ((MediaSegment *)tail_)->overlap(s)) 
			// Don't need to merge because it's merged at the end
			((MediaSegment*)tail_)->merge(s);
		else {
			MediaSegment *p = new MediaSegment(s);
			if (head_ == NULL)
				head_ = tail_ = p;
			else 
				append(p, tail_);
		}
		if (getsize() != length_) {
			fprintf(stderr, 
				"MediaSegmentList corrupted: Point 1.\n");
			abort();
		}
		return;
	}

	// Update total stored length ONLY IF s is not in tmp.
	if (tmp->in(s)) {
		fprintf(stderr, 
			"MediaSegmentList: get a seg (%d %d) which is already in cache!\n",
			s.start(), s.end());
		fprintf(stderr, "List contents: ");
		print();
#if 0 
		//Tcl::instance().eval("[Simulator instance] flush-trace");
		//abort();
#endif
		// XXX Don't abort, simply continue
		return;
	}

	// Insert a MediaSegment into list. Note: Don't do merge!
	if (tmp->overlap(s)) {
		length_ += (s.datasize() - tmp->merge(s));
	} else {
		MediaSegment *p = new MediaSegment(s);
		insert(p, tmp);
		tmp = p;
		length_ += s.datasize();
	}

	if (getsize() != length_) {
		fprintf(stderr, "MediaSegmentList corrupted: Point 2.\n");
		abort();
	}

	merge_seg(tmp);

	if (getsize() != length_) {
		fprintf(stderr, "MediaSegmentList corrupted: Point 3.\n");
		abort();
	}
}

void MediaSegmentList::merge_seg(MediaSegment* tmp)
{
	// See if <tmp> can be merged with next segments
	MediaSegment *q = tmp->next();
	while (q && q->overlap(*tmp)) {
#if 1
		if ((tmp->start() == q->start()) && (tmp->end() == q->end())) {
			abort();
		}
#endif
		tmp->merge(*q);
		detach(q);
		delete q;
		q = tmp->next();
	}
	// See if <tmp> can be merged with previous segments
	q = tmp->prev();
	while (q && q->overlap(*tmp)) {
		tmp->merge(*q);
		assert(tail_ != q);
		detach(q);
		delete q;
		q = tmp->prev();
	}
}

int MediaSegmentList::in(const MediaSegment& s)
{
	MediaSegment* tmp = (MediaSegment *)head_;
	while ((tmp != NULL) && (tmp->before(s)))
		tmp = tmp->next();

	// If all segments are before s, or the first segment which isn't 
	// before s doesn't overlap with s, s isn't in this list.
	if ((tmp == NULL) || !s.in(*tmp))
		return 0;
	else 
		return 1;
}

// Get the next segment which is not before 's', but with the same size
// as the given 's'. This segment may not overlap with s. 
MediaSegment MediaSegmentList::get_nextseg(const MediaSegment& s) 
{
	MediaSegment res(0, 0); // If unsuccessful, return start() = 0

	MediaSegment* tmp = (MediaSegment *)head_;
	while ((tmp != NULL) && (tmp->before(s))) 
		tmp = tmp->next();
	if (tmp == NULL) {
		res.set_last();
		return res;
	}
	assert(tmp->end() > s.start());
// 	// Don't return a segment which do not *OVERLAP* with s 
// 	// (boundary overlap is excluded).
// 	if ((tmp->end() <= s.start()) || (tmp->start() >= s.end())) 
// 	    return res;

	// XXX How to flag that no more data is available in the future??
	res = s;
	int orig_size = s.datasize();
	if (res.start() < tmp->start()) {
		// |-------| (s)    ---> time axis
		//    |--------| (tmp)
		//
		// The start time of s is invalid, we need to adjust both 
		// the start time (and size if necessary)
		res.set_start(tmp->start());
		if (tmp->datasize() < orig_size) 
			// Not enough data available??
			res.set_datasize(tmp->datasize());
		else
			res.set_datasize(orig_size);
	} else if (res.end() > tmp->end()) {
		//    |---------| (s)    ---> time axis
		// |-------| (tmp)
		// 
		// The start time in s is valid, but we may need to adjust the 
		// end time (i.e., size) of s.
		res.set_datasize(tmp->end()-res.start());
	}
	// Falling through means that the requested segment is available 
	// and can be returned as it is.

	assert(res.datasize() <= tmp->datasize());
	if ((res.end() == tmp->end()) && (tmp->next() == NULL))
		// This is the last data segment of the layer
		res.set_last();
	return res;
}

// Note that evicting all segments in this layer may not leave enough 
// space, so we return the number of bytes evicted from this layer
int MediaSegmentList::evict_tail(int size)
{
	int sz = size, tz;
	MediaSegment *tmp = (MediaSegment *)tail_;
	while ((tmp != NULL) && (sz > 0)) {
		// Reduce the last segment's size and adjust its playout time
		tz = tmp->evict_tail(sz);
		length_ -= tz;
		sz -= tz; 
		if (tmp->datasize() == 0) {
			// This segment is empty now
			detach(tmp);
			delete tmp;
			tmp = (MediaSegment *)tail_;
		}
	}
	return size - sz;
}

// Evicting <size> from the head of the list
int MediaSegmentList::evict_head(int size)
{
	int sz = size, tz;
	MediaSegment *tmp = (MediaSegment *)head_;
	while ((tmp != NULL) && (sz > 0)) {
		// Reduce the last segment's size and adjust its playout time
		tz = tmp->evict_head(sz);
		sz -= tz; 
		length_ -= tz;
		if (tmp->datasize() == 0) {
			// This segment is empty now
			detach(tmp);
			delete tmp;
			tmp = (MediaSegment *)head_;
		}
	}
	return size - sz;
}

// Evict all segments before <offset> from head and returns the size of 
// evicted segments.
int MediaSegmentList::evict_head_offset(int offset)
{
	int sz = 0;
	MediaSegment *tmp = (MediaSegment *)head_;
	while ((tmp != NULL) && (tmp->start() < offset)) {
		if (tmp->end() <= offset) {
			// delete whole segment
			sz += tmp->datasize();
			length_ -= tmp->datasize();
			detach(tmp);
			delete tmp;
			tmp = (MediaSegment *)head_;
		} else {
			// remove part of the segment
			sz += offset - tmp->start();
			length_ -= offset - tmp->start();
			tmp->set_start(offset);
		}
	}
	if (head_ == NULL)
		tail_ = NULL;
	return sz;
}

// Return a list of "holes" between the given offsets
MediaSegmentList MediaSegmentList::check_holes(const MediaSegment& s)
{
	MediaSegmentList res;  // empty list
	MediaSegment* tmp = (MediaSegment *)head_;
	while ((tmp != NULL) && (tmp->before(s)))
		tmp = tmp->next();
	// If all segments are before s, s is a hole
	if (tmp == NULL) {
		res.add(s);
		return res;
	}
	// If s is within *tmp, there is no hole
	if (s.in(*tmp))
		return res;

	// Otherwise return a list of holes
	int soff, eoff;
	soff = s.start();
	eoff = s.end();
	while ((tmp != NULL) && (tmp->overlap(s))) {
		if (soff < tmp->start()) {
			// Only refetches the missing part
			res.add(MediaSegment(soff, min(eoff, tmp->start())));
#if 1
			// DEBUG ONLY
			// Check if these holes are really holes!
			if (in(MediaSegment(soff, min(eoff, tmp->start())))) {
				fprintf(stderr, "Wrong hole: (%d %d) ", 
					soff, min(eoff, tmp->start()));
				fprintf(stderr, "tmp(%d %d), s(%d %d)\n",
					tmp->start(), tmp->end(),
					soff, eoff);
				fprintf(stderr, "List content: ");
				print();
			}
#endif
		}
		soff = tmp->end();
		tmp = tmp->next();
	}
	if (soff < eoff) {
		res.add(MediaSegment(soff, eoff));
#if 1		
		// DEBUG ONLY
		// Check if these holes are really holes!
		if (in(MediaSegment(soff, eoff))) {
			fprintf(stderr, "Wrong hole #2: (%d %d)\n", 
				soff, eoff);
			fprintf(stderr, "List content: ");
			print();
		}
#endif
	}
#if 0
	check_integrity();
#endif
	return res;
}

void MediaSegmentList::check_integrity()
{
	MediaSegment *p, *q;
	p = (MediaSegment*)head_;
	while (p != NULL) {
		q = p; 
		p = p->next();
		if (p == NULL)
			break;
		if (!q->before(*p)) {
			fprintf(stderr, 
				"Invalid segment added: (%d %d), (%d %d)\n", 
				q->start(), q->end(), p->start(), p->end());
			abort();
		}
	}
}

// Return the portion in s that is overlap with any segments in this list
// Sort of complementary to check_holes(), but it does not return a list, 
// hence smaller overhead. 
int MediaSegmentList::overlap_size(const MediaSegment& s) const
{
	int res = 0;
	MediaSegment* tmp = (MediaSegment *)head_;
	while ((tmp != NULL) && (tmp->before(s)))
		tmp = tmp->next();
	// If all segments are before s, there's no overlap
	if (tmp == NULL)
		return 0;
	// If s is within *tmp, entire s overlaps with the list
	if (s.in(*tmp))
		return s.datasize();
	// Otherwise adds all overlapping parts together.
	int soff, eoff;
	soff = s.start();
	eoff = s.end();
	while ((tmp != NULL) && (tmp->overlap(s))) {
		res += min(eoff, tmp->end()) - max(soff, tmp->start());
		soff = tmp->end();
		tmp = tmp->next();
	}
	return res;
}

// Debug only
void MediaSegmentList::print() 
{
	MediaSegment *p = (MediaSegment *)head_;
	int i = 0, sz = 0;
	while (p != NULL) {
		printf("(%d, %d)  ", p->start(), p->end());
		sz += p->datasize();
		p = p->next();
		if (++i % 8 == 0)
			printf("\n");
	}
	printf("\nTotal = %d\n", sz);
}

// Debug only
int MediaSegmentList::getsize()
{
	MediaSegment *p = (MediaSegment *)head_;
	int sz = 0;
	while (p != NULL) {
		sz += p->datasize();
		p = p->next();
	}
	return sz;
}

// Print into a char array with a given size. Abort if the size is exceeded.
char* MediaSegmentList::dump2buf()
{
	char *buf = new char[1024];
	char *b = buf;
	MediaSegment *p = (MediaSegment *)head_;
	int i = 0, sz = 1024;
	buf[0] = 0;
	while (p != NULL) {
		// XXX snprintf() should either be in libc or implemented
		// by TclCL (see Tcl2.cc there).
		i = snprintf(b, sz, "{%d %d} ", p->start(), p->end());
		sz -= i;
		// Boundary check: if less than 50 bytes, allocate new buf
		if (sz < 50) {
			char *tmp = new char[strlen(buf)+1024];
			strcpy(tmp, buf);
			delete []buf;
			buf = tmp;
			b = buf + strlen(buf);
			sz += 1024;
		} else 
			b += i;
		p = p->next();
	}
	return buf;
}



HttpMediaData::HttpMediaData(const char* sender, const char* page, int layer, 
			     int st, int et) :
	HttpData(MEDIA_DATA, 0), layer_(layer), st_(st), et_(et), flags_(0)
{
	assert(strlen(page)+1 <= (size_t)HTTP_MAXURLLEN);
	strcpy(page_, page);
	assert(strlen(sender)+1 <= (size_t)HTTP_MAXURLLEN);
	strcpy(sender_, sender);
}



static class MappClass : public TclClass {
public:
	MappClass() : TclClass("Application/MediaApp") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc > 4) 
			return (new MediaApp(argv[4]));
		return NULL;
	}
} class_mapp;

MediaApp::MediaApp(const char* page) : 
	log_(0), num_layer_(0), last_layer_(0)
{
	strcpy(page_, page);

	// Initialize all layer data pointers
	for (int i = 0; i < MAX_LAYER; i++)
		data_[i].set_start(0); 

	bind("segmentSize_", &seg_size_);
}

void MediaApp::start()
{
 	fprintf(stderr, "MediaApp::start() not supported\n");
 	abort();
}

void MediaApp::stop()
{
	// Called when we want to stop the RAP agent
	rap()->stop();
}

AppData* MediaApp::get_data(int& nbytes, AppData* req) 
{
	AppData *res;
	if (req == NULL) {
		MediaRequest p(MEDIAREQ_GETSEG);
		p.set_name(page_);
		// We simply rotating the layers from which to send data
		if (num_layer_ > 0) {
			p.set_layer(last_layer_++);
			last_layer_ = last_layer_ % num_layer_;
		} else 
			p.set_layer(0); 
		p.set_st(data_[0].start());
		p.set_datasize(seg_size_);
		p.set_app(this);
		res = target()->get_data(nbytes, &p);
	} else 
		res = target()->get_data(nbytes, req);

	// Update the current data pointer
	assert(res != NULL);
	HttpMediaData *p = (HttpMediaData *)res;

	// XXX For now, if the return size is 0, we assume that the 
	// transmission stops. Otherwise there is no way to tell the 
	// RAP agent that there's no more data to send
	if (p->datasize() <= 0) {
		// Should NOT advance sending data pointer because 
		// if this is a cache which is downloading from a slow
		// link, it is possible that the requested data will
		// become available in the near future!!
		delete p;
		return NULL;
	} else {
		// Set current data pointer to the right ones
		// If available data is more than seg_size_, only advance data
		// pointer by seg_size_. If less data is available, only 
		// advance data by the amount of available data.
		//
		// XXX Currently the cache above does NOT pack data from 
		// discontinugous blocks into one packet. May need to do 
		// that later. 
		assert((p->datasize() > 0) && (p->datasize() <= seg_size_));
		data_[p->layer()].set_start(p->et());
		data_[p->layer()].set_datasize(seg_size_);
	}
	return res;
}

int MediaApp::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (strcmp(argv[1], "log") == 0) {
		int mode;
		log_ = Tcl_GetChannel(tcl.interp(), 
				      (char*)argv[2], &mode);
		if (log_ == 0) {
			tcl.resultf("%s: invalid log file handle %s\n",
				    name(), argv[2]);
			return TCL_ERROR;
		}
		return TCL_OK;
	} else if (strcmp(argv[1], "evTrace") == 0) { 
		char buf[1024], *p;
		if (log_ != 0) {
			sprintf(buf, "%.17g ", 
				Scheduler::instance().clock());
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
	} else if (strcmp(argv[1], "set-layer") == 0) {
		int n = atoi(argv[2]);
		if (n >= MAX_LAYER) {
			fprintf(stderr, 
				"Too many layers than maximum allowed.\n");
			return TCL_ERROR;
		}
		num_layer_ = n;
		return TCL_OK;
	}
	return Application::command(argc, argv);
}

void MediaApp::log(const char* fmt, ...)
{
	char buf[1024], *p;
	char *src = Address::instance().print_nodeaddr(rap()->addr());
	sprintf(buf, "%.17g i %s ", Scheduler::instance().clock(), src);
	delete []src;
	p = &(buf[strlen(buf)]);
	va_list ap;
	va_start(ap, fmt);
	vsprintf(p, fmt, ap);
	if (log_ != 0)
		Tcl_Write(log_, buf, strlen(buf));
}


//----------------------------------------------------------------------
// MediaApp enhanced with quality adaptation
//----------------------------------------------------------------------
void QATimer::expire(Event *)
{
	a_->UpdateState();
	resched(a_->UpdateInterval());
}

static class QAClass : public TclClass {
public:
	QAClass() : TclClass("Application/MediaApp/QA") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc > 4) 
			return (new QA((const char *)(argv[4])));
		return NULL;
	}
} class_qa_app;

//#define CHECK 1
//#define DBG 1

QA::QA(const char *page) : MediaApp(page)
{
	updTimer_ = new QATimer(this);

	bind("LAYERBW_", &LAYERBW_);
	bind("MAXACTIVELAYERS_", &MAXACTIVELAYERS_);
	bind("SRTTWEIGHT_", &SRTTWEIGHT_);
	bind("SMOOTHFACTOR_", &SMOOTHFACTOR_);
	bind("MAXBKOFF_", &MAXBKOFF_);
	bind("debug_output_", &debug_);
	bind("pref_srtt_", &pref_srtt_);

	for(int j = 0; j < MAX_LAYER; j++) {
		buffer_[j] = 0.0;
		sending_[j] = 0;
		playing_[j] = 0;
		drained_[j] = 0.0;
		bw_[j] = 0.0;
		pref_[j] = 0;
	}
	poffset_ = 0; 
	playTime_ = 0;   // Should initialize it
	startTime_ = -1; // Used to tell the first packet

	// Moving average weight for transmission rate average
	rate_weight_ = 0.01;
	avgrate_ = 0.0;
}

QA::~QA()
{
	if (updTimer_) {
		if (updTimer_->status() != TIMER_IDLE)
			updTimer_->cancel();
		delete updTimer_;
	}
}

void QA::debug(const char* fmt, ...)
{
	if (!debug_) 
		return;

	char buf[1024], *p;
	char *src = Address::instance().print_nodeaddr(rap()->addr());
	char *port = Address::instance().print_portaddr(rap()->addr());
	sprintf(buf, "# t %.17g i %s.%s QA ", 
		Scheduler::instance().clock(), src, port);
	delete []port;
	delete []src;
	p = &(buf[strlen(buf)]);
	va_list ap;
	va_start(ap, fmt);
	vsprintf(p, fmt, ap);
	fprintf(stderr, "%s", buf);
}

void QA::panic(const char* fmt, ...) 
{
	char buf[1024], *p;
	char *src = Address::instance().print_nodeaddr(rap()->addr());
	char *port = Address::instance().print_portaddr(rap()->addr());
	sprintf(buf, "# t %.17g i %s.%s QA PANIC ", 
		Scheduler::instance().clock(), src, port);
	delete []port;
	delete []src;
	p = &(buf[strlen(buf)]);
	va_list ap;
	va_start(ap, fmt);
	vsprintf(p, fmt, ap);
	fprintf(stderr, "%s", buf);

#if 0
	// XXX This is specific to OUR test. Remove it in release!!
	Tcl::instance().eval("[Simulator instance] flush-trace");
	abort();
#endif
}

// Stop all timers
void QA::stop()
{
	rap()->stop();
	if (updTimer_->status() != TIMER_IDLE)
		updTimer_->cancel();
}

// Empty for now
int QA::command(int argc, const char*const* argv)
{
	return MediaApp::command(argc, argv);
}

// When called by RAP, req is NULL. We fill in the next data segment and 
// return its real size in 'size' and return the app data. 
AppData* QA::get_data(int& size, AppData*)
{
	int layers, dropped, i, l, idx, bs1, bs2,scenario, done, cnt;
	double slope, bufavail, bufneeded, totbufs1, totbufs2, 
		optbufs1[MAX_LAYER], optbufs2[MAX_LAYER], bufToDrain;
  
	static double last_rate = 0.0, nextAdjPoint = -1,
		FinalDrainArray[MAX_LAYER],
		tosend[MAX_LAYER], FinalBuffer[MAX_LAYER];
	
	static int flag,  /* flag keeps the state of the last phase */
		tosendPtr = 0;
  
	// Get RAP info
	double rate = seg_size_ / rap()->ipg();
	double srtt =  rap()->srtt();
	Scheduler& s = Scheduler::instance();
	double now = s.clock();
	int anyAck = rap()->anyack();

	assert((num_layer_ > 0) && (num_layer_ < MAX_LAYER));
  
	// this part is added for the startup
	// to send data for the base layer until the first ACK arrives.
	// This is because we don't have an estimate for SRTT and slope of inc
	// Make sure that SRTT is updated properly when ACK arrives
	if (anyAck == 0) {
		sending_[0] = 1;
		return output(size, 0);
		debug("INIT Phase, send packet: layer 0 in send_pkt, \
rate: %.3f, avgrate: %.3f, srtt:%.3f\n", rate, avgrate_, srtt);
	}
  
	layers = 0;
	// we can only calc slope when srttt has a right value
	// i.e. RAP has received an ACK
	slope = seg_size_/srtt;
	bufavail = 0.0;

	// XXX Is this a correct initial value????
	bufneeded = 0.0; 
  
	// calculate layers & bufavail
	for (i = 0; i < MAX_LAYER; i++) {
		layers += sending_[i];
		if (sending_[i] == 1) 
			bufavail += buffer_[i];
		else
			/* debug only */
			if ((i < MAX_LAYER - 1) && (sending_[i+1] == 1))
				panic("ERROR L%d is not sent but L%d is.\n",
				      i, i+1);
	}
	
	// check for startup phase
	if((layers == 1) && (playing_[0] != 1)){
		// L0 still buffers data, we are in startup phase
		// let's check
		if (sending_[0] == 0) {
			panic("ERROR sending[0]=0 !!!");
		}
		AppData *res = output(size, 0);
		debug("STARTUP, send packet: layer 0\n");
		
		// Start playout if we have enough data for L0
		// The amount of buffered data for startup can be diff
		bufneeded = max(4*BufNeed((LAYERBW_-rate/2.0), slope), 
				2*MWM(srtt));

		if (buffer_[0] >= bufneeded) {
			playing_[0] = 1;
			sending_[0] = 1;  
			drained_[0] = 0;  /* srtt*LAYERBW; */
			startTime_ = now; // start the playback at the client
			playTime_ = now;  // playout time of the receiver. 
			debug("... START Playing_ layer 0, buffer[0] = %f!\n",
			      buffer_[0]);
			// start emulating clients consumption
			if (updTimer_->status() == TIMER_IDLE)
				updTimer_->sched(srtt);
		}
		return(res);
	}
  
	// Store enough buffer before playing a layer. 
	// XXX, NOTE: it is hard to do this, when we add a new layer
	// the server sets the playout time of the first segment
	// to get to the client in time, It is hard to make sure
	// that a layer has MRM worth if data before stasting its
	// playback because it adds more delay
	// the base layer starts when it has enough buffering
	// the higher layers are played out when their data is available
	// so this is not needed 
	//for (i = 0; i < MAX_LAYER; i++) {
	// if ((sending_[i] == 1) && (playing_[i] == 0) &&
	//    (buffer_[i] > MWM(srtt))) {
	//  debug("Resume PLAYING Layer %d, play: %d send: %d\n",
	//	  i, playing_[i], sending_[i]);
	//  playing_[i]=1;
	//  drained_[i] = 0; /* XXX, not sure about this yet 
	//		      * but if we set this to max it causes
	//	      * a spike at the adding time
	//		      */
	//  /* drained_[i]=LAYERBW*SRTT; */
	//}
	//}
  
	// perform the primary drop if we are in drain phase 
	if (rate < layers*LAYERBW_) {
		bufneeded = (MWM(srtt)*layers) + 
			BufNeed((layers*LAYERBW_-rate), slope);
		//   debug("tot_bufavail: %7.1f bufneeded: %7.1f, layers: %d",
		// 			bufavail, bufneeded, layers);
		dropped = 0;
		// XXX Never ever do primary drop layer 0!!!!
		while ((bufneeded > TotalBuf(layers, buffer_)) && 
		       (layers > 1)) {
			debug("** Primary DROPPED L%d, TotBuf(avail:%.1f \
needed:%.1f), buf[%d]: %.2f\n", 
			      layers-1, TotalBuf(layers, buffer_), bufneeded,
			      layers-1,buffer_[layers-1]);
			layers--;
			dropped++;
			sending_[layers] = 0;
			bufneeded = (MWM(srtt)*layers)+ 
				BufNeed(((layers)*LAYERBW_-rate),slope); 
		}
	} 
    
	// just for debugging
	// here is the case when even the base layer can not be kept
	if ((bufneeded > TotalBuf(layers, buffer_)) && (layers == 1)) {
		// XXX We should still continue, shouldn't we????
		debug("** Not enough buf to keep the base layer, \
TotBuf(avail:%.1f, needed:%.1f), \n",
		      TotalBuf(layers, buffer_), bufneeded);
	}

	if (layers == 0) {
		//		panic("** layers =0 !!");
		sending_[0] = 1;
		playing_[0] = 0;
		if (updTimer_->status() != TIMER_IDLE)
			updTimer_->cancel();
		debug("** RESTART Phase, set playing_[0] to 0 to rebuffer data\n");
		return output(size, 0);
	}

	// now check to see which phase we are in
	if (rate >= layers*LAYERBW_) {

		/******************
		 ** filling phase **
		 *******************/
/*      
debug("-->> FILLING, layers: %d now: %.2f, rate: %.3f, avgrate: %.3f, \
 srtt:%.3f, slope: %.3f\n",
 	  layers, now, rate, avgrate_, srtt, slope);
*/
      
		last_rate = rate; /* this is used for the next drain phase */
		flag = 1;
		/* 
		 * 1) send for any layer that its buffer is below MWM
		 * MWM is the min amount of buffering required to absorbe 
		 * jitter
		 * each active layer must have atleast MWM data at all time
		 * this also ensures proper bw share, we do NOT explicitly 
		 * alloc BW share during filling
		 * Note: since we update state of the buffers on a per-packet 
		 * basis, we don't need to ensure that each layer gets a share 
		 * of bandwidth equal to its consumption rate. 
		 */
		for (i=0;i<layers;i++) {
			if (buffer_[i] < MWM(srtt)) {
				if ((buffer_[i-1] <= buffer_[i]+seg_size_) &&
				    (i > 0))
					idx = i-1;
				else 
					idx = i;
// 	  debug("A:sending layer %d, less than MWM, t: %.2f\n", 
// 		i,now);
				return output(size, idx);
			}
		}

		/* 
		 * Main filling algorithm based on the pesudo code 
		 * find the next optimal state to reach 
		 */
		/* init param */
		bs1 = 0;
		bs2 = 0;
		totbufs1 = 0;
		totbufs2 = 0;
		for (l=0; l<MAX_LAYER; l++) {
			optbufs1[l] = 0.0;
			optbufs2[l] = 0.0;
		}
		
		// XXX Note: when per-layer BW is low, and srtt is very small 
		// (e.g., in a LAN), the following code will result in that 
		// one buffered 
		// segment will produce a abort() of "maximum backoff reached".

		/* next scenario 1 state */
		while ((totbufs1 <= TotalBuf(layers, buffer_)) && 
		       (bs1 <= MAXBKOFF_)) {
			totbufs1 = 0.0;
			bs1++;
			for (l=0; l<layers;l++) {
				optbufs1[l] = bufOptScen1(l,layers,rate,slope,
							  bs1)+MWM(srtt);
				totbufs1 += optbufs1[l];
			}
		}

		// bs1 is the min no of back off that we can not handle for 
		// s1 now
		/* next secenario 2 state */
		while ((totbufs2 <= TotalBuf(layers, buffer_)) && 
		       (bs2 <= MAXBKOFF_)) {
			totbufs2 = 0.0;
			bs2++;
			for (l=0; l<layers;l++) {
				optbufs2[l] = bufOptScen2(l,layers,rate,slope,
							  bs2)+MWM(srtt);
				totbufs2 += optbufs2[l];
			}
		}
		
		/* 
		 * NOTE: at this point, totbufs1 could be less than total 
		 * buffering
		 * when it is enough for recovery from rate = 0;
		 * so totbufs1 <= TotalBuf(layers, buffer) is OK
		 * HOWEVER, in this case, we MUST shoot for scenario 2
		 */

		/* debug */
/*
       if ((totbufs2 <= TotalBuf(layers, buffer_)) && (bs2 <= MAXBKOFF_)) {
 	panic("# ERROR: totbufs1: %.2f,tot bufs2: %.2f, \
 totbuf: %.2f, bs1: %d, bs2: %d, totneededbuf1: %.2f, totneededbuf2: %2f\n",
 	      totbufs1, totbufs2, TotalBuf(layers, buffer_), bs1, bs2,
 	      TotalBuf(layers, optbufs1), TotalBuf(layers, optbufs2));
       }
*/
		/* debug */
		if (bs2 >= MAXBKOFF_)
			debug("WARNING: MAX No of backoff Reached, bs1: %d, \
bs2: %d\n", bs1, bs2);

		/* Check for adding condition */
		//if ((bs1 > SMOOTHFACTOR_) && (bs2 > SMOOTHFACTOR_) && 
		//  (layers < MAX_LAYER)) {
		if ((bs1 > SMOOTHFACTOR_) && (bs2 > SMOOTHFACTOR_)){
			// Check if all layers are already playing
			// Assume all streams have the same # of layer: 
			// MAX_LAYER
			assert(layers <= num_layer_);

			// XXX Only limit the rate when we have all layers 
			// playing. There should be a better way to limit the 
			// transmission rate earlier! Note that we need RAP to
			// fix its IPG as soon as we fix the rate here. Thus, 
			// RAP should do that in its IpgTimeout()
			// instead of DecreaseIpg(). See rap.cc.
			if (layers == num_layer_){
#if 0
				if (rate < num_layer_*LAYERBW_)
					panic("ERROR: rate: %.2f is less than \
MAX BW for all %d layers!\n", rate, layers);
#endif
				// Ask RAP to fix the rate at MAX_LAYER*LAYERBW
				rap()->FixIpg((double)seg_size_/
					      (double)(num_layer_*LAYERBW_));
				// Mux the bandwidth evenly among layers  
				return output(size, layers - 1);	  
			}

			// Calculate the first packet offset in this new layer
			int off_start = (int)floor((poffset_ + MWM(srtt)) / 
						   seg_size_) * seg_size_;
			// XXX Does the application have data between 
			// off_start_ and off_start_+MWM(srtt)??
	  
			// XXX If the computed offset falls behind, we just 
			// continue to send.
			if (data_[layers].start() <= off_start) {
				// Set LayerOffset[newlayer] = 
				//     poffset_ + MWM(srtt) * n: 
				// - n times roundtrip time of data, LET n BE 1
				// Round this offset to whole segment
				data_[layers].set_start(off_start);
				data_[layers].set_datasize(seg_size_);
			}
			// Make sure that all corresponding data in lower 
			// layers have been sent out, i.e., the last byte of 
			// current segment of the new layer should be less 
			// than the last byte of all lower layers
			if (data_[layers].end() > data_[layers-1].start()) 
				// XXX Do not send anything if we don't have
				// data!! Otherwise we'll dramatically increase
				// the sending rate of lower laters. 
				return NULL;
				//  return output(size, layers-1);

			sending_[layers] = 1;
			AppData *res = output(size, layers);
			if (res == NULL) {
				// Drop the newly added layer because we 
				// don't have data 
				sending_[layers] = 0;
				// However, do prefetching in case we'll add 
				// it again later
				int st = (int)floor((data_[layers].start()+
						pref_srtt_*LAYERBW_)
					       /seg_size_+0.5)*seg_size_;
				int et = (int)floor((data_[layers].end()+
						pref_srtt_*LAYERBW_)
					       /seg_size_+0.5)*seg_size_;
				if (et > pref_[layers]) {
					pref_[i] = et;
					MediaSegment s(st, et);
					check_availability(i, s);
				}
				for (i = 0; i < layers; i++) 
					if (buffer_[i] < MWM(srtt)) {
						res = output(size, i);
						if (res != NULL)
							break;
					}
			} else {
				/* LAYERBW_*srtt;should we drain this */
				drained_[layers]= 0; 
				debug("sending Just ADDED layer %d, t: %.2f\n",
				      i, now);
			}
			return res;
		}

		/* 
		 * Find out which next step is closer
		 * Second cond is for the cases where totbufs2 becomes 
		 * saturated
		 */
		scenario = 0; // Initial value
		if((totbufs1 <= totbufs2) && 
		   (totbufs1 > TotalBuf(layers, buffer_))) {
			/* go for next scenario 1 with sb1 backoff */
			scenario = 1;
		} else {
			/* go for next scenario 2 with sb2 backoffs */
			scenario = 2;
		}

		/* decide which layer needs more data */
		if (scenario == 1) {
			for (l=0; l<layers; l++) {
				if (buffer_[l] >= optbufs1[l]) 
					continue;
				//if (buffer_[l] < optbufs1[l]) {
				if ((buffer_[l-1] <= buffer_[l]+seg_size_) && 
				    (l > 0))
					idx = l-1;
				else 
					idx = l;
				// debug("Cs1:sending layer %d to fill buffer, t: %.2f\n", 
				// idx,now);
				return output(size, idx);
			}
		} else if (scenario == 2) {
			l=0;
			done = 0;
			while ((l<layers) && (!done)){
				if (TotalBuf(layers, buffer_) >= totbufs2) {
					done ++;
				} else {
					if (buffer_[l]<min(optbufs2[l],
							   optbufs1[l])) {
						if((buffer_[l-1] <= buffer_[l]+
						    seg_size_) && (l>0))
							idx = l-1;
						else 
							idx = l;
//  		debug("Cs2:sending layer %d to fill buffer, t: %.2f\n", 
// 			idx,now);
						return output(size, idx);
					}
					l++;
				}
			} /* while */
		} else 
			panic("# ERROR: Unknown scenario: %d !!\n", scenario);

		/* special cases when we get out of this for loop */
		if(scenario == 1){
			panic("# Should not reach here, totbuf: %.2f, \
totbufs1: %.2f, layers: %d\n",
			      TotalBuf(layers, buffer_), totbufs1, layers);
		}

		if (scenario == 2) {
			/* 
			 * this is the point where we have satisfied buffer 
			 * requirement for the next scenario 1 already, 
			 * i.e. the MIN() value.
			 * so we relax that and shoot for bufs2[l]
			 */

			/* 
			 * if scenario 2, repeat the while loop without min 
			 * cond we have alreddy satisfied the condition for 
			 * the next scenario 1
			 */
			l=0;
			while (l < layers) {
				if (buffer_[l] < optbufs2[l]) {
					if ((buffer_[l-1] <= buffer_[l]+
					     seg_size_) && (l>0))
						idx = l-1;
					else 
						idx = l;
// 	    debug("Cs22:sending layer %d to fill buffer, t: %.2f\n", idx,now);
					return output(size, idx);
				}
				l++;
			}/* while */
		}

		panic("# Opps, should not reach here, bs1: %d, bs2: %d, \
scen: %d, totbufs1: %.2f, totbufs2: %.2f, totbufavail: %.2f\n", 
		      bs1, bs2, scenario, totbufs1,
		      totbufs2, TotalBuf(layers, buffer_));
    
		/* NEVER REACH HERE */
    
	} else { /* rate < layers*LAYERBW_ */
		/*******************
		 ** Draining phase **
		 *******************/
/*
    debug("-->> DRAINING, layers: %d rate: %.3f, avgrate: %.3f, srtt:%.3f, \
 slope: %.3f\n", 
 	 layers, rate, avgrate_, srtt, seg_size_/srtt);
*/

		/*
		 * At the beginning of a new drain phase OR 
		 * another drop in rate during a draining phase OR
		 * dec of slope during a draining phase that results in 
		 * a new drop 
		 */

		/*
		 * 1) the highest priority action at this point is to ensure 
		 * all surviving layers have min amount of buffering, if not, 
		 * try to fill that layer
		 */
		double lowest=buffer_[0];
		int lowix=0;
		for(i=0;i<layers;i++) {
			if (lowest>buffer_[i]) {
				lowest=buffer_[i];
				lowix=i;
			}
		}
		if (lowest<MWM(srtt)) {
//       debug("A':sending layer %d, below MWM in Drain t: %.2f\n",
// 	    lowix, now);
			return output(size, lowix);
		}

		if((nextAdjPoint < 0) || /* first draining phase */
		   (flag >= 0) || /* after a filling phase */
		   (now >= nextAdjPoint) || /* end of the curr interval */
		   ((rate < last_rate) && (flag < 0)) || /* new backoff */
		   (AllZero(tosend, layers))) /* all pkt are sent */ {

			/* start of a new interval */
			/* 
			 * XXX, should update the nextAdjPoint diff for 
			 * diff cases 
			 */
			nextAdjPoint = now + srtt;
			bufToDrain = LAYERBW_*layers - rate;
			/*
			 * calculate optimal dist. of bufToDrain across all 
			 * layers. FinalDrainArray[] is the output
			 * FinalBuffer[] is the final state
			 */
			if (bufToDrain <= 0)
				panic("# ERROR: bufToDrain: %.2f\n", 
				      bufToDrain);

			DrainPacket(bufToDrain, FinalDrainArray, layers, rate, 
				    srtt, FinalBuffer);

			for(l=0; l<MAX_LAYER; l++){
				tosend[l] = 0;
			}

			for(l=0; l<layers; l++){
				tosend[l] = srtt*LAYERBW_ - FinalDrainArray[l];
				// Correct for numerical error
				if (fabs(tosend[l]) < QA_EPSILON)
					tosend[l] = 0.0;
			}

			/* 
			 * XXX, not sure if this is the best thing 
			 * we might only increase it
			 */
			tosendPtr = 0;

			/* debug only */
			if ((bufToDrain <= 0) || 
			    AllZero(FinalDrainArray, layers) ||
			    AllZero(tosend, layers)) {
				debug("# Error: bufToDrain: %.2f, %d layers, "
				      "srtt: %.2f\n", 
				      bufToDrain, layers, srtt);
				for (l=0; l<layers; l++)
					debug("# FinalDrainArray[%d]: %.2f, "
					      "tosend[%d]: %.2f\n", l, 
					      FinalDrainArray[l],l, tosend[l]);
			}
			/*******/
		}

		flag = -1;
		last_rate = rate;
		done = 0;
		cnt = 1;  
		while ((!done) && (cnt <= layers)) {
			if (tosend[tosendPtr] > 0) {
				if ((buffer_[tosendPtr-1] <= buffer_[tosendPtr]
				     + seg_size_) && (tosendPtr > 0))
					idx = tosendPtr-1;
				else 
					idx = tosendPtr;
				tosend[tosendPtr] -= seg_size_;
				if (tosend[tosendPtr] < 0)
					tosend[tosendPtr] = 0;
				return output(size, idx);
			}
			cnt++;
			tosendPtr = (tosendPtr+1) % layers;
		}

		// XXX End of Drain Phase
		// For now, send a chunk from the base layer. Modify it later!!
		return output(size, 0);
	} /* if (rate >= layers*LAYERBW_) */

	panic("# QA::get_data() reached the end. \n");
	/*NOTREACHED*/
	return NULL;
}

//-----------------------------------------
//-------------- misc routine
//------------------------------------------

// return 1 is all first "len" element of "arr" are zero
// and 0 otherwise
int QA::AllZero(double *arr, int len)
{
	int i;
  
	for (i=0; i<len; i++)
		if (arr[i] != 0.0)
			// debug("-- arr[%d}: %f\n", i, arr[i]);
			return 0;
	return 1;
}


//
// Calculate accumulative amount of buffering for the lowest "n" layers
//
double QA::TotalBuf(int n, double *buffer)
{
	double totbuf = 0.0;
	int i;

	for(i=0; i<n; i++)
		totbuf += buffer[i]; 
	return totbuf;
}

// Update buffer_ information for a given layer
// Get an output data packet from applications above
AppData* QA::output(int& size, int layer)
{
	int i;
	assert((sending_[layer] == 1) || (startTime_ == -1));

	// In order to send out a segment, all corresponding segments of 
	// the lower layers must have been sent out
	if (layer > 0)
		if (data_[layer-1].start() <= data_[layer].start())
			return output(size, layer-1);

	// Get and output the data at the current data pointer
	MediaRequest q(MEDIAREQ_GETSEG);
	q.set_name(page_);
	q.set_layer(layer);
	q.set_st(data_[layer].start());
	q.set_datasize(seg_size_);
	q.set_app(this);
	AppData* res = target()->get_data(size, &q);

	assert(res != NULL);
	HttpMediaData *p = (HttpMediaData *)res; 

	if (p->datasize() <= 0) {
		// When the data is not available:
		// Should NOT advance sending data pointer because 
		// if this is a cache which is downloading from a slow
		// link, it is possible that the requested data will
		// become available in the near future!!

		// We have already sent out the last segment of the base layer,
		// now we are requested for the segment beyond the last one
		// in the base layer. In this case, consider the transmission
		// is complete and tear down the connection.
		if (p->is_finished()) {
			rap()->stop();
			// XXX Shouldn't this be done inside mcache/mserver??
			Tcl::instance().evalf("%s finish-stream %s", 
					      target()->name(), name());
		} else if (!p->is_last()) {
			// If we coulnd't find anything within q, move data 
			// pointer forward to skip holes.
			MediaSegment tmp(q.et(), q.et()+seg_size_);
			check_layers(p->layer(), tmp);
			// If we can, advance. Otherwise wait for
			// lower layers to advance first.
			if (tmp.datasize() > 0) {
				assert(tmp.datasize() <= seg_size_);
				data_[p->layer()].set_start(tmp.start());
				data_[p->layer()].set_end(tmp.end());
			}
		}
		delete p;
		return NULL;
	}

	// Set current data pointer to the right ones
	// If available data is more than seg_size_, only 
	// advance data pointer by seg_size_. If less data 
	// is available, only advance data by the amount 
	// of available data.
	//
	// XXX Currently the cache above does NOT pack data 
	// from discontinugous blocks into one packet. May 
	// need to do that later. 
// 		if (p->is_last())
// 			data_[p->layer()].set_last();
	assert((p->datasize() > 0) && (p->datasize() <= seg_size_));
	// XXX Before we move data pointer forward, make sure we don't violate
	// layer ordering rules. Note we only need to check end_ because 
	// start_ is p->et() which is guaranteed to be valid
	MediaSegment tmp(p->et(), p->et()+seg_size_);
	check_layers(p->layer(), tmp);
	if (tmp.datasize() > 0) {
		assert(tmp.datasize() <= seg_size_);
		data_[p->layer()].set_start(tmp.start());
		data_[p->layer()].set_end(tmp.end());
	} else {
		// Print error messages, do not send anything and wait for 
		// next time so that hopefully lower layers will already 
		// have advanced.
		fprintf(stderr, "# ERROR We cannot advance pointers for "
			"segment (%d %d)\n", tmp.start(), tmp.end());
		for (i = 0; i < layer; i++) 
			fprintf(stderr, "Layer %d, data ptr (%d %d) \n",
				i, data_[i].start(), data_[i].end());
		delete p;
		return NULL;
	}

	// Let me know that we've sent out this segment. This is used
	// later to drain data (DrainBuffers())
	outlist_[p->layer()].add(MediaSegment(p->st(), p->et()));

	buffer_[layer] += p->datasize();
	bw_[layer] += p->datasize();
	drained_[layer] -= p->datasize();
  
	//offset_[layer] += seg_size_;
	avgrate_ = rate_weight_*rate() + (1-rate_weight_)*avgrate_;

	// DEBUG check
	for (i = 0; i < layer-1; i++)
		if (data_[i].end() < data_[i+1].end()) {
			for (int j = 0; j < layer; j++)
				fprintf(stderr, "layer i: (%d %d)\n", 
					data_[i].start(), data_[i].end());
			panic("# ERROR Wrong layer sending order!!\n");
		}

	return res;
}

void QA::check_layers(int layer, MediaSegment& tmp) {
	// XXX While we are moving pointer forward, make sure
	// that we are not violating layer boundary constraint
	for (int i = layer-1; i >= 0; i--) 
		// We cannot go faster than a lower layer!!
		if (tmp.end() > data_[i].end())
			tmp.set_end(data_[i].end());
}

//
// This is optimal buffer distribution for scenario 1.
// NOTE: rate is the current rate before the backoff
// Jan 28, 99
//
// This routines performs buffer sharing by giveing max share
// to the lowest layer, i.e. it fills the triangle in a bottom-up
// starting from the base layer. We use this routine instead of bufOpt,
// for all cases during filling phase. Allocation based on diagonal strips
//
double QA::bufOptScen1(int layer, int layers, double currrate, 
		       double slope, int backoffs)
{
	double smallt, larget, side, rate;
  
	if (backoffs < 0) {
		panic("# ERROR: backoff: %d in bufOptScen1\n", 
		      backoffs);
	}
	rate = currrate/pow(2,backoffs);
	side = LAYERBW_*layers - (rate + layer*LAYERBW_);
	if (side <= 0.0) 
		return(0.0);
	larget = BufNeed(side, slope);
	side = LAYERBW_*layers - (rate + (layer+1)*LAYERBW_);
	if (side < 0.0) 
		side = 0.0;
	smallt = BufNeed(side, slope);

	return (larget-smallt);
}

//
// This routine calculate optimal buffer distribution for a layer
// in scenario 2 based on the 
// 1) current rate, 2) no of layers, 3) no of backoffs
//
// Jan 28, 99bufOptScen1(layer, layers, currrate, slope, backoffs)
//
double QA::bufOptScen2(int layer, int layers, double currrate, 
		       double slope, int backoffs)
{
	double bufopt = 0.0;
	int bmin, done;

	if(backoffs < 0) {
		panic("# ERROR: backoff: %d in bufOptScen2\n", 
			backoffs);
	}
	if ((currrate/pow(2,backoffs)) >= layers*LAYERBW_)
		return(0.0);

	bmin = 0;
	done = 0;
	while ((!done) && bmin<=backoffs) {
		if(currrate/pow(2,bmin) >= LAYERBW_*layers)
			bmin++;
		else 
			done++;
	}
	// buf required for the first triangle
	// we could have dec bmin and go for 1 backoff as well
	bufopt = bufOptScen1(layer, layers, currrate/pow(2,bmin), slope, 0);
  
	// remaining sequential backoffs
	bufopt += (backoffs - bmin)*BufNeed(layers*LAYERBW_/2, slope);
	return(bufopt);
}


//
// This routine returns the optimal distribution of requested-to-drained
// buffer across active layers based on:
// 1) curr rate, 2) curr drain distr(FinalDrainArry), etc
// NOTE, the caller must update FinalDrainArray from 
// 
// Jan 29, 99
//
// DrainArr: return value, used as an incremental chaneg for 
//   FinalDrainArray
// bufAvail:  current buffer_ state
void QA::drain_buf(double* DrainArr, double bufToDrain, 
		   double* FinalDrainArray, double* bufAvail, 
		   int layers, double rate, double srtt)
{
	double bufReq1, bufReq2, bufs1[MAX_LAYER], bufs2[MAX_LAYER], slope, 
		extra, targetArr[MAX_LAYER], maxDrainRemain;
	int bs1, bs2, l;

	slope = seg_size_/srtt;
	bs1 = MAXBKOFF_ + 1;
	bs2 = MAXBKOFF_ + 1;
	bufReq1 = bufReq2 = 0;
	for(l=0; l<layers; l++){
		bufReq1 += bufOptScen1(l, layers, rate, slope, bs1);
		bufReq2 += bufOptScen2(l, layers, rate, slope, bs2);
	} 

	for(l=0; l<MAX_LAYER; l++){
		bufs1[l] = 0;
		bufs2[l] = 0;
		DrainArr[l] = 0.0;
	}

	while(bufReq1 > TotalBuf(layers, bufAvail)){
		bufReq1 = 0;
		bs1--;
		for(l=0; l<layers; l++){
			bufs1[l] = bufOptScen1(l, layers, rate, slope, bs1);
			bufReq1 += bufs1[l];
		} 
	}
  
	while(bufReq2 > TotalBuf(layers, bufAvail)){
		bufReq2 = 0;
		bs2--;
		for(l=0; l<layers; l++){
			bufs2[l] =  bufOptScen2(l, layers, rate, slope, bs2);
			bufReq2 += bufs2[l];
		} 
	}

	if (bufReq1 >= bufReq2) {
		// drain toward last optimal scenario 1
		for (l=layers-1; l>=0; l--){
			// we try to drain the maximum amount from
			// min no of highest layers
			// note that there is a limit on total draining
			// from a layer
			maxDrainRemain = srtt*LAYERBW_ - FinalDrainArray[l];
			if ((bufAvail[l] > bufs1[l] + maxDrainRemain) &&
			    (bufToDrain >= maxDrainRemain)) {
				DrainArr[l] = maxDrainRemain;
				bufToDrain -= maxDrainRemain;
			} else {
				if(bufAvail[l] > bufs1[l] + maxDrainRemain){
					DrainArr[l] = bufToDrain;
					bufToDrain = 0.0;
				} else {
					DrainArr[l] = bufAvail[l] - bufs1[l];
					bufToDrain -= bufAvail[l] - bufs1[l];

					/* for debug */
					if(DrainArr[l] < 0.0){
// 	    panic("# ERROR, DrainArr[%d]: %.2f, bufAvail: %.2f, bufs1: %.2f\n",
// 		  l, DrainArr[l], bufAvail[l], bufs1[l]);
						DrainArr[l] = 0.0;
					}
				}
			}
			if(bufToDrain == 0.0)
				return;
		}
		return;
	} else {   /* if (bufReq1 >= bufReq2) */

		// Drain towards he last optima scenario 2 
		// We're draining - don't care about the upper bound on 
		// scenario 2.
		// Have to calculate all the layers together to get this max 
		// thing to work
		extra = 0.0;
		// Calculate the extra buffering 
		for (l=0; l<layers; l++) {
			if(bufs1[l] > bufs2[l])
				extra += bufs1[l] - bufs2[l];
		}

		for (l=layers-1; l>=0; l--)
			if(bufs1[l] >= bufs2[l])
				targetArr[l] = bufs1[l];
			else
				if (bufs2[l] - bufs1[l] >= extra) {
					targetArr[l] = bufs2[l] - extra;
					extra = 0;
				} else {
					// there is enough extra to compenstae the dif
					if (extra > 0) {
						targetArr[l] = bufs2[l];
						extra -= bufs2[l] - bufs1[l];
					} else 
						panic("# ERROR Should not \
reach here, extra: %.2f, bufs2: %.2f, bufs1: %.2f, L%d\n", 
						extra, bufs2[l], bufs1[l], l);
				}
	} /* end of if (bufReq1 >= bufReq2) */
   
	// drain toward last optimal scenario 2
	for (l=layers-1; l>=0; l--) {
		// we try to drain the maximum amount from
		// min no of highest layers
		// note that there is a limit on total draining
		// from a layer
		maxDrainRemain = srtt*LAYERBW_ - FinalDrainArray[l];
		if ((bufAvail[l] > targetArr[l] + maxDrainRemain) &&
		    (bufToDrain >= maxDrainRemain)) {
			DrainArr[l] = maxDrainRemain;
			bufToDrain -= maxDrainRemain;
		} else {
			if(bufAvail[l] > targetArr[l] + maxDrainRemain){
				DrainArr[l] = bufToDrain;
				bufToDrain = 0.0;
			} else {
				DrainArr[l] = bufAvail[l] - targetArr[l];
				bufToDrain -= bufAvail[l] - targetArr[l];
	
				// for debug 
				if (DrainArr[l] < 0.0) {
// 	  panic("# ERROR, DrainArr[%d]: %.2f, bufAvail: %.2f, bufs1: %.2f\n",
// 		l, DrainArr[l], bufAvail[l], bufs1[l]);
					DrainArr[l] = 0;
				}
			}
		}
		if (bufToDrain == 0.0)
			return;
	} /* end of for */
	return;
}


//
// This routine calculate an optimal distribution of a given 
// amount of buffered data to drain.
// the main algorithm is in drain_buf() and this one mainly init
// the input and calls that routine ad then update FinalDrainArray,
// based on its old value and return value for DrainArr.
//
// FinalDrainArray: output
// FinalBuffer: output, expected buf state at the end of the interval
void QA::DrainPacket(double bufToDrain, double* FinalDrainArray, int layers,
		     double rate, double srtt, double* FinalBuffer)
{
	double DrainArr[MAX_LAYER],  bufAvail[MAX_LAYER], TotBufAvail;
	int l,cnt;
  
	for(l=0; l<MAX_LAYER; l++){
		FinalDrainArray[l] = 0.0;
		bufAvail[l] = buffer_[l];
	}

	TotBufAvail = TotalBuf(layers, bufAvail);
	cnt = 0;
	while ((bufToDrain > 0) && (cnt < 10)) {
		// debug("bufToDrain%d: %.2f\n", cnt, bufToDrain);
		drain_buf(DrainArr, bufToDrain, FinalDrainArray, bufAvail, 
			  layers, rate, srtt);
		
		for(l=0; l<layers; l++){
			bufToDrain -= DrainArr[l];
			TotBufAvail -= DrainArr[l];
			FinalDrainArray[l] += DrainArr[l];
			bufAvail[l] -= DrainArr[l];
			FinalBuffer[l] = buffer_[l] - FinalDrainArray[l];
		}
		cnt++;
	}
}

void QA::check_availability(int layer, const MediaSegment& s) 
{
	int dummy; 
	MediaRequest p(MEDIAREQ_CHECKSEG);
	p.set_name(page_);
	p.set_layer(layer);
	p.set_st(s.start());
	p.set_et(s.end());
	p.set_app(this);
	// Ask cache/server to do prefetching if necessary.
	target()->get_data(dummy, &p);
}

/* 
 * This routine is called once every SRTT to drain some data from
 * recv's buffer and src's image from recv's buf.
 */
void QA::DrainBuffers()
{
	int i, j, layers = 0;
	Scheduler& s = Scheduler::instance();
	double now = s.clock();
	// interval since last drain
	double interval = now - playTime_;
	playTime_ = now;  // update playTime

	if ((layers > 1) && (playing_[0] != 1)) {
		panic("ERROR in DrainBuffer: layers>0 but L0 isn't playing\n");
	}

	// Updating playout offset, but do nothing if we are in the initial
	// startup filling phase! This offset measures the playing progress
	// of the client side. It is actually the playing offset of the lowest
	// layer.

	// This is the real amount of data to be drained from layers
	int todrain[MAX_LAYER]; 
	// Expected offset of base layer after draining, without considering 
	// holes in data. This has to be satisfied, otherwise base layer will
	// be dropped and an error condition will be raised.
	poffset_ += (int)floor(interval*LAYERBW_+0.5);

	// Started from MAX_LAYER to make debugging easier
	for (i = MAX_LAYER-1; i >= 0; i--) {
		// If this layer is not being played, don't drain anything
		if (sending_[i] == 0) {
			todrain[i] = 0;
			drained_[i] = 0.0;
			continue; 
		}
		todrain[i] = outlist_[i].evict_head_offset(poffset_);
		assert(todrain[i] >= 0);
		buffer_[i] -= todrain[i];
		// A buffer must have more than one byte
		if ((int)buffer_[i] <= 0) {
			debug("Buffer %d ran dry: %.2f after draining, DROP\n",
			      i, buffer_[i]);
			playing_[i] = 0;
			sending_[i] = 0;
			buffer_[i] = 0;
	    
			/* Drop all higher layers if they still have data */
			for (j = i+1; j < MAX_LAYER; j++)
				if (sending_[j] == 1) {
/*
 					panic("# ERROR: layer %d \
is playing with %.2f buf but layer %d ran dry with %.2f buf\n",
 					      j, buffer_[j], i, buffer_[i]);
*/
 					debug("# DROP layer %d: it \
is playing with %.2f buf but layer %d ran dry with %.2f buf\n",
 					      j, buffer_[j], i, buffer_[i]);
					sending_[j] = 0;
					playing_[j] = 0;
					buffer_[j] = 0;
				}
			// We don't need to set it to -1. The old address 
			// will be used to see if we are sending old data if 
			// that later is added again
			//
			// XXX Where is this -1 mark ever used????
			// data_[i].set_start(-1); // drop layer i
		} else {
			// Prefetch for this layer. Round to whole segment
			int st = (int)floor((poffset_+pref_srtt_*LAYERBW_)
					    /seg_size_+0.5)*seg_size_;
			int et = (int)floor((poffset_+(pref_srtt_+interval)*
					   LAYERBW_)/seg_size_+0.5)*seg_size_;
			if (et > pref_[i]) {
				pref_[i] = et;
				MediaSegment s(st, et);
				check_availability(i, s);
			}
		}
	} /* end of for */
}

// This routine dumps info into a file
// format of each line is as follows:
//  time tot-rate avg-rate per-layer-bw[MAXLAYER] tot-bw drain-rate[MAXLAYER] 
//  & cumulative-buffer[MAXLAYER] & no-of-layers 
// ADDED: use the old value of SRTT for bw/etc estimation !!! Jan 26
// XXX: need to be more compressed add more hooks to for ctrling from  
// tcl level
void QA::DumpInfo(double t, double last_t, double rate, 
		  double avgrate, double srtt)
{
#define MAXLEN 2000
	int i,j;
	char s1[MAXLEN], s2[MAXLEN], tmp[MAXLEN];
	static double last_srtt = 0, t2 = 0;
/*
        static double t1 = 0;
*/
#undef MAXLEN

	double  tot_bw = 0.0, interval, diff;
// 	if(rate > 1000000.0){
// 		debug("WARNING rate: %f is too large\n", rate);
// 	}
	
	interval = t - last_t ;
  
	if((t2 != last_t) && (t2 > 0)){
		diff = interval - last_srtt;
		if ((diff > 0.001) || (diff < -0.001)) {
			if (last_t == 0) 
				// Startup phase
				return;
/*
 			debug("WARNING: last_srtt: %.4f != \
interval: %.4f, diff: %f t1: %f, t2: %f, last_t: %f, t: %f\n",
 				last_srtt, interval, diff, t1, t2, last_t, t);
*/
			//abort();
		}
	} else 
		/* for the first call to init */
		last_srtt = srtt;

/*
	t1 = last_t;
*/
	t2 = t;

	if (interval <= 0.0) {
		panic("# ERROR interval is negative\n");
	}

	sprintf(s1, " %.2f %.2f %.2f X", last_t, rate, avgrate); 
	sprintf(s2, " %.2f %.2f %.2f X", t, rate, avgrate);
	
	j = 0;
	for (i = 0; i < MAX_LAYER; i++) 
	  //if (playing_[i] == 1)
	  if (sending_[i] == 1)
	    j++;
	//no of layers being playback
	sprintf(tmp, " %d", j*LAYERBW_);
	strcat(s1, tmp);
	strcat(s2, tmp);

	for (i = 0; i < MAX_LAYER; i++) {
		sprintf(tmp, " %.2g ", (bw_[i]/interval)+i*10000.0);
		strcat(s1,tmp);
		strcat(s2,tmp);

		tot_bw += bw_[i]/interval;
		bw_[i] = 0;
	}

	sprintf(tmp, " %.2f X", tot_bw );
	strcat(s1,tmp);
	strcat(s2,tmp);
  
	j = 0;
	for (i = 0; i < MAX_LAYER; i++) {
	  //if (playing_[i] == 1) {
	  if(sending_[i] ==1){
	    j++;
	    // drained_[] can be neg when allocated buf for this 
	    // layer is more than consumed data
	    if (drained_[i] < 0.0) {
	      // this means that this layer was drained 
	      // with max rate
	      drained_[i] = 0.0;
	    } 
	    // XXX, we could have used interval*LAYERBW_ - bw_[i]
	    // that was certainly better
	    sprintf(tmp, " %.2f ", 
		    (drained_[i]/interval)+i*10000.0);
	    strcat(s1,tmp);
	    strcat(s2,tmp);
	    // Note that drained[] shows the amount of data that 
	    // is used from buffered data, i.e. rd[i]
	    // This must be srtt instead of interval because this 
	    // is for next dumping.
	    drained_[i]=srtt*LAYERBW_;
	  } else {
	    sprintf(tmp, " %.2f ", i*10000.0);
	    strcat(s1,tmp);
	    strcat(s2,tmp);
	    drained_[i] = 0.0;
	  }
	}

	for (i=0;i<MAX_LAYER;i++) {
		sprintf(tmp, " %.2f", buffer_[i]+i*10000);
		strcat(s1,tmp);
		strcat(s2,tmp);
	}
	
	log("QA %s \n", s1);
	log("QA %s \n", s2);
	fflush(stdout);
}

// This routine models draining of buffers at the recv
// it periodically updates state of buffers
// Ir must be called once and then it reschedules itself
// it is first called after playout is started!
void QA::UpdateState()
{
	double last_ptime = playTime_; // Last time to drain buffer
	DrainBuffers();
	DumpInfo(Scheduler::instance().clock(), last_ptime, 
		 rate(), avgrate_, rap()->srtt());
}


