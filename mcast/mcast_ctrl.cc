/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * Copyright (C) 1997 by the University of Southern California
 * $Id: mcast_ctrl.cc,v 1.7 2005/08/25 18:58:07 johnh Exp $
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
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/mcast/mcast_ctrl.cc,v 1.7 2005/08/25 18:58:07 johnh Exp $ (LBL)";
#endif

#include "agent.h"
#include "packet.h"
#include "mcast_ctrl.h"

class mcastControlAgent : public Agent {
public:
	mcastControlAgent() : Agent(PT_NTYPE) {
		bind("packetSize_", &size_);
	}

	void recv(Packet* pkt, Handler*) {
		hdr_mcast_ctrl* ph = hdr_mcast_ctrl::access(pkt);
		hdr_cmn* ch = hdr_cmn::access(pkt);
		// Agent/Mcast/Control instproc recv type from src group iface
		Tcl::instance().evalf("%s recv %s %d %d", name(),
				      ph->type(), ch->iface(), ph->args());
		Packet::free(pkt);
	}

	/*
 	 * $proc send $type $src $group
 	 */

#define	CASE(c,str,type)						\
		case (c):	if (strcmp(argv[2], (str)) == 0) {	\
			type_ = (type);					\
			break;						\
		} else {						\
			/*FALLTHROUGH*/					\
		}

	int command(int argc, const char*const* argv) {
		if (argc == 4) {
			if (strcmp(argv[1], "send") == 0) {
				switch (*argv[2]) {
					CASE('p', "prune", PT_PRUNE);
					CASE('g', "graft", PT_GRAFT);
					CASE('X', "graftAck", PT_GRAFTACK);
					CASE('j', "join",  PT_JOIN);
					CASE('a', "assert", PT_ASSERT);
				default:
					Tcl& tcl = Tcl::instance();
					tcl.result("invalid control message");
					return (TCL_ERROR);
				}
				Packet* pkt = allocpkt();
				hdr_mcast_ctrl* ph=hdr_mcast_ctrl::access(pkt);
				strcpy(ph->type(), argv[2]);
				ph->args()  = atoi(argv[3]);
				send(pkt, 0);
				return (TCL_OK);
			}
		}
		return (Agent::command(argc, argv));
	}
};

//
// Now put the standard OTcl linkage templates here
//
int hdr_mcast_ctrl::offset_;
static class mcastControlHeaderClass : public PacketHeaderClass {
public:
        mcastControlHeaderClass() : PacketHeaderClass("PacketHeader/mcastCtrl",
					     sizeof(hdr_mcast_ctrl)) {
		bind_offset(&hdr_mcast_ctrl::offset_);
	}
} class_mcast_ctrl_hdr;

static class mcastControlClass : public TclClass {
public:
	mcastControlClass() : TclClass("Agent/Mcast/Control") {}
	TclObject* create(int, const char*const*) {
		return (new mcastControlAgent());
	}
} class_mcast_ctrl;
