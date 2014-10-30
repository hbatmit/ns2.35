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

/* Token Bucket filter which has  3 parameters :
   a. Token Generation rate
   b. Token bucket depth
   c. Max. Queue Length (a finite length would allow this to be used as  policer as packets are dropped after queue gets full)
   */

#include "connector.h" 
#include "packet.h"
#include "queue.h"
#include "tbf.h"

TBF::TBF() :tokens_(0),tbf_timer_(this), init_(1)
{
	q_=new PacketQueue();
	bind_bw("rate_",&rate_);
	bind("bucket_",&bucket_);
	bind("qlen_",&qlen_);
}
	
TBF::~TBF()
{
	if (q_->length() != 0) {
		//Clear all pending timers
		tbf_timer_.cancel();
		//Free up the packetqueue
		for (Packet *p=q_->head();p!=0;p=p->next_) 
			Packet::free(p);
	}
	delete q_;
}


void TBF::recv(Packet *p, Handler *)
{
	//start with a full bucket
	if (init_) {
		tokens_=bucket_;
		lastupdatetime_ = Scheduler::instance().clock();
		init_=0;
	}

	
	hdr_cmn *ch=hdr_cmn::access(p);

	//enque packets appropriately if a non-zero q already exists
	if (q_->length() !=0) {
		if (q_->length() < qlen_) {
			q_->enque(p);
			return;
		}

		drop(p);
		return;
	}

	getupdatedtokens();
	int pktsize = ch->size()<<3;
	if (tokens_ >=pktsize) {
		target_->recv(p);
		tokens_-=pktsize;
	}
	else {
		
		if (qlen_!=0) {
			q_->enque(p);
			tbf_timer_.resched((pktsize-tokens_)/rate_);
		}
		else {
			drop(p);
		}
	}
}

double TBF::getupdatedtokens(void)
{
	double now=Scheduler::instance().clock();
	
	tokens_ += (now-lastupdatetime_)*rate_;
	if (tokens_ > bucket_)
		tokens_=bucket_;
	lastupdatetime_ = Scheduler::instance().clock();
	return tokens_;
}

void TBF::timeout(int)
{
	if (q_->length() == 0) {
		fprintf (stderr,"ERROR in tbf\n");
		abort();
	}
	
	Packet *p=q_->deque();
	getupdatedtokens();
	hdr_cmn *ch=hdr_cmn::access(p);
	int pktsize = ch->size()<<3;

	//We simply send the packet here without checking if we have enough tokens
	//because the timer is supposed to fire at the right time
	target_->recv(p);
	tokens_-=pktsize;

	if (q_->length() !=0 ) {
		p=q_->head();
		hdr_cmn *ch=hdr_cmn::access(p);
		pktsize = ch->size()<<3;
		tbf_timer_.resched((pktsize-tokens_)/rate_);
	}
}

void TBF_Timer::expire(Event* /*e*/)
{
	tbf_->timeout(0);
}


static class TBFClass : public TclClass {
public:
	TBFClass() : TclClass ("TBF") {}
	TclObject* create(int,const char*const*) {
		return (new TBF());
	}
}class_tbf;
