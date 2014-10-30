/*******************************************************************************
 *                                                                             *
 *   M-DART Routing Protocol                                                   *
 *                                                                             *
 *   Copyright (C) 2006 by Marcello Caleffi                                    *
 *   marcello.caleffi@unina.it                                                 *
 *                                                                             *
 * Redistribution and use in source and binary forms, with or without          *
 * modification, are permitted provided that the following conditions are met: *
 * 1. Redistributions of source code must retain the above copyright notice,   *
 *    this list of conditions and the following disclaimer.                    *
 * 2. Redistributions in binary form must reproduce the above copyright        *
 *    notice, this list of conditions and the following disclaimer in the      *
 *    documentation and/or other materials provided with the distribution.     *
 * 3. The name of the author may not be used to endorse or promote products    *
 *    derived from this software without specific prior written permission.    *
 *                                                                             *
 * This software is provided by the author ``as is'' and any express or        *
 * implied warranties, including, but not limited to, the implied warranties   *
 * of merchantability and fitness for a particular purpose are disclaimed.     *
 * in no event shall the author be liable for any direct, indirect,            *
 * incidental, special, exemplary, or consequential damages (including, but    *
 * not limited to, procurement of substitute goods or services; loss of use,   *
 * data, or profits; or business interruption) however caused and on any       *
 * theory of liability, whether in contract, strict liability, or tort         *
 * (including negligence or otherwise) arising in any way out of the use of    *
 * this software, even if advised of the possibility of such damage.           *
 *                                                                             *
 * The M-DART code has been developed by Marcello Caleffi during his Ph.D. at  *
 * the Department of Biomedical, Electronic and Telecommunications Engineering *
 * University of Naples Federico II, Italy.                                    *
 *                                                                             *
 * In order to give credit and recognition to the author, if you use M-DART    *
 * results or results obtained by modificating the M-DART source code, please  *
 * cite one of the following papers:                                           *
 * - M. Caleffi, L. Paura, "M-DART: Multi-Path Dynamic Address RouTing",       *
 *   Wireless Communications and Mobile Computing, 2010                        *
 * - M. Caleffi, G. Ferraiuolo, L. Paura, "Augmented Tree-based Routing        *
 *   Protocol for Scalable Ad Hoc Networks", Proc. of IEEE MASS '07: IEEE      *
 *   Internatonal Conference on Mobile Adhoc and Sensor Systems, Pisa (Italy), *
 *   October 8-11 2007.                                                        *
 *                                                                             *
 ******************************************************************************/



#ifndef __mdart_packet_h__
#define __mdart_packet_h__



#include <packet.h>



//------------------------------------------------------------------------------
// Packet Formats
//------------------------------------------------------------------------------
// Hello packet used by Neighbour Discovery Process
#define MDART_TYPE_HELLO		0x00
// Dynamic Address Request packet used by Address Discovery Process
#define MDART_TYPE_DARQ		0x01
// Dynamic Address Reply packet used by Address Discovery Process
#define MDART_TYPE_DARP		0x02
// Dynamic Address Update packet used by Address Discovery Process
#define MDART_TYPE_DAUP		0x03
// Dynamic Address Broadcast packet used by Address Discovery Process
#define MDART_TYPE_DABR		0x04
//Packet used by MDART to encapsulate data packets
#define MDART_TYPE_ENCP		0x05
// Info Store packet used by Info Discovery Process
#define MDART_TYPE_INST		0x06
// Info Request packet used by Info Discovery Process
#define MDART_TYPE_INRQ		0x07
// Info Reply packet used by Info Discovery Process
#define MDART_TYPE_INRP		0x08



//------------------------------------------------------------------------------
// Field Constant
//------------------------------------------------------------------------------
// Broadcast Dynamic Address
#define DATYPE_BROADCAST	((u_int32_t) 0xffffffff)



//------------------------------------------------------------------------------
// MDART header macros
//------------------------------------------------------------------------------
// General MDART header macro
#define HDR_MDART(p)			(struct hdr_mdart *)hdr_mdart::access(p)
// Hello packet header macro
#define HDR_MDART_HELLO(p)	(struct hdr_mdart_hello *)hdr_mdart::access(p)
// Dynamic Address Request packet header macro
#define HDR_MDART_DARQ(p)		(struct hdr_mdart_darq *)hdr_mdart::access(p)
// Dynamic Address Reply packet header macro
#define HDR_MDART_DARP(p)		(struct hdr_mdart_darp *)hdr_mdart::access(p)
// Dynamic Address Update packet header macro
#define HDR_MDART_DAUP(p)		(struct hdr_mdart_daup *)hdr_mdart::access(p)
// Dynamic Address Broadcast packet header macro
#define HDR_MDART_DABR(p)		(struct hdr_mdart_dabr *)hdr_mdart::access(p)
// Encapsualting packet header macro
#define HDR_MDART_ENCP(p)		(struct hdr_mdart_encp *)hdr_mdart::access(p)
// Info packet header macro
#define HDR_MDART_INFO(p)		(struct hdr_mdart_info *)hdr_mdart::access(p)



//------------------------------------------------------------------------------
// General MDART header, shared by all formats
//------------------------------------------------------------------------------
struct hdr_mdart {
	u_int8_t	type_;
	// required by PacketHeaderManager
	static int offset_;
	inline static int& offset() {
		return offset_;
	}
	// Header access method
	inline static hdr_mdart* access(const Packet* p) {
		return (hdr_mdart*)p->access(offset_);
	}
	inline int size() {
		int size_ = 0;
		size_ = sizeof(int)			// offset_
				+ sizeof(u_int8_t);	// type_
		assert(size_ >= 0);
		return size_;
	}
};



//------------------------------------------------------------------------------
// Hello packet header
//------------------------------------------------------------------------------
struct hdr_mdart_hello {
	u_int8_t	type_;
	nsaddr_t	srcId_;					// Source identifier
	nsaddr_t	srcAdd_;				// Source dynamic address
	nsaddr_t	dstAdd_;				// Destination dynamic address
	u_int32_t	seqNum_;				// Sequence number
	inline int size() {
		int size_ = 0;
		size_ = sizeof(u_int8_t)		// type_
				+ sizeof(nsaddr_t)		// dst_
				+ sizeof(nsaddr_t)		// src_
				+ sizeof(nsaddr_t)		// id_
				+ sizeof(u_int32_t);	// seqNum_
		assert(size_ >= 0);
		return size_;
	}
};



//------------------------------------------------------------------------------
// Dynamic Address Request packet header
//------------------------------------------------------------------------------
struct hdr_mdart_darq {
	u_int8_t		type_;
	nsaddr_t		srcId_;				// Source identifier
	nsaddr_t		srcAdd_;			// Source dynamic address
	nsaddr_t		forId_;				// Forwarder identifier
	nsaddr_t		forAdd_;			// Forwarder dynamic address
	nsaddr_t		dstId_;				// Destination identifier
	nsaddr_t		dstAdd_;			// Destination dynamic address
	nsaddr_t		reqId_;				// Identifier for which a dynamic address is request
	double			lifetime_;			// Lifetime
	u_int32_t		seqNum_;			// Sequence number
	inline int size() {
		int size_ = 0;
		size_ = sizeof(u_int8_t)	// type_
			+ sizeof(nsaddr_t)		// srcId_
			+ sizeof(nsaddr_t)		// srcAdd_
			+ sizeof(nsaddr_t)		// forId_
			+ sizeof(nsaddr_t)		// forAdd_
			+ sizeof(nsaddr_t)		// dstId_
			+ sizeof(nsaddr_t)		// dstAdd_
			+ sizeof(nsaddr_t)		// reqId_
			+ sizeof(double)			// lifetime_
			+ sizeof(u_int32_t);		// seqNum_
		assert(size_ >= 0);
		return size_;
	}
};



//------------------------------------------------------------------------------
// Dynamic Address Reply packet header
//------------------------------------------------------------------------------
struct hdr_mdart_darp {
	u_int8_t		type_;				// Type
	nsaddr_t		srcId_;				// Source identifier
	nsaddr_t		srcAdd_;				// Source dynamic address
	nsaddr_t		forId_;				// Forwarder identifier
	nsaddr_t		forAdd_;				// Forwarder dynamic address
	nsaddr_t		dstId_;				// Destination identifier
	nsaddr_t		dstAdd_;				// Destination dynamic address
	nsaddr_t		reqId_;				// Identifier request
	nsaddr_t		reqAdd_;				// Dynamic address request
	double		lifetime_;			// Lifetime
	u_int32_t	seqNum_;				// Sequence number
	inline int size() {
		int size_ = 0;
		size_ = sizeof(u_int8_t)	// type_
			+ sizeof(nsaddr_t)		// srcId_
			+ sizeof(nsaddr_t)		// srcAdd_
			+ sizeof(nsaddr_t)		// forId_
			+ sizeof(nsaddr_t)		// forAdd_
			+ sizeof(nsaddr_t)		// dstId_
			+ sizeof(nsaddr_t)		// dstAdd_
			+ sizeof(nsaddr_t)		// reqId_
			+ sizeof(nsaddr_t)		// reqAdd_
			+ sizeof(double)			// lifetime_
			+ sizeof(u_int32_t);		// seqNum_
		assert(size_ >= 0);
		return size_;
	}
};



//------------------------------------------------------------------------------
// Dynamic Address Update packet header
//------------------------------------------------------------------------------
struct hdr_mdart_daup {
	u_int8_t	type_;
	nsaddr_t	srcId_;				// Source identifier
	nsaddr_t	srcAdd_;			// Source dynamic address
	nsaddr_t	forId_;				// Forwarder identifier
	nsaddr_t	forAdd_;			// Forwarder dynamic address
	nsaddr_t	dstId_;				// Destination identifier
	nsaddr_t	dstAdd_;			// Destination dynamic address
	double		lifetime_;			// Lifetime
	u_int32_t	seqNum_;			// Sequence number
		inline int size() {
		int size_ = 0;
		size_ = sizeof(u_int8_t)	// type_
			+ sizeof(nsaddr_t)		// srcId_
			+ sizeof(nsaddr_t)		// srcAdd_
			+ sizeof(nsaddr_t)		// forId_
			+ sizeof(nsaddr_t)		// forAdd_
			+ sizeof(nsaddr_t)		// dstId_
			+ sizeof(nsaddr_t)		// dstAdd_
			+ sizeof(double)		// lifetime_
			+ sizeof(u_int32_t);	// seqNum_
		assert(size_ >= 0);
		return size_;
	}
};



//------------------------------------------------------------------------------
// Dynamic Address Broadcast packet header
//------------------------------------------------------------------------------
struct hdr_mdart_dabr {
	u_int8_t	type_;
	nsaddr_t	srcId_;			// Source identifier
	nsaddr_t	srcAdd_;		// Source dynamic address
	nsaddr_t	dstAdd_;		// Destination dynamic address
	inline int size() {
		int size_ = 0;
		size_ = sizeof(u_int8_t)	// type_
				+ sizeof(nsaddr_t)	// dst_
				+ sizeof(nsaddr_t)	// src_
				+ sizeof(nsaddr_t);	// id_
		assert(size_ >= 0);
		return size_;
	}
};



//------------------------------------------------------------------------------
// Dynamic Address Encapsulate packet header
//------------------------------------------------------------------------------
struct hdr_mdart_encp {
	nsaddr_t	srcId_;					// Source identifier
	nsaddr_t	srcAdd_;					// Source dynamic address
	nsaddr_t	forId_;					// Forwarder identifier
	nsaddr_t	forAdd_;					// Forwarder dynamic address
	nsaddr_t	dstId_;					// Destination identifier
	nsaddr_t	dstAdd_;					// Destination dynamic address
	double		lifetime_;			// Lifetime
	u_int32_t	txCount_;			// # of mac recovery
	u_int32_t	darqCount_;			// # of DARQ packets already sent
	inline int size() {
		int size_ = 0;
		size_ = sizeof(nsaddr_t)	// srcId_
			+ sizeof(nsaddr_t)		// srcAdd_
			+ sizeof(nsaddr_t)		// forId_
			+ sizeof(nsaddr_t)		// forAdd_
			+ sizeof(nsaddr_t)		// dstId_
			+ sizeof(nsaddr_t)		// dstAdd_
			+ sizeof(double)			// lifetime_
			+ sizeof(u_int32_t)		// txCount_
			+ sizeof(u_int32_t);		// darqCount_
		assert(size_ >= 0);
		return size_;
	}
};



//------------------------------------------------------------------------------
// Info Store packet header
//------------------------------------------------------------------------------
struct hdr_mdart_info {
	u_int8_t	type_;
	nsaddr_t	srcId_;					// Source identifier
	nsaddr_t	srcAdd_;					// Source dynamic address
	nsaddr_t	forId_;					// Forwarder identifier
	nsaddr_t	forAdd_;					// Forwarder dynamic address
	nsaddr_t	dstId_;					// Destination identifier
	nsaddr_t	dstAdd_;					// Destination dynamic address
	double lifetime_;					// Lifetime
	u_int32_t seqNum_;				// Sequence number
	inline int size() {
		int size_ = 0;
		size_ = sizeof(u_int8_t)	// type_
			+ sizeof(nsaddr_t)		// srcId_
			+ sizeof(nsaddr_t)		// srcAdd_
			+ sizeof(nsaddr_t)		// forId_
			+ sizeof(nsaddr_t)		// forAdd_
			+ sizeof(nsaddr_t)		// dstId_
			+ sizeof(nsaddr_t)		// dstAdd_
			+ sizeof(double)			// lifetime_
			+ sizeof(u_int32_t);		// seqNum_
		assert(size_ >= 0);
		return size_;
	}
};


// for size calculation of header-space reservation
union hdr_all_mdart {
	hdr_mdart           h;
	hdr_mdart_hello hello;
	hdr_mdart_darq darq;
	hdr_mdart_darp darp;
	hdr_mdart_daup daup;
	hdr_mdart_dabr dabr;
	hdr_mdart_encp encp;
	hdr_mdart_info info;
};

#endif
