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
 * Contributed by Tom Henderson, UCB Daedalus Research Group, June 1999
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/satellite/satroute.cc,v 1.13 2005/05/19 03:19:02 tomh Exp $";
#endif

#include "satroute.h"
#include "sattrace.h"
#include "satnode.h"
#include "satlink.h"
#include "route.h"
#include <address.h>

static class SatRouteClass:public TclClass
{
  public:
	SatRouteClass ():TclClass ("Agent/SatRoute") { }
	TclObject *create (int, const char *const *) {
    		return (new SatRouteAgent ());
	}
} class_satroute;

SatRouteAgent::SatRouteAgent (): Agent (PT_MESSAGE), maxslot_(0), nslot_(0), slot_(0)
{
	bind ("myaddr_", &myaddr_);
}

SatRouteAgent::~SatRouteAgent()
{
	if (slot_)
	    delete [] slot_;
}

void SatRouteAgent::alloc(int slot)
{
	slot_entry *old = slot_;
	int n = nslot_;
	if (old == 0)
		nslot_ = 32;
	while (nslot_ <= slot)
		nslot_ <<= 1;
	slot_ = new slot_entry[nslot_];
	memset(slot_, 0, nslot_ * sizeof(slot_entry));
	for (int i = 0; i < n; ++i) {
		slot_[i].next_hop = old[i].next_hop;
		slot_[i].entry = old[i].entry;
	}
	delete [] old;
}

void SatRouteAgent::install(int slot, int nh, NsObject* p)
{
	if (slot >= nslot_)
		alloc(slot);
	slot_[slot].next_hop = nh;
	slot_[slot].entry = p;
	if (slot >= maxslot_)
		maxslot_ = slot;
}

void SatRouteAgent::clear_slots()
{
	if (slot_)
		delete [] slot_;
	slot_ = 0;
	nslot_ = 0;
	maxslot_ = -1;
}

int SatRouteAgent::command (int argc, const char *const *argv)
{
        Tcl& tcl = Tcl::instance();
        if (argc == 2) {
        }
        if (argc == 3) {
               if (strcmp(argv[1], "set_node") == 0) {
                        node_ = (SatNode *) TclObject::lookup(argv[2]);
                        if (node_ == 0) {
                                tcl.resultf("no such object %s", argv[2]);
                                return (TCL_ERROR);
                        }
                        return (TCL_OK);
		}
	}
	return (Agent::command (argc, argv));
}

/*
 *  Find a target for the received packet
 */
void SatRouteAgent::forwardPacket(Packet * p)
{
	hdr_ip *iph = hdr_ip::access(p);
  	hdr_cmn *hdrc = HDR_CMN (p);
	NsObject *link_entry_;

	hdrc->direction() = hdr_cmn::DOWN; // send it down the stack
	int dst = Address::instance().get_nodeaddr(iph->daddr());
	// Here we need to have an accurate encoding of the next hop routing
	// information
	if (myaddr_ == iph->daddr()) {
		printf("Error:  trying to forward a packet destined to self: %d\n", myaddr_); 
		Packet::free(p);
	}
	hdrc->addr_type_ = NS_AF_INET;
	hdrc->last_hop_ = myaddr_; // for tracing purposes 
	if (SatRouteObject::instance().data_driven_computation())
		SatRouteObject::instance().recompute_node(myaddr_);
	if (SatNode::dist_routing_ == 0) {
		if (slot_ == 0) { // No routes to anywhere
			if (node_->trace())
				node_->trace()->traceonly(p);
			Packet::free(p);
			return;
		}
		link_entry_ = slot_[dst].entry;
		if (link_entry_ == 0) {
			if (node_->trace())
				node_->trace()->traceonly(p);
			Packet::free(p);
			return;
		}
		hdrc->next_hop_ = slot_[dst].next_hop;
		link_entry_->recv(p, (Handler *)0);
		return;
	} else {
		// DISTRIBUTED ROUTING LOOKUP COULD GO HERE
		printf("Error:  distributed routing not available\n");
		exit(1);
	}
}

void SatRouteAgent::recv (Packet * p, Handler *)
{
	hdr_ip *iph = hdr_ip::access(p);
	hdr_cmn *cmh = hdr_cmn::access(p);

	if (iph->saddr() == myaddr_ && cmh->num_forwards() == 0) {
	 	// Must be a packet I'm originating... add the IP header
		iph->ttl_ = IP_DEF_TTL;
	} else if (iph->saddr() == myaddr_) {
		// I received a packet that I sent.  Probably a routing loop.
		Packet::free(p);
		return;
	} else {
		// Packet I'm forwarding...
		// Check the TTL.  If it is zero, then discard.
		if(--iph->ttl_ == 0) {
			Packet::free(p);
			return;
		}
	}
	if ((iph->saddr() != myaddr_) && (iph->dport() == ROUTER_PORT)) {
		// DISTRIBUTED ROUTING PROTOCOL COULD GO HERE
		printf("Error:  distributed routing not available\n");
		exit(1);
	} else {
		forwardPacket(p);
	}
}

//###########################################################################

static class SatRouteObjectClass:public TclClass
{
  public:
        SatRouteObjectClass ():TclClass ("SatRouteObject") { }
        TclObject *create (int, const char *const *) {
                return (new SatRouteObject ());
        }
} class_satrouteobject;

SatRouteObject* SatRouteObject::instance_;

SatRouteObject::SatRouteObject() : suppress_initial_computation_(0) 
{
	bind_bool("wiredRouting_", &wiredRouting_);
	bind_bool("metric_delay_", &metric_delay_);
	bind_bool("data_driven_computation_", &data_driven_computation_);
}

int SatRouteObject::command (int argc, const char *const *argv)
{
        if (instance_ == 0)
                instance_ = this;
	if (argc == 2) {
		// While suppress_initial_computation_ may seem more 
		// appropriate as a bound variable, it is useful to 
		// implement the setting of this variable this way so that 
		// the ``instance_ = this'' assignment is made at the
		// start of simulation.
		if (strcmp(argv[1], "suppress_initial_computation") == 0) {
			suppress_initial_computation_ = 1;
			return (TCL_OK);
		}
		if (strcmp(argv[1], "compute_routes") == 0) {
			recompute();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "dump") == 0) {
			printf("Dumping\n");
			dump();
			return (TCL_OK);
		}
	}
	return (RouteLogic::command(argc, argv));
}                       

// Wrapper to catch whether OTcl-based (wired-satellite) routing is enabled
void SatRouteObject::insert_link(int src, int dst, double cost)
{
	if (wiredRouting_) {
		Tcl::instance().evalf("[Simulator instance] sat_link_up %d %d %f", (src - 1), (dst - 1), cost);
	} else
		insert(src, dst, cost);
}

// Wrapper to catch whether OTcl-based (wired) routing is enabled
void SatRouteObject::insert_link(int src, int dst, double cost, void* entry)
{
	SatLinkHead* slhp = (SatLinkHead*) entry;
	if (wiredRouting_) {
		// Here we do an upcall to an instproc in ns-sat.tcl
		// that populates the link_(:) array
		Tcl::instance().evalf("[Simulator instance] sat_link_up %d %d %f %s %s", (src - 1), (dst - 1), cost, slhp->name(), slhp->queue()->name());
	} else
		insert(src, dst, cost, entry); // base class insert()
}

void SatRouteObject::recompute_node(int node)
{
	compute_topology();
	node_compute_routes(node);
	populate_routing_tables(node);
}
void SatRouteObject::recompute()
{
	// For very large topologies (e.g., Teledesic), we don't want to
	// waste a lot of time computing routes at the beginning of the
	// simulation.  This first if() clause suppresses route computations.
	if (data_driven_computation_ ||
	    (NOW < 0.001 && suppress_initial_computation_) ) 
		return;
	else {
		compute_topology();
		if (wiredRouting_) {
			Tcl::instance().evalf("[Simulator instance] compute-flat-routes");
		} else {
			compute_routes(); // base class function
		}
		populate_routing_tables();
	}
}

// Derives link adjacency information from the nodes and gives the current
// topology information to the RouteLogic.
void SatRouteObject::compute_topology()
{
	Node *nodep;
	Phy *phytxp, *phyrxp, *phytxp2, *phyrxp2;
	SatLinkHead *slhp;
	Channel *channelp, *channelp2;
	int src, dst; 
	double delay;

	// wired-satellite integration
	if (wiredRouting_) {
		// There are two route objects being used
		// a SatRouteObject and a RouteLogic (for wired)
		// We need to also reset the RouteLogic one
		Tcl::instance().evalf("[[Simulator instance] get-routelogic] reset");
	}
	reset_all();
	// Compute adjacencies.  Traverse linked list of nodes 
        for (nodep=Node::nodehead_.lh_first; nodep; nodep = nodep->nextnode()) {
	    // Cycle through the linked list of linkheads
	    if (!SatNode::IsASatNode(nodep->address()))
	        continue;
	    for (slhp = (SatLinkHead*) nodep->linklisthead().lh_first; slhp; 
	      slhp = (SatLinkHead*) slhp->nextlinkhead()) {
		if (slhp->type() == LINK_GSL_REPEATER)
		    continue;
		if (!slhp->linkup_)
		    continue;
		phytxp = (Phy *) slhp->phy_tx();
		assert(phytxp);
		channelp = phytxp->channel();
		if (!channelp) 
	 	    continue; // Not currently connected to channel
		// Next, look for receive interfaces on this channel
		phyrxp = channelp->ifhead_.lh_first;
		for (; phyrxp; phyrxp = phyrxp->nextchnl()) {
		    if (phyrxp == phytxp) {
			printf("Configuration error:  a transmit interface \
			  is a channel target\n");
			exit(1);
		    } 
		    if (phyrxp->head()->type() == LINK_GSL_REPEATER) {
			double delay_firsthop = ((SatChannel*)
				    channelp)->get_pdelay(phytxp->node(), 
				    phyrxp->node());
			if (!((SatLinkHead*)phyrxp->head())->linkup_)
		    	    continue;
			phytxp2 = ((SatLinkHead*)phyrxp->head())->phy_tx();
			channelp2 = phytxp2->channel();
			if (!channelp2) 
	 	    	    continue; // Not currently connected to channel
			phyrxp2 = channelp2->ifhead_.lh_first;
			for (; phyrxp2; phyrxp2 = phyrxp2->nextchnl()) {
		    	    if (phyrxp2 == phytxp2) {
			        printf("Config error: a transmit interface \
			          is a channel target\n");
			        exit(1);
			    }
		            // Found an adjacency relationship.
		            // Add this link to the RouteLogic
		            src = phytxp->node()->address() + 1;
		            dst = phyrxp2->node()->address() + 1;
			    if (src == dst)
				continue;
			    if (metric_delay_)
		                delay = ((SatChannel*) 
			          channelp2)->get_pdelay(phytxp2->node(), 
			          phyrxp2->node());
			    else {
				delay = 1;
				delay_firsthop = 0;
			    }
			    insert_link(src, dst, delay+delay_firsthop, (void*)slhp);
			}
		    } else {
		        // Found an adjacency relationship.
		        // Add this link to the RouteLogic
		        src = phytxp->node()->address() + 1;
		        dst = phyrxp->node()->address() + 1;
			if (metric_delay_)
		            delay = ((SatChannel*) 
		              channelp)->get_pdelay(phytxp->node(), 
			      phyrxp->node());
			else
			    delay = 1;
			insert_link(src, dst, delay, (void*)slhp);
		    }
		}
	    }
	}
	//dump();
}

void SatRouteObject::populate_routing_tables(int node)
{
	SatNode *snodep = (SatNode*) Node::nodehead_.lh_first;
	SatNode *snodep2;
	int next_hop, src, dst;
	NsObject *target;

	if (wiredRouting_) {
		Tcl::instance().evalf("[Simulator instance] populate-flat-classifiers [Node set nn_]");
		return;
	}
        for (; snodep; snodep = (SatNode*) snodep->nextnode()) {
		if (!SatNode::IsASatNode(snodep->address()))
			continue;   
		// First, clear slots of the current routing table
		if (snodep->ragent())
			snodep->ragent()->clear_slots();
		src = snodep->address();
		if (node != -1 && node != src)
			continue;
		snodep2 = (SatNode*) Node::nodehead_.lh_first;
		for (; snodep2; snodep2 = (SatNode*) snodep2->nextnode()) {
                        if (!SatNode::IsASatNode(snodep->address()))
                                continue;
			dst = snodep2->address();
			next_hop = lookup(src, dst);
			if (next_hop != -1 && src != dst) {
				// Here need to insert target into slot table
				target = (NsObject*) lookup_entry(src, dst);
				if (target == 0) {
					printf("Error, routelogic target ");
					printf("not populated %f\n", NOW); 
					exit(1);
				}
				((SatNode*)snodep)->ragent()->install(dst, 
				    next_hop, target); 
			}
		}
	}
		
}

int SatRouteObject::lookup(int s, int d)
{                                       
	int src = s + 1;        
	int dst = d + 1;
	if (src >= size_ || dst >= size_) {
		return (-1); // Next hop = -1
	}
	return (route_[INDEX(src, dst, size_)].next_hop - 1);
}

void* SatRouteObject::lookup_entry(int s, int d)
{                       
	int src = s + 1;
	int dst = d + 1;
	if (src >= size_ || dst >= size_) {
		return (0); // Null pointer
	}
	return (route_[INDEX(src, dst, size_)].entry);
}

// This method is used for debugging only
void SatRouteObject::dump()
{
	int i, src, dst;
	for (i = 0; i < (size_ * size_); i++) {
		if (adj_[i].cost != SAT_ROUTE_INFINITY) {
			src = i / size_ - 1;
			dst = i % size_ - 1;
			printf("Found a link from %d to %d with cost %f\n", src, dst, adj_[i].cost);
		}
        }
}

void SatRouteObject::node_compute_routes(int node)
{
        int n = size_;
        int* parent = new int[n];
        double* hopcnt = new double[n];
#define ADJ(i, j) adj_[INDEX(i, j, size_)].cost
#define ADJ_ENTRY(i, j) adj_[INDEX(i, j, size_)].entry
#define ROUTE(i, j) route_[INDEX(i, j, size_)].next_hop
#define ROUTE_ENTRY(i, j) route_[INDEX(i, j, size_)].entry
        delete[] route_;
        route_ = new route_entry[n * n];
        memset((char *)route_, 0, n * n * sizeof(route_[0]));
        /* compute routes only for node "node" */
        int k = node + 1; // must add one to get the right offset in tables  
        int v;
        for (v = 0; v < n; v++) 
                parent[v] = v;

        /* set the route for all neighbours first */
        for (v = 1; v < n; ++v) {
                if (parent[v] != k) {
                        hopcnt[v] = ADJ(k, v);
                        if (hopcnt[v] != SAT_ROUTE_INFINITY) {
                                ROUTE(k, v) = v;
                                ROUTE_ENTRY(k, v) = ADJ_ENTRY(k, v);
                        }
                }
        }
        for (v = 1; v < n; ++v) {
                /*
                 * w is the node that is the nearest to the subtree
                 * that has been routed
                 */
                int o = 0;
                /* XXX */
                hopcnt[0] = SAT_ROUTE_INFINITY;
                int w;
                for (w = 1; w < n; w++)
                        if (parent[w] != k && hopcnt[w] < hopcnt[o])
                                o = w;
                parent[o] = k;
                /*
                 * update distance counts for the nodes that are
                 * adjacent to o
                 */
                if (o == 0)
                        continue;
                for (w = 1; w < n; w++) {
                        if (parent[w] != k &&
                            hopcnt[o] + ADJ(o, w) < hopcnt[w]) {
                                ROUTE(k, w) = ROUTE(k, o);
                                ROUTE_ENTRY(k, w) =
                                    ROUTE_ENTRY(k, o);
                                hopcnt[w] = hopcnt[o] + ADJ(o, w);
                        }
                }
        }
        /*
         * The route to yourself is yourself.
         */
        ROUTE(k, k) = k;
        ROUTE_ENTRY(k, k) = 0; // This should not matter

        delete[] hopcnt;
        delete[] parent;
}
