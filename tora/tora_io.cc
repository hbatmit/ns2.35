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
  $Id: tora_io.cc,v 1.4 1999/10/13 22:53:11 heideman Exp $
  
  marshall TORA packets 
  */

#include <tora/tora_packet.h>
#include <tora/tora.h>

/* ======================================================================
   Outgoing Packets
   ====================================================================== */
void
toraAgent::sendQRY(nsaddr_t id)
{
	Packet *p = Packet::alloc();
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_tora_qry *th = HDR_TORA_QRY(p);

	ch->uid() = uidcnt_++;
	ch->ptype() = PT_TORA;
	ch->size() = QRY_HDR_LEN;
	ch->iface() = -2;
	ch->error() = 0;
	ch->addr_type() = NS_AF_NONE;
        ch->prev_hop_ = ipaddr();

	ih->saddr() = ipaddr();
	ih->daddr() = IP_BROADCAST;
	ih->sport() = RT_PORT;
	ih->dport() = RT_PORT;
	ih->ttl_ = 1;

	th->tq_type = TORATYPE_QRY;
	th->tq_dst = id;

	trace("T %.9f _%d_ tora sendQRY %d",
	      Scheduler::instance().clock(), ipaddr(), id);

	tora_output(p);
}


void
toraAgent::sendUPD(nsaddr_t id)
{
	Packet *p = Packet::alloc();
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_tora_upd *th = HDR_TORA_UPD(p);
	TORADest *td;

	td = dst_find(id);
	assert(td);

	ch->uid() = uidcnt_++;
	ch->ptype() = PT_TORA;
	ch->size() = UPD_HDR_LEN;
	ch->iface() = -2;
	ch->error() = 0;
	ch->addr_type() = NS_AF_NONE;
        ch->prev_hop_ = ipaddr();

	ih->saddr() = ipaddr();
	ih->daddr() = IP_BROADCAST;
	ih->sport() = RT_PORT;
	ih->dport() = RT_PORT;
	ih->ttl_ = 1;

	th->tu_type = TORATYPE_UPD;
	th->tu_dst = id;
	th->tu_tau = td->height.tau;
	th->tu_oid = td->height.oid;
	th->tu_r = td->height.r;
	th->tu_delta = td->height.delta;
	th->tu_id = td->height.id;

	tora_output(p);
}


void
toraAgent::sendCLR(nsaddr_t id, double tau, nsaddr_t oid)
{
	Packet *p = Packet::alloc();
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_tora_clr *th = HDR_TORA_CLR(p);

	ch->uid() = uidcnt_++;
	ch->ptype() = PT_TORA;
	ch->size() = CLR_HDR_LEN;
	ch->iface() = -2;
	ch->error() = 0;
	ch->addr_type() = NS_AF_NONE;
        ch->prev_hop_ = ipaddr();

	ih->saddr() = ipaddr();
	ih->daddr() = IP_BROADCAST;
	ih->sport() = RT_PORT;
	ih->dport() = RT_PORT;
	ih->ttl_ = 1;

	th->tc_type = TORATYPE_CLR;
	th->tc_dst = id;
	th->tc_tau = tau;
	th->tc_oid = oid;

	tora_output(p);
}

void
toraAgent::tora_output(Packet *p)
{
	target_->recv(p, (Handler*) 0);
}

