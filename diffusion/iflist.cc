
/*
 * iflist.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: iflist.cc,v 1.4 2005/08/25 18:58:04 johnh Exp $
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

/***********************************************************/
/* iflist.cc : Chalermek Intanagonwiwat (USC/ISI) 06/25/99 */
/***********************************************************/


#include <stdio.h>
#include "config.h"
#include "random.h"
#include "iflist.h"


void Agent_List::InsertFront(Agent_List **start, Agent_List *cur)
{
  if (cur == NULL)
     return;

  cur->next = *start;
  *start = cur;
}


void Agent_List::Remove(Agent_List **prev, Agent_List *cur)
{
  if (cur == NULL)
    return;

  *prev = cur->next;  
}


void Agent_List::FreeAll(Agent_List **prev)
{
  Agent_List *temp;
  Agent_List *next;

  for (temp = *prev; temp != NULL; temp=next) {
    next = temp->next;
    delete temp;
  }

  *prev = NULL;
}


PrvCurPtr Agent_List::Find(Agent_List **start, ns_addr_t addr)
{
  PrvCurPtr RetVal;
  Agent_List **prv, *cur;

  for (prv= start, cur=*start; cur != NULL; prv= &(cur->next), cur=cur->next) {
    if ( NODE_ADDR(cur) == addr.addr_ && PORT(cur)==addr.port_) 
      break;
  }
 
  RetVal.prv = prv;
  RetVal.cur = cur;

  return RetVal;
}


// Copy list1's elements not already in *result, and insert in *result 

void Agent_List::Union(Agent_List **result, Agent_List *list1)
{
  PrvCurPtr RetVal;
  Agent_List *dupAgent;
  
  for (Agent_List *cur=list1; cur != NULL; cur=AGENT_NEXT(cur)) {
    RetVal = Find(result, AGT_ADDR(cur));
    if (RetVal.cur == NULL) {
      dupAgent = new Agent_List;
      *dupAgent = *cur;
      InsertFront(result, dupAgent);
    }
  }
}

void Agent_List::print()
{
  Agent_List *cur;

  for (cur=this; cur!=NULL; cur=cur->next) {
    printf("(%d,%d)\n", cur->agent_addr.addr_, cur->agent_addr.port_);
  }
}


In_List *In_List::FindMaxIn()
{
  In_List *cur_in, *max_in;
  int     max_diff;

  max_in = NULL;
  max_diff = 0;

  for (cur_in = this; cur_in != NULL;
       cur_in = IN_NEXT(cur_in) ) {
    if ( TOTAL_RECV(cur_in) - PREV_RECV(cur_in) > max_diff) {
      max_diff = TOTAL_RECV(cur_in) - PREV_RECV(cur_in);
      max_in = cur_in;
    }
  }

  return max_in;
}


Out_List *Out_List::WhereToGo()
{
  Out_List *cur_out;
  double slot = Random::uniform();

  for (cur_out = this; cur_out!=NULL; cur_out = OUT_NEXT(cur_out) ) {
    if (slot >= FROM_SLOT(cur_out) && slot < TO_SLOT(cur_out) )
      return cur_out;
  }

  if (cur_out == NULL && this != NULL)
    printf("Something must be wrong! \n");

  return cur_out;
}


void Out_List::CalRange()
{
  Out_List *cur_out=this;
  double cur_slot=0.0;

  for (cur_out = this; cur_out != NULL; cur_out = OUT_NEXT(cur_out) ) {

    if ( GRADIENT(cur_out) <= 0.0 ) {
      FROM_SLOT(cur_out) = -1.0;
      TO_SLOT(cur_out) = -1.0;
      continue;
    }

    FROM_SLOT(cur_out) = cur_slot;
    cur_slot = cur_slot + GRADIENT(cur_out);
    TO_SLOT(cur_out) = cur_slot;
  }   

  return;
}


void Out_List::NormalizeGradient()
{
  Out_List *cur;
  float sum=0.0;
  int   num=0; 

  for (cur=this; cur!=NULL; cur=OUT_NEXT(cur)) {
    sum = sum + GRADIENT(cur);
    num++;
  }

  if (num == 0) return;

  if (sum == 0.0) {
    float share= 1.0/num;

    for (cur=this; cur != NULL; cur = OUT_NEXT(cur)) {
      GRADIENT(cur) = share;
    }
    
    return;
  }

  for (cur=this; cur != NULL; cur = OUT_NEXT(cur)) {
    GRADIENT(cur) = GRADIENT(cur)/sum;
  }
}







