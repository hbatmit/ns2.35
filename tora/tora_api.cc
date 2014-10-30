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
   tora_api.cc
   $Id: tora_api.cc,v 1.3 1999/09/09 04:02:52 salehi Exp $
   
   Implement the API used by IMEP
   */


#include <tora/tora.h>

#define	CURRENT_TIME	Scheduler::instance().clock()

void
toraAgent::rtNotifyLinkUP(nsaddr_t index)
{
	TORADest *td = dstlist.lh_first;

        /*
         * Update the destination lists...
         */
	for( ; td; td = td->link.le_next) {
		if(td->nb_find(index) == 0) {
                        (void) td->nb_add(index);
		}
		if (td->rt_req) 
		  { // must send a new query for this dest so the new
		    // neighbor can hear it
		    // IMEP will take care of aggregating all these into
		    // one physical pkt

		    trace("T %.9f _%d_ QRY %d for %d (rtreq set)",
			  Scheduler::instance().clock(), ipaddr(), 
			  td->index, index);

		    sendQRY(td->index);
		    td->time_tx_qry = CURRENT_TIME;
		}
	}
}

void
toraAgent::rtNotifyLinkDN(nsaddr_t index)
{
	TORADest *td = dstlist.lh_first;

        /*
         *  Purge each Destination's Neighbor List
         */
        for( ; td; td = td->link.le_next) {
                if(td->nb_del(index)) {
                        /*
                         * Send an UPD packet if you've lost the last
                         * downstream link.
                         */
                        sendUPD(td->index);
                }
        }

	/*
	 *  Now purge the Network Interface queues that
	 *  may have packets destined for this broken
	 *  neighbor.
	 */
        {
                Packet *head = 0;
                Packet *p;
                Packet *np = 0;

                while((p = ifqueue->filter(index))) {
                        p->next_ = head;
                        head = p;
                }

                for(p = head; p; p = np) {
                        np = p->next_;
                        /*
                         * This make a lot of sense for TORA since we
                         * will almost always have multiple routes to
                         * a destination.
                         */
                        log_link_layer_recycle(p);
                        rt_resolve(p);
                }
        }
}


void
toraAgent::rtNotifyLinkStatus(nsaddr_t /* index */, u_int32_t /* status */)
{
	abort();
}

void
toraAgent::rtRoutePacket(Packet *p)
{
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ip = HDR_IP(p);

	// Non-data packets and BROADCAST packets can be dropped.
	if(DATA_PACKET(ch->ptype()) == 0 || 
	   ip->daddr() == (nsaddr_t) IP_BROADCAST) {
		drop(p, DROP_RTR_MAC_CALLBACK);
		return;
	}

	rt_resolve(p);
}
