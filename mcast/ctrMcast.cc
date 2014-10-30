/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * Copyright (C) 1997 by the University of Southern California
 * $Id: ctrMcast.cc,v 1.10 2005/08/25 18:58:07 johnh Exp $
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

/* 
 * Contributed by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
 * 
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/mcast/ctrMcast.cc,v 1.10 2005/08/25 18:58:07 johnh Exp $ (USC/ISI)";
#endif

#include "agent.h"
#include "ip.h"
#include "ctrMcast.h"

int hdr_CtrMcast::offset_;

static class CtrMcastHeaderClass : public PacketHeaderClass {
public:
	CtrMcastHeaderClass() : PacketHeaderClass("PacketHeader/CtrMcast",
						  sizeof(hdr_CtrMcast)) {
		bind_offset(&hdr_CtrMcast::offset_);
	} 
} class_CtrMcast_hdr;


class CtrMcastEncap : public Agent {
public:
	CtrMcastEncap() : Agent(PT_CtrMcast_Encap) {}
	int command(int argc, const char*const* argv);
	void recv(Packet* p, Handler*);
};

class CtrMcastDecap : public Agent {
public:
	CtrMcastDecap() : Agent(PT_CtrMcast_Decap) {}
	int command(int argc, const char*const* argv);
	void recv(Packet* p, Handler*);
};

static class CtrMcastEncapclass : public TclClass {
public:
	CtrMcastEncapclass() : TclClass("Agent/CtrMcast/Encap") {}
	TclObject* create(int, const char*const*) {
		return (new CtrMcastEncap);
	}
} class_CtrMcastEncap;

static class CtrMcastDecapclass : public TclClass {
public:
	CtrMcastDecapclass() : TclClass("Agent/CtrMcast/Decap") {}
	TclObject* create(int, const char*const*) {
		return (new CtrMcastDecap);
	}
} class_CtrMcastDecap;


int CtrMcastEncap::command(int argc, const char*const* argv)
{
	return Agent::command(argc, argv);
}

int CtrMcastDecap::command(int argc, const char*const* argv)
{
	return Agent::command(argc, argv);
}

void CtrMcastEncap::recv(Packet* p, Handler*)
{
	hdr_CtrMcast* ch = hdr_CtrMcast::access(p);
	hdr_ip* ih = hdr_ip::access(p);

	ch->src() = ih->saddr();
	ch->group() = ih->daddr();
	ch->flowid() = ih->flowid();
	ih->saddr() = addr();
	ih->sport() = port();
	ih->daddr() = daddr();
	ih->dport() = dport();
	ih->flowid() = fid_;

	target_->recv(p);
}

void CtrMcastDecap::recv(Packet* p, Handler*)
{
	hdr_CtrMcast* ch = hdr_CtrMcast::access(p);
	hdr_ip* ih = hdr_ip::access(p);

	ih->saddr() = ch->src();
	ih->daddr() = ch->group();
	ih->flowid() = ch->flowid();

	target_->recv(p);
}
