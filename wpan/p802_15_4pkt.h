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

// File:  p802_15_4pkt.h
// Mode:  C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t

// $Header: /cvsroot/nsnam/ns-2/wpan/p802_15_4pkt.h,v 1.2 2011/06/22 04:03:26 tom_henderson Exp $

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


#ifndef p802_15_4pkt_h
#define p802_15_4pkt_h

#include <packet.h>
#include "p802_15_4const.h"
#include "p802_15_4field.h"

#define HDR_LRWPAN(p)	(hdr_lrwpan::access(p))

struct panAddrInfo
{
	UINT_16 panID;
	union
	{
		UINT_16 addr_16;
		IE3ADDR addr_64;
	};
};

//---PHY header---
#define defSHR_PreSeq		0x00000000
#define defSHR_SFD		0xe5
#define defPHY_HEADER_LEN	6

struct lrwpan_beacon_frame
{
	//---beacon frame (Figures 10,37)---
	UINT_32		SHR_PreSeq;		//(PPDU) Preamble Sequence (const 0x00000000)(sec. 6.3.1.1)
	UINT_8		SHR_SFD;		//(PPDU) Start of Frame Delimiter (const 0xe5)(sec 6.3.1.2)
	UINT_8		PHR_FrmLen;		//(PPDU) Frame Length (sec. 6.3.1.3)
	UINT_16		MHR_FrmCtrl;		//(PSDU/MPDU) Frame Control (Figure 35)
						// --leftmost bit numbered 0 and transmitted first
						// --(012):	Frame type (Table 65)
						//		 --(210)=000:		Beacon
						//		 --(210)=001:		Data
						//		 --(210)=010:		Ack.
						//		 --(210)=011:		MAC command
						//		 --(210)=others:	Reserved
						// --(3):	Security enabled
						// --(4):	Frame pending
						// --(5):	Ack. req.
						// --(6):	Intra PAN
						// --(789):	Reserved
						// --(ab):	Dest. addressing mode (Table 66)
						//		 --(ba)=00:	PAN ID and Addr. field not present
						//		 --(ba)=01:	Reserved
						//		 --(ba)=10:	16-bit short address
						//		 --(ba)=11:	64-bit extended address
						// --(cd):	Reserved
						// --(ef):	Source addressing mode (see Dest. addressing mode)
	UINT_8		MHR_BSN;		//(PSDU/MPDU) Beacon Sequence Number
	panAddrInfo	MHR_SrcAddrInfo;	//(PSDU/MPDU) Source Address Information
	UINT_16		MSDU_SuperSpec;		//(MSDU) Superframe Specification (Figures 40,59)
						// --(0123):	Beacon order
						// --(4567):	Superframe order
						// --(89ab):	Final CAP slot
						// --(c):	Battery life extension
						// --(d):	Reserved
						// --(e):	PAN Coordinator
						// --(f):	Association permit
	GTSFields	MSDU_GTSFields;		//GTS Fields (Figure 38)
	PendAddrFields	MSDU_PendAddrFields;	//(MSDU) Address Fields (Figure 39)
						// --(012):	# of short addressing pending
						// --(3):	Reserved
						// --(456):	# of extended addressing pending
						// --(7):	Reserved
	//MSDU_BeaconPL;			//(MSDU) Beacon Payload
	UINT_16		MFR_FCS;		//(PSDU/MPDU) FCS
};

struct lrwpan_data_frame
{
	//---data frame (Figures 11,45)---
	UINT_32		SHR_PreSeq;		//same as above
	UINT_8		SHR_SFD;		//same as above
	UINT_8		PHR_FrmLen;		//same as above
	UINT_16		MHR_FrmCtrl;		//same as above
	UINT_8		MHR_DSN;		//Date Sequence Number
	panAddrInfo	MHR_DstAddrInfo;	//(PSDU/MPDU) Source Address Information
	panAddrInfo	MHR_SrcAddrInfo;	//(PSDU/MPDU) Destination Address Information
	//MSDU_DataPL;				//(MSDU) Data Payload
	UINT_16		MFR_FCS;		//same as above
};

struct lrwpan_ack_frame
{
	//---acknowledgement frame (Figures 12,46)---
	UINT_32		SHR_PreSeq;		//same as above
	UINT_8		SHR_SFD;		//same as above
	UINT_8		PHR_FrmLen;		//same as above
	UINT_16		MHR_FrmCtrl;		//same as above
	UINT_8		MHR_DSN;		//same as above
	UINT_16		MFR_FCS;		//same as above
};

struct lrwpan_command_frame
{
	//---MAC command frame (Figures 13,47)---
	UINT_32		SHR_PreSeq;		//same as above
	UINT_8		SHR_SFD;		//same as above
	UINT_8		PHR_FrmLen;		//same as above
	UINT_16		MHR_FrmCtrl;		//same as above
	UINT_8		MHR_DSN;		//same as above
	panAddrInfo	MHR_DstAddrInfo;	//same as above
	panAddrInfo	MHR_SrcAddrInfo;	//same as above
	UINT_8		MSDU_CmdType;		//(MSDU) Command Type/Command frame identifier (Table 67)
						// --0x01:	Association request (Figures 48 -> 49)
						// --0x02:	Association response (Figure 50;Table 68)
						// --0x03:	Disassociation notification (Figure 51;Table 69)
						// --0x04:	Data request (Figure 52)
						// --0x05:	PAN ID conflict notification (Figure 53)
						// --0x06:	Orphan notification (Figure 54)
						// --0x07:	Beacon request (Figure 55)
						// --0x08:	Coordinator realignment (Figure 56
						// --0x09:	GTS request (Figures 57 -> 58)
						// --0x0a-0xff:	Reserved
	//MSDU_CmdPL;				//(MSDU) Command Payload
	UINT_16		MFR_FCS;		//same as above
};

struct hdr_lrwpan
{
	//---PHY header---
	UINT_32	SHR_PreSeq;
	UINT_8	SHR_SFD;
	UINT_8	PHR_FrmLen;
	
	//---MAC header---
	UINT_16		MHR_FrmCtrl;
	UINT_8		MHR_BDSN;
	panAddrInfo	MHR_DstAddrInfo;
	panAddrInfo	MHR_SrcAddrInfo;

	//---PHY layer---
	UINT_8	ppduLinkQuality;
	double rxTotPower;
	
	//---MAC sublayer---
	UINT_16		MFR_FCS;
	UINT_16		MSDU_SuperSpec;
	GTSFields	MSDU_GTSFields;
	PendAddrFields	MSDU_PendAddrFields;
	UINT_8		MSDU_CmdType;
	UINT_8		MSDU_PayloadLen;
	UINT_16		pad;					
	UINT_8		MSDU_Payload[aMaxMACFrameSize];		//MSDU_BeaconPL/MSDU_DataPL/MSDU_CmdPL
	bool		SecurityUse;
	UINT_8		ACLEntry;

	//---SSCS entity---
	UINT_8		msduHandle;

	//---Other---
	bool		setSN;		//SN already been set
	UINT_8		phyCurrentChannel;
	bool		indirect;	//this is a pending packet (indirect transmission)
	UINT_32		uid;		//for debug purpose
	UINT_16		clusTreeDepth;
	UINT_16		clusTreeParentNodeID;
	
	bool		colFlag;	//for nam purpose
	int		attribute;	//for nam purpose


	//---Packet header access functions---
	static int offset_;
	inline static int& offset() {return offset_;}
	inline static hdr_lrwpan* access(const Packet* p)
	{
		return (hdr_lrwpan*) p->access(offset_);
	}
};

#endif

// End of file: p802_15_4pkt.h
