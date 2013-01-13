
/*
 * Encapsulator.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: Encapsulator.cc,v 1.6 2005/08/25 18:58:02 johnh Exp $
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
//
// $Header: /cvsroot/nsnam/ns-2/common/Encapsulator.cc,v 1.6 2005/08/25 18:58:02 johnh Exp $

#include "packet.h"
#include "ip.h"
#include "Encapsulator.h"
#include "encap.h"

static class EncapsulatorClass : public TclClass {
public:
	EncapsulatorClass() : TclClass("Agent/Encapsulator") {}
	TclObject* create(int, const char*const*) {
		return (new Encapsulator());
	}
} class_encapsulator;

Encapsulator::Encapsulator() : 
	Agent(PT_ENCAPSULATED), 
	d_target_(0)
{
	bind("status_", &status_);
	bind("overhead_", &overhead_);
}

int Encapsulator::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "decap-target") == 0) {
			if (d_target_ != 0) tcl.result(d_target_->name());
			return (TCL_OK);
		}
	}
	else if (argc == 3) {
		if (strcmp(argv[1], "decap-target") == 0) {
			d_target_ = (NsObject*)TclObject::lookup(argv[2]);
			// even if it's zero, it's OK, we'll just not send to such
			// a target then.
			return (TCL_OK);
		}
	}
	return (Agent::command(argc, argv));
}

void Encapsulator::recv(Packet* p, Handler* h)
{
	if (d_target_) {
		Packet *copy_p= p->copy();
		d_target_->recv(copy_p, h);
	}
	if (status_) {
		Packet* ep= allocpkt(); //sizeof(Packet*));
		hdr_encap* eh= hdr_encap::access(ep);
		eh->encap(p);
		//Packet** pp= (Packet**) encap_p->accessdata();
		//*pp= p;

		hdr_cmn* ch_e= hdr_cmn::access(ep);
		hdr_cmn* ch_p= hdr_cmn::access(p);
	  
		ch_e->ptype()= PT_ENCAPSULATED;
 		ch_e->size()= ch_p->size() + overhead_;
		ch_e->timestamp()= ch_p->timestamp();
		send(ep, h);
	}
	else send(p, h); //forward the packet as it is
}
