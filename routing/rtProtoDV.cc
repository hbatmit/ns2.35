// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- 

/*
 * Copyright (C) 1997 by the University of Southern California
 * $Id: rtProtoDV.cc,v 1.9 2005/08/25 18:58:12 johnh Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/routing/rtProtoDV.cc,v 1.9 2005/08/25 18:58:12 johnh Exp $ (USC/ISI)";
#endif

#include "agent.h"
#include "rtProtoDV.h"

int hdr_DV::offset_;

static class rtDVHeaderClass : public PacketHeaderClass {
public:
	rtDVHeaderClass() : PacketHeaderClass("PacketHeader/rtProtoDV",
					      sizeof(hdr_DV)) {
		bind_offset(&hdr_DV::offset_);
	} 
} class_rtProtoDV_hdr;

static class rtProtoDVclass : public TclClass {
public:
	rtProtoDVclass() : TclClass("Agent/rtProto/DV") {}
	TclObject* create(int, const char*const*) {
		return (new rtProtoDV);
	}
} class_rtProtoDV;


int rtProtoDV::command(int argc, const char*const* argv)
{
	if (strcmp(argv[1], "send-update") == 0) {
		ns_addr_t dst;
		dst.addr_ = atoi(argv[2]);
		dst.port_ = atoi(argv[3]);
		u_int32_t mtvar = atoi(argv[4]);
		u_int32_t size  = atoi(argv[5]);
		sendpkt(dst, mtvar, size);
		return TCL_OK;
	}
	return Agent::command(argc, argv);
}

void rtProtoDV::sendpkt(ns_addr_t dst, u_int32_t mtvar, u_int32_t size)
{
	daddr() = dst.addr_;
	dport() = dst.port_;
	size_ = size;
	
	Packet* p = Agent::allocpkt();
	hdr_DV *rh = hdr_DV::access(p);
	rh->metricsVar() = mtvar;

	target_->recv(p);
}

void rtProtoDV::recv(Packet* p, Handler*)
{
	hdr_DV* rh = hdr_DV::access(p);
	hdr_ip* ih = hdr_ip::access(p);
	Tcl::instance().evalf("%s recv-update %d %d", name(),
			      ih->saddr(), rh->metricsVar());
	Packet::free(p);
}
