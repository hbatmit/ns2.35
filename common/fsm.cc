/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * Copyright (C) 1999 by the University of Southern California
 * $Id: fsm.cc,v 1.12 2005/08/25 18:58:02 johnh Exp $
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
 * @(#) $Header: /cvsroot/nsnam/ns-2/common/fsm.cc,v 1.12 2005/08/25 18:58:02 johnh Exp $ (LBL)
 */

/*
 * The contents of this file are described in the paper:
 *   Polly Huang and John Heidemann.
 *     Capturing TCP Burstiness in Light-weight Simulations.
 *     In Proceedings of the SCS Conference on Communication Networks and Distributed Systems Modeling and Simulation, pp. 90-96.
 *     Phoenix, Arizona, USA, USC/Information Sciences Institute, Society for Computer Simulation.
 *     January, 2001.
 *     <http://www.isi.edu/~johnh/PAPERS/Huang01a.html>.
 *
 * (Although this code talks about FSMs or Finite-State Machines,
 * the paper uses the term FSA or Finite-State Automoton.)
 */

#include "fsm.h"
#include <assert.h>


#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif


FSM* FSM::instance_;
TahoeAckFSM* TahoeAckFSM::instance_;
RenoAckFSM* RenoAckFSM::instance_;
TahoeDelAckFSM* TahoeDelAckFSM::instance_;
RenoDelAckFSM* RenoDelAckFSM::instance_;



void
FSMState::number_all()
{
	if (processed())
		return;
	static int next_i = 0;
	print_i_ = ++next_i;
	//
	int i;
	for (i = 0; i < 17; i++)
		if (drop_[i])
			drop_[i]->number_all();
}

void
FSMState::reset_all_processed()
{
	if (print_i_ == 0)
		number_all();
	// requires a full traversal always to work
	if (!processed())
		return;
	print_i_ = -print_i_;
	int i;
	for (i = 0; i < 17; i++)
		if (drop_[i])
			drop_[i]->reset_all_processed();
}

void
FSMState::print_all(int level)
{
	if (processed())
		return;

	const int SPACES_PER_LEVEL = 2;
	printf("#%-2d %*s %d:\n", print_i_, level * SPACES_PER_LEVEL + 1, " ", batch_size_);
	int i;
	for (i = 0; i <= batch_size_; i++) {
		static char *delay_names[] = {"done", "error", "RTT", "timeout" };
		assert(transition_[i] >= -1 && transition_[i] <= TIMEOUT);
		printf("   %*s %d %s -> #%d\n", level * SPACES_PER_LEVEL + 3, " ",
		       i,
		       delay_names[transition_[i]+1],
		       drop_[i] ? drop_[i]->print_i_ : 0);
		if (drop_[i])
			drop_[i]->print_all(level + 1);
	};
}

static void
report_stat_terminus(int desired_pkts,	// # needed
			  int pkts,	     	// # got so far
			  int rtts,		// # of rtt events
			  int timeouts,		// # of to events
			  int ps,		// # of times taken a prob. p event (pkt received OK)
			  int qs,		// # of times taken a prob. q event (pkt dropped OK)
			  int num_states,	// size of the stack
			  int num_state_names,
		     FSMState **states,
		     char *state_names)
{
	// print states and probability
	printf("%s: p^%d*q^%d, %d rtt, %d timeouts, %d states:",
	       (pkts > desired_pkts ? "exceeded-pkts" :
	        (pkts == desired_pkts ? "desired_pkts" : "unimplemented-qs")),
	       ps, qs,
	       rtts, timeouts,
	       num_states);
	char ch = ' ';
	int i;
	for (i = 0; i < num_states; i++) {
		printf ("%c#%d", ch, states[i]->print_i_);
		ch = ',';
	};
	printf(" [%.*s]\n", num_state_names, state_names);
}

/*
 * FSMState::print_all_stats:
 * Walk through the tcp state table exhaustively.
 * Recurse to handle errors.
 * Very hairy.
 * johnh.
 */
void
FSMState::print_all_stats(int desired_pkts_total,	// # needed
			  int pkts,	     	// # got so far
			  int rtts,		// # of rtt events
			  int timeouts,		// # of to events
			  int ps,		// # of times taken a prob. p event (pkt received OK)
			  int qs,		// # of times taken a prob. q event (pkt dropped OK)
			  int num_states,	// size of the stack
			  int num_state_names)
{
	int i;
#define LARGER_NUMBER_OF_STATES 31   // was 17
	static FSMState *states[LARGER_NUMBER_OF_STATES];
	static char state_names[LARGER_NUMBER_OF_STATES*4]; // xxx: this is just some random big size :-(

	if (pkts >= desired_pkts_total || qs > 5) {
		// done; print states and probability
		// (give up when we're where we want to be [good],
		// or we've taken too many losses [to prevent recursion])
		report_stat_terminus(desired_pkts_total, pkts, rtts, timeouts, ps, qs, num_states, num_state_names, states, state_names);
		return;
	};

	// remember us!
	states[num_states] = this;
	num_states++;


	// xxx: doesn't handle TCP tail behavior

	//
	// first, consider the no-loss case
	//
	int desired_pkts_remaining = desired_pkts_total - pkts;
	int desired_pkts_this_round = MIN(desired_pkts_remaining, batch_size_);
	for (i = 0; i< desired_pkts_this_round; i++)
		state_names[num_state_names + i] = 's';
	if (desired_pkts_remaining > desired_pkts_this_round) {
		// more to do?  take a rtt hit and keep going
		state_names[num_state_names + desired_pkts_this_round] = '.';
		drop_[0]->print_all_stats(desired_pkts_total,
					  pkts + desired_pkts_this_round,
					  rtts + 1, timeouts,
					  ps + desired_pkts_this_round, qs,
					  num_states,
					  num_state_names + desired_pkts_this_round + 1);
	} else {
		// no more to do... report out
		report_stat_terminus(desired_pkts_total,
				     pkts + desired_pkts_this_round,
				     rtts, timeouts,
				     ps + desired_pkts_this_round, qs,
				     num_states,
				     num_state_names + desired_pkts_this_round,
				     states,
				     state_names);
	};

	//
	// now consider losses
	//
	int desired_pkts_with_loss = MAX(desired_pkts_this_round - 1, 0);
	// loop through losing the i'th packet for all possible i's.
	// Can't loop through more than we could have sent.
	for (i = 1; i <= desired_pkts_this_round; i++) {
		// keep track of sending patterns
		if (i > 1)
			state_names[num_state_names + i - 2] = 's';
		state_names[num_state_names + i - 1] = 'd';
		state_names[num_state_names + desired_pkts_this_round] = (transition_[i] == RTT ? '.' : '-');
		// can we even have any?
		if (qs) {
			// not if we already had one!
			report_stat_terminus(desired_pkts_total,
					     pkts + i - 1,
					     rtts, timeouts,
					     ps + i - 1, qs + 1,
					     num_states,
					     num_state_names + i,
					     states,
					     state_names);
		} else {
			// recurse... assume the rest made it
			drop_[i]->print_all_stats(desired_pkts_total, pkts + desired_pkts_with_loss,
					  rtts + (transition_[i] == RTT ? 1 : 0),
					  timeouts + (transition_[i] == TIMEOUT ? 1 : 0),
					  ps + desired_pkts_with_loss, qs + 1,
					  num_states,
					  num_state_names + desired_pkts_this_round + 1);
			// 2nd drop somewhere in this round?
			int remaining_pkts_this_round = desired_pkts_this_round - i;
			if (qs == 0 && remaining_pkts_this_round > 0) {
				// yes, generate the probs
				int j;
				for (j = i+1; j <= desired_pkts_this_round; j++) {
					if (j > i+1)
						state_names[num_state_names + j - 1] = 's';
					state_names[num_state_names + j] = 'd';
					report_stat_terminus(desired_pkts_total,
							     pkts + j - 2,
							     rtts, timeouts,
							     ps + j - 2, qs + 2,
							     num_states,
							     num_state_names + j,
							     states,
							     state_names);
				};
			};
		};
	};
}


void
FSM::print_FSM(FSMState* state)
{
#if 0
	int i;

	if (state != NULL) {
		for (i=0; i<17; i++) {
			if (state->drop_[i] != NULL) {
				if (i==0) 
					printf("%d->(%d) ", state->transition_[i], state->drop_[i]->batch_size_);
				else
					printf("\n%d->(%d) ", state->transition_[i], state->drop_[i]->batch_size_);
				print_FSM(state->drop_[i]);
			}
		}
	}
#else /* ! 0 */
	state->reset_all_processed();
	state->print_all(0);
#endif /* 0 */
}

void
FSM::print_FSM_stats(FSMState* state, int n)
{
	state->reset_all_processed();
	state->print_all_stats(n);
	fflush(stdout);
}

static class TahoeAckFSMClass : public TclClass {
public:
        TahoeAckFSMClass() : TclClass("FSM/TahoeAck") {}
        TclObject* create(int , const char*const*) {
                return (new TahoeAckFSM);
        }
} class_tahoeackfsm;

static class RenoAckFSMClass : public TclClass {
public:
        RenoAckFSMClass() : TclClass("FSM/RenoAck") {}
        TclObject* create(int , const char*const*) {
                return (new RenoAckFSM);
        }
} class_renoackfsm;

static class TahoeDelAckFSMClass : public TclClass {
public:
        TahoeDelAckFSMClass() : TclClass("FSM/TahoeDelAck") {}
        TclObject* create(int , const char*const*) {
                return (new TahoeDelAckFSM);
        }
} class_tahoedelackfsm;

static class RenoDelAckFSMClass : public TclClass {
public:
        RenoDelAckFSMClass() : TclClass("FSM/RenoDelAck") {}
        TclObject* create(int , const char*const*) {
                return (new RenoDelAckFSM);
        }
} class_renodelackfsm;


// ***********************************************
// Tahoe-Ack TCP Connection Finite State Machine *
// ***********************************************
TahoeAckFSM::TahoeAckFSM() : FSM(), start_state_(NULL) 
{
	int i;
	FSMState* tmp;


	instance_ = this;
	// (wnd, ssh) == (1, 20)
	start_state_ = new FSMState;
	//printf("creating Tahoe Ack FSM\n"); 
	for (i=0; i<17; i++) {
		start_state_->drop_[i] = NULL;
		start_state_->transition_[i] = -1;
	}
	start_state_->batch_size_ = 1;

	// (wnd, ssh) == (2, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0] = tmp;
	start_state_->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->transition_[2] = RTT;

	// (wnd, ssh) == (4, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[2] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[3] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[3] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[4] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[4] = RTT;

	//(wnd, ssh) == (8, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 8;
	start_state_->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[2] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[3] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[4] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 8;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[5] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 10;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[6] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[6] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 12;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[7] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[7] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 14;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[8] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[8] = RTT;

	//(wnd, ssh) == (16, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 16;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	for (i=1; i<17; i++) start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[i] = tmp;
	for (i=1; i<14; i++)
		start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[i] = RTT;
	for (i=14; i<17; i++)
		start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[i] = TIMEOUT;


	//(wnd, ssh) == (1, 2), timeout
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[1] = tmp;
	start_state_->transition_[1] = TIMEOUT;
	start_state_->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->transition_[1] = TIMEOUT;
	start_state_->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[2]->transition_[0] = TIMEOUT;
	start_state_->drop_[0]->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[1] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[2]->transition_[0] = RTT;

	//(wnd, ssh) == (2, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[1]->drop_[0] = tmp;
	start_state_->drop_[1]->transition_[0] = RTT;

	//(wnd, ssh) == (2.5, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[1]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (3, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (4, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (5, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (6, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (7, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 7;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (7, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 7;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;


	//(wnd, ssh) == (1, 3)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[4]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[4]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (1, 4)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[1] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->transition_[0] = 0;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->transition_[0] = RTT;


	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (1, 5)	
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->transition_[0] = 0;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4]->transition_[0] = 0;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (1, 6)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->transition_[0] = 0;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[6]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[6]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (1, 7)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[7]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[7]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[8]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[8]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[7]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[7]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[7]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[7]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//print_FSM(start_state_);
	//printf("\n");
}

// **********************************************
// Reno-ACK TCP Connection Finite State Machine *
// **********************************************
RenoAckFSM::RenoAckFSM() : FSM(), start_state_(NULL) 
{
	int i;
	FSMState* tmp;
	//printf("creating Reno Ack FSM\n");

	instance_ = this;
	// (wnd, ssh) == (1, 20)
	start_state_ = new FSMState;
	for (i=0; i<17; i++) {
		start_state_->drop_[i] = NULL;
		start_state_->transition_[i] = -1;
	}
	start_state_->batch_size_ = 1;

	// (wnd, ssh) == (2, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0] = tmp;
	start_state_->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->transition_[2] = RTT;

	// (wnd, ssh) == (4, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[2] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[3] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[3] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[4] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[4] = RTT;

	//(wnd, ssh) == (8, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 8;
	start_state_->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[3] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[4] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 8;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[5] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 10;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[6] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[6] = RTT;


	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[6]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[6]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 12;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[7] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[7] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 14;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[8] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[8] = RTT;

	//(wnd, ssh) == (16, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 16;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	for (i=1; i<17; i++) start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[i] = tmp;
	for (i=1; i<14; i++)
		start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[i] = RTT;
	for (i=14; i<17; i++)
		start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[i] = TIMEOUT;

	//(wnd, ssh) == (1, 2), timeout
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[1] = tmp;
	start_state_->transition_[1] = TIMEOUT;
	start_state_->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->transition_[1] = TIMEOUT;
	start_state_->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[2]->transition_[0] = TIMEOUT;

	//(wnd, ssh) == (2, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[1]->drop_[0] = tmp;
	start_state_->drop_[1]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[1] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[2]->transition_[0] = RTT;

	//(wnd, ssh) == (2.5, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[1]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (3, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (4, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (5, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (6, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (7, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 7;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (7.5, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 7;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;


	//(wnd, ssh) == (3, 3)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[4]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[4]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;



	//(wnd, ssh) == (4, 4)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[1] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[2] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->transition_[0] = RTT;


	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->transition_[0] = RTT;


	//(wnd, ssh) == (5, 5)	
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->transition_[0] = 0;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4]->transition_[0] = 0;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 7;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (6, 6)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->transition_[0] = 0;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[6]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[6]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 7;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (7, 7)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 7;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[7]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[7]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[8]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[8]->transition_[0] = RTT;

	//print_FSM(start_state_);
	//printf("\n");

}

// *****************************************************
// Tahoe-Delay Ack TCP Connection Finite State Machine *
// *****************************************************
TahoeDelAckFSM::TahoeDelAckFSM() : FSM(), start_state_(NULL) 
{
	int i;
	FSMState* tmp;
	//printf("creating Tahoe DelAck FSM\n");

	instance_ = this;
	// (wnd, ssh) == (1, 20)
	start_state_ = new FSMState;
	for (i=0; i<17; i++) {
		start_state_->drop_[i] = NULL;
		start_state_->transition_[i] = -1;
	}
	start_state_->batch_size_ = 1;

	// (wnd, ssh) == (2, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0] = tmp;
	start_state_->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->transition_[2] = RTT;

	// (wnd, ssh) == (3, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[2] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[3] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[3] = RTT;

	//(wnd, ssh) == (5, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[2] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[3] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[4] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[5] = RTT;

	//(wnd, ssh) == (8, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 8;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[2] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[3] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[3] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[4] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[5] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[5] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 8;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[6] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[6] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 9;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[7] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[7] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 11;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[8] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[8] = RTT;

	//(wnd, ssh) == (12, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 12;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	for (i=1; i<13; i++) start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[i] = tmp;
	for (i=1; i<10; i++)
		start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[i] = RTT;
	for (i=10; i<13; i++)
		start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[i] = TIMEOUT;


	//(wnd, ssh) == (1, 2), timeout
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[1] = tmp;
	start_state_->transition_[1] = TIMEOUT;
	start_state_->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->transition_[1] = TIMEOUT;
	start_state_->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[2]->transition_[0] = TIMEOUT;

	start_state_->drop_[0]->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[1] = TIMEOUT;
	start_state_->drop_[0]->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[2]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->transition_[0] = RTT;

	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[1] = RTT;

	//(wnd, ssh) == (2, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[1]->drop_[0] = tmp;
	start_state_->drop_[1]->transition_[0] = RTT;

	//(wnd, ssh) == (2.5, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[1]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (2.9, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (3, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (3.3, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (4, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (4.7, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (5, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (6, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;


	//(wnd, ssh) == (1, 3)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (1, 4)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[1] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[2]->transition_[0] = 0;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[3]->transition_[0] = 0;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->transition_[0] = RTT;


	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;


	//(wnd, ssh) == (1, 5)	
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->transition_[0] = 0;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[5]->transition_[0] = 0;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[6]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[6]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[7]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[7]->transition_[0] = RTT;


	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;


	//(wnd, ssh) == (1, 6)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[8]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[8]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[8]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[8]->drop_[0]->transition_[0] = RTT;


	//print_FSM(start_state_);
	//printf("\n");
}

// ****************************************************
// Reno-Delay Ack TCP Connection Finite State Machine *
// ****************************************************
RenoDelAckFSM::RenoDelAckFSM() : FSM(), start_state_(NULL) 
{
	int i;
	FSMState* tmp;
	//printf("creating Reno DelAck FSM\n");

	instance_ = this;
	// (wnd, ssh) == (1, 20)
	start_state_ = new FSMState;
	for (i=0; i<17; i++) {
		start_state_->drop_[i] = NULL;
		start_state_->transition_[i] = -1;
	}
	start_state_->batch_size_ = 1;

	// (wnd, ssh) == (2, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0] = tmp;
	start_state_->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->transition_[2] = RTT;

	// (wnd, ssh) == (3, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[2] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[3] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[3] = RTT;

	//(wnd, ssh) == (5, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[2] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[3] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[4] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[5] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->transition_[0] = RTT;

	//(wnd, ssh) == (8, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 8;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[2] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[3] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[3] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[2]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[3]->transition_[0] = RTT;


	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[4] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[5] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[5] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 8;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[6] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[6] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 9;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[7] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[7] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 11;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[8] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[8] = RTT;

	//(wnd, ssh) == (12, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 12;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	for (i=1; i<13; i++) start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[i] = tmp;
	for (i=1; i<10; i++)
		start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[i] = RTT;
	for (i=10; i<13; i++)
		start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[i] = TIMEOUT;


	//(wnd, ssh) == (1, 2), timeout
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[1] = tmp;
	start_state_->transition_[1] = TIMEOUT;
	start_state_->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->transition_[1] = TIMEOUT;
	start_state_->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[2]->transition_[0] = TIMEOUT;

	start_state_->drop_[0]->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[1] = TIMEOUT;
	start_state_->drop_[0]->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[2]->transition_[0] = TIMEOUT;

	//(wnd, ssh) == (2, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[1]->drop_[0] = tmp;
	start_state_->drop_[1]->transition_[0] = RTT;

	//(wnd, ssh) == (2.5, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[1]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (2.9, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (3, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (3.3, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (4, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (4.7, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (5, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (6, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;


	//(wnd, ssh) == (2, 2), rtt
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[1] = RTT;

	//(wnd, ssh) == (2.5, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (3, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (4, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (4.3, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (4.7, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (5, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;


	//(wnd, ssh) == (3.3, 3)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (4, 4)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[1] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->transition_[0] = RTT;


	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->transition_[0] = RTT;


	//(wnd, ssh) == (5, 5)	
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->transition_[0] = 0;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[5]->transition_[0] = 0;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[6]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[6]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[7]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[7]->transition_[0] = RTT;


	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (6, 6)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[8]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[8]->transition_[0] = RTT;

	//print_FSM(start_state_);
	//printf("\n");
}

