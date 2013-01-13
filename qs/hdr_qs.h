
/*
 * hdr_qs.h
 * Copyright (C) 2001 by the University of Southern California
 * $Id: hdr_qs.h,v 1.6 2005/08/25 18:58:10 johnh Exp $
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

/*
 * Quick Start for TCP and IP.
 * A scheme for transport protocols to dynamically determine initial 
 * congestion window size.
 *
 * http://www.ietf.org/internet-drafts/draft-amit-quick-start-02.ps
 *
 * This defines the Quick Start packet header
 * 
 * hdr_qs.h
 *
 * Srikanth Sundarrajan, 2002
 * sundarra@usc.edu
 */

#ifndef _HDR_QS_H
#define _HDR_QS_H

#include <assert.h>
#include <packet.h>

//#define QS_DEBUG 1 // get some output on QS events -PS

enum QS_STATE { QS_DISABLE, QS_REQUEST, QS_RESPONSE };

struct hdr_qs {
 
	int flag_;	// use QS_STATE
	int ttl_; 
	int rate_; 

        static double rate_to_Bps(int rate);
        static int Bps_to_rate(double Bps);

	static int offset_;	// offset for this header 
	inline int& offset() { return offset_; }

	inline static hdr_qs* access(Packet* p) {
		return (hdr_qs*) p->access(offset_);
	}

	int& flag() { return flag_; }
	int& ttl() { return ttl_; }
	int& rate() { return rate_; }
};

// Local Variables:
// mode:c++
// c-basic-offset: 8
// End:
#endif

