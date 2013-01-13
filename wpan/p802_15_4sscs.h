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

// File:  p802_15_4sscs.h
// Mode:  C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t

// $Header: /cvsroot/nsnam/ns-2/wpan/p802_15_4sscs.h,v 1.3 2011/06/22 04:03:26 tom_henderson Exp $

// Functions in this file are out of the scope of 802.15.4.
// But they elaborate how a higher layer interfaces with 802.15.4.
// You can modify this file at will to fit your need.

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


#ifndef p802_15_4sscs_h
#define p802_15_4sscs_h

//enable/disable ZigBee interface
//#define ZigBeeIF

#include "p802_15_4def.h"
#include "p802_15_4mac.h"


//task pending (callback)
#define sscsTP_startPANCoord		1
#define sscsTP_startDevice		2
struct sscsTaskPending
{
	sscsTaskPending()
	{
		init();
	}
	inline void init()
	{
		startPANCoord = false;
		startPANCoord_STEP = 0;
		startDevice = false;
		startDevice_STEP = 0;
	}

	bool &taskStatus(UINT_8 task)
	{
		switch (task)
		{
			case sscsTP_startPANCoord:
				return startPANCoord;
			case sscsTP_startDevice:
				return startDevice;
			default:
				assert(0);
				// shutup compiler.
				return startDevice;
		}
	}

	UINT_8 &taskStep(UINT_8 task)
	{
		switch (task)
		{
			case sscsTP_startPANCoord:
				return startPANCoord_STEP;
			case sscsTP_startDevice:
				return startDevice_STEP;
			default:
				assert(0);
				// shutup compiler.
				return startDevice_STEP;
		}
	}

	//----------------
	bool	startPANCoord;
	bool	startPANCoord_isCluster_Tree;
	UINT_8	startPANCoord_STEP;
	bool	startPANCoord_txBeacon;
	UINT_8	startPANCoord_BO;
	UINT_8	startPANCoord_SO;
	UINT_8	startPANCoord_Channel;
	//----------------
	bool	startDevice;
	bool	startDevice_isCluster_Tree;
	UINT_8	startDevice_STEP;
	bool	startDevice_isFFD;
	bool	startDevice_assoPermit;
	bool	startDevice_txBeacon;
	UINT_8	startDevice_BO;
	UINT_8	startDevice_SO;
	UINT_8	startDevice_Channel;
	PAN_ELE	startDevice_panDes;
	//----------------
};

class SSCS802_15_4;
class SSCS802_15_4Timer : public Handler
{
	friend class SSCS802_15_4;
public:
	SSCS802_15_4Timer(SSCS802_15_4 *s) : Handler()
	{
		sscs = s;
		active = false;
	}
	virtual void start(double wtime);
	virtual void cancel(void);
	virtual void handle(Event* e);

protected:
	SSCS802_15_4 *sscs;
	bool active;
	Event nullEvent;
};

#ifdef ZigBeeIF
class ZBR;
#endif
class SSCS802_15_4
{
	friend class Mac802_15_4;
	friend class SSCS802_15_4Timer;
public:
	SSCS802_15_4(Mac802_15_4 *m);
	~SSCS802_15_4();
	void MCPS_DATA_confirm(UINT_8 msduHandle,MACenum status);
	void MCPS_DATA_indication(UINT_8 SrcAddrMode,UINT_16 SrcPANId,IE3ADDR SrcAddr,
				  UINT_8 DstAddrMode,UINT_16 DstPANId,IE3ADDR DstAddr,
				  UINT_8 msduLength,Packet *msdu,UINT_8 mpduLinkQuality,
				  bool SecurityUse,UINT_8 ACLEntry);
	void MCPS_PURGE_confirm(UINT_8 msduHandle,MACenum status);
	void MLME_ASSOCIATE_indication(IE3ADDR DeviceAddress,UINT_8 CapabilityInformation,bool SecurityUse,UINT_8 ACLEntry);
	void MLME_ASSOCIATE_confirm(UINT_16 AssocShortAddress,MACenum status);
	void MLME_DISASSOCIATE_confirm(MACenum status);
	void MLME_BEACON_NOTIFY_indication(UINT_8 BSN,PAN_ELE *PANDescriptor,UINT_8 PendAddrSpec,IE3ADDR *AddrList,UINT_8 sduLength,UINT_8 *sdu);
	void MLME_GET_confirm(MACenum status,MPIBAenum PIBAttribute,MAC_PIB *PIBAttributeValue);
	void MLME_ORPHAN_indication(IE3ADDR OrphanAddress,bool SecurityUse,UINT_8 ACLEntry);
	void MLME_RESET_confirm(MACenum status);
	void MLME_RX_ENABLE_confirm(MACenum status);
	void MLME_SET_confirm(MACenum status,MPIBAenum PIBAttribute);
	void MLME_SCAN_confirm(MACenum status,UINT_8 ScanType,UINT_32 UnscannedChannels,
			       UINT_8 ResultListSize,UINT_8 *EnergyDetectList,
			       PAN_ELE *PANDescriptorList);
	void MLME_COMM_STATUS_indication(UINT_16 PANId,UINT_8 SrcAddrMode,IE3ADDR SrcAddr,
					 UINT_8 DstAddrMode,IE3ADDR DstAddr,MACenum status);
	void MLME_START_confirm(MACenum status);
	void MLME_SYNC_LOSS_indication(MACenum LossReason);
	void MLME_POLL_confirm(MACenum status);

protected:
	void checkTaskOverflow(UINT_8 task);
	void dispatch(MACenum status,char *frFunc);
	void startPANCoord(bool isClusterTree,bool txBeacon,UINT_8 BO,UINT_8 SO,bool firsttime,MACenum status = m_SUCCESS);
	void startDevice(bool isClusterTree,bool isFFD,bool assoPermit,bool txBeacon,UINT_8 BO,UINT_8 SO,bool firsttime,MACenum status = m_SUCCESS);
	int command(int argc, const char*const* argv);

#ifdef ZigBeeIF
	//for cluster tree
	void assertZBR(void);
	int RNType(void);
	void setGetClusTreePara(char setGet,Packet *p);
#endif

protected:
	bool t_isCT,t_txBeacon,t_isFFD,t_assoPermit;
	UINT_8 t_BO,t_SO;
	//for cluster tree
	UINT_16 rt_myDepth;
	UINT_16 rt_myNodeID;
	UINT_16 rt_myParentNodeID;

public:
	static UINT_32 ScanChannels;
	bool neverAsso;			

private:
	Mac802_15_4 *mac;
#ifdef ZigBeeIF
	ZBR *zbr;			
#endif
	SSCS802_15_4Timer assoH;
	sscsTaskPending sscsTaskP;

	//--- store results returned from MLME_SCAN_confirm() ---
	UINT_32	T_UnscannedChannels;
	UINT_8	T_ResultListSize;
	UINT_8	*T_EnergyDetectList;
	PAN_ELE	*T_PANDescriptorList;
	UINT_8	Channel;
	//-------------------------------------------------------

	HLISTLINK *hlistLink1;
	HLISTLINK *hlistLink2;
};

#endif

// End of file: p802_15_4sscs.h
