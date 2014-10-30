
/*
 * hash_table.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: hash_table.cc,v 1.5 2005/08/25 18:58:04 johnh Exp $
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


// Written by Chalermek Intanagonwiwat

#include "tclcl.h"
#include "diff_header.h"
#include "hash_table.h"
#include "diff_prob.h"


Pkt_Hash_Entry::~Pkt_Hash_Entry() {
    clear_fromagent(from_agent);
    if (timer != NULL)
      delete timer;
}


void Pkt_Hash_Entry::clear_fromagent(From_List *list)
{
  From_List *cur=list;
  From_List *temp = NULL;

  while (cur != NULL) {
    temp = FROM_NEXT(cur);
    delete cur;
    cur = temp;
  }
}

void Pkt_Hash_Table::reset()
{
  Pkt_Hash_Entry *hashPtr;
  Tcl_HashEntry *entryPtr;
  Tcl_HashSearch searchPtr;

  entryPtr = Tcl_FirstHashEntry(&htable, &searchPtr);
  while (entryPtr != NULL) {
    hashPtr = (Pkt_Hash_Entry *)Tcl_GetHashValue(entryPtr);
    delete hashPtr;
    Tcl_DeleteHashEntry(entryPtr);
    entryPtr = Tcl_NextHashEntry(&searchPtr);
  }
}

Pkt_Hash_Entry *Pkt_Hash_Table::GetHash(ns_addr_t sender_id, 
					unsigned int pk_num)
{
  unsigned int key[3];

  key[0] = sender_id.addr_;
  key[1] = sender_id.port_;
  key[2] = pk_num;

  Tcl_HashEntry *entryPtr = Tcl_FindHashEntry(&htable, (char *)key);

  if (entryPtr == NULL )
     return NULL;

  return (Pkt_Hash_Entry *)Tcl_GetHashValue(entryPtr);
}


void Pkt_Hash_Table::put_in_hash(hdr_cdiff *dfh)
{
    Tcl_HashEntry *entryPtr;
    Pkt_Hash_Entry    *hashPtr;
    unsigned int key[3];
    int newPtr;

    key[0]=(dfh->sender_id).addr_;
    key[1]=(dfh->sender_id).port_;
    key[2]=dfh->pk_num;

    entryPtr = Tcl_CreateHashEntry(&htable, (char *)key, &newPtr);
    if (!newPtr)
      return;

    hashPtr = new Pkt_Hash_Entry;
    hashPtr->forwarder_id = dfh->forward_agent_id;
    hashPtr->from_agent = NULL;
    hashPtr->is_forwarded = false;
    hashPtr->num_from = 0;
    hashPtr->timer = NULL;

    Tcl_SetHashValue(entryPtr, hashPtr);
}


void Data_Hash_Table::reset()
{
  Tcl_HashEntry *entryPtr;
  Tcl_HashSearch searchPtr;

  entryPtr = Tcl_FirstHashEntry(&htable, &searchPtr);
  while (entryPtr != NULL) {
    Tcl_DeleteHashEntry(entryPtr);
    entryPtr = Tcl_NextHashEntry(&searchPtr);
  }
}


Tcl_HashEntry *Data_Hash_Table::GetHash(int *attr)
{
  return Tcl_FindHashEntry(&htable, (char *)attr);
}


void Data_Hash_Table::PutInHash(int *attr)
{
    int newPtr;

    Tcl_CreateHashEntry(&htable, (char *)attr, &newPtr);
}





