/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) Sun Microsystems, Inc. 1998 All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Sun Microsystems, Inc.
 *
 * 4. The name of the Sun Microsystems, Inc nor may not be used to endorse or
 *      promote products derived from this software without specific prior
 *      written permission.
 *
 * SUN MICROSYSTEMS MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS
 * SOFTWARE FOR ANY PARTICULAR PURPOSE.  The software is provided "as is"
 * without express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this software.
 */

// #ident "@(#)mip.cc  1.4     98/08/21 SMI"

#include <address.h>
#include "mip.h"


#define IP_HEADER_SIZE	20

int hdr_ipinip::offset_;
static class IPinIPHeaderClass : public PacketHeaderClass {
public:
        IPinIPHeaderClass() : PacketHeaderClass("PacketHeader/IPinIP",
					    sizeof(hdr_ipinip*)) {
		bind_offset(&hdr_ipinip::offset_);
	}
} class_ipiniphdr;

static class MIPEncapsulatorClass : public TclClass {
public:
	MIPEncapsulatorClass() : TclClass("MIPEncapsulator") {}
	TclObject* create(int, const char*const*) {
		return (new MIPEncapsulator());
	}
} class_mipencapsulator;

MIPEncapsulator::MIPEncapsulator() : Connector(), mask_(0xffffffff), 
	shift_(8), defttl_(32)
{
	bind("addr_", (int*)&(here_.addr_));
	bind("port_", (int*)&(here_.port_));
	bind("shift_", &shift_);
	bind("mask_", &mask_);
	bind("ttl_", &defttl_);
}

void MIPEncapsulator::recv(Packet* p, Handler *h)
{
	Tcl& tcl = Tcl::instance();

	hdr_ip* hdr = hdr_ip::access(p);
	hdr_ipinip **ppinhdr = (hdr_ipinip**)hdr_ipinip::access(p);
	if (--hdr->ttl_ <= 0) {
		/*
		 * XXX this should be "dropped" somehow.  Right now,
		 * these events aren't traced.
		 */
		hdr_ipinip *ptr = *ppinhdr, *temp;
		while (ptr != NULL) {
			temp = ptr;
			ptr = ptr->next_;
			delete temp;
		}
		*ppinhdr = NULL;
		Packet::free(p);
		return;
	}
	hdr_ipinip *inhdr = new hdr_ipinip;
	//int dst = ((hdr->dst() >> shift_) & mask_);
	int dst = Address::instance().get_nodeaddr(hdr->daddr());
	tcl.evalf("%s tunnel-exit %d", name_, dst);
	int te = atoi(tcl.result());

	inhdr->next_ = *ppinhdr;
	*ppinhdr = inhdr;
	inhdr->hdr_ = *hdr;

	hdr->saddr() = here_.addr_;
	hdr->sport() = here_.port_;
	//hdr->dst() = addr_ & ~(~(nsaddr_t)0 << shift_) | (te & mask_) << shift_;;
	hdr->daddr() = te;
	hdr->dport() = 1;
	hdr->ttl() = defttl_;
	hdr_cmn::access(p)->size() += IP_HEADER_SIZE;

	target_->recv(p,h);
}

static class MIPDecapsulatorClass : public TclClass {
public:
	MIPDecapsulatorClass() : TclClass("Classifier/Addr/MIPDecapsulator") {}
	TclObject* create(int, const char*const*) {
		return (new MIPDecapsulator());
	}
} class_mipdecapsulator;

MIPDecapsulator::MIPDecapsulator() : AddressClassifier()
{
}

void MIPDecapsulator::recv(Packet* p, Handler *h)
{
	hdr_ipinip **ppinhdr = (hdr_ipinip **)hdr_ipinip::access(p);
	// restore inner header
	hdr_ip *pouthdr = hdr_ip::access(p);
	assert(ppinhdr);
	hdr_ip *pinhdr = &(*ppinhdr)->hdr_;
	*ppinhdr = (*ppinhdr)->next_;
	*pouthdr = *pinhdr;

	NsObject* link = find(p);
	// for mobilenodes use default-target which is probably the 
	// RA. cannot use node_entry point instead, as hier address
	// of MH point to HA. hence hand decapsulated pkt directly 
	// to RA.

	if (link == NULL || pinhdr->ttl_ <= 0) {
		/*
		 * XXX this should be "dropped" somehow.  Right now,
		 * these events aren't traced.
		 */
		hdr_ipinip *ptr = *ppinhdr, *temp;
		while (ptr != NULL) {
			temp = ptr;
			ptr = ptr->next_;
			delete temp;
		}
		*ppinhdr = NULL;
		delete pinhdr;
		Packet::free(p);
		return;
	}
	delete pinhdr;

	hdr_cmn::access(p)->size() -= IP_HEADER_SIZE;

	link->recv(p,h);
}










