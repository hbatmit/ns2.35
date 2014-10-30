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
 * $Header: /cvsroot/nsnam/ns-2/tora/tora.h,v 1.5 2000/09/01 03:04:12 haoboy Exp $
 */

#ifndef __tora_h__
#define __tora_h__

#include <packet.h>
#include <ip.h>
#include <cmu-trace.h>
#include <priqueue.h>
#include <rtqueue.h>
#include "lib/bsd-list.h"

#include <rtproto/rtproto.h>
#include <imep/imep.h>
#include <tora/tora_packet.h>
#include <tora/tora_neighbor.h>
#include <tora/tora_dest.h>

LIST_HEAD(td_head, TORADest);

//#define LOGGING

typedef double Time;

/*
 *  Causes TORA to discard any packets that have "looped".
 */
#define TORA_DISALLOW_ROUTE_LOOP

#define MinQueryRate		0.200	// seconds
// used to rate limit queries

class TORANeighbor;
class TORADest;
class Height;

class toraAgent : public rtAgent {
        friend class TORANeighbor;
        friend class TORADest;
public:
        toraAgent(nsaddr_t id);
        void recv(Packet* p, Handler*);
        int		command(int argc, const char*const* argv);

	// ============================================================
	// Routing API (see cmu/rtproto/rtproto.h)

	void rtNotifyLinkUP(nsaddr_t index);
	void rtNotifyLinkDN(nsaddr_t index);
	void rtNotifyLinkStatus(nsaddr_t index, u_int32_t status);
	void rtRoutePacket(Packet *p);

	// ============================================================

private:

        nsaddr_t        index;  // added for line 78 of tora.cc, needed for
                                // further verification

	TORADest*	dst_find(nsaddr_t id);
	TORADest*	dst_add(nsaddr_t id);
	void		dst_dump(void);

	void		rt_resolve(Packet *p);
        void            forward(Packet *p, nsaddr_t nexthop, Time delay = 0.0);

	void		purge_queue(void);
	void		enque(TORADest *td, Packet *p);
	Packet*		deque(TORADest *td);

	/*
	 * Incoming Packets
	 */
        void		recvTORA(Packet* p);
	void		recvQRY(Packet *p);
	void		recvUPD(Packet *p);
	void		recvCLR(Packet *p);

	/*
	 *  Outgoing Packets
	 */
	void		sendQRY(nsaddr_t id);
	void		sendUPD(nsaddr_t id);
	void		sendCLR(nsaddr_t id, double tau, nsaddr_t oid);
	void		tora_output(Packet *p);


	// ============================================================
	// ============================================================

	inline int initialized() {
		 return logtarget && ifqueue && imepagent;
	}

	td_head		dstlist;	// Active destinations

	imepAgent	*imepagent;
	// a handle to the IMEP layer

	/*
	 * A mechanism for logging the contents of the routing
	 * table.
	 */
	Trace		*logtarget;
	void trace(char* fmt, ...);
	virtual void reset();

        /*
         *  A "drop-front" queue used by the routing layer to buffer
         *  packets to which it does not have a route.
         */
        rtqueue        rqueue;

	/*
	 * A pointer to the network interface queue that sits
	 * between the "classifier" and the "link layer".
	 */
	PriQueue	*ifqueue;

        /*
         * Logging Routines
         */
	void		log_route_loop(nsaddr_t prev, nsaddr_t next);

	void		log_link_layer_feedback(Packet *p);
        void            log_link_layer_recycle(Packet *p);
        void            log_lnk_del(nsaddr_t dst);
        void            log_lnk_kept(nsaddr_t dst);

        void            log_nb_del(nsaddr_t dst, nsaddr_t id);
	void		log_recv_qry(Packet *p);
	void		log_recv_upd(Packet *p);
	void		log_recv_clr(Packet *p);
	void		log_route_table(void);

        void            log_dst_state_change(TORADest *td);

	void		logNextHopChange(TORADest *td);
	void		logNbDeletedLastDN(TORADest *td);
	void		logToraDest(TORADest *td);
	void		logToraNeighbor(TORANeighbor *tn);
};

#endif /* __tora_h__ */

