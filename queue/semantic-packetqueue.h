/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1996-1997 The Regents of the University of California.
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
 * 	This product includes software developed by the Daedalus 
 *      Research Group at the University of California, Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be used
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
 * @(#) $Header: /cvsroot/nsnam/ns-2/queue/semantic-packetqueue.h,v 1.9 2002/05/07 18:28:28 haldar Exp $ (UCB)
 */

#ifndef ns_semantic_packetqueue_h
#define ns_semantic_packetqueue_h

#include "object.h"
#include "connector.h"
#include "packet.h"
#include "queue.h"
#include "flags.h"
#include "random.h"
#include "tcp.h"

class AckReconsController;

/*
 * This flavor of PacketQueue includes several buffer management and
 * scheduling policies that are based on higher-layer semantics (e.g.,
 * TCP semantics) of packets.
 */
class SemanticPacketQueue : public PacketQueue {
  public:
	SemanticPacketQueue();
	int command(int argc, const char*const* argv);
	/* deque TCP acks before any other type of packet */
	Packet* deque_acksfirst();
	/* determine whether two packets belong to the same connection */
	inline int compareFlows(hdr_ip *ip1, hdr_ip *ip2) {
		return ((ip1->saddr() == ip2->saddr()) &&
			(ip1->sport() == ip2->sport()) &&
			(ip1->daddr() == ip2->daddr()) && 
			(ip1->dport() == ip2->dport())); 
	}
	/*
	 * When a new ack is enqued, purge older acks (as determined by the 
	 * sequence number of the ack field) from the queue. The enqued ack
	 * remains at the tail of the queue, unless replace_head is true, in 
	 * which case the new ack takes the place of the old ack closest to
	 * the head of the queue.
	 */
	void filterAcks(Packet *pkt, int replace_head); 

	/* check whether packet is marked */
	int isMarked(Packet *p);
	/* pick out the index'th packet of the right kind (marked/unmarked) */
	Packet *lookup(int index, int markedFlag);
	/* pick packet for ECN notification (either marking or dropping) */
	Packet *pickPacketForECN(Packet *pkt);
	/* pick a packet to drop when the queue is full */
	Packet *pickPacketToDrop();

	/* 
	 * Remove a specific packet given pointers to it and its predecessor
	 * in the queue. p and/or pp may be NULL.
	 */
	void remove(Packet* p, Packet* pp);

	inline Packet* head() { return head_; }

	/* count of packets of various types */
	int ack_count;		/* number of TCP acks in the queue */
	int data_count;		/* number of non-ack packets in the queue */
	int acks_to_send;	/* number of ack to send in current schedule */
	int marked_count_;	/* number of marked packets */
	int unmarked_count_;	/* number of unmarked packets */

	/* 
	 * These indicator variables are bound in derived objects and
	 * define queueing/scheduling polcies.
	 */
	int acksfirst_;		/* deque TCP acks before any other data */
	int filteracks_;	/* purge old acks when new one arrives */
	int reconsAcks_;	/* set up queue as an ack recontructor */
	int replace_head_;	/* new ack should take the place of old ack
				   closest to the head */
	int priority_drop_;	/* drop marked (low priority) packets first */
	int random_drop_;	/* pick packet to drop at random */
	int random_ecn_;	/* pick packet for ECN at random */
	virtual Packet* deque();
	Packet* enque(Packet *); // Returns prev tail
	virtual inline void enque_head(Packet *p) {
		if (len_ == 0)
			PacketQueue::enque(p);
		else {
			p->next_ = head_;
			head_ = p;
			++len_;
		}
	}
	virtual void remove(Packet *);
	AckReconsController *reconsCtrl_;
};

#endif
