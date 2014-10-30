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
 * Contributed by the Daedalus Research Group, UC Berkeley 
 * (http://daedalus.cs.berkeley.edu)
 *
 * Multi-state error model patches contributed by Jianping Pan 
 * (jpan@bbcr.uwaterloo.ca).
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/queue/fec.cc,v 1.1 2001/03/07 18:30:02 jahn Exp $ (UCB)
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/queue/fec.cc,v 1.1 2001/03/07 18:30:02 jahn Exp $ (UCB)";
#endif

#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include "packet.h"
#include "flags.h"
#include "mcast_ctrl.h"
#include "fec.h"
#include "srm-headers.h"		// to get the hdr_srm structure
#include "classifier.h"

static class FECModelClass : public TclClass {
public:
	FECModelClass() : TclClass("FECModel") {}
	TclObject* create(int, const char*const*) {
		return (new FECModel);
	}
} class_fecmodel;

FECModel::FECModel() : firstTime_(1), FECstrength_(1) 
{
}

int FECModel::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "FECstrength") == 0) {
			FECstrength_ = atoi(argv[2]);
			return (TCL_OK);
		}
	} else if (argc == 2) {
		if (strcmp(argv[1], "FECstrength") == 0) {
			tcl.resultf("%d", FECstrength_);
			return (TCL_OK);
		} 
	}
	return BiConnector::command(argc, argv);
}

int fix_ = 0;
void FECModel::recv(Packet* p, Handler* h)
{
	hdr_cmn* ch = hdr_cmn::access(p);

	if(ch->direction() == hdr_cmn::DOWN) {
		addfec(p);
		downtarget_->recv(p, h);
		return;
	} else {
		if(ch->errbitcnt() > FECstrength_) {
			Packet::free(p);
			return;
		} else if (ch->errbitcnt() && (ch->errbitcnt() <= FECstrength_)) {
			ch->errbitcnt() = 0;
			ch->error() = 0;
			printf("FEC: %d fixed\n", fix_++);
		}
		subfec(p);
		uptarget_->recv(p, h);
	}
}

void FECModel::addfec(Packet* p)
{
	int bit = hdr_cmn::access(p)->size() * 8;
	FECbyte_ = (int)(log(bit) / log(2));
	FECbyte_ *= FECstrength_;
	FECbyte_ = (FECbyte_ + 7) / 8;

	//	firstTime_ = 0;

        // printf("FEC Down = %d \n", hdr_cmn::access(p)->size());	

	hdr_cmn::access(p)->size() += FECbyte_;
	hdr_cmn::access(p)->fecsize() = FECbyte_;
}

void FECModel::subfec(Packet* p)
{
	FECbyte_ = hdr_cmn::access(p)->fecsize();
	hdr_cmn::access(p)->size() -= FECbyte_;
        // printf("FEC Up = %d \n", hdr_cmn::access(p)->size());	
}

