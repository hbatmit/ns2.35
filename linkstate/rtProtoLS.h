
/*
 * rtProtoLS.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: rtProtoLS.h,v 1.4 2005/08/25 18:58:06 johnh Exp $
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
//
// $Header: /cvsroot/nsnam/ns-2/linkstate/rtProtoLS.h,v 1.4 2005/08/25 18:58:06 johnh Exp $

#ifndef ns_rtprotols_h
#define ns_rtprotols_h

#include "packet.h"
#include "agent.h"
#include "ip.h"
#include "ls.h" 
#include "hdr-ls.h"

extern LsMessageCenter messageCenter;

class rtProtoLS : public Agent , public LsNode {
public:
        rtProtoLS() : Agent(PT_RTPROTO_LS) { 
		LS_ready_ = 0;
	}
        int command(int argc, const char*const* argv);
        void sendpkt(ns_addr_t dst, u_int32_t z, u_int32_t mtvar);
        void recv(Packet* p, Handler*);

protected:
	void initialize(); // init nodeState_ and routing_
	void setDelay(int nbrId, double delay) {
		delayMap_.insert(nbrId, delay);
	}
	void sendBufferedMessages() { routing_.sendBufferedMessages(); }
	void computeRoutes() { routing_.computeRoutes(); }
	void intfChanged();
	void sendUpdates() { routing_.sendLinkStates(); }
	void lookup(int destinationNodeId);

public:
	bool sendMessage(int destId, u_int32_t messageId, int size);
	void receiveMessage(int sender, u_int32_t msgId);

	int getNodeId() { return nodeId_; }
	LsLinkStateList* getLinkStateListPtr()  { return &linkStateList_; }
	LsNodeIdList* getPeerIdListPtr() { return &peerIdList_; }
	LsDelayMap* getDelayMapPtr() { 
		return delayMap_.empty() ? (LsDelayMap *)NULL : &delayMap_;
	}
	void installRoutes() {
		Tcl::instance().evalf("%s route-changed", name());
	}

private:
	typedef LsMap<int, ns_addr_t> PeerAddrMap; // addr for peer Id
	PeerAddrMap peerAddrMap_;
	int nodeId_;
	int LS_ready_;	// to differentiate fake and real LS, debug, 0 == no
			// needed in recv and sendMessage;

	LsLinkStateList linkStateList_;
	LsNodeIdList peerIdList_;
	LsDelayMap delayMap_;
	LsRouting routing_;

	int findPeerNodeId(ns_addr_t agentAddr);
};

#endif // ns_rtprotols_h
