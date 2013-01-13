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
 *	This product includes software developed by the Daedalus Research
 *	Group at the University of California Berkeley.
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
 * Contributed by the Daedalus Research Group, U.C.Berkeley
 * http://daedalus.cs.berkeley.edu
 *
 * @(#) $Header: 
 */

/*
 * tcp-asym includes modifications to several flavors of TCP to enhance
 * performance over asymmetric networks, where the ack channel is
 * constrained.  Types of asymmetry we have studied and used these mods
 * include bandwidth asymmetry and latency asymmetry (where  variable 
 * latencies cause problems to TCP, e.g., in packet radio networks.
 * The receiver-side code in this file is derived from the regular
 * TCP sink code. The main additional functionality is that the sink responds
 * to ECN by performing ack congestion control, i.e. it multiplicatively backs
 * off the frequency with which it sends acks (up to a limit). For each 
 * subsequent round-trip period during which it does not receive an ECN, 
 * it gradually increases the frequency of acks (up to a maximum of 1 
 * per data packet).
 *
 * For questions/comments, please contact:
 *   Venkata N. Padmanabhan (padmanab@cs.berkeley.edu)
 *   http://www.cs.berkeley.edu/~padmanab
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/tcp/tcp-asym-sink.cc,v 1.17 2003/08/14 04:26:42 sfloyd Exp $ (UCB)";
#endif

#include "template.h"
#include "flags.h"
#include "tcp-sink.h"
#include "tcp-asym.h"


class TcpAsymSink : public DelAckSink {
public:
	TcpAsymSink(Acker*);
	virtual void recv(Packet* pkt, Handler* h);
	virtual void timeout(int tno);
protected:
	virtual void add_to_ack(Packet* pkt);
	int delackcount_;	/* the number of consecutive packets that have
				   not been acked yet */
	int maxdelack_;		/* the maximum extent to which acks can be
				   delayed */
	int delackfactor_;	/* the dynamically varying limit on the extent
				   to which acks can be delayed */
	int delacklim_;		/* limit on the extent of del ack based on the
				   sender's window */
	double ts_ecn_;		/* the time when an ECN was received last */
	double ts_decrease_;    /* the time when delackfactor_ was decreased last */
	double highest_ts_echo_;/* the highest timestamp echoed by the peer */
};	

static class TcpAsymSinkClass : public TclClass {
public:
	TcpAsymSinkClass() : TclClass("Agent/TCPSink/Asym") {}
	TclObject* create(int, const char*const*) {
		return (new TcpAsymSink(new Acker));
	}
} class_tcpasymsink;

TcpAsymSink::TcpAsymSink(Acker* acker) : DelAckSink(acker), delackcount_(0), 
    delackfactor_(1), delacklim_(0), ts_ecn_(0), ts_decrease_(0)
{
	bind("maxdelack_", &maxdelack_);
}

/* Add fields to the ack. Not needed? */
void TcpAsymSink::add_to_ack(Packet* pkt) 
{
	hdr_tcpasym *tha = hdr_tcpasym::access(pkt);
	tha->ackcount() = delackcount_;
}

void TcpAsymSink::recv(Packet* pkt, Handler*) 
{
	int olddelackfactor = delackfactor_;
	int olddelacklim = delacklim_; 
	int max_sender_can_send = 0;
	hdr_flags *fh = hdr_flags::access(pkt);
	hdr_tcp *th = hdr_tcp::access(pkt);
	hdr_tcpasym *tha = hdr_tcpasym::access(pkt);
	double now = Scheduler::instance().clock();
	int numBytes = hdr_cmn::access(pkt)->size();


	acker_->update_ts(th->seqno(),th->ts(),ts_echo_rfc1323_);
	acker_->update(th->seqno(), numBytes);

#if 0  // johnh
	int numToDeliver;
	/* XXX if the #if 0 is removed, delete the call to acker_->update() above */
	numToDeliver = acker_->update(th->seqno(), numBytes);
	if (numToDeliver)
		recvBytes(numToDeliver);
#endif /* 0 */

	/* determine the highest timestamp the sender has echoed */
	highest_ts_echo_ = max(highest_ts_echo_, th->ts_echo());
	/* 
	 * if we receive an ECN and haven't received one in the past
	 * round-trip, double delackfactor_ (and consequently halve
	 * the frequency of acks) subject to a maximum
	 */
	if (fh->ecnecho() && highest_ts_echo_ >= ts_ecn_) {
		delackfactor_ = min(2*delackfactor_, maxdelack_);
		ts_ecn_ = now;
	}
	/*
	 * else if we haven't received an ECN in the past round trip and
	 * haven't (linearly) decreased delackfactor_ in the past round
	 * trip, we decrease delackfactor_ by 1 (and consequently increase
	 * the frequency of acks) subject to a minimum
	 */
	else if (highest_ts_echo_ >= ts_ecn_ && highest_ts_echo_ >= ts_decrease_) {
		delackfactor_ = max(delackfactor_ - 1, 1);
		ts_decrease_ = now;
	}

	/*
         * if this is the next packet in sequence, we can consider delaying the ack. 
	 * Set delacklim_ based on how much data the sender can send if we don't
	 * send back any more acks. The idea is to avoid stalling the sender because
	 * of a lack of acks.
         */
        if (th->seqno() == acker_->Seqno()) {
		max_sender_can_send = (int) min(tha->win()+acker_->Seqno()-tha->highest_ack(), tha->max_left_to_send());
		/* XXXX we use a safety factor 2 */
		delacklim_ = min(maxdelack_, max_sender_can_send/2); 
	}
	else
		delacklim_ = 0;

	if (delackfactor_ < delacklim_) 
		delacklim_ = delackfactor_;

	/* 
	 * Log values of variables of interest. Since this is the only place
	 * where this is done, we decided against using a more general method
	 * as used for logging TCP sender state variables.
	 */
	if (channel_ && (olddelackfactor != delackfactor_ || olddelacklim != delacklim_)) {
		char wrk[500];
		int n;

		/* we print src and dst in reverse order to conform to sender side */
		sprintf(wrk, "time: %-6.3f saddr: %-2d sport: %-2d daddr:"
			" %-2d dport: %-2d dafactor: %2d dalim: %2d max_scs:"
			" %4d win: %4d\n", now, addr(), port(),
			daddr(), dport(), delackfactor_,
			delacklim_,max_sender_can_send, tha->win());  
		n = strlen(wrk);
		wrk[n] = '\n';
		wrk[n+1] = 0;
		(void)Tcl_Write(channel_, wrk, n+1);
		wrk[n] = 0;
	}
		
	delackcount_++;
	/* check if we have waited long enough that we should send an ack */
	if (delackcount_ < delacklim_) { /* it is not yet time to send an ack */
		/* if the delayed ack timer is not set, set it now */
		if (!(delay_timer_.status() == TIMER_PENDING)) {
			save_ = pkt;
			delay_timer_.resched(interval_);
		}
		else {
			hdr_tcp *sth = hdr_tcp::access(save_);
			/* save the pkt with the more recent timestamp */
			if (th->ts() > sth->ts()) {
				Packet::free(save_);
				save_ = pkt;
			}
		}
		return;
	}
	else { /* send back an ack now */
		if (delay_timer_.status() == TIMER_PENDING) {
			delay_timer_.cancel();
			Packet::free(save_);
			save_ = 0;
		}
		hdr_flags* hf = hdr_flags::access(pkt);
		hf->ect() = 1;
		ack(pkt);
		delackcount_ = 0;
		Packet::free(pkt);
	}
}


void TcpAsymSink::timeout(int /*tno*/)
{
	/*
	 * The timer expired so we ACK the last packet seen.
	 */
	Packet* pkt = save_;
	delackcount_ = 0;
	ack(pkt);
	save_ = 0;
	Packet::free(pkt);
}

