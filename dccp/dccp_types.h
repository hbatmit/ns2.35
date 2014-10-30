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
 * $Id: dccp_types.h,v 1.1 2010/04/06 05:09:01 tom_henderson Exp $ */

#ifndef ns_dccp_types_h
#define ns_dccp_types_h

#define DCCP_NUM_PTYPES 8

enum dccp_packet_type {
     DCCP_REQUEST = 0, DCCP_RESPONSE = 1, DCCP_DATA = 2,
     DCCP_ACK = 3, DCCP_DATAACK = 4, DCCP_CLOSEREQ = 5,
     DCCP_CLOSE = 6, DCCP_RESET = 7
};

#define DCCP_IP_HDR_SIZE 20
#define DCCP_GEN_HDR_SIZE 12+DCCP_IP_HDR_SIZE

#define DCCP_REQ_HDR_SIZE   4+DCCP_GEN_HDR_SIZE
#define DCCP_RESP_HDR_SIZE  4+DCCP_GEN_HDR_SIZE
#define DCCP_DATA_HDR_SIZE  0+DCCP_GEN_HDR_SIZE
#define DCCP_ACK_HDR_SIZE   4+DCCP_GEN_HDR_SIZE
#define DCCP_DACK_HDR_SIZE  4+DCCP_GEN_HDR_SIZE
#define DCCP_CREQ_HDR_SIZE  4+DCCP_GEN_HDR_SIZE
#define DCCP_CLOSE_HDR_SIZE 4+DCCP_GEN_HDR_SIZE
#define DCCP_RESET_HDR_SIZE 8+DCCP_GEN_HDR_SIZE

#define DCCP_NDP_LIMIT 8
#define DCCP_CCVAL_LIMIT 16

#define DCCP_NUM_RESET_REASONS 17

enum dccp_reset_reason {
	DCCP_RST_UNSPEC = 0, DCCP_RST_CLOSED = 1,
	DCCP_RST_INVALID_PKT = 2, DCCP_RST_OPT_ERR = 3,
	DCCP_RST_FEATURE_ERR = 4, DCCP_RST_CONN_REF = 5,
	DCCP_RST_BAD_SNAME = 6, DCCP_RST_TOO_BUSY = 7,
	DCCP_RST_BAD_INITC = 8, DCCP_RST_UNANSW_CHAL = 10,
	DCCP_RST_FLESS_NEG = 11, DCCP_RST_AGG_PEN = 12,
	DCCP_RST_NO_CONN = 13, DCCP_RST_ABORTED = 14,
	DCCP_RST_EXT_SEQ = 15, DCCP_RST_MANDATORY = 16
};

#define DCCP_NUM_STATES 7

enum dccp_state { DCCP_STATE_CLOSED, DCCP_STATE_LISTEN,
		  DCCP_STATE_RESPOND, DCCP_STATE_REQUEST,
		  DCCP_STATE_OPEN, DCCP_STATE_CLOSEREQ,
		  DCCP_STATE_CLOSING //, DCCP_STATE_TIMEWAIT
};

#define DCCP_NUM_FEAT_LOCS 2

enum dccp_feature_location { DCCP_FEAT_LOC_LOCAL = 0,
			     DCCP_FEAT_LOC_REMOTE = 1 };

enum dccp_feature_type { DCCP_FEAT_TYPE_SP = 0,
			 DCCP_FEAT_TYPE_NN = 1,
                         DCCP_FEAT_TYPE_UNKNOWN = 3};

#define DCCP_NUM_PACKET_STATES 4

enum dccp_packet_state { DCCP_PACKET_RECV = 0, DCCP_PACKET_ECN = 1,
			 DCCP_PACKET_RESERVED = 2, DCCP_PACKET_NOT_RECV = 3};

enum dccp_ecn_codepoint { ECN_ECT0 = 0, ECN_ECT1 = 1,
			   ECN_NOT_ECT = 2, ECN_CE = 3 };

#endif

