/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 The Regents of the University of California.
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
 * 	This product includes software developed by the Daedalus Research
 * 	Group at the University of California at Berkeley.
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
 * semantic-packetqueue.cc: contributed by the Daedalus Research Group, 
 * UC Berkeley (http://daedalus.cs.berkeley.edu).
 */

#include "ip.h"
#include "tcp.h"
#include "template.h"
#include "semantic-packetqueue.h"
#include "ack-recons.h"

static class SemanticPacketQueueClass : public TclClass {
public:
	SemanticPacketQueueClass() : TclClass("PacketQueue/Semantic") {}
	TclObject* create(int , const char*const*) {
		return (new SemanticPacketQueue());
	}
} class_semanticpacketqueue;

SemanticPacketQueue::SemanticPacketQueue() : ack_count(0), data_count(0), 
	acks_to_send(0), marked_count_(0), unmarked_count_(0) 
{
	bind_bool("acksfirst_", &acksfirst_);
	bind_bool("filteracks_", &filteracks_);
	bind_bool("reconsAcks_", &reconsAcks_);
	bind_bool("replace_head_", &replace_head_);
	bind_bool("priority_drop_", &priority_drop_);
	bind_bool("random_drop_", &random_drop_);
	bind_bool("random_ecn_", &random_ecn_);
}	

int
SemanticPacketQueue::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "ackrecons") == 0) {
			if ((reconsCtrl_ = (AckReconsController *) 
			     TclObject::lookup(argv[2]))) {
				reconsCtrl_->spq_ = this;
				reconsAcks_ = 1;
			}
		}
		return (TCL_OK);
	}
	return (TclObject::command(argc, argv));
}

/* 
 * Deque TCP acks before any other type of packet.
 */
Packet* 
SemanticPacketQueue::deque_acksfirst() {
	Packet* p = head_;
	Packet* pp = NULL;
	packet_t type;

	if (ack_count > 0) {
		while (p) {
			type = hdr_cmn::access(p)->ptype_;
			if (type == PT_ACK)
				break;
			pp = p;
			p = p->next_;
		}
		if (!p) 
			fprintf(stderr, "In deque_acksfirst(): ack_count: %d but no acks in queue, length = %d\n", ack_count, length());
		PacketQueue::remove(p, pp);
	} else {
		p = PacketQueue::deque();
	}
	return p;
}

/*
 * Purge the queue of acks that are older (i.e., have a smaller sequence 
 * number) than the most recent ack. If replace_head is set, the most recent
 * ack (pointed to by pkt) takes the place of the oldest ack that is purged. 
 * Otherwise, it remains at the tail of the queue.  pkt must be an ACK -- this
 * is checked by the caller.
 */
void
SemanticPacketQueue::filterAcks(Packet *pkt, int replace_head) 
{
	int done_replacement = 0;

	Packet *p, *pp, *new_p;
	hdr_tcp *tcph = hdr_tcp::access(pkt);
	int &ack = tcph->seqno();

	hdr_ip *iph = hdr_ip::access(pkt);
	for (p = head(), pp = p; p != 0; ) {
		/* 
		 * Check if packet in the queue belongs to the 
		 * same connection as the most recent ack
		 */
		if (compareFlows(hdr_ip::access(p), iph)) {
			/* check if queued packet is an ack */
			if (hdr_cmn::access(p)->ptype_==PT_ACK) {
				hdr_tcp *th = hdr_tcp::access(p);
				/* is this ack older than the current one? */
				if ((th->seqno() < ack) ||
				    (replace_head && th->seqno() == ack)) { 
					/* 
					 * If we haven't yet replaced the ack 
					 * closest to the head with the most 
					 * recent ack, do so now.
					 */
					if (replace_head && pkt != p &&
					    !done_replacement) {
						PacketQueue::remove(pkt);
						ack_count--; /* XXX */
						pkt->next_ = p;
						if (pp)
							pp->next_ = pkt;
						pp = pkt;
						done_replacement = 1;
						continue;
					} else if (done_replacement||pkt != p){
						new_p = p->next_;
						/* 
						 * If p is in scheduler queue,
						 * cancel the event. Also, 
						 * print out a warning because
						 * this should never happen.
						 */
						Scheduler &s = Scheduler::instance();
						if (s.lookup(p->uid_)) {
							s.cancel(p);
							fprintf(stderr, "Warning: In filterAcks(): packet being dropped from queue is in scheduler queue\n");
						}
						PacketQueue::remove(p, pp);
						/* XXX should drop, but we
						   don't have access to q */
						Packet::free(p); 
						ack_count--;
						p = new_p;
						continue;
					}
					if (ack_count <= 0)
						fprintf(stderr, 
							"oops! ackcount %d\n",
							ack_count);
				}
			}
		}
		pp = p;
		p = p->next_;
	}
}

/* check if packet is marked */
int
SemanticPacketQueue::isMarked(Packet *p) 
{
	return (hdr_flags::access(p)->fs_);
}


/* pick out the index'th of the appropriate kind (marked/unmarked) depending on markedFlag */
Packet*
SemanticPacketQueue::lookup(int index, int markedFlag) 
{
	if (index < 0) {
		fprintf(stderr, "In SemanticPacketQueue::lookup(): index = %d\n", index);
		return (NULL);
	}
	for (Packet* p = head_; p != 0; p = p->next_) {
		if (isMarked(p) == markedFlag)
			if (--index < 0)
				return (p);
	}
	return (NULL);
}

/*
 * If random_ecn_ is set, pick out the packet for ECN at random from among the
 * packets in the queue and the packet that just arrived ('pkt'). Otherwise, just
 * pick the packet that just arrived.
 */
Packet*
SemanticPacketQueue::pickPacketForECN(Packet* pkt) 
{
	Packet *victim;
	int victimIndex;

	if (random_ecn_) {
		victimIndex = Random::integer(length()+1);
		if (victimIndex == length())
			victim = pkt;
		else
			victim = PacketQueue::lookup(victimIndex);
	}
	else 
		victim = pkt;
	return (victim);
}
		

/* 
 * If priority_drop_ is set, drop marked packets before unmarked ones.
 * If in addition or separately random_drop_ is set, use randomization in
 * picking out the victim. XXX not used at present 
 */
Packet*
SemanticPacketQueue::pickPacketToDrop()
{
	Packet *victim;
	int victimIndex, victimMarked;

	if (!priority_drop_) {
		if (random_drop_)
			victim=PacketQueue::lookup(Random::integer(length()));
		else
			victim = PacketQueue::lookup(length() - 1);
	} else {
		/* if there are marked (low priority) packets */
		if (marked_count_) {
			victimMarked = 1;
			if (!random_drop_) 
				victimIndex = marked_count_ - 1;
			else
				victimIndex = Random::integer(marked_count_);
		}
		else {
			victimMarked = 0;
			if (!random_drop_)
				victimIndex = unmarked_count_ - 1;
			else
				victimIndex = Random::integer(unmarked_count_);
		}
		victim = lookup(victimIndex, victimMarked);
	}
	return (victim);
}
				
Packet*
SemanticPacketQueue::enque(Packet *pkt)
{
	if (reconsAcks_&&(hdr_cmn::access(pkt)->ptype_==PT_ACK)) {
		reconsCtrl_->recv(pkt);
		return NULL;
	}
	if (hdr_cmn::access(pkt)->ptype_ == PT_ACK)
		ack_count++;
	else
		data_count++;
	if (isMarked(pkt)) 
		marked_count_++;
	else
		unmarked_count_++;

	Packet* pt = PacketQueue::enque(pkt); /* actually enque the packet */

	if (filteracks_ && (hdr_cmn::access(pkt)->ptype_==PT_ACK))
		filterAcks(pkt, replace_head_);
	return pt;
}

Packet *
SemanticPacketQueue::deque()
{
	Packet *pkt;

	if (acksfirst_)
		pkt = deque_acksfirst();
	else
		pkt = PacketQueue::deque();
	
	if (pkt) {
		if (hdr_cmn::access(pkt)->ptype_ == PT_ACK)
			ack_count--;
		else
			data_count--;
		if (isMarked(pkt))
			marked_count_--;
		else
			unmarked_count_--;
	}
	return pkt;
}

void 
SemanticPacketQueue::remove(Packet *pkt)
{
	PacketQueue::remove(pkt);
	if (pkt) {
		if (hdr_cmn::access(pkt)->ptype_ == PT_ACK)
			ack_count--;
		else
			data_count--;
		if (isMarked(pkt))
			marked_count_--;
		else
			unmarked_count_--;
	}
}

