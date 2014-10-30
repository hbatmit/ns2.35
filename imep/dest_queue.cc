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
 */
/* Ported from CMU/Monarch's code*/

/* 
   dest_queue.cc
   $Id: dest_queue.cc,v 1.2 1999/08/12 21:17:11 yaxu Exp $
   
   implement a group of resequencing queues.  one for each source of packets.
   the name destination queue is a misnomer.
   */

#include <imep/dest_queue.h>
#ifdef TEST_ONLY
#include <stdio.h>
#else
#include <imep/imep.h>
#endif
#ifndef TEST_ONLY
#define CURRENT_TIME Scheduler::instance().clock()
#else
extern double CURRENT_TIME;
#endif


static const int verbose = 0;

//////////////////////////////////////////////////////////////////////
// Transmission Queue Entry
txent::txent(double e, u_int32_t s, Packet *p)
{
	expire_ = e;
	seqno_ = s;
	pkt_ = p;
}

//////////////////////////////////////////////////////////////////////
// Destination Queue Entry
dstent::dstent(nsaddr_t index)
{
	LIST_INIT(&txentHead);
	ipaddr_ = index;
	seqno_ = ILLEGAL_SEQ;
}

static int
SEQ_GT(u_int8_t a, u_int8_t b)
{
  int8_t reg = (int8_t)a - (int8_t) b;
  return reg > 0;
}
	
void
dstent::addEntry(double e, u_int32_t s, Packet *p)
{
	txent *t, *u, *v = 0;

	if((t = findEntry(s)) == 0) {
		t = new txent(e, s, p);
		assert(t);

		for(u = txentHead.lh_first; u; u = u->link.le_next) {
			if(SEQ_GT(u->seqno(), s))
			  break;
			v = u;
		}

		if(u == 0 && v == 0) {
			LIST_INSERT_HEAD(&txentHead, t, link);
		} else if(u) {
			LIST_INSERT_BEFORE(u, t, link);
		} else {
			assert(v);
			LIST_INSERT_AFTER(v, t, link);
		}

	} else {
		Packet::free(p);
		// already have a copy of this packet
	}

#ifdef  DEBUG
	// verify that I did not fuck up...
	u_int32_t max = 0;
	for(t = txentHead.lh_first; t; t = t->link.le_next) {
		if(max == 0)
			max = t->seqno();
		else {
			assert(t->seqno() > max);
			max = t->seqno();
		}
	} 
#endif
}			 

void
dstent::delEntry(txent *t)
{
	LIST_REMOVE(t, link);
	delete t;
}

txent*
dstent::findEntry(u_int32_t s)
{
	txent *t;

	for(t = txentHead.lh_first; t; t = t->link.le_next) {
		if(t->seqno() == s)
			return t;
	}
	return 0;
}

txent*
dstent::findFirstEntry(void)
{
	return txentHead.lh_first;
	// this gives the minimum sequence number for the destination
}

//////////////////////////////////////////////////////////////////////
// Destination Queue
dstQueue::dstQueue(imepAgent *a, nsaddr_t index) : agent_(a), ipaddr_(index)
{
	LIST_INIT(&dstentHead);
}

void
dstQueue::addEntry(nsaddr_t dst, double e, u_int32_t s, Packet *p)
{
	dstent *t;

	if((t = findEntry(dst)) == 0) {
		t = new dstent(dst);
		assert(t);
		LIST_INSERT_HEAD(&dstentHead, t, link);
	}

	if (NULL == t->txentHead.lh_first) agent_->stats.num_holes_created++;
	t->addEntry(e, s, p);
}

	
dstent*
dstQueue::findEntry(nsaddr_t dst)
{
	dstent *t;

	for(t = dstentHead.lh_first; t; t = t->link.le_next) {
		if(t->ipaddr() == dst)
			return t;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

Packet*
dstQueue::getPacket(nsaddr_t dst, u_int32_t seqno)
{
	dstent *d;
	txent *t;

	for(d = dstentHead.lh_first; d; d = d->link.le_next) {
		if(d->ipaddr() == dst)
			break;
	}

	if(d && (t = d->findEntry(seqno))) {
		Packet *p = t->pkt();
		d->delEntry(t);

		// make sure packets come out in increasing order only
		//	int8_t reg = (int8_t) seqno - (int8_t) d->seqno();
		//assert(reg > 0); // SEQ_GT(seqno, d->seqno())
		//d->seqno() = seqno; 

		return p;
	}
	return 0;
}

double
dstQueue::getNextExpire()
{
	dstent *t;
	Time min = 0.0;

	for(t = dstentHead.lh_first; t; t = t->link.le_next) {
		Time texp = t->expire(); // computed by traversing a list
		if(min == 0.0 || (texp && texp < min))
			min = texp;
	}
	if (verbose)
	  agent_->trace("T %.9f _%d_ dest_queue -  getNextExpire is %.9f",
			CURRENT_TIME, ipaddr_, min);

	return min;
}

Packet*
dstQueue::getNextPacket(u_int32_t& s)
{
	dstent *d;

	for(d = dstentHead.lh_first; d; d = d->link.le_next) {
	  txent *t = d->findFirstEntry();

	  if (t == 0) 
	    {
	      d->seqno() = ILLEGAL_SEQ;
	      continue;	// no packets here
	    }

	  Time texp = d->expire();
	  assert(texp);
#ifdef TEST_ONLY
	  fprintf(stderr,
		  "IN:\td->expire: %f, d->seqno: %d, t->expire: %f, t->seqno: %d\n",
		  d->expire(), d->seqno(), t->expire(), t->seqno());
#endif
	  if (texp > CURRENT_TIME && d->seqno() == ILLEGAL_SEQ) continue;

	  if (t->expire() <= CURRENT_TIME)
	    { // remember this seq as starting a chain we can pull off
	      d->seqno() = t->seqno();
	    }
	  else if (d->seqno() != ILLEGAL_SEQ && t->seqno() != (u_int8_t) (d->seqno() + 1))
	    { // the next pkt isn't part of the chain we were pulling off
	      // stop pulling off packets
	      d->seqno() = ILLEGAL_SEQ;
	      continue;		
	    }

 	    Packet *p = t->pkt();
	    assert(p);

	    s = t->seqno();

#ifdef TEST_ONLY
	    fprintf(stderr, "\t%s: returning seqno: %d\n",
		    __FUNCTION__, s);
#endif
	    if (d->seqno() != ILLEGAL_SEQ) 
	      { // advance d->seqno() along the chain
		d->seqno() = t->seqno();
	      }
	    
	    d->delEntry(t);
#ifdef TEST_ONLY
	  fprintf(stderr,
		  "OUT:\td->expire: %f, d->seqno: %d, t->expire: %f, t->seqno: %d\n",
		  d->expire(), d->seqno(), t->expire(), t->seqno());
#endif
	    return p;
	}
	return 0;
}

void 
dstQueue::deleteDst(nsaddr_t dst)
{
  dstent *d;
  txent *t;
  
  if (verbose)
    agent_->trace("T %.9f _%d_ purge dstQ id %d",
		  CURRENT_TIME, ipaddr_, dst);

  for(d = dstentHead.lh_first; d; d = d->link.le_next) 
    {
      if(d->ipaddr() == dst)
	break;
    }
  if (!d) return;

  while ((t = d->findFirstEntry()))
    {
      Packet *p = t->pkt();
      if (verbose)
	agent_->trace("T %.9f _%d_ dstQ id %d delete seq %d",
		      CURRENT_TIME, ipaddr_, dst, t->seqno());
      Packet::free(p);
      d->delEntry(t); 
      agent_->stats.num_reseqq_drops++;
    }

  LIST_REMOVE(d,link);
  delete d;
}

void
dstQueue::dumpAll()
{
	dstent *t;

	for(t = dstentHead.lh_first; t; t = t->link.le_next) {
	  if (verbose)
		agent_->trace("T %.9f _%d_ dest_queue - src %d expire %.9f seqno %d",
			      CURRENT_TIME, ipaddr_, t->ipaddr(), t->expire(), t->seqno());

		txent *u = t->findFirstEntry();
		for(;u; u = u->link.le_next) {
		  if(verbose)
		    agent_->trace("T %.9f _%d_ dest_queue - src %d seq %d expire %.9f",
				  CURRENT_TIME, ipaddr_, t->ipaddr(),
				  u->seqno(), u->expire());
		}
	}
}
