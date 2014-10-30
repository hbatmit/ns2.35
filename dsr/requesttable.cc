
/*
 * requesttable.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: requesttable.cc,v 1.4 2005/08/25 18:58:05 johnh Exp $
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
// Ported from CMU/Monarch's code, appropriate copyright applies.  

/* requesttable.h

   implement a table to keep track of the most current request
   number we've heard from a node in terms of that node's id

*/

#include "path.h"
#include "constants.h"
#include "requesttable.h"

RequestTable::RequestTable(int s): size(s)
{
  table = new Entry[size];
  size = size;
  ptr = 0;
}

RequestTable::~RequestTable()
{
  delete[] table;
}

int
RequestTable::find(const ID& net_id, const ID& MAC_id) const
{
  for (int c = 0 ; c < size ; c++)
	  if (table[c].net_id == net_id || table[c].MAC_id == MAC_id)
		  return c;
  return size;
}

int
RequestTable::get(const ID& id) const
{
  int existing_entry = find(id, id);

  if (existing_entry >= size)    
    {
      return 0;
    }
  return table[existing_entry].req_num;
}


Entry*
RequestTable::getEntry(const ID& id)
{
  int existing_entry = find(id, id);

  if (existing_entry >= size)    
    {
      table[ptr].MAC_id = ::invalid_addr;
      table[ptr].net_id = id;
      table[ptr].req_num = 0;
      table[ptr].last_arp = 0.0;
      table[ptr].rt_reqs_outstanding = 0;
      table[ptr].last_rt_req = -(rt_rq_period + 1.0);
      existing_entry = ptr;
      ptr = (ptr+1)%size;
    }
  return &(table[existing_entry]);
}

void
RequestTable::insert(const ID& net_id, int req_num)
{
  insert(net_id,::invalid_addr,req_num);
}


void
RequestTable::insert(const ID& net_id, const ID& MAC_id, int req_num)
{
  int existing_entry = find(net_id, MAC_id);

  if (existing_entry < size)
    {
      if (table[existing_entry].MAC_id == ::invalid_addr)
	table[existing_entry].MAC_id = MAC_id; // handle creations by getEntry
      table[existing_entry].req_num = req_num;
      return;
    }

  // otherwise add it in
  table[ptr].MAC_id = MAC_id;
  table[ptr].net_id = net_id;
  table[ptr].req_num = req_num;
  table[ptr].last_arp = 0.0;
  table[ptr].rt_reqs_outstanding = 0;
  table[ptr].last_rt_req = -(rt_rq_period + 1.0);
  ptr = (ptr+1)%size;
}
