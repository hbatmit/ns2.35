
/*
 * dsragent.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: dsragent.cc,v 1.38 2009/12/30 22:06:34 tom_henderson Exp $
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
/* 
   dsragent.cc

   requires a radio model such that sendPacket returns true
   iff the packet is recieved by the destination node.
   
   Ported from CMU/Monarch's code, appropriate copyright applies.  
*/

extern "C" {
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <float.h>
}

#include <object.h>
#include <agent.h>
#include <trace.h>
#include <packet.h>
#include <scheduler.h>
#include <random.h>

#include <mac.h>
#include <ll.h>
#include <cmu-trace.h>

#include "path.h"
#include "srpacket.h"
#include "routecache.h"
#include "requesttable.h"
#include "dsragent.h"

/*==============================================================
  Declarations and global defintions
------------------------------------------------------------*/
// #define NEW_IFQ_LOGIC
// #define NEW_REQUEST_LOGIC
#define NEW_SALVAGE_LOGIC

#ifdef NEW_SALVAGE_LOGIC
/*
 *  Maximum number of times that a packet may be salvaged.
 */
static int dsr_salvage_max_attempts = 15;
/*
 *  Maximum number of Route Requests that can be sent for a salvaged
 *  packets that was originated at another node.
 */
static int dsr_salvage_max_requests = 1;
/*
 *  May an intermediate node send a propagating Route Request for
 *  a salvaged packet that was originated elsewhere.
 */
static bool dsr_salvage_allow_propagating = 0;

#endif

/* couple of flowstate constants... */
static const bool dsragent_enable_flowstate = true;
static const bool dsragent_prefer_default_flow = true;
static const bool dsragent_prefer_shorter_over_default = true;
static const bool dsragent_always_reestablish = true;
static const int min_adv_interval = 5;
static const int default_flow_timeout = 60;
//#define DSRFLOW_VERBOSE

static const int verbose = 0;
static const int verbose_srr = 0;
static const int verbose_ssalv = 1;

DSRAgent_List DSRAgent::agthead = { 0 };

Time arp_timeout = 30.0e-3;	// (sec) arp request timeout
Time rt_rq_period = 0.5;	// (sec) length of one backoff period
Time rt_rq_max_period = 10.0;	// (sec) maximum time between rt reqs
Time send_timeout = SEND_TIMEOUT; // (sec) how long a packet can live in sendbuf

#if 0
/* used in route reply holdoffs, which are currently disabled -dam 5/98 */
Time rt_rep_holdoff_period = 3.0e-3; // secs (about 2*process_time)
// to determine how long to sit on our rt reply we pick a number
// U(O.0,rt_rep_holdoff_period) + (our route length-1)*rt_rep_holdoff
#endif //0

Time grat_hold_down_time = 1.0;	// (sec) min time between grat replies for
				// same route

Time max_err_hold = 1.0;        // (sec) 
// maximum time between when we recv a route error told to us, and when we
// transmit a propagating route request that can carry that data.  used to
// keep us from propagating stale route error data


/*************** selectors ******************/
bool dsragent_snoop_forwarded_errors = true;
// give errors we forward to our cache?
bool dsragent_snoop_source_routes = true;
// should we snoop on any source routes we see?
bool dsragent_reply_only_to_first_rtreq = false;
// should we only respond to the first route request we receive from a host?
bool dsragent_propagate_last_error = true;
// should we take the data from the last route error msg sent to us
// and propagate it around on the next propagating route request we do?
// this is aka grat route error propagation
bool dsragent_send_grat_replies = true;
// should we send gratuitous replies to effect route shortening?
bool dsragent_salvage_with_cache = true;
// should we consult our cache for a route if we get a xmitfailure
// and salvage the packet using the route if possible
bool dsragent_use_tap = true;
// should we listen to a promiscuous tap?
bool dsragent_reply_from_cache_on_propagating = true;
// should we consult the route cache before propagating rt req's and
// answer if possible?
bool dsragent_ring_zero_search = true;
// should we send a non-propagating route request as the first action
// in each route discovery action?

// NOTE: to completely turn off replying from cache, you should
// set both dsragent_ring_zero_search and 
// dsragent_reply_from_cache_on_propagating to false

bool dsragent_dont_salvage_bad_replies = true;
// if we have an xmit failure on a packet, and the packet contains a 
// route reply, should we scan the reply to see if contains the dead link?
// if it does, we won't salvage the packet unless there's something aside
// from a reply in it (in which case we salvage, but cut out the rt reply)
bool dsragent_require_bi_routes = true;
// do we need to have bidirectional source routes? 
// [XXX this flag doesn't control all the behaviors and code that assume
// bidirectional links -dam 5/14/98]

#if 0
bool lsnode_holdoff_rt_reply = true;
// if we have a cached route to reply to route_request with, should we
// hold off and not send it for a while?
bool lsnode_require_use = true;
// do we require ourselves to hear a route requestor use a route
// before we withold our route, or is merely hearing another (better)
// route reply enough?
#endif

/*


Our strategy is as follows:

 - it's only worth discovering bidirectional routes, since all data
 paths will have to have to be bidirectional for 802.11 ACKs to work

 - reply to all route requests for us that we recv (not just the first one)
 but reply to them by reversing the route and unicasting.  don't do
 a route request (since that will end up returning the requestor lots of
 routes that are potentially unidirectional). By reversing the discovered 
 route for the route reply, only routes that are bidirectional will make it
 back the original requestor

 - once a packet goes into the sendbuffer, it can't be piggybacked on a 
 route request.  the code assumes that the only thing that removes
 packets from the send buff is the StickPktIn routine, or the route reply
 arrives routine

*/

/* Callback helpers */
void
XmitFailureCallback(Packet *pkt, void *data)
{
  DSRAgent *agent = (DSRAgent *)data; // cast of trust
  agent->xmitFailed(pkt);
}

void
XmitFlowFailureCallback(Packet *pkt, void *data)
{
  DSRAgent *agent = (DSRAgent *)data;
  agent->xmitFlowFailed(pkt);
}


/*===========================================================================
  SendBuf management and helpers
---------------------------------------------------------------------------*/
void
SendBufferTimer::expire(Event *) 
{ 
  a_->sendBufferCheck(); 
  resched(BUFFER_CHECK + BUFFER_CHECK * Random::uniform(1.0));
}

void
DSRAgent::dropSendBuff(SRPacket &p)
  // log p as being dropped by the sendbuffer in DSR agent
{
  trace("Ssb %.5f _%s_ dropped %s -> %s", Scheduler::instance().clock(), 
	net_id.dump(), p.src.dump(), p.dest.dump());
  drop(p.pkt, DROP_RTR_QTIMEOUT);
  p.pkt = 0;
  p.route.reset();
}

void
DSRAgent::stickPacketInSendBuffer(SRPacket& p)
{
  Time min = DBL_MAX;
  int min_index = 0;
  int c;

  if (verbose)
    trace("Sdebug %.5f _%s_ stuck into send buff %s -> %s",
	  Scheduler::instance().clock(), 
	  net_id.dump(), p.src.dump(), p.dest.dump());

  for (c = 0 ; c < SEND_BUF_SIZE ; c ++)
    if (send_buf[c].p.pkt == NULL)
      {
	send_buf[c].t = Scheduler::instance().clock();
	send_buf[c].p = p;
	return;
      }
    else if (send_buf[c].t < min)
      {
	min = send_buf[c].t;
	min_index = c;
      }
  
  // kill somebody
  dropSendBuff(send_buf[min_index].p);
  send_buf[min_index].t = Scheduler::instance().clock();
  send_buf[min_index].p = p;
}

void
DSRAgent::sendBufferCheck()
  // see if any packets in send buffer need route requests sent out
  // for them, or need to be expired
{ // this is called about once a second.  run everybody through the
  // get route for pkt routine to see if it's time to do another 
  // route request or what not
  int c;

  for (c  = 0 ; c <SEND_BUF_SIZE ; c++) {
	  if (send_buf[c].p.pkt == NULL)
		  continue;
	  if (Scheduler::instance().clock() - send_buf[c].t > send_timeout) {
		  dropSendBuff(send_buf[c].p);
		  send_buf[c].p.pkt = 0;
		  continue;
	  }
#ifdef DEBUG
	  trace("Sdebug %.5f _%s_ checking for route for dst %s",
		Scheduler::instance().clock(), net_id.dump(), 
		send_buf[c].p.dest.dump());
#endif

	  handlePktWithoutSR(send_buf[c].p, true);
#ifdef DEBUG
	  if (send_buf[c].p.pkt == NULL) 
		  trace("Sdebug %.5f _%s_ sendbuf pkt to %s liberated by handlePktWOSR",
			Scheduler::instance().clock(), net_id.dump(), 
			send_buf[c].p.dest.dump());
#endif
  }
}

/*==============================================================
  Route Request backoff
------------------------------------------------------------*/
static bool
BackOffTest(Entry *e, Time time)
// look at the entry and decide if we can send another route
// request or not.  update entry as well
{
  Time next = ((Time) (0x1 << (e->rt_reqs_outstanding * 2))) * rt_rq_period;

  if (next > rt_rq_max_period)
	  next = rt_rq_max_period;

  if (next + e->last_rt_req > time)
	  return false;

  // don't let rt_reqs_outstanding overflow next on the LogicalShiftsLeft's
  if (e->rt_reqs_outstanding < 15)
	  e->rt_reqs_outstanding++;

  e->last_rt_req = time;

  return true;
}

/*===========================================================================
  DSRAgent OTcl linkage
---------------------------------------------------------------------------*/
static class DSRAgentClass : public TclClass {
public:
  DSRAgentClass() : TclClass("Agent/DSRAgent") {}
  TclObject* create(int, const char*const*) {
    return (new DSRAgent);
  }
} class_DSRAgent;

/*===========================================================================
  DSRAgent methods
---------------------------------------------------------------------------*/
DSRAgent::DSRAgent(): Agent(PT_DSR), request_table(128), route_cache(NULL),
send_buf_timer(this), flow_table(), ars_table()
{
  int c;
  route_request_num = 1;

  route_cache = makeRouteCache();

  for (c = 0 ; c < RTREP_HOLDOFF_SIZE ; c++)
	  rtrep_holdoff[c].requested_dest = invalid_addr;
  num_heldoff_rt_replies = 0;

  target_ = 0;
  logtarget = 0;

  grat_hold_victim = 0;
  for (c = 0; c < RTREP_HOLDOFF_SIZE ; c++) {
    grat_hold[c].t = 0;
    grat_hold[c].p.reset();
  }

  //bind("off_SR_", &off_sr_);
  //bind("off_ll_", &off_ll_);
  //bind("off_mac_", &off_mac_);
  //bind("off_ip_", &off_ip_);

  ll = 0;
  ifq = 0;
  mac_ = 0;

  LIST_INSERT_HEAD(&agthead, this, link);
#ifdef DSR_FILTER_TAP
  bzero(tap_uid_cache, sizeof(tap_uid_cache));
#endif
  route_error_held = false;
}

DSRAgent::~DSRAgent()
{
  fprintf(stderr,"DFU: Don't do this! I haven't figured out ~DSRAgent\n");
  exit(-1);
}

void
DSRAgent::Terminate()
{
	int c;
	for (c  = 0 ; c < SEND_BUF_SIZE ; c++) {
		if (send_buf[c].p.pkt) {
			drop(send_buf[c].p.pkt, DROP_END_OF_SIMULATION);
			send_buf[c].p.pkt = 0;
		}
	}
}

void
DSRAgent::testinit()
{
  struct hdr_sr hsr;
  
  if (net_id == ID(1,::IP))
    {
      printf("adding route to 1\n");
      hsr.init();
      hsr.append_addr( 1, NS_AF_INET );
      hsr.append_addr( 2, NS_AF_INET );
      hsr.append_addr( 3, NS_AF_INET );
      hsr.append_addr( 4, NS_AF_INET );
      
      route_cache->addRoute(Path(hsr.addrs(),
				 hsr.num_addrs()), 0.0, ID(1,::IP));
    }
  
  if (net_id == ID(3,::IP))
    {
      printf("adding route to 3\n");
      hsr.init();
      hsr.append_addr( 3, NS_AF_INET );
      hsr.append_addr( 2, NS_AF_INET );
      hsr.append_addr( 1, NS_AF_INET );
      
      route_cache->addRoute(Path(hsr.addrs(),
				 hsr.num_addrs()), 0.0, ID(3,::IP));
    }
}


int
DSRAgent::command(int argc, const char*const* argv)
{
  TclObject *obj;  

  if (argc == 2) 
    {
      if (strcasecmp(argv[1], "testinit") == 0)
	{
	  testinit();
	  return TCL_OK;
	}
      if (strcasecmp(argv[1], "reset") == 0)
	{
	  Terminate();
	  return Agent::command(argc, argv);
	}
      if (strcasecmp(argv[1], "check-cache") == 0)
	{
	  return route_cache->command(argc, argv);
	}
      if (strcasecmp(argv[1], "startdsr") == 0)
	{
	  if (ID(1,::IP) == net_id) 
	    { // log the configuration parameters of the dsragent
  trace("Sconfig %.5f tap: %s snoop: rts? %s errs? %s",
		    Scheduler::instance().clock(),
		    dsragent_use_tap ? "on" : "off",
		    dsragent_snoop_source_routes ? "on" : "off",
		    dsragent_snoop_forwarded_errors ? "on" : "off");
  trace("Sconfig %.5f salvage: %s !bd replies? %s",
		    Scheduler::instance().clock(),
		    dsragent_salvage_with_cache ? "on" : "off",
		    dsragent_dont_salvage_bad_replies ? "on" : "off");
  trace("Sconfig %.5f grat error: %s grat reply: %s",
	            Scheduler::instance().clock(),
	            dsragent_propagate_last_error ? "on" : "off",
	            dsragent_send_grat_replies ? "on" : "off");
  trace("Sconfig %.5f $reply for props: %s ring 0 search: %s",
	            Scheduler::instance().clock(),
	            dsragent_reply_from_cache_on_propagating ? "on" : "off",
	            dsragent_ring_zero_search ? "on" : "off");
	    }
	  // cheap source of jitter
	  send_buf_timer.sched(BUFFER_CHECK 
			       + BUFFER_CHECK * Random::uniform(1.0));	  
          return route_cache->command(argc,argv);
	}
    }
  else if(argc == 3) 
    {
      if (strcasecmp(argv[1], "addr") == 0) 
	{
	  int temp;
	  temp = Address::instance().str2addr(argv[2]);
	 net_id = ID(temp, ::IP);
	 flow_table.setNetAddr(net_id.addr);
	 route_cache->net_id = net_id;
	 return TCL_OK;
	} 
      else if(strcasecmp(argv[1], "mac-addr") == 0) 
	{
	  MAC_id = ID(atoi(argv[2]), ::MAC);
	  route_cache->MAC_id = MAC_id;
	  return TCL_OK;
	}
      else if(strcasecmp(argv[1], "rt_rq_max_period") == 0)
        {
          rt_rq_max_period = strtod(argv[2],NULL);
          return TCL_OK;
        }
      else if(strcasecmp(argv[1], "rt_rq_period") == 0)
        {
          rt_rq_period = strtod(argv[2],NULL);
          return TCL_OK;
        }
      else if(strcasecmp(argv[1], "send_timeout") == 0)
        {
          send_timeout = strtod(argv[2],NULL);
          return TCL_OK;
        }

      
      if( (obj = TclObject::lookup(argv[2])) == 0) 
	{
	  fprintf(stderr, "DSRAgent: %s lookup of %s failed\n", argv[1],
		  argv[2]);
	  return TCL_ERROR;
	}

      if (strcasecmp(argv[1], "log-target") == 0)  {
	      logtarget = (Trace*) obj;
	      return route_cache->command(argc, argv);
      }
      else if (strcasecmp(argv[1], "tracetarget") == 0 )
       	{
	  logtarget = (Trace*) obj;
	  return route_cache->command(argc, argv);
	}
      else if (strcasecmp(argv[1], "install-tap") == 0)  
	{
	  mac_ = (Mac*) obj;
	  mac_->installTap(this);
	  return TCL_OK;
	}
      else if (strcasecmp(argv[1], "node") == 0)
	{
	  node_ = (MobileNode *) obj;
	  return TCL_OK;
	}
      else if (strcasecmp (argv[1], "port-dmux") == 0) 
	{
	  port_dmux_ = (NsObject *) obj;
	  return TCL_OK;
	}
    }
  else if (argc == 4)
    {
      if (strcasecmp(argv[1], "add-ll") == 0) 
	{
	  if( (obj = TclObject::lookup(argv[2])) == 0) {
	    fprintf(stderr, "DSRAgent: %s lookup of %s failed\n", argv[1],
		    argv[2]);
	    return TCL_ERROR;
	  }
	  ll = (NsObject*) obj;
	  if( (obj = TclObject::lookup(argv[3])) == 0) {
	    fprintf(stderr, "DSRAgent: %s lookup of %s failed\n", argv[1],
		    argv[3]);
	    return TCL_ERROR;
	  }
	  ifq = (CMUPriQueue *) obj;
	  return TCL_OK;

	}


    }
  return Agent::command(argc, argv);
}

void
DSRAgent::sendOutBCastPkt(Packet *p)
{
  hdr_cmn *cmh =  hdr_cmn::access(p);
  if(cmh->direction() == hdr_cmn::UP)
    cmh->direction() = hdr_cmn::DOWN;
  // no jitter required
  Scheduler::instance().schedule(ll, p, 0.0);
}



void
DSRAgent::recv(Packet* packet, Handler*)
  /* handle packets with a MAC destination address of this host, or
     the MAC broadcast addr */
{
  hdr_sr *srh =  hdr_sr::access(packet);
  hdr_ip *iph =  hdr_ip::access(packet);
  hdr_cmn *cmh =  hdr_cmn::access(packet);

  // special process for GAF
  if (cmh->ptype() == PT_GAF) {
    if (iph->daddr() == (int)IP_BROADCAST) { 
      if(cmh->direction() == hdr_cmn::UP)
	cmh->direction() = hdr_cmn::DOWN;
      Scheduler::instance().schedule(ll,packet,0);
      return;
    } else {
      target_->recv(packet, (Handler*)0);
      return;	  
    }
  }

  assert(cmh->size() >= 0);

  SRPacket p(packet, srh);
  //p.dest = ID(iph->dst(),::IP);
  //p.src = ID(iph->src(),::IP);
  p.dest = ID((Address::instance().get_nodeaddr(iph->daddr())),::IP);
  p.src = ID((Address::instance().get_nodeaddr(iph->saddr())),::IP);

  assert(logtarget != 0);

  if (srh->valid() != 1) {
    unsigned int dst = cmh->next_hop();
    if (dst == IP_BROADCAST) {
      // extensions for mobileIP --Padma, 04/99.
      // Brdcast pkt - treat differently
      if (p.src == net_id)
	// I have originated this pkt
	sendOutBCastPkt(packet);
      else 
	//hand it over to port-dmux
	port_dmux_->recv(packet, (Handler*)0);
      
    } else {
      // this must be an outgoing packet, it doesn't have a SR header on it
      
      srh->init();		 // give packet an SR header now
      cmh->size() += IP_HDR_LEN; // add on IP header size
      if (verbose)
	trace("S %.9f _%s_ originating %s -> %s",
	      Scheduler::instance().clock(), net_id.dump(), p.src.dump(), 
	      p.dest.dump());
      handlePktWithoutSR(p, false);
      goto done;
    }
  }
  else if (srh->valid() == 1) 
    {
      if (p.dest == net_id || p.dest == IP_broadcast)
	{ // this packet is intended for us
	  handlePacketReceipt(p);
	  goto done;
	}
      
      // should we check to see if it's an error packet we're handling
      // and if so call processBrokenRouteError to snoop
      if (dsragent_snoop_forwarded_errors && srh->route_error())
	{
	  processBrokenRouteError(p);
	}

      if (srh->route_request())
	{ // propagate a route_request that's not for us
	  handleRouteRequest(p);
	}
      else
	{ // we're not the intended final recpt, but we're a hop
	  handleForwarding(p);
	}
    }
  else {
    // some invalid pkt has reached here
    fprintf(stderr,"dsragent: Error-received Invalid pkt!\n");
    Packet::free(p.pkt);
    p.pkt =0; // drop silently
  }

 done:
  assert(p.pkt == 0);
  
  p.pkt = 0;
  return;
}


/*===========================================================================
  handlers for each class of packet
---------------------------------------------------------------------------*/
void
DSRAgent::handlePktWithoutSR(SRPacket& p, bool retry)
  /* obtain a source route to p's destination and send it off.
     this should be a retry if the packet is already in the sendbuffer */
{
  assert(HDR_SR (p.pkt)->valid());

  if (p.dest == net_id)
    { // it doesn't need a source route, 'cause it's for us
      handlePacketReceipt(p);
      return;
    }

  // Extensions for wired cum wireless simulation mode
  //if pkt dst outside my subnet, route to base_stn

  ID dest;
  if (diff_subnet(p.dest,net_id)) {
  dest = ID(node_->base_stn(),::IP);
  p.dest = dest;
  }

  if (route_cache->findRoute(p.dest, p.route, 1))
    { // we've got a route...
      if (verbose)
	trace("S$hit %.5f _%s_ %s -> %s %s",
	      Scheduler::instance().clock(), net_id.dump(),
	      p.src.dump(), p.dest.dump(), p.route.dump());      
      sendOutPacketWithRoute(p, true);
      return;
    } // end if we have a route
  else
    { // we don't have a route...
      if (verbose) 
	trace("S$miss %.5f _%s_ %s -> %s", 
	      Scheduler::instance().clock(), net_id.dump(), 
	      net_id.dump(), p.dest.dump());

      getRouteForPacket(p, retry);
      return;
    } // end of we don't have a route
}

void
DSRAgent::handlePacketReceipt(SRPacket& p)
  /* Handle a packet destined to us */
{
  hdr_cmn *cmh =  hdr_cmn::access(p.pkt);
  hdr_sr *srh =  hdr_sr::access(p.pkt);

  if (srh->route_reply())
    { // we got a route_reply piggybacked on a route_request
      // accept the new source route before we do anything else
      // (we'll send off any packet's we have queued and waiting)
      acceptRouteReply(p);
    }
  
  if (srh->route_request())
    {
      if (dsragent_reply_only_to_first_rtreq  && ignoreRouteRequestp(p)) 
	{ //we only respond to the first route request
	  // we receive from a host 
	  Packet::free(p.pkt);     // drop silently
	  p.pkt = 0;
	  return;
	}
      else
	{ // we're going to process this request now, so record the req_num
	  request_table.insert(p.src, p.src, srh->rtreq_seq());
	  returnSrcRouteToRequestor(p);
	}
    }

  if (srh->route_error())
    { // register the dead route      
      processBrokenRouteError(p);
    }

  if (srh->flow_unknown())
    processUnknownFlowError(p, false);

  if (srh->flow_default_unknown())
    processUnknownFlowError(p, true);

  /* give the data in the packet to our higher layer (our port dmuxer, most 
   likely) */
  //handPktToDmux(p);
  assert(p.dest == net_id || p.dest == MAC_id);
  
#if 0
  if (iph->dport() == 255) {
    int mask = Address::instance().portmask();
    int shift = Address::instance().portshift();  
    iph->daddr() = ((iph->dport() & mask) << shift) | ((~(mask) << shift) & iph->dst());
  }
#endif
  
  cmh->size() -= srh->size();	// cut off the SR header 4/7/99 -dam
  srh->valid() = 0;
  cmh->size() -= IP_HDR_LEN;    // cut off IP header size 4/7/99 -dam
  target_->recv(p.pkt, (Handler*)0);
  p.pkt = 0;

}


void
DSRAgent::handleDefaultForwarding(SRPacket &p) {
  hdr_ip *iph = hdr_ip::access(p.pkt);
  u_int16_t flowid;
  int       flowidx;

  if (!flow_table.defaultFlow(p.src.addr, p.dest.addr, flowid)) {
    sendUnknownFlow(p, true);
    assert(p.pkt == 0);
    return;
  }

  if ((flowidx = flow_table.find(p.src.addr, p.dest.addr, flowid)) == -1) {
    sendUnknownFlow(p, false, flowid);
    assert(p.pkt == 0);
    return;
  }

  if (iph->ttl() != flow_table[flowidx].expectedTTL) {
    sendUnknownFlow(p, true);
    assert(p.pkt == 0);
    return;
  }

  // XXX should also check prevhop

  handleFlowForwarding(p, flowidx);
}

void
DSRAgent::handleFlowForwarding(SRPacket &p, int flowidx) {
  hdr_sr *srh = hdr_sr::access(p.pkt);
  hdr_ip *iph = hdr_ip::access(p.pkt);
  hdr_cmn *cmnh =  hdr_cmn::access(p.pkt);
  int amt;

  assert(flowidx >= 0);
  assert(!srh->num_addrs());

  cmnh->next_hop() = flow_table[flowidx].nextHop;
  cmnh->addr_type() = ::IP;

  cmnh->xmit_failure_ = XmitFlowFailureCallback;
  cmnh->xmit_failure_data_ = (void *) this;

  // make sure we aren't cycling packets
  //assert(p.pkt->incoming == 0); // this is an outgoing packet
  assert(cmnh->direction() == hdr_cmn::UP);

  if (!iph->ttl()--) {
    drop(p.pkt, DROP_RTR_TTL);
    p.pkt = 0;
    return;
  }

  trace("SFf %.9f _%s_ %d [%s -> %s] %d to %d", 
	Scheduler::instance().clock(), net_id.dump(), cmnh->uid(),
	p.src.dump(), p.dest.dump(), flow_table[flowidx].flowId,
	flow_table[flowidx].nextHop);

  // XXX ych 5/8/01 ARS also should check previous hop
  if (!srh->salvaged() && 
      (amt = ars_table.findAndClear(cmnh->uid(), flow_table[flowidx].flowId)) &&
      p.route.index() - amt > 0) {
    trace("SFARS %.9f _%s_ %d [%s -> %s] %d %d", 
	  Scheduler::instance().clock(), net_id.dump(), cmnh->uid(),
	  p.src.dump(), p.dest.dump(), flow_table[flowidx].flowId, amt);

    // stamp a route in the packet...
    p.route = flow_table[flowidx].sourceRoute;
    p.route.index() -= amt;
    sendRouteShortening(p, p.route.index(), 
			flow_table[flowidx].sourceRoute.index());
  }

  if (dsragent_always_reestablish) {
    // XXX this is an utter hack. the flow_table needs to remember the original
    // timeout value specified, as well as the original time to timeout. No
    // establishment packets are allowed after the original time. Must make sure
    // flowids assigned do not overlap. ych 5/8/01
    flow_table[flowidx].timeout = Scheduler::instance().clock() + 
				  default_flow_timeout;
  }
  // set the direction pkt to be down
  cmnh->direction() = hdr_cmn::DOWN;
  Scheduler::instance().schedule(ll, p.pkt, 0);
  p.pkt = 0;
}

void
DSRAgent::handleFlowForwarding(SRPacket &p) {
  hdr_sr *srh = hdr_sr::access(p.pkt);
  hdr_ip *iph = hdr_ip::access(p.pkt);
  int flowidx = flow_table.find(p.src.addr, p.dest.addr, srh->flow_id());

  assert(srh->flow_header());

  if (srh->num_addrs()) {
    assert(srh->flow_timeout());

    if (flowidx == -1) {
      flow_table.cleanup();
      flowidx = flow_table.createEntry(p.src.addr, p.dest.addr, srh->flow_id());

      assert(flowidx != -1);

      flow_table[flowidx].timeout = Scheduler::instance().clock() + 
				    srh->flow_timeout_time();
      flow_table[flowidx].hopCount = srh->hopCount();
      flow_table[flowidx].expectedTTL = iph->ttl();
      flow_table[flowidx].sourceRoute = p.route;
      flow_table[flowidx].nextHop = srh->get_next_addr();
      assert(srh->hopCount() == srh->cur_addr());
      assert(srh->get_next_type() == ::IP);
      assert(flow_table[flowidx].sourceRoute[flow_table[flowidx].hopCount] == 
	     net_id);

      flow_table[flowidx].count = 0;            // shouldn't be used
      flow_table[flowidx].allowDefault = false; // shouldn't be used
    }

    assert(flowidx != -1);
    //assert(flow_table[flowidx].hopCount == srh->hopCount());
    
    srh->hopCount()++;
    return;
  }

  if (flowidx == -1) {
    // return an error
    sendUnknownFlow(p, false, srh->flow_id());
    assert(p.pkt == 0);
    return;
  }

  //assert(flow_table[flowidx].hopCount == srh->hopCount());

  srh->hopCount()++;

  // forward the packet
  handleFlowForwarding(p, flowidx);
}

void
DSRAgent::handleForwarding(SRPacket &p)
  /* forward packet on to next host in source route,
   snooping as appropriate */
{
  hdr_sr *srh =  hdr_sr::access(p.pkt);
  hdr_ip *iph = hdr_ip::access(p.pkt);
  hdr_cmn *ch =  hdr_cmn::access(p.pkt);
  bool flowOnly = !srh->num_addrs();

  if (srh->flow_header())
    handleFlowForwarding(p);
  else if (!srh->num_addrs())
    handleDefaultForwarding(p);

  if (flowOnly)
    return;

  assert(p.pkt); // make sure flow state didn't eat the pkt

  // first make sure we are the ``current'' host along the source route.
  // if we're not, the previous node set up the source route incorrectly.
  assert(p.route[p.route.index()] == net_id
	 || p.route[p.route.index()] == MAC_id);

  if (p.route.index() >= p.route.length())
    {
      fprintf(stderr,"dfu: ran off the end of a source route\n");
      trace("SDFU:  ran off the end of a source route\n");
      drop(p.pkt, DROP_RTR_ROUTE_LOOP);
      p.pkt = 0;
      // maybe we should send this packet back as an error...
      return;
    }

  // if there's a source route, maybe we should snoop it too
  if (dsragent_snoop_source_routes)
    route_cache->noticeRouteUsed(p.route, Scheduler::instance().clock(), 
				 net_id);

  // sendOutPacketWithRoute will add in the size of the src hdr, so
  // we have to subtract it out here
  ch->size() -= srh->size();

  // we need to manually decr this, since nothing else does.
  if (!iph->ttl()--) {
    drop(p.pkt, DROP_RTR_TTL);
    p.pkt = 0;
    return;
  }

  // now forward the packet...
  sendOutPacketWithRoute(p, false);
}

void
DSRAgent::handleRouteRequest(SRPacket &p)
  /* process a route request that isn't targeted at us */
{
  hdr_sr *srh =  hdr_sr::access(p.pkt);
  assert (srh->route_request());

#ifdef notdef
  {
          int src = mac_->hdr_src(HDR_MAC(p.pkt));

          if(mac_->is_neighbor(src) == 0) {
                  Packet::free(p.pkt);
                  p.pkt = 0;
                  return;
          }
  }
#endif

  if (ignoreRouteRequestp(p)) 
    {
      if (verbose_srr) 
        trace("SRR %.5f _%s_ dropped %s #%d (ignored)",
              Scheduler::instance().clock(), net_id.dump(), p.src.dump(),
              srh->rtreq_seq());
      Packet::free(p.pkt);  // pkt is a route request we've already processed
      p.pkt = 0;
      return; // drop silently
    }

  // we're going to process this request now, so record the req_num
  request_table.insert(p.src, p.src, srh->rtreq_seq());

  /*  - if it's a Ring 0 search, check the rt$ for a reply and use it if
     possible.  There's not much point in doing Ring 0 search if you're 
     not going to check the cache.  See the comment about turning off all
     reply from cache behavior near the definition of d_r_f_c_o_p (if your 
     workload had really good spatial locality, it might still make 
     sense 'cause your target is probably sitting next to you)
      - if reply from cache is on, check the cache and reply if possible
      - otherwise, just propagate if possible. */
  if ((srh->max_propagation() == 0 || dsragent_reply_from_cache_on_propagating)
      && replyFromRouteCache(p))
	  return;			// all done

#ifdef NEW_REQUEST_LOGIC
  /*
   * If we are congested, don't forward or answer the Route Reply
   */
  if(ifq->prq_length() > 10) {
	  trace("SRR %.9f _%s_ discarding %s #%d (ifq length %d)",
		Scheduler::instance().clock(),
		net_id.dump(),
		p.src.dump(),
		srh->rtreq_seq(),
		ifq->prq_length());
	  Packet::free(p.pkt);
	  p.pkt = 0;
	  return;
  }

  /*
   *  If "free air time" < 15%, don't forward or answer the Route Reply
   */
  {
	  double atime = mac_->air_time_free(10);

	  if(atime > 0.0 && atime < 0.15) {
		  trace("SRR %.9f _%s_ discarding %s #%d (free air time %f)",
			Scheduler::instance().clock(),
			net_id.dump(),
			p.src.dump(),
			srh->rtreq_seq(),
			atime);
		  Packet::free(p.pkt);
		  p.pkt = 0;
		  return;
	  }
  }  
#endif /* NEW_REQUEST_LOGIC */

  // does the orginator want us to propagate?
  if (p.route.length() > srh->max_propagation())
    { 	// no propagation
      if (verbose_srr) 
        trace("SRR %.5f _%s_ dropped %s #%d (prop limit exceeded)",
              Scheduler::instance().clock(), net_id.dump(), p.src.dump(),
              srh->rtreq_seq());
      Packet::free(p.pkt); // pkt isn't for us, and isn't data carrying
      p.pkt = 0;
      return;		
    }

  // can we propagate?
  if (p.route.full())
    { 	// no propagation
      trace("SRR %.5f _%s_ dropped %s #%d (SR full)",
            Scheduler::instance().clock(), net_id.dump(), p.src.dump(),
	    srh->rtreq_seq());
      /* pkt is a rt req, even if data carrying, we don't want to log 
	 the drop using drop() since many nodes could be dropping the 
	 packet in this fashion */
      Packet::free(p.pkt);
      p.pkt = 0;
      return;		
    }

  // add ourselves to the source route
  p.route.appendToPath(net_id);

  if (verbose_srr)
    trace("SRR %.5f _%s_ rebroadcast %s #%d ->%s %s",
          Scheduler::instance().clock(), net_id.dump(), p.src.dump(),
          srh->rtreq_seq(), p.dest.dump(), p.route.dump());

  sendOutPacketWithRoute(p, false);
  return;      
}

/*===========================================================================
  Helpers
---------------------------------------------------------------------------*/
bool
DSRAgent::ignoreRouteRequestp(SRPacket &p)
// should we ignore this route request?
{
  hdr_sr *srh =  hdr_sr::access(p.pkt);

  if (request_table.get(p.src) >= srh->rtreq_seq())
    { // we've already processed a copy of this reqest so
      // we should drop the request silently
      return true;
    }
  if (p.route.member(net_id,MAC_id))
    { // we're already on the route, drop silently
      return true;
    }

  if (p.route.full())
    { // there won't be room for us to put our address into
      // the route
      // so drop silently - sigh, so close, and yet so far...
      // Note that since we don't record the req_id of this message yet,
      // we'll process the request if it gets to us on a shorter path
      return true;
    }
  return false;
}


bool
DSRAgent::replyFromRouteCache(SRPacket &p)
  /* - see if can reply to this route request from our cache
     if so, do it and return true, otherwise, return false 
     - frees or hands off p iff returns true */
{
  Path rest_of_route;
  Path complete_route = p.route;

  /* we shouldn't yet be on on the pkt's current source route */
  assert(!p.route.member(net_id, MAC_id));

  // do we have a cached route the target?
  /* XXX what if we have more than 1?  (and one is legal for reply from
     cache and one isn't?) 1/28/97 -dam */
  if (!route_cache->findRoute(p.dest, rest_of_route, 0))
    { // no route => we're done
      return false;
    }

  /* but we should be on on the remainder of the route (and should be at
   the start of the route */
  assert(rest_of_route[0] == net_id || rest_of_route[0] == MAC_id);

  if (rest_of_route.length() + p.route.length() > MAX_SR_LEN)
    return false; // too long to work with...

  // add our suggested completion to the route so far
  complete_route.appendPath(rest_of_route);

  // call compressPath to remove any double backs
  ::compressPath(complete_route);

  if (!complete_route.member(net_id, MAC_id))
    { // we're not on the suggested route, so we can't return it
      return false;
    }

  // if there is any other information piggybacked into the
  // route request pkt, we need to forward it on to the dst
  hdr_cmn *cmh =  hdr_cmn::access(p.pkt);
  hdr_sr *srh =  hdr_sr::access(p.pkt);
  int request_seqnum = srh->rtreq_seq();
  
  if (PT_DSR != cmh->ptype()	// there's data
      || srh->route_reply()
      || (srh->route_error() && 
	  srh->down_links()[srh->num_route_errors()-1].tell_addr 
	  != GRAT_ROUTE_ERROR))
    { // must forward the packet on
      SRPacket p_copy = p;
      p.pkt = 0;
      srh->route_request() = 0;

      p_copy.route = complete_route;
      p_copy.route.setIterator(p.route.length());

      assert(p.route[p.route.index()] == net_id);
      
      if (verbose) trace("Sdebug %.9f _%s_ splitting %s to %s",
                         Scheduler::instance().clock(), net_id.dump(),
                         p.route.dump(), p_copy.route.dump());

      sendOutPacketWithRoute(p_copy,false);
    }
  else 
    {
      Packet::free(p.pkt);  // free the rcvd rt req before making rt reply
      p.pkt = 0;
    }

  // make up and send out a route reply
  p.route.appendToPath(net_id);
  p.route.reverseInPlace();
  route_cache->addRoute(p.route, Scheduler::instance().clock(), net_id);
  p.dest = p.src;
  p.src = net_id;
  p.pkt = allocpkt();

  hdr_ip *iph =  hdr_ip::access(p.pkt);
  iph->saddr() = Address::instance().create_ipaddr(p.src.addr, RT_PORT);
  iph->sport() = RT_PORT;
  iph->daddr() = Address::instance().create_ipaddr(p.dest.addr, RT_PORT);
  iph->dport() = RT_PORT;
  iph->ttl() = 255;

  srh = hdr_sr::access(p.pkt);
  srh->init();
  for (int i = 0 ; i < complete_route.length() ; i++)
    complete_route[i].fillSRAddr(srh->reply_addrs()[i]);
  srh->route_reply_len() = complete_route.length();
  srh->route_reply() = 1;

  // propagate the request sequence number in the reply for analysis purposes
  srh->rtreq_seq() = request_seqnum;

  hdr_cmn *cmnh =  hdr_cmn::access(p.pkt);
  cmnh->ptype() = PT_DSR;
  cmnh->size() = IP_HDR_LEN;

  if (verbose_srr)
    trace("SRR %.9f _%s_ cache-reply-sent %s -> %s #%d (len %d) %s",
	  Scheduler::instance().clock(), net_id.dump(),
	  p.src.dump(), p.dest.dump(), request_seqnum, complete_route.length(),
	  complete_route.dump());
  sendOutPacketWithRoute(p, true);
  return true;
}


void
DSRAgent::sendOutPacketWithRoute(SRPacket& p, bool fresh, Time delay)
     // take packet and send it out, packet must a have a route in it
     // return value is not very meaningful
     // if fresh is true then reset the path before using it, if fresh
     //  is false then our caller wants us use a path with the index
     //  set as it currently is
{
  hdr_sr *srh =  hdr_sr::access(p.pkt);
  hdr_cmn *cmnh = hdr_cmn::access(p.pkt);

  assert(srh->valid());
  assert(cmnh->size() > 0);

  ID dest;
  if (diff_subnet(p.dest,net_id)) {
  dest = ID(node_->base_stn(),::IP);
  p.dest = dest;
  }

  if (p.dest == net_id)
    { // it doesn't need to go on the wire, 'cause it's for us
      recv(p.pkt, (Handler *) 0);
      p.pkt = 0;
      return;
    }

  if (fresh)
    {
      p.route.resetIterator();
      if (verbose && !srh->route_request())
	{
	  trace("SO %.9f _%s_ originating %s %s", 
		Scheduler::instance().clock(), 
		net_id.dump(), packet_info.name(cmnh->ptype()), p.route.dump());
	}
    }

  p.route.fillSR(srh);

  // set direction of pkt to DOWN , i.e downward
  cmnh->direction() = hdr_cmn::DOWN;

  // let's see if we can snag this packet for flow state... ych 5/2/01
  if (dsragent_enable_flowstate &&
      p.src == net_id && !srh->route_request() && !srh->cur_addr() &&
      // can't yet decode flow errors and route errors/replies together
      // so don't tempt the system... ych 5/7/01
      !srh->route_error() && !srh->route_reply()) {
    hdr_ip *iph =  hdr_ip::access(p.pkt);
    int flowidx;
    u_int16_t flowid, default_flowid;
    double now = Scheduler::instance().clock();

    // hmmm, let's see if we can save us some overhead...
    if (dsragent_prefer_default_flow &&
	flow_table.defaultFlow(p.src.addr, p.dest.addr, flowid) &&
	-1 != (flowidx = flow_table.find(p.src.addr, p.dest.addr, flowid)) &&
	flow_table[flowidx].timeout >= now &&
	(!dsragent_prefer_shorter_over_default || 
	  flow_table[flowidx].sourceRoute.length() <= p.route.length()) &&
	!(p.route == flow_table[flowidx].sourceRoute)) {

      p.route = flow_table[flowidx].sourceRoute;
      p.route.fillSR(srh);
    }

    flowidx = flow_table.find(p.src.addr, p.dest.addr, p.route);

    if (flowidx == -1 || flow_table[flowidx].timeout < now) {
      // I guess we don't know about this flow; allocate it.
      flow_table.cleanup();
      flowid = flow_table.generateNextFlowId(p.dest.addr, true);
      flowidx = flow_table.createEntry(p.src.addr, p.dest.addr, flowid);
      assert(flowidx != -1);

      // fill out the table
      flow_table[flowidx].count = 1;
      flow_table[flowidx].lastAdvRt = Scheduler::instance().clock();
      flow_table[flowidx].timeout = now + default_flow_timeout;
      flow_table[flowidx].hopCount = 0;
      flow_table[flowidx].expectedTTL = iph->ttl();
      flow_table[flowidx].allowDefault = true;
      flow_table[flowidx].sourceRoute = p.route;
      flow_table[flowidx].nextHop = srh->get_next_addr();
      assert(srh->get_next_type() == ::IP);

      // fix up the srh for the timeout
      srh->flow_timeout() = 1;
      srh->flow_timeout_time() = default_flow_timeout;
      srh->cur_addr() = srh->cur_addr() + 1;
    } else if (flow_table[flowidx].count <= END_TO_END_COUNT ||
		flow_table[flowidx].lastAdvRt < 
		   (Scheduler::instance().clock() - min_adv_interval)) {
      // I've got it, but maybe someone else doesn't
      if (flow_table[flowidx].expectedTTL != iph->ttl())
	flow_table[flowidx].allowDefault = false;

      flow_table[flowidx].count++;
      flow_table[flowidx].lastAdvRt = Scheduler::instance().clock();

      srh->flow_timeout() = 1;
      if (dsragent_always_reestablish)
	srh->flow_timeout_time() = default_flow_timeout;
      else
	srh->flow_timeout_time() = (int)(flow_table[flowidx].timeout - now);
      srh->cur_addr() = srh->cur_addr() + 1;
    } else {
      // flow is established end to end
      assert (flow_table[flowidx].sourceRoute == p.route);
      srh->flow_timeout() = srh->cur_addr() = srh->num_addrs() = 0;
    }

    if (dsragent_always_reestablish) {
      // XXX see major problems detailed above (search for dsragent_always_re..)
      flow_table[flowidx].timeout = now + default_flow_timeout;
    }

    cmnh->next_hop() = flow_table[flowidx].nextHop;
    cmnh->addr_type() = ::IP;

    if (flow_table.defaultFlow(p.src.addr, p.dest.addr, default_flowid) &&
	flow_table[flowidx].flowId == default_flowid &&
	!srh->num_addrs() && iph->ttl() == flow_table[flowidx].expectedTTL &&
	flow_table[flowidx].allowDefault) {
      // we can go without anything... woo hoo!
      assert(!srh->flow_header());
    } else {
      srh->flow_header() = 1;
      srh->flow_id() = flow_table[flowidx].flowId;
      srh->hopCount() = 1;
    }

    trace("SF%ss %.9f _%s_ %d [%s -> %s] %d(%d) to %d %s", 
	srh->num_addrs() ? "EST" : "",
	Scheduler::instance().clock(), net_id.dump(), cmnh->uid(),
	p.src.dump(), p.dest.dump(), flow_table[flowidx].flowId,
	srh->flow_header(), flow_table[flowidx].nextHop,
	srh->num_addrs() ? srh->dump() : "");

    cmnh->size() += srh->size();
    cmnh->xmit_failure_ = srh->num_addrs() ? XmitFailureCallback : 
					     XmitFlowFailureCallback;
    cmnh->xmit_failure_data_ = (void *) this;

    assert(!srh->num_addrs() || srh->flow_timeout());
  } else {
    // old non-flowstate stuff...
    assert(p.src != net_id || !srh->flow_header());
    cmnh->size() += srh->size();

    if (srh->route_request())
      { // broadcast forward
        cmnh->xmit_failure_ = 0;
        cmnh->next_hop() = MAC_BROADCAST;
        cmnh->addr_type() = NS_AF_ILINK;
      }
    else
      { // forward according to source route
        cmnh->xmit_failure_ = XmitFailureCallback;
        cmnh->xmit_failure_data_ = (void *) this;

        cmnh->next_hop() = srh->get_next_addr();
        cmnh->addr_type() = srh->get_next_type();
        srh->cur_addr() = srh->cur_addr() + 1;
      } /* route_request() */
  } /* can snag for path state */

  /* put route errors at the head of the ifq somehow? -dam 4/13/98 */

  // make sure we aren't cycling packets
  
#ifdef notdef
  if (ifq->prq_length() > 25)
	  trace("SIFQ %.5f _%s_ len %d",
		Scheduler::instance().clock(),
		net_id.dump(), ifq->prq_length());
#endif
#ifdef NEW_IFQ_LOGIC
  /*
   *  If the interface queue is full, there's no sense in sending
   *  the packet.  Drop it and generate a Route Error?
   */
  /* question for the author: this seems rife with congestion/infinite loop
   * possibilities. you're responding to an ifq full by sending a rt err.
   * sounds like the source quench problem. ych 5/5/01
   */
  if(ifq->prq_isfull(p.pkt)) {
	  xmitFailed(p.pkt, DROP_IFQ_QFULL);
	  p.pkt = 0;
	  return;
  }
#endif /* NEW_IFQ_LOGIC */

  // ych debugging
  assert(!srh->flow_header() || !srh->num_addrs() || srh->flow_timeout());

  // off it goes!
  if (srh->route_request())
    { // route requests need to be jittered a bit
      Scheduler::instance().schedule(ll, p.pkt, 
				     Random::uniform(RREQ_JITTER) + delay);
    }
  else
    { // no jitter required 
      Scheduler::instance().schedule(ll, p.pkt, delay);
    }
  p.pkt = NULL; /* packet sent off */
}

void
DSRAgent::getRouteForPacket(SRPacket &p, bool retry)
  /* try to obtain a route for packet
     pkt is freed or handed off as needed, unless retry == true
     in which case it is not touched */
{
  // since we'll commonly be only one hop away, we should
  // arp first before route discovery as an optimization...

  Entry *e = request_table.getEntry(p.dest);
  Time time = Scheduler::instance().clock();

#if 0
  /* pre 4/13/98 logic -dam removed b/c it seemed more complicated than
     needed since we're not doing piggybacking and we're returning
     route replies via a reversed route (the copy in this code is
     critical if we need to piggyback route replies on the route request to
     discover the return path) */

  /* make the route request packet */
  SRPacket rrp = p;
  rrp.pkt = p.pkt->copy();
  hdr_sr *srh = hdr_sr::access(rrp.pkt);
  hdr_ip *iph = hdr_ip::access(rrp.pkt);
  hdr_cmn *cmh =  hdr_cmn::access(rrp.pkt);
  //iph->daddr() = p.dest.getNSAddr_t();
  iph->daddr() = Address::instance().create_ipaddr(p.dest.getNSAddr_t(),RT_PORT);
  iph->dport() = RT_PORT;
  //iph->saddr() = net_id.getNSAddr_t();
  iph->saddr() = Address::instance().create_ipaddr(net_id.getNSAddr_t(),RT_PORT);
  iph->sport() = RT_PORT;
  cmnh->ptype() = PT_DSR;
  cmnh->size() = size_;
  cmnh->num_forwards() = 0;
#endif

  /* make the route request packet */
  SRPacket rrp;
  rrp.dest = p.dest;
  rrp.src = net_id;
  rrp.pkt = allocpkt();

  hdr_sr *srh = hdr_sr::access(rrp.pkt); 
  hdr_ip *iph = hdr_ip::access(rrp.pkt);
  hdr_cmn *cmnh =  hdr_cmn::access(rrp.pkt);
  
  iph->daddr() = Address::instance().create_ipaddr(p.dest.getNSAddr_t(),RT_PORT);
  iph->dport() = RT_PORT;
  iph->saddr() = Address::instance().create_ipaddr(net_id.getNSAddr_t(),RT_PORT);
  iph->sport() = RT_PORT;
  cmnh->ptype() = PT_DSR;
  cmnh->size() = size_ + IP_HDR_LEN; // add in IP header
  cmnh->num_forwards() = 0;
  
  srh->init();


  if (BackOffTest(e, time)) {
	  // it's time to start another route request cycle

#ifdef NEW_SALVAGE_LOGIC
	  if(p.src != net_id) {

		  assert(dsr_salvage_max_requests > 0);
		  assert(p.pkt);

		  if(e->rt_reqs_outstanding > dsr_salvage_max_requests) {
			  drop(p.pkt, DROP_RTR_NO_ROUTE);
			  p.pkt = 0;

			  // dump the route request packet we made up
			  Packet::free(rrp.pkt);
			  rrp.pkt = 0;

			  return;
		  }
	  }
#endif /* NEW_SALVAGE_LOGIC */

	  if (dsragent_ring_zero_search) {
		  // do a ring zero search
		  e->last_type = LIMIT0;
		  sendOutRtReq(rrp, 0);
	  } else {
		  // do a propagating route request right now
		  e->last_type = UNLIMIT;
		  sendOutRtReq(rrp, MAX_SR_LEN);
	  }

	  e->last_arp = time;
  }  else if (LIMIT0 == e->last_type &&
#ifdef NEW_SALVAGE_LOGIC
	      (dsr_salvage_allow_propagating || p.src == net_id) &&
#endif
	   (time - e->last_arp) > arp_timeout) {
	  // try propagating rt req since we haven't heard back
	  // from limited one

	  e->last_type = UNLIMIT;
	  sendOutRtReq(rrp, MAX_SR_LEN);
  }
  else {
	  // it's not time to send another route request...
	  if (!retry && verbose_srr)
		  trace("SRR %.5f _%s_ RR-not-sent %s -> %s", 
			Scheduler::instance().clock(), 
			net_id.dump(), rrp.src.dump(), rrp.dest.dump());
	  Packet::free(rrp.pkt); // dump the route request packet we made up
	  rrp.pkt = 0;
  }

  /* for now, no piggybacking at all, queue all pkts */
  if (!retry) {
	  stickPacketInSendBuffer(p);
	  p.pkt = 0; // pkt is handled for now (it's in sendbuffer)
  }

}

void
DSRAgent::sendOutRtReq(SRPacket &p, int max_prop)
  // turn p into a route request and launch it, max_prop of request is
  // set as specified
  // p.pkt is freed or handed off
{
  hdr_sr *srh =  hdr_sr::access(p.pkt);
  assert(srh->valid());

  srh->route_request() = 1;
  srh->rtreq_seq() = route_request_num++;
  srh->max_propagation() = max_prop;
  p.route.reset();
  p.route.appendToPath(net_id);

  if (dsragent_propagate_last_error && route_error_held 
      && Scheduler::instance().clock() - route_error_data_time  < max_err_hold)
    {
      assert(srh->num_route_errors() < MAX_ROUTE_ERRORS);
      srh->route_error() = 1;
      link_down *deadlink = &(srh->down_links()[srh->num_route_errors()]);
      deadlink->addr_type = NS_AF_INET;
      deadlink->from_addr = err_from.getNSAddr_t();
      deadlink->to_addr = err_to.getNSAddr_t();
      deadlink->tell_addr = GRAT_ROUTE_ERROR;
      srh->num_route_errors() += 1;
      /*
       * Make sure that the Route Error gets on a propagating request.
       */
      if(max_prop > 0) route_error_held = false;
    }

  if (verbose_srr)
    trace("SRR %.5f _%s_ new-request %d %s #%d -> %s", 
	  Scheduler::instance().clock(), net_id.dump(), 
	  max_prop, p.src.dump(), srh->rtreq_seq(), p.dest.dump());
  sendOutPacketWithRoute(p, false);
}

void
DSRAgent::returnSrcRouteToRequestor(SRPacket &p)
  // take the route in p, add us to the end of it and return the
  // route to the sender of p
  // doesn't free p.pkt
{
  hdr_sr *old_srh = hdr_sr::access(p.pkt);

  if (p.route.full()) 
    return; // alas, the route would be to long once we add ourselves

  SRPacket p_copy = p;
  p_copy.pkt = allocpkt();
  p_copy.dest = p.src;
  p_copy.src = net_id;

  p_copy.route.appendToPath(net_id);

  hdr_ip *new_iph =  hdr_ip::access(p_copy.pkt);
  //new_iph->daddr() = p_copy.dest.addr;
  new_iph->daddr() = Address::instance().create_ipaddr(p_copy.dest.getNSAddr_t(),RT_PORT);
  new_iph->dport() = RT_PORT;
  //new_iph->saddr() = p_copy.src.addr;
  new_iph->saddr() =
    Address::instance().create_ipaddr(p_copy.src.getNSAddr_t(),RT_PORT); 
  new_iph->sport() = RT_PORT;
  new_iph->ttl() = 255;

  hdr_sr *new_srh =  hdr_sr::access(p_copy.pkt);
  new_srh->init();
  for (int i = 0 ; i < p_copy.route.length() ; i++)
    p_copy.route[i].fillSRAddr(new_srh->reply_addrs()[i]);
  new_srh->route_reply_len() = p_copy.route.length();
  new_srh->route_reply() = 1;

  // propagate the request sequence number in the reply for analysis purposes
  new_srh->rtreq_seq() = old_srh->rtreq_seq();
  
  hdr_cmn *new_cmnh =  hdr_cmn::access(p_copy.pkt);
  new_cmnh->ptype() = PT_DSR;
  new_cmnh->size() = IP_HDR_LEN;

  if (verbose_srr)
    trace("SRR %.9f _%s_ reply-sent %s -> %s #%d (len %d) %s",
	  Scheduler::instance().clock(), net_id.dump(),
	  p_copy.src.dump(), p_copy.dest.dump(), old_srh->rtreq_seq(),
	  p_copy.route.length(), p_copy.route.dump());

  // flip the route around for the return to the requestor, and 
  // cache the route for future use
  p_copy.route.reverseInPlace();
  route_cache->addRoute(p_copy.route, Scheduler::instance().clock(), net_id);

  p_copy.route.resetIterator();
  p_copy.route.fillSR(new_srh);
  new_cmnh->size() += new_srh->size();
  
  /* we now want to jitter when we first originate route replies, since
     they are a transmission we make in response to a broadcast packet 
     -dam 4/23/98
     sendOutPacketWithRoute(p_copy, true); */
  {
	  double d = Random::uniform(RREQ_JITTER);
#if 0
	  fprintf(stderr, "Random Delay: %f\n", d);
#endif
	  Scheduler::instance().schedule(this, p_copy.pkt, d);
  }
}

int
DSRAgent::diff_subnet(ID dest, ID myid) 
{
	int dst = dest.addr;
	int id = myid.addr;
	char* dstnet = Address::instance().get_subnetaddr(dst);
	char * subnet = Address::instance().get_subnetaddr(id);
	if (subnet != NULL) {
		if (dstnet != NULL) {
			if (strcmp(dstnet, subnet) != 0) {
				delete [] dstnet;
				return 1;
			}
			delete [] dstnet;
		}
		delete [] subnet;
	}
	assert(dstnet == NULL);
	return 0;
}


void
DSRAgent::acceptRouteReply(SRPacket &p)
  /* - enter the packet's source route into our cache
     - see if any packets are waiting to be sent out with this source route
     - doesn't free the pkt */
{
  hdr_sr *srh =  hdr_sr::access(p.pkt);
  Path reply_route(srh->reply_addrs(), srh->route_reply_len());

  if (!srh->route_reply())
    { // somethings wrong...
      trace("SDFU non route containing packet given to acceptRouteReply");
      fprintf(stderr,
	      "dfu: non route containing packet given to acceptRouteReply\n");
    }

  bool good_reply = true;  
  //#ifdef USE_GOD_FEEDBACK
  /* check to see if this reply is valid or not using god info */
  int i;
  
  for (i = 0; i < reply_route.length()-1 ; i++) 
    if (God::instance()->hops(reply_route[i].getNSAddr_t(), 
			      reply_route[i+1].getNSAddr_t()) != 1)
      {
	good_reply = false;
	break;
      }
  //#endif //GOD_FEEDBACK

  if (verbose_srr)
    trace("SRR %.9f _%s_ reply-received %d from %s  %s #%d -> %s %s",
	  Scheduler::instance().clock(), net_id.dump(),
	  good_reply ? 1 : 0,
	  p.src.dump(), reply_route[0].dump(), srh->rtreq_seq(),
	  reply_route[reply_route.length()-1].dump(),
	  reply_route.dump());

  // add the new route into our cache
  route_cache->addRoute(reply_route, Scheduler::instance().clock(), p.src);

  // back down the route request counters
  Entry *e = request_table.getEntry(reply_route[reply_route.length()-1]);
  e->rt_reqs_outstanding = 0;
  e->last_rt_req = 0.0;	 

  // see if the addtion of this route allows us to send out
  // any of the packets we have waiting
  Time delay = 0.0;
  ID dest;
  for (int c = 0; c < SEND_BUF_SIZE; c++)
    {
      if (send_buf[c].p.pkt == NULL) continue;

      // check if pkt is destined to outside domain
      if (diff_subnet(send_buf[c].p.dest,net_id)) {
	dest = ID(node_->base_stn(),::IP);
	send_buf[c].p.dest = dest;
      }

      if (route_cache->findRoute(send_buf[c].p.dest, send_buf[c].p.route, 1))
	{ // we have a route!
#ifdef DEBUG
	  struct hdr_cmn *ch = HDR_CMN(send_buf[c].p.pkt);
	  if(ch->size() < 0) {
		drop(send_buf[c].p.pkt, "XXX");
		abort();
	  }
#endif
	  if (verbose)
	    trace("Sdebug %.9f _%s_ liberated from sendbuf %s->%s %s",
		  Scheduler::instance().clock(), net_id.dump(),
		  send_buf[c].p.src.dump(), send_buf[c].p.dest.dump(), 
		  send_buf[c].p.route.dump());
	  /* we need to spread out the rate at which we send packets
	     in to the link layer to give ARP time to complete.  If we
	     dump all the packets in at once, all but the last one will
	     be dropped.  XXX THIS IS A MASSIVE HACK -dam 4/14/98 */
	  sendOutPacketWithRoute(send_buf[c].p, true, delay);
	  delay += arp_timeout;	
	  send_buf[c].p.pkt = NULL;
	}
    }
}

void
DSRAgent::processUnknownFlowError(SRPacket &p, bool asDefault) {
  hdr_sr *srh = hdr_sr::access(p.pkt);
  int flowidx = -1;
  struct flow_error *fe;
  u_int16_t flowid;

  if (asDefault) {
    assert (srh->flow_default_unknown() && srh->num_default_unknown());
    fe = &srh->unknown_defaults()[srh->num_default_unknown()-1];
  } else {
    assert (srh->flow_unknown() && srh->num_flow_unknown());
    fe = &srh->unknown_flows()[srh->num_flow_unknown()-1];
    if (!flow_table.defaultFlow(fe->flow_src, fe->flow_dst, flowid))
      goto skip_proc;
  }

  /* not for us; hope it gets the right place... */
  if (fe->flow_src != (int) net_id.addr)
    return;

  if (-1 != (flowidx = flow_table.find(fe->flow_src, fe->flow_dst, 
				       asDefault ? flowid : fe->flow_id)))
    flow_table[flowidx].count = 0;

skip_proc:
  trace("SFEr %.9f _%s_ from %d re %d : %d [%d]",
	Scheduler::instance().clock(), net_id.dump(), p.src.addr, fe->flow_dst,
	asDefault ? -1 : fe->flow_id, 
	flowidx != -1 ? flow_table[flowidx].count : -1);

  if ((asDefault ? srh->num_default_unknown() : srh->num_flow_unknown()) == 1)
    return;

  SRPacket p_copy = p;
  p_copy.pkt = p.pkt->copy();

  hdr_sr *new_srh = hdr_sr::access(p_copy.pkt);
  hdr_ip *new_iph = hdr_ip::access(p_copy.pkt);
  
  // remove us from the list of errors
  if (asDefault)
    new_srh->num_default_unknown()--;
  else
    new_srh->num_flow_unknown()--;
  
  // send the packet to the person listed in what's now the last entry
  p_copy.dest = ID(fe[-1].flow_src, ::IP);
  p_copy.src = net_id;

  //new_iph->daddr() = p_copy.dest.addr;
  new_iph->daddr() = Address::instance().create_ipaddr(p_copy.dest.getNSAddr_t(),RT_PORT);
  new_iph->dport() = RT_PORT;
  //new_iph->saddr() = p_copy.src.addr;
  new_iph->saddr() = Address::instance().create_ipaddr(p_copy.src.getNSAddr_t(),RT_PORT);
  new_iph->sport() = RT_PORT;
  new_iph->ttl() = 255;

  new_srh->flow_header() = 0;
  new_srh->flow_timeout() = 0;

  // an error packet is a first class citizen, so we'll
  // use handlePktWOSR to obtain a route if needed
  handlePktWithoutSR(p_copy, false);
}

void
DSRAgent::processBrokenRouteError(SRPacket& p)
// take the error packet and proccess our part of it.
// if needed, send the remainder of the errors to the next person
// doesn't free p.pkt
{
  hdr_sr *srh = hdr_sr::access(p.pkt);

  if (!srh->route_error())
    return; // what happened??
  
  /* if we hear A->B is dead, should we also run the link B->A through the
     cache as being dead, since 802.11 requires bidirectional links 
      XXX -dam 4/23/98 */

  // since CPU time is cheaper than network time, we'll process
  // all the dead links in the error packet
  assert(srh->num_route_errors() > 0);
  for (int c = 0 ; c < srh->num_route_errors() ; c++)
    {
      assert(srh->down_links()[c].addr_type == NS_AF_INET);
      route_cache->noticeDeadLink(ID(srh->down_links()[c].from_addr,::IP),
				 ID(srh->down_links()[c].to_addr,::IP),
				 Scheduler::instance().clock());
      flow_table.noticeDeadLink(ID(srh->down_links()[c].from_addr,::IP),
				 ID(srh->down_links()[c].to_addr,::IP));
      // I'll assume everything's of type NS_AF_INET for the printout... XXX
      if (verbose_srr)
        trace("SRR %.9f _%s_ dead-link tell %d  %d -> %d",
              Scheduler::instance().clock(), net_id.dump(),
              srh->down_links()[c].tell_addr,
              srh->down_links()[c].from_addr,
              srh->down_links()[c].to_addr);
    }

  ID who = ID(srh->down_links()[srh->num_route_errors()-1].tell_addr, ::IP);
  if (who != net_id && who != MAC_id)
    { // this error packet wasn't meant for us to deal with
      // since the outer entry doesn't list our name
      return;
    }

  // record this route error data for possible propagation on our next
  // route request
  route_error_held = true;
  err_from = ID(srh->down_links()[srh->num_route_errors()-1].from_addr,::IP);
  err_to = ID(srh->down_links()[srh->num_route_errors()-1].to_addr,::IP);
  route_error_data_time = Scheduler::instance().clock();

  if (1 == srh->num_route_errors())
    { // this error packet has done its job
      // it's either for us, in which case we've done what it sez
      // or it's not for us, in which case we still don't have to forward
      // it to whoever it is for
      return;
    }

  /* make a copy of the packet and send it to the next tell_addr on the
     error list.  the copy is needed in case there is other data in the
     packet (such as nested route errors) that need to be delivered */
  if (verbose) 
    trace("Sdebug %.5f _%s_ unwrapping nested route error",
          Scheduler::instance().clock(), net_id.dump());
  
  SRPacket p_copy = p;
  p_copy.pkt = p.pkt->copy();

  hdr_sr *new_srh = hdr_sr::access(p_copy.pkt);
  hdr_ip *new_iph = hdr_ip::access(p_copy.pkt);
  
  // remove us from the list of errors
  new_srh->num_route_errors() -= 1;
  
  // send the packet to the person listed in what's now the last entry
  p_copy.dest = ID(new_srh->down_links()[new_srh->num_route_errors()-1].tell_addr, ::IP);
  p_copy.src = net_id;

  //new_iph->daddr() = p_copy.dest.addr;
  new_iph->daddr() = Address::instance().create_ipaddr(p_copy.dest.getNSAddr_t(),RT_PORT);
  new_iph->dport() = RT_PORT;
  //new_iph->saddr() = p_copy.src.addr;
  new_iph->saddr() = Address::instance().create_ipaddr(p_copy.src.getNSAddr_t(),RT_PORT);
  new_iph->sport() = RT_PORT;
  new_iph->ttl() = 255;

  new_srh->flow_header() = 0;
  new_srh->flow_timeout() = 0;
      
  // an error packet is a first class citizen, so we'll
  // use handlePktWOSR to obtain a route if needed
  handlePktWithoutSR(p_copy, false);
}

#ifdef DSR_FILTER_TAP
int64_t dsr_tap = 0;
int64_t dsr_tap_skip = 0;
#endif

// Process flow state Automatic Route Shortening
void
DSRAgent::processFlowARS(const Packet *packet) {
  
  hdr_sr *srh = hdr_sr::access(packet);
  hdr_ip *iph = hdr_ip::access(packet);
  hdr_cmn *cmh = hdr_cmn::access(packet);
  //hdr_sr  *srh = (hdr_sr*) ((Packet *)packet)->access(off_sr_);
  //hdr_ip  *iph = (hdr_ip*) ((Packet *)packet)->access(off_ip_);
  //hdr_cmn *cmh =  (hdr_cmn*)((Packet *)packet)->access(off_cmn_);
  u_int16_t flowid;
  int flowidx;
  int shortamt;

  assert(!srh->num_addrs());

  if (srh->flow_header()) {
    flowid = srh->flow_id();

    // do I know about this flow?
    if (-1 == (flowidx = flow_table.find(iph->saddr(), iph->daddr(), flowid)))
      return;

    shortamt = flow_table[flowidx].hopCount - srh->hopCount();
  } else {
    // do I know which flow is default?
    if (!flow_table.defaultFlow(iph->saddr(), iph->daddr(), flowid))
      return;

    // do I know about this flow?
    if (-1 == (flowidx = flow_table.find(iph->saddr(), iph->daddr(), flowid)))
      return;

    shortamt = iph->ttl() - flow_table[flowidx].expectedTTL;
  }

  // transmitter downstream from us
  if (shortamt <= 0)
    return;

  // this is a _MAJOR_ problem!!!
  if (flow_table[flowidx].sourceRoute.length() < shortamt)
    return;

  ars_table.insert(cmh->uid(), flowid, shortamt);
}

void 
DSRAgent::tap(const Packet *packet)
  /* process packets that are promiscously listened to from the MAC layer tap
  *** do not change or free packet *** */
{
  hdr_sr *srh = hdr_sr::access(packet);
  hdr_ip *iph = hdr_ip::access(packet);
  hdr_cmn *cmh =  hdr_cmn::access(packet);
  
  if (!dsragent_use_tap) return;

  if (!srh->valid()) return;	// can't do anything with it

  if (!srh->num_addrs()) {
    processFlowARS(packet);
    return;
  }

  // don't trouble me with packets I'm about to receive anyway
  /* this change added 5/13/98 -dam */
  ID next_hop(srh->addrs()[srh->cur_addr()]);
  if (next_hop == net_id || next_hop == MAC_id) return;

  SRPacket p((Packet *) packet, srh);
  //p.dest = ID(iph->dst(),::IP);
  //p.src = ID(iph->src(),::IP);
  p.dest = ID((Address::instance().get_nodeaddr(iph->daddr())),::IP);
  p.src = ID((Address::instance().get_nodeaddr(iph->saddr())),::IP);

  // don't trouble me with my own packets
  if (p.src == net_id) return; 

#ifdef DSR_FILTER_TAP
  /* 
   * Don't process packets more than once.  In real implementations
   * this can be done with the (IP Source, IP ID) pair, but it is
   * simpler to implement it with the global "uid" in simulation.
   */
  {
          int uid = cmh->uid();
          if(tap_uid_cache[(uid & TAP_BITMASK)] == uid) {
		  dsr_tap_skip++;
                  return;
	  }
	  dsr_tap++;
          tap_uid_cache[(uid & TAP_BITMASK)] = uid;
  }
#endif

  /* snoop on the SR data */
  if (srh->route_error())
    {
      if (verbose)
	trace("Sdebug _%s_ tap saw error %d",  net_id.dump(), cmh->uid());
      processBrokenRouteError(p);
    }

  if (srh->route_reply())
    {
      Path reply_path(srh->reply_addrs(), srh->route_reply_len());
      if(verbose)
	trace("Sdebug _%s_ tap saw route reply %d  %s",
	       net_id.dump(), cmh->uid(), reply_path.dump());
      route_cache->noticeRouteUsed(reply_path, Scheduler::instance().clock(), 
				   p.src);
    }

  /* we can't decide whether we should snoop on the src routes in 
     route requests.  We've seen cases where we hear a route req from a
     node, but can't complete an arp with that node (and so can't actually
     deliver packets through it if called on to do so) -dam 4/16/98 */

  if (srh->route_request()) return; // don't path shorten route requests
  // the logic is wrong for shortening rtreq's anyway, cur_addr always = 0

  if (dsragent_snoop_source_routes)
    {
      if (verbose)
	trace("Sdebug _%s_ tap saw route use %d %s", net_id.dump(), 
	      cmh->uid(), p.route.dump());
      route_cache->noticeRouteUsed(p.route, Scheduler::instance().clock(), 
				   net_id);
    }

  if (PT_DSR == cmh->ptype()) return; //  no route shortening on any
  // DSR packet

  /* I think we ended up sending grat route replies for source routes on 
     route replies for route requests that were answered by someone else's
     cache, resulting in the wrong node receiving the route.  For now, I 
     outlaw it.

     The root of the problem is that when we salvage a pkt from a failed
     link using a route from our cache, we break what had been an invariant
     that the IP src of a packet was also the first machine listed on the
     source route.  Here's the route of the problem that results in the 
     simulator crashing at 8.56135 when 44 recieves a route reply that
     has 24 listed as the first node in the route.

SSendFailure 8.52432 24 [10 |24 46 45 1 40 ]
S$hit 8.52432 salvaging 10 -> 40 with [(24) 44 50 9 40 ]
S$hit 8.52432 salvaging 44 -> 40 with [(24) 44 50 9 40 ]
D 8.52432 [20 42 2e 18 800] 24 DSR 156 -- 10->40 6 [0] [1 9 39] [0 0 0->0]
s 8.52438 [1b 45e 2c 18 0] 24 MAC 20
r 8.52446 [1b 45e 2c 18 0] 44 MAC 20
s 8.52454 [101b 27e 23 1b 0] 27 MAC 20
s 8.52564 [101b 27e 23 1b 0] 27 MAC 20
s 8.52580 [101b 45e 2c 18 0] 24 MAC 20
r 8.52588 [101b 45e 2c 18 0] 44 MAC 20
s 8.52589 [1c 41c 18 0 0] 44 MAC 14
r 8.52595 [1c 41c 18 0 0] 24 MAC 14
s 8.52600 [20 42 2c 18 800] 24 DSR 244 -- 10->40 5 [0] [1 9 39] [0 0 24->46]
r 8.52698 [20 42 2c 18 800] 44 DSR 216 -- 10->40 5 [0] [1 9 39] [0 0 24->46]

s 8.53947 [20 42 2c 18 800] 24 DSR 204 -- 44->40 5 [0] [1 8 39] [0 0 0->0]
r 8.54029 [20 42 2c 18 800] 44 DSR 176 -- 44->40 5 [0] [1 8 39] [0 0 0->0]
Sdebug 50 consider grat arp for [24 (44) 50 9 40 ]
SRR 8.54029 50 gratuitous-reply-sent 50 -> 44 [24 (50) 9 40 ]
SF 8.54029 44 [44 -> 40] via 0x3200 [24 |44 50 9 40 ]
s 8.54030 [1d 0 18 0 0] 44 MAC 14
r 8.54036 [1d 0 18 0 0] 24 MAC 14
s 8.54044 [101b 54f 32 2c 0] 44 MAC 20
r 8.54053 [101b 54f 32 2c 0] 50 MAC 20
s 8.54054 [1c 50d 2c 0 0] 50 MAC 14
r 8.54059 [1c 50d 2c 0 0] 44 MAC 14
s 8.54064 [20 42 32 2c 800] 44 DSR 304 -- 10->40 5 [0] [1 9 39] [0 0 24->46]
r 8.54186 [20 42 32 2c 800] 50 DSR 276 -- 10->40 5 [0] [1 9 39] [0 0 24->46]
SF 8.54186 50 [10 -> 40] via 0x900 [24 44 |50 9 40 ]

s 8.56101 [20 42 2c 18 800] 24 DSR 84 -- 50->44 2 [0] [1 4 40] [0 0 0->0]
r 8.56135 [20 42 2c 18 800] 44 DSR 56 -- 50->44 2 [0] [1 4 40] [0 0 0->0]

*/


  /* check to see if we can shorten the route being used */
  if (p.route[p.route.index()] != net_id
      && p.route[p.route.index()] != MAC_id)
    { // it's not immeadiately to us
      for (int i = p.route.index() + 1; i < p.route.length(); i++)
	if (p.route[i] == net_id || p.route[i] == MAC_id)
	  { // but it'll get here eventually...
	    sendRouteShortening(p, p.route.index(), i);
	  }
    }
}

static GratReplyHoldDown *
FindGratHoldDown(GratReplyHoldDown *hd, int sz, Path& query)
{
  int c;
  for (c = 0; c < sz; c++)
    if (query == hd[c].p) return &hd[c];
  return NULL;
}

void
DSRAgent::sendRouteShortening(SRPacket &p, int heard_at, int xmit_at)
  // p was overheard at heard_at in it's SR, but we aren't supposed to
  // get it till xmit_at, so all the nodes between heard_at and xmit_at
  // can be elided.  Send originator of p a gratuitous route reply to 
  // tell them this.
{
  // this shares code with returnSrcRouteToRequestor - factor them -dam */

  if (!dsragent_send_grat_replies) return;

  if (verbose)
    trace("Sdebug %s consider grat arp for %s", net_id.dump(), p.route.dump());
  GratReplyHoldDown *g = FindGratHoldDown(grat_hold, RTREP_HOLDOFF_SIZE, 
					  p.route);
  if (!g)
    { 
      grat_hold[grat_hold_victim].p = p.route;
      grat_hold_victim = (grat_hold_victim + 1) % RTREP_HOLDOFF_SIZE;
      g = &grat_hold[grat_hold_victim];      
    }
  else if (Scheduler::instance().clock() - g->t < grat_hold_down_time) return;
  g->t = Scheduler::instance().clock();

  SRPacket p_copy = p;
  p_copy.pkt = allocpkt();
  p_copy.dest = p.route[0];   // tell the originator of this long source route
  p_copy.src = net_id;

  // reverse the route to get the packet back
  p_copy.route[p_copy.route.index()] = net_id;
  p_copy.route.reverseInPlace();
  p_copy.route.removeSection(0,p_copy.route.index());

  hdr_ip *new_iph =  hdr_ip::access(p_copy.pkt);
  //new_iph->daddr() = p_copy.dest.addr;
  new_iph->daddr() = Address::instance().create_ipaddr(p_copy.dest.getNSAddr_t(),RT_PORT);
  new_iph->dport() = RT_PORT;
  //new_iph->saddr() = p_copy.src.addr;
  new_iph->saddr() = Address::instance().create_ipaddr(p_copy.src.getNSAddr_t(),RT_PORT);
  new_iph->sport() = RT_PORT;
  new_iph->ttl() = 255;

  // shorten's p's route
  p.route.removeSection(heard_at, xmit_at);
  hdr_sr *new_srh =  hdr_sr::access(p_copy.pkt);
  new_srh->init();
  for (int i = 0 ; i < p.route.length() ; i++)
    p.route[i].fillSRAddr(new_srh->reply_addrs()[i]);
  new_srh->route_reply_len() = p.route.length();
  new_srh->route_reply() = 1;
  // grat replies will have a 0 seq num (it's only for trace analysis anyway)
  new_srh->rtreq_seq() = 0;

  hdr_cmn *new_cmnh =  hdr_cmn::access(p_copy.pkt);
  new_cmnh->ptype() = PT_DSR;
  new_cmnh->size() += IP_HDR_LEN;

  if (verbose_srr)
    trace("SRR %.9f _%s_ gratuitous-reply-sent %s -> %s (len %d) %s",
	  Scheduler::instance().clock(), net_id.dump(),
	  p_copy.src.dump(), p_copy.dest.dump(), p.route.length(), 
	  p.route.dump());

  // cache the route for future use (we learned the route from p)
  route_cache->addRoute(p_copy.route, Scheduler::instance().clock(), p.src);
  sendOutPacketWithRoute(p_copy, true);
}

/*==============================================================
  debug and trace output
------------------------------------------------------------*/
void
DSRAgent::trace(char* fmt, ...)
{
  va_list ap;
  
  if (!logtarget) return;

  va_start(ap, fmt);
  vsprintf(logtarget->pt_->buffer(), fmt, ap);
  logtarget->pt_->dump();
  va_end(ap);
}


/*==============================================================
  Callback for link layer transmission failures
------------------------------------------------------------*/
// XXX Obviously this structure and FilterFailure() is not used anywhere, 
// because off_cmn_ in this structure cannot be populated at all!
// Instead of deleting, I'm simply commenting them out, perhaps they'll be 
// salvaged sometime in the future. - haoboy

//  struct filterfailuredata {
//    nsaddr_t dead_next_hop;
//    int off_cmn_;
//    DSRAgent *agent;
//  };

//  int
//  FilterFailure(Packet *p, void *data)
//  {
//    struct filterfailuredata *ffd = (filterfailuredata *) data;
//    hdr_cmn *cmh = (hdr_cmn*)p->access(ffd->off_cmn_);
//    int remove = cmh->next_hop() == ffd->dead_next_hop;

//    if (remove)
//  	  ffd->agent->undeliverablePkt(p,1);
//    return remove;
//  }

void
DSRAgent::undeliverablePkt(Packet *pkt, int mine)
  /* when we've got a packet we can't deliver, what to do with it? 
     frees or hands off p if mine = 1, doesn't hurt it otherwise */
{
  hdr_sr *srh = hdr_sr::access(pkt);
  hdr_ip *iph = hdr_ip::access(pkt);
  hdr_cmn *cmh;

  SRPacket p(pkt,srh);
  //p.dest = ID(iph->dst(),::IP);
  //p.src = ID(iph->src(),::IP);
  p.dest = ID((Address::instance().get_nodeaddr(iph->daddr())),::IP);
  p.src = ID((Address::instance().get_nodeaddr(iph->saddr())),::IP);
  p.pkt = mine ? pkt : pkt->copy();

  srh = hdr_sr::access(p.pkt);
  iph = hdr_ip::access(p.pkt);
  cmh = hdr_cmn::access(p.pkt);

  // we're about to salvage. flowstate rules say we must strip all flow
  // state info out of this packet. ych 5/5/01
  cmh->size() -= srh->size(); // changes affect size of header...
  srh->flow_timeout() = 0;
  srh->flow_header() = 0;
  cmh->size() += srh->size(); // done fixing flow state headers

  if (ID((Address::instance().get_nodeaddr(iph->saddr())),::IP) == net_id) {
    // it's our packet we couldn't send
    cmh->size() -= srh->size(); // remove size of SR header
    assert(cmh->size() >= 0);
    
    handlePktWithoutSR(p, false);
    
    return;
  }

  /*
   * Am I allowed to salvage?
   */
  if(dsragent_salvage_with_cache == 0) {
	  assert(mine);
	  drop(pkt, DROP_RTR_NO_ROUTE);  
	  return;
  }

#ifdef NEW_SALVAGE_LOGIC
  if(srh->salvaged() >= dsr_salvage_max_attempts) {
	  assert(mine);
	  drop(pkt, DROP_RTR_SALVAGE);
	  return;
  }
#endif /* NEW_SALVAGE_LOGIC */

  // it's a packet we're forwarding for someone, save it if we can...
  Path salvage_route;
      
  if (route_cache->findRoute(p.dest, salvage_route, 0)) {
	  // be nice and send the packet out
#if 0
	  /* we'd like to create a ``proper'' source route with the
	     IP src of the packet as the first node, but we can't actually 
	     just append the salvage route onto the route used so far, 
	     since the append creates routes with loops in them 
	     like  1 2 3 4 3 5 
	     If we were to squish the route to remove the loop, then we'd be
	     removing ourselves from the route, which is verboten.
	     If we did remove ourselves, and our salvage route contained
	     a stale link, we might never hear the route error.
	     -dam 5/13/98

	     Could we perhaps allow SRs with loops in them on the air?
	     Since it's still a finite length SR, the pkt can't loop
	     forever... -dam 8/5/98 */

	  // truncate the route at the bad link and append good bit
	  int our_index = p.route.index();

	  p.route.setLength(our_index);
	  // yes this cuts us off the route,

	  p.route.appendPath(salvage_route);
	  // but we're at the front of s_r
	  p.route.setIterator(our_index);
#else
	  p.route = salvage_route;
	  p.route.resetIterator();
#endif

	  if (dsragent_dont_salvage_bad_replies && srh->route_reply()) {
		  // check to see if we'd be salvaging a packet
		  // with the dead link in it

		  ID to_id(srh->addrs()[srh->cur_addr()+1].addr,
			   (ID_Type) srh->addrs()[srh->cur_addr()].addr_type);
		  bool bad_reply = false;

		  for (int i = 0 ; i < srh->route_reply_len()-1; i++) {

			  if ((net_id == ID(srh->reply_addrs()[i]) &&
			      to_id == ID(srh->reply_addrs()[i+1])) ||
			      (dsragent_require_bi_routes &&
			       to_id == ID(srh->reply_addrs()[i]) &&
			       net_id == ID(srh->reply_addrs()[i+1]))) {
					  
				  bad_reply = true;
				  break;
			  }
		  }
		  if (bad_reply) {
			  // think about killing this packet
			  srh->route_reply() = 0;
			  if (PT_DSR == cmh->ptype() &&
			      ! srh->route_request() &&
			      ! srh->route_error()) {
				  // this packet has no reason to live
				  if (verbose_srr)
					  trace("SRR %.5f _%s_ --- %d dropping bad-reply %s -> %s", 
						Scheduler::instance().clock(), net_id.dump(), 
						cmh->uid(), p.src.dump(), p.dest.dump());
				  if (mine)
					  drop(pkt, DROP_RTR_MAC_CALLBACK);
				  return;
			  }
		  }
	  }

	  if (verbose_ssalv) 
		  trace("Ssalv %.5f _%s_ salvaging %s -> %s --- %d with %s",
			Scheduler::instance().clock(), net_id.dump(),
			p.src.dump(), p.dest.dump(),
			cmh->uid(), p.route.dump());

	  // remove size of SR header, added back in sendOutPacketWithRoute
	  cmh->size() -= srh->size(); 
	  assert(cmh->size() >= 0);
#ifdef NEW_SALVAGE_LOGIC
	  srh->salvaged() += 1;
#endif
	  sendOutPacketWithRoute(p, false);
  }
#ifdef NEW_SALVAGE_LOGIC
  else if(dsr_salvage_max_requests > 0) {
	  /*
	   * Allow the node to perform route discovery for an
	   * intermediate hop.
	   */
	  if (verbose_ssalv) 
		  trace("Ssalv %.5f _%s_ adding to SB --- %d %s -> %s [%d]", 
			Scheduler::instance().clock(), 
			net_id.dump(),
			cmh->uid(),
			p.src.dump(), p.dest.dump(),
			srh->salvaged());
	  stickPacketInSendBuffer(p);
  }
#endif
  else {
	  // we don't have a route, and it's not worth us doing a
	  // route request to try to help the originator out, since
	  // it might be counter productive
	  if (verbose_ssalv) 
		  trace("Ssalv %.5f _%s_ dropping --- %d %s -> %s [%d]", 
			Scheduler::instance().clock(), 
			net_id.dump(), cmh->uid(),
			p.src.dump(), p.dest.dump(),
			srh->salvaged());
	  if (mine)
		  drop(pkt, DROP_RTR_NO_ROUTE);
  }
}

#ifdef USE_GOD_FEEDBACK
static int linkerr_is_wrong = 0;
#endif

void
DSRAgent::sendUnknownFlow(SRPacket &p, bool asDefault, u_int16_t flowid) {
  hdr_sr *srh = hdr_sr::access(p.pkt);
  hdr_ip *iph = hdr_ip::access(p.pkt);
  hdr_cmn *cmh = hdr_cmn::access(p.pkt);
  struct flow_error *fe;

  assert(!srh->num_addrs()); // flow forwarding basis only.
#if 0
  // this doesn't always hold true; if an xmit fails, we'll dump the
  // thing from our flow table, possibly before we even get here (though how
  // we found out, other than from this packet, is anyone's guess, considering
  // that underliverablePkt() should have been called in any other circumstance,
  // so we shouldn't go through the failed stuff.
  assert(p.src != net_id); // how'd it get here if it were?

  // this doesn't always hold true; I may be sending it default, fail,
  // the flow times out, but I still know the flowid (whacked paths through
  // the code, I know... ych 5/7/01
  assert(srh->flow_header() ^ asDefault); // one or the other, not both
#endif

  if (p.src == net_id) {
    Packet::free(p.pkt);
    p.pkt = 0;
    return; // gimme a break, we already know!
  }

  undeliverablePkt(p.pkt, false); // salvage, but don't molest.
 
  /* warp into an error... */
  if (asDefault) {
    if (!srh->flow_default_unknown()) {
      srh->num_default_unknown() = 1;
      srh->flow_default_unknown() = 1;
      fe = srh->unknown_defaults();
    } else if (srh->num_default_unknown() < MAX_ROUTE_ERRORS) {
      fe = srh->unknown_defaults() + srh->num_default_unknown();
      srh->num_default_unknown()++;
    } else {
      trace("SYFU  %.5f _%s_ dumping maximally nested Flow error %d -> %d",
      Scheduler::instance().clock(), net_id.dump(), p.src.addr, p.dest.addr);

      Packet::free(p.pkt);        // no drop needed
      p.pkt = 0;
      return;
    }
  } else {
    if (!srh->flow_unknown()) {
      srh->num_flow_unknown() = 1;
      srh->flow_unknown() = 1;
      fe = srh->unknown_flows();
    } else if (srh->num_default_unknown() < MAX_ROUTE_ERRORS) {
      fe = srh->unknown_flows() + srh->num_flow_unknown();
      srh->num_flow_unknown()++;
    } else {
      trace("SYFU  %.5f _%s_ dumping maximally nested Flow error %d -> %d",
      Scheduler::instance().clock(), net_id.dump(), p.src.addr, p.dest.addr);

      Packet::free(p.pkt);        // no drop needed
      p.pkt = 0;
      return;
    }
  }

  trace("SFErr %.5f _%s_ %d -> %d : %d",
	Scheduler::instance().clock(), net_id.dump(), p.src.addr, p.dest.addr,
	flowid);

  srh->route_reply() = 0;
  srh->route_request() = 0;
  srh->flow_header() = 0;
  srh->flow_timeout() = 0;

  //iph->daddr() = p.src.addr;
  iph->daddr() = Address::instance().create_ipaddr(p.src.getNSAddr_t(),RT_PORT);
  iph->dport() = RT_PORT;
  //iph->saddr() = net_id.addr;
  iph->saddr() = Address::instance().create_ipaddr(net_id.getNSAddr_t(),RT_PORT);
  iph->sport() = RT_PORT;
  iph->ttl() = 255;

  //fe->flow_src = p.src.addr;
  fe->flow_src = p.src.getNSAddr_t();
  //fe->flow_dst = p.dest.addr;
  fe->flow_dst = p.dest.getNSAddr_t();
  fe->flow_id  = flowid;

  //p.src = ID(iph->src(), ::IP);
  //p.dest = ID(iph->dst(), ::IP);
  p.dest = ID((Address::instance().get_nodeaddr(iph->daddr())),::IP);
  p.src = ID((Address::instance().get_nodeaddr(iph->saddr())),::IP);


  cmh->ptype() = PT_DSR;                // cut off data
  cmh->size() = IP_HDR_LEN;
  cmh->num_forwards() = 0;
  // assign this packet a new uid, since we're sending it
  cmh->uid() = uidcnt_++;

  handlePktWithoutSR(p, false);
  assert(p.pkt == 0);
}

void 
DSRAgent::xmitFlowFailed(Packet *pkt, const char* reason)
{
  hdr_sr *srh = hdr_sr::access(pkt);
  hdr_ip *iph = hdr_ip::access(pkt);
  hdr_cmn *cmh = hdr_cmn::access(pkt);
  int flowidx = flow_table.find(iph->saddr(), iph->daddr(), srh->flow_id());
  u_int16_t default_flow;

  assert(!srh->num_addrs());

  if (!srh->flow_header()) {
    if (!flow_table.defaultFlow(iph->saddr(), iph->daddr(), default_flow)) {
      SRPacket p(pkt, srh);
      //p.src = ID(iph->src(), ::IP);
      //p.dest = ID(iph->dst(), ::IP);
      p.dest = ID((Address::instance().get_nodeaddr(iph->daddr())),::IP);
      p.src = ID((Address::instance().get_nodeaddr(iph->saddr())),::IP);


      sendUnknownFlow(p, true);
      return;
    }
    flowidx = flow_table.find(iph->saddr(), iph->daddr(), default_flow);
  }

  if (flowidx == -1 || 
      flow_table[flowidx].timeout < Scheduler::instance().clock()) {
    // blah, the flow has expired, or been forgotten.
    SRPacket p(pkt, srh);
    //p.src = ID(iph->src(), ::IP);
    //p.dest = ID(iph->dst(), ::IP);
    p.dest = ID((Address::instance().get_nodeaddr(iph->daddr())),::IP);
    p.src = ID((Address::instance().get_nodeaddr(iph->saddr())),::IP);


    return;
  }

  cmh->size() -= srh->size(); // gonna change the source route size
  assert(cmh->size() >= 0);
  
  flow_table[flowidx].sourceRoute.fillSR(srh);
  srh->cur_addr() = flow_table[flowidx].hopCount;
  assert(srh->addrs()[srh->cur_addr()].addr == (nsaddr_t) net_id.addr);
  cmh->size() += srh->size();

  // xmitFailed is going to assume this was incr'ed for send
  srh->cur_addr()++;
  xmitFailed(pkt, reason);
}

void 
DSRAgent::xmitFailed(Packet *pkt, const char* reason)
  /* mark our route cache reflect the failure of the link between
     srh[cur_addr] and srh[next_addr], and then create a route err
     message to send to the orginator of the pkt (srh[0])
     p.pkt freed or handed off */
{
  hdr_sr *srh = hdr_sr::access(pkt);
  hdr_ip *iph = hdr_ip::access(pkt);
  hdr_cmn *cmh = hdr_cmn::access(pkt);

  assert(cmh->size() >= 0);

  srh->cur_addr() -= 1;		// correct for inc already done on sending
  
  if (srh->cur_addr() >= srh->num_addrs() - 1)
    {
      trace("SDFU: route error beyond end of source route????");
      fprintf(stderr,"SDFU: route error beyond end of source route????\n");
      Packet::free(pkt);
      return;
    }

  if (srh->route_request())
    {
      trace("SDFU: route error forwarding route request????");
      fprintf(stderr,"SDFU: route error forwarding route request????\n");
      Packet::free(pkt);
      return;
    }


  ID tell_id(srh->addrs()[0].addr,
	     (ID_Type) srh->addrs()[srh->cur_addr()].addr_type);
  ID from_id(srh->addrs()[srh->cur_addr()].addr,
	     (ID_Type) srh->addrs()[srh->cur_addr()].addr_type);
  ID to_id(srh->addrs()[srh->cur_addr()+1].addr,
	     (ID_Type) srh->addrs()[srh->cur_addr()].addr_type);
  assert(from_id == net_id || from_id == MAC_id);

  trace("SSendFailure %.9f _%s_ %d %d %d:%d %d:%d %s->%s %d %d %d %d %s",
	Scheduler::instance().clock(), net_id.dump(), 
	cmh->uid(), cmh->ptype(),
	iph->saddr(), iph->sport(),
	iph->daddr(), iph->dport(),
	from_id.dump(),to_id.dump(),
	God::instance()->hops(from_id.getNSAddr_t(), to_id.getNSAddr_t()),
	God::instance()->hops(iph->saddr(),iph->daddr()),
	God::instance()->hops(from_id.getNSAddr_t(), iph->daddr()),
	srh->num_addrs(), srh->dump());

#ifdef USE_GOD_FEEDBACK
  if (God::instance()->hops(from_id.getNSAddr_t(), to_id.getNSAddr_t()) == 1)
    { /* god thinks this link is still valid */
      linkerr_is_wrong++;
      trace("SxmitFailed %.5f _%s_  %d->%d god okays #%d",
            Scheduler::instance().clock(), net_id.dump(),
            from_id.getNSAddr_t(), to_id.getNSAddr_t(), linkerr_is_wrong);
      fprintf(stderr,
	      "xmitFailed on link %d->%d god okays - ignoring & recycling #%d\n",
	      from_id.getNSAddr_t(), to_id.getNSAddr_t(), linkerr_is_wrong);
      /* put packet back on end of ifq for xmission */
      srh->cur_addr() += 1;	// correct for decrement earlier in proc 
      // make sure we aren't cycling packets
      // also change direction in pkt hdr
      cmh->direction() = hdr_cmn::DOWN;
      ll->recv(pkt, (Handler*) 0);
      return;
    }
#endif

  if(strcmp(reason, "DROP_IFQ_QFULL") != 0) {
	  assert(strcmp(reason, "DROP_RTR_MAC_CALLBACK") == 0);

	  /* kill any routes we have using this link */
	  route_cache->noticeDeadLink(from_id, to_id,
				      Scheduler::instance().clock());
	  flow_table.noticeDeadLink(from_id, to_id);

	  /* give ourselves a chance to save the packet */
	  undeliverablePkt(pkt->copy(), 1);

	  /* now kill all the other packets in the output queue that would
	     use the same next hop.  This is reasonable, since 802.11 has
	     already retried the xmission multiple times => a persistent
	     failure. */

	  /* XXX YCH 5/4/01 shouldn't each of these packets get Route Errors
	   * if one hasn't already been sent? ie if two different routes
	   * are using this link?
	   */
	  {
	    Packet *r, *nr, *queue1 = 0, *queue2 = 0;
	    // pkts to be recycled
	    
	    while((r = ifq->prq_get_nexthop(to_id.getNSAddr_t()))) {
	      r->next_ = queue1;
	      queue1 = r; 
	    }

	    // the packets are now in the reverse order of how they
	    // appeared in the IFQ so reverse them again
	    for(r = queue1; r; r = nr) {
	      nr = r->next_;
	      r->next_ = queue2;
	      queue2 = r;
	    }

	    // now process them in order
	    for(r = queue2; r; r = nr) {
	      nr = r->next_;
	      undeliverablePkt(r, 1);
	    }
	  }
  }
  
  /* warp pkt into a route error message */
  if (tell_id == net_id || tell_id == MAC_id)
    { // no need to send the route error if it's for us
      if (verbose) 
        trace("Sdebug _%s_ not bothering to send route error to ourselves", 
	      tell_id.dump());
      Packet::free(pkt);	// no drop needed
      pkt = 0;
      return;
    }

  if (srh->num_route_errors() >= MAX_ROUTE_ERRORS)
    { // no more room in the error packet to nest an additional error.
      // this pkt's been bouncing around so much, let's just drop and let
      // the originator retry
      // Another possibility is to just strip off the outer error, and
      // launch a Route discovey for the inner error XXX -dam 6/5/98
      trace("SDFU  %.5f _%s_ dumping maximally nested error %s  %d -> %d",
	    Scheduler::instance().clock(), net_id.dump(),
	    tell_id.dump(),
	    from_id.dump(),
	    to_id.dump());
      Packet::free(pkt);	// no drop needed
      pkt = 0;
      return;
    }

  link_down *deadlink = &(srh->down_links()[srh->num_route_errors()]);
  deadlink->addr_type = srh->addrs()[srh->cur_addr()].addr_type;
  deadlink->from_addr = srh->addrs()[srh->cur_addr()].addr;
  deadlink->to_addr = srh->addrs()[srh->cur_addr()+1].addr;
  deadlink->tell_addr = srh->addrs()[0].addr;
  srh->num_route_errors() += 1;

  if (verbose)
    trace("Sdebug %.5f _%s_ sending into dead-link (nest %d) tell %d  %d -> %d",
          Scheduler::instance().clock(), net_id.dump(),
          srh->num_route_errors(),
          deadlink->tell_addr,
          deadlink->from_addr,
          deadlink->to_addr);

  srh->route_error() = 1;
  srh->route_reply() = 0;
  srh->route_request() = 0;
  srh->flow_header() = 0;
  srh->flow_timeout() = 0;

  //iph->daddr() = deadlink->tell_addr;
  iph->daddr() = Address::instance().create_ipaddr(deadlink->tell_addr,RT_PORT);
  iph->dport() = RT_PORT;
  //iph->saddr() = net_id.addr;
  iph->saddr() = Address::instance().create_ipaddr(net_id.addr,RT_PORT);
  iph->sport() = RT_PORT;
  iph->ttl() = 255;

  cmh->ptype() = PT_DSR;		// cut off data
  cmh->size() = IP_HDR_LEN;
  cmh->num_forwards() = 0;
  // assign this packet a new uid, since we're sending it
  cmh->uid() = uidcnt_++;

  SRPacket p(pkt, srh);
  p.route.setLength(p.route.index()+1);
  p.route.reverseInPlace();
  p.dest = tell_id;
  p.src = net_id;

  /* send out the Route Error message */
  sendOutPacketWithRoute(p, true);
}


#if 0

/* this is code that implements Route Reply holdoff to prevent route 
   reply storms.  It's described in the kluwer paper and was used in 
   those simulations, but isn't currently used.  -dam 8/5/98 */

/*==============================================================
  Callback Timers to deal with holding off  route replies

  Basic theory: if we see a node S that has requested a route to D
  send a packet to D via a route of length <= ours then don't send
  our route.  We record that S has used a good route to D by setting
  the best_length to -1, meaning that our route can't possibly do
  S any good (unless S has been lied to, but we probably can't know
  that).
  
  NOTE: there is confusion in this code as to whether the requestor
  and requested_dest ID's are MAC or IP... It doesn't matter for now
  but will later when they are not the same.

------------------------------------------------------------*/
struct RtHoldoffData: public EventData {
  RtHoldoffData(DSRAgent *th, Packet *pa, int ind):t(th), p(pa), index(ind)
  {}
  DSRAgent *t;
  Packet *p;
  int index;
};

void
RouteReplyHoldoffCallback(Node *node, Time time, EventData *data)
// see if the packet inside the data is still in the
// send buffer and expire it if it is
{
  Packet *p = ((RtHoldoffData *)data)->p;
  DSRAgent *t = ((RtHoldoffData *)data)->t;
  int index = ((RtHoldoffData *)data)->index;

  RtRepHoldoff *entry = &(t->rtrep_holdoff[index]);
  assert((entry->requestor == p->dest));

  // if we haven't heard the requestor use a route equal or better
  // than ours then send our reply.
  if ((lsnode_require_use && entry->best_length != -1)
      || (!lsnode_require_use && entry->best_length > entry->our_length))
    { // we send
      world_statistics.sendingSrcRtFromCache(t,time,p);
      t->sendPacket(t,time,p);
    }
  else
    { // dump our packet
      delete p;
    }
  entry->requestor = invalid_addr;
  entry->requested_dest = invalid_addr;
  delete data;
  t->num_heldoff_rt_replies--;
}

void
DSRAgent::scheduleRouteReply(Time t, Packet *new_p)
  // schedule a time to send new_p if we haven't heard a better
  // answer in the mean time.  Do not modify new_p after calling this
{
  for (int c = 0; c < RTREP_HOLDOFF_SIZE; c ++)
    if (rtrep_holdoff[c].requested_dest == invalid_addr) break;
  assert(c < RTREP_HOLDOFF_SIZE);

  Path *our_route = &(new_p->data.getRoute().source_route);
  rtrep_holdoff[c].requested_dest = (*our_route)[our_route->length() - 1];
  rtrep_holdoff[c].requestor = new_p->dest;
  rtrep_holdoff[c].best_length = MAX_ROUTE_LEN + 1;
  rtrep_holdoff[c].our_length = our_route->length();

  Time send_time = t +
    (Time) (our_route->length() - 1) * rt_rep_holdoff_period
    + U(0.0, rt_rep_holdoff_period);
  RegisterCallback(this,&RouteReplyHoldoffCallback, send_time,
		   new RtHoldoffData(this,new_p,c));
  num_heldoff_rt_replies++;
}

void
DSRAgent::snoopForRouteReplies(Time t, Packet *p)
  // see if p is a route reply that we're watching for
  // or if it was sent off using a route reply we're watching for
{
  for (int c = 0 ; c <RTREP_HOLDOFF_SIZE ; c ++)
    {
      RtRepHoldoff *entry = &(rtrep_holdoff[c]);

      // there is no point in doing this first check if we're always
      // going to send our route reply unless we hear the requester use one
      // better or equal to ours
      if (entry->requestor == p->dest
	  && (p->type == ::route_reply || p->data.sourceRoutep()))
	{ // see if this route reply is one we're watching for
	  Path *srcrt = &(p->data.getRoute().source_route);
	  if (!(entry->requested_dest == (*srcrt)[srcrt->length()-1]))
	    continue;		// it's not ours
	  if (entry->best_length > srcrt->length())
	    entry->best_length = srcrt->length();
	} // end if we heard a route reply being sent
      else if (entry->requestor == p->src
	       && entry->requested_dest == p->dest)
	{ // they're using a route  reply! see if ours is better
          if (p->route.length() <= entry->our_length)
            { // Oh no! they've used a better path than ours!
              entry->best_length = -1; //there's no point in replying.
            }
        } // end if they used used route reply
      else
        continue;
    }
}

#endif //0








