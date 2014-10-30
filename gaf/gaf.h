
/*
 * gaf.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: gaf.h,v 1.4 2005/08/25 18:58:05 johnh Exp $
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

// Header file for grid-based adaptive fidelity algorithm 

#ifndef ns_gaf_h
#define ns_gaf_h

#include <assert.h>
#include "agent.h"
#include "node.h"

#define MAXQUEUE  1             // 1 = only my grid, 5 = all of my neighbos
#define GN_UPDATE_INTERVAL 10 	// how often to update neighbor list
#define GAF_STARTUP_JITTER 1.0	// secs to jitter start of periodic activity
#define GAF_NONSTART_JITTER 3.0
#define MIN_DISCOVERY_TIME 1    // min interval to send discovery
#define MAX_DISCOVERY_TIME 15   // max interval to send discovery
#define MIN_SELECT_TIME 5	// start to tell your neighbor that
#define MAX_SELECT_TIME 6	// you are the leader b/w, my need
				// to be set by apps. 
#define MAX_DISCOVERY   10      // send selection after 10 time try
#define MIN_LIFETIME    60
#define GAF_LEADER_JITTER 3
#define MIN_TURNOFFTIME 1

class GAFAgent;

typedef enum {
  GAF_DISCOVER, GAF_SELECT, GAF_DUTY
} GafMsgType;

typedef enum {
  GAF_FREE, GAF_LEADER, GAF_SLEEP
} GafNodeState;

/*
 * data structure for exchanging existence message and selection msg
 */

struct DiscoveryMsg {
        u_int32_t gid;	// grid id
        u_int32_t nid;  // node id
        u_int32_t state; // what is my state
        u_int32_t ttl;  // My time to live
        u_int32_t stime;  // I may stay on this grid for only stime

};


// gaf header

struct hdr_gaf {
	// needs to be extended

	int seqno_;
        GafMsgType type_;

	// Header access methods
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_gaf* access(const Packet* p) {
		return (hdr_gaf*) p->access(offset_);
	}
};


// GAFTimer is used for discovery phase

class GAFDiscoverTimer : public TimerHandler {
public: 
	GAFDiscoverTimer(GAFAgent *a) : TimerHandler(), a_(a) { }
protected:
	virtual void expire(Event *);
	GAFAgent *a_;
};

// GAFSelectTimer is for the slecting phase

class GAFSelectTimer : public TimerHandler {
public:
        GAFSelectTimer(GAFAgent *a) : TimerHandler(), a_(a) { }
protected:
        virtual void expire(Event *);
        GAFAgent *a_;
};

// GAFDutyTimer is for duty cycle. It is the place of adaptive fidelity
// plays

class GAFDutyTimer : public TimerHandler {
public:
        GAFDutyTimer(GAFAgent *a) : TimerHandler(), a_(a) { }
protected:
        inline void expire(Event *);
        GAFAgent *a_;
};

class GAFAgent : public Agent {
public:
	GAFAgent(nsaddr_t id);
	virtual void recv(Packet *, Handler *);
	void timeout(GafMsgType);
	//void select_timeout(int);

	u_int32_t nodeid() {return nid_;}
	double myttl();
		
protected:
	int command(int argc, const char*const*argv);
	

	void node_on();
	void node_off();
	void duty_timeout();
	void send_discovery();
	void makeUpDiscoveryMsg(Packet *p);
	void processDiscoveryMsg(Packet *p);
	void schedule_wakeup(struct DiscoveryMsg);
	double beacon_; /* beacon period */
	void setGAFstate(GafNodeState);
	int randomflag_;
	GAFDiscoverTimer timer_;
	GAFSelectTimer stimer_;
	GAFDutyTimer dtimer_;
	int seqno_;
	int gid_;	// group id of this node
	int nid_;	// the node id of this node belongs to.
	Node *thisnode; // the node object where this agent resides
	int maxttl_;	// life of a node in the neighbor list
	
	GafNodeState	state_;
	int leader_settime_;
	int adapt_mobility_;  // control the use of 
	                      // GAF-3: load balance with aggressive sleeping
	                      // GAF-4:  load 3 + mobility adaption
};

/* assisting getting broadcast msg */

class GAFPartner : public Connector {
public:
        GAFPartner();
        void recv(Packet *p, Handler *h);
	
protected:
	int command(int argc, const char*const*argv);
        ns_addr_t here_;
	int gafagent_;
        int mask_;
        int shift_;
};



#endif
