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
    "@(#) $Header: /cvsroot/nsnam/ns-2/classifier/classifier-mcast.cc,v 1.32 2005/09/26 07:38:08 lacage Exp $";
#endif

#include <stdlib.h>
#include "config.h"
#include "packet.h"
#include "ip.h"
#include "classifier.h"
#include "classifier-mcast.h"

const char MCastClassifier::STARSYM[]= "x"; //"source" field for shared trees

static class MCastClassifierClass : public TclClass {
public:
	MCastClassifierClass() : TclClass("Classifier/Multicast") {}
	TclObject* create(int, const char*const*) {
		return (new MCastClassifier());
	}
} class_mcast_classifier;

MCastClassifier::MCastClassifier()
{
	memset(ht_, 0, sizeof(ht_));
	memset(ht_star_, 0, sizeof(ht_star_));
}

MCastClassifier::~MCastClassifier()
{
	clearAll();
}

void MCastClassifier::clearHash(hashnode* h[], int size) 
{
	for (int i = 0; i < size; ++i) {
		hashnode* p = h[i];
		while (p != 0) {
			hashnode* n = p->next;
			delete p;
			p = n;
		}
	}
	memset(h, 0, size * sizeof(hashnode*));
}

void MCastClassifier::clearAll()
{
	clearHash(ht_, HASHSIZE);
	clearHash(ht_star_, HASHSIZE);
}

MCastClassifier::hashnode*
MCastClassifier::lookup(nsaddr_t src, nsaddr_t dst, int iface) const
{
	int h = hash(src, dst);
	hashnode* p;
	for (p = ht_[h]; p != 0; p = p->next) {
		if (p->src == src && p->dst == dst)
 			if (p->iif == iface ||
 			    //p->iif == UNKN_IFACE.value() ||
 			    iface == ANY_IFACE.value())
			break;
	}
	return (p);
}

MCastClassifier::hashnode*
MCastClassifier::lookup_star(nsaddr_t dst, int iface) const
{
	int h = hash(0, dst);
	hashnode* p;
	for (p = ht_star_[h]; p != 0; p = p->next) {
		if (p->dst == dst && 
		    (iface == ANY_IFACE.value() || p->iif == iface))
  		       break;
  	}
	return (p);
}

int MCastClassifier::classify(Packet *pkt)
{
	hdr_cmn* h = hdr_cmn::access(pkt);
	hdr_ip* ih = hdr_ip::access(pkt);

	nsaddr_t src = ih->saddr();
	nsaddr_t dst = ih->daddr();

	int iface = h->iface();
	Tcl& tcl = Tcl::instance();

	hashnode* p = lookup(src, dst, iface);
	//printf("%s, src %d, dst %d, iface %d, p %d\n", name(), src, dst, iface, p);
 	if (p == 0)
 	        p = lookup_star(dst, iface);
		
	if (p == 0) {
		if ((p = lookup(src, dst)) == 0)
			p = lookup_star(dst);
		if (p == 0) {
  			// Didn't find an entry.
			tcl.evalf("%s new-group %d %d %d cache-miss", 
				  name(), src, dst, iface);
			// XXX see McastProto.tcl for the return values 0 -
			// once, 1 - twice 
			//printf("cache-miss result= %s\n", tcl.result());
			int res= atoi(tcl.result());

			return (res)? Classifier::TWICE : Classifier::ONCE;
		}
		if (p->iif == ANY_IFACE.value()) // || iface == UNKN_IFACE.value())
			return p->slot;

		tcl.evalf("%s new-group %d %d %d wrong-iif", 
			  name(), src, dst, iface);
		//printf("wrong-iif result= %s\n", tcl.result());
		int res= atoi(tcl.result());
		return (res)? Classifier::TWICE : Classifier::ONCE;
	}
	return p->slot;
}

int MCastClassifier::findslot()
{
	int i;
	for (i = 0; i < nslot_; ++i)
		if (slot_[i] == 0)
			break;
	return (i);
}

void MCastClassifier::set_hash(hashnode* ht[], nsaddr_t src, nsaddr_t dst,
			       int slot, int iface)
{
	int h = hash(src, dst);
	hashnode* p = new hashnode;
	p->src = src;
	p->dst = dst;
	p->slot = slot;
	p->iif = iface;
	p->next = ht[h];
	ht[h] = p;
}

int MCastClassifier::command(int argc, const char*const* argv)
{
	if (argc == 6) {
		if (strcmp(argv[1], "set-hash") == 0) {
			// $classifier set-hash $src $group $slot $iif
			//      $iif can be:(1) <number>
			//                  (2) "*" - matches any interface
			//                  (3) "?" - interface is unknown (usually this means that
			//			       the packet came from a local agent)
			nsaddr_t src = strtol(argv[2], (char**)0, 0);
			nsaddr_t dst = strtol(argv[3], (char**)0, 0);
			int slot = atoi(argv[4]);
                        int iface = (strcmp(argv[5], ANY_IFACE.name())==0) ? ANY_IFACE.value() 
				: (strcmp(argv[5], UNKN_IFACE.name())==0) ? UNKN_IFACE.value() 
				: atoi(argv[5]); 
			if (strcmp(STARSYM, argv[2]) == 0) {
			    // install a <x,G> entry: give 0 as src, but can be anything
			    set_hash(ht_star_, 0, dst, slot, iface);
			} else {
			    //install a <S,G> entry
			    set_hash(ht_, src, dst, slot, iface);
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "change-iface") == 0) {
			// $classifier change-iface $src $dst $olfiif $newiif
			nsaddr_t src = strtol(argv[2], (char**)0, 0);
			nsaddr_t dst = strtol(argv[3], (char**)0, 0);
			int oldiface = atoi(argv[4]);
			int newiface = atoi(argv[5]);
			if (strcmp(STARSYM, argv[2]) == 0) {
				change_iface(dst, oldiface, newiface);
			} else {
				change_iface(src, dst, oldiface, newiface);
			}
			return (TCL_OK);
		}
	} else if (argc == 5) {
		if (strcmp(argv[1], "lookup") == 0) {
			// $classifier lookup $src $group $iface
			// returns name of the object (replicator)
			Tcl &tcl = Tcl::instance();
			nsaddr_t src = strtol(argv[2], (char**)0, 0);
			nsaddr_t dst = strtol(argv[3], (char**)0, 0);
			int iface = atoi(argv[4]);
			
			hashnode* p= (strcmp(STARSYM, argv[2]) == 0) ? lookup_star(dst, iface)
				: lookup(src, dst, iface);
			if ((p == 0) || (slot_[p->slot] == 0))
				tcl.resultf("");
			else 
				tcl.resultf("%s", slot_[p->slot]->name());
			return (TCL_OK);
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "lookup-iface") == 0) {
			// $classifier lookup-iface $src $group 
			// returns incoming iface
			Tcl &tcl = Tcl::instance();
			nsaddr_t src = strtol(argv[2], (char**)0, 0);
			nsaddr_t dst = strtol(argv[3], (char**)0, 0);
			hashnode* p= (strcmp(argv[2], STARSYM) == 0) ? lookup_star(dst)
				: lookup(src, dst);
			if (p == 0)
				tcl.resultf("");
			else 
				tcl.resultf("%d", p->iif);
			return (TCL_OK);
		}
	} else if (argc == 2) {
		if (strcmp(argv[1], "clearAll") == 0) {
			clearAll();
			return (TCL_OK);
		}
	}
	return (Classifier::command(argc, argv));
}


/* interface look up for the interface code*/
void MCastClassifier::change_iface(nsaddr_t src, nsaddr_t dst, int oldiface, int newiface)
{

	hashnode* p = lookup(src, dst, oldiface);
	if (p) p->iif = newiface;
}

void MCastClassifier::change_iface(nsaddr_t dst, int oldiface, int newiface)
{
	hashnode* p = lookup_star(dst, oldiface);
	if (p) p->iif = newiface;
}

