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
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/tcp/snoop.h,v 1.14 2002/05/06 22:23:16 difa Exp $ (UCB)
 */

#ifndef ns_snoop_h
#define ns_snoop_h

#include "scheduler.h"
#include "packet.h"
#include "ip.h"
#include "tcp.h"
#include "ll.h"
#include "mac.h"
#include "flags.h"
#include "template.h"

/* Snoop states */
#define SNOOP_ACTIVE    0x01	/* connection active */
#define SNOOP_CLOSED    0x02	/* connection closed */
#define SNOOP_NOACK     0x04	/* no ack seen yet */
#define SNOOP_FULL      0x08	/* snoop cache full */
#define SNOOP_RTTFLAG   0x10	/* can compute RTT if this is set */
#define SNOOP_ALIVE     0x20	/* connection has been alive past 1 sec */
#define SNOOP_WLALIVE   0x40	/* wl connection has been alive past 1 sec */
#define SNOOP_WLEMPTY   0x80

#define SNOOP_MAXWIND   100	/* XXX */
#define SNOOP_WLSEQS    8
#define SNOOP_MIN_TIMO  0.100	/* in seconds */
#define SNOOP_MAX_RXMIT 10	/* quite arbitrary at this point */
#define SNOOP_PROPAGATE 1
#define SNOOP_SUPPRESS  2

#define SNOOP_MAKEHANDLER 1
#define SNOOP_TAIL 1

struct hdr_seq {
	int seq;
	int num;
};

struct hdr_snoop {
	int seqno_;
	int numRxmit_;
	int senderRxmit_;
	double sndTime_;

	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_snoop* access(const Packet* p) {
		return (hdr_snoop*) p->access(offset_);
	}

	inline int& seqno() { return seqno_; }
	inline int& numRxmit() { return numRxmit_; }
	inline int& senderRxmit() { return senderRxmit_; }
	inline double& sndTime() { return sndTime_; }
};

class LLSnoop : public LL {
  public:
	LLSnoop() : LL() { bind("integrate_", &integrate_);}
	void recv(Packet *, Handler *);
	void snoop_rtt(double);
	inline double timeout() { 
		return max(srtt_+4*rttvar_, snoopTick_);
	}
	inline int integrate() { return integrate_; }
  protected:
	int    integrate_;
	double srtt_;
	double rttvar_;
	double g_;
	double snoopTick_;	/* minimum rxmission timer granularity */
};


class SnoopRxmitHandler;
class SnoopPersistHandler;

class Snoop : public NsObject {
	friend class SnoopRxmitHandler;
	friend class SnoopPersistHandler;
  public:
	Snoop();
	void recv(Packet *, Handler *);
	void handle(Event *);
	int snoop_rxmit(Packet *);
	inline int next(int i) { return (i+1) % maxbufs_; }
	inline int prev(int i) { return ((i == 0) ? maxbufs_-1 : i-1); };
	inline int wl_next(int i) { return (i+1) % SNOOP_WLSEQS; }
	inline int wl_prev(int i) { return ((i == 0) ? SNOOP_WLSEQS-1 : i-1);};

  protected:
	int command(int argc, const char*const* argv);
	void reset();
	void wlreset();
	void snoop_data(Packet *);
	int  snoop_ack(Packet *);
	void snoop_wless_data(Packet *);
	void snoop_wired_ack(Packet *);
	int  snoop_wlessloss(int);
	double snoop_cleanbufs_(int);
	void snoop_rtt(double);
	int snoop_qlong();
	int snoop_insert(Packet *);
	inline int empty_()
		{return (bufhead_==buftail_ &&!(fstate_&SNOOP_FULL));}
	void savepkt_(Packet *, int, int);
	void update_state_();
	inline double timeout() { 
		if (!parent_->integrate())
			return max(srtt_+4*rttvar_, snoopTick_);
		else
			return parent_->timeout();
	}
	void snoop_cleanup();
	
	LLSnoop *parent_;	/* the parent link layer object */
	NsObject* recvtarget_;	/* where packet is passed up the stack */
	Handler  *callback_;
	SnoopRxmitHandler *rxmitHandler_; /* used in rexmissions */
	SnoopPersistHandler *persistHandler_; /* for connection (in)activity */
	int      snoopDisable_;	/* disable snoop for this mobile */
	u_short  fstate_;	/* state of connection */
	int      lastSeen_;	/* first byte of last packet buffered */
        int      lastAck_;	/* last byte recd. by mh for sure */
	int      expNextAck_;	/* expected next ack after dup sequence */
	short    expDupacks_;	/* expected number of dup acks */
	double   srtt_;		/* smoothed rtt estimate */
	double   rttvar_;	/* linear deviation */
	double   tailTime_;	/* time at which earliest unack'd pkt sent */
	int      rxmitStatus_;
	short    bufhead_;	/* next pkt goes here */
	Event    *toutPending_;	/* # pending timeouts */
	short    buftail_;	/* first unack'd pkt */
	Packet   *pkts_[SNOOP_MAXWIND]; /* ringbuf of cached pkts */
	
	int      wl_state_;
	int      wl_lastSeen_;
	int      wl_lastAck_;
	int      wl_bufhead_;
	int      wl_buftail_;
	hdr_seq  *wlseqs_[SNOOP_WLSEQS];	/* ringbuf of wless data */

	int      maxbufs_;	/* max number of pkt bufs */
	double   snoopTick_;	/* minimum rxmission timer granularity */
	double   g_;		/* gain in EWMA for srtt_ and rttvar_ */
	int      integrate_;	/* integrate loss rec across active conns */
	int      lru_;		/* an lru cache? */
};

class SnoopRxmitHandler : public Handler {
  public:
	SnoopRxmitHandler(Snoop *s) : snoop_(s) {}
	void handle(Event *event);
  protected:
	Snoop *snoop_;
};

class SnoopPersistHandler : public Handler {
  public:
	SnoopPersistHandler(Snoop *s) : snoop_(s) {}
	//void handle(Event *);
  protected:
	Snoop *snoop_;
};

#endif
