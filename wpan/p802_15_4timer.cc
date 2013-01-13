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

// File:  p802_15_4timer.cc
// Mode:  C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t

// $Header: /cvsroot/nsnam/ns-2/wpan/p802_15_4timer.cc,v 1.4 2011/06/22 04:03:26 tom_henderson Exp $

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


#include <packet.h>
#include <random.h>
#include "p802_15_4csmaca.h"
#include "p802_15_4timer.h"


//--base timer class for MAC sublayer---

Mac802_15_4Timer::Mac802_15_4Timer()
{
	reset();
}

void Mac802_15_4Timer::reset(void)
{
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	wtime = 0.0;
}

void Mac802_15_4Timer::start(double time)
{
	Scheduler &s = Scheduler::instance();

	assert(busy_ == 0);
	busy_ = 1;
	paused_ = 0;
	stime = s.clock();
	wtime = time;
	assert(wtime >= 0.0);
	event.uid_ = 0;
	s.schedule(this, &event, wtime);
}

void Mac802_15_4Timer::stop(void)
{
	Scheduler &s = Scheduler::instance();

	assert(busy_);
	if(paused_ == 0)
		s.cancel(&event);
	reset();
}

//---timers for MAC sublayer---

macBackoffTimer::macBackoffTimer(CsmaCA802_15_4 *csma) : Mac802_15_4Timer()
{
	csmaca = csma;
}

/*
void macBackoffTimer::start(double time, double st, bool idle)
{
	wtime = time;
	slottime = st;

	if(idle == 0)
		paused_ = 1;
	else
		Mac802_15_4Timer::start(wtime);
}
*/

void macBackoffTimer::handle(Event *)
{
	reset();
	csmaca->backoffHandler();
}

/*
void macBackoffTimer::pause()
{
	int slots;
	double sr;
	Scheduler &s = Scheduler::instance();

	sr = s.clock() - stime;
	slots = (int)(sr/slottime);
	if(slots < 0) slots = 0;
	assert(busy_ && ! paused_);
	paused_ = 1;
	wtime -= (slots * slottime);
	assert(wtime >= 0.0);
	s.cancel(&event);
}

void macBackoffTimer::resume(double beacontime)
{
	Scheduler &s = Scheduler::instance();

	assert(busy_ && paused_);
	paused_ = 0;
	stime = s.clock();
	assert(wtime >= 0.0);
	s.schedule(this, &event, wtime);
}
*/

//------------------------------------------------------

macBeaconOtherTimer::macBeaconOtherTimer(CsmaCA802_15_4 *csma) : Mac802_15_4Timer()
{
	csmaca = csma;
}

void macBeaconOtherTimer::handle(Event *)
{
	reset();
	csmaca->bcnOtherHandler();
}

//------------------------------------------------------

macDeferCCATimer::macDeferCCATimer(CsmaCA802_15_4 *csma) : Mac802_15_4Timer()
{
	csmaca = csma;
}

void macDeferCCATimer::handle(Event *)
{
	reset();
	csmaca->deferCCAHandler();
}

//------------------------------------------------------

void macTxOverTimer::handle(Event *)
{
	reset();
	mac->txOverHandler();
}

//------------------------------------------------------

void macTxTimer::handle(Event *)
{
	reset();
	mac->txHandler();
}

//------------------------------------------------------

void macExtractTimer::backoffCAP(double time)
{
	double t_bcnRxTime,t_sSlotDuration,t_endCAP,t_endBackoff;

	t_bcnRxTime = mac->macBcnRxTime / mac->phy->getRate('s');
	t_sSlotDuration = mac->sfSpec2.sd / mac->phy->getRate('s');

	/* Linux floating number compatibility
	t_endCAP = t_bcnRxTime + (mac->sfSpec2.FinCAP + 1) * t_sSlotDuration;
	*/
	{
	double tmpf;
	tmpf = (mac->sfSpec2.FinCAP + 1) * t_sSlotDuration;
	t_endCAP = t_bcnRxTime + tmpf;
	}

	t_endBackoff = CURRENT_TIME + time;
	if (t_endBackoff > t_endCAP)	//count-down should be paused at the end of CAP and resumed when receiving next superframe
		leftTime = t_endBackoff - t_endCAP;
	else
	{
		onlyCAP = false;
		Mac802_15_4Timer::start(time);
	}
}

void macExtractTimer::start(double time,bool onlycap)
{
	if (!onlycap)
	{
		onlyCAP = false;
		Mac802_15_4Timer::start(time);
	}
	else
	{
		onlyCAP = true;
		backoffCAP(time);
	}
}

void macExtractTimer::stop(void)
{
	if (onlyCAP)
		onlyCAP = false;
	else
		Mac802_15_4Timer::stop();
}

void macExtractTimer::handle(Event *)
{
	onlyCAP = false;
	reset();
	mac->extractHandler();
}

void macExtractTimer::resume(void)
{
	//this function will be called by MAC each time a new beacon transmitted by the coordinator (only in beacon enabled mode)
	if (onlyCAP)
		backoffCAP(leftTime);
}

//------------------------------------------------------

void macAssoRspWaitTimer::handle(Event *)
{
	reset();
	mac->assoRspWaitHandler();
}

//------------------------------------------------------

void macDataWaitTimer::handle(Event *)
{
	reset();
	mac->dataWaitHandler();
}

//------------------------------------------------------

void macRxEnableTimer::handle(Event *)
{
	reset();
	mac->rxEnableHandler();
}

//------------------------------------------------------

void macScanTimer::handle(Event *)
{
	reset();
	mac->scanHandler();
}

//------------------------------------------------------

void macBeaconTxTimer::start(bool reset, bool fortx, double wt)
{
	double wtime;

	if (reset)
	{
		forTX = fortx;
		macBeaconOrder_last = 15;
		if (Mac802_15_4Timer::busy())
			Mac802_15_4Timer::stop();
		Mac802_15_4Timer::start(wt);
		return;
	}
	else
	{
		forTX = (!forTX);
	}
	if (!forTX)
		Mac802_15_4Timer::start(12 / mac->phy->getRate('s'));
	else if (mac->mpib.macBeaconOrder != 15)
	{
		wtime = ((aBaseSuperframeDuration * (1 << mac->mpib.macBeaconOrder) - 12) / mac->phy->getRate('s'));
		Mac802_15_4Timer::start(wtime);
		macBeaconOrder_last = mac->mpib.macBeaconOrder;
	}
	else if (macBeaconOrder_last != 15)
	{
		wtime = ((aBaseSuperframeDuration * (1 << macBeaconOrder_last) - 12) / mac->phy->getRate('s'));
		Mac802_15_4Timer::start(wtime);
	}
}

void macBeaconTxTimer::handle(Event *)
{
	reset();
	mac->beaconTxHandler(forTX);
}

//------------------------------------------------------

void macBeaconRxTimer::start(void)
{
	double BI,bcnRxTime,now,len12s,wtime;
	double tmpf;

	BI = (aBaseSuperframeDuration * (1 << mac->macBeaconOrder2)) / mac->phy->getRate('s');
	bcnRxTime = mac->macBcnRxTime / mac->phy->getRate('s');
	now = CURRENT_TIME;
	while (now - bcnRxTime > BI)
		bcnRxTime += BI;
	len12s = 12 / mac->phy->getRate('s');

	/* Linux floating number compatibility
	wtime = BI - (now - bcnRxTime);
	*/
	{
	tmpf = (now - bcnRxTime);;
	wtime = BI - tmpf;
	}

	if (wtime >= len12s)
		wtime -= len12s;

	/* Linux floating number compatibility
	if (now + wtime - lastTime < BI - len12s)
	*/
	tmpf = now + wtime;
	if (tmpf - lastTime < BI - len12s)
	{
		tmpf = 2 * BI;
		tmpf = tmpf - now;
		tmpf = tmpf + bcnRxTime;
		wtime = tmpf - len12s;
		//wtime = 2* BI - (now - bcnRxTime) - len12s;
	}
	lastTime = now + wtime;
	Mac802_15_4Timer::start(wtime);
}

void macBeaconRxTimer::handle(Event *)
{
	reset();
	mac->beaconRxHandler();
}

//------------------------------------------------------

void macBeaconSearchTimer::handle(Event *)
{
	reset();
	mac->beaconSearchHandler();
}

// 2.31 change: Timer to control node shutdown and wakeup
void macWakeupTimer::start(void)
{
	double BI,bcnRxTime,now,wtime;
	double tmpf;
	BI = (aBaseSuperframeDuration * (1 << mac->macBeaconOrder2)) / mac->phy->getRate('s');
	bcnRxTime = mac->macBcnRxTime / mac->phy->getRate('s');
	now = CURRENT_TIME;
	while (now - bcnRxTime > BI)
	bcnRxTime += BI;
	{
		tmpf = (now - bcnRxTime);;
		wtime = BI - tmpf-aTurnaroundTime/mac->phy->getRate('s');
//		wtime=wtime-3*mac->csmaca->bPeriod;
		if (wtime < 0) {
			printf("WARNING: negative time for wakeup timer");
			abort();
		}
	}
	tmpf = now + wtime;
	lastTime = now + wtime;
	Mac802_15_4Timer::start(wtime);
}

// 2.31 change: Timer to control node shutdown and wakeup
void macWakeupTimer::handle(Event *)
{
	reset();
	EnergyModel *em = mac->netif_->node()->energy_model();
	if (em)
	{
		mac->phy->wakeupNode(0);
	}
}
// End of file: p802_15_4timer.cc
