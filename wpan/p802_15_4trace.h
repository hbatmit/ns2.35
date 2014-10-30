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

// File:  p802_15_4trace.h
// Mode:  C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t

// $Header: /cvsroot/nsnam/ns-2/wpan/p802_15_4trace.h,v 1.2 2011/06/22 04:03:26 tom_henderson Exp $

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


#ifndef p802_15_4trace_h
#define p802_15_4trace_h

#include <packet.h>

//#define MAC_Subtype_RTS		0x0B
//#define MAC_Subtype_CTS		0x0C
//#define MAC_Subtype_ACK		0x0D
//#define MAC_Subtype_Data		0x00
#define MAC_Subtype_Beacon		0x0f
#define MAC_Subtype_Command_AssoReq	0x01
#define MAC_Subtype_Command_AssoRsp	0x02
#define MAC_Subtype_Command_DAssNtf	0x03
#define MAC_Subtype_Command_DataReq	0x04
#define MAC_Subtype_Command_PIDCNtf	0x05
#define MAC_Subtype_Command_OrphNtf	0x06
#define MAC_Subtype_Command_BconReq	0x07
#define MAC_Subtype_Command_CoorRea	0x08
#define MAC_Subtype_Command_GTSReq	0x09

int p802_15_4macSA(Packet *p);
int p802_15_4macDA(Packet *p);
u_int16_t p802_15_4hdr_type(Packet *p);
int p802_15_4hdr_dst(char* hdr,int dst = -2);
int p802_15_4hdr_src(char* hdr,int src = -2);
int p802_15_4hdr_type(char* hdr, u_int16_t type = 0);
void p802_15_4hdrDATA(Packet *p);
void p802_15_4hdrACK(Packet *p);
void p802_15_4hdrBeacon(Packet *p);
void p802_15_4hdrCommand(Packet *p, u_int16_t type);
const char *wpan_pName(Packet *p);

#endif

// End of file: p802_15_4trace.h
