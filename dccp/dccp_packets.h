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
 * $Id: dccp_packets.h,v 1.1 2010/04/06 05:09:01 tom_henderson Exp $ */

#ifndef ns_dccp_packets_h
#define ns_dccp_packets_h

#include "dccp_types.h"
#include "dccp_opt.h"

struct hdr_dccp {
	//generic dccp header
	dccp_packet_type type_;
	u_int8_t ccval_;
	u_int8_t ndp_;
	u_int32_t seq_num_;
	u_int8_t data_offset_;
	u_int8_t cscov_;
	
	//options      
	DCCPOptions *options_;  
	
	//ns-2 header offset
	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_dccp* access(const Packet* p) {
		return (hdr_dccp*) p->access(offset_);
	}
};

struct hdr_dccpack {
	u_int32_t ack_num_;

	//ns-2 header offset
	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_dccpack* access(const Packet* p) {
		return (hdr_dccpack*) p->access(offset_);
	}
};

struct hdr_dccpreset {
	//reset packet info
	dccp_reset_reason rst_reason_;
	u_int8_t rst_data1_;
	u_int8_t rst_data2_;
	u_int8_t rst_data3_;

	//ns-2 header offset
	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_dccpreset* access(const Packet* p) {
		return (hdr_dccpreset*) p->access(offset_);
	}
};

struct hdr_dccpreq {
	//ns-2 header offset
	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_dccpreq* access(const Packet* p) {
		return (hdr_dccpreq*) p->access(offset_);
	}
};

struct hdr_dccpresp {
	//ns-2 header offset
	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_dccpresp* access(const Packet* p) {
		return (hdr_dccpresp*) p->access(offset_);
	}
};

struct hdr_dccpdata {
	//ns-2 header offset
	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_dccpdata* access(const Packet* p) {
		return (hdr_dccpdata*) p->access(offset_);
	}
};

struct hdr_dccpdataack {
	//ns-2 header offset
	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_dccpdataack* access(const Packet* p) {
		return (hdr_dccpdataack*) p->access(offset_);
	}
};

struct hdr_dccpclose {
	//ns-2 header offset
	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_dccpclose* access(const Packet* p) {
		return (hdr_dccpclose*) p->access(offset_);
	}
};

struct hdr_dccpclosereq {
	//ns-2 header offset
	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_dccpclosereq* access(const Packet* p) {
		return (hdr_dccpclosereq*) p->access(offset_);
	}
};

#endif
