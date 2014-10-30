/********************************************/
/*     NS2 Simulator for IEEE 802.15.4      */
/*           (per P802.15.4/D18)            */
/*------------------------------------------*/
/* by:        Jianliang Zheng               */
/*        (zheng@ee.ccny.cuny.edu)          */
/*              Myung J. Lee                */
/*          (lee@ccny.cuny.edu)             */
/*        ~~~~~~~~~~~~~~~~~~~~~~~~~         */
/*           SAIT-CUNY Joint Lab            */
/********************************************/

// File:  p802_15_4trace.cc
// Mode:  C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t

// $Header: /cvsroot/nsnam/ns-2/wpan/p802_15_4trace.cc,v 1.3 2011/06/22 04:03:26 tom_henderson Exp $

/*
 * Copyright (c) 2003-2004 Samsung Advanced Institute of Technology and
 * The City University of New York. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Samsung Advanced Institute of Technology nor of 
 *    The City University of New York may be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE JOINT LAB OF SAMSUNG ADVANCED INSTITUTE
 * OF TECHNOLOGY AND THE CITY UNIVERSITY OF NEW YORK ``AS IS'' AND ANY EXPRESS 
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN 
 * NO EVENT SHALL SAMSUNG ADVANCED INSTITUTE OR THE CITY UNIVERSITY OF NEW YORK 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <packet.h>
#include <mac.h>
#include <mac-802_11.h>
#include "p802_15_4trace.h"

//ns-2.27 changed 'dh_sa' to 'dh_ta' and 'dh_da' to 'dh_ra' in mac-802_11.h/cc
#define dh_sa dh_ta
#define dh_da dh_ra

#define use_hdr_802_11_for_trace

#ifndef use_hdr_802_11_for_trace

int p802_15_4macSA(Packet *p)
{
	struct hdr_mac *mh = HDR_MAC(p);
	return mh->macSA();
}

int p802_15_4macDA(Packet *p)
{
	struct hdr_mac *mh = HDR_MAC(p);
	return mh->macDA();
}

u_int16_t p802_15_4hdr_type(Packet *p)
{
	struct hdr_mac *mh = HDR_MAC(p);
	return mh->hdr_type();
}

int p802_15_4hdr_dst(char* hdr,int dst)
{
	struct hdr_mac *mh = (struct hdr_mac*) hdr;
	if(dst > -2)
		mh->macDA_ = dst;
	return mh->macDA();
}

int p802_15_4hdr_src(char* hdr,int src)
{
	struct hdr_mac *mh = (struct hdr_mac*) hdr;
	if(src > -2)
		mh->macSA_ = src;
	return mh->macSA();
}

int p802_15_4hdr_type(char* hdr, u_int16_t type)
{
	struct hdr_mac *mh = (struct hdr_mac*) hdr;
	if (type)
		mh->hdr_type_ = type;
	return mh->hdr_type();
}

void p802_15_4hdrDATA(Packet *p)
{
	;	//nothing to do
}

void p802_15_4hdrACK(Packet *p)
{
	;	//nothing to do
}

void p802_15_4hdrBeacon(Packet *p)
{
	;	//nothing to do
}

void p802_15_4hdrCommand(Packet *p, u_int16_t type)
{
	;	//nothing to do
}

#else

int p802_15_4macSA(Packet *p)
{
	struct hdr_mac802_11 *mh = HDR_MAC802_11(p);
	return ETHER_ADDR(mh->dh_sa);
	
}

int p802_15_4macDA(Packet *p)
{
	struct hdr_mac802_11 *mh = HDR_MAC802_11(p);
	return ETHER_ADDR(mh->dh_da);
}

u_int16_t p802_15_4hdr_type(Packet *p)
{
	struct hdr_mac802_11 *mh = HDR_MAC802_11(p);
	return GET2BYTE(mh->dh_body);
}

int p802_15_4hdr_dst(char* hdr,int dst)
{
	struct hdr_mac802_11 *mh = (struct hdr_mac802_11*) hdr;
	if(dst > -2)
		STORE4BYTE(&dst, (mh->dh_da));
	return ETHER_ADDR(mh->dh_da);
}

int p802_15_4hdr_src(char* hdr,int src)
{
	struct hdr_mac802_11 *mh = (struct hdr_mac802_11*) hdr;
	if(src > -2)
		STORE4BYTE(&src, (mh->dh_sa));
	return ETHER_ADDR(mh->dh_sa);
}

int p802_15_4hdr_type(char* hdr, u_int16_t type)
{
	struct hdr_mac802_11 *mh = (struct hdr_mac802_11*) hdr;
	if(type)
		STORE2BYTE(&type,(mh->dh_body));
	return GET2BYTE(mh->dh_body);
}

void p802_15_4hdrDATA(Packet *p)
{
	struct hdr_mac802_11* mh = HDR_MAC802_11(p);

	//ch->size() += ETHER_HDR_LEN11;

	mh->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
	mh->dh_fc.fc_type = MAC_Type_Data;
	mh->dh_fc.fc_subtype = MAC_Subtype_Data;
}

void p802_15_4hdrACK(Packet *p)
{
	struct ack_frame *af = (struct ack_frame*)p->access(hdr_mac::offset_);

	af->af_fc.fc_protocol_version = MAC_ProtocolVersion;
 	af->af_fc.fc_type = MAC_Type_Control;
 	af->af_fc.fc_subtype = MAC_Subtype_ACK;
}

void p802_15_4hdrBeacon(Packet *p)
{
	struct hdr_mac802_11* mh = HDR_MAC802_11(p);

	mh->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
	mh->dh_fc.fc_type = MAC_Type_Control;		//doesn't really matter
	mh->dh_fc.fc_subtype = MAC_Subtype_Beacon;
}

void p802_15_4hdrCommand(Packet *p, u_int16_t type)
{
	struct hdr_mac802_11* mh = HDR_MAC802_11(p);

	mh->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
	mh->dh_fc.fc_type = MAC_Type_Control;		//doesn't really matter
	mh->dh_fc.fc_subtype = type;
}

#endif

const char *wpan_pName(Packet *p)
{
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_mac802_11 *mh = HDR_MAC802_11(p);
	return
	((ch->ptype() == PT_MAC) ? (
	  (mh->dh_fc.fc_subtype == MAC_Subtype_Beacon) ? "M_BEACON"  :			//Beacon
	  (mh->dh_fc.fc_subtype == MAC_Subtype_Command_AssoReq) ? "M_CM1_AssoReq"  :	//CMD: Association request
	  (mh->dh_fc.fc_subtype == MAC_Subtype_Command_AssoRsp) ? "M_CM2_AssoRsp"  :	//CMD: Association response
	  (mh->dh_fc.fc_subtype == MAC_Subtype_Command_DAssNtf) ? "M_CM3_DisaNtf"  :	//CMD: Disassociation notification
	  (mh->dh_fc.fc_subtype == MAC_Subtype_Command_DataReq) ? "M_CM4_DataReq"  :	//CMD: Data request
	  (mh->dh_fc.fc_subtype == MAC_Subtype_Command_PIDCNtf) ? "M_CM5_PidCNtf"  :	//CMD: PAN ID conflict notification
	  (mh->dh_fc.fc_subtype == MAC_Subtype_Command_OrphNtf) ? "M_CM6_OrphNtf"  :	//CMD: Orphan notification
	  (mh->dh_fc.fc_subtype == MAC_Subtype_Command_BconReq) ? "M_CM7_Bcn-Req"  :	//CMD: Beacon request
	  (mh->dh_fc.fc_subtype == MAC_Subtype_Command_CoorRea) ? "M_CM8_CoorRal"  :	//CMD: Coordinator realignment
	  (mh->dh_fc.fc_subtype == MAC_Subtype_Command_GTSReq) ?  "M_CM9_GTS-Req"  :	//CMD: GTS request
	  (mh->dh_fc.fc_subtype == MAC_Subtype_ACK) ? "M_ACK"  :			//Acknowledgement
	  "M_UNKN"
	  ) : packet_info.name(ch->ptype()));
}

// End of file: p802_15_4trace.cc
