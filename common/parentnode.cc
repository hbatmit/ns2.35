// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
//
// Time-stamp: <2000-09-15 12:52:53 haoboy>
//

/*
 * parentnode.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: parentnode.cc,v 1.3 2010/03/08 05:54:49 tom_henderson Exp $
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
// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
#include "parentnode.h"

class BcastAddressClassifier;

static class LanNodeClass : public TclClass {
public:
	LanNodeClass() : TclClass("LanNode") {}
	TclObject* create(int, const char*const*) {
                return (new LanNode);
        }
} class_lan_node;

static class AbsLanNodeClass : public TclClass {
public:
	AbsLanNodeClass() : TclClass("AbsLanNode") {}
	TclObject* create(int, const char*const*) {
                return (new AbsLanNode);
        }
} class_abslan_node;

static class BroadcastNodeClass : public TclClass {
public:
	BroadcastNodeClass() : TclClass("Node/Broadcast") {}
	TclObject* create(int, const char*const*) {
                return (new BroadcastNode);
        }
} class_broadcast_node;

int LanNode::command(int argc, const char*const* argv) {
  if (argc == 3) {
    if (strcmp(argv[1], "addr") == 0) {
      address_ = Address::instance().str2addr(argv[2]);
      return TCL_OK;
    } else if (strcmp(argv[1], "nodeid") == 0) {
      nodeid_ = atoi(argv[2]);
      return TCL_OK;
    }
  }
  return ParentNode::command(argc,argv);
}

int AbsLanNode::command(int argc, const char*const* argv) {
  if (argc == 3) {
    if (strcmp(argv[1], "addr") == 0) {
      address_ = Address::instance().str2addr(argv[2]);
      return TCL_OK;
    } else if (strcmp(argv[1], "nodeid") == 0) {
      nodeid_ = Address::instance().str2addr(argv[2]);
      return TCL_OK;
    }
  }
  return ParentNode::command(argc,argv);
}

int BroadcastNode::command(int argc, const char*const* argv) {
  Tcl& tcl = Tcl::instance();
  if (argc == 3) {
    if (strcmp(argv[1], "addr") == 0) {
      address_ = Address::instance().str2addr(argv[2]);
      return TCL_OK;
    } 
    else if (strcmp(argv[1], "nodeid") == 0) {
      nodeid_ = Address::instance().str2addr(argv[2]);
      return TCL_OK;
    } 
    else if (strcmp(argv[1], "attach-classifier") == 0) {
      classifier_ = (BcastAddressClassifier*)(TclObject::lookup(argv[2]));
      if (classifier_ == NULL) {
	tcl.add_errorf("Wrong object name %s",argv[2]);
	return TCL_ERROR;
      }
      return TCL_OK;
    }
  }
  return ParentNode::command(argc,argv);
}

void BroadcastNode::add_route(char *dst, NsObject *target) {
  classifier_->do_install(dst,target);
}

void BroadcastNode::delete_route(char *dst, NsObject *nullagent) {
  classifier_->do_install(dst, nullagent); 
}

void ParentNode::set_table_size(int) {}
void ParentNode::set_table_size(int, int) {}
