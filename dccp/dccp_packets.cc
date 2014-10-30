/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/* Copyright (c) 2004  Nils-Erik Mattsson 
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: dccp_packets.cc,v 1.1 2010/04/06 05:09:01 tom_henderson Exp $ */

#include "packet.h"
#include "dccp_packets.h"

int hdr_dccp::offset_;
int hdr_dccpack::offset_;
int hdr_dccpreq::offset_;
int hdr_dccpresp::offset_;
int hdr_dccpdata::offset_;
int hdr_dccpdataack::offset_;
int hdr_dccpclose::offset_;
int hdr_dccpclosereq::offset_;
int hdr_dccpreset::offset_;

//OTcl linkage for packet headers
static class DCCPHeaderClass : public PacketHeaderClass {
public:
	DCCPHeaderClass() : PacketHeaderClass("PacketHeader/DCCP",
					      sizeof(hdr_dccp)){
		bind_offset(&hdr_dccp::offset_);
	}
} class_dccphdr;

static class DCCPResetHeaderClass : public PacketHeaderClass {
public:
	DCCPResetHeaderClass() : PacketHeaderClass("PacketHeader/DCCP_RESET",
					      sizeof(hdr_dccpreset)){
		bind_offset(&hdr_dccpreset::offset_);
	}
} class_dccpresethdr;

static class DCCPResponseHeaderClass : public PacketHeaderClass {
public:
	DCCPResponseHeaderClass() : PacketHeaderClass("PacketHeader/DCCP_RESP",
					      sizeof(hdr_dccpresp)){
		bind_offset(&hdr_dccpresp::offset_);
	}
} class_dccpresphdr;

static class DCCPRequestHeaderClass : public PacketHeaderClass {
public:
	DCCPRequestHeaderClass() : PacketHeaderClass("PacketHeader/DCCP_REQ",
					      sizeof(hdr_dccpreq)){
		bind_offset(&hdr_dccpreq::offset_);
	}
} class_dccpreqhdr;

static class DCCPAckHeaderClass : public PacketHeaderClass {
public:
	DCCPAckHeaderClass() : PacketHeaderClass("PacketHeader/DCCP_ACK",
					      sizeof(hdr_dccpack)){
		bind_offset(&hdr_dccpack::offset_);
	}
} class_dccpackhdr;

static class DCCPDataAckHeaderClass : public PacketHeaderClass {
public:
	DCCPDataAckHeaderClass() : PacketHeaderClass("PacketHeader/DCCP_DATAACK",
					      sizeof(hdr_dccpdataack)){
		bind_offset(&hdr_dccpdataack::offset_);
	}
} class_dccpdataackhdr;

static class DCCPDataHeaderClass : public PacketHeaderClass {
public:
	DCCPDataHeaderClass() : PacketHeaderClass("PacketHeader/DCCP_DATA",
					      sizeof(hdr_dccpdata)){
		bind_offset(&hdr_dccpdata::offset_);
	}
} class_dccpdatahdr;

static class DCCPCloseHeaderClass : public PacketHeaderClass {
public:
	DCCPCloseHeaderClass() : PacketHeaderClass("PacketHeader/DCCP_CLOSE",
					      sizeof(hdr_dccpclose)){
		bind_offset(&hdr_dccpclose::offset_);
	}
} class_dccpclosehdr;

static class DCCPCloseReqHeaderClass : public PacketHeaderClass {
public:
	DCCPCloseReqHeaderClass() : PacketHeaderClass("PacketHeader/DCCP_CLOSEREQ",
					      sizeof(hdr_dccpclosereq)){
		bind_offset(&hdr_dccpclosereq::offset_);
	}
} class_dccpclosereqhdr;

