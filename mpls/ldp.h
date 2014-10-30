// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-

/*
 * ldp.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: ldp.h,v 1.4 2005/08/25 18:58:09 johnh Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */

// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
// Original source contributed by Gaeil Ahn. See below.
//
// $Header: /cvsroot/nsnam/ns-2/mpls/ldp.h,v 1.4 2005/08/25 18:58:09 johnh Exp $

/**************************************************************************
* Copyright (c) 2000 by Gaeil Ahn                                   	  *
* Everyone is permitted to copy and distribute this software.		  *
* Please send mail to fog1@ce.cnu.ac.kr when you modify or distribute     *
* this sources.								  *
**************************************************************************/

/***********************************************************
*                                                          *
*    File: Header File for LDP & CR-LDP protocol           *
*    Author: Gaeil Ahn (fog1@ce.cnu.ac.kr), Jan. 2000      *
*                                                          *
***********************************************************/

#ifndef ns_ldp_h
#define ns_ldp_h

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "address.h"
#include "ip.h"

const int LDP_MaxMSGTEntryNB = 100;

/* LDP msg types */
const int LDP_NotificationMSG = 0x0001;
const int LDP_MappingMSG =      0x0400;
const int LDP_RequestMSG =      0x0401;
const int LDP_WithdrawMSG =     0x0402;
const int LDP_ReleaseMSG =      0x0403;

const int LDP_LoopDetected =    0x000B;
const int LDP_NoRoute =         0x000D;

const int LDP_LabelALLOC =      0;
const int LDP_LabelPASS =       1;
const int LDP_LabelSTACK =      2;

struct hdr_ldp {
	int  msgtype;
	int  msgid;  	// request msg id (mapping msg triggered by request 
			// msg). if (msgid > -1): by request, else:  no 
	int  fec;
	int  label;
	
	int  reqmsgid;
	int  status;      // for Notification

	// XXX This is VERY BAD behavior! Should NEVER put a pointer
	// in a packet header!!

	char *pathvec; 
	// The following is for CR-LSP
	char *er;

	int  lspid;
	int  rc;

	// Header access methods
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_ldp* access(const Packet* p) {
		return (hdr_ldp*)p->access(offset_);
	}
};

// Used for supporting ordered distribuiton mode (Explicit LSP)
struct MsgTable {
	int  MsgID;
	int  FEC;
	int  LspID;
	int  Src;
	int  PMsgID;	// previsou msg-id
	int  LabelOp;   // LabelALLOC, LabelPASS, or LabelSTACK
	int  ERLspID;
};

struct MsgT {
	MsgTable Entry[LDP_MaxMSGTEntryNB];
	int      NB;
};


class LDPAgent : public Agent {
public:
	LDPAgent();

	virtual int command(int argc, const char*const* argv);
	virtual void recv(Packet*, Handler*);

	virtual void delay_bind_init_all();
	virtual int delay_bind_dispatch(const char *, const char *, 
					TclObject *);
  
	inline int peer() const { return peer_; }
	inline void turn_on_trace() { trace_ldp_ = 1; }

	void PKTinit(hdr_ldp *hdrldp, int msgtype, const char *pathvec, 
		     const char *er);
	int  PKTsize(const char *pathvec, const char *er);
	
	int  MSGTinsert(int MsgID, int FEC, int LspID, int Src, int PMsgID);
	void MSGTdelete(int entrynb);
	int  MSGTlocate(int MsgID);
	int  MSGTlocate(int FEC,int LspID,int Src);
	void MSGTlookup(int entrynb, int &MsgID, int &FEC, int &LspID, 
			int &src, int &PMsgID, int &LabelOp);
	void MSGTdump();
   
protected:
	int    new_msgid_;
	int    trace_ldp_;
	int    peer_;
	void   trace(ns_addr_t src, hdr_ldp *hdrldp);
  
	char* parse_msgtype(int msgtype, int lspid);
	char* parse_status(int status);
  
	MsgT  MSGT_;
};

#endif
