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
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/classifier/classifier-virtual.cc,v 1.14 2010/03/08 05:54:49 tom_henderson Exp $";
#endif

extern "C" {
#include <tcl.h>
}
#include "config.h"
#include "packet.h"
#include "ip.h"
#include "classifier.h"
#include "route.h"
#include "object.h"
#include "address.h"

class VirtualClassifier : public Classifier {
public:
	VirtualClassifier() : routelogic_(0) {
		Tcl_InitHashTable(&ht_, TCL_ONE_WORD_KEYS);
	}
	~VirtualClassifier() {
		Tcl_DeleteHashTable(&ht_);
	}
	//virtual void do_install(char *dst, NsObject *target) { }
	virtual void do_install(char *, NsObject *) { }
protected:
	NsObject* next_;
	Tcl_HashTable ht_;
	RouteLogic *routelogic_;
	NsObject* target_;
	bool enableHrouting_;
	char nodeaddr_[SMALL_LEN];

	int classify(Packet *const p) {
		hdr_ip* iph = hdr_ip::access(p);
		return mshift(iph->daddr());
	}
	
	void recv(Packet* p, Handler* h) {
		if (!routelogic_) {
			Tcl &tcl = Tcl::instance();
			tcl.evalc("[Simulator instance] get-routelogic");
			routelogic_= (RouteLogic*) TclObject::lookup(tcl.result());
			//tcl.evalf("%s info class", tcl.result());
		}
		/* first we find the next hop by asking routelogic
		 * then we use a hash next_hop -> target_object
		 * thus, the size of the table is at most N-1
		 */
		Tcl &tcl = Tcl::instance();
		hdr_ip* iph = hdr_ip::access(p);
		char* adst= Address::instance().print_nodeaddr(iph->daddr());
		//adst[strlen(adst)-1]= 0;
		target_= 0;

		int next_hopIP;
		routelogic_->lookup_flat(nodeaddr_, adst, next_hopIP);
		delete [] adst;

		int newEntry;
		long key = next_hopIP;
		Tcl_HashEntry *ep= Tcl_CreateHashEntry(&ht_, (const char*)key, 
						       &newEntry); 
		if (newEntry) {
			tcl.evalf("%s find %d", name(), next_hopIP);
			Tcl_SetHashValue(ep, target_= (NsObject*)tcl.lookup(tcl.result()));
		} else {
			target_= (NsObject*)Tcl_GetHashValue(ep);
		}
		
		if (!target_) {
			/*
			 * XXX this should be "dropped" somehow.  Right now,
			 * these events aren't traced.
			 */
			Packet::free(p);
			return;
		}
		target_->recv(p,h);
	}
	int command(int argc, const char*const* argv) {
		if (argc == 3) {
			if (strcmp(argv[1], "nodeaddr") == 0) {
				strcpy(nodeaddr_, argv[2]);
				return(TCL_OK);
			}
		}
		return (NsObject::command(argc, argv));
	}
};

static class VirtualClassifierClass : public TclClass {
public:
	VirtualClassifierClass() : TclClass("Classifier/Virtual") {}
	TclObject* create(int, const char*const*) {
		return (new VirtualClassifier());
	}
} class_virtual_classifier;

