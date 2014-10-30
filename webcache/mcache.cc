/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * mcache.cc
 * Copyright (C) 1997 by the University of Southern California
 * $Id: mcache.cc,v 1.14 2011/10/15 16:01:05 tom_henderson Exp $
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
// Multimedia cache implementation
//
// $Header: /cvsroot/nsnam/ns-2/webcache/mcache.cc,v 1.14 2011/10/15 16:01:05 tom_henderson Exp $

#include <assert.h>
#include <stdio.h>

#include "rap/media-app.h"
#include "mcache.h"


MediaPage::MediaPage(const char *n, int s, double mt, double et, 
		     double a, int l) :
	ClientPage(n, s, mt, et, a), num_layer_(l), locked_(0), realsize_(0)
{
	for (int i = 0; i < num_layer_; i++) {
		hc_[i] = new HitCount(this, i);
		flags_[i] = 0;
	}
}

MediaPage::~MediaPage()
{
	int i; 
	for (i = 0; i < num_layer_; i++) {
		// Delete hit count list
		// These hit count records should have already been removed
		// from the cache's hit count list. 
		assert((hc_[i]->prev() == NULL) && (hc_[i]->next() == NULL));
		delete hc_[i];
		// Delete media segment list
		layer_[i].destroy();
	}
}

void MediaPage::print_info(char *buf) 
{
	ClientPage::print_info(buf);
	buf += strlen(buf);
	sprintf(buf, " pgtype MEDIA layer %d", num_layer_);
}

// Make the page full with stream data
void MediaPage::create()
{
	assert((num_layer_ >= 0) && (num_layer_ < MAX_LAYER));
	int i, sz = size_ / num_layer_;
	for (i = 0; i < num_layer_; i++) {
		// Delete whatever that was there. 
		layer_[i].destroy();
		add_segment(i, MediaSegment(0, sz));
		set_complete_layer(i);
	}
	realsize_ = size_;
}

void MediaPage::add_segment(int layer, const MediaSegment& s) 
{
	assert((layer >= 0) && (layer < MAX_LAYER));
	layer_[layer].add(s);
	realsize_ += s.datasize();
	if (s.is_last())
		set_complete_layer(layer);
}

int MediaPage::is_complete()
{
	// Consider a page finished when all NON-EMPTY layers are 
	// marked as "completed"
	for (int i = 0; i < num_layer_; i++)
		if (!is_complete_layer(i) && (layer_[i].length() > 0))
			return 0;
	return 1;
}

void MediaPage::set_complete()
{
	for (int i = 0; i < num_layer_; i++)
		set_complete_layer(i);
}

// Used for cache replacement
int MediaPage::evict_tail_segment(int layer, int size)
{
	if (is_locked() || is_tlocked())
		return 0;

	assert((layer >= 0) && (layer < MAX_LAYER));
	//#ifdef MCACHE_DEBUG
#if 0
	char buf[20];
	name(buf);
	fprintf(stderr, "Page %s evicted layer %d: ", buf, layer);
#endif
	int sz = layer_[layer].evict_tail(size);
	realsize_ -= sz;
	//#ifdef MCACHE_DEBUG
#if 0
	fprintf(stderr, "\n");
#endif
	return sz;
}



//----------------------------------------------------------------------
// Classes related to a multimedia client page pool
//
// HitCountList and MClientPagePool
//----------------------------------------------------------------------

void HitCountList::update(HitCount *h)
{
	HitCount *tmp = h->prev();
	if ((tmp != NULL) && (tmp->hc() < h->hc())) {
		// Hit count increased, need to move this one up
		detach(h);
		while ((tmp != NULL) && (tmp->hc() < h->hc())) {
			if ((tmp->page() == h->page()) &&
			    (tmp->layer() < h->layer()))
				// XXX Don't violate layer encoding within the
				// same page!
				break;
			tmp = tmp->prev();
		}
		if (tmp == NULL) 
			// Insert it at the head
			insert(h, head_);
		else 
			append(h, tmp);
	} else if ((h->next() != NULL) && (h->hc() < h->next()->hc())) {
		// Hit count decreased, need to move this one down
		tmp = h->next();
		detach(h);
		while ((tmp != NULL) && (h->hc() < tmp->hc())) {
			if ((h->page() == tmp->page()) && 
			    (h->layer() < tmp->layer()))
				// XXX Don't violate layer encoding within 
				// the same page!
				break;
			tmp = tmp->next();
		}
		if (tmp == NULL)
			// At the tail
			append(h, tail_);
		else
			insert(h, tmp);
	}
	// We may end up with two cases here:
	//
	// (1) tmp->hc()>h->hc() && tmp->layer()<h->layer(). This is
	// the normal case, where both hit count ordering and layer 
	// ordering are preserved;
	//
	// (2) tmp->hc()>h->hc() && tmp->layer()>h->layer(). In this
	// case, we should move h BEFORE tmp so that the layer 
	// ordering is not violated. We basically order the list using 
	// layer number as primary key, and use hit count as secondary
	// key. 
	// Note that the hit count ordering is only violated when more packets 
	// in layer i are dropped than those in layer i+1.
}

// Check the integrity of the resulting hit count list
void HitCountList::check_integrity()
{
	HitCount *p = (HitCount*)head_, *q;
	while (p != NULL) {
		q = p->next();
		while (q != NULL) {
			// Check layer ordering 
			if ((p->page() == q->page()) && 
			    (p->layer() > q->layer())) {
				fprintf(stderr, "Wrong hit count list.\n");
				abort();
			}
			q = q->next();
		}
		p = p->next();
	}
}

void HitCountList::add(HitCount *h)
{
	HitCount *tmp = (HitCount*)head_;

	// XXX First, ensure that the layer ordering within the same page
	// is not violated!!
	while ((tmp != NULL) && (tmp->hc() > h->hc())) {
		if ((tmp->page() == h->page()) && (tmp->layer() > h->layer()))
			break;
		tmp = tmp->next();
	}
	// Then order according to layer number
	while ((tmp != NULL) && (tmp->hc() == h->hc()) && 
	       (tmp->layer() < h->layer()))
		tmp = tmp->next();

	if (tmp == NULL) {
		if (head_ == NULL) 
			head_ = tail_ = h;
		else
			append(h, tail_);
		return;
	} else if ((tmp == head_) && 
		   ((tmp->hc() < h->hc()) || (tmp->layer() > h->layer()))) {
		insert(h, head_);
		return;
	}

	// Now tmp->hc()<=h->hc(), or tmp->hc()>h->hc() but 
	// tmp->layer()>h->layer(), insert h BEFORE tmp
	insert(h, tmp);
}

// Debug only
void HitCountList::print() 
{
	HitCount *p = (HitCount *)head_;
	int i = 0;
	char buf[20];
	while (p != NULL) {
		p->page()->name(buf);
	        fprintf(stderr, "(%s %d %f) ", buf, p->layer(), p->hc());
		if (++i % 4 == 0)
			printf("\n");
		p = p->next();
	}
	if (i % 4 != 0)
		fprintf(stderr, "\n");
}

//------------------------------
// Multimedia client page pool
//------------------------------
static class MClientPagePoolClass : public TclClass {
public:
        MClientPagePoolClass() : TclClass("PagePool/Client/Media") {}
        TclObject* create(int, const char*const*) {
		return (new MClientPagePool());
	}
} class_mclientpagepool_agent;

MClientPagePool::MClientPagePool() : 
	used_size_(0), repl_style_(FINEGRAIN)
{
	bind("max_size_", &max_size_);
	used_size_ = 0;
}

int MClientPagePool::command(int argc, const char*const* argv)
{
	if (argc == 3) 
		if (strcmp(argv[1], "set-repl-style") == 0) {
			// Set replacement style
			// <obj> set-repl-style <style>
			if (strcmp(argv[2], "FINEGRAIN") == 0) 
				repl_style_ = FINEGRAIN;
			else if (strcmp(argv[2], "ATOMIC") == 0)
				repl_style_ = ATOMIC;
			else {
				fprintf(stderr, "Unknown style %s", argv[3]);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
	return ClientPagePool::command(argc, argv);
}

void MClientPagePool::hc_update(const char *name, int max_layer)
{
	MediaPage *pg = (MediaPage*)get_page(name);
	assert(pg != NULL);

	int i;
	HitCount *h;
	// First we update the hit count of each layer of the given page
	for (i = 0; i <= max_layer; i++)
		pg->hit_layer(i);
	// Then we update the position of these hit count records
	for (i = 0; i <= max_layer; i++) {
		h = pg->get_hit_count(i);
		hclist_.update(h);
	}
#if 1
	hclist_.check_integrity();
#endif
}

// Add a segment to an object, and adjust hit counts accordingly
// XXX Call cache replacement algorithm if necessary
int MClientPagePool::add_segment(const char* name, int layer, 
				 const MediaSegment& s)
{
	MediaPage* pg = (MediaPage *)get_page(name);
	if (pg == NULL)
		return -1;
	if (layer >= pg->num_layer()) {
		if (s.datasize() == 0)
			return 0;
		else {
			fprintf(stderr, 
				"MClientPagePool: cannot add a new layer.\n");
			abort();
		}
	}

	// Check space availability
	if (used_size_ + s.datasize() > max_size_) {
		// If atomic replacement is used, page size is deducted in
		// remove_page(). If fine-grain is used, evicted size is 
		// deducted in repl_finegrain().
		cache_replace(pg, s.datasize());
		//#ifdef MCACHE_DEBUG
#if 0
		fprintf(stderr, 
			"Replaced for page %s segment (%d %d) layer %d\n",
			name, s.start(), s.end(), layer);
#endif
	} 
	// Add new page. When we are doing atomic replacement, the size that
	// we evicted may be larger than what we add.
	used_size_ += s.datasize();

	// If this layer was not 'in' before, add its hit count block
	if (pg->layer_size(layer) == 0)
		hclist_.add(pg->get_hit_count(layer));

	// Add new segment
	pg->add_segment(layer, s);

	return 0;
}

void MClientPagePool::fill_page(const char* pgname)
{
	MediaPage *pg = (MediaPage*)get_page(pgname);
	used_size_ -= pg->realsize();
	// Lock this page before we do any replacement. 
	pg->lock();
	pg->create();
	// If we cannot hold the nominal size of the page, do replacement
	if (used_size_ + pg->size() > max_size_)
		// Size deduction has already been done in remove_page()
		cache_replace(pg, pg->size());
	used_size_ += pg->size();
	pg->unlock();
}

ClientPage* MClientPagePool::enter_page(int argc, const char*const* argv)
{
	double mt = -1, et, age = -1, noc = 0;
	int size = -1, media_page = 0, layer = -1;
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
		else if (strcmp(argv[i], "pgtype") == 0) {
			if (strcmp(argv[i+1], "MEDIA") == 0)
				media_page = 1;
		} else if (strcmp(argv[i], "layer") == 0)
			layer = atoi(argv[i+1]);
	}
	// XXX allow mod time < 0 and age < 0!
	if ((size < 0) || (media_page && (layer <= 0))) {
		fprintf(stderr, "%s: wrong page information %s\n",
			name_, argv[2]);
		return NULL;
	}
	et = Scheduler::instance().clock();
	ClientPage *pg;
	if (media_page)
		pg = new MediaPage(argv[2], size, mt, et, age, layer);
	else 
		pg = new ClientPage(argv[2], size, mt, et, age);
	if (add_page(pg) < 0) {
		delete pg;
		return NULL;
	}
	if (noc) 
		pg->set_uncacheable();
	if (media_page) 
		((MediaPage *)pg)->lock();
	return pg;
}

int MClientPagePool::cache_replace(ClientPage *pg, int size)
{
	switch (repl_style_) {
	case FINEGRAIN:
		return repl_finegrain(pg, size);
	case ATOMIC:
#if 0
		char tmp[128];
		pg->name(tmp);
		fprintf(stderr, "Replaced for page %s size %d\n", tmp, size);
		fprintf(stderr, "Used size %d, max size %d\n", used_size_, 
			max_size_);
#endif
		return repl_atomic(pg, size);
	default:
		fprintf(stderr, "Corrupted replacement style.\n");
		abort();
	}
	// To make msvc happy
	return -1;
}

int MClientPagePool::repl_atomic(ClientPage*, int size)
{
	// XXX We use standard LRU to determine the stream to be kicked out.
	// The major problem is that we do not keep discrete hit counts. 
	// We solve the problem by using hit counts of the base layer as 
	// a close approximate. Because whenever a stream is accessed, 
	// it's assumed that the client bw can always afford the base layer,
	// this should be a fairly good approximation. 

	HitCount *h, *p;
	int sz, totalsz = 0;
	// Repeatedly get rid of streams until get enough space
	h = (HitCount*)hclist_.tail();
	while (h != NULL) {
		if (h->layer() != 0) {
			// We only look for the base layer
			h = h->prev();
			continue;
		}
		MediaPage *pg = (MediaPage *)h->page();
		// Don't touch locked pages
		if (pg->is_tlocked() || pg->is_locked()) {
			h = h->prev();
			continue;
		}
		sz = pg->realsize();
		totalsz += sz;
		char tmp[HTTP_MAXURLLEN];
		pg->name(tmp);
		// Before we delete, find the previous hit count record that
		// does not belong to this page. 
		p = h->prev(); 
		while ((p != NULL) && (p->page() == h->page()))
			p = p->prev();
		h = p;
		// XXX Manually remove hit count before deleting it
		for (int i = 0; i < pg->num_layer(); i++) {
			p = pg->get_hit_count(i);
			hclist_.detach(p);
		}
		// Delete the page, together with its media segment list
#if 0
		fprintf(stderr, "At time %g, atomic replacement evicted page %s\n", 
			Scheduler::instance().clock(), tmp); 
		fprintf(stderr, "Hit count list: \n");
		hclist_.print();
		fprintf(stderr,"----------------------------------------\n\n");
#endif		
		remove_page(tmp);
		if (sz >= size)
			return totalsz;
		// Continue to evict to meet the space requirement
		size -= sz;
	}
	fprintf(stderr, "Cache replacement cannot get enough space.\n");
	abort();
	return 0; // Make msvc happy
}

int MClientPagePool::repl_finegrain(ClientPage *, int size)
{
	// Traverse through hit count table, evict segments from the tail
	// of a layer with minimum hit counts
	HitCount *h, *p;
	int sz, totalsz = 0;

	// Repeatedly evict pages/segments until get enough space
	h = (HitCount*)hclist_.tail();
	while (h != NULL) {
		MediaPage *pg = (MediaPage *)h->page();
		// XXX Don't touch locked pages
		if (pg->is_tlocked() || pg->is_locked()) {
			h = h->prev();
			continue;
		}
		// Try to get "size" space by evicting other segments
		sz = pg->evict_tail_segment(h->layer(), size);
		// Decrease the cache used space
		used_size_ -= sz;
		totalsz += sz;
		// If we have not got enough space, we must have got rid of 
		// the entire layer
		assert((sz == size) || 
		       ((sz < size) && (pg->layer_size(h->layer()) == 0)));

		// If we don't have anything of this layer left, get rid of 
		// the hit count record. 
		// XXX Must do this BEFORE removing the page
		p = h;
		h = h->prev();
		if (pg->layer_size(p->layer()) == 0) {
			// XXX Should NEVER delete a hit count record!!
			// A hit count record is ONLY deleted when the page
			// is deleted (evicted from cache: ~MediaPage())
			hclist_.detach(p);
			p->reset();
		}
		// Furthermore, if the page has nothing left, get rid of it
		if (pg->realsize() == 0) {
			// NOTE: we do not manually remove hit counts of 
			// this page because if its realsize is 0, all 
			// hit count records must have already been 
			// detached from the page. 
			char tmp[HTTP_MAXURLLEN];
			pg->name(tmp);
#if 0
			fprintf(stderr, "At time %g, fine-grain evicted page %s\n",
				Scheduler::instance().clock(), tmp);
			fprintf(stderr, "Hit count list: \n");
			hclist_.print();
			fprintf(stderr,
				"---------------------------------------\n\n");
#endif
			// Then the hit count record will be deleted in here
			remove_page(tmp);
		}
		// If we've got enough space, return; otherwise continue
		if (sz >= size)
			return totalsz;
		size -= sz;	// Evict to fill the rest
	}
	fprintf(stderr, "Cache replacement cannot get enough space.\n");
	abort();
	return 0; // Make msvc happy
}

// Clean all hit count record of a page regardless of whether it's in the 
// hit count list. Used when hclist_ is not used at all, e.g., by MediaClient.
int MClientPagePool::force_remove(const char *name)
{
	// XXX Bad hack. Needs to integrate this into ClientPagePool.
	ClientPage *pg = (ClientPage*)get_page(name);
	// We should not remove a non-existent page!!
	assert(pg != NULL);
	if (pg->type() == MEDIA) {
		HitCount *p;
		MediaPage *q = (MediaPage*)pg;
		used_size_ -= q->realsize();
		for (int i = 0; i < q->num_layer(); i++) {
			p = q->get_hit_count(i);
			hclist_.detach(p);
		}
	} else if (pg->type() == HTML)
		used_size_ -= pg->size();
	return ClientPagePool::remove_page(name);
}

int MClientPagePool::remove_page(const char *name)
{
	// XXX Bad hack. Needs to integrate this into ClientPagePool.
	ClientPage *pg = (ClientPage*)get_page(name);
	// We should not remove a non-existent page!!
	assert(pg != NULL);
	if (pg->type() == MEDIA)
		used_size_ -= ((MediaPage *)pg)->realsize();
	else if (pg->type() == HTML)
		used_size_ -= pg->size();
	return ClientPagePool::remove_page(name);
}



//------------------------------------------------------------
// MediaPagePool
// Generate requests and pages for clients and servers 
//------------------------------------------------------------
static class MediaPagePoolClass : public TclClass {
public:
        MediaPagePoolClass() : TclClass("PagePool/Media") {}
        TclObject* create(int, const char*const*) {
		return (new MediaPagePool());
	}
} class_mediapagepool_agent;

MediaPagePool::MediaPagePool() : PagePool()
{
	size_ = NULL;
	duration_ = 0;
	layer_ = 1;
}

// For now, only one page, fixed size, fixed layer
int MediaPagePool::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();

	if (argc == 2) {
		if (strcmp(argv[1], "get-poolsize") == 0) { 
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
			// Generating requested page id
			if (rvReq_ == NULL) {
				tcl.add_errorf("no page id ranvar.");
				return TCL_ERROR;
			}
			int p = (int)rvReq_->value();
			assert((p >= 0) && (p < num_pages_));
			tcl.resultf("%d", p);
			return TCL_OK;
		} else if (strcmp(argv[1], "is-media-page") == 0) {
			// XXX Currently all pages are media pages. Should
			// be able to allow both normal pages and media pages
			// in the future
			tcl.result("1");
			return TCL_OK;
		} else if (strcmp(argv[1], "get-layer") == 0) {
			// XXX Currently all pages have the same number of 
			// layers. Should be able to change this in future.
			tcl.resultf("%d", layer_); 
			return TCL_OK;
		} else if (strcmp(argv[1], "set-start-time") == 0) {
			double st = strtod(argv[2], NULL);
			start_time_ = st;
			end_time_ = st + duration_;
			return TCL_OK;
		} else if (strcmp(argv[1], "set-duration") == 0) {
			// XXX Need this info to set page mod time!!
			duration_ = atoi(argv[2]);
			end_time_ = start_time_ + duration_;
			return TCL_OK;
		} else if (strcmp(argv[1], "gen-init-modtime") == 0) {
			// XXX We are not interested in page consistency here,
			// so never change this page.
			tcl.resultf("%d", -1);
			return TCL_OK;
		} else if (strcmp(argv[1], "gen-size") == 0) {
			int pagenum = atoi(argv[2]);
			if (pagenum >= num_pages_) {
				tcl.add_errorf("Invalid page id %d", pagenum);
				return TCL_ERROR;
			}
			tcl.resultf("%d", size_[pagenum]);
			return TCL_OK;
		} else if (strcmp(argv[1], "set-layer") == 0) {
			layer_ = atoi(argv[2]);
			return TCL_OK;
		} else if (strcmp(argv[1], "set-num-pages") == 0) {
			if (size_ != NULL) {
				tcl.add_errorf("can't change number of pages");
				return TCL_ERROR;
			}
			num_pages_ = atoi(argv[2]);
			size_ = new int[num_pages_];
			return TCL_OK;
		} else if (strcmp(argv[1], "ranvar-req") == 0) {
			rvReq_ = (RandomVariable*)TclObject::lookup(argv[2]);
			return TCL_OK;
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "gen-modtime") == 0) {
			// This should never be called, because we never
			// deals with page modifications!!
			fprintf(stderr, "%s: gen-modtime called!\n", name());
			abort();
		} else if (strcmp(argv[1], "set-pagesize") == 0) {
			// <pagepool> set-pagesize <pagenum> <size>
			int pagenum = atoi(argv[2]);
			if (pagenum >= num_pages_) {
				tcl.add_errorf("Invalid page id %d", pagenum);
				return TCL_ERROR;
			}
			size_[pagenum] = atoi(argv[3]);
			return TCL_OK;
		}
	}
	return PagePool::command(argc, argv);
}



//----------------------------------------------------------------------
// PagePool that generates requests using the SURGE model
//
// Part of the code by Paul Barford (barford@cs.bu.edu).
// Copyright (c) 1997 Trustees of Boston University
//
// Allow two options: (1) setting if all pages are media page or normal
// HTTP pages; (2) average page size
//----------------------------------------------------------------------
//  static class SurgePagePoolClass : public TclClass {
//  public:
//          SurgePagePoolClass() : TclClass("PagePool/Surge") {}
//          TclObject* create(int, const char*const*) {
//  		return (new SurgePagePool());
//  	}
//  } class_surgepagepool_agent;

//  SurgePagePool::SurgePagePool() : PagePool()
//  {
//  }



//----------------------------------------------------------------------
// Multimedia web applications: cache, etc.
//----------------------------------------------------------------------

static class MediaCacheClass : public TclClass {
public:
	MediaCacheClass() : TclClass("Http/Cache/Media") {}
	TclObject* create(int, const char*const*) {
		return (new MediaCache());
	}
} class_mediacache;

// By default we use online prefetching
MediaCache::MediaCache() : pref_style_(ONLINE_PREF)
{
	cmap_ = new Tcl_HashTable;
	Tcl_InitHashTable(cmap_, TCL_ONE_WORD_KEYS);
}

MediaCache::~MediaCache()
{
	Tcl_HashEntry *he;
	Tcl_HashSearch hs;
	if (cmap_) {
		for (he = Tcl_FirstHashEntry(cmap_, &hs);  he != NULL;
		     he = Tcl_NextHashEntry(&hs))
			delete (RegInfo*)Tcl_GetHashValue(he);
		Tcl_DeleteHashTable(cmap_);
		delete cmap_;
	}
}

AppData* MediaCache::get_data(int& size, AppData* req)
{
	assert(req != NULL);
	if (req->type() != MEDIA_REQUEST) {
		return HttpApp::get_data(size, req);
	}

	MediaRequest *r = (MediaRequest *)req;

	// Get statistics block for the requestor
	Tcl_HashEntry *he = 
		Tcl_FindHashEntry(cmap_, (const char *)(r->app()));
	assert(he != NULL);
	RegInfo *ri = (RegInfo *)Tcl_GetHashValue(he);

	// Process request
	if (r->request() == MEDIAREQ_GETSEG) {
		// Get a new data segment
		MediaPage* pg = (MediaPage*)pool_->get_page(r->name());
		assert(pg != NULL);
		MediaSegment s1(r->st(), r->et());
		MediaSegment s2 = pg->next_overlap(r->layer(), s1);
		HttpMediaData *p;
		if (s2.datasize() == 0) {
			// No more data available for this layer, allocate
			// an ADU with data size 0 to signal the end
			// of transmission for this layer
			size = 0;
			p = new HttpMediaData(name(), r->name(),
					      r->layer(), 0, 0);
		} else {
			size = s2.datasize();
			p = new HttpMediaData(name(), r->name(),
					   r->layer(), s2.start(), s2.end());
		}
		// XXX If we are still receiving the stream, don't 
		// ever say that this is the last segment. If the 
		// page is not locked, it's still possible that we
		// return a NULL segment because the requested one
		// is not available. Don't set the 'LAST' flag in this 
		// case.
		if (s2.is_last()) {
			p->set_last();
			if (!pg->is_locked() && (s2.datasize() == 0) && 
			    (r->layer() == 0)) 
				p->set_finish();
		}

		//----------------------------------------
		// Update statistics of this connection
		//----------------------------------------
		// Update the highest layer that this client has requested
		if (ri->hl_ < r->layer())
			ri->hl_ = r->layer();
		if (size > 0) {
			// Update total delivered bytes
			ri->db_[r->layer()] += size;
			// Update prefetched bytes that've been delivered
			ri->eb_[r->layer()] += ri->pref_size(r->layer(), s2);
		}
		return p;
	} else if (r->request() == MEDIAREQ_CHECKSEG) {
		// If we are not doing online prefetching, return nothing
		if (pref_style_ != ONLINE_PREF)
			return NULL;
		// Check the availability of a new data segment
		// And refetch if it is not available
		MediaPage* pg = (MediaPage*)pool_->get_page(r->name());
		assert(pg != NULL);
		if (pg->is_locked()) 
			// If we are during the first retrieval, don't prefetch
			return NULL;
		MediaSegmentList ul = pg->is_available(r->layer(),
				      MediaSegment(r->st(),r->et()));
		if (ul.length() == 0)
			// All segments are available
			return NULL;
		// Otherwise do prefetching on these "holes"
		char *buf = ul.dump2buf();
		Tcl::instance().evalf("%s pref-segment %s %s %d %s", name(), 
				      r->app()->name(), r->name(), 
				      r->layer(), buf);
//  		log("E PREF p %s l %d %s\n", r->name(), r->layer(), buf);
		delete []buf;
		ul.destroy();

		// Update the highest layer that this client has requested
		Tcl_HashEntry *he = 
			Tcl_FindHashEntry(cmap_, (const char *)(r->app()));
		assert(he != NULL);
		RegInfo *ri = (RegInfo *)Tcl_GetHashValue(he);
		if (ri->hl_ < r->layer())
			ri->hl_ = r->layer();
		return NULL;
	}
		
	fprintf(stderr, 
		"MediaCache %s gets an unknown MediaRequest type %d\n",
		name(), r->request());
	abort();
	return NULL; // Make msvc happy
}

// Add received media segment into page pool
void MediaCache::process_data(int size, AppData* data) 
{
	switch (data->type()) {
	case MEDIA_DATA: {
		HttpMediaData* d = (HttpMediaData*)data;
		// Cache this segment, do replacement if necessary
		if (mpool()->add_segment(d->page(), d->layer(), 
					 MediaSegment(*d)) == -1) {
			fprintf(stderr, "MediaCache %s gets a segment for an "
				"unknown page %s\n", name(), d->page());
			abort();
		}
		if (d->is_pref()) {
			// Update total prefetched bytes
			Tcl_HashEntry *he = Tcl_FindHashEntry(cmap_, 
						(const char*)(d->conid()));
			// Client-cache-server disconnection procedure:
			// (1) client disconnects from cache, then
			// (2) cache disconnects from server and shuts down 
			//     prefetching channel. 
			// Therefore, after client disconnects, the cache 
			// may still receive a few prefetched segments. 
			// Ignore those because we no longer keep statistics
			// about the torn-down connection.
			if (he != NULL) {
				RegInfo *ri = (RegInfo *)Tcl_GetHashValue(he);
				ri->add_pref(d->layer(), MediaSegment(*d));
				ri->pb_[d->layer()] += d->datasize();
			}
		}
		// XXX debugging only
#if 1
  		log("E RSEG p %s l %d s %d e %d z %d f %d\n", 
  		    d->page(), d->layer(), d->st(), d->et(), d->datasize(),
  		    d->is_pref());
#endif
		break;
	} 
	default:
		HttpCache::process_data(size, data);
	}
}

int MediaCache::command(int argc, const char*const* argv) 
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "get-pref-style") == 0) {
			switch (pref_style_) {
			case NOPREF:
				tcl.result("NOPREF");
				break;
			case ONLINE_PREF:
				tcl.result("ONLINE_PREF");
				break;
			case OFFLINE_PREF:
				tcl.result("OFFLINE_PREF");
				break;
			default:
				fprintf(stderr, 
					"Corrupted prefetching style %d", 
					pref_style_);
				return TCL_ERROR;
			}
			return TCL_OK;
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "offline-complete") == 0) {
			// Delete whatever segments in the given page, 
			// make it complete. Used by offline prefetching
			ClientPage *pg = mpool()->get_page(argv[2]);
			if (pg == NULL)
				// XXX It's possible that we've already kicked
				// it out of the cache. Do nothing.
				return TCL_OK;
			assert(pg->type() == MEDIA);
			assert(!((MediaPage*)pg)->is_locked());
			mpool()->fill_page(argv[2]);
			return TCL_OK;
		} else if (strcmp(argv[1], "set-pref-style") == 0) {
			// Set prefetching style
			// <obj> set-pref-style <style>
			//
			// style can be: NOPREF, ONLINE_PREF, OFFLINE_PREF
			if (strcmp(argv[2], "NOPREF") == 0) 
				pref_style_ = NOPREF;
			else if (strcmp(argv[2], "ONLINE_PREF") == 0) 
				pref_style_ = ONLINE_PREF;
			else if (strcmp(argv[2], "OFFLINE_PREF") == 0) 
				pref_style_ = OFFLINE_PREF;
			else {
				fprintf(stderr, "Wrong prefetching style %s",
					argv[2]);
				return TCL_ERROR;
			}
			return TCL_OK;
		} else if (strcmp(argv[1], "dump-page") == 0) {
			// Dump segments of a given page
			ClientPage *p=(ClientPage*)mpool()->get_page(argv[2]);
			if (p->type() != MEDIA)
				// Do nothing for non-media pages
				return TCL_OK;
			MediaPage *pg = (MediaPage *)p;
			char *buf;
			for (int i = 0; i < pg->num_layer(); i++) {
				buf = pg->print_layer(i);
				if (strlen(buf) > 0)
					log("E SEGS p %s l %d %s\n", argv[2], 
					    i, buf);
				delete []buf;
			}
			return TCL_OK;
		} else if (strcmp(argv[1], "stream-received") == 0) {
			// We've got the entire page, unlock it
			MediaPage *pg = (MediaPage*)mpool()->get_page(argv[2]);
			assert(pg != NULL);
			pg->unlock();
			// XXX Should we clear all "last" flag of segments??
#ifdef MCACHE_DEBUG
			// Printing out current buffer status of the page
			char *buf;
			for (int i = 0; i < pg->num_layer(); i++) {
				buf = pg->print_layer(i);
				log("E SEGS p %s l %d %s\n", argv[2], i, buf);
				delete []buf;
			}
#endif
			// Show cache free size
			log("E SIZ n %d z %d t %d\n", mpool()->num_pages(),
			    mpool()->usedsize(), mpool()->maxsize());
			return TCL_OK;
		}
	} else if (argc == 5) {
		if (strcmp(argv[1], "register-client") == 0) {
			// <server> register-client <app> <client> <pageid>
			TclObject *a = TclObject::lookup(argv[2]);
			assert(a != NULL);
			int newEntry;
			Tcl_HashEntry *he = Tcl_CreateHashEntry(cmap_, 
					(const char *)a, &newEntry);
			if (he == NULL) {
				tcl.add_errorf("cannot create hash entry");
				return TCL_ERROR;
			}
			if (!newEntry) {
				tcl.add_errorf("duplicate connection");
				return TCL_ERROR;
			}
			RegInfo *p = new RegInfo;
			p->client_ = (HttpApp*)TclObject::lookup(argv[3]);
			assert(p->client_ != NULL);
			strcpy(p->name_, argv[4]);
			Tcl_SetHashValue(he, (ClientData)p);

			// Lock the page while transmitting it to a client
			MediaPage *pg = (MediaPage*)mpool()->get_page(argv[4]);
			assert((pg != NULL) && (pg->type() == MEDIA));
			pg->tlock();

			return TCL_OK;
		} else if (strcmp(argv[1], "unregister-client") == 0) {
			// <server> unregister-client <app> <client> <pageid>
			TclObject *a = TclObject::lookup(argv[2]);
			assert(a != NULL);
			Tcl_HashEntry *he = 
				Tcl_FindHashEntry(cmap_, (const char*)a);
			if (he == NULL) {
				tcl.add_errorf("cannot find hash entry");
				return TCL_ERROR;
			}
			RegInfo *ri = (RegInfo*)Tcl_GetHashValue(he);
			// Update hit count
			mpool()->hc_update(argv[4], ri->hl_);
#ifdef MCACHE_DEBUG
			printf("Cache %d hit counts: \n", id_);
			mpool()->dump_hclist();
#endif
			// Dump per-connection statistics
			for (int i = 0; i <= ri->hl_; i++)
				log("E STAT p %s l %d d %d e %d p %d\n",
				    ri->name_, i, ri->db_[i], ri->eb_[i], 
				    ri->pb_[i]);
			delete ri;
			Tcl_DeleteHashEntry(he);

			// Lock the page while transmitting it to a client
			MediaPage *pg = (MediaPage*)mpool()->get_page(argv[4]);
			assert((pg != NULL) && (pg->type() == MEDIA));
			pg->tunlock();

			return TCL_OK;
		}
	}

	return HttpCache::command(argc, argv);
}



//----------------------------------------------------------------------
// Media web client 
//   Use C++ interface to records quality of received stream.
// NOTE: 
//   It has OTcl inheritance, but no C++ inheritance!
//----------------------------------------------------------------------

static class HttpMediaClientClass : public TclClass {
public:
	HttpMediaClientClass() : TclClass("Http/Client/Media") {}
	TclObject* create(int, const char*const*) {
		return (new MediaClient());
	}
} class_httpmediaclient;

// Records the quality of stream received
void MediaClient::process_data(int size, AppData* data)
{
	assert(data != NULL);

	switch (data->type()) {
	case MEDIA_DATA: {
		HttpMediaData* d = (HttpMediaData*)data;
		// XXX Don't pass any data to page pool!!
   		if (mpool()->add_segment(d->page(), d->layer(), 
   					 MediaSegment(*d)) == -1) {
   			fprintf(stderr, 
  "MediaCache %s gets a segment for an unknown page %s\n", name(), d->page());
//     			abort();
   		}
		// Note: we store the page only to produce some statistics
		// later so that we need not do postprocessing of traces.
#if 1
    		log("C RSEG p %s l %d s %d e %d z %d\n", 
    		    d->page(), d->layer(), d->st(), d->et(), d->datasize());
#endif
		break;
	}
	default:
		HttpClient::process_data(size, data);
	}
}

int MediaClient::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "stream-received") == 0) {
			// XXX This is the place to do statistics collection
			// about quality of received stream.
			// 
			// Dump delivered quality log
			MediaPage *pg = (MediaPage*)mpool()->get_page(argv[2]);
			assert(pg != NULL);
			// Printing out current buffer status of the page
			char *buf;
			for (int i = 0; i < pg->num_layer(); i++) {
				buf = pg->print_layer(i);
				if (strlen(buf) > 0) 
					log("C SEGS p %s l %d %s\n", 
					    argv[2], i, buf);
				delete []buf;
			}
			// then delete the stream from buffer
			mpool()->force_remove(argv[2]);
			return TCL_OK;
		}
	}
	return HttpClient::command(argc, argv);
}



//----------------------------------------------------------------------
// Multimedia web server
//----------------------------------------------------------------------

static class MediaServerClass : public TclClass {
public:
	MediaServerClass() : TclClass("Http/Server/Media") {}
	TclObject* create(int, const char*const*) {
		return (new MediaServer());
	}
} class_mediaserver;

MediaServer::MediaServer() : HttpServer() 
{
	pref_ = new Tcl_HashTable;
	Tcl_InitHashTable(pref_, TCL_STRING_KEYS);
	cmap_ = new Tcl_HashTable;
	Tcl_InitHashTable(cmap_, TCL_ONE_WORD_KEYS);
}

MediaServer::~MediaServer() 
{
	Tcl_HashEntry *he;
	Tcl_HashSearch hs;
	if (pref_ != NULL) {
		for (he = Tcl_FirstHashEntry(pref_, &hs);  he != NULL;
		     he = Tcl_NextHashEntry(&hs)) {
			PrefInfo *pi = (PrefInfo*)Tcl_GetHashValue(he);
			pi->sl_->destroy();
			delete pi->sl_;
		}
		Tcl_DeleteHashTable(pref_);
		delete pref_;
	}
	if (cmap_ != NULL) {
		for (he = Tcl_FirstHashEntry(cmap_, &hs);  he != NULL;
		     he = Tcl_NextHashEntry(&hs))
			delete (RegInfo*)Tcl_GetHashValue(he);
		Tcl_DeleteHashTable(cmap_);
		delete cmap_;
	}
}

// Return the next segment to be sent to a particular application
MediaSegment MediaServer::get_next_segment(MediaRequest *r, Application*& ci)
{
	MediaPage* pg = (MediaPage*)pool_->get_page(r->name());
	assert(pg != NULL);

	// XXX Extremely hacky way to map media app names to 
	// HTTP connections. Should maintain another hash table for this.
	RegInfo *ri = get_reginfo(r->app());
	assert(ri != NULL);
	PrefInfoQ* q = get_piq(r->name(), ri->client_);

	// We are not on the prefetching list, send a normal data segment
	if ((q == NULL) || (q->is_empty())) {
		MediaSegment s1(r->st(), r->et());
		return pg->next_overlap(r->layer(), s1);
	}

	// Cycle through the prefetched segments that we need to send
	int found = 0;
	int searched = 0;
	PrefInfo *pi; 
	while (!found) {
		PrefInfoE *pe = q->dequeue();
		pi = pe->data();
		q->enqueue(pe);
		// If there's a pending segment in any layer, send it
		for (int i = 0; i < pg->num_layer(); i++) 
			if (pi->sl_[i].length() > 0) 
				found = 1;
		// If no pending prefetched segments, return empty
		if (searched++ == q->size()) 
			return MediaSegment(0, 0);
	}

	// Send a segment from the prefetching list. Only use the data size
	// included in the request.
	MediaSegmentList *p = pi->sl_;
	// Set return conid
	ci = pi->conid_;

	// Find one available segment in prefetching list if there is none
	// in the given layer
	int l = r->layer(), i = 0;
	MediaSegment res;
	while ((res.datasize() == 0) && (i < pg->num_layer())) {
		// next() doesn't work. Need a method that returns the 
		// *FIRST* non-empty segment which satisfies the size 
		// constraint.
		res = p[l].get_nextseg(MediaSegment(0, r->datasize()));
		i++;
		l = (l+1) % pg->num_layer();
	}
	// XXX We must do boundary check of the prefetched segments to make
	// sure that the start and end offsets are valid!
	if (res.start() < 0) 
		res.set_start(0);
	if (res.end() > pg->layer_size(l))
		res.set_end(pg->layer_size(l));
	if (res.datasize() > 0) {
		// XXX We may end up getting data from another layer!!
		l = (l-1+pg->num_layer()) % pg->num_layer();
		if (l != r->layer())
			r->set_layer(l);
		// We may not be able to get the specified data size, due 
		// to arbitrary stream lengths
		//assert(res.datasize() == r->datasize());
		p[r->layer()].evict_head(r->datasize());
	}
	// Set the prefetching flag of this segment
	res.set_pref();
	return res;
}

// Similar to MediaCache::get_data(), but ignore segment availability checking
AppData* MediaServer::get_data(int& size, AppData *req)
{
	assert((req != NULL) && (req->type() == MEDIA_REQUEST));
	MediaRequest *r = (MediaRequest *)req;
	Application* conid = NULL;

	if (r->request() == MEDIAREQ_GETSEG) {
		// Get a new data segment
		MediaSegment s2 = get_next_segment(r, conid);
		HttpMediaData *p;
		if (s2.datasize() == 0) {
			// No more data available for this layer, most likely
			// it's because this layer is finished.
			size = 0;
			p = new HttpMediaData(name(), r->name(),
					      r->layer(), 0, 0);
		} else {
			size = s2.datasize();
			p = new HttpMediaData(name(), r->name(),
					   r->layer(), s2.start(), s2.end());
		}
		if (s2.is_last()) {
			p->set_last();
			// Tear down the connection after we've sent the last
			// segment of the base layer and are requested again.
			if ((s2.datasize() == 0) && (r->layer() == 0))
				p->set_finish();
		}
		if (s2.is_pref()) {
			// Add connection id into returned data
			p->set_conid(conid);
			p->set_pref();
		}
		return p;
	} else if (r->request() == MEDIAREQ_CHECKSEG) 
		// We don't need to return anything, so just NULL
		return NULL;
	else {
		fprintf(stderr, 
		       "MediaServer %s gets an unknown MediaRequest type %d\n",
			name(), r->request());
		abort();
	}
	/*NOTREACHED*/
	return NULL; // Make msvc happy
}

int MediaServer::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "is-media-page") == 0) {
			ClientPage *pg = pool_->get_page(argv[2]);
			if (pg && (pg->type() == MEDIA))
				tcl.result("1");
			else 
				tcl.result("0");
			return TCL_OK;
		}
	} else if (argc == 5) { 
		if (strcmp(argv[1], "stop-prefetching") == 0) {
			/*
			 * <server> stop-prefetching <Client> <conid> <pagenum>
			 */
			HttpApp *app = static_cast <HttpApp *> (TclObject::lookup(argv[2]));
			assert(app != NULL);
			int id = atoi (argv[4]);
			PageID pageId (app, id);
			int length = strlen (app->name()) + strlen (argv[4]) + 2;
			char* buf = new char[length];
			memset(buf, 0, sizeof(buf));
			sprintf(buf, "%s:%-d", app->name(), id);
			Tcl_HashEntry *he = 
				Tcl_FindHashEntry(pref_, buf);
			delete [] buf;
			if (he == NULL) {
				tcl.add_errorf(
				  "Server %d cannot stop prefetching!\n", id_);
				return TCL_ERROR;
			}
			TclObject *conId = TclObject::lookup(argv[3]);
			assert(conId != NULL);
			PrefInfoQ *q = (PrefInfoQ*)Tcl_GetHashValue(he);
			PrefInfoE *pe = find_prefinfo(q, (Application*)conId);
			assert(pe != NULL);
			PrefInfo *pi = pe->data();
			MediaSegmentList *p = pi->sl_;
			assert(p != NULL);
			for (int i = 0; i < MAX_LAYER; i++)
				p[i].destroy();
			delete []p;
			delete pi;
			q->detach(pe);
			delete pe;
			// If no more prefetching streams left for this client,
			// delete all the information.
			// Return 0 means that we still have prefetching 
			// clients left, don't tear down the channel yet. 
			// Otherwise return 1. 
			int res = 0;
			if (q->is_empty()) {
				delete q;
				Tcl_DeleteHashEntry(he);
				res = 1;
			}
			tcl.resultf("%d", res);
			return (TCL_OK);
		} else if (strcmp(argv[1], "register-client") == 0) {
			// <cache> register-client <app> <client> <pageid>
			TclObject *a = TclObject::lookup(argv[2]);
			assert(a != NULL);
			int newEntry;
			Tcl_HashEntry *he = Tcl_CreateHashEntry(cmap_, 
					(const char *)a, &newEntry);
			if (he == NULL) {
				tcl.add_errorf("cannot create hash entry");
				return TCL_ERROR;
			}
			if (!newEntry) {
				tcl.add_errorf("duplicate connection");
				return TCL_ERROR;
			}
			RegInfo *p = new RegInfo;
			p->client_ = (HttpApp*)TclObject::lookup(argv[3]);
			assert(p->client_ != NULL);
			strcpy(p->name_, argv[4]);
			Tcl_SetHashValue(he, (ClientData)p);
			return TCL_OK;
		} else if (strcmp(argv[1], "unregister-client") == 0) {
			// <cache> unregister-client <app> <client> <pageid>
			TclObject *a = TclObject::lookup(argv[2]);
			assert(a != NULL);
			Tcl_HashEntry *he = 
				Tcl_FindHashEntry(cmap_, (const char*)a);
			if (he == NULL) {
				tcl.add_errorf("cannot find hash entry");
				return TCL_ERROR;
			}
			RegInfo *p = (RegInfo*)Tcl_GetHashValue(he);
			delete p;
			Tcl_DeleteHashEntry(he);
			return TCL_OK;
		}
	} else {
		if (strcmp(argv[1], "enter-page") == 0) {
			ClientPage *pg = pool_->enter_page(argc, argv);
			if (pg == NULL)
				return TCL_ERROR;
			if (pg->type() == MEDIA) 
				((MediaPage*)pg)->create();
			// Unlock the page after creation
			((MediaPage*)pg)->unlock(); 
			return TCL_OK;
		} else if (strcmp(argv[1], "register-prefetch") == 0) {
			/*
			 * <server> register-prefetch <client> <pagenum> 
			 * 	<conid> <layer> {<segments>}
			 * Registers a list of segments to be prefetched by 
			 * <client>, where each <segment> is a pair of 
			 * (start, end). <pagenum> should be pageid without 
			 * preceding [server:] prefix.
			 * 
			 * <conid> is the OTcl name of the original client 
			 * who requested the page. This is used for the cache
			 * to get statistics about a particular connection.
			 * 
			 * <client> is the requestor of the stream.
			 */
			HttpApp *app = static_cast <HttpApp *> (TclObject::lookup(argv[2]));
			assert(app != NULL);
			int id = atoi (argv[3]);
			PageID pageId (app, id);
			int newEntry = 1;
			int length = strlen (app->name()) + strlen (argv[3]) + 2;
			char* buf = new char[length];
			memset(buf, 0, sizeof(buf));
			sprintf(buf, "%s:%-d", app->name(), id);
			Tcl_HashEntry *he = Tcl_CreateHashEntry(pref_, 
					buf, &newEntry);
			delete [] buf;
			if (he == NULL) {
				fprintf(stderr, "Cannot create entry.\n");
				return TCL_ERROR;
			}
			PrefInfo *pi;
			PrefInfoE *pe;
			PrefInfoQ *q; 
			MediaSegmentList *p;
			TclObject *conId = TclObject::lookup(argv[4]);
			if (newEntry) {
				q = new PrefInfoQ;
				Tcl_SetHashValue(he, (ClientData)q);
				pe = NULL;
			} else {
				q = (PrefInfoQ *)Tcl_GetHashValue(he);
				pe = find_prefinfo(q, (Application*)conId);
			}
			if (pe == NULL) {
				pi = new PrefInfo;
				pi->conid_ = (Application*)conId;
				p = pi->sl_ = new MediaSegmentList[MAX_LAYER];
				q->enqueue(new PrefInfoE(pi));
			} else {
				pi = pe->data();
				p = pi->sl_;
			}
			assert((pi != NULL) && (p != NULL));
			// Preempt all old requests because they 
			// cannot reach the cache "in time"
			int layer = atoi(argv[5]);
			p[layer].destroy();
			// Add segments into prefetching list
			assert(argc % 2 == 0);
			for (int i = 6; i < argc; i+=2)
				p[layer].add(MediaSegment(atoi(argv[i]), 
							  atoi(argv[i+1])));
			return TCL_OK;
		}
	}
			
	return HttpServer::command(argc, argv);
}
