/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * Copyright (C) 1999 by the University of Southern California
 * $Id: classifier-port.cc,v 1.9 2006/02/21 15:20:17 mahrenho Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */

// @(#) $Header: /cvsroot/nsnam/ns-2/classifier/classifier-port.cc,v 1.9 2006/02/21 15:20:17 mahrenho Exp $ (USC/ISI)

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/classifier/classifier-port.cc,v 1.9 2006/02/21 15:20:17 mahrenho Exp $";
#endif

#include "classifier-port.h"

int PortClassifier::classify(Packet *p) 
{
	// Port classifier returns the destination port.  No shifting
	// or masking is required since in the 32-bit addressing,
	// ports are stored in a seperate variable.
	hdr_ip* iph = hdr_ip::access(p);
	return iph->dport();
}

static class PortClassifierClass : public TclClass {
public:
	PortClassifierClass() : TclClass("Classifier/Port") {}
	TclObject* create(int, const char*const*) {
		return (new PortClassifier());
	}
} class_address_classifier;

static class ReservePortClassifierClass : public TclClass {
public:
        ReservePortClassifierClass() : TclClass("Classifier/Port/Reserve") {}
        TclObject* create(int, const char*const*) {
                return (new ReservePortClassifier());
        }
} class_reserve_port_classifier;

int ReservePortClassifier::command(int argc, const char*const* argv)
{
        if (argc == 3 && strcmp(argv[1],"reserve-port") == 0) {
                reserved_ = atoi(argv[2]);
                alloc((maxslot_ = reserved_ - 1));
                return(TCL_OK);
        }
        return (Classifier::command(argc, argv));
}

void ReservePortClassifier::clear(int slot)
{
        slot_[slot] = 0;
        if (slot == maxslot_) {
                while (--maxslot_ >= reserved_ && slot_[maxslot_] == 0)
                        ;
        }
}
 
int ReservePortClassifier::getnxt(NsObject *nullagent)
{
        int i;
        for (i=reserved_; i < nslot_; i++)
                if (slot_[i]==0 || slot_[i]==nullagent)
                        return i;
        i=nslot_;
        alloc(nslot_); 
        return i;
}

