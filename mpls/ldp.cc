// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-

/*
 * ldp.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: ldp.cc,v 1.9 2005/08/25 18:58:09 johnh Exp $
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

//
// Original source contributed by Gaeil Ahn. See below.
//
// $Header: /cvsroot/nsnam/ns-2/mpls/ldp.cc,v 1.9 2005/08/25 18:58:09 johnh Exp $

/**************************************************************************
* Copyright (c) 2000 by Gaeil Ahn                                   	  *
* Everyone is permitted to copy and distribute this software.		  *
* Please send mail to fog1@ce.cnu.ac.kr when you modify or distribute     *
* this sources.								  *
**************************************************************************/

/***********************************************************
*                                                          *
*    File: File for LDP & CR-LDP protocol                   *
*    Author: Gaeil Ahn (fog1@ce.cnu.ac.kr), Jan. 2000      *
*                                                          *
***********************************************************/

#include "config.h"

#include <iostream>

#include "ldp.h"

int hdr_ldp::offset_;

static class LDPHeaderClass : public PacketHeaderClass {
public:
	LDPHeaderClass() : PacketHeaderClass("PacketHeader/LDP", 
					     sizeof(hdr_ldp)) {
		bind_offset(&hdr_ldp::offset_);
	}
} class_ldphdr;

static class LDPClass : public TclClass {
public:
	LDPClass() : TclClass("Agent/LDP") {}
	TclObject* create(int, const char*const*) {
		return (new LDPAgent());
	}
} class_agentldp;

LDPAgent::LDPAgent() : Agent(PT_LDP), 
	new_msgid_(0), trace_ldp_(0), peer_(0)
{
	MSGT_.NB      = 0;
}

void LDPAgent::delay_bind_init_all()
{
	delay_bind_init_one("trace_ldp_");
	Agent::delay_bind_init_all();
}

int LDPAgent::delay_bind_dispatch(const char *varName, const char *localName, 
				  TclObject *tracer)
{
	if (delay_bind_bool(varName,localName,"trace_ldp_",&trace_ldp_,tracer))
		return TCL_OK;
	return Agent::delay_bind_dispatch(varName, localName, tracer);
}

int LDPAgent::PKTsize(const char *pathvec, const char *er)
{
	// header size for Version, PDU Length, and LDP Identifier
	int psize = 10; 
	// size for message type and message id
	psize += 8; 
    
	int len = strlen(pathvec);
	if (len > 1) {
		psize += 4;   // size for path vector header
		for (int i=0; i<len; i++)
			if ( *(pathvec+i) == ' ' ) {
				psize += 4;   // size for path vector data
			}  
	}     

	len = strlen(er);
	if (len > 1) { 
		psize += 4;   // size for explicit route header
		for (int i=0; i<len; i++)
			if (*(er+i) == ' ')
				psize += 4;   // size for explicit route data 
	}
	return (psize);
}

void LDPAgent::PKTinit(hdr_ldp *hdrldp, int msgtype, 
		       const char *pathvec, const char *er)
{
	hdrldp->msgtype = msgtype;
	hdrldp->msgid   = new_msgid_;
	hdrldp->fec     = -1;
	hdrldp->label   = -1;
	hdrldp->reqmsgid= -1;
	hdrldp->status  = -1;

	int i;
	int len = strlen(pathvec);
	hdrldp->pathvec = (char *) malloc(len+1);
	for (i=0; i<len; i++) { 
		if ( *(pathvec+i) == ' ' )
			*(hdrldp->pathvec+i) = '_';
		else
			*(hdrldp->pathvec+i) = *(pathvec+i);
	}
	*(hdrldp->pathvec+len) = '\0';

	len = strlen(er);
	hdrldp->er = (char *) malloc(len+1);
	for (i=0; i<len; i++) { 
		if ( *(er+i) == ' ' )
			*(hdrldp->er+i) = '_';
		else
			*(hdrldp->er+i) = *(er+i);
	}
	*(hdrldp->er+len) = '\0';

	hdrldp->lspid = -1;
	hdrldp->rc    = -1;
}

int LDPAgent::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {      
		if (strcmp(argv[1], "msgtbl-dump") == 0) {              
			MSGTdump();
			return (TCL_OK);
		} else if (strcmp(argv[1], "new-msgid") == 0) {
			tcl.resultf("%d", ++new_msgid_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "peer-ldpnode") == 0) {
			tcl.resultf("%d", peer_);
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "set-peer") == 0) {
			peer_ = atoi(argv[2]);
			return (TCL_OK);
		}

		// The following is shared by all if-s in this branch
		int MsgID = atoi(argv[2]);
		int msgid,fec,lspid,src,pmsgid,labelop;
		int entrynb = MSGTlocate(MsgID);
		MSGTlookup(entrynb,msgid,fec,lspid,src,pmsgid,labelop);

		if (strcmp(argv[1], "msgtbl-clear") == 0) {
			/* <agent> MSGTdelete MsgID */
			if (entrynb > -1)  
				MSGTdelete(entrynb);
			return (TCL_OK);
		} else if (strcmp(argv[1], "msgtbl-get-src") == 0) {
			/* <agent> GetMSGTsrc MsgID */
			tcl.resultf("%d", src);
			return (TCL_OK);
		} else if (strcmp(argv[1], "msgtbl-get-reqmsgid") == 0)	{
			/* <agent> GetMSGTpmsgid msgid */
			tcl.resultf("%d",pmsgid);
			return (TCL_OK);
		} else if (strcmp(argv[1], "msgtbl-get-labelop") == 0) {
			/* <agent> GetMSGTlabelpass msgid */
			tcl.resultf("%d", labelop);
			return (TCL_OK);
		} else if (strcmp(argv[1], "msgtbl-get-erlspid") == 0) {
			/* 
			 * <agent> msgtbl-get-erlspid <msgid>
			 */
			tcl.resultf("%d",MSGT_.Entry[entrynb].ERLspID);
			return (TCL_OK);
		} else if (strcmp(argv[1], "msgtbl-set-labelpass") == 0) {
			/* 
			 * <agent> msgtbl-set-labelpass <msgid>
			 */
			MSGT_.Entry[entrynb].LabelOp = LDP_LabelPASS;
			return (TCL_OK);
		} 
	} else if (argc == 4) {      
		if (strcmp(argv[1], "notification-msg") == 0) {
			/* 
			 * <agent> notification-msg <status> <lspid>
			 */
			size_ = PKTsize("*", "*");
			if ( atoi(argv[3]) > -1 )  /* packet size adjustment */
				size_ += 16;
			else
				size_ += 8;
			Packet* pkt = allocpkt();
			hdr_ldp *hdrldp = hdr_ldp::access(pkt);
			PKTinit(hdrldp, LDP_NotificationMSG, "*", "*");

			if (strcmp(argv[2], "LoopDetected") == 0)
				hdrldp->status = LDP_LoopDetected;
			else if (strcmp(argv[2], "NoRoute") == 0)
				hdrldp->status  = LDP_NoRoute;
       
			hdrldp->lspid   = atoi(argv[3]);
			send(pkt, 0);
			return (TCL_OK);
		} else if (strcmp(argv[1], "withdraw-msg") == 0) {
			/* 
			 * <agent> withdraw-msg <fec> <lspid>
			 */
			size_ = PKTsize("*","*");
			if ( atoi(argv[3]) > -1 )  /* packet size adjustment */
				size_ += 16;
			else
				size_ += 8;
			Packet* pkt = allocpkt();
			hdr_ldp *hdrldp = hdr_ldp::access(pkt);
			PKTinit(hdrldp, LDP_WithdrawMSG, "*", "*");
			hdrldp->fec    = atoi(argv[2]);
			hdrldp->lspid  = atoi(argv[3]);
			send(pkt, 0);
			return (TCL_OK);
		} else if (strcmp(argv[1], "release-msg") == 0) {
			/* 
			 * <agent> release-msg <fec> <lspid>
			 */
			size_ = PKTsize("*","*");
			if ( atoi(argv[3]) > -1 )  /* packet size adjustment */
				size_ += 16;
			else
				size_ += 8;
			Packet* pkt = allocpkt();
			hdr_ldp *hdrldp = hdr_ldp::access(pkt);
			PKTinit(hdrldp, LDP_ReleaseMSG,"*","*");
			hdrldp->fec     = atoi(argv[2]);
			hdrldp->lspid   = atoi(argv[3]);
			send(pkt, 0);
			return (TCL_OK);
		} else if (strcmp(argv[1], "request-msg") == 0) {
			/* 
			 * <agent> request-msg <fec> <pathvec>
			 */
			size_ = PKTsize(argv[3],"*");
			size_ += 8;      /* packet size adjustment */
			Packet* pkt = allocpkt();
			hdr_ldp *hdrldp = hdr_ldp::access(pkt);
			PKTinit(hdrldp, LDP_RequestMSG, argv[3], "*");
			hdrldp->fec     = atoi(argv[2]);
			send(pkt, 0);
			return (TCL_OK);
		} else if (strcmp(argv[1], "msgtbl-set-labelstack") == 0) {
			/*
			 * <agent> msgtbl-set-labelstack <msgid> <erlspid>
			 */
			int MsgID   = atoi(argv[2]);
			int ERLspID = atoi(argv[3]);
			int entrynb = MSGTlocate(MsgID);
			MSGT_.Entry[entrynb].LabelOp = LDP_LabelSTACK;
			MSGT_.Entry[entrynb].ERLspID = ERLspID;
			return (TCL_OK);
		} 
	} else if (argc == 5) {      
		if (strcmp(argv[1], "msgtbl-get-msgid") == 0) {
			/* 
			 * <classifier> msgtbl-get-msgid <FEC> <LspID> <Src>
			 */
			int msgid,tmp;
			int fec    = atoi(argv[2]);
			int LspID  = atoi(argv[3]);
			int src    = atoi(argv[4]);     
			int entrynb = MSGTlocate(fec,LspID,src);

			MSGTlookup(entrynb,msgid,tmp,tmp,tmp,tmp,tmp);
			tcl.resultf("%d", msgid);
			return (TCL_OK);
		}
	} else if (argc == 6) {
		if (strcmp(argv[1], "mapping-msg") == 0) {
			/* 
			 * <agent> mapping <fec> <label> <pathvec> <premsgid>
			 */
			size_ = PKTsize(argv[4],"*");
			if ( atoi(argv[5]) > -1 )  /* packet size adjustment*/
				size_ += 24;
			else
				size_ += 16;
         		Packet* pkt = allocpkt();
			hdr_ldp *hdrldp = hdr_ldp::access(pkt);
			PKTinit(hdrldp, LDP_MappingMSG, argv[4], "*");
			hdrldp->fec     = atoi(argv[2]);
			hdrldp->label   = atoi(argv[3]);
			hdrldp->reqmsgid= atoi(argv[5]);
			send(pkt, 0);
			return (TCL_OK);
		} else if (strcmp(argv[1], "cr-mapping-msg") == 0) {
			/* 
			 * <agent> cr-mapping-msg <fec> <label> <lspid>
			 *         <premsgid>
			 */
			size_ = PKTsize("*","*");
			size_ += 32;      /* packet size adjustment */
			Packet* pkt = allocpkt();
			hdr_ldp *hdrldp = hdr_ldp::access(pkt);
			PKTinit(hdrldp, LDP_MappingMSG, "*", "*");
			hdrldp->fec     = atoi(argv[2]);
			hdrldp->label   = atoi(argv[3]);
			hdrldp->lspid   = atoi(argv[4]);
			hdrldp->reqmsgid= atoi(argv[5]);
			send(pkt, 0);
			return (TCL_OK);
		}
	} else if (argc == 7) {      
		if (strcmp(argv[1], "cr-request-msg") == 0) {
			/* 
			 * <agent> cr-request <fec> <pathvec> <er> <lspid> <rc>
			 */
			size_ = PKTsize(argv[3],argv[4]);
			if (atoi(argv[6]) > -1)  /* packet size adjustment */
				size_ += 24;
			else
				size_ += 16;
			Packet* pkt = allocpkt();
			hdr_ldp *hdrldp = hdr_ldp::access(pkt);
			PKTinit(hdrldp, LDP_RequestMSG, argv[3], argv[4]);
			hdrldp->fec     = atoi(argv[2]);
			hdrldp->lspid   = atoi(argv[5]);
			hdrldp->rc      = atoi(argv[6]);
			send(pkt, 0);
			return (TCL_OK);
		} else if (strcmp(argv[1], "msgtbl-install") == 0) {
			/* 
			 * <agent> msgtbl-install <msgid> <FEC> <LspID> 
			 *         <Src> <Pmsgid>
			 */
			int msgid  = atoi(argv[2]);
			int fec    = atoi(argv[3]);
			int LspID  = atoi(argv[4]);
			int src    = atoi(argv[5]);     
			int PMsgID = atoi(argv[6]);     
			int entrynb = MSGTinsert(msgid,fec,LspID,src,PMsgID);
			tcl.resultf("%d", entrynb);
			return (TCL_OK);
		}
	}
	return (Agent::command(argc, argv));
}

void LDPAgent::recv(Packet* pkt, Handler*)
{
	char out[400];  
	Tcl& tcl = Tcl::instance();

	hdr_ldp *hdrldp = hdr_ldp::access(pkt);
	int msgtype = hdrldp->msgtype;
	int msgid   = hdrldp->msgid;
	int fec     = hdrldp->fec;
	int label   = hdrldp->label;
	int reqmsgid= hdrldp->reqmsgid;
	int status  = hdrldp->status;
	int lspid   = hdrldp->lspid;
	int rc      = hdrldp->rc;
  
	char pathvec[400];
	char er[400];
	strcpy(pathvec, hdrldp->pathvec);
	strcpy(er, hdrldp->er);

	ns_addr_t src = hdr_ip::access(pkt)->src_;
	trace(src, hdrldp);

	free(hdrldp->pathvec);
	free(hdrldp->er);
	Packet::free(pkt);

	switch (msgtype) {
	case LDP_NotificationMSG:    /* Notification   */
		char code[30];
		strcpy(code, parse_status(status));
		sprintf(out, "%s get-notification-msg %d %s %d", 
			name(), src.addr_, code, lspid);
		tcl.eval(out);
		break;

	case LDP_MappingMSG:    /* Label Mapping  */
		if (lspid >= 0) {
			/* CR-LDP */
			sprintf(out, "%s get-cr-mapping-msg %d %d %d %d %d %d",
				name(), msgid, src.addr_, fec, 
				label, lspid, reqmsgid);
		} else { 
			sprintf(out, "%s get-mapping-msg %d %d %d %d %s %d", 
				name(), msgid, src.addr_, fec, 
				label, pathvec, reqmsgid);
		}
		tcl.eval(out);
		break;

	case LDP_RequestMSG:    /* Label Request  */
		if (lspid >= 0) { 
			sprintf(out,
				"%s get-cr-request-msg %d %d %d %s %s %d %d", 
				name(), msgid, src.addr_, fec, pathvec,
				er, lspid, rc);
		} else { 
			sprintf(out, "%s get-request-msg %d %d %d %s", 
				name(), msgid, src.addr_, fec, pathvec);
		}
		tcl.eval(out);
		break;

	case LDP_WithdrawMSG:    /* Label Withdraw */
		sprintf(out, "%s get-withdraw-msg %d %d %d", 
			name(), src.addr_, fec, lspid);
		tcl.eval(out);
		break;

	case LDP_ReleaseMSG:    /* Label Release */
		sprintf(out, "%s get-release-msg %d %d %d", 
			name(), src.addr_, fec, lspid);
		tcl.eval(out);
		break;
	}
}

int LDPAgent::MSGTinsert(int MsgID, int FEC, int LspID, int Src, int PMsgID)
{
	if (MSGT_.NB == LDP_MaxMSGTEntryNB - 1)
		return(-1);

	if (MSGTlocate(FEC, LspID, Src) > -1)
		return(-1);
    
	MSGT_.Entry[MSGT_.NB].MsgID     = MsgID;
	MSGT_.Entry[MSGT_.NB].FEC       = FEC;
	MSGT_.Entry[MSGT_.NB].LspID     = LspID;
	MSGT_.Entry[MSGT_.NB].Src       = Src;
	MSGT_.Entry[MSGT_.NB].PMsgID    = PMsgID;
	MSGT_.Entry[MSGT_.NB].LabelOp   = LDP_LabelALLOC;
	MSGT_.Entry[MSGT_.NB].ERLspID   = -1;
    
	MSGT_.NB++;

	return(MSGT_.NB-1);
}

void LDPAgent::MSGTdelete(int entrynb)
{
	if ( (entrynb > -1) && (entrynb < MSGT_.NB) ) {
		MSGT_.Entry[entrynb].MsgID = -1;
		MSGT_.Entry[entrynb].FEC   = MSGT_.Entry[entrynb].LspID  = -1;
		MSGT_.Entry[entrynb].Src   = MSGT_.Entry[entrynb].PMsgID = -1;
		MSGT_.Entry[entrynb].LabelOp = -1;
		MSGT_.Entry[entrynb].ERLspID = -1;
	}   
}

int LDPAgent::MSGTlocate(int MsgID)
{
	if ( MsgID < 0 )
		return(-1);
    
	for (int i=0; i<MSGT_.NB; i++) {
		if ( MSGT_.Entry[i].MsgID == MsgID )
			return(i);
	}

	return(-1);
}

int LDPAgent::MSGTlocate(int FEC, int LspID, int Src)
{
	int i;
	if ( (FEC < 0) && (Src < 0) ) {
		if ( LspID > -1) {
			for (i=0; i<MSGT_.NB; i++)
				if ( MSGT_.Entry[i].LspID  == LspID )
					return(i);
		}
		return(-1);
	}
       
	for (i=0; i<MSGT_.NB; i++) {
		if ( (MSGT_.Entry[i].FEC   == FEC  ) &&
		     (MSGT_.Entry[i].LspID == LspID) &&
		     (MSGT_.Entry[i].Src   == Src  ) )
			
			return(i);
	}

	return(-1);
}

void LDPAgent::MSGTlookup(int entrynb, int &MsgID, int &FEC, int &LspID, 
			  int &Src, int &PMsgID, int &LabelOp)
{
	if ( (entrynb > -1) && (entrynb < MSGT_.NB) ) {  
		MsgID    = MSGT_.Entry[entrynb].MsgID;
		FEC      = MSGT_.Entry[entrynb].FEC;
		LspID    = MSGT_.Entry[entrynb].LspID;
		Src      = MSGT_.Entry[entrynb].Src;
		PMsgID   = MSGT_.Entry[entrynb].PMsgID;
		LabelOp= MSGT_.Entry[entrynb].LabelOp;
	} else {  
		MsgID = FEC = LspID = Src = PMsgID = LabelOp = -1;
	}
}

void LDPAgent::MSGTdump()
{
	for (int i = 0; i < MSGT_.NB; i++) {
		cerr << "  # MsgID =" << MSGT_.Entry[i].MsgID << "  ";
		cerr << "  # FEC   =" << MSGT_.Entry[i].FEC   << "  ";
		cerr << "  # LspID =" << MSGT_.Entry[i].LspID << "  ";
		cerr << "  # Src   =" << MSGT_.Entry[i].Src   << "  ";
		cerr << "  # PMsgID=" << MSGT_.Entry[i].PMsgID<< "  ";
		cerr << "  # LabelOp=" << MSGT_.Entry[i].LabelOp << "\n";
	}
}

void LDPAgent::trace(ns_addr_t src, hdr_ldp *hdrldp)
{
	// XXX will be changed later to directly write to a output channel
	// instead of sending stuff to otcl
	if (trace_ldp_ == 1) { 
		const char *msgtype = parse_msgtype(hdrldp->msgtype,
						    hdrldp->lspid);
		const char *status = (hdrldp->msgtype == 0x0001) ? 
			parse_status(hdrldp->status) : "*";
		Tcl::instance().evalf("%s trace-ldp-packet %d %d %s %d %d %d "
				      "%s %d %s %d %d %s %7f",
				      name(),
				      src.addr_,
				      src.port_,
				      msgtype, 
				      hdrldp->msgid, 
				      hdrldp->fec, 
				      hdrldp->label, 
				      hdrldp->pathvec, 
				      hdrldp->lspid, 
				      hdrldp->er, 
				      hdrldp->rc, 
				      hdrldp->reqmsgid,
				      status,
				      Scheduler::instance().clock());  
	}	
}

char* LDPAgent::parse_msgtype(int msgtype, int lspid)
{

	switch (msgtype) {
	case LDP_NotificationMSG:    /* Notification   */
		return("Notification");

	case LDP_MappingMSG:    /* Label Mapping  */
		if (lspid >= 0)          /* CR-LDP */
			return("CR-Mapping");
		else
			return("Mapping");
		
	case LDP_RequestMSG:    /* Label Request  */
		if (lspid >= 0)
			return("CR-Request");
		else
			return("Request");
		
	case LDP_WithdrawMSG:    /* Label Withdraw */
		return("Withdraw");
		
	case LDP_ReleaseMSG:    /* Label Release */
		return("Release");
	}

	return ("Error");
}

char* LDPAgent::parse_status(int status)
{
	switch (status) {
	case LDP_LoopDetected:
		return "LoopDetected";
		
	case LDP_NoRoute:
		return "NoRoute";
		
	default:
		return "Unknown";
	}
}
