/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1996-1997 Regents of the University of California.
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
 * 	This product includes software developed by the MASH Research
 * 	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/common/ttl.cc,v 1.11 1998/08/12 23:41:24 gnguyen Exp $";
#endif

#include "packet.h"
#include "ip.h"
#include "connector.h"

class TTLChecker : public Connector {
public:
	TTLChecker() : noWarn_(1), tick_(1) {}
	int command(int argc, const char*const* argv) {
		if (argc == 3) {
			if (strcmp(argv[1], "warning") == 0) {
				noWarn_ = ! atoi(argv[2]);
				return TCL_OK;
			}
			if (strcmp(argv[1], "tick") == 0) {
				int tick = atoi(argv[2]);
				if (tick > 0) {
					tick_ = tick;
					return TCL_OK;
				} else {
					Tcl& tcl = Tcl::instance();
					tcl.resultf("%s: TTL must be positive (specified = %d)\n",
						    name(), tick);
					return TCL_ERROR;
				}
			}
		}
		return Connector::command(argc, argv);
	}
	void recv(Packet* p, Handler* h) {
		hdr_ip* iph = hdr_ip::access(p);
		int ttl = iph->ttl() - tick_;
		if (ttl <= 0) {
			/* XXX should send to a drop object.*/
			// Yes, and now it does...
			// Packet::free(p);
			if (! noWarn_)
				printf("ttl exceeded\n");
			drop(p);
			return;
		}
		iph->ttl() = ttl;
		send(p, h);
	}
protected:
	int noWarn_;
	int tick_;
};

static class TTLCheckerClass : public TclClass {
public:
	TTLCheckerClass() : TclClass("TTLChecker") {}
	TclObject* create(int, const char*const*) {
		return (new TTLChecker);
	}
} ttl_checker_class;


class SessionTTLChecker : public Connector {
public:
	SessionTTLChecker() {}
	int command(int argc, const char*const* argv);
	void recv(Packet* p, Handler* h) {
		hdr_ip* iph = hdr_ip::access(p);
		int ttl = iph->ttl() - tick_;
		if (ttl <= 0) {
			/* XXX should send to a drop object.*/
			// Yes, and now it does...
			// Packet::free(p);
			printf("ttl exceeded\n");
			drop(p);
			return;
		}
		iph->ttl() = ttl;
		send(p, h);
	}
protected:
	int tick_;
};

static class SessionTTLCheckerClass : public TclClass {
public:
	SessionTTLCheckerClass() : TclClass("TTLChecker/Session") {}
	TclObject* create(int, const char*const*) {
		return (new SessionTTLChecker);
	}
} session_ttl_checker_class;

int SessionTTLChecker::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "tick") == 0) {
			tick_ = atoi(argv[2]);
			return (TCL_OK);
		}
	}
	return (Connector::command(argc, argv));
}
