// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- 

/*
 * Copyright (C) 2000 by the University of Southern California
 * $Id: mpls-module.cc,v 1.4 2005/08/25 18:58:09 johnh Exp $
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

/*
 * $Header: /cvsroot/nsnam/ns-2/mpls/mpls-module.cc,v 1.4 2005/08/25 18:58:09 johnh Exp $
 *
 * MPLS node plugin module
 */

#include <tclcl.h>

#include "node.h"
#include "mpls/ldp.h"
#include "mpls/mpls-module.h"
#include "mpls/classifier-addr-mpls.h"

static class MPLSModuleClass : public TclClass {
public:
	MPLSModuleClass() : TclClass("RtModule/MPLS") {}
	TclObject* create(int, const char*const*) {
		return (new MPLSModule);
	}
} class_mpls_module;

MPLSModule::~MPLSModule() 
{
	// Delete LDP agent list
	LDPListElem *e;
	for (LDPListElem *p = ldplist_.lh_first; p != NULL; ) {
		e = p;
		p = p->link.le_next;
		delete e;
	}
}

void MPLSModule::detach_ldp(LDPAgent *a)
{
	for (LDPListElem *p = ldplist_.lh_first; p != NULL; 
	     p = p->link.le_next)
		if (p->agt_ == a) {
			LIST_REMOVE(p, link);
			delete p;
			break;
		}
}

LDPAgent* MPLSModule::exist_ldp(int nbr)
{
	for (LDPListElem *p = ldplist_.lh_first; p != NULL; 
	     p = p->link.le_next)
		if (p->agt_->peer() == nbr)
			return p->agt_;
	return NULL;
}

int MPLSModule::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "new-incoming-label") == 0) {
			tcl.resultf("%d", ++last_inlabel_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "new-outgoing-label") == 0) {
			tcl.resultf("%d", --last_outlabel_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "get-ldp-agents") == 0) {
			for (LDPListElem *e = ldplist_.lh_first; e != NULL;
			     e = e->link.le_next)
				tcl.resultf("%s %s", tcl.result(), 
					    e->agt_->name());
			return (TCL_OK);
		} else if (strcmp(argv[1], "trace-ldp") == 0) {
			for (LDPListElem *e = ldplist_.lh_first; e != NULL;
			     e = e->link.le_next)
				e->agt_->turn_on_trace();
			return (TCL_OK);
		} else if (strcmp(argv[1], "trace-msgtbl") == 0) {
			printf("%d : message-table\n", node()->nodeid());
			for (LDPListElem *e = ldplist_.lh_first; e != NULL;
			     e = e->link.le_next)
				e->agt_->MSGTdump();
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "attach-ldp") == 0) {
			LDPAgent *a = (LDPAgent*)TclObject::lookup(argv[2]);
			if (a == NULL) {
				fprintf(stderr,"Wrong object name %s",argv[2]);
				return (TCL_ERROR);
			}
			attach_ldp(a);
			return (TCL_OK);
		} else if (strcmp(argv[1], "detach-ldp") == 0) {
			LDPAgent *a = (LDPAgent*)TclObject::lookup(argv[2]);
			if (a == NULL) {
				fprintf(stderr,"Wrong object name %s",argv[2]);
				return (TCL_ERROR);
			}
			detach_ldp(a);
			return (TCL_OK);
		} else if (strcmp(argv[1], "exist-ldp-agent") == 0) {
			tcl.resultf("%d", 
				    exist_ldp(atoi(argv[2])) == NULL ? 0 : 1 );
			return (TCL_OK);
		} else if (strcmp(argv[1], "get-ldp-agent") == 0) {
			LDPAgent *a = exist_ldp(atoi(argv[2]));
			if (a == NULL)
				tcl.result("");
			else 
				tcl.resultf("%s", a->name());
			return (TCL_OK);
		}
		else if (strcmp(argv[1] , "route-notify") == 0) {
			Node *node = (Node *)(TclObject::lookup(argv[2]));
			if (node == NULL) {
				tcl.add_errorf("Invalid node object %s", argv[2]);
				return TCL_ERROR;
			}
			if (node != n_) {
				tcl.add_errorf("Node object %s different from n_", argv[2]);
				return TCL_ERROR;
			}
			n_->route_notify(this);
			return TCL_OK;
		}
		if (strcmp(argv[1] , "unreg-route-notify") == 0) {
			Node *node = (Node *)(TclObject::lookup(argv[2]));
			if (node == NULL) {
				tcl.add_errorf("Invalid node object %s", argv[2]);
				return TCL_ERROR;
			}
			if (node != n_) {
				tcl.add_errorf("Node object %s different from n_", argv[2]);
				return TCL_ERROR;
			}
			n_->unreg_route_notify(this);
			return TCL_OK;
		}
		else if (strcmp(argv[1], "attach-classifier") == 0) {
			classifier_ = (MPLSAddressClassifier*)(TclObject::lookup(argv[2]));
			if (classifier_ == NULL) {
				tcl.add_errorf("Wrong object name %s",argv[2]);
				return TCL_ERROR;
			}
			return TCL_OK;
		}
	}
	return RoutingModule::command(argc, argv);
}

void MPLSModule::add_route(char *dst, NsObject *target) {
	if (classifier_) 
		((MPLSAddressClassifier *)classifier_)->do_install(dst, target);
	if (next_rtm_ != NULL)
		next_rtm_->add_route(dst, target); 
}
