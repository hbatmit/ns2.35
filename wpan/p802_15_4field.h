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

// File:  p802_15_4field.h
// Mode:  C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t

// $Header: /cvsroot/nsnam/ns-2/wpan/p802_15_4field.h,v 1.2 2011/06/22 04:03:26 tom_henderson Exp $

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


#ifndef p802_15_4field_h
#define p802_15_4field_h

#include "p802_15_4def.h"

/*
#define STORELADDR(value,addr)	((*((unsigned char *)addr)) = ((*value) & 255 ,\
				 (*((unsigned char *)addr+1)) = ((*value) >> 8) & 255 ,\
				 (*((unsigned char *)addr+2)) = ((*value) >> 16) & 255 ,\
				 (*((unsigned char *)addr+3)) = ((*value) >> 24) & 255 ,\
				 (*((unsigned char *)addr+4)) = ((*value) >> 32) & 255 ,\
				 (*((unsigned char *)addr+5)) = ((*value) >> 40) & 255 ,\
				 (*((unsigned char *)addr+6)) = ((*value) >> 48) & 255 ,\
				 (*((unsigned char *)addr+7)) = ((*value) >> 56) & 255))
#define GETLADDR(addr)		 ((*(unsigned char *)(addr)) |\
				(*(((unsigned char *)(addr))+1) << 8) |\
				(*(((unsigned char *)(addr))+2) << 16) |\
				(*(((unsigned char *)(addr))+3) << 24) |\
				(*(((unsigned char *)(addr))+4) << 32) |\
				(*(((unsigned char *)(addr))+5) << 40) |\
				(*(((unsigned char *)(addr))+6) << 48) |\
				(*(((unsigned char *)(addr))+7) << 56))

#define STORESADDR(value,addr)	((*((unsigned char *)addr)) = ((*value) & 255 ,\
				 (*((unsigned char *)addr+1)) = ((*value) >> 8) & 255))

#define GETSADDR(addr)		((*(unsigned char *)(addr)) |\
				(*(((unsigned char *)(addr))+1) << 8))
*/	

//--MAC frame control field (leftmost bit numbered 0)---
//types (3 bits) -- we reverse the bit order for convenient operation
#define defFrmCtrl_Type_Beacon		0x00
#define defFrmCtrl_Type_Data		0x04
#define defFrmCtrl_Type_Ack		0x02
#define defFrmCtrl_Type_MacCmd		0x06
//dest/src addressing mode (2 bits) -- we reverse the bit order for convenient operation
#define defFrmCtrl_AddrModeNone		0x00
#define defFrmCtrl_AddrMode16		0x01
#define defFrmCtrl_AddrMode64		0x03

struct FrameCtrl
{
	UINT_16 FrmCtrl;		//(PSDU/MPDU) Frame Control (Figure 35)
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
	UINT_8 frmType;
	bool secu;
	bool frmPending;
	bool ackReq;
	bool intraPan;
	UINT_8 dstAddrMode;
	UINT_8 srcAddrMode;
	
	void parse(void)
	{
		frmType = (FrmCtrl & 0xe000) >> 13;
		secu = ((FrmCtrl & 0x1000) == 0)?false:true;
		frmPending = ((FrmCtrl & 0x0800) == 0)?false:true;
		ackReq = ((FrmCtrl & 0x0400) == 0)?false:true;
		intraPan = ((FrmCtrl & 0x0200) == 0)?false:true;
		dstAddrMode = (FrmCtrl & 0x0030) >> 4;
		srcAddrMode = (FrmCtrl & 0x0003);
	}

	void setFrmType(UINT_8 frmtype)
	{
		frmType = frmtype;
		FrmCtrl = (FrmCtrl & 0x1fff) + (frmtype << 13);
	}
	void setSecu(bool sc)
	{
		secu = sc;
		FrmCtrl = (FrmCtrl & 0xefff);
		if (sc) FrmCtrl += 0x1000;
	}
	void setFrmPending(bool pending)
	{
		frmPending = pending;
		FrmCtrl = (FrmCtrl & 0xf7ff);
		if (pending) FrmCtrl += 0x0800;
	}
	void setAckReq(bool ack)
	{
		ackReq = ack;
		FrmCtrl = (FrmCtrl & 0xfbff);
		if (ack) FrmCtrl += 0x0400;
	}
	void setIntraPan(bool intrapan)
	{
		intraPan = intrapan;
		FrmCtrl = (FrmCtrl & 0xfdff);
		if (intrapan) FrmCtrl += 0x0200;
	}
	void setDstAddrMode(UINT_8 dstmode)
	{
		dstAddrMode = dstmode;
		FrmCtrl = (FrmCtrl & 0xffcf) + (dstmode << 4);
	}
	void setSrcAddrMode(UINT_8 srcmode)
	{
		srcAddrMode = srcmode;
		FrmCtrl = (FrmCtrl & 0xfffc) + srcmode;
	}
};

struct SuperframeSpec			//refer to Figures 40,59
{
	UINT_16 SuperSpec;		//(MSDU) Superframe Specification (Figure 40)
					// --(0123):	Beacon order
					// --(4567):	Superframe order
					// --(89ab):	Final CAP slot
					// --(c):	Battery life extension
					// --(d):	Reserved
					// --(e):	PAN Coordinator
					// --(f):	Association permit
	UINT_8 BO;
	UINT_32 BI;
	UINT_8 SO;
	UINT_32 SD;
	UINT_32 sd;
	UINT_8 FinCAP;
	bool BLE;
	bool PANCoor;
	bool AssoPmt;

	void parse(void)
	{
		BO = (SuperSpec & 0xf000) >> 12;
		BI = aBaseSuperframeDuration * (1 << BO);
		SO = (SuperSpec & 0x0f00) >> 8;
		SD = aBaseSuperframeDuration * (1 << SO);	//superframe duration
		sd = aBaseSlotDuration * (1 << SO);		//slot duration
		FinCAP = (SuperSpec & 0x00f0) >> 4;
		BLE = ((SuperSpec & 0x0008) == 0)?false:true;
		PANCoor = ((SuperSpec & 0x0002) == 0)?false:true;
		AssoPmt = ((SuperSpec & 0x0001) == 0)?false:true;
	}

	void setBO(UINT_8 bo)
	{
		BO = bo;
		BI = aBaseSuperframeDuration * (1 << BO);
		SuperSpec = (SuperSpec & 0x0fff) + (bo << 12);
	}
	void setSO(UINT_8 so)
	{
		SO = so;
		SD = aBaseSuperframeDuration * (1 << SO);
		sd = aBaseSlotDuration * (1 << SO);
		SuperSpec = (SuperSpec & 0xf0ff) + (so << 8);
	}
	void setFinCAP(UINT_8 fincap)
	{
		FinCAP = fincap;
		SuperSpec = (SuperSpec & 0xff0f) + (fincap << 4);
	}
	void setBLE(bool ble)
	{
		BLE = ble;
		SuperSpec = (SuperSpec & 0xfff7);
		if (ble) SuperSpec += 8;
	}
	void setPANCoor(bool pancoor)
	{
		PANCoor = pancoor;
		SuperSpec = (SuperSpec & 0xfffd);
		if (pancoor) SuperSpec += 2;
	}
	void setAssoPmt(bool assopmt)
	{
		AssoPmt = assopmt;
		SuperSpec = (SuperSpec & 0xfffe);
		if (assopmt) SuperSpec += 1;
	}
	
};

struct GTSDescriptor
{
	UINT_16 devAddr16;
	UINT_8 slotSpec;	// --(0123):	GTS starting slot
				// --(4567):	GTS length
};

struct GTSFields
{
	UINT_8 spec;		//GTS specification
				// --(012):	GTS descriptor count
				// --(3456):	reserved
				// --(7):	GTS permit
	UINT_8 dir;		//GTS directions
				// --(0123456):	for up to 7 descriptors:
				//		1 = receive only (w.r.t. data transmission by the device)
				//		0 = transmit only (w.r.t. data transmission by the device)
				// --(7):	reserved
	GTSDescriptor list[7];	//GTS descriptor list
};

struct GTSSpec
{
	GTSFields fields;

	UINT_8 count;		//GTS descriptor count
	bool permit;		//GTS permit
	bool recvOnly[7];	//reception only
	UINT_8 slotStart[7];	//starting slot
	UINT_8 length[7];	//length in slots

	void parse(void)
	{
		int i;
		count = (fields.spec & 0xe0) >> 5;
		permit = ((fields.spec & 0x01) != 0)?true:false;
		for (i=0;i<count;i++)
		{
			recvOnly[i] = ((fields.dir & (1<<(7-i))) != 0);
			slotStart[i] = (fields.list[i].slotSpec & 0xf0) >> 4;
			length[i] = (fields.list[i].slotSpec & 0x0f);
		}
	}

	void setCount(UINT_8 cnt)
	{
		count = cnt;
		fields.spec = (fields.spec & 0x1f) + (cnt << 5);
	}
	void setPermit(bool pmt)
	{
		permit = pmt;
		fields.spec = (fields.spec & 0xfe);
		if (pmt) fields.spec += 1;
	}
	void setRecvOnly(UINT_8 ith,bool rvonly)
	{
		recvOnly[ith] = rvonly;
		fields.dir = fields.dir & ((1<<(7-ith))^0xff);
		if (rvonly) fields.dir += (1<<(7-ith));
	}
	void setSlotStart(UINT_8 ith,UINT_8 st)
	{
		slotStart[ith] = st;
		slotStart[ith] = (fields.list[ith].slotSpec & 0x0f) + (st << 4);
	}
	void setLength(UINT_8 ith,UINT_8 len)
	{
		length[ith] = len;
		length[ith] = (fields.list[ith].slotSpec & 0xf0) + len;
	}
	int size(void)
	{
		count = (fields.spec & 0xe0) >> 5;
		return (1 + 1 + 3 * count);
	}
};

struct PendAddrFields
{
	UINT_8 spec;		//Pending address specification field (refer to Figure 44)
				// --(012):	num of short addresses pending
				// --(3):	reserved
				// --(456):	num of extended addresses pending
				// --(7):	reserved
	IE3ADDR addrList[7];	//pending address list (shared by short/extended addresses)
};

struct PendAddrSpec
{
	PendAddrFields fields;

	UINT_8 numShortAddr;	//num of short addresses pending
	UINT_8 numExtendedAddr;	//num of extended addresses pending

	//for constructing the fields
	UINT_8 addShortAddr(UINT_16 sa)
	{
		int i;

		if (numShortAddr + numExtendedAddr >= 7)
			return (numShortAddr + numExtendedAddr);
		//only unique address added
		for (i=0;i<numShortAddr;i++)
		if (fields.addrList[i] == sa)
			return (numShortAddr + numExtendedAddr);
		fields.addrList[numShortAddr] = sa;
		numShortAddr++;
		return (numShortAddr + numExtendedAddr);
	}
	UINT_8 addExtendedAddr(IE3ADDR ea)
	{
		int i;

		if (numShortAddr + numExtendedAddr >= 7)
			return (numShortAddr + numExtendedAddr);
		//only unique address added
		for (i=6;i>6-numExtendedAddr;i--)
		if (fields.addrList[i] == ea)
			return (numShortAddr + numExtendedAddr);
		//save the extended address in reverse order
		fields.addrList[6-numExtendedAddr] = ea;
		numExtendedAddr++;
		return (numShortAddr + numExtendedAddr);
	}
	void format(void)
	{
		//realign the addresses
		int i;
		IE3ADDR tmpAddr;
		//restore the order of extended addresses
		for (i=0;i<numExtendedAddr;i++)
		{
			if ((7 - numExtendedAddr + i) < (6 - i))
			{
				tmpAddr = fields.addrList[7 - numExtendedAddr + i];
				fields.addrList[7 - numExtendedAddr + i] = fields.addrList[6 - i];
				fields.addrList[6 - i] = tmpAddr;
			}
		}
		//attach the extended addresses to short addresses
		for (i=0;i<numExtendedAddr;i++)
			fields.addrList[numShortAddr + i] = fields.addrList[7 - numExtendedAddr + i];
		//update address specification
		fields.spec = ((numShortAddr) << 5) + (numExtendedAddr << 1);
	}
	//for parsing the received fields
	void parse(void)
	{
		numShortAddr = (fields.spec & 0xe0) >> 5;
		numExtendedAddr = (fields.spec & 0x0e) >> 1;
	}

	int size(void)
	{
		parse();
		return (1 + numShortAddr * 2 + numExtendedAddr * 8);
	}
};

struct DevCapability
{
	UINT_8 cap;		//refer to Figure 49
				// --(0):	alternate PAN coordinator
				// --(1):	device type (1=FFD,0=RFD)
				// --(2):	power source (1=mains powered,0=non mains powered)
				// --(3):	receiver on when idle
				// --(45):	reserved
				// --(6):	security capability
				// --(7):	allocate address (asking for allocation of a short address during association)

	bool alterPANCoor;
	bool FFD;
	bool mainsPower;
	bool recvOnWhenIdle;
	bool secuCapable;
	bool alloShortAddr;

	void parse(void)
	{
		alterPANCoor = ((cap & 0x80) != 0)?true:false;
		FFD = ((cap & 0x40) != 0)?true:false;
		mainsPower = ((cap & 0x20) != 0)?true:false;
		recvOnWhenIdle = ((cap & 0x10) != 0)?true:false;
		secuCapable = ((cap & 0x02) != 0)?true:false;
		alloShortAddr = ((cap & 0x01) != 0)?true:false;
	}

	void setAlterPANCoor(bool alterPC)
	{
		alterPANCoor = alterPC;
		cap = (cap & 0x7f);
		if (alterPC) cap += 0x80;
	}
	void setFFD(bool ffd)
	{
		FFD = ffd;
		cap = (cap & 0xbf);
		if (ffd) cap += 0x40;
	}
	void setMainPower(bool mp)
	{
		mainsPower = mp;
		cap = (cap & 0xdf);
		if (mp) cap += 0x20;
	}
	void setRecvOnWhenIdle(bool onidle)
	{
		recvOnWhenIdle = onidle;
		cap = (cap & 0xef);
		if (onidle) cap += 0x10;
	}
	void setSecuCapable(bool sc)
	{
		secuCapable = sc;
		cap = (cap & 0xfd);
		if (sc) cap += 0x02;
	}
	void setAlloShortAddr(bool alloc)
	{
		alloShortAddr = alloc;
		cap = (cap & 0xfe);
		if (alloc) cap += 0x01;
	}
};

#endif

// End of file: p802_15_4field.h
