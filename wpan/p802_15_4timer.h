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

// File:  p802_15_4timer.h
// Mode:  C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t

// $Header: /cvsroot/nsnam/ns-2/wpan/p802_15_4timer.h,v 1.3 2011/06/22 04:03:26 tom_henderson Exp $

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


#ifndef p802_15_4timer_h
#define p802_15_4timer_h

#include <scheduler.h>
#include <assert.h>
#include <math.h>

class Mac802_15_4;

//--base timer class for MAC sublayer---

class Mac802_15_4Timer : public Handler
{
public:
	Mac802_15_4Timer();
	void		reset(void);
	virtual void	handle(Event *e) = 0;		
	virtual void	start(double time);
	virtual void	stop(void);
	virtual void	pause(void) {assert(0);}	//to be overloaded in subclass
	virtual void	resume(void) {assert(0);}	//to be overloaded in subclass
	inline int	busy(void) {return busy_;}
	inline int	paused(void) {return paused_;}
	inline double	expire(void) 			//remaining time
	{
		return ((stime + wtime) - Scheduler::instance().clock());
	}

protected:
	int		busy_;
	int		paused_;
	Event		event;
	double		stime;		//start time
	double		wtime;		//waiting time
};

//---timers for MAC sublayer---

class CsmaCA802_15_4;
class macBackoffTimer : public Mac802_15_4Timer
{
public:
	macBackoffTimer(CsmaCA802_15_4 *csma);
	//void	start(double time, double st, bool idle);
	void	handle(Event *e);
	//void	pause(void);
	//void	resume(double beacontime);
	/*
	inline void slot_wtime(double beacontime)	//calculate slot waiting time -- force timers to expire on slot boundries
	{
		double rmd;
		assert(slottime);
		rmd = fmod(Scheduler::instance().clock() + wtime - beacontime, slottime);
		if(rmd > 0.0)
			wtime += (slottime - rmd);
		else
			wtime -= rmd;	//no effect
	}
	*/
private:
	CsmaCA802_15_4	*csmaca;
};

class macBeaconOtherTimer : public Mac802_15_4Timer
{
public:
	macBeaconOtherTimer(CsmaCA802_15_4 *csma);
	void	handle(Event *e);
private:
	CsmaCA802_15_4	*csmaca;
};

class macDeferCCATimer : public Mac802_15_4Timer
{
public:
	macDeferCCATimer(CsmaCA802_15_4 *csma);
	void	handle(Event *e);
private:
	CsmaCA802_15_4	*csmaca;
};

class macTxOverTimer : public Mac802_15_4Timer
{
public:
	macTxOverTimer(Mac802_15_4 *m) : Mac802_15_4Timer() {mac = m;}
	void	handle(Event *e);
private:
	Mac802_15_4	*mac;
};

class macTxTimer : public Mac802_15_4Timer
{
public:
	macTxTimer(Mac802_15_4 *m) : Mac802_15_4Timer() {mac = m;}
	void	handle(Event *e);
private:
	Mac802_15_4	*mac;
};

class macExtractTimer : public Mac802_15_4Timer
{
public:
	macExtractTimer(Mac802_15_4 *m) : Mac802_15_4Timer() {mac = m;onlyCAP = false;}
	void	backoffCAP(double time);
	void	start(double time,bool onlycap);
	void	stop(void);
	void	handle(Event *e);
	void	resume(void);
private:
	Mac802_15_4	*mac;
	double		leftTime;
	bool		onlyCAP;
};

class macAssoRspWaitTimer : public Mac802_15_4Timer
{
public:
	macAssoRspWaitTimer(Mac802_15_4 *m) : Mac802_15_4Timer() {mac = m;}
	void	handle(Event *e);
private:
	Mac802_15_4	*mac;
};

class macDataWaitTimer : public Mac802_15_4Timer
{
public:
	macDataWaitTimer(Mac802_15_4 *m) : Mac802_15_4Timer() {mac = m;}
	void	handle(Event *e);
private:
	Mac802_15_4	*mac;
};

class macRxEnableTimer : public Mac802_15_4Timer
{
public:
	macRxEnableTimer(Mac802_15_4 *m) : Mac802_15_4Timer() {mac = m;}
	void	handle(Event *e);
private:
	Mac802_15_4	*mac;
};

class macScanTimer : public Mac802_15_4Timer
{
public:
	macScanTimer(Mac802_15_4 *m) : Mac802_15_4Timer() {mac = m;}
	void	handle(Event *e);
private:
	Mac802_15_4	*mac;
};

class macBeaconTxTimer : public Mac802_15_4Timer
{
public:
	macBeaconTxTimer(Mac802_15_4 *m) : Mac802_15_4Timer() {macBeaconOrder_last = 15; mac = m;}
	void	start(bool reset = false, bool fortx = false, double wt = 0.0);
	void	handle(Event *e);
private:
	bool		forTX;
	unsigned char	macBeaconOrder_last;
	Mac802_15_4	*mac;
};

class macBeaconRxTimer : public Mac802_15_4Timer
{
public:
	macBeaconRxTimer(Mac802_15_4 *m) : Mac802_15_4Timer() {mac = m;lastTime = 0.0;}
	void	start(void);
	void	handle(Event *e);
private:
	Mac802_15_4	*mac;
	double		lastTime;
};

class macBeaconSearchTimer : public Mac802_15_4Timer
{
public:
	macBeaconSearchTimer(Mac802_15_4 *m) : Mac802_15_4Timer() {mac = m;}
	void	handle(Event *e);
private:
	Mac802_15_4	*mac;
};

//2.31 change: Timer to control node shutdown and wakeup
class macWakeupTimer : public Mac802_15_4Timer
{
public:
	macWakeupTimer(Mac802_15_4 *m) : Mac802_15_4Timer() {mac = m;lastTime = 0.0;}
	void	start(void);
	void	handle(Event *e);
private:
	Mac802_15_4	*mac;
	double		lastTime;
};

#endif

// End of file: p802_15_4timer.h
