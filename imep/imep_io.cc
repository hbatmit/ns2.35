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
  imep_io.cc
  $Id: imep_io.cc,v 1.4 1999/10/13 22:53:06 heideman Exp $

  marshall IMEP packets 
*/

#include <random.h>
#include <packet.h>
#include <imep/imep.h>

// ======================================================================
// ======================================================================
// Outgoing Packets

void
imepAgent::sendBeacon()
{
	Packet *p = Packet::alloc();
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_imep *im = HDR_IMEP(p);

	ch->ptype() = PT_IMEP;
	ch->size() = BEACON_HDR_LEN;
	ch->iface() = -2;
	ch->error() = 0;
	ch->addr_type() = NS_AF_NONE;
        ch->prev_hop_ = ipaddr;
	ch->uid() = uidcnt_++;

	ih->saddr() = ipaddr;
	ih->daddr() = IP_BROADCAST;
	ih->sport() = RT_PORT;
	ih->dport() = RT_PORT;
	ih->ttl_ = 1;

	im->imep_version = IMEP_VERSION;
	im->imep_block_flags = 0x00;
	U_INT16_T(im->imep_length) = sizeof(struct hdr_imep);

	imep_output(p);
}

void
imepAgent::sendHello(nsaddr_t index)
{
	Packet *p = Packet::alloc();
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_imep *im = HDR_IMEP(p);
	struct imep_hello_block *hb = (struct imep_hello_block*) (im + 1);
	struct imep_hello *hello = (struct imep_hello*) (hb + 1);

	ch->ptype() = PT_IMEP;
	ch->size() = HELLO_HDR_LEN;
	ch->iface() = -2;
	ch->error() = 0;
	ch->addr_type() = NS_AF_NONE;
        ch->prev_hop_ = ipaddr;
	ch->uid() = uidcnt_++;

	ih->saddr() = ipaddr;
	ih->daddr() = IP_BROADCAST;
	ih->sport() = RT_PORT;
	ih->dport() = RT_PORT;
	ih->ttl_ = 1;

	im->imep_version = IMEP_VERSION;
	im->imep_block_flags = BLOCK_FLAG_HELLO;
	U_INT16_T(im->imep_length) = sizeof(struct hdr_imep) +
		sizeof(struct imep_hello_block) + sizeof(struct imep_hello);

	hb->hb_num_hellos = 1;

	INT32_T(hello->hello_ipaddr) = index;

	helloQueue.enque(p);
	// aggregate as many control messages as possible before sending

	if(controlTimer.busy() == 0) {
		controlTimer.start(MIN_TRANSMIT_WAIT_TIME_LOWP
			 + ((MAX_TRANSMIT_WAIT_TIME_LOWP - MIN_TRANSMIT_WAIT_TIME_LOWP)
			 * Random::uniform()));
	}

}

void
imepAgent::sendAck(nsaddr_t index, u_int32_t seqno)
{
	Packet *p = Packet::alloc();
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_imep *im = HDR_IMEP(p);
	struct imep_ack_block *ab = (struct imep_ack_block*) (im + 1);
	struct imep_ack *ack = (struct imep_ack*) (ab + 1);

	ch->ptype() = PT_IMEP;
	ch->size() = ACK_HDR_LEN;
	ch->iface() = -2;
	ch->error() = 0;
	ch->addr_type() = NS_AF_NONE;
        ch->prev_hop_ = ipaddr;
	ch->uid() = uidcnt_++;

	ih->saddr() = ipaddr;
	ih->daddr() = IP_BROADCAST;
	ih->sport() = RT_PORT;
	ih->dport() = RT_PORT;
	ih->ttl_ = 1;

	im->imep_version = IMEP_VERSION;
	im->imep_block_flags = BLOCK_FLAG_ACK;
	U_INT16_T(im->imep_length) = sizeof(struct hdr_imep) +
		sizeof(struct imep_ack_block) + sizeof(struct imep_ack)
;
	ab->ab_num_acks = 1;
	ack->ack_seqno = seqno;
	INT32_T(ack->ack_ipaddr) = index;

	// aggregate as many control messages as possible before sending
	ackQueue.enque(p);
	if(controlTimer.busy() == 0) {
		controlTimer.start(MIN_TRANSMIT_WAIT_TIME_LOWP
			 + ((MAX_TRANSMIT_WAIT_TIME_LOWP - MIN_TRANSMIT_WAIT_TIME_LOWP)
			 * Random::uniform()));
	}
}
