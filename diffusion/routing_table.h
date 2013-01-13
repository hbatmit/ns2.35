
/*
 * routing_table.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: routing_table.h,v 1.5 2005/08/25 18:58:04 johnh Exp $
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
/* routing_table.h : Chalermek Intanagonwiwat (USC/ISI)  08/16/99   */
/********************************************************************/

// Important Note: Work still in progress ! 

#ifndef ns_routing_table_h
#define ns_routing_table_h

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
#include "iflist.h"
#include "hash_table.h"
#include "arp.h"
#include "mac.h"
#include "ll.h"
#include "dsr/path.h"


// Routing Entry

class Diff_Routing_Entry {
 public:
  int      counter;
  int      num_active;
  int      num_iif;
  Out_List *active;                    // active and up oif   	       
  Out_List *inactive;                  // inactive and down oif 
  In_List  *iif;                       // active and up iif
  In_List  *down_iif;                  // inactive and down iif
  Agent_List *source;
  Agent_List *sink;

  double   last_fwd_time;              // the last time forwarding interest
                                       // For Diffusion/RateGradient
  int new_org_counter;                 // Across all incoming gradients.

  Diff_Routing_Entry();

  void reset();
  void clear_outlist(Out_List *);
  void clear_inlist(In_List *);
  void clear_agentlist(Agent_List *);
  int MostRecvOrg();
  bool ExistOriginalGradient();

  void IncRecvCnt(ns_addr_t);

  void CntPosSend(ns_addr_t);
  void CntNeg(ns_addr_t);
  void CntNewSub(ns_addr_t);
  void ClrNewSub(ns_addr_t);
  void CntNewOrg(ns_addr_t);
  void CntOldOrg(ns_addr_t);
  void ClrAllNewOrg();
  void ClrAllOldOrg();

  In_List *MostRecentIn();
  In_List *AddInList(ns_addr_t);
};

#endif

