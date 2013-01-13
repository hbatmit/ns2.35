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
 * $Header: /cvsroot/nsnam/ns-2/webcache/tcp-simple.cc,v 1.10 2005/08/26 05:05:31 tomh Exp $
 *
 */
//
// SimpleTcp: Only share the same interface as FullTcp.
// It's inherited from FullTcp solely for interface reason... :(
//
// If we have interface declaration independent from class type definition,
// we'll be better off.
//

#include <stdlib.h>
#include "tclcl.h"
#include "packet.h"
#include "ip.h"
#include "app.h"
#include "tcp-simple.h"

static class SimpleTcpClass : public TclClass {
public:
	SimpleTcpClass() : TclClass("Agent/TCP/SimpleTcp") {}
	TclObject* create(int, const char*const*) {
		return (new SimpleTcpAgent());
	}
} class_simple_tcp_agent;

SimpleTcpAgent::SimpleTcpAgent() : TcpAgent(), seqno_(0)
{
}

// XXX Do *NOT* support infinite send of TCP (bytes == -1).
void SimpleTcpAgent::sendmsg(int bytes, const char* /*flags*/)
{
	if (bytes == -1) {
		fprintf(stderr, 
"SimpleTcp doesn't support infinite send. Do not use FTP::start(), etc.\n");
		return;
	}
	// Simply sending out bytes out to target_
	curseq_ += bytes;
	seqno_ ++;

	Packet *p = allocpkt();
	hdr_tcp *tcph = HDR_TCP(p);
	tcph->seqno() = seqno_;
	tcph->ts() = Scheduler::instance().clock();
	tcph->ts_echo() = ts_peer_;
        hdr_cmn *th = HDR_CMN(p);
	th->size() = bytes + tcpip_base_hdr_size_;
	send(p, 0);
}

void SimpleTcpAgent::recv(Packet *pkt, Handler *)
{
        hdr_cmn *th = HDR_CMN(pkt);
	int datalen = th->size() - tcpip_base_hdr_size_;
	if (app_)
		app_->recv(datalen);
	// No lastbyte_ callback, because no packet fragmentation.
	Packet::free(pkt);
}

int SimpleTcpAgent::command(int argc, const char*const* argv)
{
	// Copy FullTcp's tcl interface

	if (argc == 2) {
		if (strcmp(argv[1], "listen") == 0) {
			// Do nothing
			return (TCL_OK);
		}
		if (strcmp(argv[1], "close") == 0) {
			// Call done{} to match tcp-full's syntax
			Tcl::instance().evalf("%s done", name());
			return (TCL_OK);
		}
	}
	return (TcpAgent::command(argc, argv));
}
