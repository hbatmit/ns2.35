// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- 

/*
 * Copyright (C) 2000 by the University of Southern California
 * $Id: rtmodule.h,v 1.16 2010/03/08 05:54:53 tom_henderson Exp $
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
 * $Header: /cvsroot/nsnam/ns-2/routing/rtmodule.h,v 1.16 2010/03/08 05:54:53 tom_henderson Exp $
 *
 * Definition of RoutingModule, base class for all extensions to routing 
 * functionality in a Node. These modules are meant to be "plugin", and 
 * should be configured via node-config{} interface in tcl/lib/ns-lib.tcl.
 */

#ifndef ns_rtmodule_h
#define ns_rtmodule_h

#include <tclcl.h>
#include "addr-params.h"
#include "classifier.h"
#include "classifier-hash.h"
#include "classifier-hier.h"



class NsObject;
class Node;
class VirtualClassifier;
class DestHashClassifier;


class RoutingModule : public TclObject {
public:
	RoutingModule(); 
	/*
	 * Returns the node to which this module is attached.
	 */ 
	inline Node* node() { return n_; }
	/*
	 * Node-related module-specific initialization can be done here.
	 * However: (1) RoutingModule::attach() must be called from derived
	 * class so the value of n_ is setup, (2) module-specific 
	 * initialization that does not require knowledge of Node should 
	 * always stay in the module constructor.
	 *
	 * Return TCL_ERROR if initialization fails.
	 */
	virtual int attach(Node *n) { n_ = n; return TCL_OK; }
	virtual int command(int argc, const char*const* argv);
	virtual const char* module_name() const { return NULL; }

	/* support for populating rtg table */
	void route_notify(RoutingModule *rtm);
	void unreg_route_notify(RoutingModule *rtm);
	virtual void add_route(char *dst, NsObject *target); 
	virtual void delete_route(char *dst, NsObject *nullagent);
	void set_table_size(int nn);
	void set_table_size(int level, int csize);
	RoutingModule *next_rtm_;
	
protected:
	Node *n_;
	Classifier *classifier_;
};

class BaseRoutingModule : public RoutingModule {
public:
	BaseRoutingModule() : RoutingModule() {}
	virtual const char* module_name() const { return "Base"; }
	virtual int command(int argc, const char*const* argv);
protected:
	DestHashClassifier *classifier_;
};

class McastRoutingModule : public RoutingModule {
public:
	McastRoutingModule() : RoutingModule() {}
	virtual const char* module_name() const { return "Mcast"; }
	virtual int command(int argc, const char*const* argv);
protected:
	DestHashClassifier *classifier_;	
};

class HierRoutingModule : public RoutingModule {
public:
	HierRoutingModule() : RoutingModule() {}
	virtual const char* module_name() const { return "Hier"; }
	virtual int command(int argc, const char*const* argv);
protected:
	HierClassifier *classifier_;
};

class ManualRoutingModule : public RoutingModule {
public:
	ManualRoutingModule() : RoutingModule() {}
	virtual const char* module_name() const { return "Manual"; }
	virtual int command(int argc, const char*const* argv);
	void add_route(char *dst, NsObject *target);
protected:
	DestHashClassifier *classifier_;
};

class SourceRoutingModule : public RoutingModule {
public:
        SourceRoutingModule() : RoutingModule() {}
        virtual const char* module_name() const { return "Source"; }
	virtual int command(int argc, const char*const* argv);
};

class QSRoutingModule : public RoutingModule {
public:
        QSRoutingModule() : RoutingModule() {}
        virtual const char* module_name() const { return "QS"; }
		virtual int command(int argc, const char*const* argv);
};

class VcRoutingModule : public RoutingModule {
public:
	VcRoutingModule() : RoutingModule() {}
	virtual const char* module_name() const { return "VC"; }
	virtual int command(int argc, const char*const* argv);
	virtual void add_route(char *, NsObject *);
};



class PgmRoutingModule : public RoutingModule {
public:
        PgmRoutingModule() : RoutingModule() {}
        virtual const char* module_name() const { return "PGM"; }
};

class LmsRoutingModule : public RoutingModule {
public:
	LmsRoutingModule() : RoutingModule() {}
	virtual const char* module_name() const { return "LMS"; }
	// virtual int command(int argc, const char*const* argv);
	virtual void add_route(char *dst, NsObject *target);
};

#endif //  ns_rtmodule_h
