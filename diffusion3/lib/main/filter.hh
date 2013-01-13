// 
// filter.hh     : Filter definitions
// authors       : Fabio Silva
//
// Copyright (C) 2000-2003 by the University of Southern California
// $Id: filter.hh,v 1.9 2005/09/13 04:53:49 tomh Exp $
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
//
// Linking this file statically or dynamically with other modules is making
// a combined work based on this file.  Thus, the terms and conditions of
// the GNU General Public License cover the whole combination.
//
// In addition, as a special exception, the copyright holders of this file
// give you permission to combine this file with free software programs or
// libraries that are released under the GNU LGPL and with code included in
// the standard release of ns-2 under the Apache 2.0 license or under
// otherwise-compatible licenses with advertising requirements (or modified
// versions of such code, with unchanged license).  You may copy and
// distribute such a system following the terms of the GNU GPL for this
// file and the licenses of the other code concerned, provided that you
// include the source code of that other code when and as the GNU GPL
// requires distribution of source code.
//
// Note that people who make modified versions of this file are not
// obligated to grant this special exception for their modified versions;
// it is their choice whether to do so.  The GNU General Public License
// gives permission to release a modified version without this exception;
// this exception also makes it possible to release a modified version
// which carries forward this exception.

#ifndef _FILTER_HH_
#define _FILTER_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <sys/time.h>
#include <unistd.h>
#include <list>

#include "message.hh"

using namespace std;

// Filter priority definitions
#define FILTER_ZERO_PRIORITY 0
#define FILTER_MIN_PRIORITY 1
#define FILTER_MAX_PRIORITY 254
#define FILTER_KEEP_PRIORITY 255
#define FILTER_INVALID_PRIORITY 256

// The following definitions should help people pick a priority when
// writing filters.

// If a filter is a pre-processing filter (e.g. logging) which should
// receive messages before they get to a routing protocol, it should
// have a priority between FILTER_MAX_PRIORITY and
// PRE_PROCESSING_COMPLETED_PRIORITY.

#define PRE_PROCESSING_COMPLETED_PRIORITY 200

// Filters implementing routing protocols, should have a priority
// between PRE_PROCESSING_COMPLETED_PRIORITY and
// ROUTING_COMPLETED_PRIORITY.

#define ROUTING_COMPLETED_PRIORITY 50

// If a filter is a post-processing filter, it should use a priority
// between ROUTING_COMPLETED_PRIORITY and FILTER_MIN_PRIORITY.

class FilterEntry;
class FilterCallback;

typedef list<FilterEntry *> FilterList;
typedef long handle;

class FilterEntry {
public:
  NRAttrVec *filter_attrs_;
  int16_t handle_;
  u_int16_t priority_;
  u_int16_t agent_;
  FilterCallback *cb_;
  struct timeval tmv_;
  bool valid_;
   
  FilterEntry(int16_t handle, u_int16_t priority, u_int16_t agent) :
    handle_(handle), priority_(priority), agent_(agent)
  {
    valid_ = true;
    cb_ = NULL;
    GetTime(&tmv_);
  }

  ~FilterEntry()
  {
    if (filter_attrs_){
      ClearAttrs(filter_attrs_);
      delete filter_attrs_;
    }
  }
};

class FilterCallback {
public:
  virtual ~FilterCallback () {}
  virtual void recv(Message *msg, handle h) = 0;
};

#endif // !_FILTER_HH_
