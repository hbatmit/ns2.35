/*
Copyright (c) 1997, 1998 Carnegie Mellon University.  All Rights
Reserved. 

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The AODV code developed by the CMU/MONARCH group was optimized and tuned by Samir Das and Mahesh Marina, University of Cincinnati. The work was partially done in Sun Microsystems.
*/


#ifndef __aodv_packet_h__
#define __aodv_packet_h__

//#include <config.h>
//#include "aodv.h"
#define AODV_MAX_ERRORS 100


/* =====================================================================
   Packet Formats...
   ===================================================================== */
#define AODVTYPE_HELLO  	0x01
#define AODVTYPE_RREQ   	0x02
#define AODVTYPE_RREP   	0x04
#define AODVTYPE_RERR   	0x08
#define AODVTYPE_RREP_ACK  	0x10

/*
 * AODV Routing Protocol Header Macros
 */
#define HDR_AODV(p)		((struct hdr_aodv*)hdr_aodv::access(p))
#define HDR_AODV_REQUEST(p)  	((struct hdr_aodv_request*)hdr_aodv::access(p))
#define HDR_AODV_REPLY(p)	((struct hdr_aodv_reply*)hdr_aodv::access(p))
#define HDR_AODV_ERROR(p)	((struct hdr_aodv_error*)hdr_aodv::access(p))
#define HDR_AODV_RREP_ACK(p)	((struct hdr_aodv_rrep_ack*)hdr_aodv::access(p))

/*
 * General AODV Header - shared by all formats
 */
struct hdr_aodv {
        u_int8_t        ah_type;
	/*
        u_int8_t        ah_reserved[2];
        u_int8_t        ah_hopcount;
	*/
		// Header access methods
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_aodv* access(const Packet* p) {
		return (hdr_aodv*) p->access(offset_);
	}
};

struct hdr_aodv_request {
        u_int8_t        rq_type;	// Packet Type
        u_int8_t        reserved[2];
        u_int8_t        rq_hop_count;   // Hop Count
        u_int32_t       rq_bcast_id;    // Broadcast ID

        nsaddr_t        rq_dst;         // Destination IP Address
        u_int32_t       rq_dst_seqno;   // Destination Sequence Number
        nsaddr_t        rq_src;         // Source IP Address
        u_int32_t       rq_src_seqno;   // Source Sequence Number

        double          rq_timestamp;   // when REQUEST sent;
					// used to compute route discovery latency

  // This define turns on gratuitous replies- see aodv.cc for implementation contributed by
  // Anant Utgikar, 09/16/02.
  //#define RREQ_GRAT_RREP	0x80

  inline int size() { 
  int sz = 0;
  /*
  	sz = sizeof(u_int8_t)		// rq_type
	     + 2*sizeof(u_int8_t) 	// reserved
	     + sizeof(u_int8_t)		// rq_hop_count
	     + sizeof(double)		// rq_timestamp
	     + sizeof(u_int32_t)	// rq_bcast_id
	     + sizeof(nsaddr_t)		// rq_dst
	     + sizeof(u_int32_t)	// rq_dst_seqno
	     + sizeof(nsaddr_t)		// rq_src
	     + sizeof(u_int32_t);	// rq_src_seqno
  */
  	sz = 7*sizeof(u_int32_t);
  	assert (sz >= 0);
	return sz;
  }
};

struct hdr_aodv_reply {
        u_int8_t        rp_type;        // Packet Type
        u_int8_t        reserved[2];
        u_int8_t        rp_hop_count;           // Hop Count
        nsaddr_t        rp_dst;                 // Destination IP Address
        u_int32_t       rp_dst_seqno;           // Destination Sequence Number
        nsaddr_t        rp_src;                 // Source IP Address
        double	        rp_lifetime;            // Lifetime

        double          rp_timestamp;           // when corresponding REQ sent;
						// used to compute route discovery latency
						
  inline int size() { 
  int sz = 0;
  /*
  	sz = sizeof(u_int8_t)		// rp_type
	     + 2*sizeof(u_int8_t) 	// rp_flags + reserved
	     + sizeof(u_int8_t)		// rp_hop_count
	     + sizeof(double)		// rp_timestamp
	     + sizeof(nsaddr_t)		// rp_dst
	     + sizeof(u_int32_t)	// rp_dst_seqno
	     + sizeof(nsaddr_t)		// rp_src
	     + sizeof(u_int32_t);	// rp_lifetime
  */
  	sz = 6*sizeof(u_int32_t);
  	assert (sz >= 0);
	return sz;
  }

};

struct hdr_aodv_error {
        u_int8_t        re_type;                // Type
        u_int8_t        reserved[2];            // Reserved
        u_int8_t        DestCount;                 // DestCount
        // List of Unreachable destination IP addresses and sequence numbers
        nsaddr_t        unreachable_dst[AODV_MAX_ERRORS];   
        u_int32_t       unreachable_dst_seqno[AODV_MAX_ERRORS];   

  inline int size() { 
  int sz = 0;
  /*
  	sz = sizeof(u_int8_t)		// type
	     + 2*sizeof(u_int8_t) 	// reserved
	     + sizeof(u_int8_t)		// length
	     + length*sizeof(nsaddr_t); // unreachable destinations
  */
  	sz = (DestCount*2 + 1)*sizeof(u_int32_t);
	assert(sz);
        return sz;
  }

};

struct hdr_aodv_rrep_ack {
	u_int8_t	rpack_type;
	u_int8_t	reserved;
};

// for size calculation of header-space reservation
union hdr_all_aodv {
  hdr_aodv          ah;
  hdr_aodv_request  rreq;
  hdr_aodv_reply    rrep;
  hdr_aodv_error    rerr;
  hdr_aodv_rrep_ack rrep_ack;
};

#endif /* __aodv_packet_h__ */
