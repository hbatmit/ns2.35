/*
 * Copyright (c) Xerox Corporation 1998. All rights reserved.
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
 * $Header: /cvsroot/nsnam/ns-2/webcache/inval-agent.cc,v 1.14 2005/08/26 05:05:31 tomh Exp $
 */

//
// Agents used to send and receive invalidation records
// 

#include "inval-agent.h"
#include "ip.h"
#include "http.h"

// Implementation 1: Invalidation via multicast heartbeat
int hdr_inval::offset_;
static class HttpInvalHeaderClass : public PacketHeaderClass {
public:
        HttpInvalHeaderClass() : PacketHeaderClass("PacketHeader/HttpInval",
						   sizeof(hdr_inval)) {
		bind_offset(&hdr_inval::offset_);
	}
} class_httpinvalhdr;

static class HttpInvalClass : public TclClass {
public:
	HttpInvalClass() : TclClass("Agent/HttpInval") {}
	TclObject* create(int, const char*const*) {
		return (new HttpInvalAgent());
	}
} class_httpinval_agent;

HttpInvalAgent::HttpInvalAgent() : Agent(PT_INVAL)
{
	// It should be initialized to the same as tcpip_base_hdr_size_
	bind("inval_hdr_size_", &inval_hdr_size_);
}

void HttpInvalAgent::recv(Packet *pkt, Handler*)
{
	hdr_ip *ip = hdr_ip::access(pkt);
	if ((ip->saddr() == addr()) && (ip->sport() == port()))
		// XXX Why do we need this?
		return;
	if (app_ == 0) 
		return;
	hdr_inval *ih = hdr_inval::access(pkt);
	((HttpApp*)app_)->process_data(ih->size(), pkt->userdata());
	Packet::free(pkt);
}

// Send a list of invalidation records in user data area
// realsize: the claimed size
// datasize: the actual size of user data, used to allocate packet
void HttpInvalAgent::send(int realsize, AppData* data)
{
	Packet *pkt = allocpkt(data->size());
	hdr_inval *ih = hdr_inval::access(pkt);
	ih->size() = data->size();
	pkt->setdata(data);

	// Set packet size proportional to the number of invalidations
	hdr_cmn *ch = hdr_cmn::access(pkt);
	ch->size() = inval_hdr_size_ + realsize;
	Agent::send(pkt, 0);
}


// Implementation 2: Invalidation via TCP. 
static class HttpUInvalClass : public TclClass {
public:
	HttpUInvalClass() : TclClass("Application/TcpApp/HttpInval") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc != 5)
			return NULL;
		Agent *a = (Agent *)TclObject::lookup(argv[4]);
		a->set_pkttype(PT_INVAL); // It's TCP but used for invalidation
		if (a == NULL)
			return NULL;
		return (new HttpUInvalAgent(a));
	}
} class_httpuinval_agent;

void HttpUInvalAgent::process_data(int size, AppData* data) 
{
	target_->process_data(size, data);
}

int HttpUInvalAgent::command(int argc, const char*const* argv)
{
	if (strcmp(argv[1], "set-app") == 0) {
		// Compatibility interface
		HttpApp* c = 
			(HttpApp*)TclObject::lookup(argv[2]);
		target_ = (Process *)c;
		return TCL_OK;
	}
	return TcpApp::command(argc, argv);
}
