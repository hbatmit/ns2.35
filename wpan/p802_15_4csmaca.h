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

// File:  p802_15_4csmaca.h
// Mode:  C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t

// $Header: /cvsroot/nsnam/ns-2/wpan/p802_15_4csmaca.h,v 1.3 2011/06/22 04:03:26 tom_henderson Exp $

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


#ifndef p802_15_4csmaca_h
#define p802_15_4csmaca_h

#include "p802_15_4timer.h"
#include "p802_15_4phy.h"
#include "p802_15_4mac.h"


class CsmaCA802_15_4
{
	friend class macBackoffTimer;
	friend class macWakeupTimer; //2.31 change: Timer to control node shutdown and wakeup
	friend class macBeaconOtherTimer;
	friend class macDeferCCATimer;
	friend class Mac802_15_4;
public:
	CsmaCA802_15_4(Phy802_15_4 *p, Mac802_15_4 *m);
	~CsmaCA802_15_4();

protected:
	void	reset(void);
	double	adjustTime(double wtime);
	bool	canProceed(double wtime, bool afterCCA = false);
	void	newBeacon(char trx);
	void	start(bool firsttime,Packet *pkt = 0,bool ackreq = 0);
	void	cancel(void);
	void	backoffHandler(void);
	void	RX_ON_confirm(PHYenum status);
	void	bcnOtherHandler(void);
	void	deferCCAHandler(void);
	void	CCA_confirm(PHYenum status);

private:
	//timers
	macBackoffTimer		*backoffT;
	macBeaconOtherTimer	*bcnOtherT;
	macDeferCCATimer	*deferCCAT;

	Phy802_15_4 *phy;
	Mac802_15_4 *mac;
	UINT_8 NB;
	UINT_8 CW;
	UINT_8 BE;

	bool ackReq;
	bool beaconEnabled,beaconOther;
	bool waitNextBeacon;
	double bcnTxTime,bcnRxTime;
	double bPeriod;				//backoff periods
	int bPeriodsLeft;			//backoff periods left for next superframe (negative value means no backoff)
	Packet *txPkt;
};

#endif

// End of file: p802_15_4csmaca.h
