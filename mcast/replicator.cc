
/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1996 Regents of the University of California.
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
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/mcast/replicator.cc,v 1.21 2000/12/20 10:12:48 alefiyah Exp $";
#endif

#include "classifier.h"
#include "packet.h"
#include "ip.h"

/*
 * A replicator is not really a packet classifier but
 * we simply find convenience in leveraging its slot table.
 * (this object used to implement fan-out on a multicast
 * router as well as broadcast LANs)
 */
class Replicator : public Classifier {
public:
	Replicator();
	void recv(Packet*, Handler* h = 0);
	virtual int classify(Packet*) {/*NOTREACHED*/ return -1;};
protected:
	virtual int command(int argc, const char*const* argv);
	int ignore_;
	int direction_;
};

static class ReplicatorClass : public TclClass {
public:
	ReplicatorClass() : TclClass("Classifier/Replicator") {}
	TclObject* create(int, const char*const*) {
		return (new Replicator());
	}
} class_replicator;

Replicator::Replicator() : ignore_(0),direction_(0)
{
	bind("ignore_", &ignore_);
	bind_bool("direction_",&direction_);
}

void Replicator::recv(Packet* p, Handler*)
{
	hdr_ip* iph = hdr_ip::access(p);
	hdr_cmn* ch = hdr_cmn::access(p);
	if (maxslot_ < 0) {
		if (!ignore_) 
			Tcl::instance().evalf("%s drop %ld %ld %d", name(), 
				iph->saddr(), iph->daddr(), ch->iface());
		Packet::free(p);
		return;
	}
	
	//If the direction of the packet is DOWN, 
	// now that is has reached the end of the stack 
	// change the direction to UP
	if(direction_){
		if( HDR_CMN(p)->direction() == hdr_cmn::DOWN){
			ch->direction() = hdr_cmn::UP; // Up the stack 
		}
	}
	for (int i = 0; i < maxslot_; ++i) {
		NsObject* o = slot_[i];
		if (o != 0)
			o->recv(p->copy());
	}
	/* we know that maxslot is non-null */
	slot_[maxslot_]->recv(p);
}

int Replicator::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		/*
		 * $replicator slot
		 */
		if (strcmp(argv[1], "slots") == 0) {
			if (maxslot_ < 0) {
				tcl.result("");
				return (TCL_OK);
			}
			for (int i = 0; i <= maxslot_; i++) {
				if (slot_[i] == 0) 
					continue;
				tcl.resultf("%s %s", tcl.result(),
					    slot_[i]->name());
			}
			return (TCL_OK);
		}
		
	}
	return Classifier::command(argc, argv);
}





