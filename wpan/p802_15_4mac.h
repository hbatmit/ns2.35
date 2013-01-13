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

// File:  p802_15_4mac.h
// Mode:  C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t

// $Header: /cvsroot/nsnam/ns-2/wpan/p802_15_4mac.h,v 1.4 2011/06/22 04:03:26 tom_henderson Exp $

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


#ifndef p802_15_4mac_h
#define p802_15_4mac_h

#include "p802_15_4pkt.h"
#include "p802_15_4phy.h"
#include "p802_15_4timer.h"
#include "p802_15_4hlist.h"
#include "p802_15_4transac.h"

//Elements of PAN descriptor (Table 41)
struct PAN_ELE
{
	UINT_8	CoordAddrMode;
	UINT_16	CoordPANId;
	union
	{
		UINT_16 CoordAddress_16;
		IE3ADDR CoordAddress_64;
	};
	UINT_8	LogicalChannel;
	UINT_16	SuperframeSpec;
	bool	GTSPermit;
	UINT_8	LinkQuality;
	UINT_32	TimeStamp:24;
	bool	SecurityUse;
	UINT_8	ACLEntry;
	bool	SecurityFailure;
	//add one field for cluster tree
	UINT_16	clusTreeDepth;
};

//task pending (callback)
#define TP_mcps_data_request		1
#define TP_mlme_associate_request	2
#define TP_mlme_associate_response	3
#define TP_mlme_disassociate_request	4
#define TP_mlme_orphan_response		5
#define TP_mlme_reset_request		6
#define TP_mlme_rx_enable_request	7
#define TP_mlme_scan_request		8
#define TP_mlme_start_request		9
#define TP_mlme_sync_request		10
#define TP_mlme_poll_request		11
#define TP_CCA_csmaca			12
#define TP_RX_ON_csmaca			13
struct taskPending
{
	taskPending()
	{
		init();
	}
	inline void init()
	{
		mcps_data_request = false;
		mcps_data_request_STEP = 0;
		mlme_associate_request = false;
		mlme_associate_request_STEP = 0;
		mlme_associate_response = false;
		mlme_associate_response_STEP = 0;
		mlme_disassociate_request = false;
		mlme_disassociate_request_STEP = 0;
		mlme_orphan_response = false;
		mlme_orphan_response_STEP = 0;
		mlme_reset_request = false;
		mlme_reset_request_STEP = 0;
		mlme_rx_enable_request = false;
		mlme_rx_enable_request_STEP = 0;
		mlme_scan_request = false;
		mlme_scan_request_STEP = 0;
		mlme_start_request = false;
		mlme_start_request_STEP = 0;
		mlme_sync_request = false;
		mlme_sync_request_STEP = 0;
		mlme_sync_request_tracking = false;
		mlme_poll_request = false;
		mlme_poll_request_STEP = 0;
		CCA_csmaca = false;
		CCA_csmaca_STEP = 0;
		RX_ON_csmaca = false;
		RX_ON_csmaca_STEP = 0;
	}

	bool &taskStatus(UINT_8 task)
	{
		switch (task)
		{
			case TP_mcps_data_request:
				return mcps_data_request;
			case TP_mlme_associate_request:
				return mlme_associate_request;
			case TP_mlme_associate_response:
				return mlme_associate_response;
			case TP_mlme_disassociate_request:
				return mlme_disassociate_request;
			case TP_mlme_orphan_response:
				return mlme_orphan_response;
			case TP_mlme_reset_request:
				return mlme_reset_request;
			case TP_mlme_rx_enable_request:
				return mlme_rx_enable_request;
			case TP_mlme_scan_request:
				return mlme_scan_request;
			case TP_mlme_start_request:
				return mlme_start_request;
			case TP_mlme_sync_request:
				return mlme_sync_request;
			case TP_mlme_poll_request:
				return mlme_poll_request;
			case TP_CCA_csmaca:
				return CCA_csmaca;
			case TP_RX_ON_csmaca:
				return RX_ON_csmaca;
			default:
				assert(0);
				// shutup the compiler.
				return RX_ON_csmaca;
		}
	}

	UINT_8 &taskStep(UINT_8 task)
	{
		switch (task)
		{
			case TP_mcps_data_request:
				return mcps_data_request_STEP;
			case TP_mlme_associate_request:
				return mlme_associate_request_STEP;
			case TP_mlme_associate_response:
				return mlme_associate_response_STEP;
			case TP_mlme_disassociate_request:
				return mlme_disassociate_request_STEP;
			case TP_mlme_orphan_response:
				return mlme_orphan_response_STEP;
			case TP_mlme_reset_request:
				return mlme_reset_request_STEP;
			case TP_mlme_rx_enable_request:
				return mlme_rx_enable_request_STEP;
			case TP_mlme_scan_request:
				return mlme_scan_request_STEP;
			case TP_mlme_start_request:
				return mlme_start_request_STEP;
			case TP_mlme_sync_request:
				return mlme_sync_request_STEP;
			case TP_mlme_poll_request:
				return mlme_poll_request_STEP;
			case TP_CCA_csmaca:
				return CCA_csmaca_STEP;
			case TP_RX_ON_csmaca:
				return RX_ON_csmaca_STEP;
			default:
				assert(0);
				// shutup compiler.
				return RX_ON_csmaca_STEP;
		}
	}

	char *taskFrFunc(UINT_8 task)
	{
		switch (task)
		{
			case TP_mcps_data_request:
				return mcps_data_request_frFunc;
			case TP_mlme_associate_request:
				return mlme_associate_request_frFunc;
			case TP_mlme_associate_response:
				return mlme_associate_response_frFunc;
			case TP_mlme_disassociate_request:
				return mlme_disassociate_request_frFunc;
			case TP_mlme_orphan_response:
				return mlme_orphan_response_frFunc;
			case TP_mlme_reset_request:
				return mlme_reset_request_frFunc;
			case TP_mlme_rx_enable_request:
				return mlme_rx_enable_request_frFunc;
			case TP_mlme_scan_request:
				return mlme_scan_request_frFunc;
			case TP_mlme_start_request:
				return mlme_start_request_frFunc;
			case TP_mlme_sync_request:
				return mlme_sync_request_frFunc;
			case TP_mlme_poll_request:
				return mlme_poll_request_frFunc;
			default:
				assert(0);
				// shutup compiler.
				return mlme_poll_request_frFunc;
		}
	}

	//----------------
	bool	mcps_data_request;
	UINT_8	mcps_data_request_STEP;
	char	mcps_data_request_frFunc[81];
	UINT_8	mcps_data_request_TxOptions;
	Packet	*mcps_data_request_pendPkt;
	//----------------
	bool	mlme_associate_request;
	UINT_8	mlme_associate_request_STEP;
	char	mlme_associate_request_frFunc[81];
	bool	mlme_associate_request_SecurityEnable;
	UINT_8	mlme_associate_request_CoordAddrMode;
	Packet	*mlme_associate_request_pendPkt;
	//----------------
	bool	mlme_associate_response;
	UINT_8	mlme_associate_response_STEP;
	char	mlme_associate_response_frFunc[81];
	IE3ADDR	mlme_associate_response_DeviceAddress;
	Packet	*mlme_associate_response_pendPkt;
	//----------------
	bool	mlme_disassociate_request;
	UINT_8	mlme_disassociate_request_STEP;
	char	mlme_disassociate_request_frFunc[81];
	bool	mlme_disassociate_request_toCoor;
	Packet	*mlme_disassociate_request_pendPkt;
	//----------------
	bool	mlme_orphan_response;
	UINT_8	mlme_orphan_response_STEP;
	char	mlme_orphan_response_frFunc[81];
	IE3ADDR	mlme_orphan_response_OrphanAddress;
	//----------------
	bool	mlme_reset_request;
	UINT_8	mlme_reset_request_STEP;
	char	mlme_reset_request_frFunc[81];
	bool	mlme_reset_request_SetDefaultPIB;
	//----------------
	bool	mlme_rx_enable_request;
	UINT_8	mlme_rx_enable_request_STEP;
	char	mlme_rx_enable_request_frFunc[81];
	UINT_32	mlme_rx_enable_request_RxOnTime;
	UINT_32	mlme_rx_enable_request_RxOnDuration;
	double	mlme_rx_enable_request_currentTime;
	//----------------
	bool	mlme_scan_request;
	UINT_8	mlme_scan_request_STEP;
	char	mlme_scan_request_frFunc[81];
	UINT_8	mlme_scan_request_ScanType;
	UINT_8	mlme_scan_request_orig_macBeaconOrder;
	UINT_8	mlme_scan_request_orig_macBeaconOrder2;
	UINT_8	mlme_scan_request_orig_macBeaconOrder3;
	UINT_16	mlme_scan_request_orig_macPANId;
	UINT_32	mlme_scan_request_ScanChannels;
	UINT_8	mlme_scan_request_ScanDuration;
	UINT_8	mlme_scan_request_CurrentChannel;
	UINT_8	mlme_scan_request_ListNum;
	UINT_8	mlme_scan_request_EnergyDetectList[27];
	PAN_ELE	mlme_scan_request_PANDescriptorList[27];
	//----------------
	bool	mlme_start_request;
	UINT_8	mlme_start_request_STEP;
	char	mlme_start_request_frFunc[81];
	UINT_8	mlme_start_request_BeaconOrder;
	UINT_8	mlme_start_request_SuperframeOrder;
	bool	mlme_start_request_BatteryLifeExtension;
	bool	mlme_start_request_SecurityEnable;
	bool	mlme_start_request_PANCoordinator;
	UINT_16	mlme_start_request_PANId;
	UINT_8	mlme_start_request_LogicalChannel;
	//----------------
	bool	mlme_sync_request;
	UINT_8	mlme_sync_request_STEP;
	char	mlme_sync_request_frFunc[81];
	UINT_8	mlme_sync_request_numSearchRetry;
	bool	mlme_sync_request_tracking;
	//----------------
	bool	mlme_poll_request;
	UINT_8	mlme_poll_request_STEP;
	char	mlme_poll_request_frFunc[81];
	UINT_8	mlme_poll_request_CoordAddrMode;
	UINT_16	mlme_poll_request_CoordPANId;
	IE3ADDR	mlme_poll_request_CoordAddress;
	bool	mlme_poll_request_SecurityEnable;
	bool	mlme_poll_request_autoRequest;
	bool	mlme_poll_request_pending;
	//----------------
	bool	CCA_csmaca;
	UINT_8	CCA_csmaca_STEP;
	//----------------
	bool	RX_ON_csmaca;
	UINT_8	RX_ON_csmaca_STEP;
	//----------------
};

//Elements of ACL entry descriptor (Table 73)
struct MAC_ACL
{
	IE3ADDR	ACLExtendedAddress;
	UINT_16	ACLShortAddress;
	UINT_16	ACLPANId;
	UINT_8	ACLSecurityMaterialLength;
	UINT_8	*ACLSecurityMaterial;
	UINT_8	ACLSecuritySuite;
};

//MAC enumerations description (Table 64)
typedef enum
{
	m_SUCCESS = 0,
	//--- following from Table 68) ---
	m_PAN_at_capacity,
	m_PAN_access_denied,
	//--------------------------------
	m_BEACON_LOSS = 0xe0,
	m_CHANNEL_ACCESS_FAILURE,
	m_DENIED,
	m_DISABLE_TRX_FAILURE,
	m_FAILED_SECURITY_CHECK,
	m_FRAME_TOO_LONG,
	m_INVALID_GTS,
	m_INVALID_HANDLE,
	m_INVALID_PARAMETER,
	m_NO_ACK,
	m_NO_BEACON,
	m_NO_DATA,
	m_NO_SHORT_ADDRESS,
	m_OUT_OF_CAP,
	m_PAN_ID_CONFLICT,
	m_REALIGNMENT,
	m_TRANSACTION_EXPIRED,
	m_TRANSACTION_OVERFLOW,
	m_TX_ACTIVE,
	m_UNAVAILABLE_KEY,
	m_UNSUPPORTED_ATTRIBUTE,
	m_UNDEFINED			//we added this for handling any case not specified in the draft
}MACenum;

//MAC PIB attributes (Tables 71,72)
typedef enum {
	//attributes from Table 71
	macAckWaitDuration,
	macAssociationPermit,
	macAutoRequest,
	macBattLifeExt,
	macBattLifeExtPeriods,
	macBeaconPayload,
	macBeaconPayloadLength,
	macBeaconOrder,
	macBeaconTxTime,
	macBSN,
	macCoordExtendedAddress,
	macCoordShortAddress,
	macDSN,
	macGTSPermit,
	macMaxCSMABackoffs,
	macMinBE,
	macPANId,
	macPromiscuousMode,
	macRxOnWhenIdle,
	macShortAddress,
	macSuperframeOrder,
	macTransactionPersistenceTime,
	//attributes from Table 72 (security attributes)
	macACLEntryDescriptorSet,
	macACLEntryDescriptorSetSize,
	macDefaultSecurity,
	macACLDefaultSecurityMaterialLength,
	macDefaultSecurityMaterial,
	macDefaultSecuritySuite,
	macSecurityMode
}MPIBAenum;

struct MAC_PIB
{
	//attributes from Table 71
	UINT_8	macAckWaitDuration;
	bool	macAssociationPermit;
	bool	macAutoRequest;
	bool	macBattLifeExt;
	UINT_8	macBattLifeExtPeriods;
	/*
	UINT_8	macBeaconPayload[aMaxPHYPacketSize-(6+9+2+1)+1];	//beacon length in octets (w/o payload):
									//	max: 6(phy) + 15(mac) + 23 (GTSs) + 57 (pending addresses)
									//	min: 6(phy) + 9(mac) + 2 (GTSs) + 1 (pending addresses)
	*/
	UINT_8	macBeaconPayload[aMaxBeaconPayloadLength+1];
	UINT_8	macBeaconPayloadLength;
	UINT_8	macBeaconOrder;
	UINT_32	macBeaconTxTime:24;
	UINT_8	macBSN;
	IE3ADDR	macCoordExtendedAddress;
	UINT_16	macCoordShortAddress;
	UINT_8	macDSN;
	bool	macGTSPermit;
	UINT_8	macMaxCSMABackoffs;
	UINT_8	macMinBE;
	UINT_16	macPANId;
	bool	macPromiscuousMode;
	bool	macRxOnWhenIdle;
	UINT_16	macShortAddress;
	UINT_8	macSuperframeOrder;
	UINT_16	macTransactionPersistenceTime;
	//attributes from Table 72 (security attributes)
	MAC_ACL	*macACLEntryDescriptorSet;
	UINT_8	macACLEntryDescriptorSetSize;
	bool	macDefaultSecurity;
	UINT_8	macACLDefaultSecurityMaterialLength;
	UINT_8	*macDefaultSecurityMaterial;
	UINT_8	macDefaultSecuritySuite;
	UINT_8	macSecurityMode;
};

#define macTxBcnCmdDataHType	1
#define macIFSHType		2
#define macBackoffBoundType	3
class Mac802_15_4;
class Mac802_15_4Handler : public Handler
{
	friend class Mac802_15_4;
public:
	Mac802_15_4Handler(Mac802_15_4 *m, int tp) : Handler()
	{
		mac = m;
		type = tp;
		nullEvent.uid_ = 0;
	}
	virtual void handle(Event* e);

protected:
	Mac802_15_4 *mac;
	int type;
	Event nullEvent;
};

#define TxOp_Acked	0x01
#define TxOp_GTS	0x02
#define TxOp_Indirect	0x04
#define TxOp_SecEnabled	0x08
class Phy802_15_4;
class CsmaCA802_15_4;
class SSCS802_15_4;
class Nam802_15_4;
class Mac802_15_4 : public Mac
{
	friend class Phy802_15_4;
	friend class Mac802_15_4Handler;
	friend class macTxOverTimer;
	friend class macTxTimer;
	friend class macExtractTimer;
	friend class macAssoRspWaitTimer;
	friend class macDataWaitTimer;
	friend class macRxEnableTimer;
	friend class macScanTimer;
	friend class macBeaconTxTimer;
	friend class macBeaconRxTimer;
	friend class macBeaconSearchTimer;
	friend class macWakeupTimer; // 2.31 change: 
	friend class CsmaCA802_15_4;
	friend class SSCS802_15_4;
	friend class Nam802_15_4;
public:
	Mac802_15_4(MAC_PIB *mp);
	~Mac802_15_4();
	void init(bool reset = false);

	//interfaces between MAC and PHY
	void PD_DATA_confirm(PHYenum status);
	void PLME_CCA_confirm(PHYenum status);
	void PLME_ED_confirm(PHYenum status,UINT_8 EnergyLevel);
	void PLME_GET_confirm(PHYenum status,PPIBAenum PIBAttribute,PHY_PIB *PIBAttributeValue);
	void PLME_SET_TRX_STATE_confirm(PHYenum status);
	void PLME_SET_confirm(PHYenum status,PPIBAenum PIBAttribute);

	//interfaces between MAC and SSCS (or some other upper layer)
	void MCPS_DATA_request(UINT_8 SrcAddrMode,UINT_16 SrcPANId,IE3ADDR SrcAddr,
			       UINT_8 DstAddrMode,UINT_16 DstPANId,IE3ADDR DstAddr,
			       UINT_8 msduLength,Packet *msdu,UINT_8 msduHandle,UINT_8 TxOptions);
	void MCPS_DATA_indication(UINT_8 SrcAddrMode,UINT_16 SrcPANId,IE3ADDR SrcAddr,
				  UINT_8 DstAddrMode,UINT_16 DstPANId,IE3ADDR DstAddr,
				  UINT_8 msduLength,Packet *msdu,UINT_8 mpduLinkQuality,
				  bool SecurityUse,UINT_8 ACLEntry);
	void MCPS_PURGE_request(UINT_8 msduHandle);
	void MLME_ASSOCIATE_request(UINT_8 LogicalChannel,UINT_8 CoordAddrMode,UINT_16 CoordPANId,IE3ADDR CoordAddress,
				    UINT_8 CapabilityInformation,bool SecurityEnable);
	void MLME_ASSOCIATE_response(IE3ADDR DeviceAddress,UINT_16 AssocShortAddress,MACenum status,bool SecurityEnable);
	void MLME_DISASSOCIATE_request(IE3ADDR DeviceAddress,UINT_8 DisassociateReason,bool SecurityEnable);
	void MLME_DISASSOCIATE_indication(IE3ADDR DeviceAddress,UINT_8 DisassociateReason,bool SecurityUse,UINT_8 ACLEntry);
	void MLME_GET_request(MPIBAenum PIBAttribute);
/*TBD*/	void MLME_GTS_request(UINT_8 GTSCharacteristics,bool SecurityEnable);
/*TBD*/	void MLME_GTS_confirm(UINT_8 GTSCharacteristics,MACenum status);
/*TBD*/	void MLME_GTS_indication(UINT_16 DevAddress,UINT_8 GTSCharacteristics,
				 bool SecurityUse, UINT_8 ACLEntry);
	void MLME_ORPHAN_response(IE3ADDR OrphanAddress,UINT_16 ShortAddress,bool AssociatedMember,bool SecurityEnable);
	void MLME_RESET_request(bool SetDefaultPIB);
	void MLME_RX_ENABLE_request(bool DeferPermit,UINT_32 RxOnTime,UINT_32 RxOnDuration);
	void MLME_SCAN_request(UINT_8 ScanType,UINT_32 ScanChannels,UINT_8 ScanDuration);
	void MLME_SET_request(MPIBAenum PIBAttribute,MAC_PIB *PIBAttributeValue);
	void MLME_START_request(UINT_16 PANId,UINT_8 LogicalChannel,UINT_8 BeaconOrder,
				UINT_8 SuperframeOrder,bool PANCoordinator,bool BatteryLifeExtension,
				bool CoordRealignment,bool SecurityEnable);
	void MLME_SYNC_request(UINT_8 LogicalChannel, bool TrackBeacon);
	void MLME_POLL_request(UINT_8 CoordAddrMode,UINT_16 CoordPANId,IE3ADDR CoordAddress,bool SecurityEnable);

	inline int hdr_dst(char* hdr,int dst = -2);
	inline int hdr_src(char* hdr,int src = -2);
	inline int hdr_type(char* hdr, UINT_16 type = 0);

	inline Tap *tap(){return tap_;}
	void recv(Packet *p, Handler *h);
	void recvBeacon(Packet *p);
	void recvAck(Packet *p);
	void recvCommand(Packet *p);
	void recvData(Packet *p);
	bool toParent(Packet *p);

protected:
	void set_trx_state_request(PHYenum state,const char *frFile,const char *frFunc,int line);
	double locateBoundary(bool parent,double wtime);
	void txOverHandler(void);		//transmission over timer handler
	void txHandler(void);			//ack expiration timer handler
	void extractHandler(void);		//data extraction timer handler
	void assoRspWaitHandler(void);		//association response wait timer handler
	void dataWaitHandler(void);		//data wait timer handler (for indirect transmission)
	void rxEnableHandler(void);		//receiver enable timer handler
	void scanHandler(void);			//scan done for current channel
	void beaconTxHandler(bool forTX);	//periodic beacon transmission
	void beaconRxHandler(void);		//periodic beacon reception
	void beaconSearchHandler(void);		//beacon searching times out during synchronization
	void isPanCoor(bool isPC);

private:
	void checkTaskOverflow(UINT_8 task);
	void dispatch(PHYenum status,const char *frFunc,PHYenum req_state = p_SUCCESS,MACenum mStatus = m_SUCCESS);
	void sendDown(Packet *p,Handler* h);
	void mcps_data_request(UINT_8 SrcAddrMode,UINT_16 SrcPANId,IE3ADDR SrcAddr,
			       UINT_8 DstAddrMode,UINT_16 DstPANId,IE3ADDR DstAddr,
			       UINT_8 msduLength,Packet *msdu,UINT_8 msduHandle,UINT_8 TxOptions,
			       bool frUpper = false,PHYenum status = p_SUCCESS,MACenum mStatus = m_SUCCESS);
	void mlme_associate_request(UINT_8 LogicalChannel,UINT_8 CoordAddrMode,UINT_16 CoordPANId,IE3ADDR CoordAddress,
				    UINT_8 CapabilityInformation,bool SecurityEnable,
				    bool frUpper = false,PHYenum status = p_SUCCESS,MACenum mStatus = m_SUCCESS);
	void mlme_associate_response(IE3ADDR DeviceAddress,UINT_16 AssocShortAddress,MACenum Status,bool SecurityEnable,
				     bool frUpper = false,PHYenum status = p_SUCCESS);
	void mlme_disassociate_request(IE3ADDR DeviceAddress,UINT_8 DisassociateReason,bool SecurityEnable,bool frUpper = false,PHYenum status = p_SUCCESS);
	void mlme_orphan_response(IE3ADDR OrphanAddress,UINT_16 ShortAddress,bool AssociatedMember,bool SecurityEnable,bool frUpper = false,PHYenum status = p_SUCCESS);
	void mlme_reset_request(bool SetDefaultPIB,bool frUpper = false,PHYenum status = p_SUCCESS);
	void mlme_rx_enable_request(bool DeferPermit,UINT_32 RxOnTime,UINT_32 RxOnDuration,bool frUpper = false,PHYenum status = p_SUCCESS);
	void mlme_scan_request(UINT_8 ScanType,UINT_32 ScanChannels,UINT_8 ScanDuration,bool frUpper = false,PHYenum status = p_SUCCESS);
	void mlme_start_request(UINT_16 PANId,UINT_8 LogicalChannel,UINT_8 BeaconOrder,
				UINT_8 SuperframeOrder,bool PANCoordinator,bool BatteryLifeExtension,
				bool CoordRealignment,bool SecurityEnable,
				bool frUpper = false,PHYenum status = p_SUCCESS);
	void mlme_sync_request(UINT_8 LogicalChannel, bool TrackBeacon,bool frUpper = false,PHYenum status = p_SUCCESS);
	void mlme_poll_request(UINT_8 CoordAddrMode,UINT_16 CoordPANId,IE3ADDR CoordAddress,bool SecurityEnable,
			       bool autoRequest = false,bool firstTime = false,PHYenum status = p_SUCCESS);

	void csmacaBegin(char pktType);
	void csmacaResume(void);
	void csmacaCallBack(PHYenum status);
	int getBattLifeExtSlotNum(void);
	double getCAP(bool small);
	double getCAPbyType(int type);
	bool canProceedWOcsmaca(Packet *p);	//can we proceed w/o CSMA-CA?
	void transmitCmdData(void);
	void reset_TRX(const char *frFile,const char *frFunc,int line);
	void taskSuccess(char task,bool csmacaRes = true);
	void taskFailed(char task,MACenum status,bool csmacaRes = true);
	void freePkt(Packet *pkt);
	UINT_8 macHeaderLen(UINT_16 FrmCtrl);
	void constructACK(Packet *p);
	void constructMPDU(UINT_8 msduLength,Packet *msdu, UINT_16 FrmCtrl,UINT_8 BDSN,panAddrInfo DstAddrInfo,
			    panAddrInfo SrcAddrInfo,UINT_16 SuperSpec,UINT_8 CmdType,UINT_16 FCS);
	void constructCommandHeader(Packet *p,FrameCtrl *frmCtrl,UINT_8 CmdType,
				   UINT_8 dstAddrMode,UINT_16 dstPANId,IE3ADDR dstAddr,
				   UINT_8 srcAddrMode,UINT_16 srcPANId,IE3ADDR srcAddr,
				   bool secuEnable,bool pending,bool ackreq);
	void log(Packet *p);
	void resetCounter(int dst);
	int command(int argc, const char*const* argv);
	void changeNodeColor(double atTime,const char *newColor,bool save = true);
	void txBcnCmdDataHandler(void);
	void IFSHandler(void);
	void backoffBoundHandler(void);

public:
	static bool verbose;
	static UINT_8 txOption;		//0x02=GTS; 0x04=Indirect; 0x00=Direct (only for 802.15.4-unaware upper layer app. packet)
	static bool ack4data;
	static UINT_8 callBack;		//0=no call back; 1=call back for failures; 2=call back for failures and successes
	static UINT_32 DBG_UID;

protected:

	taskPending taskP;
	MAC_PIB mpib;
	PHY_PIB tmp_ppib;
	DevCapability capability;		//device capability (refer to Figure 49)

	//--- for beacon ---
	//(most are temp. variables which should be populated before being used)
	bool secuBeacon;
	SuperframeSpec sfSpec,sfSpec2,sfSpec3;	//superframe specification
	GTSSpec gtsSpec,gtsSpec2;		//GTS specification
	PendAddrSpec pendAddrSpec;		//pending address specification
	UINT_8 beaconPeriods,beaconPeriods2;	//# of backoff periods it takes to transmit the beacon
	PAN_ELE	panDes,panDes2;			//PAN descriptor
	Packet *rxBeacon;			//the beacon packet just received
	double	macBcnTxTime;			//the time last beacon sent (in symbol) (we use this double variable instead of integer mpib.macBeaconTxTime for accuracy reason)
	double	macBcnRxTime;			//the time last beacon received from within PAN (in symbol)
	double	macBcnOtherRxTime;		//the time last beacon received from outside PAN (in symbol)
	//To support beacon enabled mode in multi-hop envirionment, we use {<mpib.macBeaconOrder>,<mpib.macSuperframeOrder>}
	//for coordinators (transmitting beacons) and the following two parameters for devices (receiving beacons). Note that,
	//in such an environment, a node can act as a coordinator and a device at the same time. More complicated algorithm 
	//is required for slotted CSMA-CA in this case.
	//(does 802.15.4 have this in mind?) 
	UINT_8	macBeaconOrder2;
	UINT_8	macSuperframeOrder2;
	UINT_8	macBeaconOrder3;
	UINT_8	macSuperframeOrder3;
	bool	oneMoreBeacon;			
	UINT_8	numLostBeacons;			//# of beacons lost in a row
	//------------------
	UINT_16	rt_myNodeID;			

	UINT_8 energyLevel;			

	//for association and transaction
	DEVICELINK *deviceLink1;
	DEVICELINK *deviceLink2;
	TRANSACLINK *transacLink1;
	TRANSACLINK *transacLink2;

private:
	IE3ADDR aExtendedAddress;

	//timers
	macTxOverTimer *txOverT;
	macTxTimer *txT;
	macExtractTimer *extractT;
	macAssoRspWaitTimer *assoRspWaitT;
	macDataWaitTimer *dataWaitT;
	macRxEnableTimer *rxEnableT;
	macScanTimer *scanT;
	macBeaconTxTimer *bcnTxT;		//beacon transmission timer
	macBeaconRxTimer *bcnRxT;		//beacon reception timer
	macBeaconSearchTimer *bcnSearchT;	//beacon search timer
	macWakeupTimer *wakeupT; 		// 2.31 change:

	//handlers
	Mac802_15_4Handler txCmdDataH;
	Mac802_15_4Handler IFSH;
	Mac802_15_4Handler backoffBoundH;

	bool isPANCoor;			//is a PAN coordinator?
	Phy802_15_4 *phy;
	CsmaCA802_15_4 *csmaca;
	SSCS802_15_4 *sscs;
	Nam802_15_4 *nam;
	PHYenum trx_state_req;		//tranceiver state required: TRX_OFF/TX_ON/RX_ON
	bool inTransmission;		//in the middle of transmission
	bool beaconWaiting;		//it's about time to transmit beacon (suppress all other transmissions)
	Packet *txBeacon;		//beacon packet to be transmitted (w/o using CSMA-CA)
	Packet *txAck;			//ack. packet to be transmitted (no waiting)
	Packet *txBcnCmd;		//beacon or command packet waiting for transmission (using CSMA-CA) -- triggered by receiving a packet
	Packet *txBcnCmd2;		//beacon or command packet waiting for transmission (using CSMA-CA) -- triggered by upper layer
	Packet *txData;			//data packet waiting for transmission (using CSMA-CA)
	Packet *txCsmaca;		//for which packet (txBcnCmd/txBcnCmd2/txData) is CSMA-CA performed
	Packet *txPkt;			//packet (any type) currently being transmitted
	Packet *rxData;			//data packet received (waiting for passing up)
	Packet *rxCmd;			//command packet received (will be handled after the transmission of ack.)
	UINT_32	txPkt_uid;		//for debug purpose
	double rxDataTime;		//the time when data packet received by MAC
	bool waitBcnCmdAck;		//only used if (txBcnCmd): waiting for an ack. or not
	bool waitBcnCmdAck2;		//only used if (txBcnCmd2): waiting for an ack. or not
	bool waitDataAck;		//only used if (txData): waiting for an ack. or not
	UINT_8 backoffStatus;		//0=no backoff yet;1=backoff successful;2=backoff failed;99=in the middle of backoff
	UINT_8 numDataRetry;		//# of retries (retransmissions) for data packet
	UINT_8 numBcnCmdRetry;		//# of retries (retransmissions) for beacon or command packet
	UINT_8 numBcnCmdRetry2;		//# of retries (retransmissions) for beacon or command packet

	NsObject *logtarget_;

	//packet duplication detection
	HLISTLINK *hlistBLink1;
	HLISTLINK *hlistBLink2;
	HLISTLINK *hlistDLink1;
	HLISTLINK *hlistDLink2;
};

#define plme_set_trx_state_request(state) \
set_trx_state_request(state,__FILE__,__FUNCTION__,__LINE__)
#define resetTRX() \
reset_TRX(__FILE__,__FUNCTION__,__LINE__)

#endif

// End of file: p802_15_4mac.h
