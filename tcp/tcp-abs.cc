/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * Copyright (C) 1999 by the University of Southern California
 * $Id: tcp-abs.cc,v 1.3 2005/08/25 18:58:12 johnh Exp $
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
 * @(#) $Header: /cvsroot/nsnam/ns-2/tcp/tcp-abs.cc,v 1.3 2005/08/25 18:58:12 johnh Exp $ (LBL)";
 */

#include "ip.h"
#include "tcp.h"
#include "tcp-abs.h"

//AbsTcp
AbsTcpAgent::AbsTcpAgent() : Agent(PT_TCP), rtt_(0), current_(NULL),offset_(0), seqno_lb_(-1), connection_size_(0), timer_(this), rescheduled_(0)
{
	size_ = 1000;
}

void AbsTcpAgent::timeout()
{
	if (rescheduled_ == 0 && current_->transition_[offset_]!= current_->transition_[0]) {
		set_timer(2*rtt_);
		rescheduled_ = 1;
	} else {
		rescheduled_ = 0;
		seqno_lb_ += current_->batch_size_;
		if (current_->drop_[offset_] == NULL) {
			printf("Error: This fsm can't handle multi losses per connection\n");
			exit(0);
		}
		current_ = current_->drop_[offset_];
		send_batch();
	}
}

void AbsTcpAgent::sendmsg(int pktcnt)
{
	connection_size_ = pktcnt;
	start();
}

void AbsTcpAgent::advanceby(int pktcnt)
{
	connection_size_ = pktcnt;
	start();
}

void AbsTcpAgent::start()
{
	//printf("starting fsm tcp, %d\n", connection_size_);
	send_batch();
}

void AbsTcpAgent::send_batch() 
{
	int seqno = seqno_lb_;
	
	offset_ = 0;
	//printf("sending batch, %d\n", current_->batch_size_);
	for (int i=0; i<current_->batch_size_ && seqno < connection_size_-1; i++) {
		seqno++;
		output(seqno);
	}
	if (seqno == connection_size_-1) {
		finish();
	}
	else if (seqno < connection_size_-1) {
		if (current_->drop_[offset_] == NULL) {
			printf("Error: current fsm can't handle this tcp connection flow id %d (possibly too long)\n", fid_);
			exit(0);
		} 
		//printf("start timer %d\n", current_->transition_[offset_]);
		if (current_->transition_[offset_] == 0) {
			current_ = current_->drop_[offset_];
			send_batch();
		} else if (current_->transition_[offset_] == RTT) {
			set_timer(rtt_);
		} else if (current_->transition_[offset_] == TIMEOUT) {
			set_timer(rtt_ * 3);
		} else {
			printf("Error: weird transition timer\n");
			exit(0);
		}
	} else {
		printf("Error: sending more than %d packets\n", connection_size_);
		exit(0);
	}
}

void AbsTcpAgent::drop(int seqno)
{
	//printf("dropped: %d\n", seqno);
	if (offset_ != 0) {
		printf("Error: Sorry, can't handle multiple drops per batch\n");
		exit(0);
	}
	offset_ = seqno - seqno_lb_;
	connection_size_++;
}

void AbsTcpAgent::finish()
{
	//printf("finish: sent %d\n", seqno_lb_+1);
	cancel_timer();
}

void AbsTcpAgent::output(int seqno)
{
        Packet* p = allocpkt();
        hdr_tcp *tcph = hdr_tcp::access(p);
        tcph->seqno() = seqno;
        send(p, 0);
}

void AbsTcpAgent::recv(Packet* pkt, Handler*)
{
	Packet::free(pkt);
}

int AbsTcpAgent::command(int argc, const char*const* argv)
{
        if (argc == 3 ) {
                if (strcmp(argv[1], "rtt") == 0) {
			rtt_ = atof(argv[2]);
			//printf("rtt %f\n", rtt_);
			return (TCL_OK);
		}
                if (strcmp(argv[1], "advance") == 0) {
			advanceby(atoi(argv[2]));
			return (TCL_OK);
                }
                if (strcmp(argv[1], "advanceby") == 0) {
                        advanceby(atoi(argv[2]));
                        return (TCL_OK);
                }
		if(strcmp(argv[1], "print-stats") == 0) {
			// xxx: works best if invoked on a new fsm
			// (otherwise you don't get the whole thing).
			int n = atoi(argv[2]);
			if (n < 0 || n >= 17)
				return TCL_ERROR;
			FSM::print_FSM_stats(current_, n);
                        return (TCL_OK);
		};
	} else if (argc == 2) {
                if (strcmp(argv[1], "print") == 0) {
			// xxx: works best if invoked on a new fsm
			// (otherwise you don't get the whole thing).
			FSM::print_FSM(current_);
                        return (TCL_OK);
		};
	};
	return (Agent::command(argc, argv));
}

void AbsTcpTimer::expire(Event*)
{
        a_->timeout();
}


//AbsTCP/TahoeAck
static class AbsTcpTahoeAckClass : public TclClass {
public:
	AbsTcpTahoeAckClass() : TclClass("Agent/AbsTCP/TahoeAck") {}
	TclObject* create(int, const char*const*) {
		return (new AbsTcpTahoeAckAgent());
	}
} class_abstcptahoeack;

AbsTcpTahoeAckAgent::AbsTcpTahoeAckAgent() : AbsTcpAgent()
{
	size_ = 1000;
	current_ = TahoeAckFSM::instance().start_state();
	DropTargetAgent::instance().insert_tcp(this);
}


//AbsTCP/RenoAck
static class AbsTcpRenoAckClass : public TclClass {
public:
	AbsTcpRenoAckClass() : TclClass("Agent/AbsTCP/RenoAck") {}
	TclObject* create(int, const char*const*) {
		return (new AbsTcpRenoAckAgent());
	}
} class_abstcprenoack;

AbsTcpRenoAckAgent::AbsTcpRenoAckAgent() : AbsTcpAgent()
{
	size_ = 1000;
	current_ = RenoAckFSM::instance().start_state();
	DropTargetAgent::instance().insert_tcp(this);
}


//AbsTCP/TahoeDelAck
static class AbsTcpTahoeDelAckClass : public TclClass {
public:
	AbsTcpTahoeDelAckClass() : TclClass("Agent/AbsTCP/TahoeDelAck") {}
	TclObject* create(int, const char*const*) {
		return (new AbsTcpTahoeDelAckAgent());
	}
} class_abstcptahoedelack;

AbsTcpTahoeDelAckAgent::AbsTcpTahoeDelAckAgent() : AbsTcpAgent()
{
	size_ = 1000;
	current_ = TahoeDelAckFSM::instance().start_state();
	DropTargetAgent::instance().insert_tcp(this);
}


//AbsTCP/RenoDelAck
static class AbsTcpRenoDelAckClass : public TclClass {
public:
	AbsTcpRenoDelAckClass() : TclClass("Agent/AbsTCP/RenoDelAck") {}
	TclObject* create(int, const char*const*) {
		return (new AbsTcpRenoDelAckAgent());
	}
} class_abstcprenodelack;

AbsTcpRenoDelAckAgent::AbsTcpRenoDelAckAgent() : AbsTcpAgent()
{
	size_ = 1000;
	current_ = RenoDelAckFSM::instance().start_state();
	DropTargetAgent::instance().insert_tcp(this);
}


//AbsTcpSink
static class AbsTcpSinkClass : public TclClass {
public:
        AbsTcpSinkClass() : TclClass("Agent/AbsTCPSink") {}
        TclObject* create(int, const char*const*) {
                return (new AbsTcpSink());
        }
} class_abstcpsink;

AbsTcpSink::AbsTcpSink() : Agent(PT_ACK)
{
	size_ = 40;
}

void AbsTcpSink::recv(Packet* pkt, Handler*)
{
	Packet* p = allocpkt();
	send(p, 0);
        Packet::free(pkt);
}

static class AbsDelAckSinkClass : public TclClass {
public:
        AbsDelAckSinkClass() : TclClass("Agent/AbsTCPSink/DelAck") {}
        TclObject* create(int, const char*const*) {
                return (new AbsDelAckSink());
        }
} class_absdelacksink;

AbsDelAckSink::AbsDelAckSink() : AbsTcpSink(), delay_timer_(this)
{
	size_ = 40;
	interval_ = 0.1;
}

void AbsDelAckSink::recv(Packet* pkt, Handler*)
{
        if (delay_timer_.status() != TIMER_PENDING) {
		delay_timer_.resched(interval_);
	} else {
                delay_timer_.cancel();
		Packet* p = allocpkt();
		send(p, 0);
	}
        Packet::free(pkt);
}

void AbsDelAckSink::timeout()
{
        /*
         * The timer expired so we ACK the last packet seen.
         * (shouldn't this check for a particular time out#?  -kf)
         */
	Packet* p = allocpkt();
	send(p, 0);
}

void AbsDelayTimer::expire(Event */*e*/) {
        a_->timeout();
}

//Special drop target agent
DropTargetAgent* DropTargetAgent::instance_;

static class DropTargetClass : public TclClass {
public:
        DropTargetClass() : TclClass("DropTargetAgent") {}
        TclObject* create(int, const char*const*) {
                return (new DropTargetAgent());
        }
} class_droptarget;

DropTargetAgent::DropTargetAgent(): Connector(), dropper_list_(NULL)
{
	instance_ = this;
}

void DropTargetAgent::recv(Packet* pkt, Handler*)
{
	Dropper* tmp = dropper_list_;

        hdr_tcp *tcph = hdr_tcp::access(pkt);
        hdr_ip *iph = hdr_ip::access(pkt);
        //printf("flow %d dropping seqno %d\n", iph->flowid(),tcph->seqno());
	while(tmp != NULL) {
		if(tmp->agent_->flowid() == iph->flowid())
			tmp->agent_->drop(tcph->seqno());
		tmp = tmp->next_;
	}
	Packet::free(pkt);
}

void DropTargetAgent::insert_tcp(AbsTcpAgent* tcp)
{
	Dropper* dppr = new Dropper;
	dppr->agent_=tcp;
	dppr->next_ = dropper_list_;
	dropper_list_ = dppr;
}

