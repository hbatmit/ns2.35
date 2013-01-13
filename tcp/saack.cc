/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linking this file statically or dynamically with other modules is making
 * a combined work based on this file.  Thus, the terms and conditions of
 * the GNU General Public License cover the whole combination.
 *
 * In addition, as a special exception, the copyright holders of this file
 * give you permission to combine this file with free software programs or
 * libraries that are released under the GNU LGPL and with code included in
 * the standard release of ns-2 under the Apache 2.0 license or under
 * otherwise-compatible licenses with advertising requirements (or modified
 * versions of such code, with unchanged license).  You may copy and
 * distribute such a system following the terms of the GNU GPL for this
 * file and the licenses of the other code concerned, provided that you
 * include the source code of that other code when and as the GNU GPL
 * requires distribution of source code.
 *
 * Note that people who make modified versions of this file are not
 * obligated to grant this special exception for their modified versions;
 * it is their choice whether to do so.  The GNU General Public License
 * gives permission to release a modified version without this exception;
 * this exception also makes it possible to release a modified version
 * which carries forward this exception.
 *
 * $Header: /cvsroot/nsnam/ns-2/tcp/saack.cc,v 1.8 2005/08/26 05:05:30 tomh Exp $
 */

//SignalAckClass
//Null receiver agent which merely acknowledges a request

#include "agent.h"
#include "ip.h"
#include "resv.h"

class SAack_Agent : public Agent {
public:
	SAack_Agent();
	
protected:
	void recv(Packet *, Handler *);
	int command(int,const char* const*);
};

SAack_Agent::SAack_Agent(): Agent(PT_TCP)
{
}

static class SAackClass : public TclClass {
public:
	SAackClass() : TclClass("Agent/SAack") {}
	TclObject* create(int, const char*const*) {
		return (new SAack_Agent());
	}
} class_saack;


void SAack_Agent::recv(Packet *p, Handler *h)
{
	hdr_cmn *ch = hdr_cmn::access(p);
	
	if (ch->ptype() == PT_REQUEST) {
		Packet *newp =allocpkt();
		hdr_cmn *newch=hdr_cmn::access(newp);
		newch->size()=ch->size();
		// turn the packet around by swapping src and dst
		hdr_ip * iph = hdr_ip::access(p);
		hdr_ip * newiph = hdr_ip::access(newp);
		newiph->dst()=iph->src();
		newiph->flowid()=iph->flowid();
		newiph->prio()=iph->prio();
		
		hdr_resv* rv=hdr_resv::access(p);
		hdr_resv* newrv=hdr_resv::access(newp);
		newrv->decision()=rv->decision();
		newrv->rate()=rv->rate();
		newrv->bucket()=rv->bucket();
		if (rv->decision())
		        newch->ptype() = PT_ACCEPT;
		else
		        newch->ptype() = PT_REJECT;
		target_->recv(newp);
		Packet::free(p);
		return;
	}
	Agent::recv(p,h);
}


int SAack_Agent::command(int argc, const char*const* argv)
{
	return (Agent::command(argc,argv));
	
}
  
  

