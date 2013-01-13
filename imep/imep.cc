/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
 *      This product includes software developed by the Computer Systems
 *      Engineering Group at Lawrence Berkeley Laboratory.
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
 *
 * Ported from CMU/Monarch's code 
 *
 * $Header: /cvsroot/nsnam/ns-2/imep/imep.cc,v 1.12 2006/02/21 15:20:18 mahrenho Exp $
 */

#include <packet.h>
#include <ip.h>
#include <random.h>

#include <cmu-trace.h>
#include <imep/imep.h>

#define CURRENT_TIME	Scheduler::instance().clock()

static const int verbose = 0;
static const int imep_use_mac_callback = 1;

// ======================================================================
//   TCL Hooks
// ======================================================================

int hdr_imep::offset_;
static class IMEPHeaderClass : public PacketHeaderClass {
public:
        IMEPHeaderClass() : PacketHeaderClass("PacketHeader/IMEP",
					      IMEP_HDR_LEN) { 
		bind_offset(&hdr_imep::offset_);
	} 
} class_imep_hdr;

static class agentIMEPclass : public TclClass {
public:
	agentIMEPclass() : TclClass("Agent/IMEP") {}
	TclObject* create(int argc, const char*const* argv) {
		assert(argc == 5);
		return (new imepAgent((nsaddr_t) atoi(argv[4])));
	}
} class_imepAgent;


// ======================================================================
// ======================================================================

// MAC layer callback
static void
imep_failed_callback(Packet *p, void *arg)
{
	if(imep_use_mac_callback)
		((imepAgent*) arg)->imepPacketUndeliverable(p);
	else {
		Packet::free(p);
		// XXX: Should probably call a "drop" agent here
	}
}

imepAgent::imepAgent(nsaddr_t index) :
	Agent(PT_TORA),
	beaconTimer(this, BEACON_TIMER),
	controlTimer(this, CONTROL_TIMER),
	rexmitTimer(this, REXMIT_TIMER),
	incomingTimer(this, INCOMING_TIMER),
	ipaddr(index),
	incomingQ(this, index)
{
	controlSequence = 0;
	recvtarget_ = sendtarget_ = 0;
	logtarget_ = 0;
	rtagent_ = 0;
	LIST_INIT(&imepLinkHead);
	bzero(&stats, sizeof(stats));
}

int
imepAgent::command(int argc, const char*const* argv)
{
	if(argc == 2) {
		if(strcmp(argv[1], "start") == 0) {
			beaconTimer.start(BEACON_PERIOD);
			return TCL_OK;
		} 
		else if(strcmp(argv[1], "reset") == 0) {
		        Terminate();
			return TCL_OK;
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "recvtarget") == 0) {
			recvtarget_ = (NsObject*) TclObject::lookup(argv[2]);
                        assert(recvtarget_);
			return (TCL_OK);
		}
		else if (strcmp(argv[1], "sendtarget") == 0) {
			sendtarget_ = (NsObject*) TclObject::lookup(argv[2]);
                        assert(sendtarget_);
			return (TCL_OK);
		}
		else if (strcmp(argv[1], "rtagent") == 0) {
			rtagent_ = (rtAgent*) TclObject::lookup(argv[2]);
			assert(rtagent_);
			return (TCL_OK);
		}
		else if(strcmp(argv[1], "log-target") == 0) {
			logtarget_ = (Trace*) TclObject::lookup(argv[2]);
			assert(logtarget_);
			return (TCL_OK);
		}
	}
	return Agent::command(argc, argv);
}


imepLink*
imepAgent::findLink(nsaddr_t index)
{
	imepLink *l;

	for(l = imepLinkHead.lh_first; l; l = l->link.le_next) {
		if(l->index() == index)
			return l;
	}
	return 0;
}

Packet*
imepAgent::findObjectSequence(u_int8_t seqno)
{
        Packet *p;	
	ReXmitQIter iter = rexmitq.iter();
	struct imep_object_block *ob;
	
	while ((p = iter.next())) {
		ob = findObjectBlock(p);

		if(ob == 0) continue;
		// no OBJECT block

		if(ob->ob_sequence != seqno) continue;
		// wrong SEQUENCE number

		if(ob->ob_num_responses <=0) {
			fprintf(stderr,
				"imepAgent::findObjectSequence: "
				"Object Block without response list\n");
			abort();
		}

		return p;
	}

	/*	abort();
		this isn't an abort condition.  consider an ack arriving 
		for a pkt after it's timed out from the rexmit q -dam */

	return NULL;
}


void
imepAgent::removeObjectResponse(Packet *p, nsaddr_t index)
{
	struct imep_object_block *ob = findObjectBlock(p);
	struct imep_response *r = findResponseList(p);
	struct imep_response *r0;
	struct hdr_cmn *ch = HDR_CMN(p);
	int i;

	assert(ob && r);

	for(i = 0, r0 = r; i < ob->ob_num_responses; i++, r0++) {
		if(INT32_T(r0->resp_ipaddr) == index)
			break;
	}

	if(INT32_T(r0->resp_ipaddr) != index) {
	  if (verbose) 
	    trace("T %.9f _%d_ dup ack(?) from %d", CURRENT_TIME, ipaddr, index);
	  return;
	}

	if(ob->ob_num_responses == 1) {
	        if (verbose) 
	           trace("T %.9f _%d_ remove from reXq pkt %d", 
		         CURRENT_TIME, ipaddr, ch->uid());
	        rexmitq.remove(p);
	        Packet::free(p);
		stats.num_rexmitable_fully_acked++;
	} else {
		// find the last "response"
  	        r += (ob->ob_num_responses - 1);

		if(r != r0) {
			INT32_T(r0->resp_ipaddr) = INT32_T(r->resp_ipaddr);
		}
		
		ob->ob_num_responses -= 1;			

	        if (verbose) 
	           trace("T %.9f _%d_ remove %d from resp list %d (%d left) RL %s", 
		         CURRENT_TIME, ipaddr, index, ch->uid(), ob->ob_num_responses, 
			 dumpResponseList(p));

		struct hdr_imep *im = HDR_IMEP(p);
		ch->size() -= sizeof(struct imep_response);
		U_INT16_T(im->imep_length) -= sizeof(struct imep_response);
	}
}


void 
imepAgent::purgeReXmitQ(nsaddr_t index)
  // remove index from any response lists in the rexmit q
{
  Packet *p;	
  ReXmitQIter iter = rexmitq.iter();
  struct imep_object_block *ob;
  struct imep_response *r,*r0;
  struct hdr_cmn *ch;
  int i;

  if (verbose)
    trace("T %.9f _%d_ purge %d from reXmit Q", 
	  CURRENT_TIME, ipaddr, index);

  while ((p = iter.next())) {
    ob = findObjectBlock(p);
    if(ob == 0) assert(0); // should always be an object block

    r = findResponseList(p);
    ch = HDR_CMN(p);

    assert(ob && r);

    for(i = 0, r0 = r; i < ob->ob_num_responses; i++, r0++) {
      if(INT32_T(r0->resp_ipaddr) == index)
	break;
    }

    if(INT32_T(r0->resp_ipaddr) != index) {
      continue; // index not in this response list
    }

    if(ob->ob_num_responses == 1) {
      if (verbose) 
	trace("T %.9f _%d_ remove from reXq pkt %d",
	      CURRENT_TIME, ipaddr, ch->uid());
      rexmitq.remove(p);
      drop(p, DROP_RTR_QTIMEOUT);
      stats.num_rexmitable_fully_acked++;
    } else {
      // find the last "response"
      r += (ob->ob_num_responses - 1);

      if(r != r0) {
	INT32_T(r0->resp_ipaddr) = INT32_T(r->resp_ipaddr);
      }
		
      ob->ob_num_responses -= 1;			

      if (verbose) 
	trace("T %.9f _%d_ purge %d from resp list %d (%d left) RL %s", 
	      CURRENT_TIME, ipaddr, index, ch->uid(), ob->ob_num_responses, 
	      dumpResponseList(p));

      struct hdr_imep *im = HDR_IMEP(p);
      ch->size() -= sizeof(struct imep_response);
      U_INT16_T(im->imep_length) -= sizeof(struct imep_response);
    }
  }
}


// ======================================================================
// ======================================================================
// Timer Handling Functions

void
imepAgent::handlerTimer(imepTimerType t)
{
	switch(t) {
	case BEACON_TIMER:
		handlerBeaconTimer();
		break;
	case CONTROL_TIMER:
		handlerControlTimer();
		break;
	case REXMIT_TIMER:
	        handlerReXmitTimer();
		break;
	case INCOMING_TIMER:
		handlerIncomingTimer();
		break;
	default:
		abort();
	}
}

void
imepAgent::handlerBeaconTimer(void)
{
  imepLink *l;

  // garbage collect old links
  purgeLink();

  if (verbose) log_neighbor_list();

  /* aside from the debugging asserts, handleControlTimer will generate a
   ``beacon'' packet if there are no objects pending, so we could
   just call it.  Since we have sendBeacon() laying around, though, 
   I'll call it and leave the debugging asserts in handleControlTimer()

   The packet generated by handlerControlTimer is a beacon equivelent, 
   so we don't need to generate a beacon. */

  if (controlTimer.busy() || helloQueue.length() > 0)
    { // a control timer is pending or there's left over Hello's in the queue, 
      // but we're about to service all pending acks, hellos, and objects now, 
      // so cancel the timer
      if (controlTimer.busy()) controlTimer.cancel();
      handlerControlTimer();
    }
  else 
    { // all the Hellos we had to send out during the last BEACON_PERIOD
      // went out.  Assuming there were some, they were beacon equivelent,
      // and we don't need to send a beacon now.  If there were none, beacon
      if (NULL == imepLinkHead.lh_first) sendBeacon();
      // this is a touch conservative, since if there were hellos that went out
      // but their links were down'd by purgeLink, we'll still beacon.
      // But, if we *have*no* adjacencies, we should do something to get some
    }

  /* Send a hello to all our IN adjacencies (everyone we've heard a
     packet from). This loads up the helloQueue with all the hellos that
     that need to go out sometime in the next BEACON_PERIOD before the
     beaconTimer goes off, but doesn't start the controlTimer.  If a
     control packet is sent for some other reason, the hellos will ride out
     for free, otherwise they'll go out when the beacon timer goes off.
     */
  int busy_before_hello_load = controlTimer.busy();
  for(l = imepLinkHead.lh_first; l; l = l->link.le_next) 
    {
      if (l->status() & LINK_IN) sendHello(l->index());
    }
  if (!busy_before_hello_load && controlTimer.busy()) controlTimer.cancel();

  // restart the beacon timer
  beaconTimer.start(BEACON_PERIOD);
}


// transmit all queued ACKs, HELLOs, and OBJECTs.
void
imepAgent::handlerControlTimer(void)
{
	Packet *p;

	int num_acks = ackQueue.length();
	int num_hellos = helloQueue.length();
	int num_objects = objectQueue.length();

MAKE_PACKET:
	assert(num_acks + num_hellos + num_objects > 0);

	// now have to aggregate multiple control packets

	p = Packet::alloc();
	
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_imep *im = HDR_IMEP(p);

	ch->uid() = uidcnt_++;
	ch->ptype() = PT_IMEP;
	ch->size() = BEACON_HDR_LEN;
	ch->iface() = -2;
	ch->error() = 0;
	ch->addr_type() = NS_AF_NONE;
        ch->prev_hop_ = ipaddr;

	ih->saddr() = ipaddr;
	ih->daddr() = IP_BROADCAST;
	ih->sport() = RT_PORT;
	ih->dport() = RT_PORT;
	ih->ttl_ = 1;

	im->imep_version = IMEP_VERSION;
	im->imep_block_flags = 0x00;
	U_INT16_T(im->imep_length) = sizeof(struct hdr_imep);

	aggregateAckBlock(p);
	aggregateHelloBlock(p);
	aggregateObjectBlock(p);

	imep_output(p);

	num_acks = ackQueue.length();
	num_hellos = helloQueue.length();
	num_objects = objectQueue.length();
	if (num_acks + num_hellos + num_objects > 0)
	  { // not done yet...
	    if (verbose) 
	      trace("T %.9f _%d_ imep pkt overflow %d %d %d leftover",
		    CURRENT_TIME, ipaddr, num_acks, num_hellos, num_objects);
	    goto MAKE_PACKET;
	  }

	// don't need to restart the controlTimer because the arrival of
	// the next packet from an ULP will start it.
}

void
imepAgent::handlerReXmitTimer() 
{
  Packet *p;
  Time rexat;
  int num_xmits_left;

  rexmitq.peekHead(&rexat, &p, &num_xmits_left);

  if (NULL == p) return;  //  no more pkts on queue
  struct hdr_cmn *ch = HDR_CMN(p);

  if (0 == num_xmits_left)
    {
      if (verbose) 
	{
	  trace("T %.9f _%d_ rexmit timed out %d RL:%s",
		CURRENT_TIME, ipaddr, ch->uid(), dumpResponseList(p));
	}

        struct imep_object_block *ob = findObjectBlock(p);
	struct imep_response *r = findResponseList(p);
	int i;

	for(i = 0; i < ob->ob_num_responses; i++, r++) 
	  {
	    if (verbose) trace("T %.9f _%d_ punting neighbor %d",
			       CURRENT_TIME, ipaddr, INT32_T(r->resp_ipaddr));
	    imepSetLinkDownStatus(INT32_T(r->resp_ipaddr));
	  }

      stats.num_rexmitable_retired++;
      stats.sum_rexmitable_retired_response_sz += ob->ob_num_responses;


      // don't need to explicitly remove p from q and drop it, since
      // by downing all the links on it's response list, it'll have bee
      // dropped anyway
      // rexmitq.removeHead();
      // drop(p, DROP_RTR_QTIMEOUT);
    }
  else if (rexat <= CURRENT_TIME) 
    {
      if (verbose) 
	trace("T %.9f _%d_ rexmit %d as %d",
	      CURRENT_TIME, ipaddr, ch->uid(), uidcnt_);
      ch->uid() = uidcnt_++;
      imep_output(p->copy());

      num_xmits_left--;
      rexmitq.removeHead(); // take it off the queue and reinsert it
      rexmitq.insert(CURRENT_TIME + RETRANS_PERIOD, p, num_xmits_left);

      stats.num_rexmits++;
    }  
  
  // reschedule the timer
  rexmitq.peekHead(&rexat, &p, &num_xmits_left);
  if (NULL == p) return;  //  no more pkts on queue
  if (verbose) trace("T %.9f _%d_ rexmit trigger again for %d at %.9f (in %.9f)",
		     CURRENT_TIME, ipaddr, ch->uid(), rexat, rexat - CURRENT_TIME );
  rexmitTimer.start(rexat - CURRENT_TIME);
}


void
imepAgent::handlerIncomingTimer()
{
	Packet *p;
	u_int32_t s;
	double expire;
	int index;

	if (verbose) trace("T %.9f _%d_ inorder - timer expired",
	      CURRENT_TIME, ipaddr);

	incomingQ.dumpAll();

	while((p = incomingQ.getNextPacket(s))) {
		stats.num_holes_retired++;
		
		index = HDR_IP(p)->saddr();
		imepLink *l = findLink(index);		
		assert(l);  // if there's no link entry, then the incoming
		// q should have been cleared, when the link entry was destroyed

		if(verbose) 
		  trace("T %.9f _%d_ inorder - src %d hole retired seq %d -> %d",
			CURRENT_TIME, ipaddr, index, l->lastSeq(), s);

		/* tell ULP that we've effectively broken our link to 
		   neighbor by retiring the hole and accepting the deletion.
		   since we don't do this till at least MAX_REXMIT_TIME after
		   receiving the out of seq packet, we're sure the packet's sender
		   must have timed us out when we didn't ack their packet.
		   can't call imepLinkDown b/c it'll purge the reseq q */
		rtagent_->rtNotifyLinkDN(index);
		stats.delete_neighbor3++;
		rtagent_->rtNotifyLinkUP(index);
		stats.new_neighbor++;

		if (verbose)
		  trace("T %.9f _%d_ inorder - src %d seq %d (timer delivery)",
			CURRENT_TIME, ipaddr, index, s);

		l->lastSeq() = s;  // advance sequence number for this neighbor
		
		stats.num_recvd_from_queue++;
		imep_object_process(p);
		Packet::free(p);

		// now deliver as many in sequence packets to ULP as possible
		Packet *p0;
		while((p0 = incomingQ.getPacket(index, l->lastSeq() + 1)))
		  {
		    if (verbose)
		      trace("T %.9f _%d_ inorder - src %d seq %d (chain" 
			    " timer delivery)", CURRENT_TIME, ipaddr, 
			    HDR_IP(p0)->saddr(), l->lastSeq() + 1);
		    l->lastSeq() += 1;
		    stats.num_recvd_from_queue++;
		    imep_object_process(p0);
		    Packet::free(p0);
		  }		
	}

	if((expire = incomingQ.getNextExpire()) != 0.0) {
		assert(expire > CURRENT_TIME);
		if (verbose)
		  trace("T %.9f _%d_ inorder - timer started (delay %.9f)",
			CURRENT_TIME, ipaddr, expire - CURRENT_TIME);
	        incomingTimer.start(expire - CURRENT_TIME);
	}
}


//////////////////////////////////////////////////////////////////////

void
imepAgent::scheduleReXmit(Packet *p)
{
  rexmitq.insert(CURRENT_TIME + RETRANS_PERIOD, p, MAX_REXMITS);
  
  // start the timer
  if (!rexmitTimer.busy()) rexmitTimer.start(RETRANS_PERIOD);    
}

void
imepAgent::scheduleIncoming(Packet *p, u_int32_t s)
{
	struct hdr_ip *ip = HDR_IP(p);

	incomingQ.addEntry(ip->saddr(), CURRENT_TIME + MAX_RETRANS_TIME, s, p);
  
	// start the timer
	if (!incomingTimer.busy()) {
		if (verbose) 
		  trace("T %.9f _%d_ inorder - timer started",
			CURRENT_TIME, ipaddr);
		incomingTimer.start(MAX_RETRANS_TIME);
	}
}


// ======================================================================
// Packet Processing Functions

void
imepAgent::recv(Packet *p, Handler *)
{
	//struct hdr_ip *ih = HDR_IP(p);
	struct hdr_cmn *ch = HDR_CMN(p);

	assert(initialized());

	if(ch->prev_hop_ == ipaddr) {
	  // I hate all uses of prev_hop, but the only other way to
	  // do this test is by checking for a nonNULL handler (like 
	  // mac-801_11.cc does), which would
	  // require changing tora to send out pkts with a non 0 hndler
	  // -dam
		recv_outgoing(p);
	} else {
	        recv_incoming(p);
	}
}

void
imepAgent::recv_outgoing(Packet *p)
{
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ip = HDR_IP(p);

	if(DATA_PACKET(ch->ptype())) {
		imep_output(p);
		return;
	}

	if(ip->daddr() != (nsaddr_t) IP_BROADCAST) {
		fprintf(stderr, "IP dst is unicast - not encapsulating\n");
		imep_output(p);
		return;
	}

	assert(ch->ptype() == PT_TORA);
	// XXX: for debugging purposes - IMEP supports other object types

	objectQueue.enque(p);
	// this queue is a queue of "packets" passed down from the 
	// upper layer routing protocols that IMEP will buffer and try
	// to aggregate before transmitting.  Although these are valid
	// packets, they must not be transmitted before encaspulating
	// them in an IMEP packet to ensure reliability.

	double send_delay = MIN_TRANSMIT_WAIT_TIME_HIGHP
	  + ((MAX_TRANSMIT_WAIT_TIME_HIGHP - MIN_TRANSMIT_WAIT_TIME_HIGHP)
	     * Random::uniform());
	if (controlTimer.busy() == 0) 
	  {
	    controlTimer.start(send_delay);
	  } 
	else if (controlTimer.timeLeft() > send_delay) 
	  {
	    controlTimer.cancel();
	    controlTimer.start(send_delay);
	  }
}

void
imepAgent::recv_incoming(Packet *p)
{
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_imep *im = HDR_IMEP(p);

	if(DATA_PACKET(ch->ptype())) {
		imep_input(p);
		return;
	}

	// if it's a data packet, the ip->src could be from far away,
	// so we can't use it for link indications.  If we RARPd the 
	// MAC source addr, we could use that...

	imepSetLinkInStatus(ih->saddr());
	// XXX: this could be done at the MAC layer.  In fact, I will 
	// augment the IEEE 802.11 layer so that the receipt of an
	// ACK confirms bidirectional status. -josh
	// hasn't actually be done. seems unlikely to be of help, and is
	// fairly hard to do. -dam 8/19/98
       	assert(ch->ptype() == PT_IMEP);
	assert(im->imep_version == IMEP_VERSION);

	if(im->imep_block_flags == 0) {
		imep_beacon_input(p);
		Packet::free(p);
		return;
	}

	if(im->imep_block_flags & BLOCK_FLAG_ACK)
		imep_ack_input(p);

	if(im->imep_block_flags & BLOCK_FLAG_HELLO)
		imep_hello_input(p);

	if(im->imep_block_flags & BLOCK_FLAG_OBJECT) {
		imep_object_input(p);
		// each upper layer object will be decapsulated and
		// placed into its own packet before being passed
		// to the upper layer.  This provides total transparency
		// to the upper layer.
	}

	Packet::free(p);
}

void
imepAgent::imep_beacon_input(Packet *p)
{
	struct hdr_ip *ip = HDR_IP(p);

	sendHello(ip->saddr());
}


// If there is an ACK for us we need to (1) removed the sender
// of the ACK from the "ack list", and we need to update the
// status of this neighbor to "BIDIRECTIONAL".
void
imepAgent::imep_ack_input(Packet *p)
{
	struct hdr_ip *ih = HDR_IP(p);
	struct imep_ack_block *ab = findAckBlock(p);
	struct imep_ack *ack;

	assert(ab);
	ack = (struct imep_ack*) (ab + 1);

	// According to the IMEP specs, the ACK block (if it exists)
	// immediately follows the 3-byte IMEP header.

	for(int i = 0; i < ab->ab_num_acks; i++, ack++) {
		if(INT32_T(ack->ack_ipaddr) == ipaddr) {

			Packet *p0 = findObjectSequence(ack->ack_seqno);
			if (NULL == p0)
			  {
			    if(verbose) 
			      trace("T %.9f _%d_ %d acks seq %d : no obj"
				    " block", CURRENT_TIME, ipaddr, 
				    ih->saddr(), ack->ack_seqno);
			    stats.num_unexpected_acks++;
			    continue;
			  }

			removeObjectResponse(p0, ih->saddr());

			imepSetLinkBiStatus(ih->saddr());
		}
	}
}

void
imepAgent::imep_hello_input(Packet *p)
{
	struct hdr_ip *ip = HDR_IP(p);
	struct imep_hello_block *hb = findHelloBlock(p);
	struct imep_hello *hello;

	assert(hb);
	hello = (struct imep_hello*) (hb + 1);

	for(int i = 0; i < hb->hb_num_hellos; i++, hello++) {
		if(INT32_T(hello->hello_ipaddr) == ipaddr) {

			imepSetLinkBiStatus(ip->saddr());

			break;
		}
	}
}

void
imepAgent::imep_object_input(Packet *p)
{
	struct imep_object_block *ob;

	// First, send an ack for the object
	imep_ack_object(p);

	// now see what to do with the object
	ob = findObjectBlock(p);
	assert(ob);

	struct hdr_ip *iph = HDR_IP(p);
	imepLink *l = findLink(iph->saddr());
	assert(l);  // if we have an object, a link entry should already exist

	if (!l->lastSeqValid()) 
	  { // first object we've heard from this node
	    l->lastSeqValid() = 1;
	    l->lastSeq() = ob->ob_sequence - 1;
	    if (verbose)
	      trace("T %.9f _d_ first object from neighbor %d seq %d",
		    CURRENT_TIME, ipaddr, iph->saddr(), ob->ob_sequence);
	  }

	// This calc requires sequence number SEQ_GT() semantics
	// Life will be very bad if this calc isn't actually done in 
	// a register the size of the sequence number space
	int8_t reg = (int8_t) ob->ob_sequence - (int8_t) l->lastSeq();

	if(reg <= 0)
	  { // already passed this pkt up to ULP or declared it a permenant
	    // hole
	    if (verbose)
	      trace("T %.9f _%d_ from %d ignored seq %d (already heard)",
		    CURRENT_TIME, ipaddr, iph->saddr(), ob->ob_sequence);
	    stats.num_out_of_window_objs++;
	    return;
	  }

	if (verbose && reg > 1)
	  { // found a hole in the sequence number space...
	    trace("T %.9f _%d_ inorder - src %d seq %d out of order (%d expected)",
		  CURRENT_TIME, ipaddr, iph->saddr(),
		  ob->ob_sequence, l->lastSeq()+1);
	  }

	if (1 == reg)
	  { // ``fast path''
	    // got the expected next seq num

	    if (verbose)
	      trace("T %.9f _%d_ inorder - fastpath src %d seq %d (delivering)",
		    CURRENT_TIME, ipaddr, HDR_IP(p)->saddr(), ob->ob_sequence);
	    stats.num_in_order_objs++;

	    imep_object_process(p);
	    assert((u_int8_t)(l->lastSeq() + 1) == ob->ob_sequence);
	    l->lastSeq() = ob->ob_sequence;
	  }
	else
	  {
	    // put this packet on the resequencing queue
	    scheduleIncoming(p->copy(), ob->ob_sequence);
	    stats.num_out_of_order_objs++;
	  }

	// now deliver as many in-sequence packets to ULP as possible
	Packet *p0;
	while((p0 = incomingQ.getPacket(iph->saddr(), l->lastSeq() + 1)))
	  {
	    stats.num_recvd_from_queue++;
	    if (verbose)
	      trace("T %.9f _%d_ inorder - src %d seq %d (delivering)",
		    CURRENT_TIME, ipaddr, HDR_IP(p0)->saddr(), l->lastSeq() + 1);
	    l->lastSeq() += 1;
	    imep_object_process(p0);
	    Packet::free(p0);
	  }
}

void
imepAgent::imep_object_process(Packet *p)
  // hand the conents of any object in the pkt to the respective ULP
{
  struct imep_object_block *ob;
  struct imep_object *object;
  int i;

  stats.num_object_pkts_recvd++;

  ob = findObjectBlock(p);
  assert(ob);
  assert(ob->ob_protocol_type == PROTO_TORA); // XXX: more general later

  object = (struct imep_object*) (ob + 1);

  for(i = 0; i < ob->ob_num_objects; i++)
    {
      Packet *p0 = p->copy();
		
      assert(object->o_length > 0); // sanity check
		
      toraCreateHeader(p0,
		       ((char*) object) + IMEP_OBJECT_SIZE,
		       object->o_length);
		
      imep_input(p0);
		
      object = (struct imep_object*) ((char*) object + 
             IMEP_OBJECT_SIZE + object->o_length);
    }
}


void
imepAgent::imep_ack_object(Packet *p)
  // send an ack for the object in p, if any
{
  struct hdr_ip *iph = HDR_IP(p);
  struct imep_object_block *ob;
  struct imep_object *object;
  int i;

  ob = findObjectBlock(p);
  if (!ob) return;

  if (0 == ob->ob_num_responses) 
    return;

  if (31 == ob->ob_num_responses) 
    { // a ``broadcast'' response list to which everyone replies
      sendAck(iph->saddr(), ob->ob_sequence);
      return;
    }

  object = (struct imep_object*) (ob + 1);

  // walk the objects to find the response list
  for(i = 0; i < ob->ob_num_objects; i++)
    {		
      object = (struct imep_object*) ((char*) object + 
	      IMEP_OBJECT_SIZE +object->o_length);
    }

  struct imep_response *r = (struct imep_response*) object;
  for (i = 0; i < ob->ob_num_responses; i++)
    {
      if (INT32_T(r->resp_ipaddr) == ipaddr)
	{
	  sendAck(iph->saddr(), ob->ob_sequence);
	  break;
	}
      r = r + 1;
    }
}

// ======================================================================
// ======================================================================
// Routines by which packets leave the IMEP object

void
imepAgent::imep_input(Packet *p)
{
	recvtarget_->recv(p, (Handler*) 0);
}

void
imepAgent::imep_output(Packet *p)
{
	struct hdr_cmn *ch = HDR_CMN(p);

	if(imep_use_mac_callback) {
		ch->xmit_failure_ = imep_failed_callback;
		ch->xmit_failure_data_ = (void*) this;
	} else {
		ch->xmit_failure_ = 0;
		ch->xmit_failure_data_ = 0;
	}
	ch->xmit_reason_ = 0;

 	sendtarget_->recv(p, (Handler*) 0);
}

// ======================================================================
// ======================================================================
// Debugging routines

void
imepAgent::imep_dump_header(Packet *p)
{
	struct hdr_imep *im = HDR_IMEP(p);

	fprintf(stderr,
		"imep_version: 0x%x\n", im->imep_version);
	fprintf(stderr,
		"imep_block_flags: 0x%x\n", im->imep_block_flags);
	fprintf(stderr,
		"imep_length: 0x%04x\n", U_INT16_T(im->imep_length));

	Packet::dump_header(p, hdr_imep::offset_, 64);
}

void
imepAgent::log_neighbor_list()
{
	imepLink *l;
	int offset = 0;

        if(! verbose ) return;
	
        sprintf(logtarget_->pt_->buffer(),
                "T %.9f _%d_ neighbors: ", CURRENT_TIME, ipaddr);

	for(l = imepLinkHead.lh_first; l; l = l->link.le_next) {
	  offset = strlen(logtarget_->pt_->buffer());
	  sprintf(logtarget_->pt_->buffer() + offset,
		  "%d%c ", l->index(),
		  l->status() == LINK_BI ? '+' : 
		    (l->status() == LINK_IN ? '-' : 
		     (l->status() == LINK_OUT ? '|' : 'X')));
	}
        logtarget_->pt_->dump();
}

void
imepAgent::trace(char* fmt, ...)
{
  va_list ap;
  
  if (!logtarget_) return;

  va_start(ap, fmt);
  vsprintf(logtarget_->pt_->buffer(), fmt, ap);
  logtarget_->pt_->dump();
  va_end(ap);
}


char *
imepAgent::dumpResponseList(Packet *p)
{
  static char buf[512];
  char *ptr = buf;
  struct imep_object_block *ob = findObjectBlock(p);
  struct imep_response *r = findResponseList(p);
  struct imep_response *r0;
  int i;

  for(i = 0, r0 = r; i < ob->ob_num_responses; i++, r0++) 
    {
      ptr += (int)sprintf(ptr,"%d ", INT32_T(r0->resp_ipaddr));
    }
  return buf;
}


void
imepAgent::Terminate()
{
  trace("IL %.9f _%d_ Add-Adj: %d New-Neigh: %d Del-Neigh1: %d Del-Neigh2: %d Del-Neigh3: %d",
	CURRENT_TIME, ipaddr, 
	stats.new_in_adjacency,
	stats.new_neighbor      ,
	stats.delete_neighbor1      ,
	stats.delete_neighbor2,
	stats.delete_neighbor3);

  trace("IL %.9f _%d_ Created QRY: %d UPD: %d CLR: %d",CURRENT_TIME, ipaddr,
	stats.qry_objs_created      ,
	stats.upd_objs_created      ,
	stats.clr_objs_created);

  trace("IL %.9f _%d_ Received QRY: %d UPD: %d CLR: %d",CURRENT_TIME, ipaddr,
	stats.qry_objs_recvd      ,
	stats.upd_objs_recvd      ,
	stats.clr_objs_recvd);

  trace("IL %.9f _%d_ Total-Obj-Created: %d Obj-Pkt-Created: %d Obj-Pkt-Recvd: %d",
	CURRENT_TIME, ipaddr,
	stats.num_objects_created      ,
	stats.num_object_pkts_created      ,
	stats.num_object_pkts_recvd);

  trace("IL %.9f _%d_ Rexmit Pkts: %d Acked: %d Retired: %d Rexmits: %d",
	CURRENT_TIME, ipaddr,
	stats.num_rexmitable_pkts      ,
	stats.num_rexmitable_fully_acked      ,
	stats.num_rexmitable_retired      ,
	stats.num_rexmits);


  trace("IL %.9f _%d_ Sum-Response-List-Size Created: %d Retired: %d",
	CURRENT_TIME, ipaddr,
	stats.sum_response_list_sz      ,
	stats.sum_rexmitable_retired_response_sz);

  trace("IL %.9f _%d_ Holes Created: %d Retired: %d ReSeqQ-Drops: %d ReSeqQ-Recvd: %d",
	CURRENT_TIME, ipaddr,
	stats.num_holes_created      ,
	stats.num_holes_retired      ,
	stats.num_reseqq_drops,
	stats.num_recvd_from_queue);

  trace("IL %.9f _%d_ Unexpected-Acks: %d Out-Win-Obj: %d Out-Order-Obj: %d In-Order-Obj: %d", CURRENT_TIME, ipaddr,
	stats.num_unexpected_acks      ,
	stats.num_out_of_window_objs      ,
	stats.num_out_of_order_objs      ,
	stats.num_in_order_objs);
}
