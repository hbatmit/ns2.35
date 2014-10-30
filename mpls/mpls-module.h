// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- 

/*
 * Copyright (C) 2000 by the University of Southern California
 * $Id: mpls-module.h,v 1.3 2005/08/25 18:58:09 johnh Exp $
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
 * $Header: /cvsroot/nsnam/ns-2/mpls/mpls-module.h,v 1.3 2005/08/25 18:58:09 johnh Exp $
 *
 * Definition of MPLS node plugin module
 */

#ifndef ns_mpls_module_h
#define ns_mpls_module_h

#include <tclcl.h>

#include "lib/bsd-list.h"
#include "rtmodule.h"
#include "mpls/ldp.h"

class MPLSModule : public RoutingModule {
public:
	MPLSModule() : RoutingModule(), last_inlabel_(0), last_outlabel_(1000) {
		LIST_INIT(&ldplist_);
	}
	virtual ~MPLSModule();
	virtual const char* module_name() const { return "MPLS"; }
	virtual void add_route(char *dst, NsObject *target);
	virtual int command(int argc, const char*const* argv);
protected:
	// Do we have a LDP agent that peers with <nbr> ?
	LDPAgent* exist_ldp(int nbr);
	inline void attach_ldp(LDPAgent *a) {
		LDPListElem *e = new LDPListElem(a);
		LIST_INSERT_HEAD(&ldplist_, e, link);
	}
	void detach_ldp(LDPAgent *a);

	int last_inlabel_;
	int last_outlabel_;
	struct LDPListElem {
		LDPListElem(LDPAgent *a) : agt_(a) {
			link.le_next = NULL;
			link.le_prev = NULL;
		}
		LDPAgent* agt_;
		LIST_ENTRY(LDPListElem) link;
	};
	LIST_HEAD(LDPList, LDPListElem);
	LDPList ldplist_;
};

#endif // ns_mpls_module_h
