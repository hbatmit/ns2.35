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

/* dsdv.h -*- c++ -*-
   $Id: dsdv.h,v 1.6 1999/08/20 18:03:16 haoboy Exp $

   */

#ifndef cmu_dsdv_h_
#define cmu_dsdv_h_

#include "config.h"
#include "agent.h"
#include "ip.h"
#include "delay.h"
#include "scheduler.h"
#include "queue.h"
#include "trace.h"
#include "arp.h"
#include "ll.h"
#include "mac.h"
#include "priqueue.h"

#include "rtable.h"

#if defined(WIN32) && !defined(snprintf)
#define snprintf _snprintf
#endif /* WIN32 && !snprintf */

typedef double Time;

#define MAX_QUEUE_LENGTH 5
#define ROUTER_PORT      0xff

class DSDV_Helper;
class DSDVTriggerHandler;

class DSDV_Agent : public Agent {
  friend class DSDV_Helper;
  friend class DSDVTriggerHandler;
public:
  DSDV_Agent();
  virtual int command(int argc, const char * const * argv);
  void lost_link(Packet *p);
  
protected:
  void helper_callback(Event *e);
  Packet* rtable(int);
  virtual void recv(Packet *, Handler *);
  void trace(char* fmt, ...);
  void tracepkt(Packet *, double, int, const char *);
  void needTriggeredUpdate(rtable_ent *prte, Time t);
  // if no triggered update already pending for route prte, make one so
  void cancelTriggersBefore(Time t);
  // Cancel any triggered events scheduled to take place *before* time
  // t (exclusive)
  Packet * makeUpdate(int& periodic);
  // return a packet advertising the state in the routing table
  // makes a full ``periodic'' update if requested, or a ``triggered''
  // partial update if there are only a few changes and full update otherwise
  // returns with periodic = 1 if full update returned, or = 0 if partial
  // update returned
  void updateRoute(rtable_ent *old_rte, rtable_ent *new_rte);
  void processUpdate (Packet * p);
  void forwardPacket (Packet * p);
  void startUp();
  int diff_subnet(int dst);
  void sendOutBCastPkt(Packet *p);
  
  
  // update old_rte in routing table to to new_rte
  Trace *tracetarget;       // Trace Target
  DSDV_Helper  *helper_;    // DSDV Helper, handles callbacks
  DSDVTriggerHandler *trigger_handler;
  RoutingTable *table_;     // Routing Table
  PriQueue *ll_queue;       // link level output queue
  int seqno_;               // Sequence number to advertise with...
  int myaddr_;              // My address...
  
  // Extensions for mixed type simulations using wired and wireless
  // nodes
  char *subnet_;            // My subnet
  MobileNode *node_;        // My node
  // for debugging
  char *address;
  NsObject *port_dmux_;    // my port dmux

  Event *periodic_callback_;           // notify for periodic update
  
  // Randomness/MAC/logging parameters
  int be_random_;
  int use_mac_;
  int verbose_;
  int trace_wst_;
  
  // last time a periodic update was sent...
  double lasttup_;		// time of last triggered update
  double next_tup;		// time of next triggered update
  //  Event *trigupd_scheduled;	// event marking a scheduled triggered update
  
  // DSDV constants:
  double alpha_;  // 0.875
  double wst0_;   // 6 (secs)
  double perup_;  // 15 (secs)  period between updates
  int    min_update_periods_;    // 3 we must hear an update from a neighbor
  // every min_update_periods or we declare
  // them unreachable
  
  void output_rte(const char *prefix, rtable_ent *prte, DSDV_Agent *a);
  
};

class DSDV_Helper : public Handler {
  public:
    DSDV_Helper(DSDV_Agent *a_) { a = a_; }
    virtual void handle(Event *e) { a->helper_callback(e); }

  private:
    DSDV_Agent *a;
};

#endif
