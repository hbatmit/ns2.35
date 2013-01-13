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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
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

 * @(#) $Header: 

 *
 * Ported from CMU/Monarch's code, nov'98 -Padma Haldar.
 * phy.cc
 */

#include <math.h>

#include "config.h"
#include <packet.h>
#include <phy.h>
#include <dsr/hdr_sr.h>

class Mac;

static int InterfaceIndex = 0;


Phy::Phy() : BiConnector() {
	index_ = InterfaceIndex++;
	bandwidth_ = 0.0;
	channel_ = 0;
	node_ = 0;
	head_ = 0;
}

int
Phy::command(int argc, const char*const* argv) {
	if (argc == 2) {
		Tcl& tcl = Tcl::instance();

		if(strcmp(argv[1], "id") == 0) {
			tcl.resultf("%d", index_);
			return TCL_OK;
		}
	}

	else if(argc == 3) {

		TclObject *obj;

		if( (obj = TclObject::lookup(argv[2])) == 0) {
			fprintf(stderr, "%s lookup failed\n", argv[1]);
			return TCL_ERROR;
		}
		if (strcmp(argv[1], "channel") == 0) {
                        assert(channel_ == 0);
			channel_ = (Channel*) obj;
			downtarget_ = (NsObject*) obj;
			// LIST_INSERT_HEAD() is done by Channel
			return TCL_OK;
		}
		else if (strcmp(argv[1], "node") == 0) {
			assert(node_ == 0);
			node_ = (Node*) obj;
			// LIST_INSERT_HEAD() is done by Node
			return TCL_OK;
		}
		else if (strcmp(argv[1], "linkhead") == 0) {
			head_ = (LinkHead*)  obj;
			return (TCL_OK);
		}

	} 
	return BiConnector::command(argc, argv);
}



void
Phy::recv(Packet* p, Handler*)
{
	struct hdr_cmn *hdr = HDR_CMN(p);	
	//struct hdr_sr *hsr = HDR_SR(p);
	
	/*
	 * Handle outgoing packets
	 */
	switch(hdr->direction()) {
	case hdr_cmn::DOWN :
		/*
		 * The MAC schedules its own EOT event so we just
		 * ignore the handler here.  It's only purpose
		 * it distinguishing between incoming and outgoing
		 * packets.
		 */
		sendDown(p);
		return;
	case hdr_cmn::UP :
		if (sendUp(p) == 0) {
			/*
			 * XXX - This packet, even though not detected,
			 * contributes to the Noise floor and hence
			 * may affect the reception of other packets.
			 */
			Packet::free(p);
			return;
		} else {
			uptarget_->recv(p, (Handler*) 0);
		}
		break;
	default:
		printf("Direction for pkt-flow not specified; Sending pkt up the stack on default.\n\n");
		if (sendUp(p) == 0) {
			/*
			 * XXX - This packet, even though not detected,
			 * contributes to the Noise floor and hence
			 * may affect the reception of other packets.
			 */
			Packet::free(p);
			return;
		} else {
			uptarget_->recv(p, (Handler*) 0);
		}
	}
	
}

/* NOTE: this might not be the best way to structure the relation
between the actual interfaces subclassed from net-if(phy) and 
net-if(phy). 
It's fine for now, but if we were to decide to have the interfaces
themselves properly handle multiple incoming packets (they currently
require assistance from the mac layer to do this), then it's not as
generic as I'd like.  The way it is now, each interface will have to
have it's own logic to keep track of the packets that are arriving.
Seems like this is general service that net-if could provide.

Ok.  A fair amount of restructuring is going to have to happen here
when/if net-if keep track of the noise floor at their location.  I'm
gonna punt on it for now.

Actually, this may be all wrong.  Perhaps we should keep a separate 
noise floor per antenna, which would mean the particular interface types
would have to track noise floor themselves, since only they know what
kind of antenna diversity they have.  -dam 8/7/98 */


// double
// Phy::txtime(Packet *p) const
// {
// 	hdr_cmn *hdr = HDR_CMN(p);
// 	return hdr->size() * 8.0 / Rb_;
// }


void
Phy::dump(void) const
{
	fprintf(stdout, "\tINDEX: %d\n",
		index_);
	fprintf(stdout, "\tuptarget: %lx, channel: %lx",
		(long) uptarget_, (long) channel_);

}


