/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * Copyright (C) 1998 by the University of Southern California
 * $Id: lanRouter.cc,v 1.10 2005/08/25 18:58:07 johnh Exp $
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
    "@(#) $Header: /usr/src/mash/repository/vint/ns-2/lanRouter.cc";
#endif

#include <tcl.h>
#include "lanRouter.h"
#include "address.h"
#include "ip.h"

static class LanRouterClass : public TclClass {
public:
	LanRouterClass() : TclClass("LanRouter") {}
	TclObject* create(int, const char*const*) {
		return (new LanRouter());
	}
} class_mac;

int LanRouter::next_hop(Packet *p) {
 	if (switch_ && switch_->classify(p)==1) {
		return -1;
 	}
	if (!routelogic_) return -1;

	hdr_ip* iph= hdr_ip::access(p);
	char* adst= Address::instance().print_nodeaddr(iph->daddr());
	int next_hopIP;

	if (enableHrouting_) {
		char* bdst;

		routelogic_->lookup_hier(lanaddr_, adst, next_hopIP);
		// hacking: get rid of the last "."
		bdst = Address::instance().print_nodeaddr(next_hopIP);
// 		bdst[strlen(bdst)-1] = '\0';
		Tcl &tcl = Tcl::instance();
		tcl.evalf("[Simulator instance] get-node-id-by-addr %s", bdst);
		sscanf(tcl.result(), "%d", &next_hopIP);
		delete [] bdst;
	} else {
		routelogic_->lookup_flat(lanaddr_, adst, next_hopIP);
	}
	delete [] adst;

	return next_hopIP;
}
int LanRouter::command(int argc, const char*const* argv)
{
	// Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		// cmd lanaddr <addr>
		if (strcmp(argv[1], "lanaddr") == 0) {
			strcpy(lanaddr_, argv[2]);
			return (TCL_OK);
		}
		// cmd routing hier|flat
		if (strcmp(argv[1], "routing") == 0) {
			if (strcmp(argv[2], "hier")==0)
				enableHrouting_= true;
			else if (strcmp(argv[2], "flat")==0)
				enableHrouting_= false;
			else return (TCL_ERROR);
			return (TCL_OK);
		}
		// cmd switch <switch>
		if (strcmp(argv[1], "switch") == 0) {
			switch_ = (Classifier*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
		// cmd routelogic <routelogic>
		if (strcmp(argv[1], "routelogic") == 0) {
			routelogic_ = (RouteLogic*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
	}
	return NsObject::command(argc, argv);
}






