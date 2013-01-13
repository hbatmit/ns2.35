/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * srm-ssm.cc
 * Copyright (C) 1997 by the University of Southern California
 * $Id: srm-ssm.cc,v 1.11 2005/08/25 18:58:08 johnh Exp $
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
// The code implements scalable session message. See
// http://catarina.usc.edu/estrin/papers/infocom98/ssession.ps


#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/mcast/srm-ssm.cc,v 1.11 2005/08/25 18:58:08 johnh Exp $ (USC/ISI)";
#endif

#include <stdlib.h>
#include <assert.h>

#include <stdio.h>

#include "config.h"
#include "tclcl.h"
#include "agent.h"
#include "packet.h"
#include "ip.h"
#include "srm.h"
#include "srm-ssm.h"
#include "trace.h"

int hdr_srm_ext::offset_;
static class SRMEXTHeaderClass : public PacketHeaderClass {
public:
	SRMEXTHeaderClass() : PacketHeaderClass("PacketHeader/SRMEXT",
						sizeof(hdr_srm_ext)) {
		bind_offset(&hdr_srm_ext::offset_);
	}
} class_srmexthdr;

static class SSMSRMAgentClass : public TclClass {
public:
  SSMSRMAgentClass() : TclClass("Agent/SRM/SSM") {}
  TclObject* create(int, const char*const*) {
    return (new SSMSRMAgent());
  }
} class_srm_ssm_agent;


SSMSRMAgent::SSMSRMAgent() 
  : SRMAgent(), glb_sessCtr_(-1), loc_sessCtr_(-1), rep_sessCtr_(-1)
{
  bind("group_scope_",&groupScope_);
  bind("local_scope_",&localScope_);
  bind("scope_flag_",&scopeFlag_);
  bind("rep_id_", &repid_);
}

int SSMSRMAgent::command(int argc, const char*const* argv)
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
    /*
      #if 0
      fprintf(stdout,"%s: send request %s passed to srm_agent", 
      name_, argv[2]);
      #endif
      return SRMAgent::command(argc, argv);
      */
  }

  if (argc == 2) {
    if (strcmp(argv[1], "start") == 0) {
      sip_->sender_ = addr();
      sip_->distance_ = 0.0;
      /* sip_->repid_ = addr_;
	 sip_->scopeFlag_ = SRM_GLOBAL;
	 repid_ = addr_;
	 scopeFlag_ = SRM_GLOBAL;
	 */		  
      groupScope_ = 32;
      senderFlag_ = 0;
      printf("%s is %d and rep-status %d\n", name_, addr(), scopeFlag_);
      return TCL_OK;
    }
    if (strcmp(argv[1], "ch-rep") == 0) {
      if(scopeFlag_ == SRM_GLOBAL) {
	sip_->repid_ = repid_ = addr();
	sip_->scopeFlag_ = SRM_GLOBAL;
      } else {
	sip_->repid_ = repid_;
	sip_->scopeFlag_ = SRM_LOCAL;
      }    
      return TCL_OK;
    }
    if (strcmp(argv[1], "distances?") == 0) {
      if (sip_->sender_ < 0) { // i.e. this agent is not
	tcl.result("");	 //      yet active.
	return TCL_OK;
      }
      for (SRMinfo* sp = sip_; sp; sp = sp->next_) {
	if((sp->distanceFlag_ == REP_DISTANCE) || 
	   (sp->distanceFlag_ == SELF_DISTANCE)) {
	  
	  tcl.resultf("%s %d %f", tcl.result(),
		      sp->sender_, sp->distance_);
	} else { /* Return reps distance */
	  SRMinfo* rsp = get_state(sp->repid_);
	  tcl.resultf("%s %d %f", tcl.result(),
		      sp->sender_, rsp->distance_);
	}
      }
      return TCL_OK;
    }
    
  }
  if (argc == 3) {
    if (strcmp(argv[1], "distance?") == 0) {
      int sender = atoi(argv[2]);
      SRMinfo* sp = get_state(sender);
      if((sp->distanceFlag_ == REP_DISTANCE) || 
	 (sp->distanceFlag_ == SELF_DISTANCE)) {
	tcl.resultf("%f", sp->distance_);
      } else { /* Return reps distance */
	SRMinfo* rsp = get_state(sp->repid_);
	tcl.resultf("%f", rsp->distance_);
      }
      return TCL_OK;
    }
  }
  return SRMAgent::command(argc, argv);
}

void SSMSRMAgent::recv(Packet* p, Handler* h)
{
  hdr_ip*  ih = hdr_ip::access(p);
  hdr_srm* sh = hdr_srm::access(p);
  hdr_srm_ext* seh = hdr_srm_ext::access(p);
	
  if (ih->daddr() == 0) {
    // Packet from local agent.  Add srm headers, set dst, and fwd
    sh->type() = SRM_DATA;
    sh->sender() = addr();
    sh->seqnum() = ++dataCtr_;
    seh->repid() = repid_;
    ih->dst() = dst_;
    ih->src() = here_;
    target_->recv(p, h);
  } else {

#if 0
    static char *foo[] = {"NONE", "DATA", "SESS", "RQST", "REPR"};
    fprintf(stdout, "%7.4f %s %d  recvd SRM_%s <%d, %d> from %d\n",
	    Scheduler::instance().clock(), name_, addr_, foo[sh->type()],
	    sh->sender(), sh->seqnum(), ih->src());
    fflush(stdout);
#endif
		
    switch (sh->type()) {
    case SRM_DATA:
      recv_data(sh->sender(), sh->seqnum(), seh->repid(), p->accessdata());
      Packet::free(p);
      break;
    case SRM_RQST:
      recv_rqst(ih->saddr(), sh->round(), sh->sender(), sh->seqnum(),
		seh->repid());  
      Packet::free(p);
      break;
    case SRM_REPR:
      recv_repr(sh->round(), sh->sender(), sh->seqnum(), p->accessdata());
      Packet::free(p);
      break;
    case SRM_SESS:
      // This seqnum() is the session sequence number,
      // not the data packet sequence numbers seen before.
      // Send the whole pkt for ttl etc..
      recv_sess(sh->seqnum(), (int*) p->accessdata(), p);
      break;
    }
  }
}


void SSMSRMAgent::recv_data(int sender, int id, int repid, u_char* data)
{
  SRMinfo* sp = get_state(sender);
  /* Just store the repid and call srmagent recv_data */
  sp->repid_ = repid;
  SRMAgent::recv_data(sender,id,data);
}

void SSMSRMAgent::send_ctrl(int type, int round, int sender, int msgid, int size)
{
  Packet* p = Agent::allocpkt();
  hdr_srm* sh = hdr_srm::access(p);
  hdr_srm_ext* seh = hdr_srm_ext::access(p);
  sh->type() = type;
  sh->sender() = sender;
  sh->seqnum() = msgid;	
  sh->round() = round;
  seh->repid() = repid_;  /* For ctrl messages this is your own repid */

  hdr_cmn* ch = hdr_cmn::access(p);
  ch->size() = sizeof(hdr_srm) + size;
  target_->recv(p, (Handler*)NULL);
}

void SSMSRMAgent::recv_rqst(int requestor, int round, int sender, 
			    int msgid, int repid)
{
  //Tcl& tcl = Tcl::instance();
  SRMinfo* rsp = get_state(requestor);
  rsp->repid_ = repid;
  SRMAgent::recv_rqst(requestor,round, sender,msgid);
}







void SSMSRMAgent::send_sess() 
{
  if (scopeFlag_ == SRM_GLOBAL) {
    send_glb_sess();
    send_rep_sess();
  } else {
    send_loc_sess();
  }
//   timeout_info();
}

#define SESSINFO_SIZE   5
#define SESS_CONST      2

void SSMSRMAgent::send_glb_sess()
{
	int size = (SESS_CONST + groupSize_ * SESSINFO_SIZE) * sizeof(int);  
	/* Currently do extra allocation, later change */
	int     num_entries;
        Packet* p = Agent::allocpkt(size);
        hdr_srm* sh = hdr_srm::access(p);
        hdr_srm_ext* seh = hdr_srm_ext::access(p);

#if 0
	printf("sending global session message\n");
#endif
        sh->type() = SRM_SESS;
        sh->sender() = addr();
        sh->seqnum() = ++glb_sessCtr_;
	seh->repid() = repid_;

        int* data = (int*) p->accessdata();
	*data++ = groupSize_;
	*data++ = SRM_GLOBAL;
	num_entries = 0;
	for (SRMinfo* sp = sip_; sp; sp = sp->next_) {
	  /* Global Session Message has information about Senders/reps */
           if ((sp->senderFlag_ || 
		(sp->scopeFlag_ == SRM_GLOBAL) ||
		(sp->sender_ == addr()))
	       && (is_active(sp))) {      
	        *data++ = sp->sender_;
                *data++ = sp->ldata_;
                *data++ = sp->recvTime_;
                *data++ = sp->sendTime_;
		*data++ = sp->repid_;
		num_entries++;
	      }
        }
	data = (int*) p->accessdata();
	data[0] = num_entries;
	data[1] = SRM_GLOBAL;
	size = (SESS_CONST + num_entries * SESSINFO_SIZE) * sizeof(int);
	data[5] = (int) (Scheduler::instance().clock()*1000);

	hdr_cmn* ch = hdr_cmn::access(p);
        ch->size() += size+ sizeof(hdr_srm); /* Add size of srm_hdr_ext */
        hdr_ip*  ih = hdr_ip::access(p);
	ih->ttl() = groupScope_;
	// Currently put this to distinguish various session messages
	ih->flowid() = SRM_GLOBAL;
	seh->ottl() = groupScope_;
        target_->recv(p, (Handler*)NULL);
}


void SSMSRMAgent::send_loc_sess()
{
	int size = (SESS_CONST + groupSize_ * SESSINFO_SIZE) * sizeof(int);  
	/* Currently do extra allocation, later change */
	int     num_entries;
        Packet* p = Agent::allocpkt(size);
        hdr_srm* sh = hdr_srm::access(p);
        hdr_srm_ext* seh = hdr_srm_ext::access(p);
        sh->type() = SRM_SESS;
        sh->sender() = addr();
        sh->seqnum() = ++loc_sessCtr_;
	seh->repid() = repid_;
#if 0
	printf("sending local session message\n");
#endif
        int* data = (int*) p->accessdata();
	//int* tmp_data = (int*) p->accessdata();
	*data++ = groupSize_;
	*data++ = SRM_LOCAL;
	num_entries = 0;
	for (SRMinfo* sp = sip_; sp; sp = sp->next_) {
	  /* Local Session Message has information 
	     about Senders/other locals */
	  if ((sp->senderFlag_ || 
	       (sp->scopeFlag_ == SRM_LOCAL) ||
	       (sp->distanceFlag_ = SELF_DISTANCE) || 
	       /* For the reps that I am hearing from */
	       (sp->sender_ == addr()) ||   
	       // just in case, I have not set the flags properly, 
	       // one entry has to be there
	       (repid_ == sp->sender_)) 
	      && (is_active(sp))) {      
	    *data++ = sp->sender_;
	    *data++ = sp->ldata_;
	    *data++ = sp->recvTime_;
	    *data++ = sp->sendTime_;
	    *data++ = sp->repid_;
	    num_entries++;
	  }
        }
	data = (int*) p->accessdata();
	data[0] = num_entries;
	data[1] = SRM_LOCAL;
	size = (SESS_CONST + num_entries * SESSINFO_SIZE) * sizeof(int);
	data[5] = (int) (Scheduler::instance().clock()*1000);

	hdr_cmn* ch = hdr_cmn::access(p);
        ch->size() += size+ sizeof(hdr_srm);
        hdr_ip*  ih = hdr_ip::access(p);
	ih->ttl() = localScope_;
	// Currently put this to distinguish various session messages
	ih->flowid() = SRM_LOCAL;
	seh->ottl() = localScope_;
        target_->recv(p, (Handler*)NULL);
}


void SSMSRMAgent::send_rep_sess()
{
	int size = (SESS_CONST + groupSize_ * SESSINFO_SIZE) * sizeof(int);  
	/* Currently do extra allocation, later change */
	int     num_entries, num_local_members;
        Packet* p = Agent::allocpkt(size);
        hdr_srm* sh = hdr_srm::access(p);
        hdr_srm_ext* seh = hdr_srm_ext::access(p);
        sh->type() = SRM_SESS;
        sh->sender() = addr();
        sh->seqnum() = ++rep_sessCtr_;
	seh->repid() = repid_;
#if 0
	printf("sending rep_info session message\n");
#endif
        int* data = (int*) p->accessdata();
	*data++ = groupSize_;
	*data++ = SRM_RINFO;
	num_entries = 0;
	num_local_members = 0;
	for (SRMinfo* sp = sip_; sp; sp = sp->next_) {
	  if (sp->activeFlag_ == ACTIVE) {
	  /* Rep info has distance to others reps and 
	     timestamps for everyone */
	        *data++ = sp->sender_;
                *data++ = sp->ldata_;
		if (sp->scopeFlag_ == SRM_GLOBAL) {
		  *data++ = (int) (sp->distance_*1000);
		  data++;
		} else { 
		  // Put a check here for only people I have heard from.??
		  *data++ = sp->recvTime_;
		  *data++ = sp->sendTime_;
		  num_local_members++;
		}
		*data++ = sp->repid_;
		num_entries++;
	      }
	}
	if (num_local_members <= 0) {
	  Packet::free(p);
	  return;
	}
	data = (int*) p->accessdata();
	data[0] = num_entries;
	data[1] = SRM_RINFO;
	size = (SESS_CONST + num_entries * SESSINFO_SIZE) * sizeof(int);  
	data[5] = (int) (Scheduler::instance().clock()*1000);

	hdr_cmn* ch = hdr_cmn::access(p);
        ch->size() += size+ sizeof(hdr_srm);
        hdr_ip*  ih = hdr_ip::access(p);
	ih->ttl() = localScope_;
	// Currently put this to distinguish various session messages
	ih->flowid() = SRM_RINFO;
	seh->ottl() = localScope_;
        target_->recv(p, (Handler*)NULL);
}


#define	GET_SESSION_INFO			\
	sender = *data++;			\
	dataCnt = *data++;			\
	rtime = *data++;			\
	stime = *data++;                         \
        repid = *data++;                        \
	// printf("s:%d, d:%d, rt:%d, st:%d, rep:%d\n",sender,
	// dataCnt,rtime,stime,repid)



void SSMSRMAgent::recv_sess(int sessCtr, int* data, Packet* p)
{
  int type = data[1];

  switch (type) {
  case SRM_GLOBAL :
    recv_glb_sess(sessCtr,data,p);
    break;
  case SRM_LOCAL :
    recv_loc_sess(sessCtr,data,p);
    break;
  case SRM_RINFO :    
    if (scopeFlag_ == SRM_GLOBAL) return;
    recv_rep_sess(sessCtr,data,p);
    break;
  }
  Packet::free(p);
}

void SSMSRMAgent::recv_glb_sess(int sessCtr, int* data, Packet* p)
{
  Tcl& tcl = Tcl::instance();
  SRMinfo* sp;
  int ttl;

  hdr_ip*  ih = hdr_ip::access(p);
  hdr_srm_ext* seh = hdr_srm_ext::access(p);
  ttl = seh->ottl() - ih->ttl();
	
  int sender, dataCnt, rtime, stime,repid;
  int now, sentAt, sentBy;
  int cnt = *data++;
  //int type = *data++;
  int i;

  // data = data + SESS_CONST;  
  /* As as included type of session message also */
  /* The first block contains the sender's own state */
  GET_SESSION_INFO;
  if (sender == addr())			
    // sender's own session message
    return;
  if (seh->repid() != repid) {
    fprintf(stdout,"%f Recvd a glb-sess with diff header(%d) != inside(%d)\n",
	    Scheduler::instance().clock(),seh->repid(),repid);
    /* abort(); */
    return;
  }
  if (sender != repid) {
    fprintf(stdout,"%f Recvd a glb-sess with repid(%d) != address(%d)\n",
	    Scheduler::instance().clock(),repid,sender);
    /* abort(); */
    return;
  }
   
  sp = get_state(sender);
  if (sp->lglbsess_ > sessCtr)		// older session message recd.
    return;
#if 0
  fprintf(stdout,"%s recv-gsess from %d\n",name_,sender);
#endif 
  tcl.evalf("%s recv-gsess %d %d", name_, sender, ttl);
  
  if (sp->scopeFlag_ != SRM_GLOBAL) {
    sp->scopeFlag_ = SRM_GLOBAL;
  }
  sp->repid_ = repid;
  
  now = (int) (Scheduler::instance().clock() * 1000);
  sentBy = sender;			// to later compute rtt
  sentAt = stime;
	
  sp->lglbsess_ = sessCtr;
  sp->recvTime_ = now;
  sp->sendTime_ = stime;
  for (i = sp->ldata_ + 1; i <= dataCnt; i++)
    if (! sp->ifReceived(i))
      tcl.evalf("%s request %d %d", name_, sender, i, sp->repid_);
  if (sp->ldata_ < dataCnt)
    sp->ldata_ = dataCnt;
  
  for (i = 1; i < cnt; i++) {
    GET_SESSION_INFO;
    if (sender == addr() && now) {
      int rtt = (now - sentAt) + (rtime - stime);
      sp = get_state(sentBy);
      sp->distance_ = (double) rtt / 2 / 1000;
      sp->distanceFlag_ = SELF_DISTANCE;
#if 0
      fprintf(stderr,
	      "%7.4f %s compute distance to %d: %f\n",
	      Scheduler::instance().clock(), name_,
	      sentBy, sp->distance_);
#endif
      continue;
    }
    sp = get_state(sender);
    for (int j = sp->ldata_ + 1; j <= dataCnt; j++)
      if (! sp->ifReceived(j))
	tcl.evalf("%s request %d %d", name_, sender, j, sp->repid_);
    if (sp->ldata_ < dataCnt)
      sp->ldata_ = dataCnt;
  }		
}


void SSMSRMAgent::recv_loc_sess(int sessCtr, int* data, Packet* p)
{
  Tcl& tcl = Tcl::instance();
  SRMinfo* sp;
  int ttl;

  hdr_ip*  ih = hdr_ip::access(p);
  hdr_srm_ext* seh = hdr_srm_ext::access(p);
  ttl = seh->ottl() - ih->ttl();
  
  int sender, dataCnt, rtime, stime,repid;
  int now, sentAt, sentBy;
  int cnt = *data++;
  /*int type = * */data++;
  int i;

  // data = data + SESS_CONST;  
  /* As as included type of session message also */

  /* The first block contains the sender's own state */
  GET_SESSION_INFO;
  if (sender == addr())			// sender's own session message
    return;
  
  sp = get_state(sender);
  if (sp->llocsess_ > sessCtr)		// older session message recd.
    return;
  if (sp->scopeFlag_ != SRM_LOCAL) {
    sp->scopeFlag_ = SRM_LOCAL;
    // Also put a check if this is my child
  }
  sp->repid_ = repid;

#if 0
  fprintf(stdout,"%s recv-lsess from %d\n",name_,sender);
#endif 
  tcl.evalf("%s recv-lsess %d %d %d", name_, sender, repid, ttl);
  
  now = (int) (Scheduler::instance().clock() * 1000);
  sentBy = sender;			// to later compute rtt
  sentAt = stime;
	
  sp->llocsess_ = sessCtr;
  sp->recvTime_ = now;
  sp->sendTime_ = stime;
  for (i = sp->ldata_ + 1; i <= dataCnt; i++)
    if (! sp->ifReceived(i))
      tcl.evalf("%s request %d %d", name_, sender, i, sp->repid_);
  if (sp->ldata_ < dataCnt)
    sp->ldata_ = dataCnt;
  
  for (i = 1; i < cnt; i++) {
    GET_SESSION_INFO;
    if (sender == addr() && now) {
      int rtt = (now - sentAt) + (rtime - stime);
      sp = get_state(sentBy);
      sp->distance_ = (double) rtt / 2 / 1000;
      sp->distanceFlag_ = SELF_DISTANCE;
#if 0
      fprintf(stderr,
	      "%7.4f %s compute distance to %d: %f\n",
	      Scheduler::instance().clock(), name_,
	      sentBy, sp->distance_);
#endif
      continue;
    }
    sp = get_state(sender);
    for (int j = sp->ldata_ + 1; j <= dataCnt; j++)
      if (! sp->ifReceived(j))
	tcl.evalf("%s request %d %d", name_, sender, j, sp->repid_);
    if (sp->ldata_ < dataCnt)
      sp->ldata_ = dataCnt;
  }		
}


// For the global members the repid == addr


void SSMSRMAgent::recv_rep_sess(int sessCtr, int* data, Packet*)
{
  Tcl& tcl = Tcl::instance();
  SRMinfo* sp;
  
  int sender, dataCnt, rtime, stime,repid;
  int now, sentAt, sentBy;
  int cnt = *data++;
  /*int type = **/data++;
  int i;

  //data = data + SESS_CONST;  
  /* As as included type of session message also */

  /* The first block contains the sender's own state */
  GET_SESSION_INFO;
  if (sender == addr())			// sender's own session message
    return;
  if (sender != repid_)                 // not from my rep
    return;
  if (sender != repid) {
    fprintf(stdout,"Recvd a rep-sess with repid(%d) != address(%d)\n",
	    repid,sender);
    abort();
  }
  sp = get_state(sender);
  if (sp->lrepsess_ > sessCtr)		// older session message recd.
    return;
  if (sp->scopeFlag_ != SRM_GLOBAL)      // Should I change the repid also??
    sp->scopeFlag_ = SRM_GLOBAL;
  
  now = (int) (Scheduler::instance().clock() * 1000);
  sentBy = sender;			// to later compute rtt
  sentAt = stime;
	
  sp->lrepsess_ = sessCtr;
  sp->recvTime_ = now;
  sp->sendTime_ = stime;
  for (i = sp->ldata_ + 1; i <= dataCnt; i++)
    if (! sp->ifReceived(i))
      tcl.evalf("%s request %d %d", name_, sender, i, sp->repid_);
  if (sp->ldata_ < dataCnt)
    sp->ldata_ = dataCnt;
	
  for (i = 1; i < cnt; i++) {
    GET_SESSION_INFO;
    if (sender == addr() && now) {
      int rtt = (now - sentAt) + (rtime - stime);
      sp = get_state(sentBy);
      sp->distance_ = (double) rtt / 2 / 1000;
      sp->distanceFlag_ = SELF_DISTANCE;
#if 0
      fprintf(stderr,
	      "%7.4f %s compute distance to %d: %f\n",
	      Scheduler::instance().clock(), name_,
	      sentBy, sp->distance_);
#endif
      continue;
    }
    if ((sender == repid) && (sender != sentBy)) {
      sp = get_state(sender);
      if (!(is_active(sp) && (sp->distanceFlag_ == SELF_DISTANCE))) {
	sp->distance_ = (double) rtime/1000;   
	/* As for global members this is distance */
	sp->distanceFlag_ = REP_DISTANCE;
	/* ?? What if I am hearing from this guy already */
      }
    }
    sp = get_state(sender);
    for (int j = sp->ldata_ + 1; j <= dataCnt; j++)
      if (! sp->ifReceived(j))
	tcl.evalf("%s request %d %d", name_, sender, j, sp->repid_);
    if (sp->ldata_ < dataCnt)
      sp->ldata_ = dataCnt;
  }		
}



#define sessionDelay   1000


void SSMSRMAgent::timeout_info()
{
  int now;
  now = (int) (Scheduler::instance().clock() * 1000);
  for (SRMinfo* sp = sip_->next_; sp; sp = sp->next_) {
    if ((now - sp->recvTime_) >= 3*sessionDelay) {
      sp->activeFlag_ = INACTIVE;
      groupSize_--;
    }

  }
}

int SSMSRMAgent::is_active(SRMinfo *sp)
{
  int now;
  now = (int) (Scheduler::instance().clock() * 1000);
  if ((sp->sender_ != addr()) && ((now - sp->recvTime_) >= 3*sessionDelay)) {
    return 0;
  } else {
    return 1;
  }
}
