
/*
 * hash_table.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: hash_table.h,v 1.4 2005/08/25 18:58:04 johnh Exp $
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

#ifndef ns_hash_table_h
#define ns_hash_table_h

#include "config.h"
#include "tclcl.h"
#include "diff_header.h"

//#include "diffusion.h"

#include "iflist.h"

class InterestTimer;

class Pkt_Hash_Entry {
 public:
  ns_addr_t    forwarder_id;
  bool        is_forwarded;
  bool        has_list;
  int         num_from;
  From_List  *from_agent;
  InterestTimer *timer;

  Pkt_Hash_Entry() { 
    forwarder_id.addr_ = 0;
    forwarder_id.port_ = 0;
    is_forwarded = false;
    has_list = false; 
    num_from=0; 
    from_agent=NULL; 
    timer=NULL;
  }

  ~Pkt_Hash_Entry();
  void clear_fromagent(From_List *);
};


class Pkt_Hash_Table {
 public:
  Tcl_HashTable htable;

  Pkt_Hash_Table() {
    Tcl_InitHashTable(&htable, 3);
  }

  void reset();
  void put_in_hash(hdr_cdiff *);
  Pkt_Hash_Entry *GetHash(ns_addr_t sender_id, unsigned int pkt_num);
};


class Data_Hash_Table {
 public:
  Tcl_HashTable htable;

  Data_Hash_Table() {
    Tcl_InitHashTable(&htable, MAX_ATTRIBUTE);
  }

  void reset();
  void PutInHash(int *attr);
  Tcl_HashEntry *GetHash(int *attr);
};

#endif
