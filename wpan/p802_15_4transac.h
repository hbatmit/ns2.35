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

// File:  p802_15_4transac.h
// Mode:  C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t

// $Header: /cvsroot/nsnam/ns-2/wpan/p802_15_4transac.h,v 1.2 2011/06/22 04:03:26 tom_henderson Exp $

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


#ifndef p802_15_4transac_h
#define p802_15_4transac_h

#include <packet.h>
#include "p802_15_4def.h"
#include "p802_15_4const.h"
#include "p802_15_4field.h"

#define tr_oper_del	1
#define tr_oper_est	2
#define tr_oper_EST	3

//----------------------------------------------------------------------------------------

class DEVICELINK
{
public:
	IE3ADDR addr64;		//extended address of the associated device
	UINT_16 addr16;		//assigned short address
	UINT_8 capability;	//device capability
	
	DEVICELINK *last;
	DEVICELINK *next;
	DEVICELINK(IE3ADDR addr, UINT_8 cap)
	{
		addr64 = addr;
		addr16 = (UINT_16)addr;		
		capability = cap;
		last = NULL;
		next = NULL;
	};
};

int addDeviceLink(DEVICELINK **deviceLink1, DEVICELINK **deviceLink2, IE3ADDR addr, UINT_8 cap);
int updateDeviceLink(int oper, DEVICELINK **deviceLink1, DEVICELINK **deviceLink2, IE3ADDR addr);
int numberDeviceLink(DEVICELINK **deviceLink1);
int chkAddDeviceLink(DEVICELINK **deviceLink1, DEVICELINK **deviceLink2, IE3ADDR addr, UINT_8 cap);
void emptyDeviceLink(DEVICELINK **deviceLink1, DEVICELINK **deviceLink2);
void dumpDeviceLink(DEVICELINK *deviceLink1, IE3ADDR coorAddr);

//----------------------------------------------------------------------------------------

#define maxNumTransactions	70

class TRANSACLINK
{
public:
	UINT_8 pendAddrMode;
	union
	{
		UINT_16 pendAddr16;
		IE3ADDR pendAddr64;
	};
	Packet *pkt;
	UINT_8 msduHandle;
	double expTime;
	TRANSACLINK *last;
	TRANSACLINK *next;
	TRANSACLINK(UINT_8 pendAM, IE3ADDR pendAddr, Packet *p, UINT_8 msduH, double kpTime)
	{
		pendAddrMode = pendAM;
		pendAddr64 = pendAddr;
		pkt = p;
		msduHandle = msduH;
		expTime = CURRENT_TIME + kpTime;
		last = NULL;
		next = NULL;
	};
};

void purgeTransacLink(TRANSACLINK **transacLink1, TRANSACLINK **transacLink2);
int addTransacLink(TRANSACLINK **transacLink1, TRANSACLINK **transacLink2, UINT_8 pendAM, IE3ADDR pendAddr, Packet *p, UINT_8 msduH, double kpTime);
Packet *getPktFrTransacLink(TRANSACLINK **transacLink1, UINT_8 pendAM, IE3ADDR pendAddr);
int updateTransacLink(int oper, TRANSACLINK **transacLink1, TRANSACLINK **transacLink2, UINT_8 pendAM, IE3ADDR pendAddr);
int updateTransacLinkByPktOrHandle(int oper, TRANSACLINK **transacLink1, TRANSACLINK **transacLink2, Packet *pkt, UINT_8 msduH = 0);
int numberTransacLink(TRANSACLINK **transacLink1, TRANSACLINK **transacLink2);
int chkAddTransacLink(TRANSACLINK **transacLink1, TRANSACLINK **transacLink2, UINT_8 pendAM, IE3ADDR pendAddr, Packet *p, UINT_8 msduH, double kpTime);
void emptyTransacLink(TRANSACLINK **transacLink1, TRANSACLINK **transacLink2);
void dumpTransacLink(TRANSACLINK *transacLink1, IE3ADDR coorAddr);

#endif

// End of file: p802_15_4transac.h
