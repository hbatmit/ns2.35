/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1999 Regents of the University of California.
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
 *      This product includes software developed by the MASH Research
 *      Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
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
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/satellite/satroute.h,v 1.4 2005/05/19 03:19:02 tomh Exp $
 *
 * Contributed by Tom Henderson, UCB Daedalus Research Group, June 1999
 */

#ifndef ns_satroute_h_
#define ns_satroute_h_

#include <agent.h>
#include "route.h"
#include "node.h"

#define ROUTER_PORT      0xff
#define SAT_ROUTE_INFINITY 0x3fff

// Entry in the forwarding table
struct slot_entry {
	int next_hop;	
	NsObject* entry;
};

class SatNode;
//
//  Currently, this only implements centralized routing.  However, by 
//  following the examples in the mobility code, one could build on this
//  agent to make it a distributed routing agent
//
class SatRouteAgent : public Agent {
public:
  SatRouteAgent();
  ~SatRouteAgent();
  int command(int argc, const char * const * argv);

  // centralized routing
  void clear_slots();
  void install(int dst, int next_hop, NsObject* p);
  SatNode* node() { return node_; }
  int myaddr() {return myaddr_; }
  
protected:
  virtual void recv(Packet *, Handler *);
  void forwardPacket(Packet*);
  int myaddr_;           // My address-- set from OTcl

  // centralized routing stuff
  int maxslot_;
  int nslot_;
  slot_entry* slot_;	// Node's forwarding table 
  void alloc(int);	// Helper function
  SatNode* node_;
  
};

////////////////////////////////////////////////////////////////////////////

// A global route computation object/genie  
// This class performs operations very similar to what "Simulator instproc
// compute-routes" does at OTcl-level, except it performs them entirely
// in C++.  Single source shortest path routing is also supported.
class SatRouteObject : public RouteLogic {
public:
  SatRouteObject(); 
  static SatRouteObject& instance() {
	return (*instance_);            // general access to route object
  }
  void recompute();
  void recompute_node(int node);
  int command(int argc, const char * const * argv);        
  int data_driven_computation() { return data_driven_computation_; } 
  void insert_link(int src, int dst, double cost);
  void insert_link(int src, int dst, double cost, void* entry);
  int wiredRouting() { return wiredRouting_;}
//void hier_insert_link(int *src, int *dst, int cost);  // support hier-rtg?

protected:
  void compute_topology();
  void populate_routing_tables(int node = -1);
  int lookup(int src, int dst);
  void* lookup_entry(int src, int dst);
  void node_compute_routes(int node);
  void dump(); // for debugging only

  static SatRouteObject*  instance_;
  int metric_delay_;
  int suppress_initial_computation_;
  int data_driven_computation_;
  int wiredRouting_;
};

#endif
