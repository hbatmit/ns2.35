// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-

/*
 * classifier-hier.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: classifier-hier.cc,v 1.9 2010/03/08 05:54:49 tom_henderson Exp $
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

// $Header: /cvsroot/nsnam/ns-2/classifier/classifier-hier.cc,v 1.9 2010/03/08 05:54:49 tom_henderson Exp $

#include <assert.h>
#include "classifier-hier.h"
#include "route.h"


int HierClassifier::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "classifier") == 0) {
			// XXX Caller must guarantee that the supplied level
			// is within range, i.e., 0 < n <= level_
			tcl.resultf("%s", clsfr_[atoi(argv[2])-1]->name());
			return TCL_OK;
		} else if (strcmp(argv[1], "defaulttarget") == 0) {
			NsObject *o = (NsObject *)TclObject::lookup(argv[2]);
			for (int i = 0; 
			     i < AddrParamsClass::instance().hlevel(); i++)
				clsfr_[i]->set_default_target(o);
			return (TCL_OK);
		} else if (strcmp(argv[1], "clear") == 0) {
			int slot = atoi(argv[2]);
			for (int i = 0; 
			     i < AddrParamsClass::instance().hlevel(); i++)
				clsfr_[i]->clear(slot);
			return (TCL_OK);
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "add-classifier") == 0) {
			int n = atoi(argv[2]) - 1;
			Classifier *c=(Classifier*)TclObject::lookup(argv[3]);
			clsfr_[n] = c;
			return (TCL_OK);
		}
	}
	return Classifier::command(argc, argv);
}

void HierClassifier::do_install(char* dst, NsObject *target) {
	int istr[TINY_LEN], n=0, len=0;
	while(n < TINY_LEN) 
		istr[n++] = 0;
		
	RouteLogic::ns_strtok(dst, istr);
	
	while(istr[len] > 0) {
		istr[len] = istr[len] - 1;
		len++;
	}
	for (int i=1; i<len; i++) 
		clsfr_[i-1]->install(istr[i-1], clsfr_[i]);
	clsfr_[len-1]->install(istr[len-1], target);
}

void HierClassifier::set_table_size(int level, int size)
{
	if (clsfr_[level-1])
		clsfr_[level-1]->set_table_size(size);
}

		

static class HierClassifierClass : public TclClass {
public:
	HierClassifierClass() : TclClass("Classifier/Hier") {}
	TclObject* create(int , const char*const* ) {
		return (new HierClassifier());
	}
} class_hier_classifier;



