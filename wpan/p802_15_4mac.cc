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

// File:  p802_15_4mac.cc
// Mode:  C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t

// $Header: /cvsroot/nsnam/ns-2/wpan/p802_15_4mac.cc,v 1.9 2011/10/02 22:32:35 tom_henderson Exp $

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


#include "p802_15_4pkt.h"
#include "p802_15_4mac.h"
#include "p802_15_4const.h"
#include "p802_15_4csmaca.h"
#include "p802_15_4sscs.h"
#include "p802_15_4trace.h"
#include "p802_15_4fail.h"
#include "p802_15_4nam.h"

bool Mac802_15_4::verbose = false;
UINT_8 Mac802_15_4::txOption = 0x00;	//0x02=GTS; 0x04=Indirect; 0x00=Direct (only for 802.15.4-unaware upper layer app. packet)
bool Mac802_15_4::ack4data = true;
UINT_8 Mac802_15_4::callBack = 1;	//0=no call back; 1=call back for failures; 2=call back for failures and successes
UINT_32 Mac802_15_4::DBG_UID = 0;

static MAC_PIB MPIB =
{
	def_macAckWaitDuration,		def_macAssociationPermit,
	def_macAutoRequest,		def_macBattLifeExt,
	def_macBattLifeExtPeriods,	def_macBeaconPayload,
	def_macBeaconPayloadLength,	def_macBeaconOrder,
	def_macBeaconTxTime,		0/*def_macBSN*/,
	def_macCoordExtendedAddress,	def_macCoordShortAddress,
	0/*def_macDSN*/,		def_macGTSPermit,
	def_macMaxCSMABackoffs,		def_macMinBE,
	def_macPANId,			def_macPromiscuousMode,
	def_macRxOnWhenIdle,		def_macShortAddress,
	def_macSuperframeOrder,		def_macTransactionPersistenceTime,
	def_macACLEntryDescriptorSet,	def_macACLEntryDescriptorSetSize,
	def_macDefaultSecurity,		def_macACLDefaultSecurityMaterialLength,
	def_macDefaultSecurityMaterial,	def_macDefaultSecuritySuite,
	def_macSecurityMode
};

void Mac802_15_4Handler::handle(Event* )
{
	nullEvent.uid_ = 0;
	if (type == macTxBcnCmdDataHType)
		mac->txBcnCmdDataHandler();
	else if (type == macIFSHType)
		mac->IFSHandler();
	else if (type == macBackoffBoundType)
		mac->backoffBoundHandler();
	else	
		assert(0);
}

int hdr_lrwpan::offset_;
static class LRWPANHeaderClass : public PacketHeaderClass
{
	public:
		LRWPANHeaderClass() : PacketHeaderClass("PacketHeader/LRWPAN",
				sizeof(hdr_lrwpan))
	{
		bind_offset(&hdr_lrwpan::offset_);
	}
} class_hdr_lrwpan;

static class Mac802_15_4Class : public TclClass
{
	public:
		Mac802_15_4Class() : TclClass("Mac/802_15_4") {}
		TclObject* create(int, const char*const*)
		{
			return (new Mac802_15_4(&MPIB));
		}
		virtual void bind(void);
		virtual int method(int argc, const char*const* argv);
} class_mac802_15_4;

void Mac802_15_4Class::bind(void)
{
	TclClass::bind();
	add_method("wpanCmd");
	add_method("wpanNam");
}

int Mac802_15_4Class::method(int ac, const char*const* av)
{
	//Available methods:
	//	------------------------------------------------------------------------------------------
	//	bool Mac802_15_4::verbose; (Tcl command: Mac/802_15_4 wpanCmd verbose [on/off]
	//	UINT_8 Mac802_15_4::txOption; (Tcl command: Mac/802_15_4 wpanCmd txOption [2/4/0]
	//	bool Mac802_15_4::ack4data; (Tcl command: Mac/802_15_4 wpanCmd ack4data [on/off]
	//	UINT_8 Mac802_15_4::callBack; (Tcl command: Mac/802_15_4 wpanCmd callBack [0/1/2]
	//	link down; (Tcl command: Mac/802_15_4 wpanCmd link-down <src> <dst>
	//	link up; (Tcl command: Mac/802_15_4 wpanCmd link-up <src> <dst>
	//	------------------------------------------------------------------------------------------
	//	bool Nam802_15_4::Nam_Status; (Tcl command: Mac/802_15_4 wpanNam namStatus [on/off]
	//	bool Nam802_15_4::emHandling; (Tcl command: Mac/802_15_4 wpanNam emHandling [on/off]
	//	char *Nam802_15_4::def_PANCoor_clr; (Tcl command: Mac/802_15_4 wpanNam PANCoorClr [clrName]
	//	char *Nam802_15_4::def_Coor_clr; (Tcl command: Mac/802_15_4 wpanNam CoorClr [clrName]
	//	char *Nam802_15_4::def_Dev_clr; (Tcl command: Mac/802_15_4 wpanNam DevClr [clrName]
	//	char *Nam802_15_4::def_ColFlash_clr; (Tcl command: Mac/802_15_4 wpanNam ColFlashClr [clrName]
	//	char *Nam802_15_4::def_NodeFail_clr; (Tcl command: Mac/802_15_4 wpanNam NodeFailClr [clrName]
	//	playback rate; (Tcl command: Mac/802_15_4 wpanNam PlaybackRate [step_in_ms]
	//	flow colors; (Tcl command: Mac/802_15_4 wpanNam FlowClr [-p <packet_type_name>] [-s <src>] [-d <dst>] [-c <clrName>]
	//	------------------------------------------------------------------------------------------

	Tcl& tcl = Tcl::instance();
	int argc = ac -	2;
	const char*const* argv = av + 2;

	if (strcmp(argv[1], "wpanCmd") == 0)
	{
		if (argc == 3)   
		{
			if (strcmp(argv[2], "verbose") == 0)
			{
				if (!Mac802_15_4::verbose)
					tcl.result("off");
				else
					tcl.result("on");
				return (TCL_OK);
			}
			else if (strcmp(argv[2], "txOption") == 0)
			{
				if (Mac802_15_4::txOption == 0x02)
					tcl.result("GTS");
				else if (Mac802_15_4::txOption == 0x04)
					tcl.result("Indirect");
				else
					tcl.result("Direct");
				return (TCL_OK);
			}
			else if (strcmp(argv[2], "ack4data") == 0)
			{
				if (!Mac802_15_4::ack4data)
					tcl.result("off");
				else
					tcl.result("on");
				return (TCL_OK);
			}
			else if (strcmp(argv[2], "callBack") == 0)
			{
				tcl.resultf("%u",Mac802_15_4::callBack);
				return (TCL_OK);
			}
		}
		else if (argc == 4)
		{
			if (strcmp(argv[2], "verbose") == 0)
			{
				if (strcmp(argv[3], "on") == 0)
					Mac802_15_4::verbose = true;
				else
					Mac802_15_4::verbose = false;
				return (TCL_OK);
			}
			else if (strcmp(argv[2], "txOption") == 0)
			{
				Mac802_15_4::txOption = atoi(argv[3]);
				return (TCL_OK);
			}
			else if (strcmp(argv[2], "ack4data") == 0)
			{
				if (strcmp(argv[3], "on") == 0)
					Mac802_15_4::ack4data = true;
				else
					Mac802_15_4::ack4data = false;
				return (TCL_OK);
			}
			else if (strcmp(argv[2], "callBack") == 0)
			{
				Mac802_15_4::callBack = atoi(argv[3]);
				return (TCL_OK);
			}
		}
		else if  (argc == 5)
		{
			if (strcmp(argv[2], "link-down") == 0)
			{
				chkAddLFailLink(atoi(argv[3]),atoi(argv[4]));
				return (TCL_OK);
			}
			else if (strcmp(argv[2], "link-up") == 0)
			{
				updateLFailLink(fl_oper_del,atoi(argv[3]),atoi(argv[4]));
				return (TCL_OK);
			}
		}
	}

	if (strcmp(argv[1], "wpanNam") == 0)
	{
		if (strcmp(argv[2], "namStatus") == 0)
		{
			if (argc == 3)
			{
				if (!Nam802_15_4::Nam_Status)
					tcl.result("off");
				else
					tcl.result("on");
			}
			else if (argc == 4)
			{
				if (strcmp(argv[3], "on") == 0)
					Nam802_15_4::Nam_Status = true;
				else
					Nam802_15_4::Nam_Status = false;
			}
			return (TCL_OK);
		}
		else if (strcmp(argv[2], "emHandling") == 0)
		{
			if (argc == 3)
			{
				if (!Nam802_15_4::emHandling)
					tcl.result("off");
				else
					tcl.result("on");
			}
			else if (argc == 4)
			{
				if (strcmp(argv[3], "on") == 0)
					Nam802_15_4::emHandling = true;
				else
					Nam802_15_4::emHandling = false;
			}
			return (TCL_OK);
		}
		else if (strcmp(argv[2], "PANCoorClr") == 0)
		{
			if (argc == 3)
				tcl.result(Nam802_15_4::def_PANCoor_clr);
			else if (argc >= 4)
			{
				strncpy(Nam802_15_4::def_PANCoor_clr,argv[3],20);
				Nam802_15_4::def_PANCoor_clr[20] = 0;
			}
			return (TCL_OK);
		}
		else if (strcmp(argv[2], "CoorClr") == 0)
		{
			if (argc == 3)
				tcl.result(Nam802_15_4::def_Coor_clr);
			else if (argc >= 4)
			{
				strncpy(Nam802_15_4::def_Coor_clr,argv[3],20);
				Nam802_15_4::def_Coor_clr[20] = 0;
			}
			return (TCL_OK);
		}
		else if (strcmp(argv[2], "DevClr") == 0)
		{
			if (argc == 3)
				tcl.result(Nam802_15_4::def_Dev_clr);
			else if (argc >= 4)
			{
				strncpy(Nam802_15_4::def_Dev_clr,argv[3],20);
				Nam802_15_4::def_Dev_clr[20] = 0;
			}
			return (TCL_OK);
		}
		else if (strcmp(argv[2], "ColFlashClr") == 0)
		{
			if (argc == 3)
				tcl.result(Nam802_15_4::def_ColFlash_clr);
			else if (argc >= 4)
			{
				strncpy(Nam802_15_4::def_ColFlash_clr,argv[3],20);
				Nam802_15_4::def_ColFlash_clr[20] = 0;
			}
			return (TCL_OK);
		}
		else if (strcmp(argv[2], "NodeFailClr") == 0)
		{
			if (argc == 3)
				tcl.result(Nam802_15_4::def_NodeFail_clr);
			else if (argc >= 4)
			{
				strncpy(Nam802_15_4::def_NodeFail_clr,argv[3],20);
				Nam802_15_4::def_NodeFail_clr[20] = 0;
			}
			return (TCL_OK);
		}
		else if (strcmp(argv[2], "PlaybackRate") == 0)
		{
			if (argc == 3)
				tcl.result("??");
			else if (argc >= 4)
				Nam802_15_4::changePlaybackRate(CURRENT_TIME,argv[3]);
			return (TCL_OK);
		}
		else if (strcmp(argv[2], "FlowClr") == 0)
		{
			int i,lp,src,dst;
			char pName[21],cName[21];
			ATTRIBUTELINK *attr;

			src = -2;
			dst = -2;
			strcpy(pName,packet_info.name(PT_NTYPE));
			lp = (argc - 3) / 2;
			for (i=0;i<lp;i++)
			{
				if (strcmp(argv[i*2+3],"-p") == 0)
				{
					strncpy(pName,argv[i*2+4],20);
					pName[20] = 0;
				}
				else if (strcmp(argv[i*2+3],"-s") == 0)
					src = atoi(argv[i*2+4]);
				else if (strcmp(argv[i*2+3],"-d") == 0)
					dst = atoi(argv[i*2+4]);
				else if (strcmp(argv[i*2+3],"-c") == 0)
				{
					strncpy(cName,argv[i*2+4],20);
					cName[20] = 0;
				}
			}
			i = chkAddAttrLink(nam_pktName2Type(pName),cName,src,dst);
			if (i == 1)		//already exist
			{
				attr = findAttrLink(nam_pktName2Type(pName),src,dst);
				if (strcmp(attr->color,cName) != 0)	//color changed
				{
					strncpy(attr->color,cName,20);
					attr->color[20] = 0;
					Nam802_15_4::flowAttribute(attr->attribute,attr->color);
				}

			}
			else if (i == 0)	//added into the link
			{
				attr = findAttrLink(nam_pktName2Type(pName),src,dst);
				Nam802_15_4::flowAttribute(attr->attribute,attr->color);
			}
			return (TCL_OK);
		}
	}

	return TclClass::method(ac, av);
}

Mac802_15_4::Mac802_15_4(MAC_PIB *mp) : Mac(),
	txCmdDataH(this,macTxBcnCmdDataHType),
	IFSH(this,macIFSHType),
	backoffBoundH(this,macBackoffBoundType)
{
	capability.cap = 0xc1;		//alterPANCoor = true
	//FFD = true
	//mainsPower = false
	//recvOnWhenIdle = false
	//secuCapable = false
	//alloShortAddr = true
	capability.parse();
	aExtendedAddress = index_;
	oneMoreBeacon = false;
	isPANCoor = false;
	inTransmission = false;
	mpib = *mp;
	mpib.macBSN = Random::random() % 0x100;
	mpib.macDSN = Random::random() % 0x100;
	macBeaconOrder2 = 15;
	macSuperframeOrder2 = def_macBeaconOrder;
	macBeaconOrder3 = 15;
	macSuperframeOrder3 = def_macBeaconOrder;
	if (mpib.macBeaconOrder == 15)		//non-beacon mode
		mpib.macRxOnWhenIdle = true;	//default is false, but should be true in non-beacon mode
	numLostBeacons = 0;
	phy = NULL;
	txOverT = new macTxOverTimer(this);
	assert(txOverT);
	txT = new macTxTimer(this);
	assert(txT);
	extractT = new macExtractTimer(this);
	assert(extractT);
	assoRspWaitT = new macAssoRspWaitTimer(this);
	assert(assoRspWaitT);
	dataWaitT = new macDataWaitTimer(this);
	assert(dataWaitT);
	rxEnableT = new macRxEnableTimer(this);
	assert(rxEnableT);
	scanT = new macScanTimer(this);
	assert(scanT);
	bcnTxT = new macBeaconTxTimer(this);
	assert(bcnTxT);
	bcnRxT = new macBeaconRxTimer(this);
	assert(bcnRxT);
	bcnSearchT = new macBeaconSearchTimer(this);
	assert(bcnSearchT);
	wakeupT = new macWakeupTimer(this); // 2.31 change: Wake up timer
	assert(wakeupT);  // 2.31 change: Wake up timer
	sscs = new SSCS802_15_4(this);
	assert(sscs);
	nam = new Nam802_15_4((isPANCoor)?Nam802_15_4::def_PANCoor_clr:"black","black",this);
	assert(nam);

	chkAddMacLink(index_,this);

	init();
}

Mac802_15_4::~Mac802_15_4()
{
	/*for some reason,this function sometimes is called with <index_> beyond the scope (ns2 bug?)
	  delete txOverT;
	  delete txT;
	  delete extractT;
	  delete assoRspWaitT;
	  delete dataWaitT;
	  delete rxEnableT;
	  delete scanT;
	  delete bcnTxT;
	  delete bcnRxT;
	  delete bcnSearchT;
	  delete csmaca;
	  delete sscs;
	  delete nam;
	  */
}

void Mac802_15_4::init(bool reset)
{
	secuBeacon = false;
	beaconWaiting = false;
	txBeacon = 0;
	txAck = 0;
	txBcnCmd = 0;
	txBcnCmd2 = 0;
	txData = 0;
	rxData = 0;
	rxCmd = 0;

	if (reset)
	{
		emptyHListLink(&hlistBLink1,&hlistBLink2);
		emptyHListLink(&hlistDLink1,&hlistDLink2);
		emptyDeviceLink(&deviceLink1,&deviceLink2);
		emptyTransacLink(&transacLink1,&transacLink2);
	}
	else
	{
		hlistBLink1 = NULL;
		hlistBLink2 = NULL;
		hlistDLink1 = NULL;
		hlistDLink2 = NULL;
		deviceLink1 = NULL;
		deviceLink2 = NULL;
		transacLink1 = NULL;
		transacLink2 = NULL;
	}

	taskP.init();
}

void Mac802_15_4::PD_DATA_confirm(PHYenum status)
{
	inTransmission = false;
	if (txOverT->busy())
		txOverT->stop();
	if (backoffStatus == 1)
		backoffStatus = 0;		
	if (status == p_SUCCESS)
	{
		dispatch(status,__FUNCTION__);
	}
	else if (txPkt == txBeacon)
	{
		beaconWaiting = false;
		Packet::free(txBeacon);
		txBeacon = 0;

	}
	else if (txPkt == txAck)		
	{
		Packet::free(txAck);
		txAck = 0;

	}
	else	//RX_ON/TRX_OFF -- possible if the transmisstion is terminated by a FORCE_TRX_OFF or change of channel, or due to energy depletion
	{}	//nothing to do -- it is the process that terminated the transmisstion to provide a way to resume the transmission
}

void Mac802_15_4::PLME_CCA_confirm(PHYenum status)
{
	if (taskP.taskStatus(TP_CCA_csmaca))
	{
		taskP.taskStatus(TP_CCA_csmaca) = false;
		csmaca->CCA_confirm(status);
	}
}

void Mac802_15_4::PLME_ED_confirm(PHYenum status,UINT_8 EnergyLevel)
{
	energyLevel = EnergyLevel;
	dispatch(status,__FUNCTION__);
}

void Mac802_15_4::PLME_GET_confirm(PHYenum status,PPIBAenum PIBAttribute,PHY_PIB *PIBAttributeValue)
{
	if (status == p_SUCCESS)
		switch(PIBAttribute)
		{
			case phyCurrentChannel:
				tmp_ppib.phyCurrentChannel = PIBAttributeValue->phyCurrentChannel;
				break;
			case phyChannelsSupported:
				tmp_ppib.phyChannelsSupported = PIBAttributeValue->phyChannelsSupported;
				break;
			case phyTransmitPower:
				tmp_ppib.phyTransmitPower = PIBAttributeValue->phyTransmitPower;
				break;
			case phyCCAMode:
				tmp_ppib.phyCCAMode = PIBAttributeValue->phyCCAMode;
				break;
			default:
				break;
		}
}

void Mac802_15_4::PLME_SET_TRX_STATE_confirm(PHYenum status)
{
	//hdr_lrwpan *wph;
	//FrameCtrl frmCtrl;
	double delay;

	if (status == p_SUCCESS) status = trx_state_req;

	if (backoffStatus == 99)
	{
		if (trx_state_req == p_RX_ON)
		{
			if (taskP.taskStatus(TP_RX_ON_csmaca))
			{
				taskP.taskStatus(TP_RX_ON_csmaca) = false;
				csmaca->RX_ON_confirm(status);
			}
		}
	}
	else
		dispatch(status,__FUNCTION__,trx_state_req);

	if (status != p_TX_ON) return;

	//transmit the packet
	if (beaconWaiting)
	{
		/* to synchronize better, we don't transmit the beacon here
#ifdef DEBUG802_15_4
fprintf(stdout,"[%s::%s][%f](node %d) transmit BEACON to %d: SN = %d, uid = %d, mac_uid = %ld\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,p802_15_4macDA(txBeacon),HDR_LRWPAN(txBeacon)->MHR_BDSN,HDR_CMN(txBeacon)->uid(),HDR_LRWPAN(txBeacon)->uid);
#endif
if (!taskP.taskStatus(TP_mlme_start_request))	//not first beacon
assert((!txAck)&&(!txCsmaca));		//all tasks should be done before next beacon
txPkt = txBeacon;
HDR_CMN(txBeacon)->direction() = hdr_cmn::DOWN;
sendDown(txBeacon->refcopy(),this);
*/
	}
	else if (txAck)
	{
		//although no CSMA-CA required for the transmission of ack., 
		//but we still need to locate the backoff period boundary if beacon enabled
		//(refer to page 157, line 25-31)
		if ((mpib.macBeaconOrder == 15)&&(macBeaconOrder2 == 15))	//non-beacon enabled
			delay = 0.0;
		else								//beacon enabled
			delay  = locateBoundary((p802_15_4macDA(txAck) == mpib.macCoordShortAddress),0.0);
		if (delay == 0.0)
			backoffBoundHandler();
		else
			Scheduler::instance().schedule(&backoffBoundH, &(backoffBoundH.nullEvent), delay);
	}
	else
		transmitCmdData();
}

void Mac802_15_4::PLME_SET_confirm(PHYenum status,PPIBAenum PIBAttribute)
{
	if ((PIBAttribute == phyCurrentChannel)&&(status == p_SUCCESS))
		dispatch(status,__FUNCTION__);
}

void Mac802_15_4::MCPS_DATA_request(UINT_8 SrcAddrMode,UINT_16 SrcPANId,IE3ADDR SrcAddr,
		UINT_8 DstAddrMode,UINT_16 DstPANId,IE3ADDR DstAddr,
		UINT_8 msduLength,Packet *msdu,UINT_8 msduHandle,UINT_8 TxOptions)
{
	mcps_data_request(SrcAddrMode,SrcPANId,SrcAddr,DstAddrMode,DstPANId,DstAddr,msduLength,msdu,msduHandle,TxOptions,true);
}

void Mac802_15_4::MCPS_DATA_indication(UINT_8 SrcAddrMode,UINT_16 SrcPANId,IE3ADDR SrcAddr,
		UINT_8 DstAddrMode,UINT_16 DstPANId,IE3ADDR DstAddr,
		UINT_8 msduLength,Packet *msdu,UINT_8 mpduLinkQuality,
		bool SecurityUse,UINT_8 ACLEntry)
{
	HDR_CMN(msdu)->num_forwards() += 1;

	if (HDR_LRWPAN(msdu)->msduHandle != 0)	//from peer SSCS
	{
		log(msdu->refcopy());
		sscs->MCPS_DATA_indication(SrcAddrMode,SrcPANId,SrcAddr,DstAddrMode,DstPANId,DstAddr,msduLength,msdu,mpduLinkQuality,SecurityUse,ACLEntry);
	}
	else
		uptarget_->recv(msdu,(Handler*) 0);
}

void Mac802_15_4::MCPS_PURGE_request(UINT_8 msduHandle)
{

	int i;
	MACenum t_status;

	i = updateTransacLinkByPktOrHandle(tr_oper_del,&transacLink1,&transacLink2,NULL,msduHandle);
	t_status = (i == 0)?m_SUCCESS:m_INVALID_HANDLE;
	sscs->MCPS_PURGE_confirm(msduHandle,t_status);
}

void Mac802_15_4::MLME_ASSOCIATE_request(UINT_8 LogicalChannel,UINT_8 CoordAddrMode,UINT_16 CoordPANId,IE3ADDR CoordAddress,
		UINT_8 CapabilityInformation,bool SecurityEnable)
{
	mlme_associate_request(LogicalChannel,CoordAddrMode,CoordPANId,CoordAddress,CapabilityInformation,SecurityEnable,true);
}

void Mac802_15_4::MLME_ASSOCIATE_response(IE3ADDR DeviceAddress,UINT_16 AssocShortAddress,MACenum status,bool SecurityEnable)
{
	mlme_associate_response(DeviceAddress,AssocShortAddress,status,SecurityEnable,true);
}

void Mac802_15_4::MLME_DISASSOCIATE_request(IE3ADDR DeviceAddress,UINT_8 DisassociateReason,bool SecurityEnable)
{
	mlme_disassociate_request(DeviceAddress,DisassociateReason,SecurityEnable,true);
}

void Mac802_15_4::MLME_DISASSOCIATE_indication(IE3ADDR ,UINT_8 ,bool ,UINT_8 )
{
}

void Mac802_15_4::MLME_GET_request(MPIBAenum PIBAttribute)
{
	MACenum t_status;

	switch(PIBAttribute)
	{
		case macAckWaitDuration:
		case macAssociationPermit:
		case macAutoRequest:
		case macBattLifeExt:
		case macBattLifeExtPeriods:
		case macBeaconPayload:
		case macBeaconPayloadLength:
		case macBeaconOrder:		
		case macBeaconTxTime:
		case macBSN:
		case macCoordExtendedAddress:
		case macCoordShortAddress:
		case macDSN:
		case macGTSPermit:
		case macMaxCSMABackoffs:
		case macMinBE:
		case macPANId:
		case macPromiscuousMode:
		case macRxOnWhenIdle:
		case macShortAddress:
		case macSuperframeOrder:
		case macTransactionPersistenceTime:
		case macACLEntryDescriptorSet:
		case macACLEntryDescriptorSetSize:
		case macDefaultSecurity:
		case macACLDefaultSecurityMaterialLength:
		case macDefaultSecurityMaterial:
		case macDefaultSecuritySuite:
		case macSecurityMode:
			t_status = m_SUCCESS;
			break;
		default:
			t_status = m_UNSUPPORTED_ATTRIBUTE;
			break;
	}
	sscs->MLME_GET_confirm(t_status,PIBAttribute,&mpib);
}

void Mac802_15_4::MLME_GTS_request(UINT_8, bool)
{
}

void Mac802_15_4::MLME_GTS_confirm(UINT_8, MACenum)
{
}

void Mac802_15_4::MLME_GTS_indication(UINT_16, UINT_8, bool, UINT_8)
{
}

void Mac802_15_4::MLME_ORPHAN_response(IE3ADDR OrphanAddress,UINT_16 ShortAddress,bool AssociatedMember,bool SecurityEnable)
{
	mlme_orphan_response(OrphanAddress,ShortAddress,AssociatedMember,SecurityEnable,true);
}

void Mac802_15_4::MLME_RESET_request(bool SetDefaultPIB)
{
	mlme_reset_request(SetDefaultPIB,true);
}

void Mac802_15_4::MLME_RX_ENABLE_request(bool DeferPermit,UINT_32 RxOnTime,UINT_32 RxOnDuration)
{
	mlme_rx_enable_request(DeferPermit,RxOnTime,RxOnDuration,true);
}

void Mac802_15_4::MLME_SCAN_request(UINT_8 ScanType,UINT_32 ScanChannels,UINT_8 ScanDuration)
{
	mlme_scan_request(ScanType,ScanChannels,ScanDuration,true);
}

void Mac802_15_4::MLME_SET_request(MPIBAenum PIBAttribute,MAC_PIB *PIBAttributeValue)
{
	PHYenum p_state;
	MACenum t_status;

	t_status = m_SUCCESS;
	switch(PIBAttribute)
	{
		case macAckWaitDuration:
			{
				phy->PLME_GET_request(phyCurrentChannel);	//value will be returned in tmp_ppib
				if (((tmp_ppib.phyCurrentChannel <= 10)&&(PIBAttributeValue->macAckWaitDuration != 120))
						|| ((tmp_ppib.phyCurrentChannel > 10)&&(PIBAttributeValue->macAckWaitDuration != 54)))
				{
					t_status = m_INVALID_PARAMETER;
				}
				else
				{
					mpib.macAckWaitDuration = PIBAttributeValue->macAckWaitDuration;
				}
				break;
			}
		case macAssociationPermit:
			{
				mpib.macAssociationPermit = PIBAttributeValue->macAssociationPermit;
				break;
			}
		case macAutoRequest:
			{
				mpib.macAutoRequest = PIBAttributeValue->macAutoRequest;
				break;
			}
		case macBattLifeExt:
			{
				mpib.macBattLifeExt = PIBAttributeValue->macBattLifeExt;
				break;
			}
		case macBattLifeExtPeriods:
			{
				phy->PLME_GET_request(phyCurrentChannel);	//value will be returned in tmp_ppib
				if (((tmp_ppib.phyCurrentChannel <= 10)&&(PIBAttributeValue->macBattLifeExtPeriods != 8))
						|| ((tmp_ppib.phyCurrentChannel > 10)&&(PIBAttributeValue->macBattLifeExtPeriods != 6)))
				{
					t_status = m_INVALID_PARAMETER;
				}
				else
				{
					mpib.macBattLifeExtPeriods = PIBAttributeValue->macBattLifeExtPeriods;
				}
				break;
			}
		case macBeaconPayload:
			{
				//<macBeaconPayloadLength> should be set first
				memcpy(mpib.macBeaconPayload,PIBAttributeValue->macBeaconPayload,mpib.macBeaconPayloadLength);
				break;
			}
		case macBeaconPayloadLength:
			{
				if (PIBAttributeValue->macBeaconPayloadLength > aMaxBeaconPayloadLength)
				{
					t_status = m_INVALID_PARAMETER;
				}
				else
				{
					mpib.macBeaconPayloadLength = PIBAttributeValue->macBeaconPayloadLength;
				}
				break;
			}
		case macBeaconOrder:
			{
				if (PIBAttributeValue->macBeaconOrder > 15)
				{
					t_status = m_INVALID_PARAMETER;
				}
				else
				{
					mpib.macBeaconOrder = PIBAttributeValue->macBeaconOrder;
				}
				break;
			}
		case macBeaconTxTime:
			{
				mpib.macBeaconTxTime = PIBAttributeValue->macBeaconTxTime;
				break;
			}
		case macBSN:
			{
				mpib.macBSN = PIBAttributeValue->macBSN;
				break;
			}
		case macCoordExtendedAddress:
			{
				mpib.macCoordExtendedAddress = PIBAttributeValue->macCoordExtendedAddress;
				break;
			}
		case macCoordShortAddress:
			{
				mpib.macCoordShortAddress = PIBAttributeValue->macCoordShortAddress;
				break;
			}
		case macDSN:
			{
				mpib.macDSN = PIBAttributeValue->macDSN;
				break;
			}
		case macGTSPermit:
			{
				mpib.macGTSPermit = PIBAttributeValue->macGTSPermit;
				break;
			}
		case macMaxCSMABackoffs:
			{
				if (PIBAttributeValue->macMaxCSMABackoffs > 5)
				{
					t_status = m_INVALID_PARAMETER;
				}
				else
				{
					mpib.macMaxCSMABackoffs = PIBAttributeValue->macMaxCSMABackoffs;
				}
				break;
			}
		case macMinBE:
			{
				if (PIBAttributeValue->macMinBE > 3)
				{
					t_status = m_INVALID_PARAMETER;
				}
				else
				{
					mpib.macMinBE = PIBAttributeValue->macMinBE;
				}
				break;
			}
		case macPANId:
			{
				mpib.macPANId = PIBAttributeValue->macPANId;
				break;
			}
		case macPromiscuousMode:
			{
				mpib.macPromiscuousMode = PIBAttributeValue->macPromiscuousMode;
				//some other operations (refer to sec. 7.5.6.6)
				mpib.macRxOnWhenIdle = PIBAttributeValue->macPromiscuousMode;
				p_state = mpib.macRxOnWhenIdle?p_RX_ON:p_TRX_OFF;
				phy->PLME_SET_TRX_STATE_request(p_state);
				break;
			}
		case macRxOnWhenIdle:
			{
				mpib.macRxOnWhenIdle = PIBAttributeValue->macRxOnWhenIdle;
				break;
			}
		case macShortAddress:
			{
				mpib.macShortAddress = PIBAttributeValue->macShortAddress;
				break;
			}
		case macSuperframeOrder:
			{
				if (PIBAttributeValue->macSuperframeOrder > 15)
				{
					t_status = m_INVALID_PARAMETER;
				}
				else
				{
					mpib.macSuperframeOrder = PIBAttributeValue->macSuperframeOrder;
				}
				break;
			}
		case macTransactionPersistenceTime:
			{
				mpib.macTransactionPersistenceTime = PIBAttributeValue->macTransactionPersistenceTime;
				break;
			}
		case macACLEntryDescriptorSet:
		case macACLEntryDescriptorSetSize:
		case macDefaultSecurity:
		case macACLDefaultSecurityMaterialLength:
		case macDefaultSecurityMaterial:
		case macDefaultSecuritySuite:
		case macSecurityMode:
			{
				break;		//currently security ignored in simulation
			}
		default:
			{
				t_status = m_UNSUPPORTED_ATTRIBUTE;
				break;
			}
	}
	sscs->MLME_SET_confirm(t_status,PIBAttribute);
}

void Mac802_15_4::MLME_START_request(UINT_16 PANId,UINT_8 LogicalChannel,UINT_8 BeaconOrder,
		UINT_8 SuperframeOrder,bool PANCoordinator,bool BatteryLifeExtension,
		bool CoordRealignment,bool SecurityEnable)
{
	mlme_start_request(PANId,LogicalChannel,BeaconOrder,SuperframeOrder,PANCoordinator,BatteryLifeExtension,CoordRealignment,SecurityEnable,true);
}

void Mac802_15_4::MLME_SYNC_request(UINT_8 LogicalChannel, bool TrackBeacon)
{
	mlme_sync_request(LogicalChannel,TrackBeacon,true);
}

void Mac802_15_4::MLME_POLL_request(UINT_8 CoordAddrMode,UINT_16 CoordPANId,IE3ADDR CoordAddress,bool SecurityEnable)
{
	mlme_poll_request(CoordAddrMode,CoordPANId,CoordAddress,SecurityEnable,false,true);
}

inline int Mac802_15_4::hdr_dst(char* hdr, int dst)
{
	return p802_15_4hdr_dst(hdr,dst);
}

inline int Mac802_15_4::hdr_src(char* hdr, int src)
{
	return p802_15_4hdr_src(hdr,src);
}

inline int Mac802_15_4::hdr_type(char* hdr, UINT_16 type)
{
	return p802_15_4hdr_type(hdr,type);
}

void Mac802_15_4::recv(Packet *p, Handler *h)
{
	hdr_lrwpan* wph = HDR_LRWPAN(p);
	hdr_cmn *ch = HDR_CMN(p);
	bool noAck;
	int i;
	UINT_8 txop;
	FrameCtrl frmCtrl;
	SuperframeSpec t_sfSpec;

	if (!Nam802_15_4::emStatus)
		Nam802_15_4::emStatus = (netif_->node()->energy_model()?true:false);	//is there a better place to do this?

	if(ch->direction() == hdr_cmn::DOWN)	//outgoing packet
	{
#ifdef DEBUG802_15_4
		fprintf(stdout,"[%s::%s][%f](node %d) outgoing pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = ??, size = %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),ch->size());
#endif
		//-- Notes for power-saving:
		//   It turns out to be very difficult to apply sleeping model in 802.15.4.
		//   First, a node shouldn't go to sleep if peer2peer transmission mode is
		//   used. Non-peer2peer means that a node only communicates with its parent 
		//   and/or children, which requests that pure tree routing be used.
		//   Second, even pure tree routing is used, a node can only go to sleep
		//   if it satisfies both the sleeping condition as a parent (to its children)
		//   and that as a child (to its parent) in a multi-hop environment. To 
		//   satisfy both conditions requires efficient scheduling scheme.
		//   Since ns2, by default, treats the power consumption in idle mode same 
		//   as that in sleeping mode, it makes no difference at this moment whether
		//   we set sleeping mode or not.

		//wake up the node if it is in sleep mode (only for legacy applications)
		EnergyModel *em = netif_->node()->energy_model();
		if (em)
		{
			if (em->energy() <= 0)
			{
				drop(p,"ENE");
				return;
			}
#ifdef SHUTDOWN
			phy->wakeupNode(1);
#endif
		}
		/* SSCS should call MCPS_DATA_request() directly
		   if (from SSCS)
		   {
		   MCPS_DATA_request(wph->SrcAddrMode,wph->SrcPANId,wph->SrcAddr,
		   wph->DstAddrMode,wph->DstPANId,wph->DstAddr,
		   ch->size(),p,wph->msduHandle,wph->TxOptions);
		   }
		   else	//802.15.4-unaware upper layer app. packet
		   */
		{
			callback_ = h;
			if (p802_15_4macDA(p) == (nsaddr_t)MAC_BROADCAST)
			{
				txop = 0;
			}
			else
			{
				if (Mac802_15_4::ack4data)
				{
					txop = TxOp_Acked;
				}
				else
				{
					txop = 0;
				}
				txop |= Mac802_15_4::txOption;
			}
			wph->msduHandle = 0;
			MCPS_DATA_request(0,0,0,defFrmCtrl_AddrMode16,mpib.macPANId,p802_15_4macDA(p),ch->size(),p,0,txop);	//direct transmission w/o security
		}
		return;
	}
	else	//incoming packet
	{
#ifdef DEBUG802_15_4
		fprintf(stdout,"[%s::%s][%f](node %d) incoming pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),wph->uid,ch->size());
#endif
		resetCounter(p802_15_4macSA(p));
		//if during ED scan, discard all frames received over the PHY layer data service (sec. 7.5.2.1.1)
		//if during Active/Passive scan, discard all frames received over the PHY layer data service that are not beacon frames (sec. 7.5.2.1.2/7.5.2.1.3)
		//if during Orphan scan, discard all frames received over the PHY layer data service that are not coordinator realignment command frames (sec. 7.5.2.1.4)
		frmCtrl.FrmCtrl = wph->MHR_FrmCtrl;
		frmCtrl.parse();
		if (taskP.taskStatus(TP_mlme_scan_request))
		{
			if (taskP.mlme_scan_request_ScanType == 0x00)		//ED scan
			{
#ifdef DEBUG802_15_4
				fprintf(stdout,"[D][ED][%s::%s::%d][%f](node %d) dropping pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,__LINE__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),wph->uid,ch->size());
#endif
				drop(p,"ED");
				return;
			}
			else if (((taskP.mlme_scan_request_ScanType == 0x01)	//Active scan
						||(taskP.mlme_scan_request_ScanType == 0x02))	//Passive scan
					&& (frmCtrl.frmType != defFrmCtrl_Type_Beacon))
			{
#ifdef DEBUG802_15_4
				fprintf(stdout,"[D][APS][%s::%s::%d][%f](node %d) dropping pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,__LINE__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),wph->uid,ch->size());
#endif
				drop(p,"APS");
				return;
			}
			else if ((taskP.mlme_scan_request_ScanType == 0x03)	//Orphan scan
					&& ((frmCtrl.frmType != defFrmCtrl_Type_MacCmd)||(wph->MSDU_CmdType != 0x08)))
			{
#ifdef DEBUG802_15_4
				fprintf(stdout,"[D][OPH][%s::%s::%d][%f](node %d) dropping pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,__LINE__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),wph->uid,ch->size());
#endif
				drop(p,"OPH");
				return;
			}
		}

		//drop the packet if corrupted
		if (ch->error())
		{
#ifdef DEBUG802_15_4
			fprintf(stdout,"[D][ERR][%s::%s::%d][%f](node %d) dropping pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,__LINE__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),wph->uid,ch->size());
#endif
			drop(p,"ERR");
			return;
		}
		//drop the packet if the link quality is too bad (basically, collisions)
		if ((wph->rxTotPower-p->txinfo_.RxPr) > 0.0)
			if (p->txinfo_.RxPr/(wph->rxTotPower-p->txinfo_.RxPr) < p->txinfo_.CPThresh)
			{
#ifdef DEBUG802_15_4
				fprintf(stdout,"[D][LQI][%s::%s::%d][%f](node %d) dropping pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,__LINE__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),wph->uid,ch->size());
#endif
				if (!wph->colFlag)	
					nam->flashNodeColor(CURRENT_TIME);
				drop(p,"LQI");
				return;
			}

		if (frmCtrl.frmType == defFrmCtrl_Type_Beacon)
		{
			t_sfSpec.SuperSpec = wph->MSDU_SuperSpec;
			t_sfSpec.parse();
			if (t_sfSpec.BO != 15)
			{
				//update superframe specification
				sfSpec3 = t_sfSpec;
				//calculate the time when we received the first bit of the beacon
				macBcnOtherRxTime = (CURRENT_TIME - phy->trxTime(p)) * phy->getRate('s');
				//update beacon order and superframe order
				macBeaconOrder3 = sfSpec3.BO;
				macSuperframeOrder3 = sfSpec3.SO;
			}
		}

		//---perform filtering (refer to sec. 7.5.6.2)---
		//drop the packet if FCS is not correct (ignored in simulation)
		if (ch->ptype() == PT_MAC)	//perform further filtering only if it is an 802.15.4 packet
			if (!mpib.macPromiscuousMode)	//perform further filtering only if the PAN is currently not in promiscuous mode
			{
				//check packet type
				if ((frmCtrl.frmType != defFrmCtrl_Type_Beacon)
						&&(frmCtrl.frmType != defFrmCtrl_Type_Data)
						&&(frmCtrl.frmType != defFrmCtrl_Type_Ack)
						&&(frmCtrl.frmType != defFrmCtrl_Type_MacCmd))
				{
#ifdef DEBUG802_15_4
					fprintf(stdout,"[D][TYPE][%s::%s::%d][%f](node %d) dropping pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,__LINE__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),wph->uid,ch->size());
#endif
					drop(p,"TYPE");
					return;
				}
				//check source PAN ID for beacon frame
				if ((frmCtrl.frmType == defFrmCtrl_Type_Beacon)
						&&(mpib.macPANId != 0xffff)
						&&(wph->MHR_SrcAddrInfo.panID != mpib.macPANId))
				{
#ifdef DEBUG802_15_4
					fprintf(stdout,"[D][PAN][%s::%s::%d][%f](node %d) dropping pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,__LINE__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),wph->uid,ch->size());
#endif
					drop(p,"PAN");
					return;
				}
				//check dest. PAN ID if it is included
				if ((frmCtrl.dstAddrMode == defFrmCtrl_AddrMode16)
						||(frmCtrl.dstAddrMode == defFrmCtrl_AddrMode64))
					if ((wph->MHR_DstAddrInfo.panID != 0xffff)
							&&(wph->MHR_DstAddrInfo.panID != mpib.macPANId))
					{
#ifdef DEBUG802_15_4
						fprintf(stdout,"[D][PAN][%s::%s::%d][%f](node %d) dropping pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,__LINE__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),wph->uid,ch->size());
#endif
						drop(p,"PAN");
						return;
					}
				//check dest. address if it is included
				if (frmCtrl.dstAddrMode == defFrmCtrl_AddrMode16)
				{
					if ((wph->MHR_DstAddrInfo.addr_16 != 0xffff)
							&& (wph->MHR_DstAddrInfo.addr_16 != mpib.macShortAddress))
					{
#ifdef DEBUG802_15_4
						fprintf(stdout,"[D][ADR][%s::%s::%d][%f](node %d) dropping pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,__LINE__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),wph->uid,ch->size());
#endif
						drop(p,"ADR");
						return;
					}

				}
				else if (frmCtrl.dstAddrMode == defFrmCtrl_AddrMode64)
				{
					if (wph->MHR_DstAddrInfo.addr_64 != aExtendedAddress)
					{
#ifdef DEBUG802_15_4
						fprintf(stdout,"[D][ADR][%s::%s::%d][%f](node %d) dropping pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,__LINE__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),wph->uid,ch->size());
#endif
						drop(p,"ADR");
						return;
					}
				}
				//check for Data/MacCmd frame only w/ source address
				if ((frmCtrl.frmType == defFrmCtrl_Type_Data)
						||(frmCtrl.frmType == defFrmCtrl_Type_MacCmd))
					if (frmCtrl.dstAddrMode == defFrmCtrl_AddrModeNone)
					{
						if (((!capability.FFD)||(numberDeviceLink(&deviceLink1) == 0))	//I am not a coordinator
								||(wph->MHR_SrcAddrInfo.panID != mpib.macPANId))
						{
#ifdef DEBUG802_15_4
							fprintf(stdout,"[D][PAN][%s::%s::%d][%f](node %d) dropping pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,__LINE__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),wph->uid,ch->size());
#endif
							drop(p,"PAN");
							return;
						}
					}
				//we need to add one more filter for supporting multi-hop beacon enabled mode (not in the draft)
				if (frmCtrl.frmType == defFrmCtrl_Type_Beacon)
					if (wph->MHR_DstAddrInfo.panID != 0xffff)
						if ((mpib.macCoordExtendedAddress != wph->MHR_SrcAddrInfo.addr_64)	//ok even for short address (in simulation)
								&&  (mpib.macCoordExtendedAddress != def_macCoordExtendedAddress))
						{
#ifdef DEBUG802_15_4
							fprintf(stdout,"[D][COO][%s::%s::%d][%f](node %d) dropping pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,__LINE__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),wph->uid,ch->size());
#endif
							drop(p,"COO");
							return;
						}
			}	//---filtering done---

		//perform security task if required (ignored in simulation)

		//send an acknowledgement if needed (no matter this is a duplicated packet or not)
		if ((frmCtrl.frmType == defFrmCtrl_Type_Data)
				||(frmCtrl.frmType == defFrmCtrl_Type_MacCmd))
		{
			if (frmCtrl.ackReq)	//acknowledgement required
			{
				//association request command will be ignored under following cases
				if ((frmCtrl.frmType == defFrmCtrl_Type_MacCmd)
						&& (wph->MSDU_CmdType == 0x01))
					if ((!capability.FFD)			//not an FFD
							|| (mpib.macShortAddress == 0xffff)	//not yet joined any PAN
							|| (!macAssociationPermit))		//association not permitted
					{
						Packet::free(p);
						return;
					}

				noAck = false;
				if (frmCtrl.frmType == defFrmCtrl_Type_MacCmd)
					if ((rxCmd)||(txBcnCmd))
						noAck = true;
				if (!noAck)
				{
					constructACK(p);
					//stop CSMA-CA if it is pending (it will be restored after the transmission of ACK)
					if (backoffStatus == 99)
					{
						backoffStatus = 0;
						csmaca->cancel();
					}
					plme_set_trx_state_request(p_TX_ON);
				}
			}
			else
			{
				resetTRX();
			}
		}

		if (frmCtrl.frmType == defFrmCtrl_Type_MacCmd)
		{
			if ((rxCmd)||(txBcnCmd))
			{
#ifdef DEBUG802_15_4
				{
					fprintf(stdout,"[D][BSY][%s::%s::%d][%f](node %d) dropping pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,__LINE__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),wph->uid,ch->size());
					if (rxCmd)
						fprintf(stdout,"\trxCmd pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",wpan_pName(rxCmd),p802_15_4macSA(rxCmd),p802_15_4macDA(rxCmd),HDR_CMN(rxCmd)->uid(),HDR_LRWPAN(rxCmd)->uid,HDR_CMN(rxCmd)->size());
					if (txBcnCmd)
						fprintf(stdout,"\ttxBcnCmd pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",wpan_pName(txBcnCmd),p802_15_4macSA(txBcnCmd),p802_15_4macDA(txBcnCmd),HDR_CMN(txBcnCmd)->uid(),HDR_LRWPAN(txBcnCmd)->uid,HDR_CMN(txBcnCmd)->size());
				}
#endif
				drop(p,"BSY");
				return;
			}
		}

		if (frmCtrl.frmType == defFrmCtrl_Type_Data)
		{
			if (rxData)
			{
#ifdef DEBUG802_15_4
				fprintf(stdout,"[D][BSY][%s::%s::%d][%f](node %d) dropping pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,__LINE__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),wph->uid,ch->size());
#endif
				drop(p,"BSY");
				return;
			}
		}

		//check duplication -- must be performed AFTER all drop's
		if (frmCtrl.frmType == defFrmCtrl_Type_Beacon)
			i = chkAddUpdHListLink(&hlistBLink1,&hlistBLink2,p802_15_4macSA(p),wph->MHR_BDSN);
		else if (frmCtrl.frmType != defFrmCtrl_Type_Ack)
			i = chkAddUpdHListLink(&hlistDLink1,&hlistDLink2,p802_15_4macSA(p),wph->MHR_BDSN);
		else	//Acknowledgement
		{
			assert(txPkt);
			if (wph->MHR_BDSN != HDR_LRWPAN(txPkt)->MHR_BDSN)
				i = 2;
			else i = 0;
		}
		if (i == 2)
		{
#ifdef DEBUG802_15_4
			fprintf(stdout,"[D][DUP][%s::%s][%f](node %d) dropping duplicated packet: type = %s, from = %d, uid = %d, mac_uid = %ld, size = %d, SN = %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),ch->uid(),wph->uid,ch->size(),wph->MHR_BDSN);
#endif
			drop(p,"DUP");
			return;
		}

		//handle the beacon packet
		if (frmCtrl.frmType == defFrmCtrl_Type_Beacon)
			recvBeacon(p);

		//handle the ack. packet
		else if (frmCtrl.frmType == defFrmCtrl_Type_Ack)
			recvAck(p);

		//handle the command packet
		else if (frmCtrl.frmType == defFrmCtrl_Type_MacCmd)
			recvCommand(p);

		//handle the data packet
		else if (frmCtrl.frmType == defFrmCtrl_Type_Data)
		{
			recvData(p);
		}
	}
}

void Mac802_15_4::recvBeacon(Packet *p)
{
	hdr_lrwpan* wph = HDR_LRWPAN(p);
	FrameCtrl frmCtrl;
	PendAddrSpec pendSpec;
	bool pending;
	double txtime;
	UINT_8 ifs;
	int i;
	//update superframe specification
	sfSpec2.SuperSpec = wph->MSDU_SuperSpec;
	sfSpec2.parse();
#ifdef DEBUG802_15_4
	hdr_cmn* ch = HDR_CMN(p);
	fprintf(stdout,"[%s::%s][%f](node %d) M_BEACON [BO:%d][SO:%d] received: from = %d, uid = %d, mac_uid = %ld, size = %d, SN = %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,sfSpec2.BO,sfSpec2.SO,p802_15_4macSA(p),ch->uid(),wph->uid,ch->size(),wph->MHR_BDSN);
#endif
	//calculate the time when we received the first bit of the beacon
	txtime = phy->trxTime(p);

	/* Linux floating number compatibility
	   macBcnRxTime = (CURRENT_TIME - txtime) * phy->getRate('s');
	   */
	{
		double tmpf;
		tmpf = CURRENT_TIME - txtime;
		macBcnRxTime = tmpf * phy->getRate('s');
	}

	//calculate <beaconPeriods2>
	if (HDR_CMN(p)->size() <= aMaxSIFSFrameSize)
		ifs = aMinSIFSPeriod;
	else
		ifs = aMinLIFSPeriod;

	/* Linux floating number compatibility
	   beaconPeriods2 = (UINT_8)((txtime * phy->getRate('s') + ifs) / aUnitBackoffPeriod);
	   */
	double tmpf;
	tmpf = txtime * phy->getRate('s');
	tmpf += ifs;
	beaconPeriods2 = (UINT_8)(tmpf / aUnitBackoffPeriod);

	/* Linux floating number compatibility
	   if (fmod((txtime * phy->getRate('s')+ ifs) ,aUnitBackoffPeriod) > 0.0)
	   */
	if (fmod(tmpf ,aUnitBackoffPeriod) > 0.0)
		beaconPeriods2++;
	//update PAN descriptor
	frmCtrl.FrmCtrl = wph->MHR_FrmCtrl;
	frmCtrl.parse();
	panDes2.CoordAddrMode = frmCtrl.srcAddrMode;
	panDes2.CoordPANId = wph->MHR_SrcAddrInfo.panID;
	panDes2.CoordAddress_64 = wph->MHR_SrcAddrInfo.addr_64;		//ok even it is a 16-bit address
	panDes2.LogicalChannel = wph->phyCurrentChannel;
	panDes2.SuperframeSpec = wph->MSDU_SuperSpec;
	gtsSpec2.fields = wph->MSDU_GTSFields;
	gtsSpec2.parse();
	panDes2.GTSPermit = gtsSpec2.permit;
	panDes2.LinkQuality = wph->ppduLinkQuality;
	panDes2.TimeStamp = (UINT_32)macBcnRxTime;
	panDes2.SecurityUse = wph->SecurityUse;
	panDes2.ACLEntry = wph->ACLEntry;
	panDes2.SecurityFailure = false;				//ignored in simulation
	panDes2.clusTreeDepth = wph->clusTreeDepth;
	//handle active and passive channel scans
	if ((taskP.taskStatus(TP_mlme_scan_request))
			|| (taskP.taskStatus(TP_mlme_rx_enable_request)))
	{
		rxBeacon = p;
		dispatch(p_SUCCESS,__FUNCTION__);
	}

	if ((mpib.macPANId == 0xffff)
			|| (mpib.macPANId != panDes2.CoordPANId)
			|| (taskP.taskStatus(TP_mlme_associate_request)))
	{
		Packet::free(p);
		return;		
	}
	numLostBeacons = 0;
	nam->flashNodeMark(CURRENT_TIME);
	macBeaconOrder2 = sfSpec2.BO;
	macSuperframeOrder2 = sfSpec2.SO;
	//populate <macCoordShortAddress> if needed
	if (mpib.macCoordShortAddress == def_macCoordShortAddress)
		if (frmCtrl.srcAddrMode == defFrmCtrl_AddrMode16)
			mpib.macCoordShortAddress = wph->MHR_SrcAddrInfo.addr_16;
	dispatch(p_SUCCESS,__FUNCTION__);
	//resume extraction timer if needed
	extractT->resume();
	//CSMA-CA may be waiting for the new beacon
	if (wph->MHR_SrcAddrInfo.panID == mpib.macPANId)
		if (backoffStatus == 99)
			csmaca->newBeacon('r');

	//check if need to notify the upper layer
	if ((!mpib.macAutoRequest)||(wph->MSDU_PayloadLen > 0))
		sscs->MLME_BEACON_NOTIFY_indication(wph->MHR_BDSN,&panDes2,wph->MSDU_PendAddrFields.spec,wph->MSDU_PendAddrFields.addrList,wph->MSDU_PayloadLen,wph->MSDU_Payload);
	if (mpib.macAutoRequest)
	{
		//handle the pending packet
		pendSpec.fields = wph->MSDU_PendAddrFields;
		pendSpec.parse();
		pending = false;
		for (i=0;i<pendSpec.numShortAddr;i++)
		{
			if (pendSpec.fields.addrList[i] == mpib.macShortAddress)
			{
				pending = true;
				break;
			}
		}
		if (!pending)
			for (i=0;i<pendSpec.numExtendedAddr;i++)
			{
				if (pendSpec.fields.addrList[pendSpec.numShortAddr + i] == aExtendedAddress)
				{
					pending = true;
					break;
				}
			}

		if (pending)
		{
			//frmCtrl.FrmCtrl = wph->MHR_FrmCtrl;
			//frmCtrl.parse();
			mlme_poll_request(frmCtrl.srcAddrMode,wph->MHR_SrcAddrInfo.panID,wph->MHR_SrcAddrInfo.addr_64,capability.secuCapable,true,true);
		}

		else
		{
#ifdef SHUTDOWN
			if ((backoffStatus!=99) && ((!capability.FFD)||(numberDeviceLink(&deviceLink1) == 0)) && (NOW>phy->T_sleep_)) { //2.31 change: added this if statement to put the node to sleep after beacon reception if the node is not attempting to tx a pkt
				phy->putNodeToSleep();
			}
#endif
		}
		log(p);
	}
}

void Mac802_15_4::recvAck(Packet *p)
{
	hdr_lrwpan *wph;
	FrameCtrl frmCtrl;

	wph = HDR_LRWPAN(p);
#ifdef DEBUG802_15_4
	hdr_cmn *ch = HDR_CMN(p);
	fprintf(stdout,"[%s::%s][%f](node %d) M_ACK received: from = %d, SN = %d, uid = %d, mac_uid = %ld\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,p802_15_4macSA(p),wph->MHR_BDSN,ch->uid(),wph->uid);
#endif
	if ((!txBcnCmd)&&(!txBcnCmd2)&&(!txData))
	{
		Packet::free(p);
		return;
	}

	//check the sequence number in the ack. to see if it matches that in the <txPkt>
	if (wph->MHR_BDSN != HDR_LRWPAN(txPkt)->MHR_BDSN)
	{
		Packet::free(p);
		return;
	}

	if (txT->busy())
		txT->stop();
	else	
	{
#ifdef DEBUG802_15_4
		fprintf(stdout,"[%s::%s][%f](node %d) LATE ACK received: from = %d, SN = %d, uid = %d, mac_uid = %ld\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,p802_15_4macSA(p),wph->MHR_BDSN,ch->uid(),wph->uid);
#endif
		//only handle late ack. for data packet
		if (txPkt != txData)
		{
			Packet::free(p);
			return;
		}

		if (backoffStatus == 99)
		{
			backoffStatus = 0;
			csmaca->cancel();
		}
	}

	//set pending flag for data polling
	if (txPkt == txBcnCmd2)
		if ((taskP.taskStatus(TP_mlme_poll_request))
				&& (strcmp(taskP.taskFrFunc(TP_mlme_poll_request),__FUNCTION__) == 0))
		{
			frmCtrl.FrmCtrl = wph->MHR_FrmCtrl;
			frmCtrl.parse();
			taskP.mlme_poll_request_pending = frmCtrl.frmPending;
		}

	dispatch(p_SUCCESS,__FUNCTION__);

	log(p);
}

void Mac802_15_4::recvCommand(Packet *p)
{
	hdr_lrwpan* wph;
	FrameCtrl frmCtrl;
	bool ackReq;

#ifdef DEBUG802_15_4
	wph = HDR_LRWPAN(p);
	hdr_cmn* ch = HDR_CMN(p);
	fprintf(stdout,"[%s::%s][%f](node %d) %s received: from = %d, uid = %d, mac_uid = %ld, size = %d, SN = %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),ch->uid(),wph->uid,ch->size(),wph->MHR_BDSN);
#endif
	ackReq = false;
	switch(HDR_LRWPAN(p)->MSDU_CmdType)
	{
		case 0x01:	//Association request
			//recv() is in charge of sending ack.
			//MLME-ASSOCIATE.indication() will be passed to upper layer after the transmission of ack.
			assert(rxCmd == 0);
			rxCmd = p;
			ackReq = true;
			break;
		case 0x02:	//Association response
			//recv() is in charge of sending ack.
			//MLME-ASSOCIATE.confirm will be passed to upper layer after the transmission of ack.
			assert(rxCmd == 0);
			rxCmd = p;
			ackReq = true;
			wph = HDR_LRWPAN(p);
			rt_myNodeID = *((UINT_16 *)wph->MSDU_Payload);
#ifdef ZigBeeIF
			sscs->setGetClusTreePara('g',p);
#endif
			break;
		case 0x03:	//Disassociation notification
			break;
		case 0x04:	//Data request
			//recv() is in charge of sending ack.
			//pending packet will be sent after the transmission of ack.
			assert(rxCmd == 0);
			rxCmd = p;
			ackReq = true;
			break;
		case 0x05:	//PAN ID conflict notification
			break;
		case 0x06:	//Orphan notification
			wph = HDR_LRWPAN(p);
			sscs->MLME_ORPHAN_indication(wph->MHR_SrcAddrInfo.addr_64,false,0);
			break;
		case 0x07:	//Beacon request
			if (capability.FFD						//I am an FFD
					&& (mpib.macAssociationPermit)					//association permitted
					&& (mpib.macShortAddress != 0xffff)				//allow to send beacons
					&& (mpib.macBeaconOrder == 15))				//non-beacon enabled mode
			{
				//send a beacon using unslotted CSMA-CA
#ifdef DEBUG802_15_4
				fprintf(stdout,"[%s::%s][%f](node %d) before alloc txBcnCmd:\n\t\ttxBeacon\t= %ld\n\t\ttxAck   \t= %ld\n\t\ttxBcnCmd\t= %ld\n\t\ttxBcnCmd2\t= %ld\n\t\ttxData  \t= %ld\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,txBeacon,txAck,txBcnCmd,txBcnCmd2,txData);
#endif
				assert(!txBcnCmd);
				txBcnCmd = Packet::alloc();
				if (!txBcnCmd) break;
				wph = HDR_LRWPAN(txBcnCmd);
				frmCtrl.FrmCtrl = 0;
				frmCtrl.setFrmType(defFrmCtrl_Type_Beacon);
				frmCtrl.setSecu(secuBeacon);
				frmCtrl.setFrmPending(false);
				frmCtrl.setAckReq(false);
				frmCtrl.setDstAddrMode(defFrmCtrl_AddrModeNone);
				if (mpib.macShortAddress == 0xfffe)
				{
					frmCtrl.setSrcAddrMode(defFrmCtrl_AddrMode64);
					wph->MHR_SrcAddrInfo.panID = mpib.macPANId;
					wph->MHR_SrcAddrInfo.addr_64 = aExtendedAddress;
				}
				else
				{
					frmCtrl.setSrcAddrMode(defFrmCtrl_AddrMode16);
					wph->MHR_SrcAddrInfo.panID = mpib.macPANId;
					wph->MHR_SrcAddrInfo.addr_16 = mpib.macShortAddress;
				}
				sfSpec.SuperSpec = 0;
				sfSpec.setBO(15);
				sfSpec.setBLE(mpib.macBattLifeExt);
				sfSpec.setPANCoor(isPANCoor);
				sfSpec.setAssoPmt(mpib.macAssociationPermit);
				wph->MSDU_GTSFields.spec = 0;
				wph->MSDU_PendAddrFields.spec = 0;
				wph->MSDU_PayloadLen = 0;
#ifdef ZigBeeIF
				sscs->setGetClusTreePara('s',txBcnCmd);
#endif
				constructMPDU(4,txBcnCmd,frmCtrl.FrmCtrl,mpib.macBSN++,wph->MHR_DstAddrInfo,wph->MHR_SrcAddrInfo,sfSpec.SuperSpec,0,0);
				hdr_dst((char *)HDR_MAC(txBcnCmd),p802_15_4macSA(p));
				hdr_src((char *)HDR_MAC(txBcnCmd),index_);
				HDR_CMN(txBcnCmd)->ptype() = PT_MAC;
				//for trace
				HDR_CMN(txBcnCmd)->next_hop_ = p802_15_4macDA(txBcnCmd);		//nam needs the nex_hop information
				p802_15_4hdrBeacon(txBcnCmd);
				csmacaBegin('c');
			}
			break;
		case 0x08:	//Coordinator realignment
			wph = HDR_LRWPAN(p);
			frmCtrl.FrmCtrl = wph->MHR_FrmCtrl;
			frmCtrl.parse();
			if (frmCtrl.dstAddrMode == defFrmCtrl_AddrMode64)		//directed to an orphan device
			{
				//recv() is in charge of sending ack.
				//further handling continues after the transmission of ack.
				assert(rxCmd == 0);
				rxCmd = p;
				ackReq = true;
			}
			else								//broadcasted realignment command
				if ((wph->MHR_SrcAddrInfo.addr_64 == macCoordExtendedAddress)
						&& (wph->MHR_SrcAddrInfo.panID == mpib.macPANId))
				{
					//no specification in the draft as how to handle this packet, so use our discretion
					mpib.macPANId = *((UINT_16 *)wph->MSDU_Payload);
					mpib.macCoordShortAddress = *((UINT_16 *)(wph->MSDU_Payload + 2));
					tmp_ppib.phyCurrentChannel = wph->MSDU_Payload[4];
					phy->PLME_SET_request(phyCurrentChannel,&tmp_ppib);
				}
			break;
		case 0x09:	//GTS request
			break;
		default:
			assert(0);
			break;
	}

	if (!ackReq)
		log(p);
	else	
		log(p->refcopy());
}

void Mac802_15_4::recvData(Packet *p)
{
	hdr_lrwpan* wph;
	hdr_cmn* ch;
	FrameCtrl frmCtrl;
	UINT_8 ifs;

	//pass the data packet to upper layer
	//(we need some time to process the packet -- so delay SIFS/LIFS symbols from now or after finishing sending the ack.)
	//(refer to Figure 60 for details of SIFS/LIFS)
	assert(rxData == 0);
	rxData = p;
	wph = HDR_LRWPAN(p);
	ch = HDR_CMN(p);
#ifdef DEBUG802_15_4
	fprintf(stdout,"[%s::%s][%f](node %d) DATA (%s) received: from = %d, uid = %d, mac_uid = %ld, size = %d, SN = %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),ch->uid(),wph->uid,ch->size(),wph->MHR_BDSN);
#endif
	frmCtrl.FrmCtrl = wph->MHR_FrmCtrl;
	frmCtrl.parse();
	rxDataTime = CURRENT_TIME;
	if (!frmCtrl.ackReq)
	{
		if (ch->size() <= aMaxSIFSFrameSize)
			ifs = aMinSIFSPeriod;
		else
			ifs = aMinLIFSPeriod;
		Scheduler::instance().schedule(&IFSH, &(IFSH.nullEvent), ifs/phy->getRate('s'));
	}
	//else	//schedule and dispatch after finishing ack. transmission
}

bool Mac802_15_4::toParent(Packet *p)
{
	hdr_lrwpan* wph = HDR_LRWPAN(p);
	FrameCtrl frmCtrl;

	frmCtrl.FrmCtrl = wph->MHR_FrmCtrl;
	frmCtrl.parse();
	if (((frmCtrl.dstAddrMode == defFrmCtrl_AddrMode16)&&(wph->MHR_DstAddrInfo.addr_16 == mpib.macCoordShortAddress))
			||  ((frmCtrl.dstAddrMode == defFrmCtrl_AddrMode64)&&(wph->MHR_DstAddrInfo.addr_64 == mpib.macCoordExtendedAddress)))
		return true;
	else
		return false;
}

double Mac802_15_4::locateBoundary(bool parent,double wtime)
{
	//In the case that a node acts as both a coordinator and a device, 
	//transmission of beacons is preferablly to be aligned with reception 
	//of beacons to achieve the best results -- but we cannot control this.
	//For example, the parent may originally work in non-beacon enabled mode
	//and later on begin to work in beacon enabled mode; the parent will
	//not align with the child since it is not supposed to handle the beacons
	//from the child.
	//So the alignment is specifically w.r.t. either transmission of beacons
	//(as a coordinator) or reception of beacons (as a device), but there is
	//no guarantee to satisfy both.

	int align;			
	double bcnTxRxTime,bPeriod;
	double newtime;

	if ((mpib.macBeaconOrder == 15)&&(macBeaconOrder2 == 15))
		return wtime;		

	if (parent)			
		align = (macBeaconOrder2 == 15)?1:2;
	else				
		align = (mpib.macBeaconOrder == 15)?2:1;

	bcnTxRxTime = (align == 1)?(macBcnTxTime / phy->getRate('s')):(macBcnRxTime / phy->getRate('s'));
	bPeriod = aUnitBackoffPeriod / phy->getRate('s');

	/* Linux floating number compatibility
	   newtime = fmod(CURRENT_TIME + wtime - bcnTxRxTime, bPeriod);
	   */
	{
		double tmpf;
		tmpf = CURRENT_TIME + wtime;
		tmpf -= bcnTxRxTime;
		newtime = fmod(tmpf, bPeriod);
	}

#ifdef DEBUG802_15_4
	//fprintf(stdout,"[%s::%s][%f](node %d) delay = bPeriod - fmod = %f - %f = %f\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,bPeriod,newtime,bPeriod - newtime);
#endif
	if(newtime > 0.000001) // 2.31 change
	{
		/* Linux floating number compatibility
		   newtime = wtime + (bPeriod - newtime);
		   */
		{
			double tmpf;
			tmpf = bPeriod - newtime;
			newtime = wtime + tmpf;
		}
	}
	else
		newtime = wtime;

	return newtime;
}

void Mac802_15_4::txOverHandler(void)
{
	assert(txPkt);
	PD_DATA_confirm(p_UNDEFINED);
}

void Mac802_15_4::txHandler(void)
{
	assert(txBcnCmd||txBcnCmd2||txData);

	Packet *p;
	UINT_8 t_numRetry;

	if (txBcnCmd) p = txBcnCmd;
	else if (txBcnCmd2) p = txBcnCmd2;
	else p = txData;

	if (txBcnCmd) t_numRetry = numBcnCmdRetry;
	else if (txBcnCmd2) t_numRetry = numBcnCmdRetry2;
	else t_numRetry = numDataRetry;
	t_numRetry++;
#ifdef DEBUG802_15_4
	hdr_lrwpan* wph = HDR_LRWPAN(p);
	hdr_cmn* ch = HDR_CMN(p);
	if (t_numRetry <= aMaxFrameRetries)
		fprintf(stdout,"[%s::%s][%f](node %d) No ACK - retransmitting: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),wph->uid,ch->size());
	else
		fprintf(stdout,"[%s::%s][%f](node %d) No ACK - giving up: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),wph->uid,ch->size());
#endif
	if (t_numRetry > aMaxFrameRetries)
		nam->flashLinkFail(CURRENT_TIME,p802_15_4macDA(p));

	dispatch(p_BUSY,__FUNCTION__);	//the status p_BUSY will be ignore
}

void Mac802_15_4::extractHandler(void)
{
	if (taskP.taskStatus(TP_mlme_associate_request))
		strcpy(taskP.taskFrFunc(TP_mlme_associate_request),__FUNCTION__);
	dispatch(p_BUSY,__FUNCTION__);
}

void Mac802_15_4::assoRspWaitHandler(void)
{
	dispatch(p_BUSY,__FUNCTION__);
}

void Mac802_15_4::dataWaitHandler(void)
{
	dispatch(p_BUSY,__FUNCTION__);
}

void Mac802_15_4::rxEnableHandler(void)
{
	dispatch(p_SUCCESS,__FUNCTION__);
}

void Mac802_15_4::scanHandler(void)
{
	if (taskP.mlme_scan_request_ScanType == 0x01)
		taskP.taskStep(TP_mlme_scan_request)++;		
	dispatch(p_SUCCESS,__FUNCTION__);
}

void Mac802_15_4::beaconTxHandler(bool forTX)
{
	hdr_lrwpan* wph;
	FrameCtrl frmCtrl;
	TRANSACLINK *tmp;
	int i;

	if ((mpib.macBeaconOrder != 15)		//beacon enabled
			|| (oneMoreBeacon))
	{
		if (forTX)
		{
			if (capability.FFD/*&&(numberDeviceLink(&deviceLink1) > 0)*/)
			{
				//enable the transmitter
				beaconWaiting = true;
				plme_set_trx_state_request(p_FORCE_TRX_OFF);	//finish your job before this!
				//assert(txAck == 0);	//It's not true, for the reason that packets can arrive 
				//at any time if the source is in non-beacon mode.
				//This could also happen if a device loses synchronization
				//with its coordinator, or if a coordinator changes beacon
				//order in the middle.
				if (txAck)
				{
#ifdef DEBUG802_15_4
					if (!updateDeviceLink(tr_oper_est, &deviceLink1, &deviceLink2, p802_15_4macDA(txAck)))	//this ACK is for my child
						fprintf(stdout,"[%f](node %d) outgoing ACK truncated by beacon: src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n", CURRENT_TIME,index_,p802_15_4macSA(txAck),p802_15_4macDA(txAck),HDR_CMN(txAck)->uid(),HDR_LRWPAN(txAck)->uid,HDR_CMN(txAck)->size());
#endif
					Packet::free(txAck);
					txAck = 0;
				}
				plme_set_trx_state_request(p_TX_ON);
			}
			else
				assert(0);
		}
		else
		{
			if (capability.FFD/*&&(numberDeviceLink(&deviceLink1) > 0)*/)	//send a beacon here
			{
				//beaconWaiting = false;				
				if ((taskP.taskStatus(TP_mlme_start_request))		
						&&  (mpib.macBeaconOrder != 15))
				{
					if (txAck||backoffStatus == 1)
					{
						beaconWaiting = false;
						bcnTxT->start();
						return;
					}
				}
#ifdef DEBUG802_15_4
				fprintf(stdout,"[%s::%s][%f](node %d) before alloc txBeacon:\n\t\ttxBeacon\t= %ld\n\t\ttxAck   \t= %ld\n\t\ttxBcnCmd\t= %ld\n\t\ttxBcnCmd2\t= %ld\n\t\ttxData  \t= %ld\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,txBeacon,txAck,txBcnCmd,txBcnCmd2,txData);
#endif
				if (updateNFailLink(fl_oper_est,index_) == 0)
				{
					if (txBeacon)
					{
						Packet::free(txBeacon);
						txBeacon = 0;
					}
					beaconWaiting = false;
					bcnTxT->start();
					return;
				}
				assert(!txBeacon);
				txBeacon = Packet::alloc();
				if (!txBeacon)
				{
					bcnTxT->start();		//try to restore the transmission of beacons next time
					return;
				}
				wph = HDR_LRWPAN(txBeacon);
				frmCtrl.FrmCtrl = 0;
				frmCtrl.setFrmType(defFrmCtrl_Type_Beacon);
				frmCtrl.setSecu(secuBeacon);
				frmCtrl.setAckReq(false);
				frmCtrl.setDstAddrMode(defFrmCtrl_AddrModeNone);
				if (mpib.macShortAddress == 0xfffe)
				{
					frmCtrl.setSrcAddrMode(defFrmCtrl_AddrMode64);
					wph->MHR_SrcAddrInfo.panID = mpib.macPANId;
					wph->MHR_SrcAddrInfo.addr_64 = aExtendedAddress;
				}
				else
				{
					frmCtrl.setSrcAddrMode(defFrmCtrl_AddrMode16);
					wph->MHR_SrcAddrInfo.panID = mpib.macPANId;
					wph->MHR_SrcAddrInfo.addr_16 = mpib.macShortAddress;
				}
				sfSpec.SuperSpec = 0;
				sfSpec.setBO(mpib.macBeaconOrder);
				sfSpec.setSO(mpib.macSuperframeOrder);
				sfSpec.setFinCAP(aNumSuperframeSlots - 1);		//TBD: may be less than <aNumSuperframeSlots> when considering GTS
				sfSpec.setBLE(mpib.macBattLifeExt);
				sfSpec.setPANCoor(isPANCoor);
				sfSpec.setAssoPmt(mpib.macAssociationPermit);
				//populate the GTS fields -- more TBD when considering GTS
				gtsSpec.fields.spec = 0;
				gtsSpec.setPermit(mpib.macGTSPermit);
				wph->MSDU_GTSFields = gtsSpec.fields;
				//--- populate the pending address list ---
				pendAddrSpec.numShortAddr = 0;
				pendAddrSpec.numExtendedAddr = 0;
				purgeTransacLink(&transacLink1,&transacLink2);
				tmp = transacLink1;
				i = 0;
				while (tmp != NULL)
				{
					if (tmp->pendAddrMode == defFrmCtrl_AddrMode16)
					{
						if (updateDeviceLink(tr_oper_est,&deviceLink1,&deviceLink2,tmp->pendAddr64) == 0)
							i = pendAddrSpec.addShortAddr(tmp->pendAddr16);		//duplicated address filtered out
					}
					else
					{
						if (updateDeviceLink(tr_oper_est,&deviceLink1,&deviceLink2,tmp->pendAddr64) == 0)
							i = pendAddrSpec.addExtendedAddr(tmp->pendAddr64);	//duplicated address filtered out
					}
					if (i >= 7) break;
					tmp = tmp->next;
				}
				pendAddrSpec.format();
				wph->MSDU_PendAddrFields = pendAddrSpec.fields;
				frmCtrl.setFrmPending(i>0);
				//To populate the beacon payload field, <macBeaconPayloadLength> and <macBeaconPayload>
				//should be set first, in that order (use primitive MLME_SET_request).
				wph->MSDU_PayloadLen = mpib.macBeaconPayloadLength;
				memcpy(wph->MSDU_Payload,mpib.macBeaconPayload,mpib.macBeaconPayloadLength);
				//-----------------------------------------
#ifdef ZigBeeIF
				sscs->setGetClusTreePara('s',txBeacon);
#endif
				constructMPDU(2 + gtsSpec.size() + pendAddrSpec.size() + mpib.macBeaconPayloadLength,txBeacon,frmCtrl.FrmCtrl,mpib.macBSN++,wph->MHR_DstAddrInfo,wph->MHR_SrcAddrInfo,sfSpec.SuperSpec,0,0);
				hdr_src((char *)HDR_MAC(txBeacon),index_);
				hdr_dst((char *)HDR_MAC(txBeacon),MAC_BROADCAST);
				HDR_CMN(txBeacon)->ptype() = PT_MAC;
				HDR_CMN(txBeacon)->next_hop_ = p802_15_4macDA(txBeacon);		//nam needs the nex_hop information
				p802_15_4hdrBeacon(txBeacon);
#ifdef DEBUG802_15_4
				fprintf(stdout,"[%s::%s][%f](node %d) transmit BEACON to %d: SN = %d, uid = %d, mac_uid = %ld\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,p802_15_4macDA(txBeacon),HDR_LRWPAN(txBeacon)->MHR_BDSN,HDR_CMN(txBeacon)->uid(),HDR_LRWPAN(txBeacon)->uid);
#endif
				txPkt = txBeacon;
				HDR_CMN(txBeacon)->direction() = hdr_cmn::DOWN;
				//nam->flashNodeMark(CURRENT_TIME);
				sendDown(txBeacon->refcopy(), this);
				mpib.macBeaconTxTime = (UINT_32)(CURRENT_TIME * phy->getRate('s'));
				macBcnTxTime = CURRENT_TIME * phy->getRate('s');	//double used for accuracy
				oneMoreBeacon = false;
			}
			else
			{
				assert(0);
			}
		}
	}
	bcnTxT->start();	//don't disable this even beacon not enabled (beacon may be temporarily disabled like in channel scan, but it will be enabled again)
}

void Mac802_15_4::beaconRxHandler(void)
{
	if (macBeaconOrder2 != 15)		//beacon enabled (do nothing if beacon not enabled)
	{
		if (txAck)
		{
			Packet::free(txAck);
			txAck = 0;
		}
		//enable the receiver
		plme_set_trx_state_request(p_RX_ON);
		if (taskP.mlme_sync_request_tracking)	
		{
			//a better way is using another timer to detect <numLostBeacons> right after the header of superframe
			if (numLostBeacons > aMaxLostBeacons)
			{
				char label[11];
				//label[0] = 0;		
				strcpy(label,"\" \"");
#ifdef ZigBeeIF
				if (sscs->t_isCT)
					sprintf(label,"\"%s\"",(sscs->RNType())?"+":"-");
#endif
				nam->changeLabel(CURRENT_TIME,label);
				changeNodeColor(CURRENT_TIME,Nam802_15_4::def_Node_clr);
				sscs->MLME_SYNC_LOSS_indication(m_BEACON_LOSS);
				numLostBeacons = 0;
			}
			else
			{
				numLostBeacons++;
				bcnRxT->start();
			}
		}
	}
}

void Mac802_15_4::beaconSearchHandler(void)
{
	dispatch(p_BUSY,__FUNCTION__);
}

void Mac802_15_4::isPanCoor(bool isPC)
{
	if (isPANCoor == isPC)
		return;

	if (isPC)
		changeNodeColor(CURRENT_TIME,Nam802_15_4::def_PANCoor_clr);
	else if (isPANCoor)
		changeNodeColor(CURRENT_TIME,
				(mpib.macPANId == 0xffff) ? Nam802_15_4::def_Node_clr :
				(mpib.macAssociationPermit) ? Nam802_15_4::def_Coor_clr : Nam802_15_4::def_Dev_clr);
	isPANCoor = isPC;
}

//-------------------------------------------------------------------------------------

void Mac802_15_4::set_trx_state_request(PHYenum state,const char *,const char *,int )
{
#ifdef DEBUG802_15_4
	fprintf(stdout,"[%s::%s][%f](node %d): %s request from [%s:%s:%d]\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,
			(state == p_RX_ON)?"RX_ON":
			(state == p_TX_ON)?"TX_ON":
			(state == p_TRX_OFF)?"TRX_OFF":
			(state == p_FORCE_TRX_OFF)?"FORCE_TRX_OFF":"???",
			frFile,frFunc,line);
#endif
	trx_state_req = state;
	phy->PLME_SET_TRX_STATE_request(state);
}

char *taskName[] = {"NONE",
	"MCPS-DATA.request",
	"MLME-ASSOCIATE.request",
	"MLME-ASSOCIATE.response",
	"MLME-DISASSOCIATE.request",
	"MLME-ORPHAN.response",
	"MLME-RESET.request",
	"MLME-RX-ENABLE.request",
	"MLME-SCAN.request",
	"MLME-START.request",
	"MLME-SYNC.request",
	"MLME-POLL.request",
	"CCA_csmaca",
	"RX_ON_csmaca"};
void Mac802_15_4::checkTaskOverflow(UINT_8 task)
{
	//Though we assume the upper layer should know what it is doing -- should send down requests one by one.
	//But we'd better check again (we have no control over upper layer and we don't know who is operating on the upper layer)
	if (taskP.taskStatus(task))
	{
		fprintf(stdout,"[%s::%s][%f](node %d) task overflow: %s\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,taskName[task]);
		exit(1);
	}
	else
	{
		taskP.taskStep(task) = 0;
		(taskP.taskFrFunc(task))[0] = 0;
	}
}

void Mac802_15_4::dispatch(PHYenum status,const char *frFunc,PHYenum req_state,MACenum mStatus)
{
	hdr_lrwpan *wph;
	hdr_cmn *ch;
	FrameCtrl frmCtrl;
	UINT_8 ifs;
	//int i;

	if (strcmp(frFunc,"csmacaCallBack") == 0)
	{
		if (txCsmaca == txBcnCmd2)
		{
			if (taskP.taskStatus(TP_mlme_scan_request)
					&& (strcmp(taskP.taskFrFunc(TP_mlme_scan_request),frFunc) == 0))
			{
				if ((taskP.mlme_scan_request_ScanType == 0x01)	//active scan
						||  (taskP.mlme_scan_request_ScanType == 0x03))	//orphan scan
					mlme_scan_request(taskP.mlme_scan_request_ScanType,taskP.mlme_scan_request_ScanChannels,taskP.mlme_scan_request_ScanDuration,false,status);
			}
			else if (taskP.taskStatus(TP_mlme_start_request)
					&& (strcmp(taskP.taskFrFunc(TP_mlme_start_request),frFunc) == 0))
				mlme_start_request(taskP.mlme_start_request_PANId,taskP.mlme_start_request_LogicalChannel,taskP.mlme_start_request_BeaconOrder,taskP.mlme_start_request_SuperframeOrder,taskP.mlme_start_request_PANCoordinator,taskP.mlme_start_request_BatteryLifeExtension,0,taskP.mlme_start_request_SecurityEnable,false,status);
			else if (taskP.taskStatus(TP_mlme_associate_request)
					&& (strcmp(taskP.taskFrFunc(TP_mlme_associate_request),frFunc) == 0))
				mlme_associate_request(0,0,0,0,0,taskP.mlme_associate_request_SecurityEnable,false,status);
			else if (taskP.taskStatus(TP_mlme_poll_request)
					&& (strcmp(taskP.taskFrFunc(TP_mlme_poll_request),frFunc) == 0))
				mlme_poll_request(taskP.mlme_poll_request_CoordAddrMode,taskP.mlme_poll_request_CoordPANId,taskP.mlme_poll_request_CoordAddress,taskP.mlme_poll_request_SecurityEnable,taskP.mlme_poll_request_autoRequest,false,status);
			else	//default handling for txBcnCmd2
			{
				if (status == p_IDLE)
					plme_set_trx_state_request(p_TX_ON);
				else
				{
					freePkt(txBcnCmd2);
					txBcnCmd2 = 0;
					csmacaResume();		//other packet may be waiting
				}
			}
		}
		else if (txCsmaca == txData)
		{
			assert(taskP.taskStatus(TP_mcps_data_request)
					&& (strcmp(taskP.taskFrFunc(TP_mcps_data_request),frFunc) == 0));

			if (taskP.mcps_data_request_TxOptions & TxOp_GTS)		//GTS transmission
			{
				;	//TBD
			}
			else if ((taskP.mcps_data_request_TxOptions & TxOp_Indirect)	//indirect transmission
					&& (capability.FFD&&(numberDeviceLink(&deviceLink1) > 0)))	//I am a coordinator
			{
				if (status != p_IDLE)
					mcps_data_request(0,0,0,0,0,0,0,0,0,taskP.mcps_data_request_TxOptions,false,p_BUSY,m_CHANNEL_ACCESS_FAILURE);
				else
				{
					strcpy(taskP.taskFrFunc(TP_mcps_data_request),"PD_DATA_confirm");
					plme_set_trx_state_request(p_TX_ON);
				}
			}
			else		//direct transmission: in this case, let mcps_data_request() take care of everything
				mcps_data_request(0,0,0,0,0,0,0,0,0,taskP.mcps_data_request_TxOptions,false,status);
		}
		else if (txCsmaca == txBcnCmd)	//default handling for txBcnCmd
		{
			wph = HDR_LRWPAN(txBcnCmd);
			frmCtrl.FrmCtrl = wph->MHR_FrmCtrl;
			frmCtrl.parse();
			if (status == p_IDLE)
			{
				if ((frmCtrl.frmType == defFrmCtrl_Type_MacCmd)		//command packet
						&& (wph->MSDU_CmdType == 0x02))				//association response packet
					strcpy(taskP.taskFrFunc(TP_mlme_associate_response),"PD_DATA_confirm");
				else if ((frmCtrl.frmType == defFrmCtrl_Type_MacCmd)	//command packet
						&& (wph->MSDU_CmdType == 0x08))				//coordinator realignment response packet
					strcpy(taskP.taskFrFunc(TP_mlme_orphan_response),"PD_DATA_confirm");
				plme_set_trx_state_request(p_TX_ON);
			}
			else
			{
				if ((frmCtrl.frmType == defFrmCtrl_Type_MacCmd)		//command packet
						&& (wph->MSDU_CmdType == 0x02))				//association response packet
					mlme_associate_response(taskP.mlme_associate_response_DeviceAddress,0,m_CHANNEL_ACCESS_FAILURE,0,false,p_BUSY);	//status returned in MACenum rather than in PHYenum
				else if ((frmCtrl.frmType == defFrmCtrl_Type_MacCmd)	//command packet
						&& (wph->MSDU_CmdType == 0x08))				//coordinator realignment response packet
					mlme_orphan_response(taskP.mlme_orphan_response_OrphanAddress,0,true,false,false,p_BUSY);
				else
				{
					freePkt(txBcnCmd);
					txBcnCmd = 0;
					csmacaResume();		//other packets may be waiting
				}
			}
		}
		//else		//may be purged from pending list
	}
	else if (strcmp(frFunc,"PD_DATA_confirm") == 0)
	{
		if (txPkt == txBeacon)
		{
			if (taskP.taskStatus(TP_mlme_start_request)
					&& (strcmp(taskP.taskFrFunc(TP_mlme_start_request),frFunc) == 0))
				mlme_start_request(taskP.mlme_start_request_PANId,taskP.mlme_start_request_LogicalChannel,taskP.mlme_start_request_BeaconOrder,taskP.mlme_start_request_SuperframeOrder,taskP.mlme_start_request_PANCoordinator,taskP.mlme_start_request_BatteryLifeExtension,0,taskP.mlme_start_request_SecurityEnable,false,status);
			else	//default handling
			{
				resetTRX();
				taskSuccess('b');
			}
		}
		else if (txPkt == txAck)
		{
			if (rxCmd)
			{
				ch = HDR_CMN(rxCmd);
				if (ch->size() <= aMaxSIFSFrameSize)
					ifs = aMinSIFSPeriod;
				else
					ifs = aMinLIFSPeriod;
				Scheduler::instance().schedule(&IFSH, &(IFSH.nullEvent), ifs/phy->getRate('s'));
				resetTRX();
				taskSuccess('a');
			}
			else if (rxData)	//default handling (virtually the only handling needed) for <rxData>
			{
				ch = HDR_CMN(rxData);
				if (ch->size() <= aMaxSIFSFrameSize)
					ifs = aMinSIFSPeriod;
				else
					ifs = aMinLIFSPeriod;
				Scheduler::instance().schedule(&IFSH, &(IFSH.nullEvent), ifs/phy->getRate('s'));
				resetTRX();
				taskSuccess('a');
			}
			else	//ack. for duplicated packet
			{
				resetTRX();
				taskSuccess('a');
			}
		}
		else if (txPkt == txBcnCmd)
		{
			//default handling -- should be replaced once a specific task will handle this
			wph = HDR_LRWPAN(txBcnCmd);
			ch = HDR_CMN(txBcnCmd);
			frmCtrl.FrmCtrl = wph->MHR_FrmCtrl;
			frmCtrl.parse();
			if (frmCtrl.ackReq)	//ack. required
			{
				//enable the receiver
				plme_set_trx_state_request(p_RX_ON);
				txT->start(mpib.macAckWaitDuration/phy->getRate('s'));
				waitBcnCmdAck = true;
			}
			else		//assume success if ack. not required
			{
				resetTRX();
				taskSuccess('c');
			}
		}
		else if (txPkt == txBcnCmd2)
		{
			if (taskP.taskStatus(TP_mlme_scan_request)
					&& ((taskP.mlme_scan_request_ScanType == 0x01)		//active scan
						||(taskP.mlme_scan_request_ScanType == 0x03))		//orphan scan
					&& (strcmp(taskP.taskFrFunc(TP_mlme_scan_request),frFunc) == 0))
				mlme_scan_request(taskP.mlme_scan_request_ScanType,taskP.mlme_scan_request_ScanChannels,taskP.mlme_scan_request_ScanDuration,false,status);
			else if (taskP.taskStatus(TP_mlme_start_request)
					&& (strcmp(taskP.taskFrFunc(TP_mlme_start_request),frFunc) == 0))
				mlme_start_request(taskP.mlme_start_request_PANId,taskP.mlme_start_request_LogicalChannel,taskP.mlme_start_request_BeaconOrder,taskP.mlme_start_request_SuperframeOrder,taskP.mlme_start_request_PANCoordinator,taskP.mlme_start_request_BatteryLifeExtension,0,taskP.mlme_start_request_SecurityEnable,false,status);
			else if (taskP.taskStatus(TP_mlme_associate_request)
					&& (strcmp(taskP.taskFrFunc(TP_mlme_associate_request),frFunc) == 0))
				mlme_associate_request(0,0,0,0,0,taskP.mlme_associate_request_SecurityEnable,false,status);
			else if (taskP.taskStatus(TP_mlme_poll_request)
					&& (strcmp(taskP.taskFrFunc(TP_mlme_poll_request),frFunc) == 0))
				mlme_poll_request(taskP.mlme_poll_request_CoordAddrMode,taskP.mlme_poll_request_CoordPANId,taskP.mlme_poll_request_CoordAddress,taskP.mlme_poll_request_SecurityEnable,taskP.mlme_poll_request_autoRequest,false,status);
			else	//default handling
			{
				wph = HDR_LRWPAN(txBcnCmd2);
				ch = HDR_CMN(txBcnCmd2);
				frmCtrl.FrmCtrl = wph->MHR_FrmCtrl;
				frmCtrl.parse();
				if (frmCtrl.ackReq)	//ack. required
				{
					//enable the receiver
					plme_set_trx_state_request(p_RX_ON);
					txT->start(mpib.macAckWaitDuration/phy->getRate('s'));
					waitBcnCmdAck2 = true;
				}
				else		//assume success if ack. not required
				{
					resetTRX();
					taskSuccess('C');
				}
			}
		}
		else if (txPkt == txData)
		{
			assert((taskP.taskStatus(TP_mcps_data_request))
					&& (strcmp(taskP.taskFrFunc(TP_mcps_data_request),frFunc) == 0));

			wph = HDR_LRWPAN(txData);
			ch = HDR_CMN(txData);
			frmCtrl.FrmCtrl = wph->MHR_FrmCtrl;
			frmCtrl.parse();
			if (taskP.taskStatus(TP_mcps_data_request))
			{
				if (taskP.mcps_data_request_TxOptions & TxOp_GTS)		//GTS transmission
				{
					;	//TBD
				}
				else if ((taskP.mcps_data_request_TxOptions & TxOp_Indirect)	//indirect transmission
						&& (capability.FFD&&(numberDeviceLink(&deviceLink1) > 0)))	//I am a coordinator
				{
					if (!frmCtrl.ackReq)	//ack. not required
						mcps_data_request(0,0,0,0,0,0,0,0,0,taskP.mcps_data_request_TxOptions,false,p_SUCCESS);
					else
					{
						strcpy(taskP.taskFrFunc(TP_mcps_data_request),"recvAck");
						//enable the receiver
						plme_set_trx_state_request(p_RX_ON);
						txT->start(mpib.macAckWaitDuration/phy->getRate('s'));
						waitDataAck = true;
					}
				}
				else		//direct transmission: in this case, let mcps_data_request() take care of everything
					mcps_data_request(0,0,0,0,0,0,0,0,0,taskP.mcps_data_request_TxOptions,false,status);
			}
			else	//default handling (seems impossible)
			{
				if (frmCtrl.ackReq)	//ack. required
				{
					//enable the receiver
					plme_set_trx_state_request(p_RX_ON);
					txT->start(mpib.macAckWaitDuration/phy->getRate('s'));
					waitDataAck = true;
				}
				else		//assume success if ack. not required
				{
					resetTRX();
					taskSuccess('d');
				}
			}
		}
		//else		//may be purged from pending list
	}
	else if (strcmp(frFunc,"recvAck") == 0)		//always check the task status if the dispatch comes from recvAck()
	{
		if (txPkt == txData)
		{
			if ((taskP.taskStatus(TP_mcps_data_request))
					&& (strcmp(taskP.taskFrFunc(TP_mcps_data_request),frFunc) == 0))
				mcps_data_request(0,0,0,0,0,0,0,0,0,taskP.mcps_data_request_TxOptions,false,p_SUCCESS);
			else	//default handling for <txData>
			{
				if (taskP.taskStatus(TP_mcps_data_request))	//seems late ACK received
					taskP.taskStatus(TP_mcps_data_request) = false;
				resetTRX();
				taskSuccess('d');
			}
		}
		else if (txPkt == txBcnCmd2)
		{
			if (taskP.taskStatus(TP_mlme_associate_request)
					&& (strcmp(taskP.taskFrFunc(TP_mlme_associate_request),frFunc) == 0))
				mlme_associate_request(0,0,0,0,0,taskP.mlme_associate_request_SecurityEnable,false,p_SUCCESS);
			else if (taskP.taskStatus(TP_mlme_poll_request)
					&& (strcmp(taskP.taskFrFunc(TP_mlme_poll_request),frFunc) == 0))
				mlme_poll_request(taskP.mlme_poll_request_CoordAddrMode,taskP.mlme_poll_request_CoordPANId,taskP.mlme_poll_request_CoordAddress,taskP.mlme_poll_request_SecurityEnable,taskP.mlme_poll_request_autoRequest,false,p_SUCCESS);
			else	//default handling for <txBcnCmd2>
				taskSuccess('C');
		}
		else if	(txPkt == txBcnCmd)	//default handling for <txBcnCmd>
		{
			wph = HDR_LRWPAN(txBcnCmd);
			frmCtrl.FrmCtrl = wph->MHR_FrmCtrl;
			frmCtrl.parse();
			if ((frmCtrl.frmType == defFrmCtrl_Type_MacCmd)			//command packet
					&& (wph->MSDU_CmdType == 0x02))				//association response packet
				mlme_associate_response(taskP.mlme_associate_response_DeviceAddress,0,m_SUCCESS,0,false,p_SUCCESS);
			else if ((frmCtrl.frmType == defFrmCtrl_Type_MacCmd)		//command packet
					&& (wph->MSDU_CmdType == 0x08))					//coordinator realignment response packet
				mlme_orphan_response(taskP.mlme_orphan_response_OrphanAddress,0,true,false,false);
			else
				taskSuccess('c');
		}
		//else		//may be purged from pending list
	}
	else if (strcmp(frFunc,"txHandler") == 0)	//always check the task status if the dispatch comes from a timer
	{
		if (txPkt == txData)
		{
			if ((!taskP.taskStatus(TP_mcps_data_request))
					|| (strcmp(taskP.taskFrFunc(TP_mcps_data_request),"recvAck") != 0))
				return;

			if (taskP.taskStatus(TP_mcps_data_request))
			{
				if (taskP.mcps_data_request_TxOptions & TxOp_GTS)		//GTS transmission
				{
					;	//TBD
				}
				else if ((taskP.mcps_data_request_TxOptions & TxOp_Indirect)	//indirect transmission
						&& (capability.FFD&&(numberDeviceLink(&deviceLink1) > 0)))	//I am a coordinator
				{
					/*
					//there is contradiction in the draft:
					//page 156, line 16: (for transaction, i.e., indirect transmission) "all subsequent retransmissions shall be transmitted using CSMA-CA"
					//page 158, line 14-16:
					//	"if a single transmission attempt has failed and the transmission was indirect, the coordinator shall not
					//	retransmit the data or MAC command frame. Instead, the frame shall remain in the transaction queue of the
					//	coordinator."
					//the description on page 158 is more reasonable (though we already proceeded according to page 156)
					// now follow page 158
					numDataRetry++;
					if (numDataRetry <= aMaxFrameRetries)
					{
					//no need to check if the packet has been purged -- if purged, then taskFailed() should have set txData = 0
					wph = HDR_LRWPAN(txData);
					frmCtrl.FrmCtrl = wph->MHR_FrmCtrl;
					frmCtrl.parse();
					i = updateTransacLinkByPktOrHandle(tr_oper_est,&transacLink1,&transacLink2,txData);
					if (i != 0)	//already purged from pending list
					{
					Packet::free(txData);
					txData = 0;
					return;
					}
					// -- don't end here, but afte 'else'
					waitDataAck = false;
					csmacaResume();
					}
					else
					*/
					mcps_data_request(0,0,0,0,0,0,0,0,0,taskP.mcps_data_request_TxOptions,false,p_BUSY,m_NO_ACK);
				}
				else
				{
					//direct transmission: in this case, let mcps_data_request() take care of everything
					mcps_data_request(0,0,0,0,0,0,0,0,0,taskP.mcps_data_request_TxOptions,false,p_BUSY);	//status can be anything but p_SUCCESS
				}
			}
		}
		else if (txPkt == txBcnCmd2)
		{
			if (taskP.taskStatus(TP_mlme_associate_request)
					&& (strcmp(taskP.taskFrFunc(TP_mlme_associate_request),"recvAck") == 0))
				mlme_associate_request(0,0,0,0,0,taskP.mlme_associate_request_SecurityEnable,false,p_BUSY);	//status can anything but p_SUCCESS
			else if (taskP.taskStatus(TP_mlme_poll_request)
					&& (strcmp(taskP.taskFrFunc(TP_mlme_poll_request),"recvAck") == 0))
				mlme_poll_request(taskP.mlme_poll_request_CoordAddrMode,taskP.mlme_poll_request_CoordPANId,taskP.mlme_poll_request_CoordAddress,taskP.mlme_poll_request_SecurityEnable,taskP.mlme_poll_request_autoRequest,false,p_BUSY);		//status can anything but p_SUCCESS
			else	//default handling for <txBcnCmd2>
			{
				numBcnCmdRetry2++;
				if (numBcnCmdRetry2 <= aMaxFrameRetries)
					waitBcnCmdAck2 = false;
				else
				{
					freePkt(txBcnCmd2);
					txBcnCmd2 = 0;
				}
				csmacaResume();		//other packets may be waiting
			}
		}
		else if (txPkt == txBcnCmd)
		{
			wph = HDR_LRWPAN(txBcnCmd);
			frmCtrl.FrmCtrl = wph->MHR_FrmCtrl;
			frmCtrl.parse();
			if ((frmCtrl.frmType == defFrmCtrl_Type_MacCmd)		//command packet
					&& (wph->MSDU_CmdType == 0x02))			//association response packet
			{
				//different from data packet, association response packet
				//should be retransmitted though it uses indirect transmission
				//(refer to page 67, line 28-32)
				numBcnCmdRetry++;
				if (numBcnCmdRetry <= aMaxFrameRetries)
				{
					/* no need to check if the packet has been purged -- if purged, then taskFailed() should have set txBcnCmd = 0
					   if (wph->indirect)				//indirect transmission
					   {
					   frmCtrl.FrmCtrl = wph->MHR_FrmCtrl;
					   frmCtrl.parse();
					   i = updateTransacLinkByPktOrHandle(tr_oper_est,&transacLink1,&transacLink2,txBcnCmd);
					   if (i != 0)	//already purged from pending list
					   {
					   Packet::free(txBcnCmd);
					   txBcnCmd = 0;
					   return;
					   }
					   }
					   */
					strcpy(taskP.taskFrFunc(TP_mlme_associate_response),"csmacaCallBack");
					waitBcnCmdAck = false;
					csmacaResume();
				}
				else
					mlme_associate_response(taskP.mlme_associate_response_DeviceAddress,0,m_NO_ACK,0,false,p_BUSY);	//status returned in MACenum rather than in PHYenum
			}
			else if ((frmCtrl.frmType == defFrmCtrl_Type_MacCmd)		//command packet
					&& (wph->MSDU_CmdType == 0x08))					//coordinator realignment response packet
			{
				numBcnCmdRetry++;
				if (numBcnCmdRetry <= aMaxFrameRetries)
				{
					strcpy(taskP.taskFrFunc(TP_mlme_orphan_response),"csmacaCallBack");
					waitBcnCmdAck = false;
					csmacaResume();
				}
				else
					mlme_orphan_response(taskP.mlme_orphan_response_OrphanAddress,0,true,false,p_BUSY);
			}
			else		//default handling for <txBcnCmd>
			{
				freePkt(txBcnCmd);
				txBcnCmd = 0;
				csmacaResume();		//other packets may be waiting
			}
		}
		//else		//may be purged from the pending list

	}
	else if (strcmp(frFunc,"PLME_SET_TRX_STATE_confirm") == 0)
	{
		//handle TRX_OFF
		if (req_state == p_TRX_OFF)
		{
			if (taskP.taskStatus(TP_mlme_reset_request)
					&& (strcmp(taskP.taskFrFunc(TP_mlme_reset_request),frFunc) == 0))
			{
				mlme_reset_request(taskP.mlme_reset_request_SetDefaultPIB,false,status);
			}
		}

		//handle RX_ON
		if (req_state == p_RX_ON)
		{
			if (taskP.taskStatus(TP_mlme_scan_request)
					&& (strcmp(taskP.taskFrFunc(TP_mlme_scan_request),frFunc) == 0))
			{
				mlme_scan_request(taskP.mlme_scan_request_ScanType,taskP.mlme_scan_request_ScanChannels,taskP.mlme_scan_request_ScanDuration,false,status);
			}
			else if (taskP.taskStatus(TP_mlme_rx_enable_request)
					&& (strcmp(taskP.taskFrFunc(TP_mlme_rx_enable_request),frFunc) == 0))
			{
				mlme_rx_enable_request(0,taskP.mlme_rx_enable_request_RxOnTime,taskP.mlme_rx_enable_request_RxOnDuration,false);
			}
		}
	}
	else if (strcmp(frFunc,"PLME_SET_confirm") == 0)
	{
		if (taskP.taskStatus(TP_mlme_scan_request)
				&& (strcmp(taskP.taskFrFunc(TP_mlme_scan_request),frFunc) == 0))
			mlme_scan_request(taskP.mlme_scan_request_ScanType,taskP.mlme_scan_request_ScanChannels,taskP.mlme_scan_request_ScanDuration,false,status);
	}
	else if (strcmp(frFunc,"PLME_ED_confirm") == 0)
	{
		if (taskP.taskStatus(TP_mlme_scan_request)
				&& (taskP.mlme_scan_request_ScanType == 0x00)		//ED scan
				&& (strcmp(taskP.taskFrFunc(TP_mlme_scan_request),frFunc) == 0))
			mlme_scan_request(taskP.mlme_scan_request_ScanType,taskP.mlme_scan_request_ScanChannels,taskP.mlme_scan_request_ScanDuration,false,status);
	}
	else if (strcmp(frFunc,"recvBeacon") == 0)
	{
		if (taskP.taskStatus(TP_mlme_scan_request)
				&& (strcmp(taskP.taskFrFunc(TP_mlme_scan_request),frFunc) == 0))
			mlme_scan_request(taskP.mlme_scan_request_ScanType,taskP.mlme_scan_request_ScanChannels,taskP.mlme_scan_request_ScanDuration,false,p_SUCCESS);
		else if (taskP.taskStatus(TP_mlme_rx_enable_request)
				&& (strcmp(taskP.taskFrFunc(TP_mlme_rx_enable_request),frFunc) == 0))
			mlme_rx_enable_request(0,taskP.mlme_rx_enable_request_RxOnTime,taskP.mlme_rx_enable_request_RxOnDuration,false);
		else if (taskP.taskStatus(TP_mlme_sync_request)
				&& (strcmp(taskP.taskFrFunc(TP_mlme_sync_request),frFunc) == 0))
			mlme_sync_request(0,taskP.mlme_sync_request_tracking,false,p_SUCCESS);
	}
	else if (strcmp(frFunc,"scanHandler") == 0)	//always check the task status if the dispatch comes from a timer
	{
		if (taskP.taskStatus(TP_mlme_scan_request))
			mlme_scan_request(taskP.mlme_scan_request_ScanType,taskP.mlme_scan_request_ScanChannels,taskP.mlme_scan_request_ScanDuration,false,p_BUSY);
	}
	else if (strcmp(frFunc,"extractHandler") == 0)	//always check the task status if the dispatch comes from a timer
	{
		if (taskP.taskStatus(TP_mlme_associate_request)
				&& (strcmp(taskP.taskFrFunc(TP_mlme_associate_request),frFunc) == 0))
		{
			mlme_associate_request(0,0,0,0,0,taskP.mlme_associate_request_SecurityEnable,false,p_BUSY);	//status ignored in case 4, but should set to any value but p_SUCCESS in case 7 -- p_BUSY will be ok anyway
		}
		else if (taskP.taskStatus(TP_mlme_poll_request)
				&& (strcmp(taskP.taskFrFunc(TP_mlme_poll_request),"IFSHandler") == 0))
			mlme_poll_request(taskP.mlme_poll_request_CoordAddrMode,taskP.mlme_poll_request_CoordPANId,taskP.mlme_poll_request_CoordAddress,taskP.mlme_poll_request_SecurityEnable,taskP.mlme_poll_request_autoRequest,false,p_BUSY);
	}
	else if (strcmp(frFunc,"assoRspWaitHandler") == 0)	//always check the task status if the dispatch comes from a timer
	{
		if (taskP.taskStatus(TP_mlme_associate_response))
		{
			taskP.taskStep(TP_mlme_associate_response) = 2;
			mlme_associate_response(taskP.mlme_associate_response_DeviceAddress,0,m_SUCCESS,0,false,p_BUSY);	//status ignored
		}
	}
	else if (strcmp(frFunc,"dataWaitHandler") == 0)	//always check the task status if the dispatch comes from a timer
	{
		if (taskP.taskStatus(TP_mcps_data_request))
		{
			taskP.taskStep(TP_mcps_data_request) = 2;
			mcps_data_request(0,0,0,0,0,0,0,0,0,taskP.mcps_data_request_TxOptions,false,p_BUSY);	//status ignored
		}
	}
	else if (strcmp(frFunc,"IFSHandler") == 0)	//always check the task status if the dispatch comes from a timer
	{
		if (taskP.taskStatus(TP_mlme_associate_request)
				&& (strcmp(taskP.taskFrFunc(TP_mlme_associate_request),frFunc) == 0))
			mlme_associate_request(0,0,0,0,0,taskP.mlme_associate_request_SecurityEnable,false,p_SUCCESS,mStatus);
		else if (taskP.taskStatus(TP_mlme_poll_request)
				&& (strcmp(taskP.taskFrFunc(TP_mlme_poll_request),frFunc) == 0))
			mlme_poll_request(taskP.mlme_poll_request_CoordAddrMode,taskP.mlme_poll_request_CoordPANId,taskP.mlme_poll_request_CoordAddress,taskP.mlme_poll_request_SecurityEnable,taskP.mlme_poll_request_autoRequest,false,p_SUCCESS);
		else if (taskP.taskStatus(TP_mlme_scan_request)
				&& (strcmp(taskP.taskFrFunc(TP_mlme_scan_request),frFunc) == 0))
			mlme_scan_request(taskP.mlme_scan_request_ScanType,taskP.mlme_scan_request_ScanChannels,taskP.mlme_scan_request_ScanDuration,false,p_SUCCESS);
	}
	else if (strcmp(frFunc,"rxEnableHandler") == 0)
	{
		//if (taskP.taskStatus(TP_mlme_rx_enable_request))	//we don't check the task status (it may be reset)
		if (strcmp(taskP.taskFrFunc(TP_mlme_rx_enable_request),frFunc) == 0)
			mlme_rx_enable_request(0,taskP.mlme_rx_enable_request_RxOnTime,taskP.mlme_rx_enable_request_RxOnDuration,false);
	}
	else if (strcmp(frFunc,"beaconSearchHandler") == 0)	//always check the task status if the dispatch comes from a timer
	{
		if (taskP.taskStatus(TP_mlme_sync_request)
				&& (strcmp(taskP.taskFrFunc(TP_mlme_sync_request),"recvBeacon") == 0))
			mlme_sync_request(0,taskP.mlme_sync_request_tracking,false,p_BUSY);	//status can anything but p_SUCCESS
	}
}

void Mac802_15_4::sendDown(Packet *p,Handler* h)
{
	if (updateNFailLink(fl_oper_est,index_) == 0)
	{
		if (txBeacon)
		{
			beaconWaiting = false;
			Packet::free(txBeacon);
			txBeacon = 0;
		}
		return;
	}
	else if (updateLFailLink(fl_oper_est,index_,p802_15_4macDA(p)) == 0)
	{
		dispatch(p_UNDEFINED,"PD_DATA_confirm");
		return;
	}

	inTransmission = true;

	//double trx_time = phy->trxTime(p,false);
	/* Linux floating number compatibility
	   txOverT->start(trx_time + 1/phy->getRate('s'));
	   */
	//{
	//double tmpf;
	//tmpf = 1/phy->getRate('s');		
	//txOverT->start(trx_time + tmpf);
	//}
	EnergyModel *em = netif_->node()->energy_model();
	if (em)
		if (em->energy() <= 0)
		{
			PD_DATA_confirm(p_UNDEFINED);
			return;
		}

	downtarget_->recv(p, h);
}

void Mac802_15_4::mcps_data_request(UINT_8 SrcAddrMode,UINT_16 SrcPANId,IE3ADDR SrcAddr,
		UINT_8 DstAddrMode,UINT_16 DstPANId,IE3ADDR DstAddr,
		UINT_8 msduLength,Packet *msdu,UINT_8 msduHandle,UINT_8 TxOptions,
		bool frUpper,PHYenum status,MACenum mStatus)
{
	UINT_8 step,task;
	hdr_lrwpan *wph;
	hdr_cmn *ch;
	FrameCtrl frmCtrl;
	double kpTime;
	int i;
	EnergyModel *em = netif_->node()->energy_model();

	task = TP_mcps_data_request;
	if (frUpper) checkTaskOverflow(task);	

	step = taskP.taskStep(task);
	if (step == 0)
	{
		//check if parameters valid or not
		ch = HDR_CMN(msdu);
		if (ch->ptype() == PT_MAC)	//we only check for 802.15.4 packets (let other packets go through -- must be changed in implementation)
			if ((SrcAddrMode > 0x03)
					||(DstAddrMode > 0x03)
					||(msduLength > aMaxMACFrameSize)
					||(TxOptions > 0x0f))
			{
				sscs->MCPS_DATA_confirm(msduHandle,m_INVALID_PARAMETER);
				return;
			}

		taskP.taskStatus(task) = true;
		taskP.mcps_data_request_TxOptions = TxOptions;
		//---construct a MPDU packet (not really a new packet in simulation, but still <msdu>)---
		frmCtrl.FrmCtrl = 0;
		frmCtrl.setFrmType(defFrmCtrl_Type_Data);	//data type
		if (TxOptions & TxOp_Acked)
			frmCtrl.setAckReq(true);
		if (SrcPANId == DstPANId)
			frmCtrl.setIntraPan(true);		//Intra PAN
		frmCtrl.setDstAddrMode(DstAddrMode);		//we reverse the bit order -- note to use the required order in implementation
		frmCtrl.setSrcAddrMode(SrcAddrMode);		//we reverse the bit order -- note to use the required order in implementation
		wph = HDR_LRWPAN(msdu);
		wph->MHR_DstAddrInfo.panID = DstPANId;
		wph->MHR_DstAddrInfo.addr_64 = DstAddr;		//it doesn't matter if this is actually a 16-bit address
		wph->MHR_SrcAddrInfo.panID = SrcPANId;
		wph->MHR_SrcAddrInfo.addr_64 = SrcAddr;		//it doesn't matter if this is actually a 16-bit address
		//ignore FCS in simulation
		constructMPDU(msduLength,msdu,frmCtrl.FrmCtrl,mpib.macDSN++,wph->MHR_DstAddrInfo,wph->MHR_SrcAddrInfo,0,0,0);
		//for trace
		p802_15_4hdrDATA(msdu);
		//---------------------------------------------------------------------------------------------------

		//perform security task if required (ignored in simulation)
	}

	if (TxOptions & TxOp_GTS)	//GTS transmission
	{
		switch(step)
		{
			//other cases: TBD
			default:
				break;
		}
	}
	else if ((TxOptions & TxOp_Indirect)				//indirect transmission
			&& (capability.FFD&&(numberDeviceLink(&deviceLink1) > 0)))	//I am a coordinator
	{
		switch(step)
		{
			case 0:
				taskP.taskStep(task)++;
				taskP.mcps_data_request_pendPkt = msdu;
				if ((DstAddrMode == defFrmCtrl_AddrMode16)		//16-bit address available
						|| (DstAddrMode == defFrmCtrl_AddrMode64))		//64-bit address available
				{
					/* Linux floating number compatibility
					   kpTime = mpib.macTransactionPersistenceTime * (aBaseSuperframeDuration * (1 << mpib.macBeaconOrder) / phy->getRate('s'));
					   */
					{
						double tmpf;
						tmpf = (aBaseSuperframeDuration * (1 << mpib.macBeaconOrder) / phy->getRate('s'));
						kpTime = mpib.macTransactionPersistenceTime * tmpf;		
					}

					chkAddTransacLink(&transacLink1,&transacLink2,DstAddrMode,DstAddr,msdu,msduHandle,kpTime);
				}
				strcpy(taskP.taskFrFunc(task),"csmacaCallBack");
				dataWaitT->start(kpTime);		
				break;
			case 1:
				if (!taskP.taskStatus(task))	
					break;
				if (status == p_SUCCESS)	//data packet transmitted and, if required, ack. received 
				{
					dataWaitT->stop();
					taskP.taskStatus(task) = false;
					resetTRX();
					taskSuccess('d');
				}
				else				//data packet transmission failed
				{
					//leave the packet in the queue waiting for next polling
					strcpy(taskP.taskFrFunc(task),"csmacaCallBack");	//wait for next polling
					//status return in MACenum, either CHANNEL_ACCESS_FAILURE or NO_ACK
					//taskP.taskStatus(task) = false;	
					resetTRX();
					taskFailed('d',mStatus);
				}
				break;
			case 2:
				if (!taskP.taskStatus(task))		
					break;
				taskP.taskStatus(task) = false;
				//check if the transaction still pending -- actually no need to check (it must be pending if case 1 didn't happen), but no harm
				i = updateTransacLinkByPktOrHandle(tr_oper_est,&transacLink1,&transacLink2,taskP.mcps_data_request_pendPkt);	//don't use <txData>, since assignment 'txData = msdu' only happens if a data request command received
				if (i == 0)	//still pending
				{
					//get a copy of the packet for taskFailed()
					if (!txData)
						txData = taskP.mcps_data_request_pendPkt->copy();
					//delete the packet from the transaction list immediately -- prevent the packet from being transmitted at the last moment
					updateTransacLinkByPktOrHandle(tr_oper_del,&transacLink1,&transacLink2,taskP.mcps_data_request_pendPkt);
					resetTRX();
					taskFailed('d',m_TRANSACTION_EXPIRED);
					return;
				}
				else	//being successfully extracted
				{
					resetTRX();
					taskFailed('d',m_SUCCESS);
					return;
				}
				break;

			default:
				break;
		}
	}
	else				//direct transmission
	{
		switch(step)
		{
			case 0:
				taskP.taskStep(task)++;
				strcpy(taskP.taskFrFunc(task),"csmacaCallBack");
				assert(!txData);
				txData = msdu;
				csmacaBegin('d');
				break;
			case 1:
				if (status == p_IDLE)
				{
					taskP.taskStep(task)++;
					strcpy(taskP.taskFrFunc(task),"PD_DATA_confirm");
					//enable the transmitter
					plme_set_trx_state_request(p_TX_ON);
				}
				else
				{
					wph = HDR_LRWPAN(txData);
					ch = HDR_CMN(txData);
					if (wph->msduHandle)	//from SSCS
					{
						//let the upper layer handle the failure (no retry)
						taskP.taskStatus(task) = false;
						resetTRX();
						taskFailed('d',m_CHANNEL_ACCESS_FAILURE);
					}
					else
					{
						// 						2.31 change. An access failure needs to be 							reported here. Earlier csmacaResume() was being 						called indefinetely. This has been commented and 							the folowing 3 lines of code have been added 							within else.
						//						csmacaResume();
						taskP.taskStatus(task) = false;
						resetTRX();
						taskFailed('d',m_CHANNEL_ACCESS_FAILURE,0);
#ifdef SHUTDOWN
						if ((em) && ((!capability.FFD)||(numberDeviceLink(&deviceLink1) == 0)) && (NOW>phy->T_sleep_)){ //I can sleep only if i am not a coordinator
							phy->putNodeToSleep();
						}
#endif
					}
				}
				break;
			case 2:
				wph = HDR_LRWPAN(txData);
				ch = HDR_CMN(txData);
				frmCtrl.FrmCtrl = wph->MHR_FrmCtrl;
				frmCtrl.parse();
				if (frmCtrl.ackReq)	//ack. required
				{
					taskP.taskStep(task)++;
					strcpy(taskP.taskFrFunc(task),"recvAck");
					//enable the receiver
					plme_set_trx_state_request(p_RX_ON);
					txT->start(mpib.macAckWaitDuration/phy->getRate('s'));
					waitDataAck = true;
				}
				else		//assume success if ack. not required
				{
					taskP.taskStatus(task) = false;
					resetTRX();
					taskSuccess('d');
					// 2.31 change: added this to put the node to sleep after successful pkt tx
#ifdef SHUTDOWN
					if ((em) && ((!capability.FFD)||(numberDeviceLink(&deviceLink1) == 0)) && (NOW>phy->T_sleep_)){ //I can sleep only if i am not a coordinator
						phy->putNodeToSleep();
					}
#endif

				}
				break;
			case 3:
				if (status == p_SUCCESS)	//ack. received
				{
					taskP.taskStatus(task) = false;
					resetTRX();
					taskSuccess('d');
				}
				else				//time out when waiting for ack.
				{
					numDataRetry++;
					if (numDataRetry <= aMaxFrameRetries)
					{
						taskP.taskStep(task) = 1;	//important
						strcpy(taskP.taskFrFunc(task),"csmacaCallBack");
						waitDataAck = false;
						csmacaResume();
					}
					else
					{
						taskP.taskStatus(task) = false;
						resetTRX();
						taskFailed('d',m_NO_ACK);
					}
				}
#ifdef SHUTDOWN
				if ((em) && ((!capability.FFD)||(numberDeviceLink(&deviceLink1) == 0)) && (NOW>phy->T_sleep_)){ //I can sleep only if i am not a coordinator
					phy->putNodeToSleep();
				}
#endif
				break;
			default:
				break;
		}
	}
}

void Mac802_15_4::mlme_associate_request(UINT_8 LogicalChannel,UINT_8 CoordAddrMode,UINT_16 CoordPANId,IE3ADDR CoordAddress,
		UINT_8, bool SecurityEnable,
		bool frUpper,PHYenum status,MACenum mStatus)
{
	//refer to Figure 25 for association details
	UINT_8 step,task;
	FrameCtrl frmCtrl;
	hdr_lrwpan* wph;

	task = TP_mlme_associate_request;
	if (frUpper) checkTaskOverflow(task);

	step = taskP.taskStep(task);
	switch(step)
	{
		case 0:
			//check if parameters valid or not
			if ((!phy->channelSupported(LogicalChannel))
					|| ((CoordAddrMode != defFrmCtrl_AddrMode16)&&(CoordAddrMode != defFrmCtrl_AddrMode64)))
			{
				sscs->MLME_ASSOCIATE_confirm(0,m_INVALID_PARAMETER);
				return;
			}

			//assert(mpib.macShortAddress == 0xffff);		//not associated yet

			//we may optionally track beacons if beacon enabled (here we don't)

			tmp_ppib.phyCurrentChannel = LogicalChannel;
			phy->PLME_SET_request(phyCurrentChannel,&tmp_ppib);
			mpib.macPANId = CoordPANId;
			mpib.macCoordExtendedAddress = CoordAddress;
			taskP.taskStatus(task) = true;
			taskP.taskStep(task)++;
			strcpy(taskP.taskFrFunc(task),"csmacaCallBack");
			taskP.mlme_associate_request_CoordAddrMode = CoordAddrMode;
			taskP.mlme_associate_request_SecurityEnable = SecurityEnable;
			//--- send an association request command ---
#ifdef DEBUG802_15_4
			fprintf(stdout,"[%s::%s][%f](node %d) before alloc txBcnCmd2:\n\t\ttxBeacon\t= %ld\n\t\ttxAck   \t= %ld\n\t\ttxBcnCmd\t= %ld\n\t\ttxBcnCmd2\t= %ld\n\t\ttxData  \t= %ld\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,txBeacon,txAck,txBcnCmd,txBcnCmd2,txData);
#endif
			assert(!txBcnCmd2);
			txBcnCmd2 = Packet::alloc();
			assert(txBcnCmd2);
			wph = HDR_LRWPAN(txBcnCmd2);
			constructCommandHeader(txBcnCmd2,&frmCtrl,0x01,CoordAddrMode,CoordPANId,CoordAddress,defFrmCtrl_AddrMode64,0xffff,aExtendedAddress,SecurityEnable,false,true);
			wph->MSDU_Payload[0] = capability.cap;
			constructMPDU(2,txBcnCmd2,frmCtrl.FrmCtrl,mpib.macDSN++,wph->MHR_DstAddrInfo,wph->MHR_SrcAddrInfo,0,0x01,0);
			csmacaBegin('C');
			//------------------------------------
			break;
		case 1:
			if (status == p_IDLE)
			{
				taskP.taskStep(task)++;
				strcpy(taskP.taskFrFunc(task),"PD_DATA_confirm");
				if (Mac802_15_4::verbose)
					fprintf(stdout,"[%f](node %d) sending association request command ...\n",CURRENT_TIME,index_);
				plme_set_trx_state_request(p_TX_ON);
				break;
			}
			else
			{
				taskP.taskStatus(task) = false;
				freePkt(txBcnCmd2);
				txBcnCmd2 = 0;
				//restore default values
				mpib.macPANId = def_macPANId;
				mpib.macCoordExtendedAddress = def_macCoordExtendedAddress;
				sscs->MLME_ASSOCIATE_confirm(0,m_CHANNEL_ACCESS_FAILURE);
				csmacaResume();
				return;
			}
			break;
		case 2:
			taskP.taskStep(task)++;
			strcpy(taskP.taskFrFunc(task),"recvAck");
			plme_set_trx_state_request(p_RX_ON);	//waiting for ack.
			txT->start(mpib.macAckWaitDuration/phy->getRate('s'));
			waitBcnCmdAck2 = true;
			break;
		case 3:
			if (status == p_SUCCESS)	//ack. received
			{
				taskP.taskStep(task)++;
				strcpy(taskP.taskFrFunc(task),"extractHandler");
				plme_set_trx_state_request(p_TRX_OFF);		//we don't want to receive any packet at this moment
				if (Mac802_15_4::verbose)
					fprintf(stdout,"[%f](node %d) ack for association request command received\n",CURRENT_TIME,index_);
				taskSuccess('C',false);
				extractT->start(aResponseWaitTime/phy->getRate('s'),false);
			}
			else				//time out when waiting for ack.
			{
				numBcnCmdRetry2++;
				if (numBcnCmdRetry2 <= aMaxFrameRetries)
				{
					taskP.taskStep(task) = 1;	//important
					strcpy(taskP.taskFrFunc(task),"csmacaCallBack");
					waitBcnCmdAck2 = false;
					csmacaResume();
				}
				else
				{
					taskP.taskStatus(task) = false;
					resetTRX();
					freePkt(txBcnCmd2);
					txBcnCmd2 = 0;
					//restore default values
					mpib.macPANId = def_macPANId;
					mpib.macCoordExtendedAddress = def_macCoordExtendedAddress;
					sscs->MLME_ASSOCIATE_confirm(0,m_NO_ACK);
					csmacaResume();
					return;
				}
			}
			break;
		case 4:
			taskP.taskStep(task)++;
			strcpy(taskP.taskFrFunc(task),"PD_DATA_confirm");
			//-- send a data request command to extract the response ---
#ifdef DEBUG802_15_4
			fprintf(stdout,"[%s::%s][%f](node %d) before alloc txBcnCmd2:\n\t\ttxBeacon\t= %ld\n\t\ttxAck   \t= %ld\n\t\ttxBcnCmd\t= %ld\n\t\ttxBcnCmd2\t= %ld\n\t\ttxData  \t= %ld\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,txBeacon,txAck,txBcnCmd,txBcnCmd2,txData);
#endif
			assert(!txBcnCmd2);
			txBcnCmd2 = Packet::alloc();
			assert(txBcnCmd2);
			wph = HDR_LRWPAN(txBcnCmd2);
			if ((mpib.macShortAddress == 0xfffe)||(mpib.macShortAddress == 0xffff))
			{
				frmCtrl.setSrcAddrMode(defFrmCtrl_AddrMode64);
				wph->MHR_SrcAddrInfo.addr_64 = aExtendedAddress;
			}
			else
			{
				frmCtrl.setSrcAddrMode(defFrmCtrl_AddrMode16);
				wph->MHR_SrcAddrInfo.addr_16 = mpib.macShortAddress;
			}
			constructCommandHeader(txBcnCmd2,&frmCtrl,0x04,taskP.mlme_associate_request_CoordAddrMode,mpib.macPANId,mpib.macCoordExtendedAddress,frmCtrl.srcAddrMode,mpib.macPANId,wph->MHR_SrcAddrInfo.addr_64,SecurityEnable,false,true);
			constructMPDU(1,txBcnCmd2,frmCtrl.FrmCtrl,mpib.macDSN++,wph->MHR_DstAddrInfo,wph->MHR_SrcAddrInfo,0,0x04,0);
			waitBcnCmdAck2 = false;		//command packet not yet transmitted
			numBcnCmdRetry2 = 0;
			if (Mac802_15_4::verbose)
				fprintf(stdout,"[%f](node %d) sending data request command ...\n",CURRENT_TIME,index_);
			txCsmaca = txBcnCmd2;
			plme_set_trx_state_request(p_TX_ON);
			//------------------------------------
			break;
		case 5:
			taskP.taskStep(task)++;
			strcpy(taskP.taskFrFunc(task),"recvAck");
			//enable the receiver
			plme_set_trx_state_request(p_RX_ON);
			txT->start(mpib.macAckWaitDuration/phy->getRate('s'));
			waitBcnCmdAck2 = true;
			break;
		case 6:
			if (status == p_SUCCESS)	//ack. received
			{
				taskP.taskStep(task)++;
				strcpy(taskP.taskFrFunc(task),"IFSHandler");
				plme_set_trx_state_request(p_RX_ON);		//wait for response
				if (Mac802_15_4::verbose)
					fprintf(stdout,"[%f](node %d) ack for data request command received\n",CURRENT_TIME,index_);
				taskSuccess('C',false);
				extractT->start(aResponseWaitTime/phy->getRate('s'),false);	//compare: for normal data, wait for <aMaxFrameResponseTime> symbols (or CAP symbols if beacon enabled) (see page 156, line 1-3)
			}
			else				//time out when waiting for ack.
			{
				//No retransmission required in general (just wait for next beacon and poll again, see page 156, line 20-24),
				//but we need to retransmit here, since the node will not handle the pending list before it has associated
				//with the coordinator.
				numBcnCmdRetry2++;
				if (numBcnCmdRetry2 <= aMaxFrameRetries)
				{
					taskP.taskStep(task) = 5;	//important
					strcpy(taskP.taskFrFunc(task),"PD_DATA_confirm");
					waitBcnCmdAck2 = false;
					txCsmaca = txBcnCmd2;
					plme_set_trx_state_request(p_TX_ON);
				}
				else
				{
					taskP.taskStatus(task) = false;
					resetTRX();
					freePkt(txBcnCmd2);
					txBcnCmd2 = 0;
					//restore default values
					mpib.macPANId = def_macPANId;
					mpib.macCoordExtendedAddress = def_macCoordExtendedAddress;
					sscs->MLME_ASSOCIATE_confirm(0,m_NO_DATA);	//assume no DATA
					csmacaResume();
					return;
				}
			}
			break;
		case 7:
			taskP.taskStatus(task) = false;
			resetTRX();
			if (status == p_SUCCESS)		//response received
			{
				if (mStatus == m_SUCCESS)
				{
					changeNodeColor(CURRENT_TIME,(mpib.macAssociationPermit)?Nam802_15_4::def_Coor_clr:Nam802_15_4::def_Dev_clr);
					char label[31];
					sprintf(label,"[%d]",mpib.macCoordExtendedAddress);
#ifdef ZigBeeIF
					if (sscs->t_isCT)
						sprintf(label,"\"%s%d (%d.%d)\"",(sscs->RNType())?"+":"-",rt_myNodeID,sscs->rt_myParentNodeID,sscs->rt_myDepth);
#endif
					nam->changeLabel(CURRENT_TIME,label);
				}
				else
				{
					//restore default values
					mpib.macPANId = def_macPANId;
					mpib.macCoordExtendedAddress = def_macCoordExtendedAddress;
				}
				//stop the timer
				if (Mac802_15_4::verbose)
					fprintf(stdout,"[%f](node %d) association response command received\n",CURRENT_TIME,index_);
				extractT->stop();
				sscs->MLME_ASSOCIATE_confirm(rt_myNodeID,mStatus);
			}
			else					//time out when waiting for response
			{
				//restore default values
				mpib.macPANId = def_macPANId;
				mpib.macCoordExtendedAddress = def_macCoordExtendedAddress;
				sscs->MLME_ASSOCIATE_confirm(0,m_NO_DATA);
			}
			csmacaResume();
		default:
			break;
	}
}

void Mac802_15_4::mlme_associate_response(IE3ADDR DeviceAddress,UINT_16 AssocShortAddress,MACenum Status,bool SecurityEnable,
		bool frUpper,PHYenum status)
{
	FrameCtrl frmCtrl;
	hdr_lrwpan* wph;
	Packet *rspPkt;
	double kpTime;
	UINT_8 step,task;
	int i;
	task = TP_mlme_associate_response;
	//checkTaskOverflow(task);	
	if (frUpper)
	{
		if (taskP.taskStatus(task))	//overflow
		{
			sscs->MLME_COMM_STATUS_indication(mpib.macPANId,defFrmCtrl_AddrMode64,aExtendedAddress,defFrmCtrl_AddrMode64,DeviceAddress,m_TRANSACTION_OVERFLOW);
			return;
		}
		taskP.taskStep(task) = 0;
		(taskP.taskFrFunc(task))[0] = 0;
	}
	step = taskP.taskStep(task);
	switch(step)
	{
		case 0:
			//check if parameters valid or not
			if ((Status != m_SUCCESS)&&(Status != m_PAN_at_capacity)&&(Status != m_PAN_access_denied))
			{
				sscs->MLME_COMM_STATUS_indication(mpib.macPANId,defFrmCtrl_AddrMode64,aExtendedAddress,defFrmCtrl_AddrMode64,DeviceAddress,m_INVALID_PARAMETER);
				return;
			}
			taskP.taskStatus(task) = true;
			taskP.taskStep(task)++;
			strcpy(taskP.taskFrFunc(task),"csmacaCallBack");
			taskP.mlme_associate_response_DeviceAddress = DeviceAddress;
			//--- construct an association response command packet and put it in the pending list ---
			rspPkt = Packet::alloc();
			assert(rspPkt);
			wph = HDR_LRWPAN(rspPkt);
#ifdef ZigBeeIF
			sscs->setGetClusTreePara('s',rspPkt);
#endif
			constructCommandHeader(rspPkt,&frmCtrl,0x02,defFrmCtrl_AddrMode64,mpib.macPANId,DeviceAddress,defFrmCtrl_AddrMode64,mpib.macPANId,aExtendedAddress,SecurityEnable,false,true);
			*((UINT_16 *)wph->MSDU_Payload) = AssocShortAddress;
			*((MACenum *)(wph->MSDU_Payload + 2)) = Status;
			constructMPDU(4,rspPkt,frmCtrl.FrmCtrl,mpib.macDSN++,wph->MHR_DstAddrInfo,wph->MHR_SrcAddrInfo,0,0x02,0);
			kpTime = (2 * aResponseWaitTime) / phy->getRate('s');
			i = chkAddTransacLink(&transacLink1,&transacLink2,defFrmCtrl_AddrMode64,DeviceAddress,rspPkt,0,kpTime);
			if (i != 0)	//overflow or failed
			{
#ifdef DEBUG802_15_4
				hdr_cmn *ch = HDR_CMN(rspPkt);
				fprintf(stdout,"[%s::%s][%f](node %d) task overflow or failed: type = %s, src = %d, dst = %d, uid = %d, mac_uid = ??, size = %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,wpan_pName(rspPkt),p802_15_4macSA(rspPkt),p802_15_4macDA(rspPkt),wph->uid,ch->size());
#endif
				taskP.taskStatus(task) = false;
				Packet::free(rspPkt);
				sscs->MLME_COMM_STATUS_indication(mpib.macPANId,defFrmCtrl_AddrMode64,aExtendedAddress,defFrmCtrl_AddrMode64,DeviceAddress,m_TRANSACTION_OVERFLOW);
				return;
			}
			//----------------------------------------------------------------------
			assoRspWaitT->start(kpTime);
			taskP.mlme_associate_response_pendPkt = rspPkt;
			break;
		case 1:
			if (!taskP.taskStatus(task))	
				break;
			taskP.taskStatus(task) = false;
			assoRspWaitT->stop();
			if (status == p_SUCCESS)	//response packet transmitted and ack. received
			{
				sscs->MLME_COMM_STATUS_indication(mpib.macPANId,defFrmCtrl_AddrMode64,aExtendedAddress,defFrmCtrl_AddrMode64,DeviceAddress,m_SUCCESS);
				taskSuccess('c');
			}
			else				//response packet transmission failed
			{
				//be careful, we use MACenum to return the status, either CHANNEL_ACCESS_FAILURE or NO_ACK
				sscs->MLME_COMM_STATUS_indication(mpib.macPANId,defFrmCtrl_AddrMode64,aExtendedAddress,defFrmCtrl_AddrMode64,DeviceAddress,Status);
				freePkt(txBcnCmd);
				txBcnCmd = 0;
			}
			break;
		case 2:
			if (!taskP.taskStatus(task))	
				break;
			taskP.taskStatus(task) = false;
			//check if the transaction still pending -- actually no need to check (it must be pending if case 1 didn't happen), but no harm
			i = updateTransacLinkByPktOrHandle(tr_oper_est,&transacLink1,&transacLink2,taskP.mlme_associate_response_pendPkt);	//don't use <txBcnCmd>, since assignment 'txBcnCmd = rspPkt' only happens if a data request command received
			if (i == 0)	//still pending
			{
				//delete the packet from the transaction list immediately -- prevent the packet from being transmitted at the last moment
				updateTransacLinkByPktOrHandle(tr_oper_del,&transacLink1,&transacLink2,taskP.mlme_associate_response_pendPkt);
				sscs->MLME_COMM_STATUS_indication(mpib.macPANId,defFrmCtrl_AddrMode64,aExtendedAddress,defFrmCtrl_AddrMode64,DeviceAddress,m_TRANSACTION_EXPIRED);
				return;
			}
			else	//being successfully extracted
			{
				sscs->MLME_COMM_STATUS_indication(mpib.macPANId,defFrmCtrl_AddrMode64,aExtendedAddress,defFrmCtrl_AddrMode64,DeviceAddress,m_SUCCESS);
				return;
			}
			break;
		default:
			break;
	}
}

//void Mac802_15_4::mlme_disassociate_request(IE3ADDR DeviceAddress,UINT_8 DisassociateReason,bool SecurityEnable,bool frUpper,PHYenum status)
void Mac802_15_4::mlme_disassociate_request(IE3ADDR , UINT_8 ,bool ,bool , PHYenum )
{
	/*
	   FrameCtrl frmCtrl;
	   hdr_lrwpan* wph;
	   double kpTime;
	   UINT_8 step,task;
	   int i;

	   task = TP_mlme_disassociate_request;
	   if (frUpper) checkTaskOverflow(task);

	   step = taskP.taskStep(task);
	   switch(step)
	   {
	   case 0:
	//check if parameters valid or not
	if (DeviceAddress != mpib.macCoordExtendedAddress)		//send to a device
	if ((!capability.FFD)||(numberDeviceLink(&deviceLink1) == 0))	//I am not a coordinator

	{
	sscs->MLME_DISASSOCIATE_confirm(m_INVALID_PARAMETER);
	return;
	}
	taskP.mlme_disassociate_request_toCoor = (DeviceAddress == mpib.macCoordExtendedAddress);
	//--- construct a disassociation notification command packet ---
#ifdef DEBUG802_15_4
fprintf(stdout,"[%s::%s][%f](node %d) before alloc txBcnCmd2:\n\t\ttxBeacon\t= %ld\n\t\ttxAck   \t= %ld\n\t\ttxBcnCmd\t= %ld\n\t\ttxBcnCmd2\t= %ld\n\t\ttxData  \t= %ld\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,txBeacon,txAck,txBcnCmd,txBcnCmd2,txData);
#endif
assert(!txBcnCmd2);
txBcnCmd2 = Packet::alloc();
assert(txBcnCmd2);
wph = HDR_LRWPAN(txBcnCmd2);
if (!taskP.mlme_disassociate_request_toCoor)
wph->MHR_DstAddrInfo.addr_64 = DeviceAddress;
else
wph->MHR_DstAddrInfo.addr_64 = mpib.macCoordExtendedAddress;
constructCommandHeader(txBcnCmd2,&frmCtrl,0x03,defFrmCtrl_AddrMode64,mpib.macPANId,wph->MHR_DstAddrInfo.addr_64,defFrmCtrl_AddrMode64,mpib.macPANId,aExtendedAddress,SecurityEnable,false,true);
	 *((UINT_8 *)wph->MSDU_Payload) = (taskP.mlme_disassociate_request_toCoor)?0x02:0x01;
	 constructMPDU(2,txBcnCmd2,frmCtrl.FrmCtrl,mpib.macDSN++,wph->MHR_DstAddrInfo,wph->MHR_SrcAddrInfo,0,0x03,0);
	//----------------------------------------------------------------------
	taskP.taskStatus(task) = true;
	taskP.taskStep(task)++;
	taskP.mlme_disassociate_request_pendPkt = txBcnCmd2;
	if (!taskP.mlme_disassociate_request_toCoor)		//indirect transmission should be used
	{
	// Linux floating number compatibility
	//kpTime = mpib.macTransactionPersistenceTime * (aBaseSuperframeDuration * (1 << mpib.macBeaconOrder) / phy->getRate('s'));
	{
	double tmpf;
	tmpf = (aBaseSuperframeDuration * (1 << mpib.macBeaconOrder) / phy->getRate('s'));
	kpTime = mpib.macTransactionPersistenceTime * tmpf;
	}

	i = chkAddTransacLink(&transacLink1,&transacLink2,defFrmCtrl_AddrMode64,wph->MHR_DstAddrInfo.addr_64,txBcnCmd2,0,kpTime);
	if (i != 0)	//overflow or failed
	{
	taskP.taskStatus(task) = false;
	Packet::free(txBcnCmd2);
	txBcnCmd2 = 0;
	sscs->MLME_DISASSOCIATE_confirm(m_TRANSACTION_OVERFLOW);
	return;
	}
	extractT->start(kpTime,false);
	}
	else
	csmacaBegin('C');
	break;
	case 1:
	if (!taskP.mlme_disassociate_request_toCoor)		//indirect transmission
	{
	//check if the transaction still pending
	wph = HDR_LRWPAN(txBcnCmd2);
	i = updateTransacLinkByPktOrHandle(tr_oper_est,&transacLink1,&transacLink2,taskP.mlme_disassociate_request_pendPkt);	//don't use <txBcnCmd2>, since it may be null if a data request command not received
	if (i == 0)	//still pending
	{
		//delete the packet from the transaction list immediately -- prevent the packet from being transmitted at the last moment
		updateTransacLinkByPktOrHandle(tr_oper_del,&transacLink1,&transacLink2,taskP.mlme_disassociate_request_pendPkt);
		taskP.taskStatus(task) = false;
		sscs->MLME_DISASSOCIATE_confirm(m_TRANSACTION_EXPIRED);
		return;
	}
	else	//being successfully extracted
	{
		taskP.taskStatus(task) = false;
		sscs->MLME_COMM_STATUS_indication(mpib.macPANId,defFrmCtrl_AddrMode64,aExtendedAddress,defFrmCtrl_AddrMode64,DeviceAddress,m_SUCCESS);
		return;
	}
}
else
{
}
break;
default:
break;
}
*/
}

void Mac802_15_4::mlme_orphan_response(IE3ADDR OrphanAddress,UINT_16 ShortAddress,bool AssociatedMember,bool SecurityEnable,bool frUpper,PHYenum status)
{
	hdr_lrwpan* wph;
	FrameCtrl frmCtrl;
	//UINT_8 step;
	UINT_8 task;

	task = TP_mlme_orphan_response;
	if (frUpper) checkTaskOverflow(task);

	switch(taskP.taskStep(task))
	{
		case 0:
			if (AssociatedMember)
			{
				//send a coordinator realignment command
#ifdef DEBUG802_15_4
				fprintf(stdout,"[%s::%s][%f](node %d) before alloc txBcnCmd:\n\t\ttxBeacon\t= %ld\n\t\ttxAck   \t= %ld\n\t\ttxBcnCmd\t= %ld\n\t\ttxBcnCmd2\t= %ld\n\t\ttxData  \t= %ld\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,txBeacon,txAck,txBcnCmd,txBcnCmd2,txData);
#endif
				taskP.taskStatus(task) = true;
				taskP.taskStep(task)++;
				strcpy(taskP.taskFrFunc(task),"csmacaCallBack");
				taskP.mlme_orphan_response_OrphanAddress = OrphanAddress;
				assert(!txBcnCmd);
				txBcnCmd = Packet::alloc();
				assert(txBcnCmd);
				wph = HDR_LRWPAN(txBcnCmd);
				constructCommandHeader(txBcnCmd,&frmCtrl,0x08,defFrmCtrl_AddrMode64,0xffff,OrphanAddress,defFrmCtrl_AddrMode64,mpib.macPANId,aExtendedAddress,SecurityEnable,false,true);
				*((UINT_16 *)wph->MSDU_Payload) = mpib.macPANId;
				*((UINT_16 *)wph->MSDU_Payload + 2) = mpib.macShortAddress;
				phy->PLME_GET_request(phyCurrentChannel);
				*((UINT_8 *)wph->MSDU_Payload + 4) = tmp_ppib.phyCurrentChannel;
				*((UINT_16 *)wph->MSDU_Payload + 5) = ShortAddress;
				constructMPDU(8,txBcnCmd,frmCtrl.FrmCtrl,mpib.macDSN++,wph->MHR_DstAddrInfo,wph->MHR_SrcAddrInfo,0,0x08,0);
				csmacaBegin('c');
			}
			break;
		case 1:
			taskP.taskStatus(task) = false;
			if (status == p_SUCCESS)	//response packet transmitted and ack. received
			{
				sscs->MLME_COMM_STATUS_indication(mpib.macPANId,defFrmCtrl_AddrMode64,aExtendedAddress,defFrmCtrl_AddrMode64,OrphanAddress,m_SUCCESS);
				taskSuccess('c');
			}
			else				//response packet transmission failed
			{
				sscs->MLME_COMM_STATUS_indication(mpib.macPANId,defFrmCtrl_AddrMode64,aExtendedAddress,defFrmCtrl_AddrMode64,OrphanAddress,m_CHANNEL_ACCESS_FAILURE);
				freePkt(txBcnCmd);
				txBcnCmd = 0;
				csmacaResume();		//other packets may be waiting
			}
			break;
		default:
			break;
	}
}

void Mac802_15_4::mlme_reset_request(bool SetDefaultPIB,bool frUpper,PHYenum status)
{
	//UINT_8 step;
	UINT_8 task;

	task = TP_mlme_reset_request;
	if (frUpper) checkTaskOverflow(task);

	switch(taskP.taskStep(task))
	{
		case 0:
			taskP.taskStatus(task) = true;
			taskP.taskStep(task)++;
			strcpy(taskP.taskFrFunc(task),"PLME_SET_TRX_STATE_confirm");
			taskP.mlme_reset_request_SetDefaultPIB = SetDefaultPIB;
			{plme_set_trx_state_request(p_TRX_OFF);}
			break;
		case 1:
			taskP.taskStatus(task) = false;
			init(true);
			if (SetDefaultPIB)
				mpib = MPIB;
			if (status == p_TRX_OFF)
				sscs->MLME_RESET_confirm(m_SUCCESS);
			else
				sscs->MLME_RESET_confirm(m_DISABLE_TRX_FAILURE);
			break;
		default:
			break;
	}
}

void Mac802_15_4::mlme_rx_enable_request(bool DeferPermit,UINT_32 RxOnTime,UINT_32 RxOnDuration,bool frUpper,PHYenum status)
{
	UINT_8 step,task;
	UINT_32 t_CAP;
	double cutTime,tmpf;

	task = TP_mlme_rx_enable_request;
	if (frUpper) checkTaskOverflow(task);

	step = taskP.taskStep(task);

	if (step == 0)
		if (RxOnDuration == 0)
		{
			sscs->MLME_RX_ENABLE_confirm(m_SUCCESS);
			plme_set_trx_state_request(p_TRX_OFF);
			return;
		}

	if (macBeaconOrder2 != 15)		//beacon enabled
	{
		switch(step)
		{
			case 0:
				taskP.mlme_rx_enable_request_RxOnTime = RxOnTime;
				taskP.mlme_rx_enable_request_RxOnDuration = RxOnDuration;
				if (RxOnTime + RxOnDuration >= sfSpec2.BI)
				{
					sscs->MLME_RX_ENABLE_confirm(m_INVALID_PARAMETER);
					return;
				}
				t_CAP = (sfSpec2.FinCAP + 1) * sfSpec2.sd;

				/* Linux floating number compatibility
				*/
				tmpf = CURRENT_TIME * phy->getRate('s');

				if ((RxOnTime - aTurnaroundTime) > t_CAP)
				{
					sscs->MLME_RX_ENABLE_confirm(m_OUT_OF_CAP);
					return;
				}
				/* Linux floating number compatibility
				   else if ((CURRENT_TIME * phy->getRate('s') - macBcnRxTime) < (RxOnTime - aTurnaroundTime))
				   */
				else if ((tmpf - macBcnRxTime) < (RxOnTime - aTurnaroundTime))
				{
					//can proceed in current superframe
					taskP.taskStatus(task) = true;
					taskP.taskStep(task)++;
					//just fall through case 1
				}
				else if (DeferPermit)
				{
					//need to defer until next superframe
					taskP.taskStatus(task) = true;
					taskP.taskStep(task)++;
					strcpy(taskP.taskFrFunc(task),"recvBeacon");
					break;
				}
				else
				{
					sscs->MLME_RX_ENABLE_confirm(m_OUT_OF_CAP);
					return;
				}
			case 1:
				taskP.taskStep(task)++;
				strcpy(taskP.taskFrFunc(task),"rxEnableHandler");
				/* Linux floating number compatibility
				   rxEnableT->start(RxOnTime / phy->getRate('s') - (CURRENT_TIME - macBcnRxTime / phy->getRate('s')));
				   */
				{
					double tmpf2;
					tmpf = macBcnRxTime / phy->getRate('s');
					tmpf = CURRENT_TIME - tmpf;
					tmpf2 = RxOnTime / phy->getRate('s');
					tmpf = tmpf2 - tmpf;
					rxEnableT->start(tmpf);
				}
				break;
			case 2:
				taskP.taskStep(task)++;
				strcpy(taskP.taskFrFunc(task),"PLME_SET_TRX_STATE_confirm");
				taskP.mlme_rx_enable_request_currentTime = CURRENT_TIME;
				plme_set_trx_state_request(p_RX_ON);
				break;
			case 3:
				taskP.taskStatus(task) = false;	
				strcpy(taskP.taskFrFunc(task),"rxEnableHandler");
				taskP.taskStep(task)++;
				if (status == p_TX_ON)
					sscs->MLME_RX_ENABLE_confirm(m_TX_ACTIVE);
				else
					sscs->MLME_RX_ENABLE_confirm(m_SUCCESS);
				//turn off the receiver before the CFP so as not to disturb it, and we see no reason to turn it on again after the CFP (i.e., inactive port of the superframe)
				t_CAP = (sfSpec2.FinCAP + 1) * sfSpec2.sd;
				cutTime = (RxOnTime + RxOnDuration - t_CAP) / phy->getRate('s');

				/* Linux floating number compatibility
				   rxEnableT->start(RxOnDuration / phy->getRate('s') - (CURRENT_TIME - taskP.mlme_rx_enable_request_currentTime) - cutTime);
				   */
				{
					tmpf = RxOnDuration / phy->getRate('s');
					tmpf -= CURRENT_TIME;
					tmpf += taskP.mlme_rx_enable_request_currentTime;
					tmpf -= cutTime;
					rxEnableT->start(tmpf);
				}
				break;
			case 4:
				strcpy(taskP.taskFrFunc(task),"");
				plme_set_trx_state_request(p_TRX_OFF);
				break;
			default:
				break;
		}
	}
	else
	{
		switch(step)
		{
			case 0:
				taskP.taskStatus(task) = true;
				strcpy(taskP.taskFrFunc(task),"PLME_SET_TRX_STATE_confirm");
				taskP.taskStep(task)++;
				taskP.mlme_rx_enable_request_RxOnDuration = RxOnDuration;
				taskP.mlme_rx_enable_request_currentTime = CURRENT_TIME;
				plme_set_trx_state_request(p_RX_ON);
				break;
			case 1:
				taskP.taskStatus(task) = false;	
				strcpy(taskP.taskFrFunc(task),"rxEnableHandler");
				taskP.taskStep(task)++;
				if (status == p_TX_ON)
					sscs->MLME_RX_ENABLE_confirm(m_TX_ACTIVE);
				else
					sscs->MLME_RX_ENABLE_confirm(m_SUCCESS);
				/* Linux floating number compatibility
				   rxEnableT->start(RxOnDuration / phy->getRate('s') - (CURRENT_TIME - taskP.mlme_rx_enable_request_currentTime));
				   */
				{
					tmpf = RxOnDuration / phy->getRate('s');
					tmpf -= CURRENT_TIME;
					tmpf += taskP.mlme_rx_enable_request_currentTime;
					rxEnableT->start(tmpf);
				}
				break;
			case 2:
				strcpy(taskP.taskFrFunc(task),"");
				plme_set_trx_state_request(p_TRX_OFF);
				break;
			default:
				break;
		}
	}
}

void Mac802_15_4::mlme_scan_request(UINT_8 ScanType,UINT_32 ScanChannels,UINT_8 ScanDuration,bool frUpper,PHYenum status)
{
	UINT_32 t_chanPos;
	UINT_8 step,task;
	FrameCtrl frmCtrl;
	hdr_lrwpan* wph;
	int i;

	task = TP_mlme_scan_request;
	if (frUpper) checkTaskOverflow(task);

	step = taskP.taskStep(task);

	if (step == 0)
	{
		if ((ScanType > 3)
				||((ScanType != 3)&&(ScanDuration > 14)))
		{
			sscs->MLME_SCAN_confirm(m_INVALID_PARAMETER,ScanType,ScanChannels,0,NULL,NULL);
			return;
		}
		//disable the beacon
		taskP.mlme_scan_request_orig_macBeaconOrder = mpib.macBeaconOrder;
		taskP.mlme_scan_request_orig_macBeaconOrder2 = macBeaconOrder2;
		taskP.mlme_scan_request_orig_macBeaconOrder3 = macBeaconOrder3;
		mpib.macBeaconOrder = 15;
		macBeaconOrder2 = 15;
		macBeaconOrder3 = 15;
		//stop the CSMA-CA if it is running
		if (backoffStatus == 99)
		{
			backoffStatus = 0;
			csmaca->cancel();
		}
		taskP.mlme_scan_request_ScanType = ScanType;
	}

	if (ScanType == 0x00)		//ED scan
		switch (step)
		{
			case 0:
				phy->PLME_GET_request(phyChannelsSupported);	//value will be returned in tmp_ppib
				taskP.mlme_scan_request_ScanChannels = ScanChannels;
				if ((taskP.mlme_scan_request_ScanChannels & tmp_ppib.phyChannelsSupported) == 0)
				{
					//restore the beacon order
					mpib.macBeaconOrder = taskP.mlme_scan_request_orig_macBeaconOrder;
					macBeaconOrder2 = taskP.mlme_scan_request_orig_macBeaconOrder2;
					macBeaconOrder3 = taskP.mlme_scan_request_orig_macBeaconOrder3;
					sscs->MLME_SCAN_confirm(m_SUCCESS,ScanType,ScanChannels,0,NULL,NULL);	//SUCCESS or INVALID_PARAMETER?
					csmacaResume();
					return;
				}
				taskP.taskStatus(task) = true;
				taskP.taskStep(task)++;
				strcpy(taskP.taskFrFunc(task),"PLME_SET_confirm");
				taskP.mlme_scan_request_CurrentChannel = 0;
				taskP.mlme_scan_request_ListNum = 0;
				t_chanPos = (1<<taskP.mlme_scan_request_CurrentChannel);
				while((t_chanPos & taskP.mlme_scan_request_ScanChannels) == 0
						||(t_chanPos & tmp_ppib.phyChannelsSupported) == 0)
				{
					taskP.mlme_scan_request_CurrentChannel++;
					t_chanPos = (1<<taskP.mlme_scan_request_CurrentChannel);
				}
				tmp_ppib.phyCurrentChannel = taskP.mlme_scan_request_CurrentChannel;
				phy->PLME_SET_request(phyCurrentChannel,&tmp_ppib);
				break;
			case 1:
				taskP.taskStep(task)++;
				strcpy(taskP.taskFrFunc(task),"PLME_SET_TRX_STATE_confirm");
				{plme_set_trx_state_request(p_RX_ON);}
				break;
			case 2:
				if (status == p_RX_ON)
				{
					taskP.taskStep(task)++;
					strcpy(taskP.taskFrFunc(task),"PLME_ED_confirm");
					phy->PLME_ED_request();
					break;
				}
				//else	//fall through case 4
			case 3:
				if (step == 3)	//note that case 2 needs to fall through case 4 via here
				{
					if (status == p_SUCCESS)
					{
						t_chanPos = (1<<taskP.mlme_scan_request_CurrentChannel);
						taskP.mlme_scan_request_ScanChannels &= (t_chanPos^0xffffffff);
						taskP.mlme_scan_request_EnergyDetectList[taskP.mlme_scan_request_ListNum] = energyLevel;
						taskP.mlme_scan_request_ListNum++;
					}
				}
				//fall through
			case 4:
				if ((taskP.mlme_scan_request_ScanChannels & tmp_ppib.phyChannelsSupported) == 0)
				{
					//restore the beacon order
					mpib.macBeaconOrder = taskP.mlme_scan_request_orig_macBeaconOrder;
					macBeaconOrder2 = taskP.mlme_scan_request_orig_macBeaconOrder2;
					macBeaconOrder3 = taskP.mlme_scan_request_orig_macBeaconOrder3;
					taskP.taskStatus(task) = false;
					sscs->MLME_SCAN_confirm(m_SUCCESS,ScanType,taskP.mlme_scan_request_ScanChannels,taskP.mlme_scan_request_ListNum,taskP.mlme_scan_request_EnergyDetectList,NULL);
					csmacaResume();
					return;
				}
				taskP.taskStep(task) = 1;	//important
				strcpy(taskP.taskFrFunc(task),"PLME_SET_confirm");
				taskP.mlme_scan_request_CurrentChannel++;
				t_chanPos = (1<<taskP.mlme_scan_request_CurrentChannel);
				while((t_chanPos & taskP.mlme_scan_request_ScanChannels) == 0
						||(t_chanPos & tmp_ppib.phyChannelsSupported) == 0)
				{
					taskP.mlme_scan_request_CurrentChannel++;
					t_chanPos = (1<<taskP.mlme_scan_request_CurrentChannel);
				}
				tmp_ppib.phyCurrentChannel = taskP.mlme_scan_request_CurrentChannel;
				phy->PLME_SET_request(phyCurrentChannel,&tmp_ppib);
				break;
			default:
				break;
		}

	else if ((ScanType == 0x01)	//active scan
			||	 (ScanType == 0x02))	//passive scan
		switch (step)
		{
			case 0:
				phy->PLME_GET_request(phyChannelsSupported);	//value will be returned in tmp_ppib
				taskP.mlme_scan_request_ScanChannels = ScanChannels;
				if ((taskP.mlme_scan_request_ScanChannels & tmp_ppib.phyChannelsSupported) == 0)
				{
					mpib.macBeaconOrder = taskP.mlme_scan_request_orig_macBeaconOrder;
					macBeaconOrder2 = taskP.mlme_scan_request_orig_macBeaconOrder2;
					macBeaconOrder3 = taskP.mlme_scan_request_orig_macBeaconOrder3;
					sscs->MLME_SCAN_confirm(m_SUCCESS,ScanType,ScanChannels,0,NULL,NULL);	//SUCCESS or INVALID_PARAMETER?
					csmacaResume();
					return;
				}
				taskP.taskStatus(task) = true;
				taskP.taskStep(task)++;
				strcpy(taskP.taskFrFunc(task),"PLME_SET_confirm");
				taskP.mlme_scan_request_orig_macPANId = mpib.macPANId;
				mpib.macPANId = 0xffff;
				taskP.mlme_scan_request_ScanDuration = ScanDuration;
				taskP.mlme_scan_request_CurrentChannel = 0;
				taskP.mlme_scan_request_ListNum = 0;
				t_chanPos = (1<<taskP.mlme_scan_request_CurrentChannel);
				while((t_chanPos & taskP.mlme_scan_request_ScanChannels) == 0
						||(t_chanPos & tmp_ppib.phyChannelsSupported) == 0)
				{
					taskP.mlme_scan_request_CurrentChannel++;
					t_chanPos = (1<<taskP.mlme_scan_request_CurrentChannel);
				}
				tmp_ppib.phyCurrentChannel = taskP.mlme_scan_request_CurrentChannel;
				phy->PLME_SET_request(phyCurrentChannel,&tmp_ppib);
				break;
			case 1:
				if (Mac802_15_4::verbose)
					fprintf(stdout,"[%f](node %d) scanning channel %d\n",CURRENT_TIME,index_,taskP.mlme_scan_request_CurrentChannel);
				if (ScanType == 0x01)		//active scan
				{
					taskP.taskStep(task)++;
					strcpy(taskP.taskFrFunc(task),"csmacaCallBack");
					//--- send a beacon request command ---
#ifdef DEBUG802_15_4
					fprintf(stdout,"[%s::%s][%f](node %d) before alloc txBcnCmd2:\n\t\ttxBeacon\t= %ld\n\t\ttxAck   \t= %ld\n\t\ttxBcnCmd\t= %ld\n\t\ttxBcnCmd2\t= %ld\n\t\ttxData  \t= %ld\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,txBeacon,txAck,txBcnCmd,txBcnCmd2,txData);
#endif
					assert(!txBcnCmd2);
					txBcnCmd2 = Packet::alloc();
					assert(txBcnCmd2);
					wph = HDR_LRWPAN(txBcnCmd2);
					constructCommandHeader(txBcnCmd2,&frmCtrl,0x07,defFrmCtrl_AddrMode16,0xffff,0xffff,defFrmCtrl_AddrModeNone,0,0,false,false,false);
					constructMPDU(1,txBcnCmd2,frmCtrl.FrmCtrl,mpib.macDSN++,wph->MHR_DstAddrInfo,wph->MHR_SrcAddrInfo,0,0x07,0);
					csmacaBegin('C');
					//------------------------------------
				}
				else
				{
					taskP.taskStep(task) = 4;	//skip the steps only for active scan
					strcpy(taskP.taskFrFunc(task),"PLME_SET_TRX_STATE_confirm");
					plme_set_trx_state_request(p_RX_ON);
				}
				break;
			case 2:
				if (status == p_IDLE)
				{
					taskP.taskStep(task)++;
					strcpy(taskP.taskFrFunc(task),"PD_DATA_confirm");
					plme_set_trx_state_request(p_TX_ON);
					break;
				}
				else
				{
					freePkt(txBcnCmd2);	//actually we can keep <txBcnCmd2> for next channel
					txBcnCmd2 = 0;
					//fall through case 7
				}
			case 3:
				if (step == 3)
				{
					taskP.taskStep(task)++;
					strcpy(taskP.taskFrFunc(task),"PLME_SET_TRX_STATE_confirm");
					taskSuccess('C',false);
					plme_set_trx_state_request(p_RX_ON);
					break;
				}
			case 4:
				if (step == 4)
				{
					if (status == p_RX_ON)
					{
						taskP.taskStep(task)++;
						strcpy(taskP.taskFrFunc(task),"recvBeacon");
						//schedule for next channel
						scanT->start((aBaseSuperframeDuration * ((1 << taskP.mlme_scan_request_ScanDuration) + 1)) / phy->getRate('s'));
						break;
					}
					//else	//fall through case 7
				}
			case 5:
				if (step == 5)
				{
					//beacon received
					//record the PAN descriptor if it is a new one
					assert(rxBeacon);
					wph = HDR_LRWPAN(rxBeacon);
					frmCtrl.FrmCtrl = wph->MHR_FrmCtrl;
					frmCtrl.parse();
					for (i=0;i<taskP.mlme_scan_request_ListNum;i++)
					{
						if ((taskP.mlme_scan_request_PANDescriptorList[i].LogicalChannel == taskP.mlme_scan_request_CurrentChannel)
								&& (taskP.mlme_scan_request_PANDescriptorList[i].CoordAddrMode == frmCtrl.srcAddrMode)
								&& (taskP.mlme_scan_request_PANDescriptorList[i].CoordPANId == wph->MHR_SrcAddrInfo.panID)		//but (page 146, line 4-5) implies not checking PAN ID
								&& (
									((frmCtrl.srcAddrMode == defFrmCtrl_AddrMode16)&&(taskP.mlme_scan_request_PANDescriptorList[i].CoordAddress_16 == wph->MHR_SrcAddrInfo.addr_16))
									||
									((frmCtrl.srcAddrMode == defFrmCtrl_AddrMode64)&&(taskP.mlme_scan_request_PANDescriptorList[i].CoordAddress_64 == wph->MHR_SrcAddrInfo.addr_64))
								   )
						   )
						{
							break;
						}
					}

					if (i >= taskP.mlme_scan_request_ListNum)	//unique beacon
					{
						taskP.mlme_scan_request_PANDescriptorList[taskP.mlme_scan_request_ListNum] = panDes2;
						taskP.mlme_scan_request_ListNum++;
						if (taskP.mlme_scan_request_ListNum >= 27)
						{
							//stop the timer
							scanT->stop();
							//fall through case 7
						}
						else
						{
							break;
						}
					}
					else
					{
						break;
					}
				}
			case 6:
				if (step == 6)
				{
					t_chanPos = (1<<taskP.mlme_scan_request_CurrentChannel);
					taskP.mlme_scan_request_ScanChannels &= (t_chanPos^0xffffffff);
					//fall through case 7
				}
			case 7:
				if (((taskP.mlme_scan_request_ScanChannels & tmp_ppib.phyChannelsSupported) == 0)
						||(taskP.mlme_scan_request_ListNum >= 27))
				{
					mpib.macPANId = taskP.mlme_scan_request_orig_macPANId;
					mpib.macBeaconOrder = taskP.mlme_scan_request_orig_macBeaconOrder;
					macBeaconOrder2 = taskP.mlme_scan_request_orig_macBeaconOrder2;
					macBeaconOrder3 = taskP.mlme_scan_request_orig_macBeaconOrder3;
					taskP.taskStatus(task) = false;
					sscs->MLME_SCAN_confirm(m_SUCCESS,ScanType,taskP.mlme_scan_request_ScanChannels,taskP.mlme_scan_request_ListNum,NULL,taskP.mlme_scan_request_PANDescriptorList);
					csmacaResume();
					return;
				}
				taskP.taskStep(task) = 1;	//important
				strcpy(taskP.taskFrFunc(task),"PLME_SET_confirm");
				taskP.mlme_scan_request_CurrentChannel++;
				t_chanPos = (1<<taskP.mlme_scan_request_CurrentChannel);
				while((t_chanPos & taskP.mlme_scan_request_ScanChannels) == 0
						||(t_chanPos & tmp_ppib.phyChannelsSupported) == 0)
				{
					taskP.mlme_scan_request_CurrentChannel++;
					t_chanPos = (1<<taskP.mlme_scan_request_CurrentChannel);
				}
				tmp_ppib.phyCurrentChannel = taskP.mlme_scan_request_CurrentChannel;
				phy->PLME_SET_request(phyCurrentChannel,&tmp_ppib);
				break;
			default:
				break;
		}

	else //if (ScanType == 0x03)	//orphan scan
		switch (step)
		{
			case 0:
				phy->PLME_GET_request(phyChannelsSupported);	//value will be returned in tmp_ppib
				taskP.mlme_scan_request_ScanChannels = ScanChannels;
				if ((taskP.mlme_scan_request_ScanChannels & tmp_ppib.phyChannelsSupported) == 0)
				{
					mpib.macBeaconOrder = taskP.mlme_scan_request_orig_macBeaconOrder;
					//macBeaconOrder2 = taskP.mlme_scan_request_orig_macBeaconOrder2;
					macBeaconOrder2 = 15;
					macBeaconOrder3 = taskP.mlme_scan_request_orig_macBeaconOrder3;
					sscs->MLME_SCAN_confirm(m_INVALID_PARAMETER,ScanType,ScanChannels,0,NULL,NULL);
					csmacaResume();
					return;
				}
				taskP.taskStatus(task) = true;
				taskP.taskStep(task)++;
				strcpy(taskP.taskFrFunc(task),"PLME_SET_confirm");
				taskP.mlme_scan_request_CurrentChannel = 0;
				t_chanPos = (1<<taskP.mlme_scan_request_CurrentChannel);
				while((t_chanPos & taskP.mlme_scan_request_ScanChannels) == 0
						||(t_chanPos & tmp_ppib.phyChannelsSupported) == 0)
				{
					taskP.mlme_scan_request_CurrentChannel++;
					t_chanPos = (1<<taskP.mlme_scan_request_CurrentChannel);
				}
				tmp_ppib.phyCurrentChannel = taskP.mlme_scan_request_CurrentChannel;
				phy->PLME_SET_request(phyCurrentChannel,&tmp_ppib);
				break;
			case 1:
				if (Mac802_15_4::verbose)
					fprintf(stdout,"[%f](node %d) orphan-scanning channel %d\n",CURRENT_TIME,index_,taskP.mlme_scan_request_CurrentChannel);
				taskP.taskStep(task)++;
				strcpy(taskP.taskFrFunc(task),"csmacaCallBack");
				//--- send an orphan notification command ---
#ifdef DEBUG802_15_4
				fprintf(stdout,"[%s::%s][%f](node %d) before alloc txBcnCmd2:\n\t\ttxBeacon\t= %ld\n\t\ttxAck   \t= %ld\n\t\ttxBcnCmd\t= %ld\n\t\ttxBcnCmd2\t= %ld\n\t\ttxData  \t= %ld\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,txBeacon,txAck,txBcnCmd,txBcnCmd2,txData);
#endif
				assert(!txBcnCmd2);
				txBcnCmd2 = Packet::alloc();
				assert(txBcnCmd2);
				wph = HDR_LRWPAN(txBcnCmd2);
				constructCommandHeader(txBcnCmd2,&frmCtrl,0x06,defFrmCtrl_AddrMode64,mpib.macPANId,mpib.macCoordExtendedAddress,defFrmCtrl_AddrMode64,mpib.macPANId,aExtendedAddress,false,false,false);
				constructMPDU(1,txBcnCmd2,frmCtrl.FrmCtrl,mpib.macDSN++,wph->MHR_DstAddrInfo,wph->MHR_SrcAddrInfo,0,0x06,0);
				csmacaBegin('C');
				//------------------------------------
				break;
			case 2:
				if (status == p_IDLE)
				{
					taskP.taskStep(task)++;
					strcpy(taskP.taskFrFunc(task),"PD_DATA_confirm");
					plme_set_trx_state_request(p_TX_ON);
					break;
				}
				else
				{
					freePkt(txBcnCmd2);
					txBcnCmd2 = 0;
					//fall through case 6
				}
			case 3:
				if (step == 3)
				{
					taskP.taskStep(task)++;
					strcpy(taskP.taskFrFunc(task),"PLME_SET_TRX_STATE_confirm");
					taskSuccess('C',false);
					plme_set_trx_state_request(p_RX_ON);
					break;
				}
			case 4:
				if (step == 4)
				{
					if (status == p_RX_ON)
					{
						taskP.taskStep(task)++;
						strcpy(taskP.taskFrFunc(task),"IFSHandler");
						scanT->start(aResponseWaitTime / phy->getRate('s'));
						break;
					}
					//else	//fall through case 6
				}
			case 5:
				if (step == 5)
				{
					if (status == p_SUCCESS)	//coordinator realignment command received
					{
						scanT->stop();
						mpib.macBeaconOrder = taskP.mlme_scan_request_orig_macBeaconOrder;
						macBeaconOrder2 = taskP.mlme_scan_request_orig_macBeaconOrder2;
						macBeaconOrder3 = taskP.mlme_scan_request_orig_macBeaconOrder3;
						taskP.taskStatus(task) = false;
						changeNodeColor(CURRENT_TIME,(mpib.macAssociationPermit)?Nam802_15_4::def_Coor_clr:Nam802_15_4::def_Dev_clr);
						t_chanPos = (1<<taskP.mlme_scan_request_CurrentChannel);
						taskP.mlme_scan_request_ScanChannels &= (t_chanPos^0xffffffff);
						sscs->MLME_SCAN_confirm(m_SUCCESS,ScanType,taskP.mlme_scan_request_ScanChannels,0,NULL,NULL);
						csmacaResume();
						break;
					}
					else	//time out
					{
						t_chanPos = (1<<taskP.mlme_scan_request_CurrentChannel);
						taskP.mlme_scan_request_ScanChannels &= (t_chanPos^0xffffffff);
						//fall through case 6
					}
				}
			case 6:
				if ((taskP.mlme_scan_request_ScanChannels & tmp_ppib.phyChannelsSupported) == 0)
				{
					mpib.macBeaconOrder = taskP.mlme_scan_request_orig_macBeaconOrder;
					//macBeaconOrder2 = taskP.mlme_scan_request_orig_macBeaconOrder2;
					macBeaconOrder2 = 15;
					macBeaconOrder3 = taskP.mlme_scan_request_orig_macBeaconOrder3;
					taskP.taskStatus(task) = false;
					sscs->MLME_SCAN_confirm(m_NO_BEACON,ScanType,taskP.mlme_scan_request_ScanChannels,0,NULL,NULL);
					csmacaResume();
					return;
				}
				taskP.taskStep(task) = 1;	//important
				strcpy(taskP.taskFrFunc(task),"PLME_SET_confirm");
				taskP.mlme_scan_request_CurrentChannel++;
				t_chanPos = (1<<taskP.mlme_scan_request_CurrentChannel);
				while((t_chanPos & taskP.mlme_scan_request_ScanChannels) == 0
						||(t_chanPos & tmp_ppib.phyChannelsSupported) == 0)
				{
					taskP.mlme_scan_request_CurrentChannel++;
					t_chanPos = (1<<taskP.mlme_scan_request_CurrentChannel);
				}
				tmp_ppib.phyCurrentChannel = taskP.mlme_scan_request_CurrentChannel;
				phy->PLME_SET_request(phyCurrentChannel,&tmp_ppib);
				break;
			default:
				break;
		}
}

void Mac802_15_4::mlme_start_request(UINT_16 PANId,UINT_8 LogicalChannel,UINT_8 BeaconOrder,
		UINT_8 SuperframeOrder,bool PANCoordinator,bool BatteryLifeExtension,
		bool CoordRealignment,bool SecurityEnable,
		bool frUpper,PHYenum status)
{
	FrameCtrl frmCtrl;
	hdr_lrwpan* wph;
	UINT_8 origBeaconOrder;
	UINT_8 step,task;

	task = TP_mlme_start_request;
	if (frUpper) checkTaskOverflow(task);

	step = taskP.taskStep(task);
	switch (step)
	{
		case 0:
			if (mpib.macShortAddress == 0xffff)
			{
				sscs->MLME_START_confirm(m_NO_SHORT_ADDRESS);
				return;
			}
			else if ((!phy->channelSupported(LogicalChannel))
					|| (BeaconOrder > 15)
					|| ((SuperframeOrder > BeaconOrder)&&(SuperframeOrder != 15)))
			{
				sscs->MLME_START_confirm(m_INVALID_PARAMETER);
				return;
			}
			else if (!capability.FFD)
			{
				sscs->MLME_START_confirm(m_UNDEFINED);
				return;
			}
			taskP.taskStatus(task) = true;
			if (CoordRealignment)		//send a realignment command before changing configuration that affects the command
			{
				taskP.taskStep(task)++;
				strcpy(taskP.taskFrFunc(task),"csmacaCallBack");
				//broadcast a realignment command
#ifdef DEBUG802_15_4
				fprintf(stdout,"[%s::%s][%f](node %d) before alloc txBcnCmd2:\n\t\ttxBeacon\t= %ld\n\t\ttxAck   \t= %ld\n\t\ttxBcnCmd\t= %ld\n\t\ttxBcnCmd2\t= %ld\n\t\ttxData  \t= %ld\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,txBeacon,txAck,txBcnCmd,txBcnCmd2,txData);
#endif
				assert(!txBcnCmd2);
				txBcnCmd2 = Packet::alloc();
				assert(txBcnCmd2);
				wph = HDR_LRWPAN(txBcnCmd2);
				constructCommandHeader(txBcnCmd2,&frmCtrl,0x08,defFrmCtrl_AddrMode16,0xffff,0xffff,defFrmCtrl_AddrMode64,mpib.macPANId,aExtendedAddress,false,false,false);
				//--- payload (refer to Figure 56) ---
				wph->MSDU_PayloadLen = 7;
				*((UINT_16 *)wph->MSDU_Payload) = PANId;			//PAN identifier
				*((UINT_16 *)(wph->MSDU_Payload + 2)) = mpib.macShortAddress;	//Coor. short address
				wph->MSDU_Payload[4] = LogicalChannel;				//Logical channel
				*((UINT_16 *)(wph->MSDU_Payload + 5)) = 0xffff;			//short address; be the assigned address if directed to an orphaned device
				constructMPDU(8,txBcnCmd2,frmCtrl.FrmCtrl,mpib.macDSN++,wph->MHR_DstAddrInfo,wph->MHR_SrcAddrInfo,0,0x08,0);
				csmacaBegin('C');
				//------------------------------------
				//change the configuration and begin to transmit beacons after the transmission of the realignment command
				taskP.mlme_start_request_BeaconOrder = BeaconOrder;
				taskP.mlme_start_request_SuperframeOrder = SuperframeOrder;
				taskP.mlme_start_request_BatteryLifeExtension = BatteryLifeExtension;
				taskP.mlme_start_request_SecurityEnable = SecurityEnable;
				taskP.mlme_start_request_PANCoordinator = PANCoordinator;
				taskP.mlme_start_request_PANId = PANId;
				taskP.mlme_start_request_LogicalChannel = LogicalChannel;
				break;
			}
			else
			{
				taskP.taskStep(task) = 2;
				step = 2;
				//fall through case 2
			}
		case 1:
			if (step == 1)
			{
				if (status == p_IDLE)
				{
					taskP.taskStep(task)++;
					strcpy(taskP.taskFrFunc(task),"PD_DATA_confirm");
					plme_set_trx_state_request(p_TX_ON);
					break;
				}
				else
				{
					freePkt(txBcnCmd2);	//actually we can keep <txBcnCmd2> for next channel
					txBcnCmd2 = 0;
					//fall through case case 2 -- ignore the failure and continue to transmit beacons
					taskP.taskStep(task) = 2;
				}
			}
		case 2:
			taskP.taskStep(task)++;
			strcpy(taskP.taskFrFunc(task),"PD_DATA_confirm");	//for beacon
			resetTRX();
			if (CoordRealignment)
				taskSuccess('C',false);
			//change the configuration
			origBeaconOrder = mpib.macBeaconOrder;
			mpib.macBeaconOrder = BeaconOrder;
			if (BeaconOrder == 15)
				mpib.macSuperframeOrder = 15;
			else
				mpib.macSuperframeOrder = SuperframeOrder;
			mpib.macBattLifeExt = BatteryLifeExtension;
			secuBeacon = SecurityEnable;
			if (isPANCoor != PANCoordinator)
				changeNodeColor(CURRENT_TIME,PANCoordinator?Nam802_15_4::def_PANCoor_clr:Nam802_15_4::def_Coor_clr);
			isPANCoor = PANCoordinator;
			if (PANCoordinator)
			{
				mpib.macPANId = PANId;
				mpib.macCoordExtendedAddress = aExtendedAddress;	//I'm the coordinator of myself
				tmp_ppib.phyCurrentChannel = LogicalChannel;
				phy->PLME_SET_request(phyCurrentChannel,&tmp_ppib);
			}
			if (origBeaconOrder == BeaconOrder)
			{
				taskP.taskStatus(task) = false;
				sscs->MLME_START_confirm(m_SUCCESS);
				csmacaResume();
			}
			else if ((origBeaconOrder == 15)&&(BeaconOrder < 15))
			{
				//transmit beacon immediately
				if (bcnTxT->busy())		//the timer may still be looping there
					bcnTxT->stop();
				bcnTxT->start(true,true,0.0);
			}
			else if ((origBeaconOrder < 15)&&(BeaconOrder == 15))
				oneMoreBeacon = true;
			break;
		case 3:
			taskP.taskStatus(task) = false;
			sscs->MLME_START_confirm(m_SUCCESS);
			taskSuccess('b');
			break;
		default:
			break;
	}
}

void Mac802_15_4::mlme_sync_request(UINT_8 LogicalChannel, bool TrackBeacon,bool frUpper,PHYenum status)
{
	UINT_8 step,task,BO;

	task = TP_mlme_sync_request;
	if (frUpper)
	{
		//checkTaskOverflow(task);	//overlapping allowed
		//stop the beacon receiving timer if it is running
		if (bcnRxT->busy())
			bcnRxT->stop();
		taskP.taskStep(task) = 0;
		(taskP.taskFrFunc(task))[0] = 0;
	}

	step = taskP.taskStep(task);
	switch(step)
	{
		case 0:
			//no validation check required in the draft, but it's better to check it
			if ((!phy->channelSupported(LogicalChannel))	//channel not supported
					|| (mpib.macPANId == 0xffff)			//broadcast PAN ID
					//|| (macBeaconOrder2 == 15)			//non-beacon mode or <macBeaconOrder2> not yet populated
			   )
			{
				sscs->MLME_SYNC_LOSS_indication(m_UNDEFINED);
				return;
			}
			taskP.taskStatus(task) = true;
			taskP.taskStep(task)++;
			strcpy(taskP.taskFrFunc(task),"recvBeacon");
			taskP.mlme_sync_request_numSearchRetry = 0;
			taskP.mlme_sync_request_tracking = TrackBeacon;
			//set current channel
			tmp_ppib.phyCurrentChannel = LogicalChannel;
			phy->PLME_SET_request(phyCurrentChannel,&tmp_ppib);
			//enable the receiver
			{plme_set_trx_state_request(p_RX_ON);}
			BO = (macBeaconOrder2 == 15)?14:macBeaconOrder2;
			if (bcnSearchT->busy())
				bcnSearchT->stop();
			bcnSearchT->start(aBaseSuperframeDuration*((1 << BO)+1) / phy->getRate('s'));
			break;
		case 1:
			if (status == p_SUCCESS)	//beacon received
			{
				//no confirm primitive for the success - it's better to have one
				taskP.taskStatus(task) = false;
				//continue to track the beacon if required
				if (TrackBeacon)
				{
					//reset <numSearchRetry> (so that tracking can work properly)
					taskP.mlme_sync_request_numSearchRetry = 0;
					if(!bcnRxT->busy())
						bcnRxT->start();
				}
				csmacaResume();
			}
			else				//time out when waiting for beacon
			{
				taskP.mlme_sync_request_numSearchRetry++;
				if (taskP.mlme_sync_request_numSearchRetry <= aMaxLostBeacons)
				{
					plme_set_trx_state_request(p_RX_ON);
					BO = (macBeaconOrder2 == 15)?14:macBeaconOrder2;
					bcnSearchT->start(aBaseSuperframeDuration*((1 << BO)+1) / phy->getRate('s'));
				}
				else
				{
					taskP.taskStatus(task) = false;
					changeNodeColor(CURRENT_TIME,Nam802_15_4::def_Node_clr);
					sscs->MLME_SYNC_LOSS_indication(m_BEACON_LOSS);
					/*If the initial beacon location fails, no need to track the beacon even it is required
					 *Note that not tracking does not mean the device will not be able to receive beacons --
					 *but the reception may be not so reliable since there is no synchronization.
					 */
					taskP.mlme_sync_request_tracking = false;
					csmacaResume();
					return;
				}
			}
			break;
		default:
			break;
	}
}

void Mac802_15_4::mlme_poll_request(UINT_8 CoordAddrMode,UINT_16 CoordPANId,IE3ADDR CoordAddress,bool SecurityEnable,
		bool autoRequest,bool firstTime,PHYenum status)
{
	UINT_8 step,task;
	FrameCtrl frmCtrl;
	hdr_lrwpan* wph;

	task = TP_mlme_poll_request;
	if (firstTime)
	{
		if (taskP.taskStatus(task))
			return;
		else
		{
			taskP.taskStep(task) = 0;
			(taskP.taskFrFunc(task))[0] = 0;
		}
	}

	step = taskP.taskStep(task);
	switch(step)
	{
		case 0:
			//check if parameters valid or not
			if (((CoordAddrMode != defFrmCtrl_AddrMode16)&&(CoordAddrMode != defFrmCtrl_AddrMode64))
					|| (CoordPANId == 0xffff))
			{
				if (!autoRequest)
					sscs->MLME_POLL_confirm(m_INVALID_PARAMETER);
				return;
			}
			taskP.taskStatus(task) = true;
			taskP.taskStep(task)++;
			strcpy(taskP.taskFrFunc(task),"csmacaCallBack");
			taskP.mlme_poll_request_CoordAddrMode = CoordAddrMode;
			taskP.mlme_poll_request_CoordPANId = CoordPANId;
			taskP.mlme_poll_request_CoordAddress = CoordAddress;
			taskP.mlme_poll_request_SecurityEnable = SecurityEnable;
			taskP.mlme_poll_request_autoRequest = autoRequest;
			//-- send a data request command ---
#ifdef DEBUG802_15_4
			fprintf(stdout,"[%s::%s][%f](node %d) before alloc txBcnCmd2:\n\t\ttxBeacon\t= %ld\n\t\ttxAck   \t= %ld\n\t\ttxBcnCmd\t= %ld\n\t\ttxBcnCmd2\t= %ld\n\t\ttxData  \t= %ld\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,txBeacon,txAck,txBcnCmd,txBcnCmd2,txData);
#endif
			assert(!txBcnCmd2);
			txBcnCmd2 = Packet::alloc();
			assert(txBcnCmd2);
			wph = HDR_LRWPAN(txBcnCmd2);
			if ((mpib.macShortAddress == 0xfffe)||(mpib.macShortAddress == 0xffff))
			{
				frmCtrl.setSrcAddrMode(defFrmCtrl_AddrMode64);
				wph->MHR_SrcAddrInfo.addr_64 = aExtendedAddress;
			}
			else
			{
				frmCtrl.setSrcAddrMode(defFrmCtrl_AddrMode16);
				wph->MHR_SrcAddrInfo.addr_16 = mpib.macShortAddress;
			}
			if (sfSpec2.PANCoor)
				frmCtrl.setDstAddrMode(defFrmCtrl_AddrModeNone);
			else
				frmCtrl.setDstAddrMode(CoordAddrMode);
			constructCommandHeader(txBcnCmd2,&frmCtrl,0x04,frmCtrl.dstAddrMode,CoordPANId,CoordAddress,frmCtrl.srcAddrMode,mpib.macPANId,wph->MHR_SrcAddrInfo.addr_64,SecurityEnable,false,true);
			constructMPDU(1,txBcnCmd2,frmCtrl.FrmCtrl,mpib.macDSN++,wph->MHR_DstAddrInfo,wph->MHR_SrcAddrInfo,0,0x04,0);
			csmacaBegin('C');
			//------------------------------------
			break;
		case 1:
			if (status == p_IDLE)
			{
				taskP.taskStep(task)++;
				strcpy(taskP.taskFrFunc(task),"PD_DATA_confirm");
				plme_set_trx_state_request(p_TX_ON);
				break;
			}
			else
			{
				taskP.taskStatus(task) = false;
				if (!autoRequest)
					sscs->MLME_POLL_confirm(m_CHANNEL_ACCESS_FAILURE);
				resetTRX();
				taskFailed('C',m_CHANNEL_ACCESS_FAILURE);
				return;
			}
			break;
		case 2:
			taskP.taskStep(task)++;
			strcpy(taskP.taskFrFunc(task),"recvAck");
			//enable the receiver
			plme_set_trx_state_request(p_RX_ON);
			txT->start(mpib.macAckWaitDuration/phy->getRate('s'));
			waitBcnCmdAck2 = true;
			break;
		case 3:
			if (status == p_SUCCESS)	//ack. received
			{
				if (!taskP.mlme_poll_request_pending)
				{
					taskP.taskStatus(task) = false;
					if (!autoRequest)
						sscs->MLME_POLL_confirm(m_NO_DATA);
					resetTRX();
					taskSuccess('C');
					return;
				}
				else
				{
					taskP.taskStep(task)++;
					strcpy(taskP.taskFrFunc(task),"IFSHandler");
					plme_set_trx_state_request(p_RX_ON);		//wait for data
					taskSuccess('C',false);
					extractT->start(aMaxFrameResponseTime/phy->getRate('s'),true);	//wait for <aMaxFrameResponseTime> symbols (or CAP symbols if beacon enabled) (see page 156, line 1-3)
				}
			}
			else				//time out when waiting for ack.
			{
				numBcnCmdRetry2++;
				if (numBcnCmdRetry2 <= aMaxFrameRetries)
				{
					taskP.taskStep(task) = 1;	//important
					strcpy(taskP.taskFrFunc(task),"csmacaCallBack");
					waitBcnCmdAck2 = false;
					csmacaResume();
				}
				else
				{
					taskP.taskStatus(task) = false;
					if (!autoRequest)
						sscs->MLME_POLL_confirm(m_NO_ACK);
					resetTRX();
					taskFailed('C',m_NO_ACK);
					return;
				}
			}
			break;
		case 4:
			taskP.taskStatus(task) = false;
			if (status == p_SUCCESS)		//data received
			{
				//stop the timer
				extractT->stop();
				if (!autoRequest)
					sscs->MLME_POLL_confirm(m_SUCCESS);
				//another step is to issue DATA.indication() which has been done in IFSHandler()

				//poll again to see if there are more packets pending -- note that, for each poll request, more than one confirm could be passed to upper layer
				mlme_poll_request(CoordAddrMode,CoordPANId,CoordAddress,SecurityEnable,autoRequest,true);
			}
			else					//time out when waiting for response
			{
				if (!autoRequest)
					sscs->MLME_POLL_confirm(m_NO_DATA);
				resetTRX();
				csmacaResume();
			}
			break;
		default:
			break;
	}
}

//-------------------------------------------------------------------------------------

void Mac802_15_4::csmacaBegin(char pktType)
{
	if (pktType == 'c')		//txBcnCmd
	{
		waitBcnCmdAck = false;			//beacon packet not yet transmitted
		numBcnCmdRetry = 0;
		if (backoffStatus == 99)		//backoffing for data packet
		{
			backoffStatus = 0;
			csmaca->cancel();
		}
		csmacaResume();
	}
	else if (pktType == 'C')	//txBcnCmd2
	{
		waitBcnCmdAck2 = false;			//command packet not yet transmitted
		numBcnCmdRetry2 = 0;
		if ((backoffStatus == 99)&&(txCsmaca != txBcnCmd))	//backoffing for data packet
		{
			backoffStatus = 0;
			csmaca->cancel();
		}
		csmacaResume();

	}
	else //if (pktType == 'd')	//txData
	{
		waitDataAck = false;			//data packet not yet transmitted
		numDataRetry = 0;
		csmacaResume();
	}
}

void Mac802_15_4::csmacaResume(void)
{
	FrameCtrl frmCtrl;

	if ((backoffStatus != 99)			//not during backoff
			&& (!inTransmission))				//not during transmission
	{
		if ((txBcnCmd)&&(!waitBcnCmdAck))
		{
			backoffStatus = 99;
			frmCtrl.FrmCtrl = HDR_LRWPAN(txBcnCmd)->MHR_FrmCtrl;
			frmCtrl.parse();
			txCsmaca = txBcnCmd;
			csmaca->start(true,txBcnCmd,frmCtrl.ackReq);
		}
		else if ((txBcnCmd2)&&(!waitBcnCmdAck2))
		{
			backoffStatus = 99;
			frmCtrl.FrmCtrl = HDR_LRWPAN(txBcnCmd2)->MHR_FrmCtrl;
			frmCtrl.parse();
			txCsmaca = txBcnCmd2;
			csmaca->start(true,txBcnCmd2,frmCtrl.ackReq);
		}
		else if ((txData)&&(!waitDataAck))
		{
			strcpy(taskP.taskFrFunc(TP_mcps_data_request),"csmacaCallBack");	//the transmission may be interrupted and need to backoff again
			taskP.taskStep(TP_mcps_data_request) = 1;				//also set the step
			backoffStatus = 99;
			frmCtrl.FrmCtrl = HDR_LRWPAN(txData)->MHR_FrmCtrl;
			frmCtrl.parse();
			txCsmaca = txData;
			csmaca->start(true,txData,frmCtrl.ackReq);
		}
	}
}

void Mac802_15_4::csmacaCallBack(PHYenum status)
{
	if (((!txBcnCmd)||(waitBcnCmdAck))
			&&((!txBcnCmd2)||(waitBcnCmdAck2))
			&&((!txData)||(waitDataAck)))
		return;

	backoffStatus = (status == p_IDLE)?1:2;

#ifdef DEBUG802_15_4
	hdr_cmn *ch = HDR_CMN(txCsmaca);
	if (status != p_IDLE)
		fprintf(stdout,"[%s::%s][%f](node %d) backoff failed: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,wpan_pName(txCsmaca),p802_15_4macSA(txCsmaca),p802_15_4macDA(txCsmaca),ch->uid(),HDR_LRWPAN(txCsmaca)->uid,ch->size());
#endif

	dispatch(status,__FUNCTION__);
}

int Mac802_15_4::getBattLifeExtSlotNum(void)
{
	phy->PLME_GET_request(phyCurrentChannel);
	return (tmp_ppib.phyCurrentChannel<=10)?8:6;
}

double Mac802_15_4::getCAP(bool small)
{
	double bcnTxTime,bcnRxTime,bcnOtherRxTime;
	double sSlotDuration,sSlotDuration2,sSlotDuration3,BI2,BI3,t_CAP,t_CAP2,t_CAP3;
	double now,oneDay,tmpf;

	now = CURRENT_TIME;
	oneDay = now + 24.0*3600;

	if ((mpib.macBeaconOrder == 15)&&(macBeaconOrder2 == 15)				//non-beacon enabled
			&&(macBeaconOrder3 == 15))								//no beacons from outside PAN
		return oneDay;									//transmission can always go ahead

	bcnTxTime = macBcnTxTime / phy->getRate('s');
	bcnRxTime = macBcnRxTime / phy->getRate('s');
	bcnOtherRxTime = macBcnOtherRxTime / phy->getRate('s');
	sSlotDuration = sfSpec.sd / phy->getRate('s');
	sSlotDuration2 = sfSpec2.sd / phy->getRate('s');
	sSlotDuration3 = sfSpec3.sd / phy->getRate('s');
	BI2 = (sfSpec2.BI / phy->getRate('s'));
	BI3 = (sfSpec3.BI / phy->getRate('s'));
	if (mpib.macBeaconOrder != 15)
	{
		if (sfSpec.BLE)
		{
			/* Linux floating number compatibility
			   t_CAP = (bcnTxTime + (beaconPeriods + getBattLifeExtSlotNum()) * aUnitBackoffPeriod);
			   */
			{
				tmpf = (beaconPeriods + getBattLifeExtSlotNum()) * aUnitBackoffPeriod;
				t_CAP = bcnTxTime + tmpf;
			}
		}
		else
		{
			/* Linux floating number compatibility
			   t_CAP = (bcnTxTime + (sfSpec.FinCAP + 1) * sSlotDuration);
			   */
			{
				tmpf = (sfSpec.FinCAP + 1) * sSlotDuration;
				t_CAP = bcnTxTime + tmpf;
			}
		}
	}
	if (macBeaconOrder2 != 15)
	{
		if (sfSpec2.BLE)
		{
			/* Linux floating number compatibility
			   t_CAP2 = (bcnRxTime + (beaconPeriods2 + getBattLifeExtSlotNum()) * aUnitBackoffPeriod);
			   */
			{
				tmpf = (beaconPeriods2 + getBattLifeExtSlotNum()) * aUnitBackoffPeriod;
				t_CAP2 = bcnRxTime + tmpf;
			}
		}
		else
		{
			/* Linux floating number compatibility
			   t_CAP2 = (bcnRxTime + (sfSpec2.FinCAP + 1) * sSlotDuration2);
			   */
			{
				tmpf = (sfSpec2.FinCAP + 1) * sSlotDuration2;
				t_CAP2 = bcnRxTime + tmpf;
			}
		}

		/* Linux floating number compatibility
		   if ((t_CAP2 < now)&&(t_CAP2 + aMaxLostBeacons * BI2 >= now))
		   */
		tmpf = aMaxLostBeacons * BI2;
		if ((t_CAP2 < now)&&(t_CAP2 + tmpf >= now))	//no more than <aMaxLostBeacons> beacons missed
			while (t_CAP2 < now)
				t_CAP2 += BI2;
	}
	if (macBeaconOrder3 != 15)
	{
		//no need to handle option <macBattLifeExt> here
		/* Linux floating number compatibility
		   t_CAP3 = (bcnOtherRxTime + (sfSpec3.FinCAP + 1) * sSlotDuration3);
		   */
		{
			tmpf = (sfSpec3.FinCAP + 1) * sSlotDuration3;
			t_CAP3 = bcnOtherRxTime + tmpf;
		}

		/* Linux floating number compatibility
		   if ((t_CAP3 < now)&&(t_CAP3 + aMaxLostBeacons * BI3 >= now))
		   */
		tmpf = aMaxLostBeacons * BI3;
		if ((t_CAP3 < now)&&(t_CAP3 + tmpf >= now))	//no more than <aMaxLostBeacons> beacons missed
			while (t_CAP3 < now)
				t_CAP3 += BI3;
	}

	if ((mpib.macBeaconOrder == 15)&&(macBeaconOrder2 == 15))
	{
		if (t_CAP3 >= now)
			return t_CAP3;
		else
			return oneDay;
	}
	else if (mpib.macBeaconOrder == 15)
	{
		if (t_CAP2 >= now)
			return t_CAP2;
		else
			return oneDay;
	}
	else if (macBeaconOrder2 == 15)
	{
		if (t_CAP >= now)
			return t_CAP;
		else
			return oneDay;
	}
	else
	{
		if (t_CAP2 < now)
			return t_CAP;

		if ((small)
				&&  (t_CAP > t_CAP2))
			t_CAP = t_CAP2;
		if ((!small)
				&&  (t_CAP < t_CAP2))
			t_CAP = t_CAP2;

		return t_CAP;
	}
}

double Mac802_15_4::getCAPbyType(int type)
{
	double bcnTxTime,bcnRxTime,bcnOtherRxTime;
	double sSlotDuration,sSlotDuration2,sSlotDuration3,BI2,BI3,t_CAP,t_CAP2,t_CAP3;
	double now,oneDay,tmpf;

	now = CURRENT_TIME;
	oneDay = now + 24.0*3600;

	if ((mpib.macBeaconOrder == 15)&&(macBeaconOrder2 == 15)				//non-beacon enabled
			&&(macBeaconOrder3 == 15))								//no beacons from outside PAN
		return oneDay;									//transmission can always go ahead

	bcnTxTime = macBcnTxTime / phy->getRate('s');
	bcnRxTime = macBcnRxTime / phy->getRate('s');
	bcnOtherRxTime = macBcnOtherRxTime / phy->getRate('s');
	sSlotDuration = sfSpec.sd / phy->getRate('s');
	sSlotDuration2 = sfSpec2.sd / phy->getRate('s');
	sSlotDuration3 = sfSpec3.sd / phy->getRate('s');
	BI2 = (sfSpec2.BI / phy->getRate('s'));
	BI3 = (sfSpec3.BI / phy->getRate('s'));

	if (type == 1)
	{
		if (mpib.macBeaconOrder != 15)
		{
			if (sfSpec.BLE)
			{
				/* Linux floating number compatibility
				   t_CAP = (bcnTxTime + (beaconPeriods + getBattLifeExtSlotNum()) * aUnitBackoffPeriod);
				   */
				{
					tmpf = (beaconPeriods + getBattLifeExtSlotNum()) * aUnitBackoffPeriod;
					t_CAP = bcnTxTime + tmpf;
				}
			}
			else
			{
				/* Linux floating number compatibility
				   t_CAP = (bcnTxTime + (sfSpec.FinCAP + 1) * sSlotDuration);
				   */
				{
					tmpf = (sfSpec.FinCAP + 1) * sSlotDuration;
					t_CAP = bcnTxTime + tmpf;
				}
			}
			return (t_CAP>=now)?t_CAP:oneDay;
		}
		else
		{
			return oneDay;
		}
	}

	if (type == 2)
	{
		if (macBeaconOrder2 != 15)
		{
			if (sfSpec2.BLE)
			{
				/* Linux floating number compatibility
				   t_CAP2 = (bcnRxTime + (beaconPeriods2 + getBattLifeExtSlotNum()) * aUnitBackoffPeriod);
				   */
				{
					tmpf = (beaconPeriods2 + getBattLifeExtSlotNum()) * aUnitBackoffPeriod;
					t_CAP2 = bcnRxTime + tmpf;
				}
			}
			else
			{
				/* Linux floating number compatibility
				   t_CAP2 = (bcnRxTime + (sfSpec2.FinCAP + 1) * sSlotDuration2);
				   */
				{
					tmpf = (sfSpec2.FinCAP + 1) * sSlotDuration2;
					t_CAP2 = bcnRxTime + tmpf;
				}
			}

			/* Linux floating number compatibility
			   if ((t_CAP2 < now)&&(t_CAP2 + aMaxLostBeacons * BI2 >= now))
			   */
			tmpf = aMaxLostBeacons * BI2;
			if ((t_CAP2 < now)&&(t_CAP2 + tmpf >= now))	//no more than <aMaxLostBeacons> beacons missed
			{
				while (t_CAP2 < now)
				{
					t_CAP2 += BI2;
				}
			}
			return (t_CAP2>=now)?t_CAP2:oneDay;
		}
		else
		{
			return oneDay;
		}
	}
	
	if (type == 3)
	{
		if (macBeaconOrder3 != 15)
		{
			//no need to handle option <macBattLifeExt> here
			/* Linux floating number compatibility
			   t_CAP3 = (bcnOtherRxTime + (sfSpec3.FinCAP + 1) * sSlotDuration3);
			   */
			{
				tmpf = (sfSpec3.FinCAP + 1) * sSlotDuration3;
				t_CAP3 = bcnOtherRxTime + tmpf;
			}

			/* Linux floating number compatibility
			   if ((t_CAP3 < now)&&(t_CAP3 + aMaxLostBeacons * BI3 >= now))
			   */
			tmpf = aMaxLostBeacons * BI3;
			if ((t_CAP3 < now)&&(t_CAP3 + tmpf >= now))	//no more than <aMaxLostBeacons> beacons missed
			{
				while (t_CAP3 < now)
				{
					t_CAP3 += BI3;
				}
			}
			return (t_CAP3>=now)?t_CAP3:oneDay;
		}
		else
		{
			return oneDay;
		}
	}

	return oneDay;
}

bool Mac802_15_4::canProceedWOcsmaca(Packet *p)
{
	//this function checks whether there is enough time in the CAP of current superframe to finish a transaction (transmit a pending packet to a device)
	//(in the case the node acts as both a coordinator and a device, both the superframes from and to this node should be taken into account)
	double wtime,t_IFS,t_transacTime,t_CAP,tmpf;
	FrameCtrl frmCtrl;
	//int type;

	if ((mpib.macBeaconOrder == 15)&&(macBeaconOrder2 == 15)				
	&&(macBeaconOrder3 == 15))								
		return true;									
	else
	{
		frmCtrl.FrmCtrl = HDR_LRWPAN(p)->MHR_FrmCtrl;
		frmCtrl.parse();
		wtime = 0.0;
		//there is no need to consider <macBattLifeExt>, since the device polling the data 
		//should be waiting rather than go to sleep after the first 6 CAP backoff perios.
		if (HDR_CMN(p)->size() <= aMaxSIFSFrameSize)
			t_IFS = aMinSIFSPeriod;
		else
			t_IFS = aMinLIFSPeriod;
		t_IFS /= phy->getRate('s');
		t_transacTime  = locateBoundary(toParent(p),wtime) - wtime;			//boundary location time
		t_transacTime += phy->trxTime(p);						//packet transmission time
		if (frmCtrl.ackReq)
		{
			t_transacTime += mpib.macAckWaitDuration/phy->getRate('s');		//ack. waiting time
			t_transacTime += 2*max_pDelay;						//round trip propagation delay (802.15.4 ignores this, but it should be there even though it is very small)
			t_transacTime += t_IFS;							//IFS time -- not only ensure that the sender can finish the transaction, but also the receiver
			t_CAP = getCAP(true);

			/* Linux floating number compatibility
			if (CURRENT_TIME + wtime + t_transacTime > t_CAP)
			*/
			tmpf = CURRENT_TIME + wtime;
			tmpf += t_transacTime;
			if (tmpf > t_CAP)
				return false;
			else
				return true;
		}
		else
		{
			//in this case, we need to handle individual CAP 
			t_CAP = getCAPbyType(1);

			/* Linux floating number compatibility
			if (CURRENT_TIME + wtime + t_transacTime > t_CAP)
			*/
			tmpf = CURRENT_TIME + wtime;
			tmpf += t_transacTime;
			if (tmpf > t_CAP)
				return false;
			t_CAP = getCAPbyType(2);
			t_transacTime += max_pDelay;						//one-way trip propagation delay (802.15.4 ignores this, but it should be there even though it is very small)
			t_transacTime += 12/phy->getRate('s');					//transceiver turn-around time (receiver may need to do this to transmit next beacon)
			t_transacTime += t_IFS;							//IFS time -- not only ensure that the sender can finish the transaction, but also the receiver

			/* Linux floating number compatibility
			if (CURRENT_TIME + wtime + t_transacTime > t_CAP)
			*/
			tmpf = CURRENT_TIME + wtime;
			tmpf += t_transacTime;
			if (tmpf > t_CAP)
				return false;
			t_CAP = getCAPbyType(3);
			t_transacTime -= t_IFS;							//the third node does not need to handle the transaction

			/* Linux floating number compatibility
			if (CURRENT_TIME + wtime + t_transacTime > t_CAP)
			*/
			tmpf = CURRENT_TIME + wtime;
			tmpf += t_transacTime;
			if (tmpf > t_CAP)
				return false;

			return true;
		}
	}
}

void Mac802_15_4::transmitCmdData(void)
{
	double delay;

	if ((mpib.macBeaconOrder != 15)||(macBeaconOrder2 != 15))	//beacon enabled -- slotted
	{
		delay =locateBoundary(toParent(txCsmaca),0.0);
		if(delay > 0.0)
		{
			Scheduler::instance().schedule(&txCmdDataH, &(txCmdDataH.nullEvent), delay);
			return;
		}
	}

	//transmit immediately
	txBcnCmdDataHandler();
}

void Mac802_15_4::reset_TRX(const char *frFile,const char *frFunc,int line)
{
	double t_CAP;
	PHYenum t_state;

	if ((mpib.macBeaconOrder != 15)||(macBeaconOrder2 != 15))	//beacon enabled
	{
		//according to the draft, <macRxOnWhenIdle> only considered during idle periods of the CAP if beacon enabled
		t_CAP = getCAP(false);
		if (CURRENT_TIME < t_CAP)
			t_state = mpib.macRxOnWhenIdle?p_RX_ON:p_TRX_OFF;
		else
			t_state = p_RX_ON;	//(not considered ==> RX_ON)?
	}
	else
		t_state = mpib.macRxOnWhenIdle?p_RX_ON:p_TRX_OFF;
	
	set_trx_state_request(t_state,frFile,frFunc,line);
}

void Mac802_15_4::taskSuccess(char task,bool csmacaRes)
{
	hdr_cmn* ch;
	hdr_lrwpan* wph;
	UINT_16 t_CAP;
	UINT_8 ifs;
	double tmpf;

#ifdef DEBUG802_15_4
	fprintf(stdout,"[%s::%s][%f](node %d) task '%c' successful:\n\t\ttxBeacon\t= %ld\n\t\ttxAck   \t= %ld\n\t\ttxBcnCmd\t= %ld\n\t\ttxBcnCmd2\t= %ld\n\t\ttxData  \t= %ld\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,task,txBeacon,txAck,txBcnCmd,txBcnCmd2,txData);
#endif

	if (task == 'b')	//beacon
	{
		if (!txBeacon)	
		{
			assert(txBcnCmd2);
			txBeacon = txBcnCmd2;
			txBcnCmd2 = 0;
		}
		//--- calculate CAP ---
		sfSpec.parse();		
		if (HDR_CMN(txBeacon)->size() <= aMaxSIFSFrameSize)
			ifs = aMinSIFSPeriod;
		else
			ifs = aMinLIFSPeriod;

		/* Linux floating number compatibility
		beaconPeriods = (UINT_8)((phy->trxTime(txBeacon) * phy->getRate('s') + ifs) / aUnitBackoffPeriod);
		*/
		{
		tmpf = phy->trxTime(txBeacon) * phy->getRate('s');
		tmpf += ifs;
		beaconPeriods = (UINT_8)(tmpf / aUnitBackoffPeriod);
		}

		/* Linux floating number compatibility
		if (fmod((phy->trxTime(txBeacon) * phy->getRate('s')+ ifs) ,aUnitBackoffPeriod) > 0.0)
		*/
		tmpf = phy->trxTime(txBeacon) * phy->getRate('s');
		tmpf += ifs;
		if (fmod(tmpf,aUnitBackoffPeriod) > 0.0)
			beaconPeriods++;

		t_CAP = (sfSpec.FinCAP + 1) * (sfSpec.sd / aUnitBackoffPeriod) - beaconPeriods;

		if (t_CAP == 0)
		{
			csmacaRes = false;
			plme_set_trx_state_request(p_TRX_OFF);
		}
		else
			plme_set_trx_state_request(p_RX_ON);
		//CSMA-CA may be waiting for the new beacon
		if (backoffStatus == 99)
			csmaca->newBeacon('t');
		//----------------------
		beaconWaiting = false;
		Packet::free(txBeacon);
		txBeacon = 0;
		/*
		//send out delayed ack.
		if (txAck)
		{
			csmacaRes = false;
			plme_set_trx_state_request(p_TX_ON);
		}
		*/
	}
	else if (task == 'a')	//ack.
	{
		assert(txAck);
		Packet::free(txAck);
		txAck = 0;
	}
	else if (task == 'c')	//command
	{
		assert(txBcnCmd);
		//if it is a pending packet, delete it from the pending list
		updateTransacLinkByPktOrHandle(tr_oper_del,&transacLink1,&transacLink2,txBcnCmd);
		freePkt(txBcnCmd);
		txBcnCmd = 0;
	}
	else if (task == 'C')	//command
	{
		assert(txBcnCmd2);
		freePkt(txBcnCmd2);
		txBcnCmd2 = 0;
	}
	else if (task == 'd')	//data
	{
		assert(txData);

		ch = HDR_CMN(txData);
		wph = HDR_LRWPAN(txData);

		Packet *p = txData;
		txData = 0;
		if (ch->ptype() == PT_MAC)
		{
			assert(wph->msduHandle);
			sscs->MCPS_DATA_confirm(wph->msduHandle,m_SUCCESS);
		}
		else
		{
			if (Mac802_15_4::callBack == 2)
			if (ch->xmit_failure_)
				if (p802_15_4macDA(p) != (nsaddr_t)MAC_BROADCAST)
			{
				ch->size() -= macHeaderLen(wph->MHR_FrmCtrl);
				ch->xmit_reason_ = 1;
				ch->xmit_failure_(p->refcopy(),ch->xmit_failure_data_);
			}
			if (callback_)	
			{
				Handler *h = callback_;
				callback_ = 0;
				h->handle((Event*) 0);
			}
		}
		//if it is a pending packet, delete it from the pending list
		updateTransacLinkByPktOrHandle(tr_oper_del,&transacLink1,&transacLink2,p);
		freePkt(p);
	}
	else
		assert(0);

	if (csmacaRes)
		csmacaResume();
}

void Mac802_15_4::taskFailed(char task,MACenum status,bool csmacaRes)
{
	hdr_cmn* ch;
	hdr_lrwpan* wph;

#ifdef DEBUG802_15_4
	fprintf(stdout,"[D][FAIL][%s::%s][%f](node %d) task '%c' failed:\n\t\ttxBeacon\t= %ld\n\t\ttxAck   \t= %ld\n\t\ttxBcnCmd\t= %ld\n\t\ttxBcnCmd2\t= %ld\n\t\ttxData  \t= %ld\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,task,txBeacon,txAck,txBcnCmd,txBcnCmd2,txData);
#endif

	if ((task == 'b')	//beacon
	|| (task == 'a')	//ack.
	|| (task == 'c'))	//command
	{
		assert(0);	//we don't handle the above failures here
	}
	else if (task == 'C')	//command
	{
		freePkt(txBcnCmd2);
		txBcnCmd2 = 0;
	}
	else if (task == 'd')	//data
	{
		wph = HDR_LRWPAN(txData);
		ch = HDR_CMN(txData);
#ifdef DEBUG802_15_4
		fprintf(stdout,"[D][FAIL][%s::%s::%d][%f](node %d) failure: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,__LINE__,CURRENT_TIME,index_,wpan_pName(txData),p802_15_4macSA(txData),p802_15_4macDA(txData),ch->uid(),wph->uid,ch->size());
#endif
		Packet *p = txData;
		txData = 0;
		if (wph->msduHandle)	//from SSCS
			sscs->MCPS_DATA_confirm(wph->msduHandle,status);
		else
		{
			if (Mac802_15_4::callBack)
			if (ch->xmit_failure_)
			{
				wph->setSN = true;		
				ch->size() -= macHeaderLen(wph->MHR_FrmCtrl);
				ch->xmit_reason_ = 0;
				ch->xmit_failure_(p->refcopy(),ch->xmit_failure_data_);
			}
			if (callback_			
			&& (!dataWaitT->busy()))	
			{
				Handler *h = callback_;
				callback_ = 0;
				h->handle((Event*) 0);
			}
		}
		freePkt(p);
	}
	else
		assert(0);

	if (csmacaRes)
		csmacaResume();
}

void Mac802_15_4::freePkt(Packet *pkt)
{
	/*
	if (HDR_LRWPAN(pkt)->indirect)
		return;			//the packet will be automatically deleted when expired
	else
		Packet::free(pkt);
	*/
	Packet::free(pkt);		//now same operation for directly transmitted and indirectly transmitted packets
}

UINT_8 Mac802_15_4::macHeaderLen(UINT_16 FrmCtrl)
{
	//calculate the MAC sublayer header (also including footer) length
	UINT_8 macHLen;
	FrameCtrl frmCtrl;

	frmCtrl.FrmCtrl = FrmCtrl;
	frmCtrl.parse();
	
	macHLen = 0;
	macHLen += 2		//FrmCtrl
		  +1		//BSN/DSN
		  +2;		//FCS
	if (frmCtrl.frmType == defFrmCtrl_Type_Beacon)		//Beacon
	{
		if (frmCtrl.srcAddrMode == defFrmCtrl_AddrMode16)
			macHLen += 2;
		else if (frmCtrl.srcAddrMode == defFrmCtrl_AddrMode64)
			macHLen += 8;
	}
	else if ((frmCtrl.frmType == defFrmCtrl_Type_Data)	//Data
		||(frmCtrl.frmType == defFrmCtrl_Type_MacCmd))	//Mac Command
	{
		if (frmCtrl.dstAddrMode == defFrmCtrl_AddrMode16)
			macHLen += 2;
		else if (frmCtrl.dstAddrMode == defFrmCtrl_AddrMode64)
			macHLen += 8;
		if (frmCtrl.srcAddrMode == defFrmCtrl_AddrMode16)
			macHLen += 2;
		else if (frmCtrl.srcAddrMode == defFrmCtrl_AddrMode64)
			macHLen += 8;
	}
	else if (frmCtrl.frmType == defFrmCtrl_Type_Ack)	//Ack.
	{
		;//nothing to do
	}
	else
		fprintf(stdout,"[%s][%f](node %d) Invalid frame type!\n",__FUNCTION__,CURRENT_TIME,index_);

	return macHLen;
}

void Mac802_15_4::constructACK(Packet *p)
{
	bool intraPan;
	UINT_8 origFrmType;
	FrameCtrl frmCtrl;
	Packet *ack = Packet::alloc();
	hdr_lrwpan* wph1 = HDR_LRWPAN(p);
	hdr_lrwpan* wph2 = HDR_LRWPAN(ack);
	hdr_cmn* ch1 = HDR_CMN(p);
	hdr_cmn* ch2 = HDR_CMN(ack);
	int i;
	
	hdr_dst((char *)HDR_MAC(ack),p802_15_4macSA(p));
	hdr_src((char *)HDR_MAC(ack),index_);
	frmCtrl.FrmCtrl = wph1->MHR_FrmCtrl;
	frmCtrl.parse();
	intraPan = frmCtrl.intraPan;
	origFrmType = frmCtrl.frmType;
	frmCtrl.FrmCtrl = 0;
	frmCtrl.setFrmType(defFrmCtrl_Type_Ack);
	frmCtrl.setSecu(false);
	//if it is a data request command, then need to check if there is any packet pending.
	//In implementation, we may not have enough time to check if packets pending. If this is the case,
	//then the pending flag in the ack. should be set to 1, and then send a zero-length data packet
	//if later it turns out there is no packet actually pending.
	//In simulation, we assume having enough time to determine the pending status -- so zero-length packet will never be sent.
	//(refer to page 155, line 46-50)
	if ((origFrmType == defFrmCtrl_Type_MacCmd)		//command packet
	&& (wph1->MSDU_CmdType == 0x04))			//data request command
	{
		i = updateTransacLink(tr_oper_est,&transacLink1,&transacLink2,frmCtrl.srcAddrMode,wph1->MHR_SrcAddrInfo.addr_64);
		frmCtrl.setFrmPending(i==0);
	}
	else
		frmCtrl.setFrmPending(false);
	frmCtrl.setAckReq(false);
	frmCtrl.setIntraPan(intraPan);
	frmCtrl.setDstAddrMode(defFrmCtrl_AddrModeNone);
	frmCtrl.setSrcAddrMode(defFrmCtrl_AddrModeNone);
	wph2->MHR_FrmCtrl = frmCtrl.FrmCtrl;
	wph2->MHR_BDSN = wph1->MHR_BDSN;	//copy the SN from Data/MacCmd packet
	wph2->uid = wph1->uid;			//for debug
	//wph2->MFR_FCS

	ch2->uid() = 0;
	ch2->ptype() = PT_MAC;
	ch2->iface() = -2;
	ch2->error() = 0;
	ch2->size() = 5;
	ch2->uid() = ch1->uid();		//copy the uid

	ch2->next_hop_ = p802_15_4macDA(ack);	//nam needs the nex_hop information
	p802_15_4hdrACK(ack);

#ifdef DEBUG802_15_4
	fprintf(stdout,"[%s::%s][%f](node %d) before alloc txAck:\n\t\ttxBeacon\t= %ld\n\t\ttxAck   \t= %ld\n\t\ttxBcnCmd\t= %ld\n\t\ttxBcnCmd2\t= %ld\n\t\ttxData  \t= %ld\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,txBeacon,txAck,txBcnCmd,txBcnCmd2,txData);
#endif
	assert(!txAck);		//it's impossilbe to receive the second packet before
				//the Ack has been sent out.
	txAck = ack;
}

void Mac802_15_4::constructMPDU(UINT_8 msduLength,Packet *msdu, UINT_16 FrmCtrl,UINT_8 BDSN,panAddrInfo DstAddrInfo,
				 panAddrInfo  SrcAddrInfo,UINT_16 SuperSpec,UINT_8 CmdType,UINT_16 FCS)
{
	hdr_lrwpan* wph = HDR_LRWPAN(msdu);
	hdr_cmn* ch = HDR_CMN(msdu);
	//FrameCtrl frmCtrl;

	//fill out fields
	wph->MHR_FrmCtrl = FrmCtrl;
	if (!wph->setSN)		
		wph->MHR_BDSN = BDSN;
	else
		wph->setSN = false;	
	if (!wph->uid)
		wph->uid = Mac802_15_4::DBG_UID++;
	wph->MHR_DstAddrInfo = DstAddrInfo;
	wph->MHR_SrcAddrInfo = SrcAddrInfo;
	wph->MSDU_SuperSpec = SuperSpec;
	wph->MSDU_CmdType = CmdType;
	wph->MFR_FCS = FCS;

	//update packet length
	ch->size() = msduLength + macHeaderLen(FrmCtrl);
}

void Mac802_15_4::constructCommandHeader(Packet *p,FrameCtrl *frmCtrl,UINT_8 CmdType,
					UINT_8 dstAddrMode,UINT_16 dstPANId,IE3ADDR dstAddr,
					UINT_8 srcAddrMode,UINT_16 srcPANId,IE3ADDR srcAddr,
					bool secuEnable,bool pending,bool ackreq)
{
	hdr_lrwpan *wph = HDR_LRWPAN(p);

	frmCtrl->FrmCtrl = 0;
	frmCtrl->setFrmType(defFrmCtrl_Type_MacCmd);
	frmCtrl->setDstAddrMode(dstAddrMode);
	wph->MHR_DstAddrInfo.panID = dstPANId;
	wph->MHR_DstAddrInfo.addr_64 = dstAddr;
	hdr_src((char *)HDR_MAC(p),index_);
	if (dstAddr == 0xffff)
		hdr_dst((char *)HDR_MAC(p),MAC_BROADCAST);
	else
		hdr_dst((char *)HDR_MAC(p),dstAddr);
	frmCtrl->setSrcAddrMode(srcAddrMode);
	wph->MHR_SrcAddrInfo.panID = srcPANId;
	wph->MHR_SrcAddrInfo.addr_64 = srcAddr;
	frmCtrl->setSecu(secuEnable);
	frmCtrl->setFrmPending(pending);
	frmCtrl->setAckReq(ackreq);

	HDR_CMN(p)->ptype() = PT_MAC;

	//for trace purpose
	HDR_CMN(p)->next_hop_ = p802_15_4macDA(p);		//nam needs the nex_hop information
	p802_15_4hdrCommand(p,CmdType);
}

void Mac802_15_4::log(Packet *p)
{
	logtarget_->recv(p, (Handler*) 0);
}

void Mac802_15_4::resetCounter(int dst)
{
	if (txBcnCmd)
	if (p802_15_4macDA(txBcnCmd) == dst)
		numBcnCmdRetry = 0;

	if (txBcnCmd2)
	if (p802_15_4macDA(txBcnCmd2) == dst)
		numBcnCmdRetry2 = 0;

	if (txData)
	if (p802_15_4macDA(txData) == dst)
		numDataRetry = 0;
}

int Mac802_15_4::command(int argc, const char*const* argv)
{
	if (argc == 3)
	{
		if (strcmp(argv[1], "log-target") == 0)
		{
			logtarget_ = (NsObject*) TclObject::lookup(argv[2]);
			if(logtarget_ == 0)
				return TCL_ERROR;
			return TCL_OK;
		}
	}

	if (strcmp(argv[1], "NodeClr") == 0)
	{
		changeNodeColor(CURRENT_TIME,argv[2]);
		return (TCL_OK);
	}

	if (strcmp(argv[1], "NodeLabel") == 0)
	{
		char label[81];
		int i;
		strcpy(label,"\"");
		strcat(label,argv[2]);
		i = 3;
		while (i < argc)
		{
			if (strlen(label) + strlen(argv[i]) < 78)
			{
				strcat(label," ");
				strcat(label,argv[i]);
			}
			else
				break;
			i++;
		}
		strcat(label,"\"");
		nam->changeLabel(CURRENT_TIME,label);
		return (TCL_OK);
	}

	if (strcmp(argv[1], "node-down") == 0)
	{
		chkAddNFailLink(index_);
		changeNodeColor(CURRENT_TIME,Nam802_15_4::def_NodeFail_clr);
		if (txAck)
		{
			Packet::free(txAck);
			txAck = 0;
		}
		if (txBcnCmd)
		{
			freePkt(txBcnCmd);
			txBcnCmd = 0;
		}
		if (txBcnCmd2)
		{
			freePkt(txBcnCmd2);
			txBcnCmd2 = 0;
		}
		if (txData)
		{
			freePkt(txData);
			txData = 0;
		}
		if (phy->rxPacket())
			HDR_CMN(phy->rxPacket())->error() = 1;
		init(true);		//reset
		return (TCL_OK);
	}
	if (strcmp(argv[1], "node-up") == 0)
	{
		updateNFailLink(fl_oper_del,index_);
		nam->changeBackNodeColor(CURRENT_TIME);
		init(true);		//reset
		if (callback_)
		{
			Handler *h = callback_;
			callback_ = 0;
			h->handle((Event*) 0);
		}
		return (TCL_OK);
	}

	//check if this is actually a SSCS command
	if ((argc >= 3)&&(strcmp(argv[1],"sscs") == 0))
		return sscs->command(argc,argv);

	int rt = Mac::command(argc, argv);
	
	//check if Mac::command has already populated netif_
	if (netif_)
	if (phy == NULL)	//only execute once
	{
		phy = (Phy802_15_4 *)netif_;
		phy->macObj(this);
		csmaca = new CsmaCA802_15_4(phy,this);
		assert(csmaca);
	}

	return rt;
}

void Mac802_15_4::changeNodeColor(double atTime,const char *newColor,bool save)
{
	nam->changeNodeColor(atTime,newColor,save);
	nam->changeNodeColor(atTime+0.030001,newColor,false);	
}

void Mac802_15_4::txBcnCmdDataHandler(void)
{
	int i;

	if (taskP.taskStatus(TP_mlme_scan_request))
	if (txBcnCmd2 != txCsmaca)
		return;			//terminate all other transmissions (will resume afte channel scan)

	if (HDR_LRWPAN(txCsmaca)->indirect)
	{
		i = updateTransacLinkByPktOrHandle(tr_oper_est,&transacLink1,&transacLink2,txCsmaca);
		if (i != 0)	//transaction expired
		{
			resetTRX();
			if (txBcnCmd == txCsmaca)
				txBcnCmd = 0;
			else if (txBcnCmd2 == txCsmaca)
				txBcnCmd2 = 0;
			else if (txData == txCsmaca)
				txData = 0;
			//Packet::free(txCsmaca);	//don't do this, since the packet will be automatically deleted when expired
			csmacaResume();
			return;
		}
	}

	if (txBcnCmd == txCsmaca)
	{
#ifdef DEBUG802_15_4
		FrameCtrl frmCtrl;
		frmCtrl.FrmCtrl = HDR_LRWPAN(txBcnCmd)->MHR_FrmCtrl;
		frmCtrl.parse();
		/*
		char frm_type[21];
		if (frmCtrl.frmType == defFrmCtrl_Type_Beacon)
			strcpy(frm_type,"BEACON");
		else
			sprintf(frm_type,"COMMAND_%d",HDR_LRWPAN(txBcnCmd)->MSDU_CmdType);
		*/
		fprintf(stdout,"[%s::%s][%f](node %d) transmit %s to %d: SN = %d, uid = %d, mac_uid = %ld\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,wpan_pName(txBcnCmd),p802_15_4macDA(txBcnCmd),HDR_LRWPAN(txBcnCmd)->MHR_BDSN,HDR_CMN(txBcnCmd)->uid(),HDR_LRWPAN(txBcnCmd)->uid);
#endif
		txPkt = txBcnCmd;
		HDR_CMN(txBcnCmd)->direction() = hdr_cmn::DOWN;
		sendDown(txBcnCmd->copy(), this);		
	}
	else if (txBcnCmd2 == txCsmaca)
	{
#ifdef DEBUG802_15_4
		FrameCtrl frmCtrl2;
		frmCtrl2.FrmCtrl = HDR_LRWPAN(txBcnCmd2)->MHR_FrmCtrl;
		frmCtrl2.parse();
		/*
		char frm_type2[21];
		if (frmCtrl2.frmType == defFrmCtrl_Type_Beacon)
			strcpy(frm_type2,"BEACON");
		else
			sprintf(frm_type2,"COMMAND_%d",HDR_LRWPAN(txBcnCmd2)->MSDU_CmdType);
		*/
		fprintf(stdout,"[%s::%s][%f](node %d) transmit %s to %d: SN = %d, uid = %d, mac_uid = %ld\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,wpan_pName(txBcnCmd2),p802_15_4macDA(txBcnCmd2),HDR_LRWPAN(txBcnCmd2)->MHR_BDSN,HDR_CMN(txBcnCmd2)->uid(),HDR_LRWPAN(txBcnCmd2)->uid);
#endif
		txPkt = txBcnCmd2;
		HDR_CMN(txBcnCmd2)->direction() = hdr_cmn::DOWN;
		sendDown(txBcnCmd2->copy(), this);		
	}
	else if (txData == txCsmaca)
	{
#ifdef DEBUG802_15_4
		fprintf(stdout,"[%s::%s][%f](node %d) transmit DATA (%s) to %d: SN = %d, uid = %d, mac_uid = %ld\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,wpan_pName(txData),p802_15_4macDA(txData),HDR_LRWPAN(txData)->MHR_BDSN,HDR_CMN(txData)->uid(),HDR_LRWPAN(txData)->uid);
#endif
		txPkt = txData;
		HDR_CMN(txData)->direction() = hdr_cmn::DOWN;
		sendDown(txData->copy(), this);		
	}
}

void Mac802_15_4::IFSHandler(void)
{
	hdr_lrwpan* wph;
	hdr_cmn* ch;
	FrameCtrl frmCtrl;
	Packet *pendPkt;
	MACenum status = m_UNDEFINED;
	int i;

	assert(rxData||rxCmd);

	if (rxCmd)
	{
		wph = HDR_LRWPAN(rxCmd);
		frmCtrl.FrmCtrl = wph->MHR_FrmCtrl;
		frmCtrl.parse();

		if (wph->MSDU_CmdType == 0x01)		//Association request
			sscs->MLME_ASSOCIATE_indication(wph->MHR_SrcAddrInfo.addr_64,wph->MSDU_Payload[0],frmCtrl.secu,0);	//ACL ignored in simulation
		else if (wph->MSDU_CmdType == 0x02)	//Association response
		{
			status = (*(MACenum *)(wph->MSDU_Payload + 2));
			//save the short address (if association successful)
			if (status == m_SUCCESS)
				mpib.macShortAddress = *((UINT_16 *)wph->MSDU_Payload);
			dispatch(p_SUCCESS,__FUNCTION__,p_SUCCESS,status);
		}
		else if (wph->MSDU_CmdType == 0x04)	//Data request
		{
			//Continue to send pending packet (an ack. already sent).
			//In implementation, we may not have enough time to check if packets pending. If this is the case,
			//then the pending flag in the ack. should be set to 1, and then send a zero-length data packet
			//if later it turns out there is no packet actually pending.
			//In simulation, we assume having enough time to determine the pending status -- so zero-length packet will never be sent.
			//(refer to page 155, line 46-50)
			i = updateTransacLink(tr_oper_EST,&transacLink1,&transacLink2,frmCtrl.srcAddrMode,wph->MHR_SrcAddrInfo.addr_64);
			if (i != 0)
			{
				i = updateTransacLink(tr_oper_est,&transacLink1,&transacLink2,frmCtrl.srcAddrMode,wph->MHR_SrcAddrInfo.addr_64);
				i = (i == 0)?1:0;
			}
			else	//more than one packet pending
			{
				i = 2;
			}
			if (i > 0)	//packet(s) pending
			{
				pendPkt = getPktFrTransacLink(&transacLink1,frmCtrl.srcAddrMode,wph->MHR_SrcAddrInfo.addr_64);
				wph = HDR_LRWPAN(pendPkt);
				wph->indirect = true;
				frmCtrl.FrmCtrl = wph->MHR_FrmCtrl;
				frmCtrl.parse();
				frmCtrl.setFrmPending(i>1);		//more packet pending?
				wph->MHR_FrmCtrl = frmCtrl.FrmCtrl;
				HDR_CMN(pendPkt)->direction() = hdr_cmn::DOWN;
				if (frmCtrl.frmType == defFrmCtrl_Type_MacCmd)
				{
					if (txBcnCmd == pendPkt)	//it's being processed
					{
						Packet::free(rxCmd);	//we logged the command packet before, here just free it
						rxCmd = 0;
						return;
					}
#ifdef DEBUG802_15_4
					fprintf(stdout,"[%s::%s][%f](node %d) before assign txBcnCmd:\n\t\ttxBeacon\t= %ld\n\t\ttxAck   \t= %ld\n\t\ttxBcnCmd\t= %ld\n\t\ttxBcnCmd2\t= %ld\n\t\ttxData  \t= %ld\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,txBeacon,txAck,txBcnCmd,txBcnCmd2,txData);
#endif
					assert(!txBcnCmd);		//we couldn't receive the data request command if we are processing txBcnCmd
					txBcnCmd = pendPkt->refcopy();	
					waitBcnCmdAck = false;
					numBcnCmdRetry = 0;

				}
				else if (frmCtrl.frmType == defFrmCtrl_Type_Data)
				{
					if (txData == pendPkt)		//it's being processed
					{
						Packet::free(rxCmd);	//we logged the command packet before, here just free it
						rxCmd = 0;
						return;
					}
#ifdef DEBUG802_15_4
					fprintf(stdout,"[%s::%s][%f](node %d) before assign txData:\n\t\ttxBeacon\t= %ld\n\t\ttxAck   \t= %ld\n\t\ttxBcnCmd\t= %ld\n\t\ttxBcnCmd2\t= %ld\n\t\ttxData  \t= %ld\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,txBeacon,txAck,txBcnCmd,txBcnCmd2,txData);
#endif
					assert(!txData);		//we couldn't receive the data request command if we are processing txData
					txData = pendPkt->refcopy();	
					waitDataAck = false;
					numDataRetry = 0;
				}
				//there are two ways to transmit the pending packet for the first time (refer to page 156), w/o or w/ CSMA-CA,
				if (canProceedWOcsmaca(pendPkt))
				{
					//change task field "frFunc"
					if(taskP.taskStatus(TP_mcps_data_request))
					{
						 if (strcmp(taskP.taskFrFunc(TP_mcps_data_request),"csmacaCallBack") == 0)
						 	strcpy(taskP.taskFrFunc(TP_mcps_data_request),"PD_DATA_confirm");
					}
					else if (taskP.taskStatus(TP_mlme_associate_response))
					{
						 if (strcmp(taskP.taskFrFunc(TP_mlme_associate_response),"csmacaCallBack") == 0)
						 	strcpy(taskP.taskFrFunc(TP_mlme_associate_response),"PD_DATA_confirm");
						 //else		//other commands using indirect transmission (may  be none): TBD
					}
					txCsmaca = pendPkt;
					plme_set_trx_state_request(p_TX_ON);
				}
				else
				{
					csmacaResume();
				}
			}
			//else		//may need to send a zero-length packet in implementation
		}
		else if (wph->MSDU_CmdType == 0x08)	//Coordinator realignment
		{
			mpib.macPANId = *((UINT_16 *)wph->MSDU_Payload);
			mpib.macCoordShortAddress = *((UINT_16 *)wph->MSDU_Payload + 2);
			tmp_ppib.phyCurrentChannel = wph->MSDU_Payload[4];
			phy->PLME_SET_request(phyCurrentChannel,&tmp_ppib);
			mpib.macShortAddress = *((UINT_16 *)wph->MSDU_Payload + 5);
			dispatch(p_SUCCESS,__FUNCTION__);
		}
		Packet::free(rxCmd);	//we logged the command packet before, here just free it
		rxCmd = 0;
	}
	else if (rxData)
	{
		wph = HDR_LRWPAN(rxData);
		ch = HDR_CMN(rxData);
		frmCtrl.FrmCtrl = wph->MHR_FrmCtrl;
		frmCtrl.parse();

		if (taskP.taskStatus(TP_mlme_poll_request))
			dispatch(p_SUCCESS,__FUNCTION__,p_SUCCESS,status);
		//else	//do nothing
		//the data waiting timer in data polling expired and the upper layer confirmed
		//with a status NO_DATA -- but we see no reason not to continue passing this data
		//packet to upper layer (note that the ack. has already been sent to data source -- 
		//we shouldn't drop the data packet here).

		//strip off the MAC sublayer header
		ch->size() -= macHeaderLen(wph->MHR_FrmCtrl);
		MCPS_DATA_indication(frmCtrl.srcAddrMode,wph->MHR_SrcAddrInfo.panID,wph->MHR_SrcAddrInfo.addr_64,
			     	frmCtrl.dstAddrMode,wph->MHR_DstAddrInfo.panID,wph->MHR_DstAddrInfo.addr_64,
			     	ch->size(),rxData,wph->ppduLinkQuality,
			     	wph->SecurityUse,wph->ACLEntry);
		rxData = 0;
	}
}

void Mac802_15_4::backoffBoundHandler(void)
{
	if (!beaconWaiting)
	if (txAck)		//<txAck> may have been cancelled by <macBeaconRxTimer>
	{
#ifdef DEBUG802_15_4
		fprintf(stdout,"[%s::%s][%f](node %d) transmit M_ACK to %d: SN = %d, uid = %d, mac_uid = %ld\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,p802_15_4macDA(txAck),HDR_LRWPAN(txAck)->MHR_BDSN,HDR_CMN(txAck)->uid(),HDR_LRWPAN(txAck)->uid);
#endif
		txPkt = txAck;
		HDR_CMN(txAck)->direction() = hdr_cmn::DOWN;
		sendDown(txAck->refcopy(), this);
	}
}

// End of file: p802_15_4mac.cc
