/*
 * Copyright (c) 2008, Marcello Caleffi, <marcello.caleffi@unina.it>,
 * http://wpage.unina.it/marcello.caleffi
 *
 * The AOMDV code has been developed at DIET, Department of Electronic
 * and Telecommunication Engineering, University of Naples "Federico II"
 *
 *
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



/*
 Copyright (c) 1997, 1998 Carnegie Mellon University.  All Rights
 Reserved. 
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 3. The name of the author may not be used to endorse or promote products
 derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 The AODV code developed by the CMU/MONARCH group was optimized and tuned by Samir Das and Mahesh Marina, University of Cincinnati. The work was partially done in Sun Microsystems.
 
 */



//#include <ip.h>

#include <aomdv/aomdv.h>
#include <aomdv/aomdv_packet.h>
#include <random.h>
#include <cmu-trace.h>
//#include <energy-model.h>

#define max(a,b)        ( (a) > (b) ? (a) : (b) )
#define CURRENT_TIME    Scheduler::instance().clock()

// AOMDV
#define DROP_RTR_RTEXPIRE               "REXP"
#define DROP_RTR_HELLO                  "HELO"

//#define DEBUG
//#define ERROR

#ifdef DEBUG
//static int extra_route_reply = 0;
//static int limit_route_request = 0;
static int route_request = 0;
#endif


/*
 TCL Hooks
 */


int hdr_aomdv::offset_;
static class AOMDVHeaderClass : public PacketHeaderClass {
public:
	AOMDVHeaderClass() : PacketHeaderClass("PacketHeader/AOMDV",
														sizeof(hdr_all_aomdv)) {
		bind_offset(&hdr_aomdv::offset_);
   } 
} class_rtProtoAOMDV_hdr;

static class AOMDVclass : public TclClass {
public:
	AOMDVclass() : TclClass("Agent/AOMDV") {}
	TclObject* create(int argc, const char*const* argv) {
		assert(argc == 5);
		//return (new AODV((nsaddr_t) atoi(argv[4])));
		return (new AOMDV((nsaddr_t) Address::instance().str2addr(argv[4])));
	}
} class_rtProtoAOMDV;


int
AOMDV::command(int argc, const char*const* argv) {
	if(argc == 2) {
		Tcl& tcl = Tcl::instance();
		
		if(strncasecmp(argv[1], "id", 2) == 0) {
			tcl.resultf("%d", index);
			return TCL_OK;
		}
		// AOMDV code - should it be removed?
		if (strncasecmp(argv[1], "dump-table", 10) == 0) {
			printf("Node %d: Route table:\n", index);
			rtable.rt_dumptable();
			return TCL_OK;
		}    
		if(strncasecmp(argv[1], "start", 2) == 0) {
			btimer.handle((Event*) 0);
			
#ifndef AOMDV_LINK_LAYER_DETECTION
			htimer.handle((Event*) 0);
			ntimer.handle((Event*) 0);
#endif // LINK LAYER DETECTION
			
			rtimer.handle((Event*) 0);
			return TCL_OK;
		}
	}
	else if(argc == 3) {
		if(strcmp(argv[1], "index") == 0) {
			index = atoi(argv[2]);
			return TCL_OK;
		}
		
		else if(strcmp(argv[1], "log-target") == 0 || strcmp(argv[1], "tracetarget") == 0) {
			logtarget = (Trace*) TclObject::lookup(argv[2]);
			if(logtarget == 0)
				return TCL_ERROR;
			return TCL_OK;
		}
		else if(strcmp(argv[1], "drop-target") == 0) {
			int stat = rqueue.command(argc,argv);
			if (stat != TCL_OK) return stat;
			return Agent::command(argc, argv);
		}
		else if(strcmp(argv[1], "if-queue") == 0) {
			AOMDVifqueue = (PriQueue*) TclObject::lookup(argv[2]);
			
			if(AOMDVifqueue == 0)
				return TCL_ERROR;
			return TCL_OK;
		}
		// AODV ns-2.31 code
		else if (strcmp(argv[1], "port-dmux") == 0) {
			dmux_ = (PortClassifier *)TclObject::lookup(argv[2]);
			if (dmux_ == 0) {
				fprintf (stderr, "%s: %s lookup of %s failed\n", __FILE__,
							argv[1], argv[2]);
				return TCL_ERROR;
			}
			return TCL_OK;
		}
	}
	return Agent::command(argc, argv);
}

/* 
Constructor
 */

AOMDV::AOMDV(nsaddr_t id) : Agent(PT_AOMDV),
btimer(this), htimer(this), ntimer(this), 
rtimer(this), lrtimer(this), rqueue() {
	
	// AOMDV code
	aomdv_max_paths_ = 3;
	bind("aomdv_max_paths_", &aomdv_max_paths_);
	
	aomdv_prim_alt_path_len_diff_ = 1;
	bind("aomdv_prim_alt_path_len_diff_", &aomdv_prim_alt_path_len_diff_);
	
	index = id;
	seqno = 2;
	bid = 1;
	
	LIST_INIT(&nbhead);
	LIST_INIT(&bihead);
	
	logtarget = 0;
	AOMDVifqueue = 0;
}

/*
 Timers
 */

void
AOMDVBroadcastTimer::handle(Event*) {
	agent->id_purge();
	Scheduler::instance().schedule(this, &intr, BCAST_ID_SAVE);
}

void
AOMDVHelloTimer::handle(Event*) {
	// AOMDV code - could it be removed?
	// agent->sendHello();
	/* Do not send a HELLO message unless we have a valid route entry. */
	if (agent->rtable.rt_has_active_route())
		agent->sendHello();
	// AODV ns-2.31 code
	// double interval = HELLO_INTERVAL + 0.01*Random::uniform();
	double interval = MinHelloInterval + ((MaxHelloInterval - MinHelloInterval) * Random::uniform());
	assert(interval >= 0);
	Scheduler::instance().schedule(this, &intr, interval);
}

void
AOMDVNeighborTimer::handle(Event*) {
	agent->nb_purge();
	Scheduler::instance().schedule(this, &intr, HELLO_INTERVAL);
}

void
AOMDVRouteCacheTimer::handle(Event*) {
	agent->rt_purge();	
#define FREQUENCY 0.5 // sec
	Scheduler::instance().schedule(this, &intr, FREQUENCY);
}

void
AOMDVLocalRepairTimer::handle(Event* p)  {  // SRD: 5/4/99
	aomdv_rt_entry *rt;
	struct hdr_ip *ih = HDR_IP( (Packet *)p);
	
	/* you get here after the timeout in a local repair attempt */
	/* fprintf(stderr, "%s\n", __FUNCTION__); */
	
	
	rt = agent->rtable.rt_lookup(ih->daddr());
	
	if (rt && rt->rt_flags != RTF_UP) {
		// route is yet to be repaired
		// I will be conservative and bring down the route
		// and send route errors upstream.
		/* The following assert fails, not sure why */
		/* assert (rt->rt_flags == RTF_IN_REPAIR); */
		
		//rt->rt_seqno++;
		agent->rt_down(rt);
		// send RERR
#ifdef DEBUG
//		fprintf(stderr,"Node %d: Dst - %d, failed local repair\n",index, rt->rt_dst);
#endif      
	}
	Packet::free((Packet *)p);
}


/*
 Broadcast ID Management  Functions
 */


// AODV ns-2.31 code
void
AOMDV::id_insert(nsaddr_t id, u_int32_t bid) {
	AOMDVBroadcastID *b = new AOMDVBroadcastID(id, bid);
	
	assert(b);
	b->expire = CURRENT_TIME + BCAST_ID_SAVE;
	LIST_INSERT_HEAD(&bihead, b, link);
}

// AODV ns-2.31 code
/* SRD */
bool
AOMDV::id_lookup(nsaddr_t id, u_int32_t bid) {
	AOMDVBroadcastID *b = bihead.lh_first;
	
	// Search the list for a match of source and bid
	for( ; b; b = b->link.le_next) {
		if ((b->src == id) && (b->id == bid))
			return true;     
	}
	return false;
}

// AOMDV ns-2.31 code
AOMDVBroadcastID*
AOMDV::id_get(nsaddr_t id, u_int32_t bid) {
	AOMDVBroadcastID *b = bihead.lh_first;
	
	// Search the list for a match of source and bid
	for( ; b; b = b->link.le_next) {
		if ((b->src == id) && (b->id == bid))
			return b;     
	}
	return NULL;
}

void
AOMDV::id_purge() {
	AOMDVBroadcastID *b = bihead.lh_first;
	AOMDVBroadcastID *bn;
	double now = CURRENT_TIME;
	
	for(; b; b = bn) {
		bn = b->link.le_next;
		if(b->expire <= now) {
			LIST_REMOVE(b,link);
			delete b;
		}
	}
}

/*
 Helper Functions
 */

double
AOMDV::PerHopTime(aomdv_rt_entry *rt) {
	int num_non_zero = 0, i;
	double total_latency = 0.0;
	
	if (!rt)
		return ((double) NODE_TRAVERSAL_TIME );
	
	for (i=0; i < MAX_HISTORY; i++) {
		if (rt->rt_disc_latency[i] > 0.0) {
			num_non_zero++;
			total_latency += rt->rt_disc_latency[i];
		}
	}
	if (num_non_zero > 0)
		return(total_latency / (double) num_non_zero);
	else
		return((double) NODE_TRAVERSAL_TIME);
	
}

/*
 Link Failure Management Functions
 */

static void
aomdv_rt_failed_callback(Packet *p, void *arg) {
	((AOMDV*) arg)->rt_ll_failed(p);
}

/*
 * This routine is invoked when the link-layer reports a route failed.
 */
void
AOMDV::rt_ll_failed(Packet *p) {
// AOMDV ns-2.31 code
#ifndef AOMDV_LINK_LAYER_DETECTION
	drop(p, DROP_RTR_MAC_CALLBACK);
#else 
	
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	aomdv_rt_entry *rt;
	nsaddr_t broken_nbr = ch->next_hop_;
	
	/*
	 * Non-data packets and Broadcast Packets can be dropped.
	 */
	if(! DATA_PACKET(ch->ptype()) ||
		(u_int32_t) ih->daddr() == IP_BROADCAST) {
		drop(p, DROP_RTR_MAC_CALLBACK);
		return;
	}
	log_link_broke(p);
	if((rt = rtable.rt_lookup(ih->daddr())) == 0) {
		drop(p, DROP_RTR_MAC_CALLBACK);
		return;
	}
	log_link_del(ch->next_hop_);
	
#ifdef AOMDV_LOCAL_REPAIR
	/* if the broken link is closer to the dest than source, 
		attempt a local repair. Otherwise, bring down the route. */
	
	
	if (ch->num_forwards() > rt->rt_hops) {
		local_rt_repair(rt, p); // local repair
										// retrieve all the packets in the ifq using this link,
										// queue the packets for which local repair is done, 
		return;
	} 
#endif // LOCAL REPAIR  
	
	{
		// AOMDV code - could it be removed?
		handle_link_failure(broken_nbr);
		// AOMDV code
#ifdef AOMDV_PACKET_SALVAGING
		
		if ( !DATA_PACKET(ch->ptype()) ) 
			drop(p, DROP_RTR_MAC_CALLBACK);
		else {
			// salvage the packet using an alternate path if available.
			aomdv_rt_entry *rt = rtable.rt_lookup(ih->daddr());
			if ( rt && (rt->rt_flags == RTF_UP) && (ch->aomdv_salvage_count_ < AOMDV_MAX_SALVAGE_COUNT) ) {
				ch->aomdv_salvage_count_ += 1;
				forward(rt, p, NO_AOMDV_DELAY);
			}
			else drop(p, DROP_RTR_MAC_CALLBACK);
		}
		while((p = AOMDVifqueue->filter(broken_nbr))) {
			struct hdr_cmn *ch = HDR_CMN(p);
			struct hdr_ip *ih = HDR_IP(p);
			if ( !DATA_PACKET(ch->ptype()) ) 
				drop(p, DROP_RTR_MAC_CALLBACK);
			else {
				// salvage the packet using an alternate path if available.
				aomdv_rt_entry *rt = rtable.rt_lookup(ih->daddr());
				if ( rt && (rt->rt_flags == RTF_UP) && (ch->aomdv_salvage_count_ < AOMDV_MAX_SALVAGE_COUNT) ) {
					ch->aomdv_salvage_count_ += 1;
					forward(rt, p, NO_AOMDV_DELAY);
				}
				else drop(p, DROP_RTR_MAC_CALLBACK);
			}
		} 
#else // NO PACKET SALVAGING
		drop(p, DROP_RTR_MAC_CALLBACK);
		// Do the same thing for other packets in the interface queue using the
		// broken link -Mahesh
		while((p = AOMDVifqueue->filter(broken_nbr))) {
			drop(p, DROP_RTR_MAC_CALLBACK);
		} 
		nb_delete(broken_nbr);
		// AOMDV code
#endif // NO PACKET SALVAGING        
	}
	
#endif // LINK LAYER DETECTION
}

// AOMDV code
void
AOMDV::handle_link_failure(nsaddr_t id) {
	bool error=true;
	aomdv_rt_entry *rt, *rtn;
	Packet *rerr = Packet::alloc();
	struct hdr_aomdv_error *re = HDR_AOMDV_ERROR(rerr);
#ifdef DEBUG
	fprintf(stderr, "%s: multipath version\n", __FUNCTION__);
#endif // DEBUG
	re->DestCount = 0;
	for(rt = rtable.head(); rt; rt = rtn) {  // for each rt entry
		AOMDV_Path* path;
		rtn = rt->rt_link.le_next; 
		if ((rt->rt_flags == RTF_UP) && (path=rt->path_lookup(id)) ) {
			assert((rt->rt_seqno%2) == 0);
			
			rt->path_delete(id);
			if (rt->path_empty()) {
				rt->rt_seqno++;
				rt->rt_seqno = max(rt->rt_seqno, rt->rt_highest_seqno_heard);
				// CHANGE
				if (rt->rt_error) {
					re->unreachable_dst[re->DestCount] = rt->rt_dst;
					re->unreachable_dst_seqno[re->DestCount] = rt->rt_seqno;
#ifdef DEBUG
					fprintf(stderr, "%s(%f): %d\t(%d\t%u\t%d)\n", __FUNCTION__, CURRENT_TIME,
							  index, re->unreachable_dst[re->DestCount],
							  re->unreachable_dst_seqno[re->DestCount], id);
#endif // DEBUG
					re->DestCount += 1;
					rt->rt_error = false;
				}
				// CHANGE
				rt_down(rt);
			}
		}
	}   
	
	if ( (re->DestCount > 0) && (error) ) {
#ifdef DEBUG
		fprintf(stdout, "%s(%f): %d\tsending RERR...\n", __FUNCTION__, CURRENT_TIME, index);
#endif // DEBUG
		sendError(rerr, false);
	}
	else {
		Packet::free(rerr);
	}
}

void
AOMDV::local_rt_repair(aomdv_rt_entry *rt, Packet *p) {
#ifdef DEBUG
	fprintf(stderr,"%s: Dst - %d\n", __FUNCTION__, rt->rt_dst); 
#endif  
	// Buffer the packet 
	rqueue.enque(p);
	
	// mark the route as under repair 
	rt->rt_flags = RTF_IN_REPAIR;
	
	sendRequest(rt->rt_dst);
	
	// set up a timer interrupt
	Scheduler::instance().schedule(&lrtimer, p->copy(), rt->rt_req_timeout);
}

void
AOMDV::rt_down(aomdv_rt_entry *rt) {
	/*
	 *  Make sure that you don't "down" a route more than once.
	 */
	
	// AOMDV code
#ifdef DEBUG
	fprintf(stderr, "%s: multipath version\n", __FUNCTION__);
#endif // DEBUG
	
	if(rt->rt_flags == RTF_DOWN) {
		return;
	}
	
	// AODV ns-2.31 code
	// assert (rt->rt_seqno%2); // is the seqno odd?
	// AOMDV code
	rt->rt_flags = RTF_DOWN;
	rt->rt_advertised_hops = INFINITY;
	rt->path_delete();
	rt->rt_expire = 0;
	
} /* rt_down function */

/*
 Route Handling Functions
 */

void
AOMDV::rt_resolve(Packet *p) {
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	aomdv_rt_entry *rt;
	
	/*
	 *  Set the transmit failure callback.  That
	 *  won't change.
	 */
	ch->xmit_failure_ = aomdv_rt_failed_callback;
	ch->xmit_failure_data_ = (void*) this;
   rt = rtable.rt_lookup(ih->daddr());
	if(rt == 0) {
		rt = rtable.rt_add(ih->daddr());
	}
	
	/*
	 * If the route is up, forward the packet 
	 */
   
	if(rt->rt_flags == RTF_UP) {
		assert(rt->rt_hops != INFINITY2);
		forward(rt, p, NO_AOMDV_DELAY);
	}
	/*
	 *  if I am the source of the packet, then do a Route Request.
	 */
   else if(ih->saddr() == index) {
		rqueue.enque(p);
		sendRequest(rt->rt_dst);
	}
	/*
	 *   A local repair is in progress. Buffer the packet. 
	 */
	else if (rt->rt_flags == RTF_IN_REPAIR) {
		rqueue.enque(p);
	}
	
	/*
	 * I am trying to forward a packet for someone else to which
	 * I don't have a route.
	 */
	else {
		Packet *rerr = Packet::alloc();
		struct hdr_aomdv_error *re = HDR_AOMDV_ERROR(rerr);
		/* 
			* For now, drop the packet and send error upstream.
		 * Now the route errors are broadcast to upstream
		 * neighbors - Mahesh 09/11/99
		 */  
		
		assert (rt->rt_flags == RTF_DOWN);
		re->DestCount = 0;
		re->unreachable_dst[re->DestCount] = rt->rt_dst;
		re->unreachable_dst_seqno[re->DestCount] = rt->rt_seqno;
		re->DestCount += 1;
#ifdef DEBUG
		fprintf(stderr, "%s: sending RERR...\n", __FUNCTION__);
#endif
		sendError(rerr, false);
		
		drop(p, DROP_RTR_NO_ROUTE);
	}
	
}

void
AOMDV::rt_purge() {
	aomdv_rt_entry *rt, *rtn;
	double now = CURRENT_TIME;
	double delay = 0.0;
	Packet *p;
	
	for(rt = rtable.head(); rt; rt = rtn) {  // for each rt entry
		rtn = rt->rt_link.le_next;
		// AOMDV code
		// MODIFIED BY US! Added '&& rt-> ...' in if-statement
		if (rt->rt_flags == RTF_UP && (rt->rt_expire < now)) {
			rt->path_purge();
			if (rt->path_empty()) {
				while((p = rqueue.deque(rt->rt_dst))) {
               drop(p, DROP_RTR_RTEXPIRE);
				}
				rt->rt_seqno++;
				rt->rt_seqno = max(rt->rt_seqno, rt->rt_highest_seqno_heard);
				if (rt->rt_seqno%2 == 0) rt->rt_seqno += 1;
				// assert (rt->rt_seqno%2);
				rt_down(rt);
			}
		}
		else if (rt->rt_flags == RTF_UP) {
			// If the route is not expired,
			// and there are packets in the sendbuffer waiting,
			// forward them. This should not be needed, but this extra 
			// check does no harm.
			assert(rt->rt_hops != INFINITY2);
			while((p = rqueue.deque(rt->rt_dst))) {
				forward (rt, p, delay);
				delay += ARP_DELAY;
			}
		} 
		else if (rqueue.find(rt->rt_dst))
			// If the route is down and 
			// if there is a packet for this destination waiting in
			// the sendbuffer, then send out route request. sendRequest
			// will check whether it is time to really send out request
			// or not.
			// This may not be crucial to do it here, as each generated 
			// packet will do a sendRequest anyway.
			
			sendRequest(rt->rt_dst); 
   }
	
}

/*
 Packet Reception Routines
 */

void
AOMDV::recv(Packet *p, Handler*) {
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	
	assert(initialized());
	//assert(p->incoming == 0);
	// XXXXX NOTE: use of incoming flag has been depracated; In order to track direction of pkt flow, direction_ in hdr_cmn is used instead. see packet.h for details.
	
	if(ch->ptype() == PT_AOMDV) {
		ih->ttl_ -= 1;
		recvAOMDV(p);
		return;
	}
	
	
	/*
	 *  Must be a packet I'm originating...
	 */
	if((ih->saddr() == index) && (ch->num_forwards() == 0)) {
		/*
		 * Add the IP Header
		 */
		ch->size() += IP_HDR_LEN;
		// AOMDV code
		ch->aomdv_salvage_count_ = 0;
		// Added by Parag Dadhania && John Novatnack to handle broadcasting
		if ( (u_int32_t)ih->daddr() != IP_BROADCAST)
			ih->ttl_ = NETWORK_DIAMETER;
	}
	/*
	 *  I received a packet that I sent.  Probably
	 *  a routing loop.
	 */
	else if(ih->saddr() == index) {
		drop(p, DROP_RTR_ROUTE_LOOP);
		return;
	}
	/*
	 *  Packet I'm forwarding...
	 */
	else {
		/*
		 *  Check the TTL.  If it is zero, then discard.
		 */
		if(--ih->ttl_ == 0) {
			drop(p, DROP_RTR_TTL);
			return;
		}
	}
	// Added by Parag Dadhania && John Novatnack to handle broadcasting
	if ( (u_int32_t)ih->daddr() != IP_BROADCAST)
		rt_resolve(p);
	else
		forward((aomdv_rt_entry*) 0, p, NO_AOMDV_DELAY);
}


void
AOMDV::recvAOMDV(Packet *p) {
	struct hdr_aomdv *ah = HDR_AOMDV(p);
	// AODV ns-2.31 code
	// struct hdr_ip *ih = HDR_IP(p);
	assert(HDR_IP (p)->sport() == RT_PORT);
	assert(HDR_IP (p)->dport() == RT_PORT);
	
	/*
	 * Incoming Packets.
	 */
	switch(ah->ah_type) {
		
		case AOMDVTYPE_RREQ:
			recvRequest(p);
			break;
			
		case AOMDVTYPE_RREP:
			recvReply(p);
			break;
			
		case AOMDVTYPE_RERR:
			recvError(p);
			break;
			
		case AOMDVTYPE_HELLO:
			recvHello(p);
			break;
			
		default:
			fprintf(stderr, "Invalid AOMDV type (%x)\n", ah->ah_type);
			exit(1);
	}
	
}


// AOMDV
void
AOMDV::recvRequest(Packet *p) {
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_aomdv_request *rq = HDR_AOMDV_REQUEST(p);
	aomdv_rt_entry *rt;
	AOMDVBroadcastID* b = NULL;
	bool kill_request_propagation = false;
	AOMDV_Path* reverse_path = NULL;
	
	/*
	 * Drop if:
	 *      - I'm the source
	 *      - I recently heard this request.
	 */
	
	if(rq->rq_src == index) {
#ifdef DEBUG
		fprintf(stderr, "%s: got my own REQUEST\n", __FUNCTION__);
#endif // DEBUG
		Packet::free(p);
		return;
	} 
	

   /* If RREQ has already been received - drop it, else remember "RREQ id" <src IP, bcast ID>. */
   if ( (b = id_get(rq->rq_src, rq->rq_bcast_id)) == NULL)  {
		// Cache the broadcast ID
		id_insert(rq->rq_src, rq->rq_bcast_id);
		b = id_get(rq->rq_src, rq->rq_bcast_id);
   }
   else 
		kill_request_propagation = true;
	
   /* If I am a neighbor to the RREQ source, make myself first hop on path from source to dest. */
   if (rq->rq_hop_count == 0) 
		rq->rq_first_hop = index;
	
	/* 
		* We are either going to forward the REQUEST or generate a
	 * REPLY. Before we do anything, we make sure that the REVERSE
	 * route is in the route table.
	 */
	aomdv_rt_entry *rt0; // rt0 is the reverse route 
   
   rt0 = rtable.rt_lookup(rq->rq_src);
   if(rt0 == 0) { /* if not in the route table */
		// create an entry for the reverse route.
		rt0 = rtable.rt_add(rq->rq_src);
   }

	/*
	 * Create/update reverse path (i.e. path back to RREQ source)
	 * If RREQ contains more recent seq number than route table entry - update route entry to source.
	 */
	if (rt0->rt_seqno < rq->rq_src_seqno) {
		rt0->rt_seqno = rq->rq_src_seqno;
		rt0->rt_advertised_hops = INFINITY;
		rt0->path_delete(); // Delete all previous paths to RREQ source 
		rt0->rt_flags = RTF_UP;
		/* Insert new path for route entry to source of RREQ. 
			(src addr, hop count + 1, lifetime, last hop (first hop for RREQ)) */
		reverse_path = rt0->path_insert(ih->saddr(), rq->rq_hop_count+1, CURRENT_TIME + REV_ROUTE_LIFE, rq->rq_first_hop);
		// CHANGE
		rt0->rt_last_hop_count = rt0->path_get_max_hopcount();
		// CHANGE
	}
	/* If a new path with smaller hop count is received 
	(same seqno, better hop count) - try to insert new path in route table. */
	else if ( (rt0->rt_seqno == rq->rq_src_seqno) &&
				 (rt0->rt_advertised_hops > rq->rq_hop_count)
				 ) {
		AOMDV_Path* erp=NULL;
		
		assert(rt0->rt_flags == RTF_UP);  // Make sure path is up
	
		/*
		 * If path already exists - adjust the lifetime of the path.
		 */
		if ((reverse_path = rt0->disjoint_path_lookup(ih->saddr(), rq->rq_first_hop))) {
			assert(reverse_path->hopcount == (rq->rq_hop_count+1));
			reverse_path->expire = max(reverse_path->expire, (CURRENT_TIME + REV_ROUTE_LIFE)); 
		}
		/*
		 * Got a new alternate disjoint reverse path - so insert it.
		 * I.e. no path exists which has RREQ source as next hop and no 
		 * path with RREQ first hop as last hop exists for this route entry.
		 * Simply stated: no path with the same last hop exists already.
		 */
		else if (rt0->new_disjoint_path(ih->saddr(), rq->rq_first_hop)) {
			/* Only insert new path if not too many paths exists for this destination 
			and new path does not differ too much in length compared to previous paths */
			if ( (rt0->rt_num_paths_ < aomdv_max_paths_) &&
				  (((rq->rq_hop_count + 1) - rt0->path_get_min_hopcount()) <= aomdv_prim_alt_path_len_diff_)
				  ) {
				/* Insert new (disjoint) reverse path */
				reverse_path = rt0->path_insert(ih->saddr(), rq->rq_hop_count+1, CURRENT_TIME + REV_ROUTE_LIFE, rq->rq_first_hop);
				// CHANGE
				rt0->rt_last_hop_count = rt0->path_get_max_hopcount();
				// CHANGE
			}
			/* If new path differs too much in length compared to previous paths - drop packet. */
			if (((rq->rq_hop_count + 1) - rt0->path_get_min_hopcount()) > aomdv_prim_alt_path_len_diff_) {
				Packet::free(p);
				return;
			}
		}
		/* (RREQ was intended for me) AND 
			((Path with RREQ first hop as last hop does not exist) OR 
			 (The path exists and has less hop count than RREQ)) - drop packet. 
			Don't know what this case is for... */
		else if ( (rq->rq_dst == index) && 
					 ( ((erp = rt0->path_lookup_lasthop(rq->rq_first_hop)) == NULL) ||
						((rq->rq_hop_count+1) > erp->hopcount)
						)
					 )  {
			Packet::free(p);
			return;
		}
	}
	/* Older seqno (or same seqno with higher hopcount), i.e. I have a 
	more recent route entry - so drop packet.*/
	else {
		Packet::free(p);
		return;
	}

	/* If route is up */   
	if (rt0->rt_flags == RTF_UP) {
		// Reset the soft state 
		rt0->rt_req_timeout = 0.0; 
		rt0->rt_req_last_ttl = 0;
		rt0->rt_req_cnt = 0;
		
		/* 
			* Find out whether any buffered packet can benefit from the 
		 * reverse route.
		 */
		Packet *buffered_pkt;
		while ((buffered_pkt = rqueue.deque(rt0->rt_dst))) {
			if (rt0 && (rt0->rt_flags == RTF_UP)) {
				forward(rt0, buffered_pkt, NO_AOMDV_DELAY);
			}
		}
	}
	/* Check route entry for RREQ destination */
	rt = rtable.rt_lookup(rq->rq_dst);

	/* I am the intended receiver of the RREQ - so send a RREP */ 
	if (rq->rq_dst == index) {
		
		if (seqno < rq->rq_dst_seqno) {
			//seqno = max(seqno, rq->rq_dst_seqno)+1;
			seqno = rq->rq_dst_seqno + 1; //CHANGE (replaced above line with this one)
		}
		/* Make sure seq number is even (why?) */
		if (seqno%2) 
			seqno++;
		
		
		sendReply(rq->rq_src,              // IP Destination
					 0,                       // Hop Count
					 index,                   // (RREQ) Dest IP Address 
					 seqno,                   // Dest Sequence Num
					 MY_ROUTE_TIMEOUT,        // Lifetime
					 rq->rq_timestamp,        // timestamp
					 ih->saddr(),             // nexthop
					 rq->rq_bcast_id,         // broadcast id to identify this route discovery
					 ih->saddr());         
		
		Packet::free(p);
	}
	/* I have a fresh route entry for RREQ destination - so send RREP */
	else if ( rt &&
				 (rt->rt_flags == RTF_UP) &&
				 (rt->rt_seqno >= rq->rq_dst_seqno) ) {
		
		assert ((rt->rt_seqno%2) == 0);  // is the seqno even?
		/* Reverse path exists */     
		if (reverse_path) {
	#ifdef AOMDV_NODE_DISJOINT_PATHS
			if (b->count == 0) {
				b->count = 1;
				
				// route advertisement
				if (rt->rt_advertised_hops == INFINITY) 
					rt->rt_advertised_hops = rt->path_get_max_hopcount();
				
				AOMDV_Path *forward_path = rt->path_find();
				// CHANGE
				rt->rt_error = true;
				// CHANGE
				sendReply(rq->rq_src,
							 rt->rt_advertised_hops,
							 rq->rq_dst,
							 rt->rt_seqno,
							 forward_path->expire - CURRENT_TIME,
							 rq->rq_timestamp,
							 ih->saddr(), 
							 rq->rq_bcast_id,
							 forward_path->lasthop);
			}
	#endif // AOMDV_NODE_DISJOINT_PATHS
	#ifdef AOMDV_LINK_DISJOINT_PATHS
			AOMDV_Path* forward_path = NULL;
			AOMDV_Path *r = rt->rt_path_list.lh_first; // Get first path for RREQ destination
			/* Make sure we don't answer with the same forward path twice in response 
				to a certain RREQ (received more than once). E.g. "middle node" 
				in "double diamond". */
			for(; r; r = r->path_link.le_next) {
				if (b->forward_path_lookup(r->nexthop, r->lasthop) == NULL) {
					forward_path = r;
					break;
				}
			}
			/* If an unused forward path is found and we have not answered
				along this reverse path (for this RREQ) - send a RREP back. */
			if ( forward_path &&
				  (b->reverse_path_lookup(reverse_path->nexthop, reverse_path->lasthop) == NULL) ) {
				/* Mark the reverse and forward path as used (for this RREQ). */
				// Cache the broadcast ID
				b->reverse_path_insert(reverse_path->nexthop, reverse_path->lasthop);
				b->forward_path_insert(forward_path->nexthop, forward_path->lasthop);
				
				// route advertisement
				if (rt->rt_advertised_hops == INFINITY) 
					rt->rt_advertised_hops = rt->path_get_max_hopcount();
				
				// CHANGE
				rt->rt_error = true;
				// CHANGE
				sendReply(rq->rq_src,
							 rt->rt_advertised_hops,
							 rq->rq_dst,
							 rt->rt_seqno,
							 forward_path->expire - CURRENT_TIME,
							 rq->rq_timestamp,
							 ih->saddr(), 
							 rq->rq_bcast_id,
							 forward_path->lasthop);
			}
	#endif // AOMDV_LINK_DISJOINT_PATHS
		}
		Packet::free(p);
	}
	/* RREQ not intended for me and I don't have a fresh 
	enough entry for RREQ dest - so forward the RREQ */
	else {
		
		if (kill_request_propagation) {
			// do not propagate a duplicate RREQ
			Packet::free(p);
			return;
		}
		else {
			ih->saddr() = index;
			
			// Maximum sequence number seen en route
			if (rt) 
				rq->rq_dst_seqno = max(rt->rt_seqno, rq->rq_dst_seqno);
			
			// route advertisement
			if (rt0->rt_advertised_hops == INFINITY)
				rt0->rt_advertised_hops = rt0->path_get_max_hopcount();
			rq->rq_hop_count = rt0->rt_advertised_hops;
	#ifdef AOMDV_NODE_DISJOINT_PATHS
			rq->rq_first_hop = (rt0->path_find())->lasthop;
	#endif // AOMDV_NODE_DISJOINT_PATHS
			
			forward((aomdv_rt_entry*) 0, p, AOMDV_DELAY);
		}
	}
}

// AOMDV
void
AOMDV::recvReply(Packet *p) {	
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_aomdv_reply *rp = HDR_AOMDV_REPLY(p);
	aomdv_rt_entry *rt0, *rt;
	AOMDVBroadcastID* b = NULL;
	AOMDV_Path* forward_path = NULL;
	
#ifdef DEBUG
	fprintf(stderr, "%d - %s: received a REPLY\n", index, __FUNCTION__);
#endif // DEBUG
	
	
   /* If I receive a RREP with myself as source - drop packet (should not occur).
Comment: rp_dst is the source of the RREP, or rather the destination of the RREQ. */
   if (rp->rp_dst == index) {
      Packet::free(p);
      return;
   } 
	
	/*
	 *  Got a reply. So reset the "soft state" maintained for 
	 *  route requests in the request table. We don't really have
	 *  have a separate request table. It is just a part of the
	 *  routing table itself. 
	 */
	// Note that rp_dst is the dest of the data packets, not the
	// the dest of the reply, which is the src of the data packets.
	
	rt = rtable.rt_lookup(rp->rp_dst);
	
	/*
	 *  If I don't have a rt entry to this host... adding
	 */
	if(rt == 0) {
		rt = rtable.rt_add(rp->rp_dst);
	}
	
   /* If RREP contains more recent seqno for (RREQ) destination -
      delete all old paths and add the new forward path to (RREQ) destination */
   if (rt->rt_seqno < rp->rp_dst_seqno) {
		rt->rt_seqno = rp->rp_dst_seqno;
		rt->rt_advertised_hops = INFINITY;
		rt->path_delete();
		rt->rt_flags = RTF_UP;
		/* Insert forward path to RREQ destination. */
		forward_path = rt->path_insert(rp->rp_src, rp->rp_hop_count+1, CURRENT_TIME + rp->rp_lifetime, rp->rp_first_hop);
		// CHANGE
		rt->rt_last_hop_count = rt->path_get_max_hopcount();
		// CHANGE
   }
   /* If the sequence number in the RREP is the same as for route entry but 
      with a smaller hop count - try to insert new forward path to (RREQ) dest. */
   else if ( (rt->rt_seqno == rp->rp_dst_seqno) && 
				 (rt->rt_advertised_hops > rp->rp_hop_count) ) {
		
		assert (rt->rt_flags == RTF_UP);
		/* If the path already exists - increase path lifetime */
		if ((forward_path = rt->disjoint_path_lookup(rp->rp_src, rp->rp_first_hop))) {
			assert (forward_path->hopcount == (rp->rp_hop_count+1));
			forward_path->expire = max(forward_path->expire, CURRENT_TIME + rp->rp_lifetime); 
		}
		/* If the path does not already exist, there is room for it and it 
			does not differ too much in length - we add the path */
		else if ( rt->new_disjoint_path(rp->rp_src, rp->rp_first_hop) &&
					 (rt->rt_num_paths_ < aomdv_max_paths_) &&
					 ((rp->rp_hop_count+1) - rt->path_get_min_hopcount() <= aomdv_prim_alt_path_len_diff_)
					 ) {
			/* Insert forward path to RREQ destination. */
			forward_path = rt->path_insert(rp->rp_src, rp->rp_hop_count+1, CURRENT_TIME + rp->rp_lifetime, rp->rp_first_hop);
			// CHANGE
			rt->rt_last_hop_count = rt->path_get_max_hopcount();
			// CHANGE
		}
		/* Path did not exist nor could it be added - just drop packet. */
		else {
			Packet::free(p);
			return;
		}
   }
   /* The received RREP did not contain more recent information 
      than route table - so drop packet */
   else {
		Packet::free(p);
		return;
   }
   /* If route is up */
   if (rt->rt_flags == RTF_UP) {
      // Reset the soft state 
      rt->rt_req_timeout = 0.0; 
      rt->rt_req_last_ttl = 0;
      rt->rt_req_cnt = 0;
		
      if (ih->daddr() == index) {
			// I am the RREP destination
			
#ifdef DYNAMIC_RREQ_RETRY_TIMEOUT // This macro does not seem to be set.
			if (rp->rp_type == AOMDVTYPE_RREP) {
				rt->rt_disc_latency[rt->hist_indx] = (CURRENT_TIME - rp->rp_timestamp)
				/ (double) (rp->rp_hop_count+1);
				// increment indx for next time
				rt->hist_indx = (rt->hist_indx + 1) % MAX_HISTORY;
			}
#endif // DYNAMIC_RREQ_RETRY_TIMEOUT
      }
		
      /* 
			* Find out whether any buffered packet can benefit from the 
       * forward route.
       */
      Packet *buffered_pkt;
      while ((buffered_pkt = rqueue.deque(rt->rt_dst))) {
         if (rt && (rt->rt_flags == RTF_UP)) {
            forward(rt, buffered_pkt, NO_AOMDV_DELAY);
         }
      }
		
   }
   /* If I am the intended receipient of the RREP nothing more needs 
      to be done - so drop packet. */
   if (ih->daddr() == index) {
      Packet::free(p);
      return;
   }
   /* If I am not the intended receipient of the RREP - check route 
      table for a path to the RREP dest (i.e. the RREQ source). */ 
   rt0 = rtable.rt_lookup(ih->daddr());
   b = id_get(ih->daddr(), rp->rp_bcast_id); // Check for <RREQ src IP, bcast ID> tuple
	
#ifdef AOMDV_NODE_DISJOINT_PATHS
	
   if ( (rt0 == NULL) || (rt0->rt_flags != RTF_UP) || (b == NULL) || (b->count) ) {
      Packet::free(p);
      return;
   }
	
   b->count = 1;
   AOMDV_Path *reverse_path = rt0->path_find();
	
   ch->addr_type() = AF_INET;
   ch->next_hop_ = reverse_path->nexthop;
   ch->xmit_failure_ = aomdv_rt_failed_callback;
   ch->xmit_failure_data_ = (void*) this;
   
   // route advertisement
   rp->rp_src = index;
   if (rt->rt_advertised_hops == INFINITY)
		rt->rt_advertised_hops = rt->path_get_max_hopcount();
   rp->rp_hop_count = rt->rt_advertised_hops;
   rp->rp_first_hop = (rt->path_find())->lasthop;
	
   reverse_path->expire = CURRENT_TIME + ACTIVE_ROUTE_TIMEOUT;
	
   // CHANGE
   rt->rt_error = true;
   // CHANGE
   forward(rt0, p, NO_AOMDV_DELAY);
//   Scheduler::instance().schedule(target_, p, 0.);
#endif // AOMDV_NODE_DISJOINT_PATHS
#ifdef AOMDV_LINK_DISJOINT_PATHS
		/* Drop the RREP packet if we do not have a path back to the source, 
      or the route is marked as down, or if we never received the original RREQ. */
		if ( (rt0 == NULL) || (rt0->rt_flags != RTF_UP) || (b == NULL) ) {
			Packet::free(p);
			return;
		}
   /* Make sure we don't answer along the same path twice in response 
      to a certain RREQ. Try to find an unused (reverse) path to forward the RREP. */
   AOMDV_Path* reverse_path = NULL;
   AOMDV_Path *r = rt0->rt_path_list.lh_first;
	for(; r; r = r->path_link.le_next) {
			if (b->reverse_path_lookup(r->nexthop, r->lasthop) == NULL) {
				fprintf(stderr, "\tcycle cycle\n");		
				reverse_path = r;
				break;
			}
		}
   /* If an unused reverse path is found and the forward path (for 
      this RREP) has not already been replied - forward the RREP. */
   if ( reverse_path &&
        (b->forward_path_lookup(forward_path->nexthop, forward_path->lasthop) == NULL) ) {
      assert (forward_path->nexthop == rp->rp_src);
      assert (forward_path->lasthop == rp->rp_first_hop);
		/* Mark the forward and reverse path used to answer this RREQ as used. */
      b->reverse_path_insert(reverse_path->nexthop, reverse_path->lasthop);
      b->forward_path_insert(forward_path->nexthop, forward_path->lasthop);
		
      ch->addr_type() = AF_INET;
      ch->next_hop_ = reverse_path->nexthop;
      ch->xmit_failure_ = aomdv_rt_failed_callback;
      ch->xmit_failure_data_ = (void*) this;
		
      // route advertisement
      if (rt->rt_advertised_hops == INFINITY)
			rt->rt_advertised_hops = rt->path_get_max_hopcount();
      rp->rp_hop_count = rt->rt_advertised_hops;
      rp->rp_src = index;
		
      reverse_path->expire = CURRENT_TIME + ACTIVE_ROUTE_TIMEOUT;
      
      // CHANGE
      rt->rt_error = true;
      // CHANGE
      forwardReply(rt0, p, NO_AOMDV_DELAY);  // CHANGE (previously used forward())
													//      Scheduler::instance().schedule(target_, p, 0.);
   }
   else {
      Packet::free(p);
      return;
   }
#endif // AOMDV_LINK_DISJOINT_PATHS
}

// AOMDV code
void
AOMDV::recvError(Packet *p) {
#ifdef DEBUG
	fprintf(stderr, "%s: node=%d\n", __FUNCTION__, index);
#endif // DEBUG
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_aomdv_error *re = HDR_AOMDV_ERROR(p);
	aomdv_rt_entry *rt;
	u_int8_t i;
	Packet *rerr = Packet::alloc();
	struct hdr_aomdv_error *nre = HDR_AOMDV_ERROR(rerr);
	
#ifdef DEBUG
	fprintf(stderr, "%s: multipath version\n", __FUNCTION__);
#endif // DEBUG
	
	nre->DestCount = 0;
	
	for (i=0; i<re->DestCount; i++) {
		// For each unreachable destination
		AOMDV_Path* path;
		rt = rtable.rt_lookup(re->unreachable_dst[i]);
		/* If route entry exists, route is up, a path to the unreachable 
			destination exists through the neigbor from which RERR was 
			received, and my sequence number is not more recent - delete 
			path and add it to the RERR message I will send. */   
		if ( rt && (rt->rt_flags == RTF_UP) &&
			  (path = rt->path_lookup(ih->saddr())) &&
			  (rt->rt_seqno <= re->unreachable_dst_seqno[i]) ) {
			assert((rt->rt_seqno%2) == 0); // is the seqno even?
#ifdef DEBUG
			fprintf(stderr, "%s(%f): %d\t(%d\t%u\t%d)\t(%d\t%u\t%d)\n",
					  __FUNCTION__,CURRENT_TIME,
					  index, rt->rt_dst, rt->rt_seqno, ih->src_.addr_,
					  re->unreachable_dst[i],re->unreachable_dst_seqno[i],
					  ih->src_.addr_);
#endif // DEBUG
			
			rt->path_delete(ih->saddr());
			rt->rt_highest_seqno_heard = max(rt->rt_highest_seqno_heard, re->unreachable_dst_seqno[i]);
			if (rt->path_empty()) {
				rt->rt_seqno = rt->rt_highest_seqno_heard;
				rt_down(rt);
				// CHANGE
				if (rt->rt_error) {
					nre->unreachable_dst[nre->DestCount] = rt->rt_dst;
					nre->unreachable_dst_seqno[nre->DestCount] = rt->rt_seqno;
					nre->DestCount += 1;
					rt->rt_error = false;
				}
				// CHANGE
			}
		}
	} 
	
	if (nre->DestCount > 0) {
#ifdef DEBUG
		fprintf(stderr, "%s(%f): %d\t sending RERR...\n", __FUNCTION__, CURRENT_TIME, index);
#endif // DEBUG
		sendError(rerr);
	}
	else {
		Packet::free(rerr);
	}
	
	Packet::free(p);
}


/*
 Packet Transmission Routines
 */

void
AOMDV::forward(aomdv_rt_entry *rt, Packet *p, double delay) {
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	
	if(ih->ttl_ == 0) {
		
#ifdef DEBUG
		fprintf(stderr, "%s: calling drop()\n", __PRETTY_FUNCTION__);
#endif // DEBUG
		
		drop(p, DROP_RTR_TTL);
		return;
	}

	
	// AODV ns-2.31 code
	if ((( ch->ptype() != PT_AOMDV && ch->direction() == hdr_cmn::UP ) &&
		 ((u_int32_t)ih->daddr() == IP_BROADCAST))
		 || (ih->daddr() == here_.addr_)) {
		dmux_->recv(p,0);
		return;
	}
	
	if (rt) {
		assert(rt->rt_flags == RTF_UP);
		// AOMDV code
		ch->addr_type() = NS_AF_INET;
		AOMDV_Path *path = rt->path_find();
		ch->next_hop() = path->nexthop;
		path->expire = CURRENT_TIME + ACTIVE_ROUTE_TIMEOUT;
		// CHANGE
		if ((ih->saddr() != index) && DATA_PACKET(ch->ptype())) {
			rt->rt_error = true;
		}
		// CHANGE 
		ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction
	}
	else { // if it is a broadcast packet
			 // assert(ch->ptype() == PT_AODV); // maybe a diff pkt type like gaf
		assert(ih->daddr() == (nsaddr_t) IP_BROADCAST);
		ch->addr_type() = NS_AF_NONE;
		ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction
	}
	
	if (ih->daddr() == (nsaddr_t) IP_BROADCAST) {
		// If it is a broadcast packet
		assert(rt == 0);
		/*
		 *  Jitter the sending of broadcast packets by 10ms
		 */
		Scheduler::instance().schedule(target_, p,
												 0.01 * Random::uniform());
	}
	else { // Not a broadcast packet 
		if(delay > 0.0) {
			Scheduler::instance().schedule(target_, p, delay);
		}
		else {
			// Not a broadcast packet, no delay, send immediately
			Scheduler::instance().schedule(target_, p, 0.);
		}
	}
	
}


void
AOMDV::sendRequest(nsaddr_t dst) {
	// Allocate a RREQ packet 
	Packet *p = Packet::alloc();
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_aomdv_request *rq = HDR_AOMDV_REQUEST(p);
	aomdv_rt_entry *rt = rtable.rt_lookup(dst);
	assert(rt);
	
	/*
	 *  Rate limit sending of Route Requests. We are very conservative
	 *  about sending out route requests. 
	 */
	
	if (rt->rt_flags == RTF_UP) {
		assert(rt->rt_hops != INFINITY2);
		Packet::free((Packet *)p);
		return;
	}
	
	if (rt->rt_req_timeout > CURRENT_TIME) {
		Packet::free((Packet *)p);
		return;
	}
	
	// rt_req_cnt is the no. of times we did network-wide broadcast
	// RREQ_RETRIES is the maximum number we will allow before 
	// going to a long timeout.
	
	if (rt->rt_req_cnt > RREQ_RETRIES) {
		rt->rt_req_timeout = CURRENT_TIME + MAX_RREQ_TIMEOUT;
		rt->rt_req_cnt = 0;
		Packet *buf_pkt;
		while ((buf_pkt = rqueue.deque(rt->rt_dst))) {
			drop(buf_pkt, DROP_RTR_NO_ROUTE);
		}
		Packet::free((Packet *)p);
		return;
	}
	
#ifdef DEBUG
   fprintf(stderr, "(%2d) - %2d sending Route Request, dst: %d\n",
			  ++route_request, index, rt->rt_dst);
#endif // DEBUG
	
	// Determine the TTL to be used this time. 
	// Dynamic TTL evaluation - SRD
	
	rt->rt_req_last_ttl = max(rt->rt_req_last_ttl,rt->rt_last_hop_count);
	
	if (0 == rt->rt_req_last_ttl) {
		// first time query broadcast
		ih->ttl_ = TTL_START;
	}
	else {
		// Expanding ring search.
		if (rt->rt_req_last_ttl < TTL_THRESHOLD)
			ih->ttl_ = rt->rt_req_last_ttl + TTL_INCREMENT;
		else {
			// network-wide broadcast
			ih->ttl_ = NETWORK_DIAMETER;
			rt->rt_req_cnt += 1;
		}
	}
	
	// remember the TTL used  for the next time
	rt->rt_req_last_ttl = ih->ttl_;
	
	// PerHopTime is the roundtrip time per hop for route requests.
	// The factor 2.0 is just to be safe .. SRD 5/22/99
	// Also note that we are making timeouts to be larger if we have 
	// done network wide broadcast before. 
	
	rt->rt_req_timeout = 2.0 * (double) ih->ttl_ * PerHopTime(rt); 
	if (rt->rt_req_cnt > 0)
		rt->rt_req_timeout *= rt->rt_req_cnt;
	rt->rt_req_timeout += CURRENT_TIME;
	
	// Don't let the timeout to be too large, however .. SRD 6/8/99
	if (rt->rt_req_timeout > CURRENT_TIME + MAX_RREQ_TIMEOUT)
		rt->rt_req_timeout = CURRENT_TIME + MAX_RREQ_TIMEOUT;
	rt->rt_expire = 0;
	
#ifdef DEBUG
	fprintf(stderr, "(%2d) - %2d sending Route Request, dst: %d, tout %f ms\n",
			  ++route_request, 
			  index, rt->rt_dst, 
			  rt->rt_req_timeout - CURRENT_TIME);
#endif	// DEBUG
	
	
	// Fill out the RREQ packet 
	// ch->uid() = 0;
	ch->ptype() = PT_AOMDV;
	ch->size() = IP_HDR_LEN + rq->size();
	ch->iface() = -2;
	ch->error() = 0;
	ch->addr_type() = NS_AF_NONE;
	ch->prev_hop_ = index;          // AODV hack
	
	ih->saddr() = index;
	ih->daddr() = IP_BROADCAST;
	ih->sport() = RT_PORT;
	ih->dport() = RT_PORT;
	
	// Fill up some more fields. 
	rq->rq_type = AOMDVTYPE_RREQ;
	// AOMDV code
	rq->rq_hop_count = 0;
	rq->rq_bcast_id = bid++;
	rq->rq_dst = dst;
	rq->rq_dst_seqno = (rt ? rt->rt_seqno : 0);
	rq->rq_src = index;
	seqno += 2;
	assert ((seqno%2) == 0);
	rq->rq_src_seqno = seqno;
	rq->rq_timestamp = CURRENT_TIME;
	
	Scheduler::instance().schedule(target_, p, 0.);
	
}

// AOMDV code
void
AOMDV::sendReply(nsaddr_t ipdst, u_int32_t hop_count, nsaddr_t rpdst,
					  u_int32_t rpseq, double lifetime, double timestamp, 
					  nsaddr_t nexthop, u_int32_t bcast_id, nsaddr_t rp_first_hop) {
	Packet *p = Packet::alloc();
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_aomdv_reply *rp = HDR_AOMDV_REPLY(p);

	
#ifdef DEBUG
	fprintf(stderr, "sending Reply from %d at %.2f\n", index, Scheduler::instance().clock());
#endif // DEBUG
	
	rp->rp_type = AOMDVTYPE_RREP;
	//rp->rp_flags = 0x00;
	rp->rp_hop_count = hop_count;
	rp->rp_dst = rpdst;
	rp->rp_dst_seqno = rpseq;
	rp->rp_src = index;
	rp->rp_lifetime = lifetime;
	rp->rp_timestamp = timestamp;
   rp->rp_bcast_id = bcast_id;
   rp->rp_first_hop = rp_first_hop;
   
	// ch->uid() = 0;
	ch->ptype() = PT_AOMDV;
	ch->size() = IP_HDR_LEN + rp->size();
	ch->iface() = -2;
	ch->error() = 0;
	ch->addr_type() = NS_AF_INET;
	
   ch->next_hop_ = nexthop;
   
   ch->xmit_failure_ = aomdv_rt_failed_callback;
   ch->xmit_failure_data_ = (void*) this;
	
	ih->saddr() = index;
	ih->daddr() = ipdst;
	ih->sport() = RT_PORT;
	ih->dport() = RT_PORT;
	ih->ttl_ = NETWORK_DIAMETER;
	
	Scheduler::instance().schedule(target_, p, 0.);
	
}

void
AOMDV::sendError(Packet *p, bool jitter) {
#ifdef ERROR
	fprintf(stderr, "%s: node=%d\n", __FUNCTION__, index);
#endif // DEBUG
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_aomdv_error *re = HDR_AOMDV_ERROR(p);
	
#ifdef ERROR
	fprintf(stderr, "sending Error from %d at %.2f\n", index, Scheduler::instance().clock());
#endif // DEBUG
	
	re->re_type = AOMDVTYPE_RERR;
	//re->reserved[0] = 0x00; re->reserved[1] = 0x00;
	// DestCount and list of unreachable destinations are already filled
	
	// ch->uid() = 0;
	ch->ptype() = PT_AOMDV;
	ch->size() = IP_HDR_LEN + re->size();
	ch->iface() = -2;
	ch->error() = 0;
	ch->addr_type() = NS_AF_NONE;
	ch->next_hop_ = 0;
	ch->prev_hop_ = index;          // AODV hack
	ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction
	
	ih->saddr() = index;
	ih->daddr() = IP_BROADCAST;
	ih->sport() = RT_PORT;
	ih->dport() = RT_PORT;
	ih->ttl_ = 1;
	
	// Do we need any jitter? Yes
	if (jitter)
		Scheduler::instance().schedule(target_, p, 0.01*Random::uniform());
	else
		Scheduler::instance().schedule(target_, p, 0.0);
	
}


/*
 Neighbor Management Functions
 */

void
AOMDV::sendHello() {
	Packet *p = Packet::alloc();
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_aomdv_reply *rh = HDR_AOMDV_REPLY(p);
	
#ifdef DEBUG
	fprintf(stderr, "sending Hello from %d at %.2f\n", index, Scheduler::instance().clock());
#endif // DEBUG
	
	rh->rp_type = AOMDVTYPE_HELLO;
	//rh->rp_flags = 0x00;
	// AOMDV code
	rh->rp_hop_count = 0;
	rh->rp_dst = index;
	rh->rp_dst_seqno = seqno;
	rh->rp_lifetime = (1 + ALLOWED_HELLO_LOSS) * HELLO_INTERVAL;
	
	// ch->uid() = 0;
	ch->ptype() = PT_AOMDV;
	ch->size() = IP_HDR_LEN + rh->size();
	ch->iface() = -2;
	ch->error() = 0;
	ch->addr_type() = NS_AF_NONE;
	ch->prev_hop_ = index;          // AODV hack
	
	ih->saddr() = index;
	ih->daddr() = IP_BROADCAST;
	ih->sport() = RT_PORT;
	ih->dport() = RT_PORT;
	ih->ttl_ = 1;
	
	Scheduler::instance().schedule(target_, p, 0.0);
}


void
AOMDV::recvHello(Packet *p) {
	// AOMDV code
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_aomdv_reply *rp = HDR_AOMDV_REPLY(p);
	AOMDV_Neighbor *nb;
	
	nb = nb_lookup(rp->rp_dst);
	if(nb == 0) {
		nb_insert(rp->rp_dst);
	}
	else {
		nb->nb_expire = CURRENT_TIME +
		(1.5 * ALLOWED_HELLO_LOSS * HELLO_INTERVAL);
	}
	
	// AOMDV code
	// Add a route to this neighbor
	ih->daddr() = index;
	rp->rp_src = ih->saddr();
	rp->rp_first_hop = index;
	recvReply(p);
	
	//Packet::free(p);  // CHANGED BY ME (commented this line)
}

// AOMDV code
void
AOMDV::nb_insert(nsaddr_t id) {
	// CHANGE 
	AOMDV_Neighbor *nb;
	if ( ( nb=nb_lookup(id) ) == NULL) {
		nb = new AOMDV_Neighbor(id);
		assert(nb);
		// CHANGE
		nb->nb_expire = CURRENT_TIME + (HELLO_INTERVAL * ALLOWED_HELLO_LOSS);
		
		LIST_INSERT_HEAD(&nbhead, nb, nb_link);
	}
	else {
		// CHANGE
		nb->nb_expire = CURRENT_TIME + (HELLO_INTERVAL * ALLOWED_HELLO_LOSS);
	}
}


AOMDV_Neighbor*
AOMDV::nb_lookup(nsaddr_t id) {
	AOMDV_Neighbor *nb = nbhead.lh_first;
	
	for(; nb; nb = nb->nb_link.le_next) {
		if(nb->nb_addr == id) break;
	}
	return nb;
}


/*
 * Called when we receive *explicit* notification that a Neighbor
 * is no longer reachable.
 */
void
AOMDV::nb_delete(nsaddr_t id) {
	AOMDV_Neighbor *nb = nbhead.lh_first;
	
	log_link_del(id);
	seqno += 2;     // Set of neighbors changed
	assert ((seqno%2) == 0);
	
	for(; nb; nb = nb->nb_link.le_next) {
		if(nb->nb_addr == id) {
			LIST_REMOVE(nb,nb_link);
			delete nb;
			break;
		}
	}
	
	handle_link_failure(id);
	
	// AOMDV code
	Packet *p;
#ifdef AOMDV_PACKET_SALVAGING
//	while((p = AOMDVifqueue->prq_get_nexthop(id))) {
	while((p = AOMDVifqueue->filter(id))) {
		struct hdr_cmn *ch = HDR_CMN(p);
		struct hdr_ip *ih = HDR_IP(p);
		if ( !DATA_PACKET(ch->ptype()) ) drop(p, DROP_RTR_HELLO);
		else {
			// salvage the packet using an alternate path if available.
			aomdv_rt_entry *rt = rtable.rt_lookup(ih->daddr());
			if ( rt && (rt->rt_flags == RTF_UP) && (ch->aomdv_salvage_count_ < AOMDV_MAX_SALVAGE_COUNT) ) {
				ch->aomdv_salvage_count_ += 1;
				forward(rt, p, NO_AOMDV_DELAY);
			}
			else drop(p, DROP_RTR_HELLO);
		}
	} 
	
#else // NO PACKET SALVAGING
	
	while((p = AOMDVifqueue->filter(id))) {
		drop(p, DROP_RTR_HELLO);
	}
	
	/*while((p = AOMDVifqueue->prq_get_nexthop(id))) {
		drop(p, DROP_RTR_HELLO);
	} */
	
#endif // NO PACKET SALVAGING 
}


/*
 * Purges all timed-out Neighbor Entries - runs every
 * HELLO_INTERVAL * 1.5 seconds.
 */
void
AOMDV::nb_purge() {
	AOMDV_Neighbor *nb = nbhead.lh_first;
	AOMDV_Neighbor *nbn;
	double now = CURRENT_TIME;
	
	for(; nb; nb = nbn) {
		nbn = nb->nb_link.le_next;
		if(nb->nb_expire <= now) {
			nb_delete(nb->nb_addr);
		}
	}
	
}


// AOMDV code
void
AOMDV::forwardReply(aomdv_rt_entry *rt, Packet *p, double delay) {
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	
	if(ih->ttl_ == 0) {
		
#ifdef DEBUG
		fprintf(stderr, "%s: calling drop()\n", __PRETTY_FUNCTION__);
#endif // DEBUG
		
		drop(p, DROP_RTR_TTL);
		return;
	}
	
	if (rt) {
		assert(rt->rt_flags == RTF_UP);
		ch->addr_type() = NS_AF_INET;
		
		// CHANGE (don't want to get a new nexthop for this route entry. Use nexthop set in recvReply().
		//   AODV_Path *path = rt->path_find();
		//   ch->next_hop() = path->nexthop;
		//   path->expire = CURRENT_TIME + ACTIVE_ROUTE_TIMEOUT; 
		// CHANGE
		
		// CHANGE
		if ((ih->saddr() != index) && DATA_PACKET(ch->ptype())) {
			rt->rt_error = true;
		}
		// CHANGE 		
		ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction
 }
	else { // if it is a broadcast packet
			 // assert(ch->ptype() == PT_AODV); // maybe a diff pkt type like gaf
		assert(ih->daddr() == (nsaddr_t) IP_BROADCAST);
		ch->addr_type() = NS_AF_NONE;
		ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction
	}
	
	if (ih->daddr() == (nsaddr_t) IP_BROADCAST) {
		// If it is a broadcast packet
		assert(rt == 0);
		/*
		 *  Jitter the sending of broadcast packets by 10ms
		 */
		Scheduler::instance().schedule(target_, p,
												 0.01 * Random::uniform());
	}
	else { // Not a broadcast packet 
		if(delay > 0.0) {
			Scheduler::instance().schedule(target_, p, delay);
		}
		else {
			// Not a broadcast packet, no delay, send immediately
			Scheduler::instance().schedule(target_, p, 0.);
		}
	}
	
}
