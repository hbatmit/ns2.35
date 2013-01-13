// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
//
// Time-stamp: <2000-09-15 12:53:32 haoboy>
//

/*
 * addr-params.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: addr-params.cc,v 1.3 2005/08/25 18:58:12 johnh Exp $
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
// Address parameters. Singleton class
//
// $Header: /cvsroot/nsnam/ns-2/routing/addr-params.cc,v 1.3 2005/08/25 18:58:12 johnh Exp $

#include <stdlib.h>
#include <tclcl.h>

#include "addr-params.h"

AddrParamsClass* AddrParamsClass::instance_ = NULL;

static AddrParamsClass addr_params_class;

void AddrParamsClass::bind()
{
	TclClass::bind();
	add_method("McastShift");
	add_method("McastMask");
	add_method("NodeShift");
	add_method("NodeMask");
	add_method("PortMask");
	add_method("PortShift");
	add_method("hlevel");
	add_method("nodebits");
}

int AddrParamsClass::method(int ac, const char*const* av)
{
	Tcl& tcl = Tcl::instance();
	int argc = ac - 2;
	const char*const* argv = av + 2;

	if (argc == 2) {
		if (strcmp(argv[1], "McastShift") == 0) {
			tcl.resultf("%d", McastShift_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "McastMask") == 0) {
			tcl.resultf("%d", McastMask_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "PortShift") == 0) {
			tcl.resultf("%d", PortShift_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "PortMask") == 0) {
			tcl.resultf("%d", PortMask_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "hlevel") == 0) {
			tcl.resultf("%d", hlevel_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "nodebits") == 0) {
			tcl.resultf("%d", nodebits_);
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "McastShift") == 0) {
			McastShift_ = atoi(argv[2]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "McastMask") == 0) {
			McastMask_ = atoi(argv[2]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "PortShift") == 0) {
			PortShift_ = atoi(argv[2]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "PortMask") == 0) {
			PortMask_ = atoi(argv[2]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "hlevel") == 0) {
			hlevel_ = atoi(argv[2]);
			if (NodeMask_ != NULL)
				delete []NodeMask_;
			if (NodeShift_ != NULL)
				delete []NodeShift_;
			NodeMask_ = new int[hlevel_];
			NodeShift_ = new int[hlevel_];
			return (TCL_OK);
		} else if (strcmp(argv[1], "nodebits") == 0) {
			nodebits_ = atoi(argv[2]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "NodeShift") == 0) {
			tcl.resultf("%d", node_shift(atoi(argv[2])-1));
			return (TCL_OK);
		} else if (strcmp(argv[1], "NodeMask") == 0) {
			tcl.resultf("%d", node_mask(atoi(argv[2])-1));
			return (TCL_OK);
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "NodeShift") == 0) {
			NodeShift_[atoi(argv[2])-1] = atoi(argv[3]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "NodeMask") == 0) {
			NodeMask_[atoi(argv[2])-1] = atoi(argv[3]);
			return (TCL_OK);
		}
	}
	return TclClass::method(ac, av);
}
