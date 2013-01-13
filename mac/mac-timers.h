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
 */
/* Ported from CMU/Monarch's code, nov'98 -Padma.*/

#ifndef __mac_timers_h__
#define __mac_timers_h__

/* ======================================================================
   Timers
   ====================================================================== */
class Mac802_11;

class MacTimer : public Handler {
public:
	MacTimer(Mac802_11* m) : mac(m) {
		busy_ = paused_ = 0; stime = rtime = 0.0;
	}

	virtual void handle(Event *e) = 0;

	virtual void start(double time);
	virtual void stop(void);
	virtual void pause(void) { assert(0); }
	virtual void resume(void) { assert(0); }

	inline int busy(void) { return busy_; }
	inline int paused(void) { return paused_; }
	inline double expire(void) {
		return ((stime + rtime) - Scheduler::instance().clock());
	}

protected:
	Mac802_11	*mac;
	int		busy_;
	int		paused_;
	Event		intr;
	double		stime;	// start time
	double		rtime;	// remaining time
};


class BackoffTimer : public MacTimer {
public:
	BackoffTimer(Mac802_11 *m) : MacTimer(m), difs_wait(0.0) {}



	void	start(int cw, int idle, double difs = 0.0);
	void	handle(Event *e);
	void	pause(void);
	void	resume(double difs);
private:
	double	difs_wait;
};

class DeferTimer : public MacTimer {
public:
	DeferTimer(Mac802_11 *m) : MacTimer(m) {}

	void	start(double);
	void	handle(Event *e);
};

class BeaconTimer : public MacTimer {
public:
	BeaconTimer(Mac802_11 *m) : MacTimer(m) {}

	void	start(double);
	void	handle(Event *e);
};

class ProbeTimer : public MacTimer {
public:
	ProbeTimer(Mac802_11 *m) : MacTimer(m) {}

	void	start(double);
	void	handle(Event *e);
};


class IFTimer : public MacTimer {
public:
	IFTimer(Mac802_11 *m) : MacTimer(m) {}

	void	handle(Event *e);
};

class NavTimer : public MacTimer {
public:
	NavTimer(Mac802_11 *m) : MacTimer(m) {}

	void	handle(Event *e);
};

class RxTimer : public MacTimer {
public:
	RxTimer(Mac802_11 *m) : MacTimer(m) {}

	void	handle(Event *e);
};

class TxTimer : public MacTimer {
public:
	TxTimer(Mac802_11 *m) : MacTimer(m) {}

	void	handle(Event *e);
};

#endif /* __mac_timers_h__ */

