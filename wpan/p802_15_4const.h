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

// File:  p802_15_4const.h
// Mode:  C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t

// $Header: /cvsroot/nsnam/ns-2/wpan/p802_15_4const.h,v 1.3 2011/06/22 04:03:26 tom_henderson Exp $

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


#ifndef p802_15_4const_h
#define p802_15_4const_h

#include "p802_15_4def.h"

//---PHY layer constants (Table 18)---
const UINT_8 aMaxPHYPacketSize	= 	127;		//max PSDU size (in bytes) the PHY shall be able to receive
const UINT_8 aTurnaroundTime		= 12;		//Rx-to-Tx or Tx-to-Rx max turnaround time (in symbol period)
const UINT_8 aCCATime		= 8; 			//2.31 change: CCA duration (in symbol period) 

//---PHY_PIB default values---
//All the default values are not given in the draft.
//They are chosen for sake of a value
#define def_phyCurrentChannel			11
#define def_phyChannelsSupported		0x07ffffff
#define def_phyTransmitPower			0
#define def_phyCCAMode				1

//---MAC sublayer constants (Table 70)---
const UINT_8 aNumSuperframeSlots			= 16;		//# of slots contained in a superframe
const UINT_8 aBaseSlotDuration				= 60;		//# of symbols comprising a superframe slot of order 0
const UINT_16 aBaseSuperframeDuration
		= aBaseSlotDuration * aNumSuperframeSlots;		//# of symbols comprising a superframe of order 0
//aExtendedAddress					= ;		//64-bit (IEEE) address assigned to the device (device specific)
const UINT_8 aMaxBE					= 5;		//max value of the backoff exponent in the CSMA-CA algorithm
const UINT_8 aMaxBeaconOverhead				= 75;		//max # of octets added by the MAC sublayer to the payload of its beacon frame
const UINT_8 aMaxBeaconPayloadLength
		= aMaxPHYPacketSize - aMaxBeaconOverhead;		//max size, in octets, of a beacon payload
const UINT_8 aGTSDescPersistenceTime			= 4;		//# of superframes that a GTS descriptor exists in the beacon frame of a PAN coordinator
const UINT_8 aMaxFrameOverhead				= 25;		//max # of octets added by the MAC sublayer to its payload w/o security.
const UINT_16 aMaxFrameResponseTime			= 1220;		//max # of symbols (or CAP symbols) to wait for a response frame
const UINT_8 aMaxFrameRetries				= 3;		//max # of retries allowed after a transmission failures
const UINT_8 aMaxLostBeacons				= 4;		//max # of consecutive beacons the MAC sublayer can miss w/o declaring a loss of synchronization
const UINT_8 aMaxMACFrameSize
	= aMaxPHYPacketSize - aMaxFrameOverhead;			//max # of octets that can be transmitted in the MAC frame payload field
const UINT_8 aMaxSIFSFrameSize				= 18;		//max size of a frame, in octets, that can be followed by a SIFS period
const UINT_16 aMinCAPLength				= 440;		//min # of symbols comprising the CAP
const UINT_8 aMinLIFSPeriod				= 40;		//min # of symbols comprising a LIFS period
const UINT_8 aMinSIFSPeriod				= 12;		//min # of symbols comprising a SIFS period
const UINT_16 aResponseWaitTime
		= 32 * aBaseSuperframeDuration;				//max # of symbols a device shall wait for a response command following a request command
const UINT_8 aUnitBackoffPeriod				= 20;		//# of symbols comprising the basic time period used by the CSMA-CA algorithm

//---MAC_PIB default values (Tables 71,72)---
//attributes from Table 71
#define def_macAckWaitDuration			54			//22(ack) + 20(backoff slot) + 12(turnaround); propagation delay ignored?
#define def_macAssociationPermit		false
#define def_macAutoRequest			true
#define def_macBattLifeExt			false
#define def_macBattLifeExtPeriods		6
#define def_macBeaconPayload			""
#define def_macBeaconPayloadLength		0
#define def_macBeaconOrder			15
#define def_macBeaconTxTime			0x000000
//#define def_macBSN				Random::random() % 0x100
//#define def_macCoordExtendedAddress		0xffffffffffffffffLL
#define def_macCoordExtendedAddress		0xffff			
									//not defined in draft
#define def_macCoordShortAddress		0xffff
//#define def_macDSN				Random::random() % 0x100
#define def_macGTSPermit			true
#define def_macMaxCSMABackoffs			4
#define def_macMinBE				3
#define def_macPANId				0xffff
#define def_macPromiscuousMode			false
#define def_macRxOnWhenIdle			false
#define def_macShortAddress			0xffff
#define def_macSuperframeOrder			15
#define def_macTransactionPersistenceTime	0x01f4
//attributes from Table 72 (security attributes)
#define def_macACLEntryDescriptorSet		NULL
#define def_macACLEntryDescriptorSetSize	0x00
#define def_macDefaultSecurity			false
#define def_macACLDefaultSecurityMaterialLength	0x15
#define def_macDefaultSecurityMaterial		NULL
#define def_macDefaultSecuritySuite		0x00
#define def_macSecurityMode			0x00

#endif

// End of file: p802_15_4const.h
