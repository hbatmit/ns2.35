/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1996-1997 The Regents of the University of California.
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
 * 	This product includes software developed by the Daedalus 
 *      Research Group at the University of California, Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be used
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
 * ack-recons.cc: contributed by the Daedalus Research Group, 
 * UC Berkeley (http://daedalus.cs.berkeley.edu).
 *
 * $Header: /cvsroot/nsnam/ns-2/tcp/ack-recons.h,v 1.4 2000/09/01 03:04:05 haoboy Exp $
 */

/*
 * TCP Ack reconstructor.  This object sits on the other end of a constrained 
 * link and intersperses TCP acks to the source (without violating the e2e 
 * semantics of TCP acks).  This allows us to get good performance for TCP 
 * over various asymmetric networks, in conjunction with techniques to reduce 
 * the frequency of acks (such as ack filtering) with making any changes to 
 * the TCP source (e.g., like those implemented in tcp-asym.cc).  
 */

#ifndef ns_ack_recons_h
#define ns_ack_recons_h

#include "semantic-packetqueue.h"

class AckReconsController : public TclObject {
public:
	AckReconsController() : spq_(0) {}
	void recv(Packet *p, Handler *h=0);
	SemanticPacketQueue *spq_;
};
	
class AckRecons : public Agent {
public:
	AckRecons(nsaddr_t src, nsaddr_t dst) :
		Agent(PT_TCP), spq_(0), src_(src), dst_(dst),
		ackTemplate_(0), ackPending_(0), lastAck_(0), 
		lastRealAck_(0) {
			bind("lastTime_", &lastTime_);
			bind("lastAck_", &lastAck_);
			bind("lastRealTime_", &lastRealTime_);
			bind("lastRealAck_", &lastRealAck_);
			bind("ackInterArr_", &ackInterArr_);
			bind("ackSpacing_", &ackSpacing_);
			bind("deltaAckThresh_", &deltaAckThresh_);
			bind("delack_", &delack_);
			bind("adaptive_", &adaptive_);
			bind("alpha_", &alpha_);
			bind("size_", &size_);
		}
	int command(int argc, const char*const* argv);
	void handle(Event *e);
	void recv(Packet *p);
	SemanticPacketQueue *spq_; /* the corresponding queue of packets */
private:
	void sendack(int ack, double t); /* send ack pkt at time t */
	nsaddr_t src_;		/* src addr:port */
	nsaddr_t dst_;		/* dst addr:port */
	Packet	*ackTemplate_;	/* used as a template for generated acks */
	int	ackPending_;
	int	lastAck_;	/* last ack sent by recons, maybe generated */
	int	lastRealAck_;	/* last ack actually received on link */
	double	lastTime_;	/* time when last ack was sent */
	double	lastRealTime_;	/* time when last ack arrived on link */
	int	dupacks_;	/* number of dup acks seen so far */
	int	size_;		/* TCP packet size -- needed because TCP
				 * acks are in terms of packets */
	/* knobs/policies */
	int	deltaAckThresh_; /* threshold for reconstruction to kick in */
	double	ackInterArr_;	/* estimate of arrival rate of acks */
	double	ackSpacing_;	/* time duration between acks sent by recons */
	int	delack_;	/* generate ack at least every delack_ acks */
	int	adaptive_;	/* whether to adapt ack bandwidth? */
	double	alpha_;	/* used in linear filter for ack rate est. */
};

#endif
