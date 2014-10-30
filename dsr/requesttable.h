
/*
 * requesttable.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: requesttable.h,v 1.4 2005/08/25 18:58:05 johnh Exp $
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

// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
// Ported from CMU/Monarch's code, appropriate copyright applies.  

/* -*- c++ -*-
   requesttable.h

   implement a table to keep track of the most current request
   number we've heard from a node in terms of that node's id

   implemented as a circular buffer

*/

#ifndef _requesttable_h
#define _requesttable_h

#include "path.h"

struct Entry;

enum LastType { LIMIT0, UNLIMIT};

class RequestTable {
public:
  RequestTable(int size = 30);
  ~RequestTable();
  void insert(const ID& net_id, int req_num);
  void insert(const ID& net_id, const ID& MAC_id, int req_num);
  int get(const ID& id) const;
  // rtns 0 if id not found
  Entry* getEntry(const ID& id);  
private:
  Entry *table;
  int size;
  int ptr;
  int find(const ID& net_id, const ID& MAC_id ) const;
};

struct Entry {
  ID MAC_id;
  ID net_id;
  int req_num;
  Time last_arp;
  int rt_reqs_outstanding;
  Time last_rt_req;
  LastType last_type;
};

#endif //_requesttable_h
