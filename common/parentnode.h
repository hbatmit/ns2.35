// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
//
// Time-stamp: <2000-09-15 12:52:53 haoboy>
//

/*
 * parentnode.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: parentnode.h,v 1.5 2010/03/08 05:54:49 tom_henderson Exp $
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

#ifndef ns_parentnode_h
#define ns_parentnode_h

#include "address.h"
#include "classifier-addr.h"
#include "rtmodule.h"

class NsObject;

/* Class ParentNode : from which all node types including Node, LanNode etc evolve */

class ParentNode : public TclObject {
public:
	ParentNode() : nodeid_(-1), address_(-1) {} 
	/*virtual int command(int argc, const char*const* argv) {}*/
	virtual inline int address() { return address_;}
	virtual inline int nodeid() { return nodeid_;}
	virtual void add_route (char *, NsObject *) {}
	virtual void delete_route (char *, NsObject *) {}
	virtual void set_table_size(int nn);
	virtual void set_table_size(int lev, int nn);
protected:
  int nodeid_;
  int address_;
};


/* LanNode: Lan implementation as a virtual node: 
   LanNode mimics a real
   node and uses an address (id) from Node's address space */

class LanNode : public ParentNode {
public:
  LanNode() : ParentNode() {} 
  virtual int command(int argc, const char*const* argv);
};


/* AbsLanNode:
   It create an abstract LAN.
   An abstract lan is one in which the complex CSMA/CD 
   contention mechanism is replaced by a simple DropTail 
   queue mechanism. */

class AbsLanNode : public ParentNode {
public:
  AbsLanNode() : ParentNode() {} 
  virtual int command(int argc, const char*const* argv);
};


/* Node/Broadcast: used for sun's implementation of MIP;
   why is this version of MIP used when (CMU's) wireless 
   version is also available?? doesn't make sense to have both;
*/

class BroadcastNode : public ParentNode {
public:
	BroadcastNode() : ParentNode(), classifier_(NULL) {}
	virtual int command(int argc, const char*const* argv);
	virtual void add_route (char *dst, NsObject *target);
	virtual void delete_route (char *dst, NsObject *nullagent);
	//virtual void set_table_size(int nn) {}
	//virtual void set_table_size(int lev, int nn) {}
private:
  BcastAddressClassifier *classifier_;
};
  
#endif /* ---ns_lannode_h */

