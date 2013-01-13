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
 * Contributed by the Daedalus Research Group, http://daedalus.cs.berkeley.edu
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/mac/ll.h,v 1.30 2010/03/08 05:54:52 tom_henderson Exp $ (UCB)
 */

#ifndef ns_ll_h
#define ns_ll_h

#include <delay.h>
#include <queue.h>
#include <arp.h>
#include <classifier.h>
#include <lanRouter.h>
#include <varp.h>

enum LLFrameType {
	LL_DATA		= 0x0001,
	LL_ACK		= 0x0010
};

struct hdr_ll {
	LLFrameType lltype_;		// link-layer frame type
	int seqno_;			// sequence number
	int ackno_;			// acknowledgement number
	int bopno_;			// begin of packet seqno
	int eopno_;			// end of packet seqno
	int psize_;			// size of packet
	double sendtime_;		// time the packet is sent

	static int offset_;
	inline int& offset() { return offset_; }
	static hdr_ll* access(const Packet* p) {
		return (hdr_ll*) p->access(offset_);
	}

	inline LLFrameType& lltype() { return lltype_; }
	inline int& seqno() { return seqno_; }
	inline int& ackno() { return ackno_; }
	inline int& bopno() { return bopno_; }
	inline int& eopno() { return eopno_; }
	inline int& psize() { return psize_; }
	inline double& sendtime() { return sendtime_; }
};


class LL : public LinkDelay {
public:
	friend void ARPTable::arpinput(Packet *p, LL* ll);
	friend void ARPTable::arprequest(nsaddr_t src, nsaddr_t dst, LL* ll);
	LL();
	virtual void recv(Packet* p, Handler* h);
	void handle(Event* e) { recv((Packet*)e, 0); }
	inline int initialized() {
		return (mac_ && uptarget_ && downtarget_);
	}

	virtual void sendUp(Packet* p);
	virtual void sendDown(Packet* p);
	
	inline int seqno() { return seqno_; }
	inline int ackno() { return ackno_; }
	inline int macDA() { return macDA_; }
	virtual void hdr_dst(Packet *p, int macDA);
        inline Queue *ifq() { return ifq_; }
        inline NsObject* downtarget() { return downtarget_; }
        inline NsObject* uptarget() { return uptarget_; }
 	inline ARPTable *arp_table() { return arptable_; }
	
protected:
	int command(int argc, const char*const* argv);
	
	int seqno_;			// link-layer sequence number
	int ackno_;			// ACK received so far
	int macDA_;			// destination MAC address
	
        Queue* ifq_;			// interface queue
	Mac*   mac_;		        // MAC object
        LanRouter* lanrouter_;          // for lookups of the next hop
	ARPTable*  arptable_;           // ARP table object
	VARPTable* varp_;               // Virtual ARP object
	
	NsObject* downtarget_;		// for outgoing packet 
	NsObject* uptarget_;		// for incoming packet
        
	
};

#endif
