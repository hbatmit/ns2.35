/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * Copyright (C) 1999 by the University of Southern California
 * $Id: fsm.h,v 1.4 2005/08/25 18:58:02 johnh Exp $
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
 * 
 * @(#) $Header: /cvsroot/nsnam/ns-2/common/fsm.h,v 1.4 2005/08/25 18:58:02 johnh Exp $ (LBL)
 */

#include "scheduler.h"

#define RTT 1
#define TIMEOUT 2

class FSMState {
protected:
	friend class FSM;
	void number_all();
	void print_all(int level);
	void print_all_stats(int desired_pkts, int pkts = 0,
			     int rtts = 0, int timeouts = 0, 
			     int ps = 0, int qs = 0,
			     int num_states = 0,
			     int num_state_names = 0);
	void reset_all_processed();
	bool processed() { return print_i_ < 0; }
public:
	FSMState() : print_i_(0) {};
	/* number of packets in this batch of transmission */
	int batch_size_; 
	/* time to make transition from previous state to this one
	* (either RTT or TIMEOUT) */
	int transition_[17];
	/* next states if dropping packet #1-16, 0 for none */
	FSMState* drop_[17];
	int print_i_;  // printing index (processed if negative)
};


class FSM : public TclObject {
public:
	FSM() {};
	inline FSMState* start_state() {	// starting state
		return (start_state_);
	}
	static FSM& instance() {
		return (*instance_);		// general access to scheduler
	}
	static void print_FSM(FSMState* state);
	static void print_FSM_stats(FSMState* state, int n);
protected:
	FSMState* start_state_;
	static FSM* instance_;
};



class TahoeAckFSM : public FSM {
public:
	TahoeAckFSM();
	inline FSMState* start_state() {	// starting state
		return (start_state_);
	}
	static TahoeAckFSM& instance() {
		return (*instance_);	       // general access to TahoeAckFSM
	}
protected:
	FSMState* start_state_;
	static TahoeAckFSM* instance_;

};

class RenoAckFSM : public FSM {
public:
	RenoAckFSM();
	inline FSMState* start_state() {	// starting state
		return (start_state_);
	}
	static RenoAckFSM& instance() {
		return (*instance_);	       // general access to TahoeAckFSM
	}
protected:
	FSMState* start_state_;
	static RenoAckFSM* instance_;
};


class TahoeDelAckFSM : public FSM {
public:
	TahoeDelAckFSM();
	inline FSMState* start_state() {	// starting state
		return (start_state_);
	}
	static TahoeDelAckFSM& instance() {
		return (*instance_);	       // general access to TahoeAckFSM
	}
protected:
	FSMState* start_state_;
	static TahoeDelAckFSM* instance_;
};

class RenoDelAckFSM : public FSM {
public:
	RenoDelAckFSM();
	inline FSMState* start_state() {	// starting state
		return (start_state_);
	}
	static RenoDelAckFSM& instance() {
		return (*instance_);	       // general access to TahoeAckFSM
	}
protected:
	FSMState* start_state_;
	static RenoDelAckFSM* instance_;
};
