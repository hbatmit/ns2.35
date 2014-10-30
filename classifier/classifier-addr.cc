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
    "@(#) $Header: /cvsroot/nsnam/ns-2/classifier/classifier-addr.cc,v 1.28 2006/02/21 15:20:17 mahrenho Exp $";
#endif

#include "classifier-addr.h"

int AddressClassifier::classify(Packet *p) {
	hdr_ip* iph = hdr_ip::access(p);
	return mshift(iph->daddr());
}

static class AddressClassifierClass : public TclClass {
public:
	AddressClassifierClass() : TclClass("Classifier/Addr") {}
	TclObject* create(int, const char*const*) {
		return (new AddressClassifier());
	}
} class_address_classifier;

/* added for mobileip code  Ya, 2/99*/
 
static class ReserveAddressClassifierClass : public TclClass {
public:
        ReserveAddressClassifierClass() : TclClass("Classifier/Addr/Reserve") {}
        TclObject* create(int, const char*const*) {
                return (new ReserveAddressClassifier());
        }
} class_reserve_address_classifier;
 
int ReserveAddressClassifier::command(int argc, const char*const* argv)
{
        // Tcl& tcl = Tcl::instance();

        if (argc == 3 && strcmp(argv[1],"reserve-port") == 0) {
                reserved_ = atoi(argv[2]);
                alloc((maxslot_ = reserved_ - 1));
                return(TCL_OK);
        }
        return (AddressClassifier::command(argc, argv));
}
 
void ReserveAddressClassifier::clear(int slot)
{
        slot_[slot] = 0;
        if (slot == maxslot_) {
                while (--maxslot_ >= reserved_ && slot_[maxslot_] == 0)
                        ;
        }
}
 
int ReserveAddressClassifier::classify(Packet *p) {
		hdr_ip* iph = hdr_ip::access(p);
		return iph->dport();
}

int ReserveAddressClassifier::getnxt(NsObject *nullagent)
{
        int i;
        for (i=reserved_; i < nslot_; i++)
                if (slot_[i]==0 || slot_[i]==nullagent)
                        return i;
        i=nslot_;
        alloc(nslot_); 
        return i;
}

static class BcastAddressClassifierClass : public TclClass {
public:
        BcastAddressClassifierClass() : TclClass("Classifier/Hash/Dest/Bcast") {}
        TclObject* create(int, const char*const*) { 
                return (new BcastAddressClassifier());
        }
} class_bcast_address_classifier;
 
NsObject* BcastAddressClassifier::find(Packet* p)
{
        NsObject* node = NULL;
        int cl = classify(p);
        if (cl < 0 || cl >= nslot_ || (node = slot_[cl]) == 0) {
                if (cl == BCAST_ADDR_MASK) {
                        // limited broadcast; assuming no such packet
                        // would be delivered back to sender
                        return bcast_recver_;
                }
		if (default_target_) 
			return default_target_;
 
                /*
                 * Sigh.  Can't pass the pkt out to tcl because it's
                 * not an object.
                 */
                Tcl::instance().evalf("%s no-slot %d", name(), cl);
                /*
                 * Try again.  Maybe callback patched up the table.
                 */
                cl = classify(p);
                if (cl < 0 || cl >= nslot_ || (node = slot_[cl]) == 0)
                        return (NULL);
        }
 
        return (node);
}
 
int BcastAddressClassifier::command(int argc, const char*const* argv)
{
        // Tcl& tcl = Tcl::instance();

        if (argc == 3 && strcmp(argv[1],"bcast-receiver") == 0) {
                bcast_recver_ = (NsObject*)TclObject::lookup(argv[2]);
                return(TCL_OK);
        }
        return (AddressClassifier::command(argc, argv));
}

