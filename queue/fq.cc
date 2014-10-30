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
 * 	This product includes software developed by the MASH Research
 * 	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
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

#ifndef lint
static const char rcsid[] =
"@(#) $Header: /cvsroot/nsnam/ns-2/queue/fq.cc,v 1.12 2005/08/22 05:08:34 tomh Exp $ (ANS)";
#endif

#include "config.h"
#include <stdlib.h>
#include <float.h>
#include "queue.h"

/*XXX*/
#define MAXFLOW 32

class FQ : public Queue {
public: 
	FQ();
	virtual int command(int argc, const char*const* argv);
	Packet *deque(void);
	void enque(Packet *pkt);
	void recv(Packet* p, Handler* h);
protected:
	int update();
	struct flowState {
		Queue* q_;
		Packet* hol_;	/* head-of-line packet for each flow */
		double finishTime_; /* FQ finish time for hol packet */
		double delta_;
		Handler* handler_;
	} fs_[MAXFLOW];
	inline double finish(const flowState& fs, int nactive)
	{
		return (fs.finishTime_ + fs.delta_ * nactive);
	}
	int maxflow_;
	double secsPerByte_;
};

static class FQClass : public TclClass {
public:
	FQClass() : TclClass("Queue/FQ") {}
	TclObject* create(int, const char*const*) {
		return (new FQ);
	}
} class_fq;

FQ::FQ()
{
	for (int i = 0; i < MAXFLOW; ++i) {
		fs_[i].q_ = 0;
		fs_[i].hol_ = 0;
		fs_[i].finishTime_ = 0.;
	}
	maxflow_ = -1;
	secsPerByte_ = 0.;
	bind("secsPerByte_", &secsPerByte_);
}

int FQ::command(int argc, const char*const* argv)
{
	if (argc == 4) {
		if (strcmp(argv[1], "install") == 0) {
			int flowID = atoi(argv[2]);
			fs_[flowID].q_ = (Queue*)TclObject::lookup(argv[3]);
			if (flowID > maxflow_)
				maxflow_ = flowID;
			/*XXX*/
			if (flowID >= MAXFLOW)
				abort();
			return (TCL_OK);
		}
	}
	return (Queue::command(argc, argv));
}

/*XXX this is quite inefficient.*/
int FQ::update()
{
	int nactive = 0;
	for (int i = 0; i <= maxflow_; ++i) {
		Queue* q = fs_[i].q_;
		if (q != 0) {
			if (fs_[i].hol_ == 0) {
				Packet* p = q->deque();
				if (p != 0) {
					fs_[i].hol_ = p;
					++nactive;
				}
			} else
				++nactive;
		}
	}
	return (nactive);
}

Packet* FQ::deque()

{
	int nactive = update();
	int target = -1;
	double best = DBL_MAX;
	for (int i = 0; i <= maxflow_; ++i) {
		if (fs_[i].hol_ != 0) {
			if (target < 0) { 
				target = i;
				best = finish(fs_[i], nactive);
			} else {
				double F = finish(fs_[i], nactive);
				if (F < best) {
					target = i;
					best = F;
				}
			}
		}
	}
	if (target >= 0) {
		Packet* p = fs_[target].hol_;
		fs_[target].hol_ = 0;
		fs_[target].finishTime_ = best;
		/* let this upstream queue resume */
		Handler* h = fs_[target].handler_;
		/*XXX null event okay because queue doesn't use it*/
		h->handle(0);
		return (p);
	}
	return (0);
}

/*
 * Called when one of our queues is unblocked by us in FQ::deque
 * (or gets its first packet).
 */
void FQ::recv(Packet* p, Handler* handler)
{
	hdr_ip* h = hdr_ip::access(p);
	int flowid = h->flowid();
	/* shouldn't be called when head-of-line is pending */
	if (flowid >= MAXFLOW || fs_[flowid].hol_ != 0)
		abort();

	/*
	 * Put this packet at the head-of-line for its queue
	 * and set up scheduling state according to the
	 * standard fair-queueing "finish time" equation.
	 */
	fs_[flowid].hol_ = p;
	double now = Scheduler::instance().clock();
	if (now > fs_[flowid].finishTime_)
		fs_[flowid].finishTime_ = now;
	fs_[flowid].handler_ = handler;
	int size = hdr_cmn::access(p)->size();
	fs_[flowid].delta_ = size * secsPerByte_;

	if (!blocked_) {
		/*
		 * We're not blocked.  Get a packet and send it on.
		 * We perform an extra check because the queue
		 * might drop the packet even if it was
		 * previously empty!  (e.g., RED can do this.)
		 */
		p = deque();
		if (p != 0) {
			blocked_ = 1;
			target_->recv(p, &qh_);
		}
	}
}

void FQ::enque(Packet*)
{
	/* should never be called because we override recv */
	abort();
}
