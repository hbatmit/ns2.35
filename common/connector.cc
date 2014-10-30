/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
    "@(#) $Header: /cvsroot/nsnam/ns-2/common/connector.cc,v 1.14 1998/12/08 23:43:05 haldar Exp $ ";
#endif

#include "packet.h"
#include "connector.h"

static class ConnectorClass : public TclClass {
public:
	ConnectorClass() : TclClass("Connector") {}
	TclObject* create(int, const char*const*) {
		return (new Connector);
	}
} class_connector;

Connector::Connector() : target_(0), drop_(0)
{
}


int Connector::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	/*XXX*/
	if (argc == 2) {
		if (strcmp(argv[1], "target") == 0) {
			if (target_ != 0)
				tcl.result(target_->name());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "drop-target") == 0) {
			if (drop_ != 0)
				tcl.resultf("%s", drop_->name());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "isDynamic") == 0) {
			return TCL_OK;
		}
	}

	else if (argc == 3) {
		if (strcmp(argv[1], "target") == 0) {
			if (*argv[2] == '0') {
				target_ = 0;
				return (TCL_OK);
			}
			target_ = (NsObject*)TclObject::lookup(argv[2]);
			if (target_ == 0) {
				tcl.resultf("no such object %s", argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "drop-target") == 0) {
			drop_ = (NsObject*)TclObject::lookup(argv[2]);
			if (drop_ == 0) {
				tcl.resultf("no object %s", argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
	}
	return (NsObject::command(argc, argv));
}

void Connector::recv(Packet* p, Handler* h)
{
	send(p, h);
}

void Connector::drop(Packet* p)
{
	if (drop_ != 0)
		drop_->recv(p);
	else
		Packet::free(p);
}

void Connector::drop(Packet* p, const char *s)
{
	if (drop_ != 0)
		drop_->recv(p, s);
	else
		Packet::free(p);
}
