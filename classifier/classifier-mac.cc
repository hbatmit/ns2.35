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
 * Contributed by Giao Nguyen, http://daedalus.cs.berkeley.edu/~gnguyen
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/classifier/classifier-mac.cc,v 1.15 1999/01/04 19:58:56 haldar Exp $ (UCB)";
#endif

#include "packet.h"
#include "mac.h"
#include "classifier.h"

class MacClassifier : public Classifier {
public:
	MacClassifier() : bcast_(0) {
		bind("bcast_", &bcast_);
	}
	void recv(Packet*, Handler*);
	int bcast_;
};

static class MacClassifierClass : public TclClass {
public:
	MacClassifierClass() : TclClass("Classifier/Mac") {}
	TclObject* create(int, const char*const*) {
		return (new MacClassifier());
	}
} class_mac_classifier;


void MacClassifier::recv(Packet* p, Handler*)
{
	Mac* mac;
	hdr_mac* mh = HDR_MAC(p);

	if (bcast_ || mh->macDA() == BCAST_ADDR || (mac = (Mac *)find(p)) == 0) {
		// Replicate packets to all slots (broadcast)
		int macSA = mh->macSA();
		for (int i = 0; i < maxslot_; ++i) {
			if ((mac = (Mac *)slot_[i]) && mac->addr() != macSA) 
				mac->recv(p->copy(), 0);
		}
		if (maxslot_ >= 0) {
			mac= (Mac *)slot_[maxslot_];
			if (mac->addr() != macSA) { 
				mac->recv(p, 0);
				return;
			}
		}
		Packet::free(p);
		return;
	}
	mac->recv(p, 0);
}
