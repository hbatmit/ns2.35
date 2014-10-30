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

/* -*- c++ -*-
   rexmit_queue.h
   $Id: dest_queue.h,v 1.3 2000/08/17 00:03:38 haoboy Exp $
   */

#ifndef __dest_queue_h__
#define __dest_queue_h__

#include <packet.h>
#include "lib/bsd-list.h"

#define ILLEGAL_SEQ 257

typedef double Time;

class txent;
class dstent;
class imepAgent;

//////////////////////////////////////////////////////////////////////
// Transmit Queue Entry
class txent {
	friend class dstent;
	friend class dstQueue;
protected:
	txent(double e, u_int32_t s, Packet *p);

	LIST_ENTRY(txent) link;
	// public so that I can use the list macros

	double expire() { return expire_; }
	u_int32_t seqno() { return seqno_; }
	Packet *pkt() { return pkt_; }

private:
	double expire_;		// time "p" expires and must be sent
	u_int32_t seqno_;	// sequence number of the packet "p".
	Packet *pkt_;
};

LIST_HEAD(txent_head, txent);

//////////////////////////////////////////////////////////////////////
// Destination Queue Entry
class dstent {
	friend class dstQueue;
protected:
	dstent(nsaddr_t index);

	LIST_ENTRY(dstent) link;

	void addEntry(double e, u_int32_t s, Packet *p);
	// add's a packet to the txentHead list.

	void delEntry(txent *t);
	// remove's a packet (transmit entry) from the txentHead list.

	txent* findEntry(u_int32_t s);
	// locates a packet with sequence "s" in the txentHead list.
	txent* findFirstEntry(void);

	nsaddr_t ipaddr() { return ipaddr_; }
	u_int32_t& seqno() { return seqno_; }

	inline double expire() {
		txent *t;
		double min = 0.0;

		for(t = txentHead.lh_first; t; t = t->link.le_next) {
			if(min == 0.0 || (t->expire() && t->expire() < min))
				min = t->expire();
		}
		return min;
	}
private:
	nsaddr_t ipaddr_;	// ip address of this destination
	txent_head txentHead;	// sorted by sequence number
	u_int32_t seqno_;	// last sequence number sent up the stack
};

LIST_HEAD(dstend_head, dstent);

//////////////////////////////////////////////////////////////////////
// Destination Queue
class dstQueue {
public:
	dstQueue(imepAgent *a, nsaddr_t index);

	void addEntry(nsaddr_t dst, double e, u_int32_t s, Packet *p);
	// add's a packet to the list.

	Packet* getPacket(nsaddr_t dst, u_int32_t seqno);
	// returns the packet with the following sequence nubmer "seqno".`
        // removes packet from queue

	Time getNextExpire();
	// returns the time that the next packet will expire.

	Packet* getNextPacket(u_int32_t& seqno);
	// returns "expired" packets (in sequence order, if any).

        void deleteDst(nsaddr_t dst);
        // remove and free all packets from dst

	void dumpAll(void);

private:
	dstent* findEntry(nsaddr_t dst);
	
	dstend_head dstentHead;		// head of the destination list
	imepAgent *agent_;
	nsaddr_t ipaddr_;
};

#endif // __dest_queue_h__
