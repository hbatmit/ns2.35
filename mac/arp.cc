/*-*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
 *
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
 *
 * $Header: /cvsroot/nsnam/ns-2/mac/arp.cc,v 1.12 2005/04/26 18:56:35 haldar Exp $
 */


/* 
 * Ported from CMU/Monarch's code, nov'98 -Padma.
 *
 * basic arp cache and MAC addr resolution
 *
 * Note: code in this file violates the convention that addresses of
 * type Af_INET stored in nsaddr_t variables are stored in 24/8 format.
 * Many variables in nsaddr_t's in this file store ip addrs as simple ints.
 */

#include <errno.h>

#include "delay.h"
//#include "debug.h"
#include "mac.h"
#include "arp.h"
#include "topography.h"
#include "cmu-trace.h"
#include "mobilenode.h"
#include "ll.h"
#include "packet.h"
#include <address.h>

// #define DEBUG

static class ARPTableClass : public TclClass {
public:
        ARPTableClass() : TclClass("ARPTable") {}
        TclObject* create(int, const char*const* argv) {
                return (new ARPTable(argv[4], argv[5]));
        }
} class_arptable;

int hdr_arp::offset_;

static class ARPHeaderClass : public PacketHeaderClass {
public:
        ARPHeaderClass() : PacketHeaderClass("PacketHeader/ARP",
                                             sizeof(hdr_arp)) { 
		bind_offset(&hdr_arp::offset_);
	}
} class_arphdr;

/* ======================================================================
   Address Resolution (ARP) Table
   ====================================================================== */

ARPTable_List ARPTable::athead_ = { 0 };

void
ARPTable::Terminate()
{
	ARPEntry *ll;
	for(ll = arphead_.lh_first; ll; ll = ll->arp_link_.le_next) {
		if(ll->hold_) {
			drop(ll->hold_, DROP_END_OF_SIMULATION);
			ll->hold_ = 0;
		}
	}
}


ARPTable::ARPTable(const char *tclnode, const char *tclmac) : LinkDelay() {
	LIST_INIT(&arphead_);

        node_ = (MobileNode*) TclObject::lookup(tclnode);
	assert(node_);

	mac_ = (Mac*) TclObject::lookup(tclmac);
	assert(mac_);
	LIST_INSERT_HEAD(&athead_, this, link_);
}

int
ARPTable::command(int argc, const char*const* argv)
{
	if (argc == 2 && strcasecmp(argv[1], "reset") == 0) {
		Terminate();
		//FALL-THROUGH to give parents a chance to reset
	}
	return LinkDelay::command(argc, argv);
}

int
ARPTable::arpresolve(nsaddr_t dst, Packet *p, LL *ll)
{
        ARPEntry *llinfo ;
	
	assert(initialized());
	llinfo = arplookup(dst);

#ifdef DEBUG
        fprintf(stderr, "%d - %s\n", node_->address(), __FUNCTION__);
#endif
	
	if(llinfo && llinfo->up_) {
		mac_->hdr_dst((char*) HDR_MAC(p), llinfo->macaddr_);
		return 0;
	}

	if(llinfo == 0) {
		/*
		 *  Create a new ARP entry
		 */
		llinfo = new ARPEntry(&arphead_, dst);
	}

        if(llinfo->count_ >= ARP_MAX_REQUEST_COUNT) {
                /*
                 * Because there is not necessarily a scheduled event between
                 * this callback and the point where the callback can return
                 * to this point in the code, the order of operations is very
                 * important here so that we don't get into an infinite loop.
                 *                                      - josh
                 */
                Packet *t = llinfo->hold_;

                llinfo->count_ = 0;
                llinfo->hold_ = 0;
		hdr_cmn* ch;
		
                if(t) {
                        ch = HDR_CMN(t);

                        if (ch->xmit_failure_) {
                                ch->xmit_reason_ = 0;
                                ch->xmit_failure_(t, ch->xmit_failure_data_);
                        }
                        else {
                                drop(t, DROP_IFQ_ARP_FULL);
                        }
                }

                ch = HDR_CMN(p);

		if (ch->xmit_failure_) {
                        ch->xmit_reason_ = 0;
                        ch->xmit_failure_(p, ch->xmit_failure_data_);
                }
                else {
                        drop(p, DROP_IFQ_ARP_FULL);
                }

                return EADDRNOTAVAIL;
        }

	llinfo->count_++;
	if(llinfo->hold_)
		drop(llinfo->hold_, DROP_IFQ_ARP_FULL);
	llinfo->hold_ = p;

	/*
	 *  We don't have a MAC address for this node.  Send an ARP Request.
	 *
	 *  XXX: Do I need to worry about the case where I keep ARPing
	 *	 for the SAME destination.
	 */
	int src = node_->address(); // this host's IP addr
	arprequest(src, dst, ll);
	return EADDRNOTAVAIL;
}


ARPEntry*
ARPTable::arplookup(nsaddr_t dst)
{
	ARPEntry *a;

	for(a = arphead_.lh_first; a; a = a->nextarp()) {
		if(a->ipaddr_ == dst)
			return a;
	}
	return 0;
}


void
ARPTable::arprequest(nsaddr_t src, nsaddr_t dst, LL *ll)
{
	Scheduler& s = Scheduler::instance();
	Packet *p = Packet::alloc();

	hdr_cmn *ch = HDR_CMN(p);
	char	*mh = (char*) HDR_MAC(p);
	hdr_ll	*lh = HDR_LL(p);
	hdr_arp	*ah = HDR_ARP(p);

	ch->uid() = 0;
	ch->ptype() = PT_ARP;
	ch->size() = ARP_HDR_LEN;
	ch->iface() = -2;
	ch->error() = 0;

	mac_->hdr_dst(mh, MAC_BROADCAST);
	mac_->hdr_src(mh, ll->mac_->addr());
	mac_->hdr_type(mh, ETHERTYPE_ARP);

	lh->seqno() = 0;
	lh->lltype() = LL_DATA;

	ch->direction() = hdr_cmn::DOWN; // send this pkt down
	ah->arp_hrd = ARPHRD_ETHER;
	ah->arp_pro = ETHERTYPE_IP;
	ah->arp_hln = ETHER_ADDR_LEN;
	ah->arp_pln = sizeof(nsaddr_t);
	ah->arp_op  = ARPOP_REQUEST;
	ah->arp_sha = ll->mac_->addr();
	ah->arp_spa = src;
	ah->arp_tha = 0;		// what were're looking for
	ah->arp_tpa = dst;

	s.schedule(ll->downtarget_, p, delay_);
}

void
ARPTable::arpinput(Packet *p, LL *ll)
{
	Scheduler& s = Scheduler::instance();
	hdr_arp *ah = HDR_ARP(p);
	ARPEntry *llinfo;

	assert(initialized());

#ifdef DEBUG
	fprintf(stderr,
                "%d - %s\n\top: %x, sha: %x, tha: %x, spa: %x, tpa: %x\n",
		node_->address(), __FUNCTION__, ah->arp_op,
                ah->arp_sha, ah->arp_tha, ah->arp_spa, ah->arp_tpa);
#endif

	if((llinfo = arplookup(ah->arp_spa)) == 0) {

		/*
		 *  Create a new ARP entry
		 */
		llinfo = new ARPEntry(&arphead_, ah->arp_spa);
	}
        assert(llinfo);

	llinfo->macaddr_ = ah->arp_sha;
	llinfo->up_ = 1;

	/*
	 *  Can we send whatever's being held?
	 */
	if(llinfo->hold_) {
		hdr_cmn *ch = HDR_CMN(llinfo->hold_);
		char *mh = (char*) HDR_MAC(llinfo->hold_);
                hdr_ip *ih = HDR_IP(llinfo->hold_);
                
		// XXXHACK for now: 
		// Future work: separate port-id from IP address ??
		int dst = Address::instance().get_nodeaddr(ih->daddr());
		
		if((ch->addr_type() == NS_AF_NONE &&
                    dst == ah->arp_spa) ||
                   (NS_AF_INET == ch->addr_type() &&
                    ch->next_hop() == ah->arp_spa)) {
#ifdef DEBUG
			fprintf(stderr, "\tsending HELD packet.\n");
#endif
			mac_->hdr_dst(mh, ah->arp_sha);
			//ll->hdr_dst(p, ah->arp_sha);
			
			s.schedule(ll->downtarget_, llinfo->hold_, delay_);
			llinfo->hold_ = 0;
		}
                else {
                        fprintf(stderr, "\tfatal ARP error...\n");
                        exit(1);
                }
	}

	if(ah->arp_op == ARPOP_REQUEST &&
		ah->arp_tpa == node_->address()) {
		
		hdr_cmn *ch = HDR_CMN(p);
		char	*mh = (char*)HDR_MAC(p);
		hdr_ll  *lh = HDR_LL(p);

		ch->size() = ARP_HDR_LEN;
		ch->error() = 0;
		ch->direction() = hdr_cmn::DOWN; // send this pkt down

		mac_->hdr_dst(mh, ah->arp_sha);
		mac_->hdr_src(mh, ll->mac_->addr());
		mac_->hdr_type(mh, ETHERTYPE_ARP);

		lh->seqno() = 0;
		lh->lltype() = LL_DATA;

		// ah->arp_hrd = 
		// ah->arp_pro =
		// ah->arp_hln =
		// ah->arp_pln =

		ah->arp_op  = ARPOP_REPLY;
		ah->arp_tha = ah->arp_sha;
		ah->arp_sha = ll->mac_->addr();

		nsaddr_t t = ah->arp_spa;
		ah->arp_spa = ah->arp_tpa;
		ah->arp_tpa = t;

		s.schedule(ll->downtarget_, p, delay_);
		return;
	}
	Packet::free(p);
}

