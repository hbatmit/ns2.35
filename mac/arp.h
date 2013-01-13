/*-*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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


#ifndef __arp_h__
#define __arp_h__

#include "scheduler.h"
#include "delay.h"
#include "lib/bsd-list.h"

#ifndef EADDRNOTAVAIL
#define EADDRNOTAVAIL 125
#endif /* !EADDRNOTAVAIL */

class LinkDelay;
class ARPEntry;
class ARPTable;
class LL;
class Mac;

LIST_HEAD(ARPTable_List, ARPTable);
LIST_HEAD(ARPEntry_List, ARPEntry);


/* ======================================================================
   Address Resolution (ARP) Header
   ====================================================================== */
#define ARPHRD_ETHER		1	/* ethernet hardware format */

#define ARPOP_REQUEST		1	/* request to resolve address */
#define ARPOP_REPLY		2	/* response to previous request */
#define ARPOP_REVREQUEST	3	/* request protocol address */
#define ARPOP_REVREPLY		4	/* response giving protocol address */
#define ARPOP_INVREQUEST	8	/* request to identify peer */
#define ARPOP_INVREPLY		9	/* response identifying peer */

#define ARP_HDR_LEN		28

struct hdr_arp {
	u_int16_t	arp_hrd;
	u_int16_t	arp_pro;
	u_int8_t	arp_hln;
	u_int8_t	arp_pln;
	u_int16_t	arp_op;
	int		arp_sha;
	u_int16_t	pad1;		// so offsets are correct
	nsaddr_t	arp_spa;
	int		arp_tha;
	u_int16_t	pad2;		// so offsets are correct
	nsaddr_t	arp_tpa;

	// Header access methods
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_arp* access(const Packet* p) {
		return (hdr_arp*) p->access(offset_);
	}
};

class ARPEntry {
	friend class ARPTable;
public:
	ARPEntry(ARPEntry_List* head, nsaddr_t dst) {
		up_ = macaddr_ = count_ = 0;
		ipaddr_ = dst;
		hold_ = 0;
		LIST_INSERT_HEAD(head, this, arp_link_);
	}
	inline ARPEntry* nextarp() { return arp_link_.le_next; }

private:
	LIST_ENTRY(ARPEntry)	arp_link_;

	int		up_;
	nsaddr_t	ipaddr_;
	int		macaddr_;
	Packet		*hold_;
	int		count_;
#define ARP_MAX_REQUEST_COUNT   3
};


class ARPTable : public LinkDelay {
public:
	ARPTable(const char *tclnode, const char *tclmac);

        int     command(int argc, const char*const* argv);
	int     arpresolve(nsaddr_t dst, Packet *p, LL *ll);
	void    arpinput(Packet *p, LL *ll);
	ARPEntry* arplookup(nsaddr_t dst);
	void arprequest(nsaddr_t src, nsaddr_t dst, LL *ll);

	void	Terminate(void);

private:
	inline int initialized() { return node_ && mac_; }

	ARPEntry_List	arphead_;
	MobileNode	*node_;
	Mac		*mac_;

	/*
	 * Used to purge all of the ll->hold packets at the end of the
	 * simulation.
	 */
public:
	LIST_ENTRY(ARPTable) link_;
	static ARPTable_List athead_;
};

#endif /* __arp_h__ */







