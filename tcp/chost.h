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
 * 	This product includes software developed by the Daedalus Research
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

#ifndef CHOST
#define CHOST
#include "agent.h"
#include "packet.h"
#include "tcp.h"
#include "tcp-fs.h"
#include "tcp-int.h"
#include "nilist.h"

#define MAX_PARALLEL_CONN 1000

class Segment : public slink {
	friend class CorresHost;
	friend class TcpSessionAgent;
  public:
	Segment() {ts_ = 0;
	seqno_ = later_acks_ = dupacks_ = dport_ = sport_ = size_ = rxmitted_ = 
		daddr_ = 0; thresh_dupacks_ = 0; partialack_ = 0;};
  protected:
	int seqno_;
	int sessionSeqno_;
	int daddr_;
	int dport_;
	int sport_;
	int size_;
	double ts_;		/* timestamp */
	int dupacks_;     /* on same connection */
	int later_acks_;  /* on other connections */
	int thresh_dupacks_; /* whether there have been a threshold # of
				dupacks/later acks for this segment */
	int partialack_;  /* whether a partial ack points to this segment */
	short rxmitted_;
	class IntTcpAgent *sender_;
};

class CorresHost : public slink, public TcpFsAgent {
	friend class IntTcpAgent;
  public:
	CorresHost();
	/* add pkt to pipe */
	virtual Segment* add_pkts(int size, int seqno, int sessionSeqno, int daddr, 
		      int dport, int sport, double ts, IntTcpAgent *sender); 
	/* remove pkt from pipe */
	int clean_segs(int size, Packet *pkt, IntTcpAgent *sender, int sessionSeqno,
		       int amt_data_acked);
	int rmv_old_segs(Packet *pkt, IntTcpAgent *sender, int amt_data_acked);

	void opencwnd(int size, IntTcpAgent *sender=0);
	void closecwnd(int how, double ts, IntTcpAgent *sender=0);
	void closecwnd(int how, IntTcpAgent *sender=0);
	void adjust_ownd(int size);
	int ok_to_snd(int size);
	virtual void add_agent(IntTcpAgent *agent, int size, double winMult,
		       int winInc, int ssthresh);
	void del_agent(IntTcpAgent *) {nActive_--;};
	void agent_tout(IntTcpAgent *) {nTimeout_++;};
	void agent_ftout(IntTcpAgent *) {nTimeout_--;};
	void agent_rcov(IntTcpAgent *) {nFastRec_++;};
	void agent_frcov(IntTcpAgent *) {nFastRec_--;};
	void quench(int how);

  protected:
	Islist<IntTcpAgent> conns_; /* active connections */
	Islist_iter<IntTcpAgent> *connIter_;
	u_int nActive_;	     /* number of active tcp conns to this host */
	u_int nTimeout_;     /* number of tcp conns to this host in timeout */
	u_int nFastRec_;     /* number of tcp conns to this host in recovery */
	double closecwTS_; 
	double winMult_;
	int winInc_;
	TracedDouble ownd_;  /* outstanding data to host */
	TracedDouble owndCorrection_; /* correction factor to account for dupacks */   
	int proxyopt_;	     /* indicates whether the connections are on behalf
			        of distinct users (like those from a proxy) */
	int fixedIw_;        /* fixed initial window (not a function of # conn) */
	Islist<Segment> seglist_;	/* list of unack'd segments to peer */
	double lastackTS_;
	/*
	 * State encompassing the round-trip-time estimate.
	 * srtt and rttvar are stored as fixed point;
	 * srtt has 3 bits to the right of the binary point, rttvar has 2.
	 */
	double wndInit_;    /* should = path_mtu_ */
	Segment *rtt_seg_;  /* segment being timed for RTT computation */
	int dontAdjustOwnd_;/* don't adjust ownd in response to dupacks */
	int dontIncrCwnd_;  /* set when pkt loss is suspected (e.g., there is
			       reordering) */
	int rexmtSegCount_; /* number of segments that we "suspect" need to be
			       retransmitted */
	int disableIntLossRecov_; /* disable integrated loss recovery */

	/* possible candidates for rxmission */
	Segment *curArray_[MAX_PARALLEL_CONN]; 
	Segment *prevArray_[MAX_PARALLEL_CONN]; /* prev segs */
	/* variables for fast start */
	class IntTcpAgent *connWithPktBeforeFS_;
	
	/* following is for right-edge timer recovery */
/*	int pending_;*/
	Event timer_;
	inline void cancel() {	
		(void)Scheduler::instance().cancel(&timer_);
		// No need to free event since it's statically allocated.
/*		pending_ = 0;*/
	}
};

#endif

