/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* Ported from CMU/Monarch's code, nov'98 -Padma.*/


/* rtable.h -*- c++ -*-
   $Id: rtable.h,v 1.3 1999/03/13 03:53:15 haoboy Exp $
   */
#ifndef cmu_rtable_h_
#define cmu_rtable_h_

#include "config.h"
#include "scheduler.h"
#include "queue.h"

#define BIG   250

#define NEW_ROUTE_SUCCESS_NEWENT       0
#define NEW_ROUTE_SUCCESS_OLDENT       1
#define NEW_ROUTE_METRIC_TOO_HIGH      2
#define NEW_ROUTE_ILLEGAL_CANCELLATION 3
#define NEW_ROUTE_INTERNAL_ERROR       4

#ifndef uint
typedef unsigned int uint;
#endif // !uint

/* NOTE: we depend on bzero setting the booleans to ``false''
   but if false != 0, so many things are screwed up, I don't
   know what to say... */

class rtable_ent {
public:
  rtable_ent() { bzero(this, sizeof(rtable_ent));}
  nsaddr_t     dst;     // destination
  nsaddr_t     hop;     // next hop
  uint         metric;  // distance

  uint         seqnum;  // last sequence number we saw
  double       advertise_ok_at; // when is it okay to advertise this rt?
  bool         advert_seqnum;  // should we advert rte b/c of new seqnum?
  bool         advert_metric;  // should we advert rte b/c of new metric?

  /*

  bool          needs_advertised; // must this rte go in out in an update?
  bool          needs_repeated_advert; // must this rte go out in all updates
 		 		      // until the next periodic update?
	 		 	      */
  Event        *trigger_event;

  uint          last_advertised_metric; // metric carried in our last advert
  double       changed_at; // when route last changed
  double       new_seqnum_at;	// when we last heard a new seq number
  double       wst;     // running wst info
  Event       *timeout_event; // event used to schedule timeout action
  PacketQueue *q;		//pkts queued for dst
};

// AddEntry adds an entry to the routing table with metric ent->metric+em.
//   You get to free the goods.
//
// GetEntry gets the entry for an address.

class RoutingTable {
  public:
    RoutingTable();
    void AddEntry(const rtable_ent &ent);
    int RemainingLoop();
    void InitLoop();
    rtable_ent *NextLoop();
    rtable_ent *GetEntry(nsaddr_t dest);

  private:
    rtable_ent *rtab;
    int         maxelts;
    int         elts;
    int         ctr;
};
    
#endif
