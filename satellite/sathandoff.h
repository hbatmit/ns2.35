/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1999 Regents of the University of California.
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
 *      This product includes software developed by the MASH Research
 *      Group at the University of California Berkeley.
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
 * @(#) $Header: /cvsroot/nsnam/ns-2/satellite/sathandoff.h,v 1.5 1999/10/26 17:35:06 tomh Exp $
 *
 * Contributed by Tom Henderson, UCB Daedalus Research Group, June 1999
 */

#ifndef ns_sathandoff_h
#define ns_sathandoff_h 

#include "timer-handler.h"
#include "rng.h"
#include "node.h"
#include <math.h>

// Handoff manager types
#define LINKHANDOFFMGR_SAT 1
#define LINKHANDOFFMGR_TERM 2


class TermLinkHandoffMgr;
class SatLinkHandoffMgr;

class TermHandoffTimer : public TimerHandler {
public:
        TermHandoffTimer(TermLinkHandoffMgr *a) : TimerHandler() {a_ = a; }
protected:
        virtual void expire(Event *e);
        TermLinkHandoffMgr *a_;
};

class SatHandoffTimer : public TimerHandler {
public:
        SatHandoffTimer(SatLinkHandoffMgr *a) : TimerHandler() {a_ = a; }
protected:
        virtual void expire(Event *e);
        SatLinkHandoffMgr *a_;
};

class SatLinkHead;
class SatNode;

class LinkHandoffMgr : public TclObject {
public:
	LinkHandoffMgr();
	void start() { handoff(); }
	Node* node() { return node_; } // backpointer to node
	virtual int handoff() { return 0; }
        int command(int argc, const char*const* argv);
	SatNode* get_peer(SatLinkHead*); // helper function for handoff
protected:
	Node* node_;
	static RNG handoff_rng_;
	static int handoff_randomization_;
	//
	// The remaining functions are helper functions for handoff
	//
	SatLinkHead* get_peer_next_linkhead(SatNode*);
	SatLinkHead* get_peer_linkhead(SatLinkHead*);
};

class SatLinkHandoffMgr : public LinkHandoffMgr {
public:
	SatLinkHandoffMgr();
	int handoff();
protected:
	SatHandoffTimer timer_;
	static double latitude_threshold_;
	static double longitude_threshold_;
	static int sat_handoff_int_;
};

class TermLinkHandoffMgr : public LinkHandoffMgr {
public:
	TermLinkHandoffMgr();
	int handoff();
protected:
	TermHandoffTimer timer_;
	static double elevation_mask_;
	static int term_handoff_int_;
};

#endif
