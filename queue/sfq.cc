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
 * 	This product includes software developed by the MASH Research
 * 	Group at the University of California Berkeley.
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
 * This file contributed by Curtis Villamizar < curtis@ans.net >, Feb 1997.
 */

#ifndef lint
static const char rcsid[] =
"@(#) $Header: /cvsroot/nsnam/ns-2/queue/sfq.cc,v 1.12 2006/02/21 15:20:19 mahrenho Exp $ (ANS)";
#endif

#include <stdlib.h>

#include "config.h"
#include "queue.h"

class PacketSFQ;		// one queue
class SFQ;			// a set of SFQ queues

class PacketSFQ : public PacketQueue {
  PacketSFQ() : pkts(0), prev(0), next(0) {}
  friend class SFQ;
protected:
  void sfqdebug();
  int pkts;
  PacketSFQ *prev;
  PacketSFQ *next;
  inline PacketSFQ * activate(PacketSFQ *head) {
    if (head) {
      this->prev = head->prev;
      this->next = head;
      head->prev->next = this;
      head->prev = this;
      return head;
    }
    this->prev = this;
    this->next = this;
    return this;
  }
  inline PacketSFQ * idle(PacketSFQ *head) {
    if (head == this) {
      if (this->next == this)
	return 0;
      this->next->prev = this->prev;
      this->prev->next = this->next;
      return this->next;
    }
    return head;
  }
};

class SFQ : public Queue {
public: 
  SFQ();
  virtual int command(int argc, const char*const* argv);
  Packet *deque(void);
  void enque(Packet *pkt);
protected:
  int maxqueue_;		// max queue size in packets
  int buckets_;			// number of queues
  PacketSFQ *bucket;
  void initsfq();
  void clear();
  int hash(Packet *);
  PacketSFQ *active;
  int occupied;
  int fairshare;
};

static class SFQClass : public TclClass {
public:
  SFQClass() : TclClass("Queue/SFQ") {}
  TclObject* create(int, const char*const*) {
    return (new SFQ);
  }
} class_sfq;

SFQ::SFQ()
{
  maxqueue_ = 40;
  buckets_ = 16;
  bucket = 0;
  active = 0;
  bind("maxqueue_", &maxqueue_);
  bind("buckets_", &buckets_);
}

void SFQ::clear()
{
  PacketSFQ *q = bucket;
  int i = buckets_;

  if (!q)
    return;
  while (i) {
    if (q->pkts) {
      fprintf(stderr, "SFQ changed while queue occupied\n");
      exit(1);
    }
    ++q;
  }
  delete[](bucket);
  bucket = 0;
}

void SFQ::initsfq()
{
  bucket = new PacketSFQ[buckets_];
  active = 0;
  occupied = 0;
  fairshare = maxqueue_ / buckets_;
  // fprintf(stderr, "SFQ initsfq: %d %d\n", maxqueue_, buckets_);
}

/*
 * This implements the following tcl commands:
 *  $sfq limit $size
 *  $sfq buckets $num
 */
int SFQ::command(int argc, const char*const* argv)
{
  if (argc == 3) {
    if (strcmp(argv[1], "limit") == 0) {
      maxqueue_ = atoi(argv[2]);
      fairshare = maxqueue_ / buckets_;
      return (TCL_OK);
    }
    if (strcmp(argv[1], "buckets") == 0) {
      clear();
      buckets_ = atoi(argv[2]);
      return (TCL_OK);
    }
  }
  return (Queue::command(argc, argv));
}

void PacketSFQ::sfqdebug()
{
  PacketSFQ *q = this;
  fprintf(stderr, "sfq: ");
  while (q) {
	  fprintf(stderr, " 0x%p(%d)", static_cast<void *>(q), q->pkts);
	  q = q->next;
	  if (q == this)
		  break;
  }
  fprintf(stderr, "\n");
}

Packet* SFQ::deque(void)
{
  Packet* pkt;

  if (!bucket)
    initsfq();
  if (!active) {
    // fprintf(stderr, "    dequeue: empty\n");
    return (0);
  }
  --active->pkts;
  --occupied;
  pkt = active->deque();
  // fprintf(stderr, "dequeue 0x%x(%d): 0x%x\n",
  //	  (int)active, active->pkts, (int)pkt);
  // active->sfqdebug();
  if (active->pkts == 0)
    active = active->idle(active);
  else
    active = active->next;
  return pkt;
}

void SFQ::enque(Packet* pkt)
{
  int which;
  PacketSFQ *q;
  int used, left;

  if (!bucket)
    initsfq();
  which = hash(pkt) % buckets_;
  q = &bucket[which];
  // log_packet_arrival(pkt);
  used = q->pkts;
  left = maxqueue_ - occupied;
  // note: if maxqueue_ is changed while running left can be < 0
  if ((used >= (left >> 1))
      || (left < buckets_ && used > fairshare)
      || (left <= 0)) {
    // log_packet_drop(pkt);
    drop(pkt);
    // fprintf(stderr, "    drop: 0x%x\n", (int)pkt);
    return;
  }
  q->enque(pkt);
  ++occupied;
  ++q->pkts;
  if (q->pkts == 1)
    active = q->activate(active);
  // fprintf(stderr, "    enqueue(%d=%d): 0x%x\n", which, q->pkts, (int)pkt);
  // active->sfqdebug();
}

int SFQ::hash(Packet* pkt)
{
  hdr_ip* iph = hdr_ip::access(pkt);
  int i = (int)iph->saddr();
  int j = (int)iph->daddr();
  int k = i + j;

  return (k + (k >> 8) + ~(k >> 4)) % ((2<<19)-1); // modulo a large prime
}
