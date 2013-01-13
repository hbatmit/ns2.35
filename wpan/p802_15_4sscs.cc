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

// File:  p802_15_4sscs.cc
// Mode:  C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t

// $Header: /cvsroot/nsnam/ns-2/wpan/p802_15_4sscs.cc,v 1.5 2011/06/22 04:03:26 tom_henderson Exp $

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


#include "p802_15_4sscs.h"

#ifdef ZigBeeIF
#include "../zbr/zbr_link.h"
#include "../zbr/zbr.h"
#endif

#define assoRetryInterval	1.0

//UINT_32 SSCS802_15_4::ScanChannels = 0x07ffffff;
UINT_32 SSCS802_15_4::ScanChannels = 0x00003800;	//only scan the first 3 channels in 2.4G

void SSCS802_15_4Timer::start(double wtime)
{
	assert(!active);
	active = true;
	nullEvent.uid_ = 0;
	Scheduler::instance().schedule(this,&nullEvent,wtime);
}

void SSCS802_15_4Timer::cancel(void)
{
	active = false;
	Scheduler::instance().cancel(&nullEvent);
}

void SSCS802_15_4Timer::handle(Event* )
{
	active = false;
	if (sscs->neverAsso)
		sscs->startDevice(sscs->t_isCT,sscs->t_isFFD,sscs->t_assoPermit,sscs->t_txBeacon,sscs->t_BO,sscs->t_SO,true);
}

char *statusName(MACenum status)
{
	switch(status)
	{
		case m_SUCCESS:
			return "SUCCESS";
		case m_PAN_at_capacity:
			return "PAN_at_capacity";
		case m_PAN_access_denied:
			return "PAN_access_denied";
		case m_BEACON_LOSS:
			return "BEACON_LOSS";
		case m_CHANNEL_ACCESS_FAILURE:
			return "CHANNEL_ACCESS_FAILURE";
		case m_DENIED:
			return "DENIED";
		case m_DISABLE_TRX_FAILURE:
			return "DISABLE_TRX_FAILURE";
		case m_FAILED_SECURITY_CHECK:
			return "FAILED_SECURITY_CHECK";
		case m_FRAME_TOO_LONG:
			return "FRAME_TOO_LONG";
		case m_INVALID_GTS:
			return "INVALID_GTS";
		case m_INVALID_HANDLE:
			return "INVALID_HANDLE";
		case m_INVALID_PARAMETER:
			return "INVALID_PARAMETER";
		case m_NO_ACK:
			return "NO_ACK";
		case m_NO_BEACON:
			return "NO_BEACON";
		case m_NO_DATA:
			return "NO_DATA";
		case m_NO_SHORT_ADDRESS:
			return "NO_SHORT_ADDRESS";
		case m_OUT_OF_CAP:
			return "OUT_OF_CAP";
		case m_PAN_ID_CONFLICT:
			return "PAN_ID_CONFLICT";
		case m_REALIGNMENT:
			return "REALIGNMENT";
		case m_TRANSACTION_EXPIRED:
			return "TRANSACTION_EXPIRED";
		case m_TRANSACTION_OVERFLOW:
			return "TRANSACTION_OVERFLOW";
		case m_TX_ACTIVE:
			return "TX_ACTIVE";
		case m_UNAVAILABLE_KEY:
			return "UNAVAILABLE_KEY";
		case m_UNSUPPORTED_ATTRIBUTE:
			return "UNSUPPORTED_ATTRIBUTE";
		case m_UNDEFINED:
		default:
			return "UNDEFINED";
	}
}

SSCS802_15_4::SSCS802_15_4(Mac802_15_4 *m):assoH(this)
{
	mac = m;
	neverAsso = true;
#ifdef ZigBeeIF
	zbr = getZBRLink(mac->index_);
#endif
	hlistLink1 = NULL;
	hlistLink2 = NULL;
}

SSCS802_15_4::~SSCS802_15_4()
{
}

void SSCS802_15_4::MCPS_DATA_confirm(UINT_8 , MACenum )
{
}

void SSCS802_15_4::MCPS_DATA_indication(UINT_8 , UINT_16     , IE3ADDR ,
					UINT_8 , UINT_16     , IE3ADDR ,
					UINT_8 , Packet *msdu, UINT_8  ,
					bool   , UINT_8 )
{
	Packet::free(msdu);
}

void SSCS802_15_4::MCPS_PURGE_confirm(UINT_8 , MACenum )
{
}

#ifdef ZigBeeIF
extern NET_SYSTEM_CONFIG NetSystemConfig;
#endif
void SSCS802_15_4::MLME_ASSOCIATE_indication(IE3ADDR DeviceAddress, UINT_8 CapabilityInformation, bool , UINT_8)
{
	
	//we assign the cluster tree address as the MAC short address

#ifdef ZigBeeIF
	if (t_isCT)			//need to assign a cluster tree logic address
	{
		assertZBR();
		noCapacity = false;
		if (zbr->myDepth >= NetSystemConfig.Lm)
			noCapacity = true;
		else
		{
			child_num = 1;
			logAddr = zbr->myNodeID + 1;				
			while (!updateCTAddrLink(zbr_oper_est,logAddr))
			{
				if (getIpAddrFrLogAddr(logAddr,false) == (UINT_16)DeviceAddress)
					break; 
				logAddr += ZBR::c_skip(zbr->myDepth);		
				child_num++;
				if (child_num > NetSystemConfig.Cm)
					break;
			}
			if (child_num > NetSystemConfig.Cm)
				noCapacity = true;
		}
		if (noCapacity)						//no capacity
		{
#ifdef DEBUG802_15_4
			fprintf(stdout,"[%s::%s][%f](node %d) no capacity for cluster-tree: request from %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,mac->index_,DeviceAddress);
#endif
			zbr->sscs_nb_insert((UINT_16)DeviceAddress,NEIGHBOR);
			mac->MLME_ASSOCIATE_response(DeviceAddress,0,m_PAN_at_capacity,false);
		}
		else
		{
			chkAddCTAddrLink(logAddr,DeviceAddress);
			chkAddDeviceLink(&mac->deviceLink1,&mac->deviceLink2,DeviceAddress,CapabilityInformation);
			zbr->sscs_nb_insert((UINT_16)DeviceAddress,CHILD);
			mac->MLME_ASSOCIATE_response(DeviceAddress,logAddr,m_SUCCESS,false);
		}
	}
	else	//just assign the IP address as the MAC short address for non-cluster tree
#endif
	{
		chkAddDeviceLink(&mac->deviceLink1,&mac->deviceLink2,DeviceAddress,CapabilityInformation);
		mac->MLME_ASSOCIATE_response(DeviceAddress,(UINT_16)DeviceAddress,m_SUCCESS,false);
	}
}

void SSCS802_15_4::MLME_ASSOCIATE_confirm(UINT_16 AssocShortAddress,MACenum status)
{
	MAC_PIB t_mpib;

	if (status == m_SUCCESS)
	{
		rt_myNodeID = AssocShortAddress;
		/*
		if (!sscsTaskP.startDevice_isCluster_Tree)
			t_mpib.macShortAddress = AssocShortAddress;
		else
		*/
			t_mpib.macShortAddress = mac->index_;		//don't use cluster tree logic address
		mac->MLME_SET_request(macShortAddress,&t_mpib);
	}
	dispatch(status,"MLME_ASSOCIATE_confirm");
}

void SSCS802_15_4::MLME_DISASSOCIATE_confirm(MACenum )
{
}

void SSCS802_15_4::MLME_BEACON_NOTIFY_indication(UINT_8 , PAN_ELE *, UINT_8 ,IE3ADDR * , UINT_8 , UINT_8 *)
{
}

void SSCS802_15_4::MLME_GET_confirm(MACenum , MPIBAenum , MAC_PIB *)
{
}

void SSCS802_15_4::MLME_ORPHAN_indication(IE3ADDR OrphanAddress, bool , UINT_8 )
{
	if (updateDeviceLink(tr_oper_est,&mac->deviceLink1,&mac->deviceLink2,OrphanAddress) == 0)
		mac->MLME_ORPHAN_response(OrphanAddress,(UINT_16)OrphanAddress,true,false);
	else
		mac->MLME_ORPHAN_response(OrphanAddress,0,false,false);
}

void SSCS802_15_4::MLME_RESET_confirm(MACenum )
{
}

void SSCS802_15_4::MLME_RX_ENABLE_confirm(MACenum )
{
}

void SSCS802_15_4::MLME_SET_confirm(MACenum , MPIBAenum )
{
}

void SSCS802_15_4::MLME_SCAN_confirm(MACenum status,UINT_8 ScanType,UINT_32 UnscannedChannels,
				     UINT_8 ResultListSize,UINT_8 *EnergyDetectList,
				     PAN_ELE *PANDescriptorList)
{
	MAC_PIB t_mpib;

	T_UnscannedChannels = UnscannedChannels;
	T_ResultListSize = ResultListSize;
	T_EnergyDetectList = EnergyDetectList;
	T_PANDescriptorList = PANDescriptorList;
	if (ScanType == 0x01)
	{
		dispatch(status,"MLME_SCAN_confirm");
	}

	if (ScanType == 0x03)
	{
		if (status == m_SUCCESS)
		{
			fprintf(stdout,"[%f](node %d) coordinator relocation successful, begin to re-synchronize with the coordinator\n",CURRENT_TIME,mac->index_);
			//re-synchronize with the coordinator
			mac->phy->PLME_GET_request(phyCurrentChannel);
			mac->MLME_SYNC_request(mac->tmp_ppib.phyCurrentChannel,true);
		}
		else
		{
			bool isCoord = ((mac->capability.FFD)&&(numberDeviceLink(&mac->deviceLink1) > 0));
			fprintf(stdout,"[%f](node %d) coordinator relocation failed%s\n",CURRENT_TIME,mac->index_,(isCoord)?".":" --> try to reassociate ...");
			if (!isCoord)		//I am not a coordinator
			{
				t_mpib.macShortAddress = 0xffff;
				mac->MLME_SET_request(macShortAddress,&t_mpib);
				t_mpib.macCoordExtendedAddress = def_macCoordExtendedAddress;
				mac->MLME_SET_request(macCoordExtendedAddress,&t_mpib);
				startDevice(t_isCT,t_isFFD,t_assoPermit,t_txBeacon,t_BO,t_SO,true);
			}
		}
	}
}

void SSCS802_15_4::MLME_COMM_STATUS_indication(UINT_16 , UINT_8 , IE3ADDR ,
				 UINT_8 , IE3ADDR , MACenum )
{
}

void SSCS802_15_4::MLME_START_confirm(MACenum status)
{
	dispatch(status,"MLME_START_confirm");
}

void SSCS802_15_4::MLME_SYNC_LOSS_indication(MACenum )
{
	fprintf(stdout,"[%f](node %d) synchronization loss\n",CURRENT_TIME,mac->index_);
	mac->MLME_SCAN_request(0x03,SSCS802_15_4::ScanChannels,0);
}

void SSCS802_15_4::MLME_POLL_confirm(MACenum )
{
}

//--------------------------------------------------------------------------

char *sscsTaskName[] = {"NONE",
			"startPANCoord",
			"startDevice"};
void SSCS802_15_4::checkTaskOverflow(UINT_8 task)
{
	if (sscsTaskP.taskStatus(task))
	{
		fprintf(stdout,"[SSCS][%f](node %d) task overflow: %s\n",CURRENT_TIME,mac->index_,sscsTaskName[task]);
		exit(1);
	}
	else
		sscsTaskP.taskStep(task) = 0;
}

void SSCS802_15_4::dispatch(MACenum status,char *frFunc)
{
	if (strcmp(frFunc,"MLME_SCAN_confirm") == 0)
	{
		if (sscsTaskP.taskStatus(sscsTP_startPANCoord))
			startPANCoord(sscsTaskP.startPANCoord_isCluster_Tree,sscsTaskP.startPANCoord_txBeacon,sscsTaskP.startPANCoord_BO,sscsTaskP.startPANCoord_SO,false,status);
		else if (sscsTaskP.taskStatus(sscsTP_startDevice))
			startDevice(sscsTaskP.startDevice_isCluster_Tree,sscsTaskP.startDevice_isFFD,sscsTaskP.startDevice_assoPermit,sscsTaskP.startDevice_txBeacon,sscsTaskP.startDevice_BO,sscsTaskP.startDevice_SO,false,status);
	}
	else if (strcmp(frFunc,"MLME_START_confirm") == 0)
	{
		if(sscsTaskP.taskStatus(sscsTP_startPANCoord))
			startPANCoord(sscsTaskP.startPANCoord_isCluster_Tree,sscsTaskP.startPANCoord_txBeacon,sscsTaskP.startPANCoord_BO,sscsTaskP.startPANCoord_SO,false,status);
		else if (sscsTaskP.taskStatus(sscsTP_startDevice))
			startDevice(sscsTaskP.startDevice_isCluster_Tree,sscsTaskP.startDevice_isFFD,sscsTaskP.startDevice_assoPermit,sscsTaskP.startDevice_txBeacon,sscsTaskP.startDevice_BO,sscsTaskP.startDevice_SO,false,status);
		else	//default handling
		{
			if (mac->mpib.macBeaconOrder == 15)
				fprintf(stdout,"[%f](node %d) beacon transmission stopped [channel:%d] [PAN_ID:%d]\n",CURRENT_TIME,mac->index_,mac->tmp_ppib.phyCurrentChannel,mac->mpib.macPANId);
			else if (status == m_SUCCESS)
				fprintf(stdout,"[%f](node %d) beacon transmission successful [channel:%d] [PAN_ID:%d]\n",CURRENT_TIME,mac->index_,mac->tmp_ppib.phyCurrentChannel,mac->mpib.macPANId);
			else
				fprintf(stdout,"<!>[%f](node %d) failed to transmit beacons -> %s [channel:%d] [PAN_ID:%d]\n",CURRENT_TIME,mac->index_,statusName(status),mac->tmp_ppib.phyCurrentChannel,mac->mpib.macPANId);
		}
	}
	else if (strcmp(frFunc,"MLME_ASSOCIATE_confirm") == 0)
	{
		if(sscsTaskP.taskStatus(sscsTP_startDevice))
			startDevice(sscsTaskP.startDevice_isCluster_Tree,sscsTaskP.startDevice_isFFD,sscsTaskP.startDevice_assoPermit,sscsTaskP.startDevice_txBeacon,sscsTaskP.startDevice_BO,sscsTaskP.startDevice_SO,false,status);
	}
}

void SSCS802_15_4::startPANCoord(bool isClusterTree,bool txBeacon,UINT_8 BO,UINT_8 SO,bool firsttime,MACenum status)
{
	UINT_8 step;
	MAC_PIB t_mpib;
	PHY_PIB t_ppib;
	int i;
		
	if (firsttime) checkTaskOverflow(sscsTP_startPANCoord);

	step = sscsTaskP.taskStep(sscsTP_startPANCoord);
	switch(step)
	{
		case 0:
			fprintf(stdout,"--- startPANCoord [%d] ---\n",mac->index_);
			sscsTaskP.taskStatus(sscsTP_startPANCoord) = true;
			sscsTaskP.taskStep(sscsTP_startPANCoord)++;
			sscsTaskP.startPANCoord_isCluster_Tree = isClusterTree;
			sscsTaskP.startPANCoord_txBeacon = txBeacon;
			sscsTaskP.startPANCoord_BO = BO;
			sscsTaskP.startPANCoord_SO = SO;
			//must be an FFD
			mac->capability.setFFD(true);
			//assign a short address for myself
#ifdef ZigBeeIF
			if (isClusterTree)
			{
				assertZBR();
				zbr->myDepth = 0;
				zbr->myNodeID = 0;			//assign logic address 0 for myself
				zbr->myParentNodeID = 0;		//no parent, assign my own ID
				chkAddCTAddrLink(zbr->myNodeID,mac->index_);
				activateCTAddrLink(zbr->myNodeID,mac->index_);
			}
#endif
			t_mpib.macShortAddress = mac->index_;
			mac->MLME_SET_request(macShortAddress,&t_mpib);
			//scan the channels
			fprintf(stdout,"[%f](node %d) performing active channel scan\n",CURRENT_TIME,mac->index_);
			mac->MLME_SCAN_request(0x01,SSCS802_15_4::ScanChannels,BO);
			break;
		case 1:
			if (status != m_SUCCESS)
			{
				fprintf(stdout,"<!>[%f](node %d) unable to start as a PAN coordinator: active channel scan failed -> %s\n",CURRENT_TIME,mac->index_,statusName(status));
				sscsTaskP.taskStatus(sscsTP_startPANCoord) = false;
				return;
			}
			//select a channel and a PAN ID (for simplicity, we just use the IP address as the PAN ID)
			//(it's not an easy task to select a channel and PAN ID in implementation!)
			for (i=11;i<27;i++)		//we give priority to 2.4G
			if ((T_UnscannedChannels & (1 << i)) == 0)
				break;
			if (i >= 27)
			for (i=0;i<11;i++)
			if ((T_UnscannedChannels & (1 << i)) == 0)
				break;
			sscsTaskP.startPANCoord_Channel = i;
			//permit association
			t_mpib.macAssociationPermit = true;
			mac->MLME_SET_request(macAssociationPermit,&t_mpib);
			if (txBeacon)
			{
				sscsTaskP.taskStep(sscsTP_startPANCoord)++;
				fprintf(stdout,"[%f](node %d) begin to transmit beacons\n",CURRENT_TIME,mac->index_);
				mac->MLME_START_request(mac->index_,i,BO,SO,true,false,false,false);
			}
			else
			{
				mac->isPanCoor(true);
				t_mpib.macCoordExtendedAddress = mac->index_;
				mac->MLME_SET_request(macCoordExtendedAddress,&t_mpib);
				t_ppib.phyCurrentChannel = i;
				mac->phy->PLME_SET_request(phyCurrentChannel,&t_ppib);
				sscsTaskP.taskStatus(sscsTP_startPANCoord) = false;
				fprintf(stdout,"[%f](node %d) successfully started a new PAN (non-beacon enabled) [channel:%d] [PAN_ID:%d]\n",CURRENT_TIME,mac->index_,sscsTaskP.startPANCoord_Channel,mac->index_);
				t_mpib.macPANId = mac->index_;
				mac->MLME_SET_request(macPANId,&t_mpib);
				t_mpib.macBeaconOrder = 15;
				mac->MLME_SET_request(macBeaconOrder,&t_mpib);
				t_mpib.macSuperframeOrder = 15;
				mac->MLME_SET_request(macSuperframeOrder,&t_mpib);
			}
#ifdef ZigBeeIF
			if (isClusterTree)
			{
				assertZBR();
				zbr->dRate = mac->phy->getRate('d');
			}
#endif
			break;
		case 2:
			sscsTaskP.taskStatus(sscsTP_startPANCoord) = false;
			if (status == m_SUCCESS)
				fprintf(stdout,"[%f](node %d) successfully started a new PAN (beacon enabled) [channel:%d] [PAN_ID:%d]\n",CURRENT_TIME,mac->index_,sscsTaskP.startPANCoord_Channel,mac->index_);
			else
				fprintf(stdout,"<!>[%f](node %d) failed to transmit beacons -> %s [channel:%d] [PAN_ID:%d]\n",CURRENT_TIME,mac->index_,statusName(status),sscsTaskP.startPANCoord_Channel,mac->index_);
			break;
		default:
			break;
	}
}

void SSCS802_15_4::startDevice(bool isClusterTree,bool isFFD,bool assoPermit,bool txBeacon,UINT_8 BO,UINT_8 SO,bool firsttime,MACenum status)
{
	UINT_8 step,scan_BO;
	MAC_PIB t_mpib;
	SuperframeSpec sfSpec;
	UINT_8 ch,fstChannel,fstChannel2_4G;
	char tmpstr[30];
	int i,k,l,n;
		
	if (firsttime) checkTaskOverflow(sscsTP_startDevice);

	step = sscsTaskP.taskStep(sscsTP_startDevice);
	switch(step)
	{
		case 0:
			fprintf(stdout,"--- startDevice [%d] ---\n",mac->index_);
			sscsTaskP.taskStatus(sscsTP_startDevice) = true;
			sscsTaskP.taskStep(sscsTP_startDevice)++;
			mac->capability.setFFD(isFFD);
			sscsTaskP.startDevice_isCluster_Tree = isClusterTree;
			sscsTaskP.startDevice_isFFD = isFFD;
			sscsTaskP.startDevice_assoPermit = assoPermit;
			sscsTaskP.startDevice_txBeacon = txBeacon;
			sscsTaskP.startDevice_BO = BO;
			sscsTaskP.startDevice_SO = SO;
			scan_BO = sscsTaskP.startDevice_BO + 1;
			//set FFD
			mac->capability.setFFD(isFFD);
			//scan the channels
			fprintf(stdout,"[%f](node %d) performing active channel scan ...\n",CURRENT_TIME,mac->index_);
			mac->MLME_SCAN_request(0x01,SSCS802_15_4::ScanChannels,scan_BO);
			break;
		case 1:
			if (status != m_SUCCESS)
			{
				sscsTaskP.taskStatus(sscsTP_startDevice) = false;
				fprintf(stdout,"<!>[%f](node %d) unable to start as a device: active channel scan failed -> %s\n",CURRENT_TIME,mac->index_,statusName(status));
				assoH.start(assoRetryInterval);
				return;
			}
			//select a PAN and a coordinator to join
			fstChannel = 0xff;
			fstChannel2_4G = 0xff;
			for (i=0;i<T_ResultListSize;i++)
			{
				sfSpec.SuperSpec = T_PANDescriptorList[i].SuperframeSpec;
				sfSpec.parse();
				n = updateHListLink(hl_oper_est,&hlistLink1,&hlistLink2,(UINT_16)T_PANDescriptorList[i].CoordAddress_64);
				if ((!sfSpec.AssoPmt)||(!n))
					continue;
				else
				{
					if (T_PANDescriptorList[i].LogicalChannel < 11)
					{
						if (fstChannel == 0xff)
						{
							fstChannel = T_PANDescriptorList[i].LogicalChannel;
							k = i;
						}
					}
					else
					{
						if (fstChannel2_4G == 0xff)
						{
							fstChannel2_4G = T_PANDescriptorList[i].LogicalChannel;
							l = i;
						}
					}
				}
			}
			if (fstChannel2_4G != 0xff)
			{
				ch = fstChannel2_4G;
				i = l;
			}
			else
			{
				ch = fstChannel;
				i = k;
			}
			if (ch == 0xff)		//cannot find any coordinator for association
			{
				sscsTaskP.taskStatus(sscsTP_startDevice) = false;
				fprintf(stdout,"<!>[%f](node %d) no coordinator found for association.\n",CURRENT_TIME,mac->index_);
				assoH.start(assoRetryInterval);
				return;
			}
			else
			{
				//select the least depth for cluster tree association
#ifdef ZigBeeIF
				if (isClusterTree)
				{
					depth = T_PANDescriptorList[i].clusTreeDepth;
					for (m=0;m<T_ResultListSize;m++)
					{
						n = updateHListLink(hl_oper_est,&hlistLink1,&hlistLink2,(UINT_16)T_PANDescriptorList[m].CoordAddress_64);
						if ((ch == T_PANDescriptorList[m].LogicalChannel)&&(n))
						if (T_PANDescriptorList[m].clusTreeDepth < depth)
						{
							depth = T_PANDescriptorList[m].clusTreeDepth;
							i = m;
						}
					}
				}
#endif
				//If the coordinator is in beacon-enabled mode, we may begin to track beacons now.
				//But this is only possible if the network is a one-hop star; otherwise we don't know
				//which coordinator to track, since there may be more than one beaconing coordinators
				//in a device's neighborhood and MLME-SYNC.request() has no parameter telling which 
				//coordinator to track. As this is an optional step, we will not track beacons here.
				t_mpib.macAssociationPermit = assoPermit;
				mac->MLME_SET_request(macAssociationPermit,&t_mpib);
				sscsTaskP.startDevice_Channel = ch;
				fprintf(stdout,"[%f](node %d) sending association request to [channel:%d] [PAN_ID:%d] [CoordAddr:%d] ... \n",CURRENT_TIME,mac->index_,ch,T_PANDescriptorList[i].CoordPANId,T_PANDescriptorList[i].CoordAddress_64);
				sscsTaskP.taskStep(sscsTP_startDevice)++;
				sscsTaskP.startDevice_panDes = T_PANDescriptorList[i];
				mac->MLME_ASSOCIATE_request(ch,T_PANDescriptorList[i].CoordAddrMode,T_PANDescriptorList[i].CoordPANId,T_PANDescriptorList[i].CoordAddress_64,mac->capability.cap,false);
			}
			break;
		case 2:
			sfSpec.SuperSpec = sscsTaskP.startDevice_panDes.SuperframeSpec;
			sfSpec.parse();
			if (sfSpec.BO != 15)
				strcpy(tmpstr,"beacon enabled");
			else
				strcpy(tmpstr,"non-beacon enabled");
			if (status != m_SUCCESS)
			{
				//reset association permission
				t_mpib.macAssociationPermit = false;
				mac->MLME_SET_request(macAssociationPermit,&t_mpib);
				fprintf(stdout,"<!>[%f](node %d) association failed -> %s (%s) [channel:%d] [PAN_ID:%d] [CoordAddr:%d]\n",CURRENT_TIME,mac->index_,statusName(status),tmpstr,sscsTaskP.startDevice_panDes.LogicalChannel,sscsTaskP.startDevice_panDes.CoordPANId,sscsTaskP.startDevice_panDes.CoordAddress_64);
				assoH.start(assoRetryInterval);
				sscsTaskP.taskStatus(sscsTP_startDevice) = false;
#ifdef ZigBeeIF
				if (isClusterTree)
				{
					assertZBR();
					zbr->sscs_nb_insert(sscsTaskP.startDevice_panDes.CoordAddress_64,NEIGHBOR);
					if (status == m_PAN_at_capacity)
						chkAddUpdHListLink(&hlistLink1,&hlistLink2,(UINT_16)sscsTaskP.startDevice_panDes.CoordAddress_64,0);
				}
#endif
			}
			else
			{
				neverAsso = false;
				fprintf(stdout,"[%f](node %d) association successful (%s) [channel:%d] [PAN_ID:%d] [CoordAddr:%d]\n",CURRENT_TIME,mac->index_,tmpstr,sscsTaskP.startDevice_panDes.LogicalChannel,sscsTaskP.startDevice_panDes.CoordPANId,sscsTaskP.startDevice_panDes.CoordAddress_64);
#ifdef ZigBeeIF
				if (isClusterTree)
				{
					assertZBR();
					zbr->myDepth = rt_myDepth;
					zbr->myNodeID = rt_myNodeID;
					zbr->myParentNodeID = rt_myParentNodeID;
					zbr->sscs_nb_insert(sscsTaskP.startDevice_panDes.CoordAddress_64,PARENT);
					//chkAddCTAddrLink(zbr->myNodeID,mac->index_);	//too late -- may result in assigning duplicated addresses
					activateCTAddrLink(zbr->myNodeID,mac->index_);
					emptyHListLink(&hlistLink1,&hlistLink2);
				}
#endif
				if (sfSpec.BO != 15)
				{
					fprintf(stdout,"[%f](node %d) begin to synchronize with the coordinator\n",CURRENT_TIME,mac->index_);
					mac->MLME_SYNC_request(sscsTaskP.startDevice_panDes.LogicalChannel,true);
					sscsTaskP.taskStatus(sscsTP_startDevice) = false;
				}
				if (isFFD && txBeacon)
				{
					sscsTaskP.taskStep(sscsTP_startDevice)++;
					fprintf(stdout,"[%f](node %d) begin to transmit beacons\n",CURRENT_TIME,mac->index_);
					mac->MLME_START_request(mac->mpib.macPANId,sscsTaskP.startDevice_Channel,BO,SO,false,false,false,false);
				}
				else
					sscsTaskP.taskStatus(sscsTP_startDevice) = false;
#ifdef ZigBeeIF
				if (isClusterTree)
					zbr->dRate = mac->phy->getRate('d');
#endif
			}
			break;
		case 3:
			sscsTaskP.taskStatus(sscsTP_startDevice) = false;
			if (status == m_SUCCESS)
				fprintf(stdout,"[%f](node %d) beacon transmission successful [channel:%d] [PAN_ID:%d]\n",CURRENT_TIME,mac->index_,sscsTaskP.startDevice_Channel,mac->mpib.macPANId);
			else
				fprintf(stdout,"<!>[%f](node %d) failed to transmit beacons -> %s [channel:%d] [PAN_ID:%d]\n",CURRENT_TIME,mac->index_,statusName(status),sscsTaskP.startDevice_Channel,mac->mpib.macPANId);
			break;
		default:
			break;
	}
}

//--------------------------------------------------------------------------

/* The following primitives are availabe from MAC sublayer. You can call these primitives from SSCS or other upper layer
 *	void MCPS_DATA_request(UINT_8 SrcAddrMode,UINT_16 SrcPANId,IE3ADDR SrcAddr,
 *			       UINT_8 DstAddrMode,UINT_16 DstPANId,IE3ADDR DstAddr,
 *			       UINT_8 msduLength,Packet *msdu,UINT_8 msduHandle,UINT_8 TxOptions);
 *	void MCPS_DATA_indication(UINT_8 SrcAddrMode,UINT_16 SrcPANId,IE3ADDR SrcAddr,
 *				  UINT_8 DstAddrMode,UINT_16 DstPANId,IE3ADDR DstAddr,
 *				  UINT_8 msduLength,Packet *msdu,UINT_8 mpduLinkQuality,
 *				  bool SecurityUse,UINT_8 ACLEntry);
 *	void MCPS_PURGE_request(UINT_8 msduHandle);
 *	void MLME_ASSOCIATE_request(UINT_8 LogicalChannel,UINT_8 CoordAddrMode,UINT_16 CoordPANId,IE3ADDR CoordAddress,
 *				    UINT_8 CapabilityInformation,bool SecurityEnable);
 *	void MLME_ASSOCIATE_response(IE3ADDR DeviceAddress,UINT_16 AssocShortAddress,MACenum status,bool SecurityEnable);
 *	void MLME_DISASSOCIATE_request(IE3ADDR DeviceAddress,UINT_8 DisassociateReason,bool SecurityEnable);
 *	void MLME_DISASSOCIATE_indication(IE3ADDR DeviceAddress,UINT_8 DisassociateReason,bool SecurityUse,UINT_8 ACLEntry);
 *	void MLME_DISASSOCIATE_confirm(MACenum status);
 *	void MLME_GET_request(MPIBAenum PIBAttribute);
 *	void MLME_GTS_request(UINT_8 GTSCharacteristics,bool SecurityEnable);
 *	void MLME_GTS_confirm(UINT_8 GTSCharacteristics,MACenum status);
 *	void MLME_GTS_indication(UINT_16 DevAddress,UINT_8 GTSCharacteristics,
 *				 bool SecurityUse, UINT_8 ACLEntry);
 *	void MLME_ORPHAN_indication(IE3ADDR OrphanAddress,bool SecurityUse,UINT_8 ACLEntry);
 *	void MLME_ORPHAN_response(IE3ADDR OrphanAddress,UINT_16 ShortAddress,bool AssociatedMember,bool SecurityEnable);
 *	void MLME_RESET_request(bool SetDefaultPIB);
 *	void MLME_RX_ENABLE_request(bool DeferPermit,UINT_32 RxOnTime,UINT_32 RxOnDuration);
 *	void MLME_RX_ENABLE_confirm(MACenum status);
 *	void MLME_SCAN_request(UINT_8 ScanType,UINT_32 ScanChannels,UINT_8 ScanDuration);
 *	void MLME_SET_request(MPIBAenum PIBAttribute,MAC_PIB *PIBAttributeValue);
 *	void MLME_SET_confirm(MACenum status,MPIBAenum PIBAttribute);
 *	void MLME_START_request(UINT_16 PANId,UINT_8 LogicalChannel,UINT_8 BeaconOrder,
 *				UINT_8 SuperframeOrder,bool PANCoordinator,bool BatteryLifeExtension,
 *				bool CoordRealignment,bool SecurityEnable);
 *	void MLME_SYNC_request(UINT_8 LogicalChannel, bool TrackBeacon);
 *	void MLME_SYNC_LOSS_indication(MACenum LossReason);
 *	void MLME_POLL_request(UINT_8 CoordAddrMode,UINT_16 CoordPANId,IE3ADDR CoordAddress,bool SecurityEnable);
 *	void MLME_POLL_confirm(MACenum status);
 */

//--------------------------------------------------------------------------

/* The following commands are available in Tcl scripts -- most of them are the wrap-up of primitives */
int SSCS802_15_4::command(int argc, const char*const* argv)
{
	//Commands from	Tcl will be one	of the following forms:
	// --- $node sscs startPANCoord	<txBeacon = 1> <beaconOrder = 3> <SuperframeOrder = 3>
	// --- $node sscs startDevice <isFFD = 1> <assoPermit = 1> <txBeacon = 0> <beaconOrder = 3> <SuperframeOrder = 3>
	// --- $node sscs startCTPANCoord <txBeacon = 1> <beaconOrder = 3> <SuperframeOrder = 3>
	// --- $node sscs startCTDevice <isFFD = 1> <assoPermit = 1> <txBeacon = 0> <beaconOrder = 3> <SuperframeOrder = 3>
	// --- $node sscs startBeacon <beaconOrder = 3> <SuperframeOrder = 3>
	// --- $node sscs stopBeacon
	int i;

	if ((strcmp(argv[2], "startPANCoord") == 0)
	||  (strcmp(argv[2], "startCTPANCoord") == 0))
	{
		i = 2;
		t_isCT = (strcmp(argv[i], "startCTPANCoord") == 0);
#ifndef ZigBeeIF
		t_isCT = false;
#endif
		if (argc == i + 1)
		{
			t_txBeacon = true;
			t_BO = 3;
			t_SO = 3;
		}
		else if (argc == i + 2)
		{
			t_txBeacon = (atoi(argv[i+1])!=0);
			t_BO = 3;
			t_SO = 3;
		}
		else if (argc == i + 3)
		{
			t_txBeacon = (atoi(argv[i+1])!=0);
			t_BO = atoi(argv[i+2]);
			t_SO = 3;
		}
		else
		{
			t_txBeacon = (atoi(argv[i+1])!=0);
			t_BO = atoi(argv[i+2]);
			t_SO = atoi(argv[i+3]);
		}
		startPANCoord(t_isCT,t_txBeacon,t_BO,t_SO,true);
	}
	else if ((strcmp(argv[2], "startDevice") == 0)
	||       (strcmp(argv[2], "startCTDevice") == 0))
	{
		i = 2;
		t_isCT = (strcmp(argv[i], "startCTDevice") == 0);
#ifndef ZigBeeIF
		t_isCT = false;
#endif
		if (argc == i + 1)
		{
			t_isFFD = true;
			t_assoPermit = true;
			t_txBeacon = false;
			t_BO = 3;
			t_SO = 3;
		}
		else if (argc == i + 2)
		{
			t_isFFD = (atoi(argv[i+1])!=0);
			t_assoPermit = true;
			t_txBeacon = false;
			t_BO = 3;
			t_SO = 3;
		}
		else if (argc == i + 3)
		{
			t_isFFD = (atoi(argv[i+1])!=0);
			t_assoPermit = (atoi(argv[i+2])!=0);
			t_txBeacon = false;
			t_BO = 3;
			t_SO = 3;
		}

		else if (argc == i + 4)
		{
			t_isFFD = (atoi(argv[i+1])!=0);
			t_assoPermit = (atoi(argv[i+2])!=0);
			t_txBeacon = (atoi(argv[i+3])!=0);
			t_BO = 3;
			t_SO = 3;
		}
		else if (argc == i + 5)
		{
			t_isFFD = (atoi(argv[i+1])!=0);
			t_assoPermit = (atoi(argv[i+2])!=0);
			t_txBeacon = (atoi(argv[i+3])!=0);
			t_BO = atoi(argv[i+4]);
			t_SO = 3;
		}
		else
		{
			t_isFFD = (atoi(argv[i+1])!=0);
			t_assoPermit = (atoi(argv[i+2])!=0);
			t_txBeacon = (atoi(argv[i+3])!=0);
			t_BO = atoi(argv[i+4]);
			t_SO = atoi(argv[i+5]);
		}
		if (!t_isFFD)
			t_assoPermit = false;
		startDevice(t_isCT,t_isFFD,t_assoPermit,t_txBeacon,t_BO,t_SO,true);
	}
	else if (strcmp(argv[2], "startBeacon") == 0)
	{
		if (argc == 3)
		{
			t_BO = 3;
			t_SO = 3;
		}
		else if (argc == 4)
		{
			t_BO = atoi(argv[3]);
			t_SO = 3;
		}
		else
		{
			t_BO = atoi(argv[3]);
			t_SO = atoi(argv[4]);
		}
		mac->phy->PLME_GET_request(phyCurrentChannel);
		mac->MLME_START_request(mac->mpib.macPANId,mac->tmp_ppib.phyCurrentChannel,t_BO,t_SO,mac->isPANCoor,false,false,false);
	}
	else if (strcmp(argv[2], "stopBeacon") == 0)
	{
		mac->phy->PLME_GET_request(phyCurrentChannel);
		mac->MLME_START_request(mac->mpib.macPANId,mac->tmp_ppib.phyCurrentChannel,15,15,mac->isPANCoor,false,false,false);
	}

	return TCL_OK;
}

#ifdef ZigBeeIF
void SSCS802_15_4::assertZBR(void)
{
	if (!zbr)
		zbr = getZBRLink(mac->index_);
	assert(zbr);
	zbr->dRate = mac->phy->getRate('d');
}

int SSCS802_15_4::RNType(void)
{
	if (!t_isCT)
		return 1;

	assertZBR();

	return zbr->RNType;
}

void SSCS802_15_4::setGetClusTreePara(char setGet,Packet *p)
{
	hdr_lrwpan *wph = HDR_LRWPAN(p);

	if (!t_isCT)
		return;

	assertZBR();
	if (setGet == 's')
	{
		wph->clusTreeDepth = zbr->myDepth;
		wph->clusTreeParentNodeID = zbr->myNodeID;
	}
	else
	{
		rt_myDepth = wph->clusTreeDepth + 1;
		rt_myParentNodeID = wph->clusTreeParentNodeID;
	}
}
#endif

// End of file: p802_15_4sscs.cc
