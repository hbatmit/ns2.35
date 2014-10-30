/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Ported from CMU/Monarch's code, nov'98 -Padma.
 * Contributions by:
 *   - Mike Holland
 */

#include <delay.h>
#include <connector.h>
#include <packet.h>
#include <random.h>
 
// #define DEBUG
//#include <debug.h>
#include <arp.h>
#include <ll.h>
#include <mac.h>
#include <mac-timers.h>
#include <mac-802_11.h> 

/*
 * Force timers to expire on slottime boundries.
 */
// #define USE_SLOT_TIME

// change wrt Mike's code

 #ifdef USE_SLOT_TIME
 #error "Incorrect slot time implementation - don't use USE_SLOT_TIME..."
 #endif

#define ROUND_TIME()	\
	{								\
		assert(slottime);					\
		double rmd = remainder(s.clock() + rtime, slottime);	\
		if(rmd > 0.0)						\
			rtime += (slottime - rmd);			\
		else							\
			rtime += (-rmd);				\
	}


/* ======================================================================
   Timers
   ====================================================================== */

void
MacTimer::start(double time)
{
	Scheduler &s = Scheduler::instance();
	assert(busy_ == 0);

	busy_ = 1;
	paused_ = 0;
	stime = s.clock();
	rtime = time;
	assert(rtime >= 0.0);


	s.schedule(this, &intr, rtime);
}

void
MacTimer::stop(void)
{
	Scheduler &s = Scheduler::instance();

	assert(busy_);

	if(paused_ == 0)
		s.cancel(&intr);

	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;
}

/* ======================================================================
   Defer Timer
   ====================================================================== */
void
DeferTimer::start(double time)
{
	Scheduler &s = Scheduler::instance();

	assert(busy_ == 0);

	busy_ = 1;
	paused_ = 0;
	stime = s.clock();
	rtime = time;
#ifdef USE_SLOT_TIME
	ROUND_TIME();
#endif
	assert(rtime >= 0.0);

	s.schedule(this, &intr, rtime);
}


void    
DeferTimer::handle(Event *)
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;



	mac->deferHandler();
}


/* ======================================================================
   Beacon Timer
   ====================================================================== */
void
BeaconTimer::start(double time)
{
	Scheduler &s = Scheduler::instance();

	assert(busy_ == 0);

	busy_ = 1;
	paused_ = 0;
	stime = s.clock();
	rtime = time;

	assert(rtime >= 0.0);

	s.schedule(this, &intr, rtime);
}


void    
BeaconTimer::handle(Event *)
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;

	mac->BeaconHandler();
}

/* ======================================================================
   Probe Timer
   ====================================================================== */
void
ProbeTimer::start(double time)
{
	Scheduler &s = Scheduler::instance();

	assert(busy_ == 0);

	busy_ = 1;
	paused_ = 0;
	stime = s.clock();
	rtime = time;

	assert(rtime >= 0.0);

	s.schedule(this, &intr, rtime);
}


void    
ProbeTimer::handle(Event *)
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;

	mac->ProbeHandler();
}

/* ======================================================================
   NAV Timer
   ====================================================================== */
void    
NavTimer::handle(Event *)
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;

	mac->navHandler();
}


/* ======================================================================
   Receive Timer
   ====================================================================== */
void    
RxTimer::handle(Event *)
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;

	mac->recvHandler();
}


/* ======================================================================
   Send Timer
   ====================================================================== */
void    
TxTimer::handle(Event *)
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;



	mac->sendHandler();
}


/* ======================================================================
   Interface Timer
   ====================================================================== */
void
IFTimer::handle(Event *)
{
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;

	mac->txHandler();
}


/* ======================================================================
   Backoff Timer
   ====================================================================== */
void
BackoffTimer::handle(Event *)
{
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;
	difs_wait = 0.0;

	mac->backoffHandler();
}

void
BackoffTimer::start(int cw, int idle, double difs)
{
	Scheduler &s = Scheduler::instance();

	assert(busy_ == 0);

	busy_ = 1;
	paused_ = 0;
	stime = s.clock();
	
	rtime = (Random::random() % cw) * mac->phymib_.getSlotTime();
#ifdef USE_SLOT_TIME
	ROUND_TIME();
#endif
	difs_wait = difs;

	if(idle == 0)
		paused_ = 1;
	else {
		assert(rtime + difs_wait >= 0.0);
		s.schedule(this, &intr, rtime + difs_wait);
	}
}


void
BackoffTimer::pause()
{
	Scheduler &s = Scheduler::instance();

	//the caculation below make validation pass for linux though it
	// looks dummy

	double st = s.clock();
	

	double rt = stime + difs_wait;
	double sr = st - rt;
	double mst = (mac->phymib_.getSlotTime());

	
        int slots = int (sr/mst);
	
	
	if(slots < 0)
		slots = 0;
	assert(busy_ && ! paused_);

	paused_ = 1;
	rtime -= (slots * mac->phymib_.getSlotTime());

	assert(rtime >= 0.0);

	difs_wait = 0.0;

	s.cancel(&intr);
}


void
BackoffTimer::resume(double difs)
{
	Scheduler &s = Scheduler::instance();

	assert(busy_ && paused_);

	paused_ = 0;
	stime = s.clock();

	/*
	 * The media should be idle for DIFS time before we start
	 * decrementing the counter, so I add difs time in here.
	 */
	difs_wait = difs;
	/*
#ifdef USE_SLOT_TIME
	ROUND_TIME();
#endif
	*/
 
	assert(rtime + difs_wait >= 0.0);
       	s.schedule(this, &intr, rtime + difs_wait);
}


