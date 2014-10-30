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
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/common/ip.h,v 1.16 2006/02/22 13:32:23 mahrenho Exp $
 */

/* a network layer; basically like IPv6 */
#ifndef ns_ip_h
#define ns_ip_h

#include "config.h"
#include "packet.h"


#define IP_HDR_LEN      20
#define IP_DEF_TTL      32

// The following undef is to suppress warnings on systems were
// IP_BROADCAST is defined.
#ifdef IP_BROADCAST		
#undef IP_BROADCAST
#endif

// #define IP_BROADCAST	((u_int32_t) 0xffffffff)
static const u_int32_t IP_BROADCAST = ((u_int32_t) 0xffffffff);


struct hdr_ip {
	/* common to IPv{4,6} */
	ns_addr_t	src_;
	ns_addr_t	dst_;
	int		ttl_;

	/* Monarch extn */
// 	u_int16_t	sport_;
// 	u_int16_t	dport_;
	
	/* IPv6 */
	int		fid_;	/* flow id */
	int		prio_;

	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_ip* access(const Packet* p) {
		return (hdr_ip*) p->access(offset_);
	}

	/* per-field member acces functions */
	ns_addr_t& src() { return (src_); }
	nsaddr_t& saddr() { return (src_.addr_); }
        int32_t& sport() { return src_.port_;}

	ns_addr_t& dst() { return (dst_); }
	nsaddr_t& daddr() { return (dst_.addr_); }
        int32_t& dport() { return dst_.port_;}
	int& ttl() { return (ttl_); }
	/* ipv6 fields */
	int& flowid() { return (fid_); }
	int& prio() { return (prio_); }
};

#endif
