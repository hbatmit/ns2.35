/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linking this file statically or dynamically with other modules is making
 * a combined work based on this file.  Thus, the terms and conditions of
 * the GNU General Public License cover the whole combination.
 *
 * In addition, as a special exception, the copyright holders of this file
 * give you permission to combine this file with free software programs or
 * libraries that are released under the GNU LGPL and with code included in
 * the standard release of ns-2 under the Apache 2.0 license or under
 * otherwise-compatible licenses with advertising requirements (or modified
 * versions of such code, with unchanged license).  You may copy and
 * distribute such a system following the terms of the GNU GPL for this
 * file and the licenses of the other code concerned, provided that you
 * include the source code of that other code when and as the GNU GPL
 * requires distribution of source code.
 *
 * Note that people who make modified versions of this file are not
 * obligated to grant this special exception for their modified versions;
 * it is their choice whether to do so.  The GNU General Public License
 * gives permission to release a modified version without this exception;
 * this exception also makes it possible to release a modified version
 * which carries forward this exception.
 */
#ifndef lint
static const char  rcsid[] =
	"@(#) $Header: /cvsroot/nsnam/ns-2/adc/consrcvr.cc,v 1.8 2005/08/26 05:05:27 tomh Exp $";
#endif


#include "agent.h"
#include "config.h"
#include "tclcl.h"
#include "packet.h"
#include "rtp.h"
#include "adaptive-receiver.h"
#ifndef WIN32
// VC 5.0 doesn't have this
#include <sys/time.h>
#endif

//#define CONS_OFFSET 0.025*SAMPLERATE
#define CONS_OFFSET 200

class ConsRcvr : public AdaptiveRcvr {
public:
	ConsRcvr();
protected:
	int adapt(Packet *pkt, u_int32_t time);
	int offset_;
};

static class ConsRcvrClass : public TclClass {
public:
	ConsRcvrClass() : TclClass("Agent/ConsRcvr") {}
	TclObject* create(int, const char*const*) {
		return (new ConsRcvr());

	}
} class_cons_rcvr;


ConsRcvr::ConsRcvr() : offset_(CONS_OFFSET)
{
}

int ConsRcvr::adapt(Packet *pkt, u_int32_t local_clock)
{
	
	int delay;
	hdr_cmn* ch = hdr_cmn::access(pkt);
	register u_int32_t tstamp = (int)ch->timestamp();
	
	if (((tstamp+offset_) < local_clock) || (offset_ == -1)) {
		/*increase the offset */
		if (offset_ < (int)(local_clock-(tstamp+offset_)))
			offset_ += local_clock -(tstamp+offset_);
		else
			offset_ += offset_;
	}
	
	delay=offset_-(local_clock-tstamp);
	
	return delay;
}
