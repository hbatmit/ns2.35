
/*
 * routing_table.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: routing_table.cc,v 1.5 2005/08/25 18:58:04 johnh Exp $
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

/********************************************************************/
/* routing_table.cc : Chalermek Intanagonwiwat (USC/ISI)  05/18/99  */
/********************************************************************/

// Important Note: Work still in progress !

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <float.h>

#include <tcl.h>
#include <stdlib.h>

#include "diff_header.h"
#include "agent.h"
#include "tclcl.h"
#include "ip.h"
#include "config.h"
#include "packet.h"
#include "trace.h"
#include "random.h"
#include "classifier.h"
#include "node.h"
#include "diffusion.h"
#include "diff_rate.h"
#include "iflist.h"
#include "hash_table.h"
#include "arp.h"
#include "mac.h"
#include "ll.h"
#include "dsr/path.h"
#include "god.h"
#include "routing_table.h"


void Diff_Routing_Entry::reset()
{
  counter = 0;
  num_active = 0;
  num_iif = 0;
  clear_outlist(active);
  clear_outlist(inactive);
  clear_inlist(iif);
  clear_inlist(down_iif);
  clear_agentlist(source);
  clear_agentlist(sink);
  active = NULL;
  inactive = NULL;
  iif = NULL;
  down_iif = NULL;
  source = NULL;
  sink = NULL;
  last_fwd_time = -2.0*INTEREST_PERIODIC;
  new_org_counter = 0;
}


void Diff_Routing_Entry::clear_outlist(Out_List *list)
{
  Out_List *cur=list;
  Out_List *temp = NULL;

  while (cur != NULL) {
    temp = OUT_NEXT(cur);
    delete cur;
    cur = temp;
  }
  
}


void Diff_Routing_Entry::clear_inlist(In_List *list)
{
  In_List *cur=list;
  In_List *temp = NULL;

  while (cur != NULL) {
    temp = IN_NEXT(cur);
    delete cur;
    cur = temp;
  }
}



void Diff_Routing_Entry::clear_agentlist(Agent_List *list)
{
  Agent_List *cur=list;
  Agent_List *temp = NULL;

  while (cur != NULL) {
    temp = AGENT_NEXT(cur);
    delete cur;
    cur = temp;
  }
}


int Diff_Routing_Entry::MostRecvOrg()
{
  In_List *cur;
  int     most = 0;

  for (cur=iif; cur!=NULL; cur = IN_NEXT(cur)) {
      most = max(most,NEW_ORG_RECV(cur));
  }
  return most;
}


bool Diff_Routing_Entry::ExistOriginalGradient()
{
  Out_List *cur_out;

  for (cur_out = active; cur_out!=NULL; cur_out=OUT_NEXT(cur_out)) {
    if (GRADIENT(cur_out) == ORIGINAL)
      return true;
  }

  return false;
}


void Diff_Routing_Entry::IncRecvCnt(ns_addr_t agent_addr)
{
  PrvCurPtr RetVal;

  counter++;

  RetVal=INTF_FIND(iif, agent_addr);
  if (RetVal.cur != NULL) {
     TOTAL_RECV(RetVal.cur)++;
     return;
  } 

  RetVal=INTF_FIND(source, agent_addr);
  if (RetVal.cur != NULL)
    return;

  // On-demand adding In_List 

  TOTAL_RECV(AddInList(agent_addr))++;

}


void Diff_Routing_Entry::CntPosSend(ns_addr_t agent_addr)
{
  PrvCurPtr RetVal;

  RetVal=INTF_FIND(iif, agent_addr);
  if (RetVal.cur != NULL) {
     NUM_POS_SEND(RetVal.cur)++;
     return;
  } 

  // On-demand adding In_List 

  In_List *cur_in = AddInList(agent_addr);
  NUM_POS_SEND(cur_in)++;
}


void Diff_Routing_Entry::CntNeg(ns_addr_t agent_addr)
{
  PrvCurPtr RetVal;

  RetVal=INTF_FIND(active, agent_addr);
  if (RetVal.cur != NULL) {
     NUM_NEG_RECV(RetVal.cur)++;
     return;
  } 
  /*
  perror("Hey man. How come you send me the negative reinforment?\n");
  exit(-1);
  */
}


void Diff_Routing_Entry::CntNewSub(ns_addr_t agent_addr)
{
  PrvCurPtr RetVal;

  RetVal=INTF_FIND(iif, agent_addr);
  if (RetVal.cur != NULL) {
     NEW_SUB_RECV(RetVal.cur)++;
     TOTAL_NEW_SUB_RECV(RetVal.cur)++;
     LAST_TS_NEW_SUB(RetVal.cur) = NOW;
     return;
  } 

  RetVal=INTF_FIND(source, agent_addr);
  if (RetVal.cur != NULL)
    return;

  // On-demand adding In_List 

  In_List *cur_in = AddInList(agent_addr);
  NEW_SUB_RECV(cur_in)++;
  TOTAL_NEW_SUB_RECV(cur_in)++;
  LAST_TS_NEW_SUB(cur_in) = NOW;
}


void Diff_Routing_Entry::ClrNewSub(ns_addr_t agent_addr)
{
  PrvCurPtr RetVal;

  RetVal=INTF_FIND(iif, agent_addr);
  if (RetVal.cur != NULL) {
     NEW_SUB_RECV(RetVal.cur)= 0;
     LAST_TS_NEW_SUB(RetVal.cur) = -1.0;
     return;
  } 
}


void Diff_Routing_Entry::CntNewOrg(ns_addr_t agent_addr)
{
  PrvCurPtr RetVal;

  RetVal=INTF_FIND(iif, agent_addr);
  if (RetVal.cur != NULL) {
     NEW_ORG_RECV(RetVal.cur)++;
     TOTAL_NEW_ORG_RECV(RetVal.cur)++;
     return;
  } 

  RetVal=INTF_FIND(source, agent_addr);
  if (RetVal.cur != NULL)
    return;

  // On-demand adding In_List 

  In_List *cur_in = AddInList(agent_addr);
  NEW_ORG_RECV(cur_in)++;
  TOTAL_NEW_ORG_RECV(cur_in)++;
}


void Diff_Routing_Entry::CntOldOrg(ns_addr_t agent_addr)
{
  PrvCurPtr RetVal;

  RetVal=INTF_FIND(iif, agent_addr);
  if (RetVal.cur != NULL) {
     OLD_ORG_RECV(RetVal.cur)++;
     TOTAL_OLD_ORG_RECV(RetVal.cur)++;
     return;
  } 

  RetVal=INTF_FIND(source, agent_addr);
  if (RetVal.cur != NULL)
    return;

  // On-demand adding In_List 

  In_List *cur_in = AddInList(agent_addr);
  OLD_ORG_RECV(cur_in)++;
  TOTAL_OLD_ORG_RECV(cur_in)++;
}


void Diff_Routing_Entry::ClrAllNewOrg()
{
  In_List *cur;

  for (cur = iif; cur!= NULL; cur = IN_NEXT(cur)) {
     NEW_ORG_RECV(cur)= 0;
  }
}


void Diff_Routing_Entry::ClrAllOldOrg()
{
  In_List *cur;

  for (cur=iif; cur!=NULL; cur = IN_NEXT(cur) ) {
     OLD_ORG_RECV(cur)= 0;
  }
}


In_List *Diff_Routing_Entry::MostRecentIn()
{
  In_List *cur, *ret;
  double recent_time;

  ret = NULL;
  recent_time = -1.0;
  for (cur = iif; cur!=NULL; cur=IN_NEXT(cur)) {
    if (LAST_TS_NEW_SUB(cur) > recent_time) {
      ret = cur;
      recent_time = LAST_TS_NEW_SUB(cur);
    }
  }
  return ret;
}


In_List * Diff_Routing_Entry::AddInList(ns_addr_t addr)
{
  In_List *inPtr= new In_List;

  AGT_ADDR(inPtr) = addr;
  INTF_INSERT(iif, inPtr); 

  num_iif++;
  return inPtr;
}


Diff_Routing_Entry:: Diff_Routing_Entry() 
{ 
    last_fwd_time = -2.0*INTEREST_PERIODIC;  
    counter      = 0;
    new_org_counter = 0;
    num_active   = 0;
    num_iif      = 0;
    active       = NULL; 
    inactive     = NULL; 
    iif          = NULL;
    down_iif     = NULL;
    source       = NULL;
    sink         = NULL;
}


