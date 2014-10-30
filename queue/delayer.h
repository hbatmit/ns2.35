/*
 * Copyright (c) 2003  International Computer Science Institute
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
 *      This product includes software developed by ICIR, the ICSI
 *      Center for Internet Research (ICSI: the International Computer
 *      Science Institute).
 * 4. Neither the name of ICIR nor of ICSI may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICSI AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ICSI OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */


#include "connector.h"
#include "random.h"
#include "timer-handler.h"
#include "ranvar.h"

class Delayer;

/* handler for Delayer's target (link) */
class TargetHandler : public Handler {
public:
        inline TargetHandler(Delayer& d) : delayer_(d) {}
        inline void handle(Event*);
private:
        Delayer& delayer_;
};

/* timer for channel allocation delay */
class AllocTimer : public TimerHandler {
public:
        inline AllocTimer(Delayer& d) : delayer_(d) {}
        inline void expire(Event*);
private:
        Delayer& delayer_;
};

/* timer for delay spikes */
class SpikeTimer : public TimerHandler {
public:
        inline SpikeTimer(Delayer& d) : delayer_(d) {}
        inline void expire(Event*);
private:
        Delayer& delayer_;
};

class Delayer : public Connector {
  public:
	Delayer();
 	int command(int argc, const char*const* argv);
 	void recv(Packet* p, Handler* h);
	void try_send();
	inline void freeTarget() {target_free = 1;}
	inline void freeAlloc() {alloc_free = 1;}
	inline void freeSpike() {spike_free = 1;}
	inline void takeSpike() {spike_free = 0;}
	int getSpike() {return spike_free;}
	double getNextSpikeLen () {return spike_len->value(); }
	double getNextSpikeInt () {return spike_int->value(); }

  private:
        double last_sent;       /* time a packet has last left the queue */
        RandomVariable *alloc_int; /* time of keeping channel without data */
        RandomVariable *alloc_len; /* delay to allocate a new channel */
        int alloc_free;              /* are we in channel allocation delay? */
	AllocTimer  at_;

        RandomVariable *spike_int; /* interval between delay spikes */
        RandomVariable *spike_len; /* delay spike length */
        int spike_free;              /* are we in a delay spike? */
	SpikeTimer st_;

	int target_free;	  /* is the link free? */
	TargetHandler th_;
	Handler *prev_h_;	  /* downstream handler */
	Packet  *pkt_;		  /* packet we store */
	Event e;		  /* something to give to prev_h_ */
};

static class DelayerClass : public TclClass {
public:
        DelayerClass() : TclClass("Delayer") {}
        TclObject* create(int, const char*const*) {
                return (new Delayer());
        }
} delayer_class;
