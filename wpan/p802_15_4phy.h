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

// File:  p802_15_4phy.h
// Mode:  C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t

// $Header: /cvsroot/nsnam/ns-2/wpan/p802_15_4phy.h,v 1.3 2011/06/22 04:03:26 tom_henderson Exp $

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


#ifndef p802_15_4phy_h
#define p802_15_4phy_h

//#include <scheduler.h>
#include <packet.h>
#include <wireless-phy.h>
#include "p802_15_4def.h"

//PHY enumerations description (Table 16)
typedef enum
{
	p_BUSY = 0,
	p_BUSY_RX,
	p_BUSY_TX,
	p_FORCE_TRX_OFF,
	p_IDLE,
	p_INVALID_PARAMETER,
	p_RX_ON,
	p_SUCCESS,
	p_TRX_OFF,
	p_TX_ON,
	p_UNSUPPORT_ATTRIBUTE,
	p_UNDEFINED			//we added this for handling any case not specified in the draft
}PHYenum;

//PHY PIB attributes (Table 19)
typedef enum
{
	phyCurrentChannel = 0x00,
	phyChannelsSupported,
	phyTransmitPower,
	phyCCAMode
}PPIBAenum;

struct PHY_PIB
{
	UINT_8	phyCurrentChannel;
	UINT_32	phyChannelsSupported;
	UINT_8	phyTransmitPower;
	UINT_8	phyCCAMode;
};

//---handlers---
#define phyCCAHType		1
#define phyEDHType		2
#define phyTRXHType		3
#define phyRecvOverHType	4
#define phySendOverHType	5
#define phyCCAReportHType	6  // 2.31 change: new timer added for CCA reporting
class Phy802_15_4;
class Phy802_15_4Timer : public Handler
{
	friend class Phy802_15_4;
public:
	Phy802_15_4Timer(Phy802_15_4 *p, int tp) : Handler()
	{
		phy = p;
		type = tp;
		active = false;
	}
	virtual void start(double wtime);
	virtual void cancel(void);
	virtual void handle(Event* e);

protected:
	Phy802_15_4 *phy;
	int type;
	bool active;
	Event nullEvent;
};

class Mac802_15_4;
class Phy802_15_4 : public WirelessPhy
{
	friend class Phy802_15_4Timer;
public:
	Phy802_15_4(PHY_PIB *pp);
	void macObj(Mac802_15_4 *m);
	bool channelSupported(UINT_8 channel);
	double getRate(char dataOrSymbol);
	double trxTime(Packet *p,bool phyPkt = false);
	void construct_PPDU(UINT_8 psduLength,Packet *psdu);
	void PD_DATA_request(UINT_8 psduLength,Packet *psdu);
	void PD_DATA_indication(UINT_8 psduLength,Packet *psdu,UINT_8 ppduLinkQuality);
	void PLME_CCA_request();
	void PLME_ED_request();
	void PLME_GET_request(PPIBAenum PIBAttribute);
	void PLME_SET_TRX_STATE_request(PHYenum state);
	void PLME_SET_request(PPIBAenum PIBAttribute,PHY_PIB *PIBAttributeValue);
	UINT_8 measureLinkQ(Packet *p);
	void recv(Packet *p, Handler *h);
	Packet* rxPacket(void) {return rxPkt;}
	void wakeupNode(int cause); // 2.31 change: for MAC to wake up the node
	void putNodeToSleep(); // 2.31 change: for MAC to put the node to sleep
	double channel_sleep_time_;	// 2.31 change: time when node was put to sleep 
	double T_transition_local_;	// 2.31 change: created a local variable since WirelessPhy::T_transition_ is not visible to CsmaCA802_15_4

	
public:
	static PHY_PIB PPIB;
	double last_tx_time;	// 2.31 change: 

protected:
	void	CCAHandler(void);
	void	EDHandler(void);
	void	TRXHandler(void);
	void	recvOverHandler(Packet *p);
	void	sendOverHandler(void);
	void	CCAReportHandler(void); // 2.31 change: new timer added to report CCA

private:
	PHY_PIB ppib;
	PHYenum trx_state;		//tranceiver state: TRX_OFF/TX_ON/RX_ON
	PHYenum trx_state_defer_set;	//defer setting tranceiver state: TX_ON/RX_ON/TRX_OFF/IDLE (IDLE = no defer pending)
	PHYenum trx_state_turnaround;	//defer setting tranceiver state in case Tx2Rx or Rx2Tx
	PHYenum tx_state;		//transmitting state: IDLE/BUSY
	PHYenum sensed_ch_state;	//Stored value of ch state done at beginning of 					//backoffSlot for forwarding to MAC after ccaDetectTime
	Packet *rxPkt;			//the packet meets the following conditions:
					// -- on the current channel
					// -- for this node (not interference)
					// -- with the strongest receiving power among all packets that are for this node and on the current channel
	Packet *txPkt;			//the packet being transmitted
	Packet *txPktCopy;		//the copy of the packet being transmitted
	double rxTotPower[27];		
	double rxEDPeakPower;		
	UINT_32 rxTotNum[27];		
	UINT_32 rxThisTotNum[27];	
	Mac802_15_4 *mac;
	Phy802_15_4Timer CCAH;
	Phy802_15_4Timer EDH;
	Phy802_15_4Timer TRXH;
	Phy802_15_4Timer recvOverH;
	Phy802_15_4Timer sendOverH;
	Phy802_15_4Timer CCAReportH; // 2.31 change: new timer to report CCA
};

#endif

// End of file: p802_15_4phy.h
