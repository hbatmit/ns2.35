/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * Copyright (C) 1998 by the University of Southern California
 * $Id: filter.cc,v 1.8 2005/08/25 18:58:02 johnh Exp $
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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /usr/src/mash/repository/vint/ns-2/filter.cc ";
#endif

#include "packet.h"
#include "filter.h"

static class FilterClass : public TclClass {
public:
	FilterClass() : TclClass("Filter") {}
	TclObject* create(int, const char*const*) {
		return (new Filter);
	}
} class_filter;

Filter::Filter() : filter_target_(0) 
{
}
Filter::filter_e Filter::filter(Packet* /*p*/) 
{
	return PASS;  // As simple connector
}
void Filter::recv(Packet* p, Handler* h)
{
	switch(filter(p)) {
	case DROP : 
		if (h) h->handle(p);
		drop(p);
		break;
	case DUPLIC :
		if (filter_target_)
			filter_target_->recv(p->copy(), h);
		/* fallthrough */
	case PASS :
		send(p, h);
		break;
	case FILTER :
		if (filter_target_)
			filter_target_->recv(p, h);
		break;
	}
}
int Filter::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "filter-target") == 0) {
			if (filter_target_ != 0)
				tcl.result(target_->name());
			return TCL_OK;
		}
	}
	else if (argc == 3) {
		if (strcmp(argv[1], "filter-target") == 0) {
			filter_target_ = (NsObject*)TclObject::lookup(argv[2]);
			return TCL_OK;
		}
	}
	return Connector::command(argc, argv);
}

static class FieldFilterClass : public TclClass {
public:
	FieldFilterClass() : TclClass("Filter/Field") {}
	TclObject* create(int, const char*const*) {
		return (new FieldFilter);
	}
} class_filter_field;

FieldFilter::FieldFilter() 
{
	bind("offset_", &offset_);
	bind("match_", &match_);
}
Filter::filter_e FieldFilter::filter(Packet *p) 
{
	return (*(int *)p->access(offset_) == match_) ? FILTER : PASS;
}

/* 10-5-98, Polly Huang, Filters that filter on multiple fields */
static class MultiFieldFilterClass : public TclClass {
public:
	MultiFieldFilterClass() : TclClass("Filter/MultiField") {}
	TclObject* create(int, const char*const*) {
		return (new MultiFieldFilter);
	}
} class_filter_multifield;

MultiFieldFilter::MultiFieldFilter() : field_list_(0)
{

}

void MultiFieldFilter::add_field(fieldobj *p) 
{
	p->next = field_list_;
	field_list_ = p;
}

MultiFieldFilter::filter_e MultiFieldFilter::filter(Packet *p) 
{
	fieldobj* tmpfield;

	tmpfield = field_list_;
	while (tmpfield != 0) {
		if (*(int *)p->access(tmpfield->offset) == tmpfield->match)
			tmpfield = tmpfield->next;
		else 
			return (PASS);
	}
	return(FILTER);
}


int MultiFieldFilter::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "filter-target") == 0) {
			if (filter_target_ != 0)
				tcl.result(target_->name());
			return TCL_OK;
		}
	}
	else if (argc == 3) {
		if (strcmp(argv[1], "filter-target") == 0) {
			filter_target_ = (NsObject*)TclObject::lookup(argv[2]);
			return TCL_OK;
		}
	}
       else if (argc == 4) {
		if (strcmp(argv[1], "filter-field") == 0) {
			fieldobj *tmp = new fieldobj;
			tmp->offset = atoi(argv[2]);
			tmp->match = atoi(argv[3]);
			add_field(tmp);
			return TCL_OK;
		}
	}
	return Connector::command(argc, argv);
}
