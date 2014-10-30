/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * srm.cc
 * Copyright (C) 1997 by the University of Southern California
 * $Id: srm.cc,v 1.27 2005/08/25 18:58:08 johnh Exp $
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

//
//	Maintainer:	Kannan Varadhan	<kannan@isi.edu>
//	Version Date:	Tue Jul 22 15:41:16 PDT 1997
//

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/mcast/srm.cc,v 1.27 2005/08/25 18:58:08 johnh Exp $ (USC/ISI)";
#endif

#include <stdlib.h>
#include <assert.h>

#include "config.h"
#include "agent.h"
#include "ip.h"
#include "srm.h"
#include "trace.h"
#include "rtp.h"


int hdr_srm::offset_;
int hdr_asrm::offset_;

static class SRMHeaderClass : public PacketHeaderClass {
public:
	SRMHeaderClass() : PacketHeaderClass("PacketHeader/SRM",
					     sizeof(hdr_srm)) {
		bind_offset(&hdr_srm::offset_);
	}
} class_srmhdr;

static class ASRMHeaderClass : public PacketHeaderClass {
public:
	ASRMHeaderClass() : PacketHeaderClass("PacketHeader/aSRM",
					      sizeof(hdr_asrm)) {
		bind_offset(&hdr_asrm::offset_);
	}
} class_adaptive_srmhdr;


static class SRMAgentClass : public TclClass {
public:
	SRMAgentClass() : TclClass("Agent/SRM") {}
	TclObject* create(int, const char*const*) {
		return (new SRMAgent());
	}
} class_srm_agent;

static class ASRMAgentClass : public TclClass {
public:
	ASRMAgentClass() : TclClass("Agent/SRM/Adaptive") {}
	TclObject* create(int, const char*const*) {
		return (new ASRMAgent());
	}
} class_adaptive_srm_agent;


SRMAgent::SRMAgent() 
	: Agent(PT_SRM), dataCtr_(-1), sessCtr_(-1), siphash_(0), seqno_(-1),
    app_type_(PT_NTYPE)
{
	sip_ = new SRMinfo(-1);

	bind("packetSize_", &packetSize_);
	bind("groupSize_", &groupSize_);
	bind("app_fid_", &app_fid_);
}

SRMAgent::~SRMAgent()
{
	cleanup();
}

int SRMAgent::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();

	if (strcmp(argv[1], "send") == 0) {
		if (strcmp(argv[2], "session") == 0) {
			send_sess();
			return TCL_OK;
		}
		if (strcmp(argv[2], "request") == 0) {
			int round = atoi(argv[3]);
			int sender = atoi(argv[4]);
			int msgid  = atoi(argv[5]);
			send_ctrl(SRM_RQST, round, sender, msgid, 0);
			return TCL_OK;
		}
		if (strcmp(argv[2], "repair") == 0) {
			int round = atoi(argv[3]);
			int sender = atoi(argv[4]);
			int msgid  = atoi(argv[5]);
			send_ctrl(SRM_REPR, round, sender, msgid, packetSize_);
			return TCL_OK;
		}
		tcl.resultf("%s: invalid send request %s", name_, argv[2]);
		return TCL_ERROR;
	}
	if (argc == 2) {
		if (strcmp(argv[1], "distances?") == 0) {
			tcl.result("");
			if (sip_->sender_ >= 0) {  // i.e. this agent is active
				for (SRMinfo* sp = sip_; sp; sp = sp->next_) {
					tcl.resultf("%s %d %f", tcl.result(),
						    sp->sender_,
						    sp->distance_);
				}
			}
			return TCL_OK;
		}
		if (strcmp(argv[1], "start") == 0) {
			start();
			return TCL_OK;
		}
	}
	if (argc == 3) {
		if (strcmp(argv[1], "distance?") == 0) {
			int sender = atoi(argv[2]);
			SRMinfo* sp = get_state(sender);
			tcl.resultf("%lf", sp->distance_);
			return TCL_OK;
		}
    	if (strcmp(argv[1], "eventtrace") == 0) {
      		return (TCL_OK);
    	}
	}
	return Agent::command(argc, argv);
}

void SRMAgent::recv(Packet* p, Handler* h)
{
	hdr_ip*  ih = hdr_ip::access(p);
	hdr_srm* sh = hdr_srm::access(p);
	
	if (ih->daddr() == -1) {
		// Packet from local agent.  Add srm headers, set dst, and fwd
		sh->type() = SRM_DATA;
		sh->sender() = addr();
		sh->seqnum() = ++dataCtr_;
		addExtendedHeaders(p);
		ih->dst() = dst_;
		target_->recv(p, h);
	} else {

#if 0
 		static char *foo[] = {"NONE", "DATA", "SESS", "RQST", "REPR"};
 		fprintf(stderr, "%7.4f %s %d recvd SRM_%s <%d, %d> from %d\n",
			Scheduler::instance().clock(), name_, addr_,
			foo[sh->type()],
			sh->sender(), sh->seqnum(), ih->src());
#endif
		
		parseExtendedHeaders(p);
		switch (sh->type()) {
		case SRM_DATA:
			recv_data(sh->sender(), sh->seqnum(), p->accessdata());
			break;
		case SRM_RQST:
			recv_rqst(ih->saddr(),
				  sh->round(), sh->sender(), sh->seqnum());
			break;
		case SRM_REPR:
			recv_repr(sh->round(), sh->sender(), sh->seqnum(),
				  p->accessdata());
			break;
		case SRM_SESS:
			// This seqnum() is the session sequence number,
			// not the data packet sequence numbers seen before.
			recv_sess(p, sh->seqnum(), (int*) p->accessdata());
			break;
		}
		Packet::free(p);
	}
}

void SRMAgent::sendmsg(int nbytes, const char* /*flags*/)
{
	if (nbytes == -1) {
		printf("Error:  sendmsg() for SRM should not be -1\n");
		return;
	}
	// The traffic generator may have reset our payload type when it
	// initialized.  If so, save the current payload type as app_type_,
	// and set type_ to PT_SRM.  Use app_type_ for all app. packets
	// 
	if (type_ != PT_SRM) {
		app_type_ = type_;
		type_ = PT_SRM;
	}
	size_ = nbytes;
	Packet *p;
	p = allocpkt();
        hdr_ip*  ih = hdr_ip::access(p);
        hdr_srm* sh = hdr_srm::access(p);
	hdr_rtp* rh = hdr_rtp::access(p);
	hdr_cmn* ch = hdr_cmn::access(p);
	//hdr_cmn* ch = hdr_cmn::access(p);
	
	ch->ptype() = app_type_;
	ch->size() =  size_;
	ih->flowid() = app_fid_;
	rh->seqno() = ++seqno_;
	// Add srm headers, set dst, and fwd
	sh->type() = SRM_DATA;
	sh->sender() = addr();
	sh->seqnum() = ++dataCtr_;
	addExtendedHeaders(p);
	ih->dst() = dst_;
	target_->recv(p);
}


void SRMAgent::send_ctrl(int type, int round, int sender, int msgid, int size)
{
	Packet* p = Agent::allocpkt();
	hdr_srm* sh = hdr_srm::access(p);
	sh->type() = type;
	sh->sender() = sender;
	sh->seqnum() = msgid;
	sh->round() = round;
	addExtendedHeaders(p);

	hdr_cmn* ch = hdr_cmn::access(p);
	ch->size() = sizeof(hdr_srm) + size;
	target_->recv(p);
}

void SRMAgent::recv_data(int sender, int msgid, u_char*)
{
	Tcl& tcl = Tcl::instance();
	SRMinfo* sp = get_state(sender);
	if (msgid > sp->ldata_) {
		(void) request(sp, msgid - 1);
		sp->setReceived(msgid);
		sp->ldata_ = msgid;
	} else {
		tcl.evalf("%s recv data %d %d", name_, sender, msgid);
	}
}

void SRMAgent::recv_rqst(int requestor, int round, int sender, int msgid)
{
	Tcl& tcl = Tcl::instance();
	SRMinfo* sp = get_state(sender);
	if (msgid > sp->ldata_) {
		(void) request(sp, msgid);	// request upto msgid
		sp->ldata_ = msgid;
	} else {
		tcl.evalf("%s recv request %d %d %d %d", name_,
			  requestor, round, sender, msgid);
	}
}

void SRMAgent::recv_repr(int round, int sender, int msgid, u_char*)
{
	Tcl& tcl = Tcl::instance();
	SRMinfo* sp = get_state(sender);
	if (msgid > sp->ldata_) {
		(void) request(sp, msgid - 1);	// request upto msgid - 1
		sp->setReceived(msgid);
		sp->ldata_ = msgid;
	} else {
		tcl.evalf("%s recv repair %d %d %d", name_,
			  round, sender, msgid);
	}
	// Notice that we currently make no provisions for a listener
	// agent to receive the data.
}

void SRMAgent::send_sess()
{
	int	size = (1 + groupSize_ * 4) * sizeof(int);
	Packet* p = Agent::allocpkt(size);
	hdr_srm* sh = hdr_srm::access(p);
	sh->type() = SRM_SESS;
	sh->sender() = addr();
	sh->seqnum() = ++sessCtr_;
	addExtendedHeaders(p);

	int* data = (int*) p->accessdata();
	*data++ = groupSize_;
	for (SRMinfo* sp = sip_; sp; sp = sp->next_) {
		*data++ = sp->sender_;
		*data++ = sp->ldata_;
		*data++ = sp->recvTime_;
		*data++ = sp->sendTime_;
	}
	data = (int*) p->accessdata();
	data[4] = (int) (Scheduler::instance().clock()*1000);

	hdr_cmn* ch = hdr_cmn::access(p);
	ch->size() = size+ sizeof(hdr_srm);

	target_->recv(p, (Handler*)NULL);
}

#define	GET_SESSION_INFO			\
	sender = *data++;			\
	dataCnt = *data++;			\
	rtime = *data++;			\
	stime = *data++

void SRMAgent::recv_sess(Packet*, int sessCtr, int* data)
{
	SRMinfo* sp;
	
	int sender, dataCnt, rtime, stime;
	int now, sentAt, sentBy;
	int cnt = *data++;
	int i;

	/* The first block contains the sender's own state */
	GET_SESSION_INFO;
	if (sender == addr())			// sender's own session message
		return;

	sp = get_state(sender);
	if (sp->lsess_ > sessCtr)		// older session message recd.
		return;
	
	now = (int) (Scheduler::instance().clock() * 1000);
	sentBy = sender;			// to later compute rtt
	sentAt = stime;
	
	sp->lsess_ = sessCtr;
	sp->recvTime_ = now;
	sp->sendTime_ = stime;
	(void) request(sp, dataCnt);
	if (sp->ldata_ < dataCnt)
		sp->ldata_ = dataCnt;
	
	for (i = 1; i < cnt; i++) {
		GET_SESSION_INFO;
		if (sender == addr() && now) {
			//
			//    This session message from sender sentBy:
			//		  vvvvv
			//	    now	<=======+ sentAt
			//		 |     |
			//	  stime	+=======> rtime
			//		  ^^^^^
			//   Earlier session message sent by ``this'' agent
			//
                        int rtt = (now - sentAt) + (rtime - stime);
			sp = get_state(sentBy);
			sp->distance_ = (double) rtt / 2 / 1000;
#if 0
			fprintf(stderr,
				"%7.4f %s compute distance to %d: %f\n",
				Scheduler::instance().clock(), name_,
				sentBy, sp->distance_);
#endif
			continue;
		}
		sp = get_state(sender);
		(void) request(sp, dataCnt);
		if (sp->ldata_ < dataCnt)
			sp->ldata_ = dataCnt;
	}
}
