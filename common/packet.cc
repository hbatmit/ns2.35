/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1994-1997 Regents of the University of California.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
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
    "@(#) $Header: /cvsroot/nsnam/ns-2/common/packet.cc,v 1.19 2008/02/18 03:39:02 tom_henderson Exp $ (LBL)";
#endif

#include "packet.h"
#include "flags.h"

p_info packet_info;
char** p_info::name_;
unsigned int p_info::nPkt_ = 0;
PacketClassifier *p_info::pc_ = 0;

int p_info::addPacket(char *name)
{
	if(!nPkt_)
		initName();
	
	int newID = nPkt_-1;
	PT_NTYPE = nPkt_;
	initName();
	name_[newID] = name;
	return newID;
}

int Packet::hdrlen_ = 0;		// size of a packet's header
Packet* Packet::free_;			// free list
int hdr_cmn::offset_;			// static offset of common header
int hdr_flags::offset_;			// static offset of flags header


PacketHeaderClass::PacketHeaderClass(const char* classname, int hdrlen) : 
	TclClass(classname), hdrlen_(hdrlen), offset_(0)
{
}


TclObject* PacketHeaderClass::create(int, const char*const*)
{
	return (0);
}

void PacketHeaderClass::bind()
{
	TclClass::bind();
	Tcl& tcl = Tcl::instance();
	tcl.evalf("%s set hdrlen_ %d", classname_, hdrlen_);
	export_offsets();
	add_method("offset");
}

void PacketHeaderClass::export_offsets()
{
}

void PacketHeaderClass::field_offset(const char* fieldname, int offset)
{
	Tcl& tcl = Tcl::instance();
	tcl.evalf("%s set offset_(%s) %d", classname_, fieldname, offset);
}

int PacketHeaderClass::method(int ac, const char*const* av)
{
	Tcl& tcl = Tcl::instance();
	int argc = ac - 2;
	const char*const* argv = av + 2;
	if (argc == 3) {
		if (strcmp(argv[1], "offset") == 0) {
			if (offset_) {
				*offset_ = atoi(argv[2]);
				return TCL_OK;
			}
			tcl.resultf("Warning: cannot set offset_ for %s",
				    classname_);
			return TCL_OK;
		}
	}
	else if (argc == 2) {
		if (strcmp(argv[1], "offset") == 0) {
			if (offset_) {
				tcl.resultf("%d", *offset_);
				return TCL_OK;
			}
		}
	}
	return TclClass::method(ac, av);
}


class CommonHeaderClass : public PacketHeaderClass {
public:
	CommonHeaderClass() : PacketHeaderClass("PacketHeader/Common",
						sizeof(hdr_cmn)) {
		bind_offset(&hdr_cmn::offset_);
	}
	void export_offsets() {
		field_offset("ptype_", OFFSET(hdr_cmn, ptype_));
		field_offset("size_", OFFSET(hdr_cmn, size_));
		field_offset("uid_", OFFSET(hdr_cmn, uid_));
		field_offset("error_", OFFSET(hdr_cmn, error_));
	};
} class_cmnhdr;

class FlagsHeaderClass : public PacketHeaderClass {
public:
	FlagsHeaderClass() : PacketHeaderClass("PacketHeader/Flags",
						sizeof(hdr_flags)) {
		bind_offset(&hdr_flags::offset_);
	}
} class_flagshdr;


/* manages active packet header types */
class PacketHeaderManager : public TclObject {
public:
	PacketHeaderManager() {
		bind("hdrlen_", &Packet::hdrlen_);
	}
};

static class PacketHeaderManagerClass : public TclClass {
public:
	PacketHeaderManagerClass() : TclClass("PacketHeaderManager") {}
	TclObject* create(int, const char*const*) {
		return (new PacketHeaderManager);
	}
} class_packethdr_mgr;
