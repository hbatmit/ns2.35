
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
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
/* Ported from CMU/Monarch's code, nov'98 -Padma.*/
/* -*- c++ -*-
   priqueue.h
   
   A simple priority queue with a remove packet function
*/
#ifndef _priqueue_h
#define _priqueue_h

#include "object.h"
#include "queue.h"
#include "drop-tail.h"
#include "packet.h"
#include "lib/bsd-list.h"

class PriQueue;
typedef int (*PacketFilter)(Packet *, void *);

LIST_HEAD(PriQueue_List, PriQueue);

class PriQueue : public DropTail {
public:
        PriQueue();

        int     command(int argc, const char*const* argv);
        void    recv(Packet *p, Handler *h);

        void    recvHighPriority(Packet *, Handler *);
        // insert packet at front of queue

        void filter(PacketFilter filter, void * data);
        // apply filter to each packet in queue, 
        // - if filter returns 0 leave packet in queue
        // - if filter returns 1 remove packet from queue

        Packet* filter(nsaddr_t id);

	void	Terminate(void);
private:
        int Prefer_Routing_Protocols;
 
	/*
	 * A global list of Interface Queues.  I use this list to iterate
	 * over all of the queues at the end of the simulation and flush
	 * their contents. - josh
	 */
public:
	LIST_ENTRY(PriQueue) link;
	static struct PriQueue_List prhead;
};

#endif /* !_priqueue_h */
