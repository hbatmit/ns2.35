/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * Copyright (C) 1999 by the University of Southern California
 * $Id: tcp-abs.h,v 1.2 2005/08/25 18:58:12 johnh Exp $
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
 * Contributed by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
 * @(#) $Header: /cvsroot/nsnam/ns-2/tcp/tcp-abs.h,v 1.2 2005/08/25 18:58:12 johnh Exp $ (LBL)";
 */

#include "agent.h"
#include "fsm.h"

class AbsTcpAgent;

class AbsTcpTimer : public TimerHandler {
public: 
        AbsTcpTimer(AbsTcpAgent *a) : TimerHandler() { a_ = a; }
protected:
        AbsTcpAgent *a_;
        void expire(Event *e);
};

class AbsTcpAgent : public Agent {
public:
        AbsTcpAgent();
        void timeout();
        void sendmsg(int pktcnt);
        void advanceby(int pktcnt);
	void start();
	void send_batch();
	void drop(int seqno);
	void finish();
	void recv(Packet* pkt, Handler*);
	void output(int seqno);
	inline int& flowid() { return fid_; }
        int command(int argc, const char*const* argv);
protected:
	double rtt_;
	FSMState* current_;
	int offset_;
	int seqno_lb_;                // seqno when finishing last batch
	int connection_size_;
        AbsTcpTimer timer_;
	int rescheduled_;
        void cancel_timer() {
                timer_.force_cancel();
        }
        void set_timer(double time) {
	  	timer_.resched(time);
	}

};


class AbsTcpTahoeAckAgent : public AbsTcpAgent {
public:
        AbsTcpTahoeAckAgent();
};

class AbsTcpRenoAckAgent : public AbsTcpAgent {
public:
        AbsTcpRenoAckAgent();
};

class AbsTcpTahoeDelAckAgent : public AbsTcpAgent {
public:
        AbsTcpTahoeDelAckAgent();
};

class AbsTcpRenoDelAckAgent : public AbsTcpAgent {
public:
        AbsTcpRenoDelAckAgent();
};

//AbsTcpSink
class AbsTcpSink : public Agent {
public:
        AbsTcpSink();
        void recv(Packet* pkt, Handler*);
protected:
};


class AbsDelAckSink;

class AbsDelayTimer : public TimerHandler {
public:
        AbsDelayTimer(AbsDelAckSink *a) : TimerHandler() { a_ = a; }
protected:
        void expire(Event *e);
        AbsDelAckSink *a_;
};

class AbsDelAckSink : public AbsTcpSink {
public:
        AbsDelAckSink();
        void recv(Packet* pkt, Handler*);
        void timeout();
protected:
        Packet* save_;          /* place to stash packet while delaying */
        double interval_;
        AbsDelayTimer delay_timer_;
};

// Dropper Class
class Dropper {
public:
	AbsTcpAgent* agent_;
        Dropper* next_;
};

class DropTargetAgent : public Connector {
public:
        DropTargetAgent();
        void recv(Packet* pkt, Handler*);
        void insert_tcp(AbsTcpAgent* tcp);
	static DropTargetAgent& instance() {
		return (*instance_);	       // general access to TahoeAckFSM
	}
protected:
	Dropper* dropper_list_;
	static DropTargetAgent* instance_;
};
