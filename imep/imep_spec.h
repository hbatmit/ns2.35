/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
 *      This product includes software developed by the Computer Systems
 *      Engineering Group at Lawrence Berkeley Laboratory.
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
 *
 * Ported from CMU/Monarch's code
 *
 * $Header: /cvsroot/nsnam/ns-2/imep/imep_spec.h,v 1.4 2006/02/21 15:20:18 mahrenho Exp $
 */

#ifndef __imep_spec_h__
#define __imep_spec_h__

#include <sys/types.h>

#include <stddef.h> // for the offsetof() macro


// **********************************************************************
#ifdef COMMENT_ONLY

The Internet MANET Encapsulation Protocol (IMEP) consists of five
different mechanisms:
     Link/Connection Status Sensing
     Control Message Aggregation
     Broadcast Reliability
     Network-layer Address Resolution
     Security Authentication

04AUG98 - jgb - For now, I am going to implement the first three mechanisms
                as the last two are not really necessary for the present
                simulation work.

#endif /* COMMENT_ONLY */


// **********************************************************************
// Link/Connection Status Sensing


// IMEP may be configured to run in the following "connection
// notification" modes.
#define MODE_BIDIRECTIONAL	0x01
#define MODE_UNIDIRECTIONAL	0x02

// Link status
#define LINK_DOWN		0x00
#define LINK_IN  		0x01
#define LINK_OUT 		0x02
#define LINK_BI  		(LINK_IN | LINK_OUT)

// XXX - The values for these constants are not specified by the IMEP draft.
#define BEACON_PERIOD		1.0 			// seconds
#define BEACON_JITTER		0.010			// seconds
#define MAX_BEACON_TIME 	(BEACON_PERIOD * 3)

// **********************************************************************
// Control Message Aggregation

// XXX - The values for these constants are not specified by the IMEP draft.
#define MAX_TRANSMIT_WAIT_TIME_LOWP	0.250			// seconds
#define MIN_TRANSMIT_WAIT_TIME_LOWP     0.150
#define MAX_TRANSMIT_WAIT_TIME_HIGHP	0.010			// seconds
#define MIN_TRANSMIT_WAIT_TIME_HIGHP    0.000

// **********************************************************************
// Broadcast Reliability
#define RETRANS_PERIOD		0.500			// seconds
#define MAX_REXMITS             2
#define MAX_RETRANS_TIME	(RETRANS_PERIOD * (MAX_REXMITS + 1))


// **********************************************************************
// Protocol Message Format

struct hdr_imep {
	u_int8_t          imep_version : 4;
	u_int8_t	  imep_block_flags : 4;
	u_int16_t	  imep_length;

	// Header access methods
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_imep* access(const Packet* p) {
		return (hdr_imep*) p->access(offset_);
	}
};

/* hdr_imep actually takes 4 bytes, not 3, b/c we aren't packing the
   two bitfields into 1 byte */

#define IMEP_VERSION_0		0x00
#define IMEP_VERSION		IMEP_VERSION_0
#define BLOCK_FLAG_ACK		0x08
#define BLOCK_FLAG_HELLO	0x04
#define BLOCK_FLAG_OBJECT	0x02
#define BLOCK_FLAG_UNUSED	0x01

/* I'd rather not deal with alignment issues in this code, so I've
just padded out the IMEP structs to be word aligned.  The sizes of
packets will be slightly inflated, but the packet formats in the IMEP
draft are really odd --- I can't imagine anyone actually implementing them,
and they'd need to be padded out or redone in real life. -dam */

struct imep_ack {
	u_int8_t  ack_seqno;
        u_int8_t  r1;
        u_int16_t  r2;
	u_int32_t ack_ipaddr;
};

struct imep_ack_block {
	u_int8_t  ab_num_acks;
        u_int8_t  r1;
        u_int16_t  r2;
	char ab_ack_list[1];	// placeholder
};

struct imep_hello {
	u_int32_t  hello_ipaddr;
};

struct imep_hello_block {
	u_int8_t  hb_num_hellos;
        u_int8_t  r1;
        u_int16_t  r2;
	char hb_hello_list[1];	// placeholder
};

struct imep_object {
	u_int16_t	o_length;
	// The IMEP spec uses the first bit to determine if this field
	// is 8 or 16 bits.  I fix its length at 16 bits to keep
	// things simple.
	char		o_data[1]; // placeholder
};

/* define the size of the an imep_object without the following bytes of o_data,
   Use this instead of sizeof(struct imep_object)
 */
#define IMEP_OBJECT_SIZE offsetof(struct imep_object, o_data[0])

struct imep_object_block {
	u_int8_t  ob_sequence;
	u_int8_t  ob_protocol_type : 4;
	u_int8_t  ob_num_objects : 7;
	u_int8_t  ob_num_responses : 5;
	char ob_object_list[1];	// placeholder
};
/*  Use this instead of sizeof(struct imep_object_block)  */
#define IMEP_OBJECT_BLOCK_SIZE offsetof(struct imep_object_block, ob_object_list[0])


#define PROTO_RESERVED	0x00
#define PROTO_NARP	0x01
#define PROTO_RNARP	0x01
#define PROTO_TORA	0x02

// The "response list" follows the "object list" in an IMEP packet.
struct imep_response {
	u_int32_t  resp_ipaddr;
};

#endif /* __imep_spec_h__ */
