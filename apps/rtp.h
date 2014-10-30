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
 * @(#) $Header: /cvsroot/nsnam/ns-2/apps/rtp.h,v 1.14 2000/09/01 03:04:06 haoboy Exp $
 */

#ifndef ns_rtp_h
#define ns_rtp_h

#include "config.h"
#include "object.h"
#include "agent.h"
#include "timer-handler.h"

#define RTP_M 0x0080 // marker for significant events

/* rtp packet.  For now, just have srcid + seqno. */
struct hdr_rtp { 
	u_int32_t srcid_;
	int seqno_;
	//rtp flags indicating significant event(begining of talkspurt)
	u_int16_t flags_; 

	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_rtp* access(const Packet* p) {
		return (hdr_rtp*) p->access(offset_);
	}

	/* per-field member functions */
	u_int32_t& srcid() { return (srcid_); }
	int& seqno() { return (seqno_); }
	u_int16_t& flags() { return (flags_); }
};

class RTPSource : public TclObject {
public:
	RTPSource* next;

	RTPSource(u_int32_t srcid);
	inline u_int32_t srcid() { return (srcid_); }
	inline int np() { return (np_); }
	inline int snp() { return (snp_); }
	inline int ehsr() { return (ehsr_); }

	inline void np(int n) { np_ += n; }
	inline void snp(int n) { snp_ = n; }
	inline void ehsr(int n) { ehsr_ = n; }
protected:
	u_int32_t srcid_;
	int np_;
	int snp_;
	int ehsr_;
};

class RTPSession : public NsObject {
public:
	RTPSession();
	~RTPSession();
	virtual void recv(Packet* p, Handler*);
	virtual void recv_ctrl(Packet* p);
	int command(int argc, const char*const* argv);
	inline u_int32_t srcid() { return (localsrc_->srcid()); }
	int build_report(int bye);
	void localsrc_update(int);
protected:
	RTPSource* allsrcs_;
	RTPSource* localsrc_;
	int build_sdes();
	int build_bye();
	RTPSource* lookup(u_int32_t);
	void enter(RTPSource*);
	int last_np_;
};

class RTPAgent;

class RTPTimer : public TimerHandler {
public: 
        RTPTimer(RTPAgent *a) : TimerHandler() { a_ = a; }
protected:
        virtual void expire(Event *e);
        RTPAgent *a_;
};

class RTPAgent : public Agent {
 public:
        RTPAgent();
        virtual void timeout(int);
        virtual void recv(Packet* p, Handler*);
        virtual int command(int argc, const char*const* argv);
        void advanceby(int delta);
        virtual void sendmsg(int nbytes, const char *flags = 0);
 protected:
        virtual void sendpkt();
	virtual void makepkt(Packet*);
        void rate_change();
        virtual void start();
        virtual void stop();
        virtual void finish();
        RTPSession* session_;
        double lastpkttime_;
        int seqno_;
        int running_;
        int random_;
        int maxpkts_;
        double interval_;
        RTPTimer rtp_timer_;
};


#endif
