
/*
 * hdr-ls.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: hdr-ls.h,v 1.3 2005/08/25 18:58:06 johnh Exp $
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

// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
// $Header: /cvsroot/nsnam/ns-2/linkstate/hdr-ls.h,v 1.3 2005/08/25 18:58:06 johnh Exp $

//  Copyright (C) 1998 by Mingzhou Sun. All rights reserved.
//  This software is developed at Rensselaer Polytechnic Institute under 
//  DARPA grant No. F30602-97-C-0274
//  Redistribution and use in source and binary forms are permitted
//  provided that the above copyright notice and this paragraph are
//  duplicated in all such forms and that any documentation, advertising
//  materials, and other materials related to such distribution and use
//  acknowledge that the software was developed by Mingzhou Sun at the
//  Rensselaer  Polytechnic Institute.  The name of the University may not 
//  be used to endorse or promote products derived from this software 
//  without specific prior written permission.

// Link state header should be present in ns regardless if the link state
// module is included (it may be omitted if standard STL is not supported
// by the compiler). The reason is we do not want a ns-packet.tcl.in, 
// and we cannot initialize a packet header in ns-stl.tcl.in. 
// Mysteriously the latter solution does not work; the only victim is DSR
// tests (e.g., DSR tests in wireless-lan, wireless-lan-newnode, wireless-tdma)

#ifndef ns_ls_hdr_h
#define ns_ls_hdr_h

#include "config.h"
#include "packet.h"

struct hdr_LS {
        u_int32_t mv_;  // metrics variable identifier
	int msgId_;

        u_int32_t& metricsVar() { return mv_; }
	int& msgId() { return msgId_; }

	// Header access methods
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_LS* access(const Packet* p) {
		return (hdr_LS*) p->access(offset_);
	}
};

#endif // ns_ls_hdr_h
