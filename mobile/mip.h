
/*
 * Copyright (C) 2000 by the University of Southern California
 * $Id: mip.h,v 1.8 2005/08/25 18:58:08 johnh Exp $
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
// $Header: /cvsroot/nsnam/ns-2/mobile/mip.h,v 1.8 2005/08/25 18:58:08 johnh Exp $

/*
 * Copyright (c) Sun Microsystems, Inc. 1998 All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Sun Microsystems, Inc.
 *
 * 4. The name of the Sun Microsystems, Inc nor may not be used to endorse or
 *      promote products derived from this software without specific prior
 *      written permission.
 *
 * SUN MICROSYSTEMS MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS
 * SOFTWARE FOR ANY PARTICULAR PURPOSE.  The software is provided "as is"
 * without express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this software.
 */

/* Ported by Ya Xu to official ns release  Jan. 1999 */


#ifndef ns_mip_h
#define ns_mip_h

#include <assert.h>
#include "agent.h"
#include "classifier-addr.h"

#define MIP_TIMER_SIMPLE	0
#define MIP_TIMER_AGTLIST	1

struct hdr_ipinip {
	hdr_ip hdr_;
	struct hdr_ipinip *next_;	// XXX multiple encapsulation OK

	// Header access methods
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_ipinip* access(const Packet* p) {
		return (hdr_ipinip*) p->access(offset_);
	}
};

typedef enum {
	MIPT_REG_REQUEST, MIPT_REG_REPLY, MIPT_ADS, MIPT_SOL
} MipRegType;

struct hdr_mip {
	int haddr_;
	int ha_;
	int coa_;
	MipRegType type_;
	double lifetime_;
	int seqno_;

	// Header access methods
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_mip* access(const Packet* p) {
		return (hdr_mip*) p->access(offset_);
	}
};

class MIPEncapsulator : public Connector {
public:
	MIPEncapsulator();
	void recv(Packet *p, Handler *h);
protected:
	ns_addr_t here_;
	int mask_;
	int shift_;
	int defttl_;
};

class MIPDecapsulator : public AddressClassifier {
public:
	MIPDecapsulator();
	void recv(Packet* p, Handler* h);
};

class SimpleTimer : public TimerHandler {
public: 
	SimpleTimer(Agent *a) : TimerHandler() { a_ = a; }
protected:
	inline void expire(Event *) { a_->timeout(MIP_TIMER_SIMPLE); }
	Agent *a_;
};

class MIPBSAgent : public Agent {
public:
	MIPBSAgent();
	virtual void recv(Packet *, Handler *);
	void timeout(int);
protected:
	int command(int argc, const char*const*argv);
	void send_ads(int dst = -1, NsObject *target = NULL);
	void sendOutBCastPkt(Packet *p);
	double beacon_; /* beacon period */
	NsObject *bcast_target_; /* where to send out ads */
	NsObject *ragent_;     /* where to send reg-replies to MH */
	SimpleTimer timer_;
	int mask_;
	int shift_;
#ifndef notdef
	int seqno_;		/* current ad seqno */
#endif
	double adlftm_;	/* ads lifetime */
};

class MIPMHAgent;

class AgtListTimer : public TimerHandler {
public: 
	AgtListTimer(MIPMHAgent *a) : TimerHandler() { a_ = a; }
protected:
	void expire(Event *e);
	MIPMHAgent *a_;
};

typedef struct _agentList {
	int node_;
	double expire_time_;
	double lifetime_;
	struct _agentList *next_;
} AgentList;

class MIPMHAgent : public Agent {
public:
	MIPMHAgent();
	void recv(Packet *, Handler *);
	void timeout(int);
protected:
	int command(int argc, const char*const*argv);
	void reg();
	void send_sols();
	void sendOutBCastPkt(Packet *p);
	int ha_;
	int coa_;
	double reg_rtx_;
	double beacon_;
	NsObject *bcast_target_; /* where to send out solicitations */
	AgentList *agts_;
	SimpleTimer rtx_timer_;
	AgtListTimer agtlist_timer_;
	int mask_;
	int shift_;
#ifndef notdef
	int seqno_;		/* current registration seqno */
#endif
	double reglftm_;	/* registration lifetime */
	double adlftm_;		/* current ads lifetime */
	MobileNode *node_;      /* ptr to my mobilenode,if appl. */

};

#endif
