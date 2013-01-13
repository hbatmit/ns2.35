/*
 * Copyright (c) 1999  International Computer Science Institute
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
 *      This product includes software developed by ACIRI, the AT&T
 *      Center for Internet Research at ICSI (the International Computer
 *      Science Institute).
 * 4. Neither the name of ACIRI nor of ICSI may be used
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
 */

#ifndef ns_tcp_rfc793edu_h
#define ns_tcp_rfc793edu_h

#include "tcp.h"

// Original code contributed by Fernando Cela Diaz,
// <fcela@ce.chalmers.se>.
// For more information, see the following:
// URL "http://www.ce.chalmers.se/~fcela/tcp-tour.html"

/* RFC793edu TCP -- 19990821, fcela@acm.org */
class RFC793eduTcpAgent : public virtual TcpAgent {
 public:
	RFC793eduTcpAgent();
	virtual void rtt_backoff();
        virtual void rtt_update(double tao);
        virtual void recv(Packet*, Handler*);
        virtual void opencwnd();
	virtual void output(int seqno, int reason = 0);
        void reset();
	virtual void recv_newack_helper(Packet*);
	void newack(Packet*);


 protected:
	virtual void delay_bind_init_all();
	virtual int delay_bind_dispatch(const char *varName, const char *localName, TclObject *tracer);

        int add793expbackoff_;  /* Add Karn's exp. backoff to rfc793 tcp */
        int add793jacobsonrtt_; /* Add Jacobson/Karels RTT estimation */
        int add793fastrtx_;     /* Add the Fast-Retransmit Algorithm */
        int add793slowstart_;   /* Add Slow Start with Congestion Avoidance */
        int add793additiveinc_; /* Add Congestion Avoidance, without
				 * Slow-Start (Additive
				 * Increase/Multiplicative Decrease) 
				 */
        int add793karnrtt_;     /* Add Karn Algorithm for RTT sampling */
        int add793exponinc_;    /* Add Multiplicative
				 * Increase/Multiplicative Decrease */
	double rto_;            /* Current value of the retx timer IN TICKS */
};

#endif




