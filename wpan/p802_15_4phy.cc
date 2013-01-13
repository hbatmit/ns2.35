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

// File:  p802_15_4phy.cc
// Mode:  C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t

// $Header: /cvsroot/nsnam/ns-2/wpan/p802_15_4phy.cc,v 1.7 2011/06/22 04:03:26 tom_henderson Exp $

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
#include "p802_15_4phy.h"
#include "p802_15_4mac.h"
#include "p802_15_4sscs.h"
#include "p802_15_4const.h"
#include "p802_15_4timer.h"
#include "p802_15_4trace.h"
#include "p802_15_4fail.h"
#include "p802_15_4nam.h"

#ifndef MAX
#define MAX(x,y)        (((x)>(y))?(x):(y))
#endif
#ifndef MIN
#define MIN(x,y)        (((x)<(y))?(x):(y))
#endif


PHY_PIB Phy802_15_4::PPIB =
{
	def_phyCurrentChannel,
	def_phyChannelsSupported,
	def_phyTransmitPower,
	def_phyCCAMode
};

void Phy802_15_4Timer::start(double wtime)
{
	active = true;
	nullEvent.uid_ = 0;
	Scheduler::instance().schedule(this,&nullEvent,wtime);
}

void Phy802_15_4Timer::cancel(void)
{
	active = false;
	Scheduler::instance().cancel(&nullEvent);
}

void Phy802_15_4Timer::handle(Event* e)
{
	active = false;
	if (type == phyCCAHType)
		phy->CCAHandler();
	else if(type == phyEDHType)
		phy->EDHandler();
	else if(type == phyTRXHType)
		phy->TRXHandler();
	else if(type == phyRecvOverHType)
		phy->recvOverHandler((Packet *)e);
	else if(type == phySendOverHType)
		phy->sendOverHandler();
	else if(type == phyCCAReportHType) // 2.31 change: new timer added for CCA reporting
		phy->CCAReportHandler();
	else	
		assert(0);
}

static class Phy802_15_4Class : public TclClass {
public:
	Phy802_15_4Class() : TclClass("Phy/WirelessPhy/802_15_4") {}
	TclObject* create(int, const char*const*) {
		return (new Phy802_15_4(&(Phy802_15_4::PPIB)));
	}
} class_Phy802_15_4;

Phy802_15_4::Phy802_15_4(PHY_PIB *pp)
: WirelessPhy(),
  CCAH(this, phyCCAHType),
  EDH(this, phyEDHType),
  TRXH(this, phyTRXHType),
  recvOverH(this, phyRecvOverHType),
  sendOverH(this, phySendOverHType),
  CCAReportH(this, phyCCAReportHType) // 2.31 change: new timer added for CCA reporting
{
	int i;
	ppib = *pp;
	trx_state = p_RX_ON;
	trx_state_defer_set = p_IDLE;
	tx_state = p_IDLE;
	rxPkt = 0;
	for (i=0;i<27;i++)
	{
		rxTotPower[i] = 0.0;
		rxTotNum[i] = 0;
		rxThisTotNum[i] = 0;
	}
	mac = 0;
	T_transition_local_ = T_transition_; // 2.31 change: set local variable since WirelessPhy::T_transition_ is not visible to CsmaCA802_15_4
}

void Phy802_15_4::macObj(Mac802_15_4 *m)
{
	mac = m;
}

bool Phy802_15_4::channelSupported(UINT_8 channel)
{
	return ((ppib.phyChannelsSupported & (1 << channel)) != 0);
}

double Phy802_15_4::getRate(char dataOrSymbol)
{
	double rate;
	
	if (ppib.phyCurrentChannel == 0)
	{
		if (dataOrSymbol == 'd')
			rate = BR_868M;
		else
			rate = SR_868M;
	}
	else if (ppib.phyCurrentChannel <= 10)
	{
		if (dataOrSymbol == 'd')
			rate = BR_915M;
		else
			rate = SR_915M;
	}
	else
	{
		if (dataOrSymbol == 'd')
			rate = BR_2_4G;
		else
			rate = SR_2_4G;
	}
	return (rate*1000);
}

double Phy802_15_4::trxTime(Packet *p,bool phyPkt)
{
	int phyHeaderLen;
	double trx;
	hdr_cmn* ch = HDR_CMN(p);
	
	phyHeaderLen = (phyPkt)?0:defPHY_HEADER_LEN;
	trx = ((ch->size() + phyHeaderLen)*8/getRate('d'));
	return trx;
}

void Phy802_15_4::construct_PPDU(UINT_8 psduLength,Packet *psdu)
{
	//not really a new packet in simulation, but just update some packet header fields.
	hdr_lrwpan* wph = HDR_LRWPAN(psdu);
	hdr_cmn* ch = HDR_CMN(psdu);
	
	wph->SHR_PreSeq = defSHR_PreSeq;
	wph->SHR_SFD = defSHR_SFD;
	wph->PHR_FrmLen = psduLength;
	//also set channel (for filtering in simulation)
	wph->phyCurrentChannel = ppib.phyCurrentChannel;
	ch->size() = psduLength + defPHY_HEADER_LEN;
	ch->txtime() = trxTime(psdu,true);	
}

void Phy802_15_4::PD_DATA_request(UINT_8 psduLength,Packet *psdu)
{
	hdr_cmn* ch = HDR_CMN(psdu);

	//check packet length
	if (psduLength > aMaxPHYPacketSize)
	{
		fprintf(stdout,"[%s::%s][%f](node %d) Invalid PSDU/MPDU length: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %u, size = %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,wpan_pName(psdu),p802_15_4macSA(psdu),p802_15_4macDA(psdu),ch->uid(),HDR_LRWPAN(psdu)->uid,ch->size());
		Packet::free(psdu);
		mac->PD_DATA_confirm(p_UNDEFINED);
		return;
	}
	
	if (trx_state == p_TX_ON)
	{
		if (tx_state == p_IDLE)
		{
#ifdef DEBUG802_15_4
			fprintf(stdout,"[%s::%s][%f](node %d) sending pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,wpan_pName(psdu),p802_15_4macSA(psdu),p802_15_4macDA(psdu),ch->uid(),HDR_LRWPAN(psdu)->uid,ch->size());
#endif
			//construct a PPDU packet (not really a new packet in simulation, but still <psdu>)
			construct_PPDU(psduLength,psdu);
			//somehow the packet size is set to 0 after sendDown() -- ok, the packet is out and anything can happen (we shouldn't care once it's out)
			//so we have to calculate the transmission time before sendDown()
			double trx_time = trxTime(psdu,true);
			//send the packet to Radio (channel target) for transmission
			txPkt = psdu;
			txPktCopy = psdu->copy();	//for debug purpose, we still want to access the packet after transmission is done
			sendDown(psdu);			//WirelessPhy::sendDown()
			tx_state = p_BUSY;		//for carrier sense
			sendOverH.start(trx_time);
		}
		else	//impossible
			assert(0);
	}
	else
	{
#ifdef DEBUG802_15_4
		fprintf(stdout,"[D][TRX][%s::%s][%f](node %d) dropping pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,wpan_pName(psdu),p802_15_4macSA(psdu),p802_15_4macDA(psdu),ch->uid(),HDR_LRWPAN(psdu)->uid,ch->size());
#endif
		Packet::free(psdu);
		mac->PD_DATA_confirm(trx_state);
	}
}

void Phy802_15_4::PD_DATA_indication(UINT_8 ,Packet *psdu,UINT_8 ppduLinkQuality)
{
	//refer to sec 6.7.8 for LQI details
	hdr_lrwpan* wph = HDR_LRWPAN(psdu);

	wph->ppduLinkQuality = ppduLinkQuality;

/*	2.31 change. sendUp() function call is not necessary as recv() already does it. 
	Including this only causes energy to be decremented a second time.
	if (sendUp(psdu) == 0)
		Packet::free(psdu);
	else
*/
		uptarget_->recv(psdu, (Handler*) 0);
}

void Phy802_15_4::PLME_CCA_request()
{
	if (trx_state == p_RX_ON)
	{
		//perform CCA
		//refer to sec 6.7.9 for CCA details
		//we need to delay 8 symbols
		CCAH.start(4/getRate('s')); // 2.32 change: start CCA at the end of 4th symbol
	}
	else
		mac->PLME_CCA_confirm(trx_state);
}

void Phy802_15_4::PLME_ED_request()
{
	if (trx_state == p_RX_ON)
	{
		//perform ED
		//refer to sec 6.7.7 for ED implementation details
		//we need to delay 8 symbols
		rxEDPeakPower = rxTotPower[ppib.phyCurrentChannel];
		EDH.start(8/getRate('s'));
	}
	else
		mac->PLME_ED_confirm(trx_state,0);
}

void Phy802_15_4::PLME_GET_request(PPIBAenum PIBAttribute)
{
	PHYenum t_status;
	
	switch(PIBAttribute)
	{
		case phyCurrentChannel:
		case phyChannelsSupported:
		case phyTransmitPower:
		case phyCCAMode:
			t_status = p_SUCCESS;
			break;
		default:
			t_status = p_UNSUPPORT_ATTRIBUTE;
			break;
	}
	mac->PLME_GET_confirm(t_status,PIBAttribute,&ppib);
}

void Phy802_15_4::PLME_SET_TRX_STATE_request(PHYenum state)
{
	bool delay;
	PHYenum t_status;
	
	//ignore any pending request
	if (trx_state_defer_set != p_IDLE)
		trx_state_defer_set = p_IDLE;
	else if (TRXH.active)
	{
		TRXH.cancel();
	}

	t_status = trx_state;
	if (state != trx_state)
	{
		delay = false;
		if (((state == p_RX_ON)||(state == p_TRX_OFF))&&(tx_state == p_BUSY))
		{
			t_status = p_BUSY_TX;
			trx_state_defer_set = state;
		}
		else if (((state == p_TX_ON)||(state == p_TRX_OFF))
		        &&(rxPkt)&&(!HDR_CMN(rxPkt)->error()))			//if after received a valid SFD
		{
			t_status = p_BUSY_RX;
			trx_state_defer_set = state;
		}
		else if (state == p_FORCE_TRX_OFF)
		{
			t_status = (trx_state == p_TRX_OFF)?p_TRX_OFF:p_SUCCESS;
			trx_state = p_TRX_OFF;
			//terminate reception if needed
			if (rxPkt)
			{
#ifdef DEBUG802_15_4
				hdr_cmn *ch = HDR_CMN(rxPkt);
				hdr_lrwpan *wph = HDR_LRWPAN(rxPkt);
				fprintf(stdout,"[%s::%s][%f](node %d) FORCE_TRX_OFF sets error bit for incoming pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,wpan_pName(rxPkt),p802_15_4macSA(rxPkt),p802_15_4macDA(rxPkt),ch->uid(),wph->uid,ch->size());
#endif
				HDR_CMN(rxPkt)->error() = 1;	//incomplete reception -- force packet discard
			}
			//terminate transmission if needed
			if (tx_state == p_BUSY)
			{
#ifdef DEBUG802_15_4
				hdr_cmn *ch = HDR_CMN(txPkt);
				hdr_lrwpan *wph = HDR_LRWPAN(txPkt);
				fprintf(stdout,"[%s::%s][%f](node %d) FORCE_TRX_OFF sets error bit for outgoing pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,wpan_pName(txPkt),p802_15_4macSA(txPkt),p802_15_4macDA(txPkt),ch->uid(),wph->uid,ch->size());
#endif
				HDR_CMN(txPkt)->error() = 1;
				sendOverH.cancel();
				Packet::free(txPktCopy);
				last_tx_time=NOW;
				tx_state = p_IDLE;
				mac->PD_DATA_confirm(p_TRX_OFF);
				if (trx_state_defer_set != p_IDLE)
					trx_state_defer_set = p_IDLE;
			}
		}
		else
		{
			t_status = p_SUCCESS;
			if (((state == p_RX_ON)&&(trx_state == p_TX_ON))
			  ||((state == p_TX_ON)&&(trx_state == p_RX_ON)))
			{
				trx_state_turnaround = state;
				delay = true;
			}
			else
				trx_state = state;
		}
		//we need to delay <aTurnaroundTime> symbols if Tx2Rx or Rx2Tx
		if (delay)
		{
			trx_state = p_TRX_OFF;	//should be disabled immediately (further transmission/reception will not succeed)
			TRXH.start(aTurnaroundTime/getRate('s'));
		}
		else
			mac->PLME_SET_TRX_STATE_confirm(t_status);
	}
	else
		mac->PLME_SET_TRX_STATE_confirm(t_status);
#ifdef DEBUG802_15_4
		fprintf(stdout,"[%s::%s][%f](node %d) SET TRX: old = %s req = %s ret = %s\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,
			(trx_state == p_RX_ON)?"RX_ON":
			(trx_state == p_TX_ON)?"TX_ON":
			(trx_state == p_TRX_OFF)?"TRX_OFF":"???",
			(state == p_RX_ON)?"RX_ON":
			(state == p_TX_ON)?"TX_ON":
			(state == p_TRX_OFF)?"TRX_OFF":
			(state == p_FORCE_TRX_OFF)?"FORCE_TRX_OFF":"???",
			(t_status == p_RX_ON)?"RX_ON":
			(t_status == p_TX_ON)?"TX_ON":
			(t_status == p_TRX_OFF)?"TRX_OFF":
			(t_status == p_BUSY_TX)?"BUSY_TX":
			(t_status == p_BUSY_RX)?"BUSY_RX":
			(t_status == p_SUCCESS)?"SUCCESS":"???");
#endif
}

void Phy802_15_4::PLME_SET_request(PPIBAenum PIBAttribute,PHY_PIB *PIBAttributeValue)
{
	PHYenum t_status;
	
	t_status = p_SUCCESS;
	switch(PIBAttribute)
	{
		case phyCurrentChannel:
			if (!channelSupported(PIBAttributeValue->phyCurrentChannel))
				t_status = p_INVALID_PARAMETER;
			if (ppib.phyCurrentChannel != PIBAttributeValue->phyCurrentChannel)
			{
				//any packet in transmission or reception will be corrupted
				if (rxPkt)
				{
#ifdef DEBUG802_15_4
					hdr_cmn *ch = HDR_CMN(rxPkt);
					hdr_lrwpan *wph = HDR_LRWPAN(rxPkt);
					fprintf(stdout,"[%s::%s][%f](node %d) SET phy channel sets error bit for incoming pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,wpan_pName(rxPkt),p802_15_4macSA(rxPkt),p802_15_4macDA(rxPkt),ch->uid(),wph->uid,ch->size());
#endif
					HDR_CMN(rxPkt)->error() = 1;
				}
				if (tx_state == p_BUSY)
				{
#ifdef DEBUG802_15_4
					hdr_cmn *ch = HDR_CMN(txPkt);
					hdr_lrwpan *wph = HDR_LRWPAN(txPkt);
					fprintf(stdout,"[%s::%s][%f](node %d) SET phy channel sets error bit for outgoing pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,wpan_pName(txPkt),p802_15_4macSA(txPkt),p802_15_4macDA(txPkt),ch->uid(),wph->uid,ch->size());
#endif
					HDR_CMN(txPkt)->error() = 1;
					sendOverH.cancel();
					Packet::free(txPktCopy);
					last_tx_time=NOW;
					tx_state = p_IDLE;
					mac->PD_DATA_confirm(p_TRX_OFF);
					if (trx_state_defer_set != p_IDLE)
						trx_state_defer_set = p_IDLE;
				}
				ppib.phyCurrentChannel = PIBAttributeValue->phyCurrentChannel;
			}
			break;
		case phyChannelsSupported:
			if ((PIBAttributeValue->phyChannelsSupported&0xf8000000) != 0)	//5 MSBs reserved
				t_status = p_INVALID_PARAMETER;
			else
				ppib.phyChannelsSupported = PIBAttributeValue->phyChannelsSupported;
			break;
		case phyTransmitPower:
			if (PIBAttributeValue->phyTransmitPower > 0xbf)
				t_status = p_INVALID_PARAMETER;
			else
				ppib.phyTransmitPower = PIBAttributeValue->phyTransmitPower;
			break;
		case phyCCAMode:
			if ((PIBAttributeValue->phyCCAMode < 1)
			 || (PIBAttributeValue->phyCCAMode > 3))
				t_status = p_INVALID_PARAMETER;
			else
				ppib.phyCCAMode = PIBAttributeValue->phyCCAMode;
			break;
		default:
			t_status = p_UNSUPPORT_ATTRIBUTE;
			break;
	}
	mac->PLME_SET_confirm(t_status,PIBAttribute);
}

UINT_8 Phy802_15_4::measureLinkQ(Packet *p)
{
	//Link quality measurement is somewhat simulation/implementation specific;
	//here's our way:
	int lq,lq2;

	//consider energy
	/* Linux floating number compatibility
	lq = (int)((p->txinfo_.RxPr/RXThresh_)*128);
	*/
	{
	double tmpf;
	tmpf = p->txinfo_.RxPr/RXThresh_;
	lq = (int)(tmpf * 128);
	}
	if (lq > 255) lq = 255;

	//consider signal-to-noise
	/* Linux floating number compatibility
	lq2 = (int)((p->txinfo_.RxPr/HDR_LRWPAN(p)->rxTotPower)*255);
	*/
	{
	double tmpf;
	tmpf = p->txinfo_.RxPr/HDR_LRWPAN(p)->rxTotPower;
	lq2 = (int)(tmpf * 255);
	}
	
	if (lq > lq2) lq = lq2;		//use worst value
		
	return (UINT_8) lq;
}

void Phy802_15_4::recv(Packet *p, Handler *)
{
	hdr_lrwpan* wph = HDR_LRWPAN(p);
	hdr_cmn *ch = HDR_CMN(p);
        FrameCtrl frmCtrl;
	PacketStamp s;

	switch(ch->direction())
	{
	case hdr_cmn::DOWN:
#ifdef DEBUG802_15_4
		fprintf(stdout,"[%s::%s][%f](node %d) outgoing pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),wph->uid,ch->size());
#endif
		PD_DATA_request((UINT_8) ch->size(),p);
		break;
	case hdr_cmn::UP:
	default:
		if (sendUp(p) == 0)	
		{
// 2.31 change: sendUp(p) returns 0 if the node is asleep or if the received power is less than CS threshold. In the former case, we still need rxTotPower and rxTotNum for CS (for future csma). recvOverHandler frees that packet when it finds that it is not destined for the node. recvOverHandler also decrements rxTotPower and rxTotNum. In the latter case, the packet is simply freed without further action.
			if(propagation_) {
				s.stamp((MobileNode*)node(), ant_, 0, lambda_);
				if (propagation_->Pr(&p->txinfo_, &s, this) < CSThresh_) {
					Packet::free(p);
					return;
				}
			}

                	frmCtrl.FrmCtrl = wph->MHR_FrmCtrl;
                	frmCtrl.parse();
			//tap out
			if (mac->tap() && frmCtrl.frmType == defFrmCtrl_Type_Data)
				mac->tap()->tap(p);
	
			if (node()->energy_model() && node()->energy_model()->adaptivefidelity())
				node()->energy_model()->add_neighbor(p802_15_4macSA(p));
	
			rxTotPower[wph->phyCurrentChannel] += p->txinfo_.RxPr;
			rxTotNum[wph->phyCurrentChannel]++;
//			printf("RxTotNum at node %d is %d\n", index_, rxTotNum[wph->phyCurrentChannel]);
			Scheduler::instance().schedule(&recvOverH, (Event *)p, trxTime(p,true));
//			Packet::free(p);
			return;
		}

		if (updateNFailLink(fl_oper_est,index_) == 0)
		{
			Packet::free(p);
			return;
		}

                if (updateLFailLink(fl_oper_est,p802_15_4macSA(p),index_) == 0)	//broadcast packets can still reach here
                {
                        Packet::free(p);
                        return;
                }

                frmCtrl.FrmCtrl = wph->MHR_FrmCtrl;
                frmCtrl.parse();
		//tap out
		if (mac->tap() && frmCtrl.frmType == defFrmCtrl_Type_Data)
			mac->tap()->tap(p);

		if (node()->energy_model() && node()->energy_model()->adaptivefidelity())
			node()->energy_model()->add_neighbor(p802_15_4macSA(p));

		//Under whatever condition, we should mark the media as busy.
		// --no matter the packet(s) is for this node or not, no matter
		//   what state the transceiver is in, RX_ON,TX_ON or TRX_OFF, and
		//   no matter which channel is being used.
		//Note that current WirelessPhy->sendUp() prevents packets with (Pr < CSThresh_)
		//from reaching here --. need to modify.WirelessPhy->sendUp() if we want to see
		//all the packets here (but seems no reason to do that).
		//	in dB as can be seen from following:
		//	not very clear (now CPThresh_ is just a ratio, not in dB?)
		rxTotPower[wph->phyCurrentChannel] += p->txinfo_.RxPr;
		rxTotNum[wph->phyCurrentChannel]++;
		if (EDH.active)
		if(rxEDPeakPower < rxTotPower[ppib.phyCurrentChannel])
			rxEDPeakPower = rxTotPower[ppib.phyCurrentChannel];
		
		if ((p802_15_4macDA(p) == index_)			//packet for this node
		  ||(p802_15_4macDA(p) == ((int)MAC_BROADCAST)))		//broadcast packet
			rxThisTotNum[wph->phyCurrentChannel]++;

		if (trx_state == p_RX_ON)
		if (wph->phyCurrentChannel == ppib.phyCurrentChannel)	//current channel
		if ((p802_15_4macDA(p) == index_)			//packet for this node
		  ||(p802_15_4macDA(p) == ((int)MAC_BROADCAST)))		//broadcast packet
		if (wph->SHR_SFD == defSHR_SFD)				//valid SFD
		{
#ifdef DEBUG802_15_4
			fprintf(stdout,"[%s::%s][%f](node %d) incoming pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),wph->uid,ch->size());
#endif
			wph->colFlag = false;
			if (rxPkt == 0)
			{
				rxPkt = p;
				HDR_LRWPAN(rxPkt)->rxTotPower = rxTotPower[wph->phyCurrentChannel];
			}
			else
			{
#ifdef DEBUG802_15_4
				fprintf(stdout,"[D][COL][%s::%s][%f](node %d) COLLISION:\n\t First (power:%f): type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n\tSecond (power:%f): type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",
					__FILE__,__FUNCTION__,CURRENT_TIME,index_,
					rxPkt->txinfo_.RxPr,wpan_pName(rxPkt),p802_15_4macSA(rxPkt),p802_15_4macDA(rxPkt),HDR_CMN(rxPkt)->uid(),HDR_LRWPAN(rxPkt)->uid,HDR_CMN(rxPkt)->size(),
					p->txinfo_.RxPr,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),wph->uid,ch->size());
#endif
				wph->colFlag = true;
				HDR_LRWPAN(rxPkt)->colFlag = true;
				mac->nam->flashNodeColor(CURRENT_TIME);
				if (p->txinfo_.RxPr > rxPkt->txinfo_.RxPr)
				{
					//What should we do if there is a transceiver state set pending?
					//  1. continue defering (could be unbounded delay)
					//..2. set transceiver state now (the incoming packet ignored)
					//We select choice 1, as the traffic rate is supposed to be low.
					rxPkt = p;
					HDR_LRWPAN(rxPkt)->rxTotPower = rxTotPower[wph->phyCurrentChannel];
				}
			}

		}
		if (rxPkt)
		if (HDR_LRWPAN(rxPkt)->rxTotPower < rxTotPower[HDR_LRWPAN(rxPkt)->phyCurrentChannel])
			HDR_LRWPAN(rxPkt)->rxTotPower = rxTotPower[HDR_LRWPAN(rxPkt)->phyCurrentChannel];
		assert(ch->size() > 0);
		if (ch->direction() != hdr_cmn::UP)
		{
			printf("Packet-flow direction not specified: sending up the stack on default.\n\n");
			ch->direction() = hdr_cmn::UP;	//we don't want MAC to handle the same problem
		}
		Scheduler::instance().schedule(&recvOverH, (Event *)p, trxTime(p,true));
		break;
	}
}

void Phy802_15_4::CCAHandler(void)
{
	PHYenum t_status;

	//refer to sec 6.7.9 for CCA details
	//  1. CCA will be affected by outgoing packets,
	//     incoming packets (both destined for this device 
	//     and not destined for this device) and other
	//     interferences.
	//  2. In implementation, we don't care about the details 
	//     and just need to perform an actual measurement.
	if ((tx_state == p_BUSY)||(rxTotNum[ppib.phyCurrentChannel] > 0))
	{
		t_status = p_BUSY;
	}
	else if (ppib.phyCCAMode == 1)	//ED detection
	{
		//sec 6.5.3.3 and 6.6.3.4
		// -- receiver sensitivity: -85 dBm or better for 2.4G
		// -- receiver sensitivity: -92 dBm or better for 868M/915M
		//sec 6.7.9
		// -- ED threshold at most 10 dB above receiver sensitivity.
		//For simulations, we simply compare with CSThresh_
		t_status = (rxTotPower[ppib.phyCurrentChannel] >= CSThresh_)?p_BUSY:p_IDLE;
	}
	else if (ppib.phyCCAMode == 2)	//carrier sense only
	{
		t_status = (rxTotNum[ppib.phyCurrentChannel] > 0)?p_BUSY:p_IDLE;
	}
	else //if (ppib.phyCCAMode == 3)	//both
	{
		t_status = ((rxTotPower[ppib.phyCurrentChannel] >= CSThresh_)&&(rxTotNum[ppib.phyCurrentChannel] > 0))?p_BUSY:p_IDLE;
	}
	CCAReportH.start(4/getRate('s')); // 2.31 change: Report CCA at the end of 8th symbol (i.e. wait 4 more symbols)
	sensed_ch_state = t_status; // 2.31 change: CCA reporting is done by CCAReportHandler

// 2.31 change: Decrement energy spent in CCA
	if (node()->energy_model()) { 
		if((NOW-4/getRate('s')-aTurnaroundTime/getRate('s'))-channel_idle_time_ > 0) {
			node()->energy_model()->DecrIdleEnergy((NOW-4/getRate('s')-aTurnaroundTime/getRate('s'))-channel_idle_time_, P_idle_);
		}
		if ((NOW-4/getRate('s')-aTurnaroundTime/getRate('s')+aCCATime/getRate('s'))-channel_idle_time_ > 0){
			node()->energy_model()->DecrRcvEnergy((aTurnaroundTime+aCCATime)/getRate('s'),Pr_consume_);
			channel_idle_time_ = NOW-4/getRate('s')+aCCATime/getRate('s');
			update_energy_time_ = NOW-4/getRate('s')+aCCATime/getRate('s');
		}
		else {
			node()->energy_model()->DecrRcvEnergy((aTurnaroundTime+aCCATime)/getRate('s'),2*Pr_consume_-P_idle_); // P_idle_ has already been decremented; just compensating for that
			channel_idle_time_ = MAX(channel_idle_time_, NOW-4/getRate('s')+aCCATime/getRate('s'));
			update_energy_time_ = MAX(channel_idle_time_,NOW-4/getRate('s')+aCCATime/getRate('s'));
		}
	} 
}

void Phy802_15_4::CCAReportHandler(void) // 2.31 change: New timer added to report CCA
{
	mac->PLME_CCA_confirm(sensed_ch_state);
}

void Phy802_15_4::EDHandler(void)
{
	int energy;
	UINT_8 t_EnergyLevel;

	//refer to sec 6.7.7 for ED implementation details
	//ED is somewhat simulation/implementation specific; here's our way:

	/* Linux floating number compatibility
	energy = (int)((rxEDPeakPower/RXThresh_)*128);
	*/
	{
	double tmpf;
	tmpf = rxEDPeakPower/RXThresh_;
	energy = (int)(tmpf * 128);
	}
	t_EnergyLevel = (energy > 255)?255:energy;
	mac->PLME_ED_confirm(p_SUCCESS,t_EnergyLevel);
}

void Phy802_15_4::TRXHandler(void)
{
	trx_state = trx_state_turnaround;
	//send a confirm
	mac->PLME_SET_TRX_STATE_confirm(trx_state);
}

void Phy802_15_4::recvOverHandler(Packet *p)
{
	UINT_8 lq;
	hdr_lrwpan* wph = HDR_LRWPAN(p);
	hdr_cmn *ch = HDR_CMN(p);
	
	rxTotPower[wph->phyCurrentChannel] -= p->txinfo_.RxPr;
	rxTotNum[wph->phyCurrentChannel]--;
	
	if (rxTotNum[wph->phyCurrentChannel] == 0)
		rxTotPower[wph->phyCurrentChannel] = 0.0;

	if ((p802_15_4macDA(p) != index_)
	  &&(p802_15_4macDA(p) != ((int)MAC_BROADCAST)))	//packet not for this node (interference)
		Packet::free(p);
	else if (p != rxPkt)				//packet corrupted (not the strongest one) or un-detectable
	{
#ifdef DEBUG802_15_4
		fprintf(stdout,"[D][%s][%s::%s::%d][%f](node %d) dropping pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",(wph->phyCurrentChannel != ppib.phyCurrentChannel)?"CHN":"NOT",__FILE__,__FUNCTION__,__LINE__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),wph->uid,ch->size());
#endif
		rxThisTotNum[wph->phyCurrentChannel]--;
		drop(p,(wph->phyCurrentChannel != ppib.phyCurrentChannel)?"CHN":"NOT");
	}
	else
	{
		rxThisTotNum[wph->phyCurrentChannel]--;
		//measure (here calculate) the link quality
		lq = measureLinkQ(p);
		ch->size() -= defPHY_HEADER_LEN;
		rxPkt = 0;
		if ((ch->size() <= 0) 
		    ||(ch->size() > aMaxPHYPacketSize)
		    ||ch->error())		//incomplete reception (due to FORCE_TRX_OFF),data packet received during ED or other errors
		{
#ifdef DEBUG802_15_4
			fprintf(stdout,"[D][ERR][%s::%s::%d][%f](node %d) dropping pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,__LINE__,index_,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),wph->uid,ch->size());
#endif
			drop(p,"ERR");
		}
		else
		{

#ifdef DEBUG802_15_4
			fprintf(stdout,"[%s::%s][%f](node %d) incoming pkt: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d --> PD_DATA_indication()\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,wpan_pName(p),p802_15_4macSA(p),p802_15_4macDA(p),ch->uid(),wph->uid,ch->size());
#endif
			PD_DATA_indication(ch->size(),p,lq);	//MAC sublayer need to further check if the packet
								//is really received successfully or not.
		}
		if (trx_state_defer_set != p_IDLE)
		{
			trx_state_turnaround = trx_state_defer_set;
			trx_state_defer_set = p_IDLE;
			if (trx_state_turnaround == p_TRX_OFF)
			{
				trx_state = trx_state_turnaround;
				mac->PLME_SET_TRX_STATE_confirm(trx_state);
			}
			else
			{
				//we need to delay <aTurnaroundTime> symbols for Rx2Tx
				trx_state = p_TRX_OFF;	//should be disabled immediately (further reception will not succeed)
				TRXH.start(aTurnaroundTime/getRate('s'));
			}
		}
	}
}

void Phy802_15_4::sendOverHandler(void)
{
#ifdef DEBUG802_15_4
	fprintf(stdout,"[%s::%s][%f](node %d) sending over: type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,index_,wpan_pName(txPktCopy),p802_15_4macSA(txPktCopy),p802_15_4macDA(txPktCopy),HDR_CMN(txPktCopy)->uid(),HDR_LRWPAN(txPktCopy)->uid,HDR_CMN(txPktCopy)->size());
#endif
	assert(tx_state == p_BUSY);
	assert(txPktCopy);
	Packet::free(txPktCopy);
	last_tx_time=NOW;
	tx_state = p_IDLE;
	mac->PD_DATA_confirm(p_SUCCESS);
	if (trx_state_defer_set != p_IDLE)
	{
		trx_state_turnaround = trx_state_defer_set;
		trx_state_defer_set = p_IDLE;
		if (trx_state_turnaround == p_TRX_OFF)
		{
			trx_state = trx_state_turnaround;
			mac->PLME_SET_TRX_STATE_confirm(trx_state);
		}
		else
		{
			//we need to delay <aTurnaroundTime> symbols for Rx2Tx
			trx_state = p_TRX_OFF;	//should be disabled immediately (further transmission will not succeed)
			TRXH.start(aTurnaroundTime/getRate('s'));
		}
	}
}

void Phy802_15_4::wakeupNode(int cause)
{
//	cause=0 for beacon reception (to account for sleep-to-wake ramp-up) and 1 for outgoing packtes
	if (node()->energy_model()->sleep())
	{
		node()->energy_model()->set_node_sleep(0);
		node()->energy_model()->DecrSleepEnergy(NOW-channel_sleep_time_-T_transition_,P_sleep_);
		if (cause==0){
			node()->energy_model()->DecrIdleEnergy(T_transition_,P_transition_);
		}
		channel_idle_time_ = NOW; 
		update_energy_time_ = NOW;
		status_ = IDLE; 
	}
}

void Phy802_15_4::putNodeToSleep(void)
{
	node()->energy_model()->set_node_sleep(1);
	channel_sleep_time_=NOW;
	update_energy_time_ = NOW;
	if (mac->wakeupT->busy())
		mac->wakeupT->stop();
	mac->wakeupT->start(); // This function will automatically determine the time to wake up for the next beacon and set itself an alarm
	status_ = SLEEP;
}

// End of file: p802_15_4phy.cc
