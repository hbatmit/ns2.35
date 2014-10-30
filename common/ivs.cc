/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1996-1997 Regents of the University of California.
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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/common/ivs.cc,v 1.16 2000/09/01 03:04:05 haoboy Exp $ (LBL)";
#endif

#include <stdlib.h>
#include <math.h>
#include "message.h"
#include "trace.h"
#include "agent.h"

/* ivs data packet; ctrl packets are sent back as "messages" */
struct hdr_ivs {
	double ts_;			/* timestamp sent at source */
	u_int8_t S_;
	u_int8_t R_;
	u_int8_t state_;
	u_int8_t rshft_;
	u_int8_t kshft_;
	u_int16_t key_;
	double maxrtt_;
	int seqno_;

	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_ivs* access(Packet* p) {
		return (hdr_ivs*) p->access(offset_);
	}

	/* per-field member functions */
	double& ts() { return (ts_); }
	u_int8_t& S() { return (S_); }
	u_int8_t& R() { return (R_); }
	u_int8_t& state() { return (state_); }
	u_int8_t& rshft() { return (rshft_); }
	u_int8_t& kshft() { return (kshft_); }
	u_int16_t& key() { return (key_); }
	double& maxrtt() { return (maxrtt_); }
	int& seqno() { return (seqno_); }
};

int hdr_ivs::offset_;

static class IvsHeaderClass : public PacketHeaderClass {
public:
	IvsHeaderClass() : PacketHeaderClass("PacketHeader/IVS",
					     sizeof(hdr_ivs)) {
		bind_offset(&hdr_ivs::offset_);
	}
} class_ivshdr;

class IvsSource : public Agent {
public:
	IvsSource();
protected:
	void reset();
	void recv(Packet *pkt, Handler*);
	void sendpkt();

	int S_;
	int R_;
	int state_;
#define ST_U 0
#define ST_L 1
#define ST_C 2
	int rttShift_;
	int keyShift_;
	int key_;
	double maxrtt_;
};

struct Mc_Hole {
	int start;
	int end;
	double time;
	Mc_Hole* next;
};

class IvsReceiver : public Agent {
public:
	IvsReceiver();
	int command(int argc, const char*const* argv);
protected:
	void recv(Packet *pkt, Handler*);
	void update_ipg(double now);
	int lossMeter(double timeDiff, u_int32_t seq, double maxrtt);
	void upcall_respond(double ts, int matchS);
	void upcall_rtt_solicit(double ts, int rshift);

	int state_;
	u_int32_t nextSeq_;

	double timeMean_;
	double timeVar_;
	double ipg_;		/* interpkt gap (estimator) */
	Mc_Hole* head_;
	Mc_Hole* tail_;
	double lastPktTime_;
	int ignoreR_;
	double lastTime_;	/* last time a resp pkt sent */
	int key_;
};

static class IvsSourceClass : public TclClass {
public:
	IvsSourceClass() : TclClass("Agent/IVS/Source") {}
	TclObject* create(int, const char*const*) {
		return (new IvsSource());
	}
} class_ivs_source;

static class IvsReceiverClass : public TclClass {
public:
	IvsReceiverClass() : TclClass("Agent/IVS/Receiver") {}
	TclObject* create(int, const char*const*) {
		return (new IvsReceiver());
	}
} class_ivs_receiver;

IvsSource::IvsSource() : Agent(PT_MESSAGE), S_(0), R_(0), state_(ST_U),
	rttShift_(0), keyShift_(0), key_(0), maxrtt_(0)
{
	bind("S_", &S_);
	bind("R_", &R_);
	bind("state_", &state_);
	bind("rttShift_", &rttShift_);
	bind("keyShift_", &keyShift_);
	bind("key_", &key_);
	bind("maxrtt_", &maxrtt_);
}

void IvsSource::reset()
{
}

/*
 * main reception path - should only see acks, otherwise the
 * network connections are misconfigured
 */
void IvsSource::recv(Packet* pkt, Handler*)
{
	char wrk[128];/*XXX*/
	Tcl& tcl = Tcl::instance();
	hdr_msg *q = hdr_msg::access(pkt);
	sprintf(wrk, "%s handle {%s}", name(), q->msg());
	tcl.eval(wrk);
	Packet::free(pkt);
}

#ifdef notdef
void IvsSource::probe_timeout()
{
	rndStart_ = now;

	if (keyShift_ == 15) {
		if (key_ == 0) {
			if (solicitedResponses_ == 0)
				estReceivers_ = 0;
				/*
				 * Got through a round without being LOADED.
				 * increase send rate.
				 */
			if (state_ == ST_U)
				increase();

				/* Reset keys et al */
			S_ = 1;
			state_ = ST_U;

				/*XXX*/
			setRttSolicit(mcstate);

			solicitedResponses_ = 0;
			keyShift_ = startShift_;
				/*XXX do all this in tcl? */
			setkey();
			
		} else { mcstate->hdr.key = 0; }
	} else {
		if (probeTimeout_ > 0)
			++keyShift_;
	}
	sched(pktTime + 2 * maxrtt_, IVS_TIMER_PROBE);
}
#endif

void IvsSource::sendpkt()
{
	Packet* pkt = allocpkt();
	hdr_ivs *p = hdr_ivs::access(pkt);
	/*fill in ivs fields */
	p->ts() = Scheduler::instance().clock();
	p->S() = S_;
	p->R() = R_;
	p->state() = state_;
	p->rshft() = rttShift_;
	p->kshft() = keyShift_;
	p->key() = key_;
	p->maxrtt() = maxrtt_;

	target_->recv(pkt, (Handler *)0);
}

IvsReceiver::IvsReceiver() : Agent(PT_MESSAGE), state_(ST_U),
	nextSeq_(0),
	timeMean_(0.), timeVar_(0.),/*XXX*/
	ipg_(0.),
	head_(0),
	tail_(0),
	lastPktTime_(0.),
	ignoreR_(0),
	lastTime_(0.),
	key_(0)
{
	bind("ignoreR_", &ignoreR_);
	bind("key_", &key_);
	bind("state_", &state_);
	bind("packetSize_", &size_);
}

inline void IvsReceiver::update_ipg(double v)
{
	/* Update the estimated interpacket gap */
	ipg_ = (15 * ipg_ + v) / 16;
}

/*
 * timestamp comes in milliseconds since start of connection according to
 * remote clock
 * now is milliseconds since start of connection
 * rtt in milliseconds
 * This congestion meter is not terribly good at figuring out when the net is 
 * loaded, since the loss of a packet over a rtt is a transitory event
 * Eventually we ought to have a memory thing, that records state once a 
 * maxrtt, with thresholds to decide current state
 */
int IvsReceiver::lossMeter(double timeDiff, u_int32_t seq, double maxrtt)
{
	/*
	 * The congestion signal is calculated here by measuring the loss in a 
	 * given period of packets - if the threshold for lost packets is
	 * passed then signal Congested.  If there are no lost packets,
	 * then we are at UNLOADED, else LOADED
	 */
	/* if sequence number is next, increase expected number */
	
	double now = Scheduler::instance().clock();
	if (nextSeq_ == 0)
		nextSeq_ = seq + 1;
	else if (seq == nextSeq_)
		nextSeq_++;
	else if (seq > nextSeq_) {
#ifdef notdef
		if (trace_ != 0) {
			sprintf(trace_->buffer(), "d %g %d",
				lastPktTime_, seq - nextSeq_);
			trace_->dump();
		}
#endif

		/* This is definitely a hole */
		Mc_Hole* hole = new Mc_Hole;
		hole->time = now;
		hole->start = nextSeq_;
		hole->end = seq - 1;
		hole->next = 0;
		/* Now add it to the list */
		if (head_ == NULL) {
			head_ = hole;
			tail_ = hole;
		} else {
			tail_->next = hole;
			tail_ = hole;
		}
		nextSeq_ = seq + 1;
	} else {
		/* XXX can't happen in current ns simulations */
		fprintf(stderr, "ns: ivs rcvr: seq number went backward\n");
		abort();
	}

	/* update the calculation of the variance in the rtt */
	/* get the time averaged mean of the difference */
	if (timeMean_ == 0)
		timeMean_ = timeDiff;
	else
		timeMean_ = (7 * timeMean_ + timeDiff) / 8;

	timeDiff -= timeMean_;
	if (timeDiff < 0)
		timeDiff = -timeDiff;

	timeVar_ = (7 * timeVar_ + timeDiff) / 8;

	int lostPkts = 0;
	/* 
	 * Check down the list of holes, discarding those that before
	 * now-rttvar-rtt, counting those that fall within
	 * now-rttvar to now-rttvar-rtt
	 */
	if (head_ == 0)
		return (ST_U);

	Mc_Hole *cur = head_, *prev = NULL;

	double validEnd = now - 2 * timeVar_;
	double validStart = validEnd - maxrtt;

	/* for each hole, if it is older than required, dump it */
	/* If it is valid, add the size to the loss count */
	/* Go to the next hole */

	while (cur != NULL) {
		if (cur->time < validStart) {
			if (prev == NULL)
				head_ = cur->next;
			else
				prev->next = cur->next;
			delete cur;
			if (prev == NULL)
				cur = head_;
			else
				cur = prev->next;
		} else {
			if (cur->time < validEnd)
				lostPkts += cur->end - cur->start + 1;
			prev = cur;
			cur = cur->next;
		}
	}

	/*
	 * Update the moving average calculation of the number of holes, if
	 * nowMs is another rtt away
	 */

	double pps = (ipg_ != 0) ? maxrtt / ipg_ : 0.;

/*XXX*/
#ifdef notdef
	if (trace_ != 0) {
		double now = Scheduler::instance().clock();
		sprintf(trace_->buffer(), "%.17g %g", now,
			(double)lostPkts / pps);
		trace_->dump();
	}
#endif

/*XXX*/
#define LOSSCONGTH 15
#define LOSSLOADTH 5
	/* If the rtt is smaller than the ipg, set the thresholds to 0,1,2 */
	if ((pps * LOSSCONGTH) / 100 < 2)
		pps = 200 / LOSSCONGTH;

	if (lostPkts <= (LOSSLOADTH * pps) / 100)
		return (ST_U);
	else if (lostPkts <= (LOSSCONGTH * pps) / 100)
		return (ST_L);
	else
		return (ST_C);
}

void IvsReceiver::recv(Packet* pkt, Handler*)
{
	hdr_ivs *p = hdr_ivs::access(pkt);
	double now = Scheduler::instance().clock();

	if (lastPktTime_ == 0.) {
		lastPktTime_ = now;
		Packet::free(pkt);
		return;
	}
	update_ipg(now - lastPktTime_);
	double ts = p->ts();
	int prevState = state_;
	state_ = lossMeter(now - ts, p->seqno(), p->maxrtt());

	lastPktTime_ = now;

	/* If soliciting rtt */
	if (p->R() && !ignoreR_)
		/* upcall into tcl */
		upcall_rtt_solicit(ts, p->rshft());

	/*
	 * send a response if we're congested and its over an rtt since
	 * we last sent one OR
	 * any response is solicited to estimate size and we match the key OR
	 * we're LOADED and we match the key and its over an rtt since we last
	 * sent a response
	 */

	if (now - lastTime_ < p->maxrtt() && state_ <= prevState) {
		Packet::free(pkt);
		return;
	}

	int shift = p->kshft();
	int match;
	if (p->key() == 0)
		match = 1;
	else
		match = (key_ >> shift) == (p->key() >> shift);

	int matchS = match ? p->S() : 0;

	if (state_ == ST_C || matchS || (match && state_ == ST_L)) {
		upcall_respond(ts, matchS);
		lastTime_ = now;
	}

	Packet::free(pkt);
}

void IvsReceiver::upcall_respond(double ts, int matchS)
{
	Tcl::instance().evalf("%s respond %.17g %d", name(), ts, matchS);
}

void IvsReceiver::upcall_rtt_solicit(double ts, int rshift)
{
	Tcl::instance().evalf("%s solicit-rtt %.17g %d", name(), ts, rshift);
}

int IvsReceiver::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "send") == 0) {
			Packet* pkt = allocpkt();
			hdr_msg* p = hdr_msg::access(pkt);
			const char* s = argv[2];
			int n = strlen(s);
			if (n >= p->maxmsg()) {
				tcl.result("message too big");
				Packet::free(pkt);
				return (TCL_ERROR);
			}
			strcpy(p->msg(), s);
			target_->recv(pkt, (Handler*)0);
			return (TCL_OK);
		}
	}
	return (Agent::command(argc, argv));
}
