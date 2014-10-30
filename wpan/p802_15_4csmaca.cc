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

// File:  p802_15_4csmaca.cc
// Mode:  C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t

// $Header: /cvsroot/nsnam/ns-2/wpan/p802_15_4csmaca.cc,v 1.7 2011/10/02 22:32:35 tom_henderson Exp $

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


#include "p802_15_4csmaca.h"
#include "p802_15_4const.h"
#include "p802_15_4trace.h"

#ifndef MAX
#define MAX(x,y)        (((x)>(y))?(x):(y))
#endif
#ifndef MIN
#define MIN(x,y)        (((x)<(y))?(x):(y))
#endif



CsmaCA802_15_4::CsmaCA802_15_4(Phy802_15_4 *p, Mac802_15_4 *m)
{
	phy = p;
	mac = m;
	txPkt = 0;
	waitNextBeacon = false;
	backoffT = new macBackoffTimer(this);
	assert(backoffT);
	bcnOtherT = new macBeaconOtherTimer(this);
	assert(bcnOtherT);
	deferCCAT = new macDeferCCATimer(this);
	assert(deferCCAT);
}

CsmaCA802_15_4::~CsmaCA802_15_4()
{
	delete backoffT;
	delete bcnOtherT;
	delete deferCCAT;
}

void CsmaCA802_15_4::reset(void)
{
	if (beaconEnabled)
	{
		NB = 0;
		CW = 2;
		BE = mac->mpib.macMinBE;
		if ((mac->mpib.macBattLifeExt)&&(BE > 2))
			BE = 2;
	}
	else
	{
		NB = 0;
		BE = mac->mpib.macMinBE;
	}
}

double CsmaCA802_15_4::adjustTime(double wtime)
{
	//find the beginning point of CAP and adjust the scheduled time
	//if it comes before CAP
	double neg;
	double tmpf;

	assert(txPkt);
	if (!mac->toParent(txPkt))
	{
		if (mac->mpib.macBeaconOrder != 15)
		{
			/* Linux floating number compatibility
			neg = (CURRENT_TIME + wtime - bcnTxTime) - mac->beaconPeriods * bPeriod;
			*/
			{
			tmpf = mac->beaconPeriods * bPeriod;
			tmpf = CURRENT_TIME - tmpf;
			tmpf += wtime;
			neg = tmpf - bcnTxTime;
			}

			if (neg < 0.0)
				wtime -= neg;
			return wtime;
		}
		else
			return wtime;
	}
	else
	{
		if (mac->macBeaconOrder2 != 15)
		{
			/* Linux floating number compatibility
			neg = (CURRENT_TIME + wtime - bcnRxTime) - mac->beaconPeriods2 * bPeriod;
			*/
			{
			tmpf = mac->beaconPeriods2 * bPeriod;
			tmpf = CURRENT_TIME - tmpf;
			tmpf += wtime;
			neg = tmpf - bcnRxTime;
			}

			if (neg < 0.0)
				wtime -= neg;
			return wtime;
		}
		else
			return wtime;
	}
}

bool CsmaCA802_15_4::canProceed(double wtime, bool afterCCA)
{
	//check if can proceed within the current superframe
	//(in the case the node acts as both a coordinator and a device, both the superframes from and to this node should be taken into account)
	bool ok;
	UINT_16 t_bPeriods,t_CAP;
	double t_fCAP,t_CCATime,t_IFS,t_transacTime,bcnOtherTime,BI2;

	waitNextBeacon = false;
	wtime = mac->locateBoundary(mac->toParent(txPkt),wtime);
	if (!mac->toParent(txPkt))
	{
		if (mac->mpib.macBeaconOrder != 15)
		{
			if (mac->sfSpec.BLE)
				t_CAP = mac->getBattLifeExtSlotNum();
			else
				t_CAP = (mac->sfSpec.FinCAP + 1) * (mac->sfSpec.sd / aUnitBackoffPeriod) - mac->beaconPeriods;	//(mac->sfSpec.sd % aUnitBackoffPeriod) = 0

			/* Linux floating number compatibility
			t_bPeriods = (UINT_16)(((CURRENT_TIME + wtime - bcnTxTime) / bPeriod) - mac->beaconPeriods);
			*/
			{
			double tmpf;
			tmpf = CURRENT_TIME + wtime;
			tmpf -= bcnTxTime;
			tmpf /= bPeriod;
			t_bPeriods = (UINT_16)(tmpf - mac->beaconPeriods);
			}

			/* Linux floating number compatibility
			if (fmod(CURRENT_TIME + wtime - bcnTxTime, bPeriod) > 0.0)
			*/
			double tmpf;
			tmpf = CURRENT_TIME + wtime;
			tmpf -= bcnTxTime;
			if (fmod(tmpf, bPeriod) > 0.0)
				t_bPeriods++;
			bPeriodsLeft = t_bPeriods - t_CAP;
		}
		else
			bPeriodsLeft = -1;
	}
	else
	{
		if (mac->macBeaconOrder2 != 15)
		{
			BI2 = mac->sfSpec2.BI / phy->getRate('s');
			
			/* Linux floating number compatibility
			t_CAP = (UINT_16)((mac->macBcnRxTime + (mac->sfSpec2.FinCAP + 1) * mac->sfSpec2.sd ) / phy->getRate('s'));
			*/
			{
			double tmpf;
			tmpf = (mac->sfSpec2.FinCAP + 1) * mac->sfSpec2.sd;
			tmpf += mac->macBcnRxTime;
			t_CAP = (UINT_16)(tmpf / phy->getRate('s'));
			}

			/* Linux floating number compatibility
			if (t_CAP + aMaxLostBeacons * BI2 < CURRENT_TIME)
			*/
			double tmpf;
			tmpf = aMaxLostBeacons * BI2;
			if (t_CAP + tmpf < CURRENT_TIME)	
				bPeriodsLeft = -1;
			else
			{
				if (mac->sfSpec2.BLE)
					t_CAP = mac->getBattLifeExtSlotNum();
				else
					t_CAP = (mac->sfSpec2.FinCAP + 1) * (mac->sfSpec2.sd / aUnitBackoffPeriod) - mac->beaconPeriods2;	

				/* Linux floating number compatibility
				t_bPeriods = (UINT_16)(((CURRENT_TIME + wtime - bcnRxTime) / bPeriod) - mac->beaconPeriods2);
				*/
				{
				double tmpf;
				tmpf = CURRENT_TIME + wtime;
				tmpf -= bcnRxTime;
				tmpf /= bPeriod;
				t_bPeriods = (UINT_16)(tmpf - mac->beaconPeriods2);
				}

				/* Linux floating number compatibility
				if (fmod(CURRENT_TIME + wtime - bcnRxTime, bPeriod) > 0.0)
				*/
				double tmpf;
				tmpf = CURRENT_TIME + wtime;
				tmpf -= bcnRxTime;
				if (fmod(tmpf, bPeriod) > 0.0)
					t_bPeriods++;
				bPeriodsLeft = t_bPeriods - t_CAP;
			}
		}
		else
			bPeriodsLeft = -1;
	}

	ok = true;
	if (bPeriodsLeft > 0)
		ok = false;
	else if (bPeriodsLeft == 0)
	{
		if ((!mac->toParent(txPkt))
		&&  (!mac->sfSpec.BLE))
			ok = false;
		else if ((mac->toParent(txPkt))
		&&  (!mac->sfSpec2.BLE))
			ok = false;
	}
	if (!ok)
	{
#ifdef DEBUG802_15_4
		fprintf(stdout,"[%s::%s][%f](node %d) cannot proceed: bPeriodsLeft = %d, orders = %d/%d/%d, type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,mac->index_,bPeriodsLeft,mac->mpib.macBeaconOrder,mac->macBeaconOrder2,mac->macBeaconOrder3,wpan_pName(txPkt),p802_15_4macSA(txPkt),p802_15_4macDA(txPkt),ch->uid(),HDR_LRWPAN(txPkt)->uid,ch->size());
#endif
		if (mac->macBeaconOrder2 != 15)
		if (!mac->bcnRxT->busy())
			mac->bcnRxT->start();
		waitNextBeacon = true;
		return false;
	}

	//calculate the time needed to finish the transaction
	t_CCATime = 8 / phy->getRate('s');
	if (HDR_CMN(txPkt)->size() <= aMaxSIFSFrameSize)
		t_IFS = aMinSIFSPeriod;
	else
		t_IFS = aMinLIFSPeriod;
	t_IFS /= phy->getRate('s');
	t_transacTime  = mac->locateBoundary(mac->toParent(txPkt),wtime) - wtime;				//boundary location time -- should be 0 here, since we have already located the boundary
	if (!afterCCA)
	{
		t_transacTime += t_CCATime;									//first CCA time
		t_transacTime += mac->locateBoundary(mac->toParent(txPkt),t_transacTime) - (t_transacTime);	//boundary location time for second CCA
		t_transacTime += t_CCATime;									//second CCA time
	}
	t_transacTime += mac->locateBoundary(mac->toParent(txPkt),t_transacTime) - (t_transacTime);		//boundary location time for transmission
	t_transacTime += phy->trxTime(txPkt);									//packet transmission time
	if (ackReq)
	{
		t_transacTime += mac->mpib.macAckWaitDuration/phy->getRate('s');				//ack. waiting time (this value does not include round trip propagation delay)
		t_transacTime += 2*max_pDelay;									//round trip propagation delay (802.15.4 ignores this, but it should be there even though it is very small)
		t_transacTime += t_IFS;										//IFS time -- not only ensure that the sender can finish the transaction, but also the receiver
		t_fCAP = mac->getCAP(true);

		/* Linux floating number compatibility
		if (CURRENT_TIME + wtime + t_transacTime > t_fCAP)
		*/
		double tmpf;
		tmpf = CURRENT_TIME + wtime;
		tmpf += t_transacTime;
		if (tmpf > t_fCAP)
			ok = false;
		else
			ok= true;
	}
	else
	{
		//in this case, we need to handle individual CAP 
		ok = true;
		t_fCAP = mac->getCAPbyType(1);

		/* Linux floating number compatibility
		if (CURRENT_TIME + wtime + t_transacTime > t_fCAP)
		*/
		double tmpf;
		tmpf = CURRENT_TIME + wtime;
		tmpf += t_transacTime;
		if (tmpf > t_fCAP)
			ok = false;
		if (ok)
		{
			t_fCAP = mac->getCAPbyType(2);
			t_transacTime += max_pDelay;						//one-way trip propagation delay (802.15.4 ignores this, but it should be there even though it is very small)
			t_transacTime += 12/phy->getRate('s');					//transceiver turn-around time (receiver may need to do this to transmit next beacon)
			t_transacTime += t_IFS;							//IFS time -- not only ensure that the sender can finish the transaction, but also the receiver

			/* Linux floating number compatibility
			if (CURRENT_TIME + wtime + t_transacTime > t_fCAP)
			*/
			double tmpf;
			tmpf = CURRENT_TIME + wtime;
			tmpf += t_transacTime;
			if (tmpf > t_fCAP)
				ok = false;
		}
		if (ok)
		{
			t_fCAP = mac->getCAPbyType(3);
			t_transacTime -= t_IFS;							//the third node does not need to handle the transaction

			/* Linux floating number compatibility
			if (CURRENT_TIME + wtime + t_transacTime > t_fCAP)
			*/
			double tmpf;
			tmpf = CURRENT_TIME + wtime;
			tmpf += t_transacTime;
			if (tmpf > t_fCAP)
				ok = false;
		}
	}

	//check if have enough CAP to finish the transaction
	if (!ok)
	{
		bPeriodsLeft = 0;
		if ((mac->mpib.macBeaconOrder == 15)
		&&  (mac->macBeaconOrder2 == 15)
		&&  (mac->macBeaconOrder3 != 15))
		{
			/* Linux floating number compatibility
			bcnOtherTime = (mac->macBcnOtherRxTime + mac->sfSpec3.BI) / phy->getRate('s');
			*/
			{
			double tmpf;
			tmpf = (mac->macBcnOtherRxTime + mac->sfSpec3.BI);
			bcnOtherTime = tmpf / phy->getRate('s');
			}

			while (bcnOtherTime < CURRENT_TIME)
				bcnOtherTime += (mac->sfSpec3.BI / phy->getRate('s'));
			bcnOtherT->start(bcnOtherTime - CURRENT_TIME);
		}
#ifdef DEBUG802_15_4
	fprintf(stdout,"[%s::%s][%f](node %d) cannot proceed: orders = %d/%d/%d, type = %s, src = %d, dst = %d, uid = %d, mac_uid = %ld, size = %d\n",__FILE__,__FUNCTION__,CURRENT_TIME,mac->index_,mac->mpib.macBeaconOrder,mac->macBeaconOrder2,mac->macBeaconOrder3,wpan_pName(txPkt),p802_15_4macSA(txPkt),p802_15_4macDA(txPkt),ch->uid(),HDR_LRWPAN(txPkt)->uid,ch->size());
#endif
		if (mac->macBeaconOrder2 != 15)
		if (!mac->bcnRxT->busy())
			mac->bcnRxT->start();
		waitNextBeacon = true;
		return false;
	}
	else
	{
		bPeriodsLeft = -1;
		return true;
	}
}

void CsmaCA802_15_4::newBeacon(char )
{
	//this function will be called by MAC each time a new beacon received or sent within the current PAN
	double rate,wtime;

	if (!mac->txAck)
		mac->plme_set_trx_state_request(p_RX_ON);	

	if (bcnOtherT->busy())
		bcnOtherT->stop();

	//update values
	beaconEnabled = ((mac->mpib.macBeaconOrder != 15)||(mac->macBeaconOrder2 != 15));
	beaconOther = (mac->macBeaconOrder3 != 15);
	reset();	
	rate = phy->getRate('s');
	bcnTxTime = mac->macBcnTxTime / rate;
	bcnRxTime = mac->macBcnRxTime / rate;
	bPeriod = aUnitBackoffPeriod / rate;

	if (waitNextBeacon)
	if ((txPkt)
        && (!backoffT->busy()))
	{
		assert(bPeriodsLeft >= 0);
		if (bPeriodsLeft == 0)
		{
			wtime = adjustTime(0.0);
			if (canProceed(wtime));
				backoffHandler();	//no need to resume backoff
		}
		else
		{
			wtime = adjustTime(0.0);
			wtime += bPeriodsLeft * bPeriod;
			if (canProceed(wtime));
				backoffT->start(wtime);
		}
	}
	waitNextBeacon = false;
}

void CsmaCA802_15_4::start(bool firsttime,Packet *pkt,bool ackreq)
{
	bool backoff;
	double rate,wtime,BI2;


	if (mac->txAck)
	{
		mac->backoffStatus = 0;
		txPkt = 0;
		return;
	}

	assert(backoffT->busy() == 0);
	if (firsttime)
	{
		beaconEnabled = ((mac->mpib.macBeaconOrder != 15)||(mac->macBeaconOrder2 != 15));
		beaconOther = (mac->macBeaconOrder3 != 15);
		reset();	
		assert(txPkt == 0);
		txPkt = pkt;
		ackReq = ackreq;
		rate = phy->getRate('s');
		bPeriod = aUnitBackoffPeriod / rate;
		if (beaconEnabled)
		{
			bcnTxTime = mac->macBcnTxTime / rate;
			bcnRxTime = mac->macBcnRxTime / rate;
			//it's possible we missed some beacons
			BI2 = (mac->sfSpec2.BI / phy->getRate('s'));
			if (mac->macBeaconOrder2 != 15)
			while (bcnRxTime + BI2 < CURRENT_TIME)
				bcnRxTime += BI2;
		}
	}

	wtime = (Random::random() % (1<<BE)) * bPeriod;
#ifdef SHUTDOWN
	if (BE == mac->mpib.macMinBE)
		wtime=MAX(wtime,ceil(phy->T_transition_local_/bPeriod)*bPeriod); // 2.31 change: added this to take care of sleep-idle ramp-up
#endif

	wtime = adjustTime(wtime);
	backoff = true;
	if (beaconEnabled||beaconOther)
	{
		if (beaconEnabled)
		if (firsttime)
			wtime = mac->locateBoundary(mac->toParent(txPkt),wtime);
		if (!canProceed(wtime))		
			backoff = false;
	}
	if (backoff)
		backoffT->start(wtime);
}

void CsmaCA802_15_4::cancel(void)
{
	if (bcnOtherT->busy())
		bcnOtherT->stop();
	else if (backoffT->busy())
		backoffT->stop();
	else if (deferCCAT->busy())
		deferCCAT->stop();
	else
		mac->taskP.taskStatus(TP_CCA_csmaca) = false;
	txPkt = 0;
}

void CsmaCA802_15_4::backoffHandler(void)
{
	mac->taskP.taskStatus(TP_RX_ON_csmaca) = true;
	mac->plme_set_trx_state_request(p_RX_ON);
}

void CsmaCA802_15_4::RX_ON_confirm(PHYenum status)
{
	double wtime;

	if (status != p_RX_ON)
	{
		if (status == p_BUSY_TX)
			mac->taskP.taskStatus(TP_RX_ON_csmaca) = true;
		else
			backoffHandler();
		return;
	}

	//locate backoff boundary if needed
	if (beaconEnabled)
		wtime = mac->locateBoundary(mac->toParent(txPkt),0.0);
	else
		wtime = 0.0;

	if (wtime == 0.0)
	{
		mac->taskP.taskStatus(TP_CCA_csmaca) = true;
		phy->PLME_CCA_request();
	}
	else
		deferCCAT->start(wtime);
}

void CsmaCA802_15_4::bcnOtherHandler(void)
{
	newBeacon('R');
}

void CsmaCA802_15_4::deferCCAHandler(void)
{
	mac->taskP.taskStatus(TP_CCA_csmaca) = true;
	phy->PLME_CCA_request();
}

void CsmaCA802_15_4::CCA_confirm(PHYenum status)
{
	//This function should be called when mac receiving CCA_confirm.
	bool idle;

	idle = (status == p_IDLE)?1:0;	
	if (idle)
	{
		if ((!beaconEnabled)&&(!beaconOther))
		{
			txPkt = 0;
			mac->csmacaCallBack(p_IDLE);
		}
		else
		{
			if (beaconEnabled)
				CW--;
			else
				CW = 0;
			if (CW == 0)
			{
				//timing condition may not still hold -- check again
				if (canProceed(0.0, true))
				{
					txPkt = 0;
					mac->csmacaCallBack(p_IDLE);
				}
				else	//postpone until next beacon sent or received
				{
					if (beaconEnabled) CW = 2;
					bPeriodsLeft = 0;
				}
			}
			else	//perform CCA again
				backoffHandler();
		}
	}
	else	//busy
	{
		if (beaconEnabled) CW = 2;
		NB++;
		if (NB > mac->mpib.macMaxCSMABackoffs)
		{
			txPkt = 0;
			mac->csmacaCallBack(p_BUSY);
		}
		else	//backoff again
		{
			BE++;
			if (BE > aMaxBE)
				BE = aMaxBE;
			start(false);
		}
	}
}

// End of file: p802_15_4csmaca.cc
