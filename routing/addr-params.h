// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-

/*
 * addr-params.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: addr-params.h,v 1.4 2005/08/25 18:58:12 johnh Exp $
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
// $Header: /cvsroot/nsnam/ns-2/routing/addr-params.h,v 1.4 2005/08/25 18:58:12 johnh Exp $

#ifndef ns_addr_params_h
#define ns_addr_params_h

#include <tclcl.h>

class AddrParamsClass : public TclClass{
public:
	// XXX Default hlevel_ to 1.
	AddrParamsClass() : TclClass("AddrParams"),
			    hlevel_(1), NodeMask_(NULL), 
			    NodeShift_(NULL) {
		instance_ = this;
	}
	virtual TclObject* create(int, const char*const*) {
		Tcl::instance().add_errorf("Cannot create instance of "
					   "AddrParamsClass");
		return NULL;
	}
	virtual void bind();
	virtual int method(int ac, const char*const* av);

	static inline AddrParamsClass& instance() { return *instance_; }

	inline int mcast_shift() { return McastShift_; }
	inline int mcast_mask() { return McastMask_; }
	inline int port_shift() { return PortShift_; }
	inline int port_mask() { return PortMask_; }
	inline int node_shift(int n) { return NodeShift_[n]; }
	inline int node_mask(int n) { return NodeMask_[n]; }
	inline int hlevel() { return hlevel_; }
	inline int nodebits() { return nodebits_; }

private:
	int McastShift_;
	int McastMask_;
	int PortShift_;
	int PortMask_;
	int hlevel_;
	int *NodeMask_;
	int *NodeShift_;
	int nodebits_;
	static AddrParamsClass *instance_;
};

#endif // ns_addr_params_h
