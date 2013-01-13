/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1994-1997 Regents of the University of California.
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
static char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/mcast/rlm.cc,v 1.8 2001/12/20 19:27:44 haldar Exp $ (LBL)";
#endif

#include "agent.h"
#include "node.h"

struct srcpkt {
	double off;
	int level;
	int size;
	srcpkt* next;
};


class RLM_Sender : public Agent {
 public:
	RLM_Sender();
	~RLM_Sender();
	int command(int argc, const char*const* argv);
 protected:
	virtual void timeout(int);
	void start();
	void stop();
	void sched_next();
	void sendpkt(int size, int level);
	void addpkt(double off, int size, int level);
	int running_;
	double interval_;
	srcpkt* pkt_;
	srcpkt* allpkt_;
	/*XXX make size dynamic*/
#define MAXLEVEL 32
	int seqno_[MAXLEVEL];
	int blkno_;
};

static class RLMMatcher : public Matcher {
public:
	RLMMatcher() : Matcher("agent") {}
	TclObject* match(const char* id) {
		if (strcmp(id, "rlm-sender") == 0)
			return (new RLM_Sender());
		return (0);
	}
} matcher_rlm;

RLM_Sender::RLM_Sender() : Agent(PT_CBR), pkt_(0), allpkt_(0)
{
	Tcl& tcl = Tcl::instance();
	link_time("ns_rlm", "interval", &interval_, 0);
	link_int("ns_rlm", "packet-size", &size_, 0);
	running_ = 0;
	memset(seqno_, 0, sizeof(seqno_));
	blkno_ = 0;
}

RLM_Sender::~RLM_Sender()
{
	/*XXX free allpkt_ */
}

void RLM_Sender::start()
{
	if (allpkt_ != 0) {
		running_ = 1;
		pkt_ = allpkt_;
		sched(pkt_->off, 0);
	}
}

void RLM_Sender::stop()
{
	running_ = 0;
}

void RLM_Sender::sched_next()
{
	srcpkt* p = pkt_;
	srcpkt* n = p->next;
	double t;
	if (n == 0) {
		t = interval_;
		n = allpkt_;
		++blkno_;
		blkitem_ = 0;
	} else
		t = 0.;
	t += n->off - p->off;
	pkt_ = n;
	sched(t, 0);
}

void RLM_Sender::timeout(int)
{
	if (running_) {
		sendpkt(pkt_->size, pkt_->level);
		sched_next();
	}
}

void RLM_Sender::sendpkt(int size, int level)
{
	Packet* p = allocpkt(seqno_[level]++);
	IPHeader *iph = IPHeader::access(p->bits());
	RLMHeader *rh = RLMHeader::access(p->bits());
	rh->blkno() = blkno_;
	rh->blkitem() = blkitem_++;
	iph->size() = size;
	iph->daddr() += level;
#ifdef notdef
	iph->class() += level;/*XXX*/
#endif
	node_->transmit(p);
}

extern double time_atof(const char*);

void RLM_Sender::addpkt(double off, int size, int level)
{
	srcpkt* sp = new srcpkt;
	sp->next = 0;
	sp->off = off;
	sp->size = size;
	sp->level = level;
	srcpkt** p;
	for (p = &allpkt_; *p != 0; p = &(*p)->next)
		if (sp->off < (*p)->off)
			break;
	*p = sp;
}

/*
 * $obj start
 * $obj stop
 * $obj packet $off $size $level
 */
int RLM_Sender::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "start") == 0) {
			start();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "stop") == 0) {
			stop();
			return (TCL_OK);
		}
	}
	if (argc == 5) {
		if (strcmp(argv[1], "packet") == 0) {
			double off = time_atof(argv[2]);
			int size = atoi(argv[3]);
			int level = atoi(argv[4]);
			addpkt(off, size, level);
			return (TCL_OK);
		}
	}
	return (Agent::command(argc, argv));
}

class RLM_Receiver : public Agent {
 public:
	RLM_Receiver();
	int command(int argc, const char*const* argv);
	void recv(Packet* pkt);
protected:
	char* proc_;
	int loss_;
	int expected_;
	int bytes_;

	int pblk_;
	int pseq_[MAXLEVEL];
};

static class RLM_ReceiverMatcher : public Matcher {
public:
	RLM_ReceiverMatcher() : Matcher("agent") {}
	TclObject* match(const char* id) {
		if (strcmp(id, "rlm-receiver") == 0)
			return (new RLM_Receiver());
		return (0);
	}
} matcher_cbr;

RLM_Receiver::RLM_Receiver() : Agent(-1), proc_(0), expected_(-1)
{
	bytes_ = 0;
	loss_ = 0;
	link_int(0, "loss", &loss_, 1);
	link_int(0, "bytes", &bytes_, 1);
}

void RLM_Receiver::recv(Packet* pkt)
{
	IPHeader *iph = IPHeader::access(pkt->bits());
	SequenceHeader *sh = SequenceHeader::access(pkt->bits());
	bytes_ += iph->size();
	int seqno = sh->seqno();
	int blkno = seqno >> 16;
	seqno &= 0xffff;
	
	/*
	 * Check for lost packets
	 */
	if (expected_ >= 0) {
		int loss = sh->seqno() - expected_;
		if (loss > 0) {
			loss_ += loss;
			if (proc_ != 0)
				Tcl::instance().eval(proc_);
		}
	}
	expected_ = sh->seqno() + 1;
	Packet::free(pkt);
}

/*
 * $proc interval $interval
 * $proc size $size
 */
int RLM_Receiver::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "clear") == 0) {
			expected_ = -1;
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "proc") == 0) {
			const char* proc = argv[2];
			int n = strlen(proc) + 1;
			delete proc_;
			proc_ = new char[n];
			strcpy(proc_, proc);
			return (TCL_OK);
		}
	}
	return (Agent::command(argc, argv));
}
