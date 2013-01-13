
/*
 * Copyright (C) 2004-2005 by the University of Southern California
 * $Id: diffagent.cc,v 1.13 2005/09/13 20:47:34 johnh Exp $
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

// DiffAppAgent - Wrapper Class for diffusion transport agent DR, ported from SCADDS's directed diffusion software. --Padma, nov 2001.  

#ifdef NS_DIFFUSION

#include "diffagent.h"
#include "diffrtg.h"


static class DiffAppAgentClass : public TclClass {
public:
  DiffAppAgentClass() : TclClass("Agent/DiffusionApp") {}
  TclObject* create(int , const char*const* ) {
    return(new DiffAppAgent());
  }
} class_diffusion_app_agent;


void NsLocal::sendPacket(DiffPacket p, int len, int dst) {
  agent_->sendPacket(p, len, dst);
}

DiffPacket NsLocal::recvPacket(int fd) {
  DiffPacket p;
  fprintf(stderr, "This function should not get called; call DiffAppAgent::recv(Packet *, Handler *) instead\n\n");
  exit(1);
  return (p);  // to keep the compiler happy
}


DiffAppAgent::DiffAppAgent() : Agent(PT_DIFF) {
  dr_ = NR::create_ns_NR(DEFAULT_DIFFUSION_PORT, this);
}


int DiffAppAgent::command(int argc, const char*const* argv) {
  //Tcl& tcl = Tcl::instance();
  
  if (argc == 3) {
	  if (strcmp(argv[1], "node") == 0) {
		  MobileNode *node = (MobileNode *)TclObject::lookup(argv[2]);
		  ((DiffusionRouting *)dr_)->getNode(node);
		  return TCL_OK;
	  } 
	  if (strcmp(argv[1], "agent-id") == 0) {
		  int id = atoi(argv[2]);
		  ((DiffusionRouting *)dr_)->getAgentId(id);
		  return TCL_OK;
	  }
  }
  return Agent::command(argc, argv);
}



void DiffAppAgent::recv(Packet* p, Handler* h) 
{
	Message *msg;
	DiffusionData *diffdata;
	
	diffdata = (DiffusionData *)(p->userdata());
	msg = diffdata->data();
	
	DiffusionRouting *dr = (DiffusionRouting*)dr_;
	dr->recvMessage(msg);
	
	Packet::free(p);
	
}

void
DiffAppAgent::initpkt(Packet* p, Message* msg, int len)
{
	hdr_cmn* ch = HDR_CMN(p);
	hdr_ip* iph = HDR_IP(p);
	AppData *diffdata;
		
	diffdata  = new DiffusionData(msg, len);
	p->setdata(diffdata);
	
	// initialize pkt
	ch->uid() = msg->pkt_num_; /* copy pkt_num from diffusion msg */
	ch->ptype() = type_;
	ch->size() = size_;
	ch->timestamp() = Scheduler::instance().clock();
	ch->iface() = UNKN_IFACE.value(); // from packet.h (agent is local)
	ch->direction() = hdr_cmn::NONE;
	ch->error() = 0;	/* pkt not corrupt to start with */

	iph->saddr() = addr();
	iph->sport() = ((DiffusionRouting*)dr_)->getAgentId();
	iph->daddr() = addr();
	iph->flowid() = fid_;
	iph->prio() = prio_;
	iph->ttl() = defttl_;
	
	hdr_flags* hf = hdr_flags::access(p);
	hf->ecn_capable_ = 0;
	hf->ecn_ = 0;
	hf->eln_ = 0;
	hf->ecn_to_echo_ = 0;
	hf->fs_ = 0;
	hf->no_ts_ = 0;
	hf->pri_ = 0;
	hf->cong_action_ = 0;
	
}

Packet* 
DiffAppAgent::createNsPkt(Message *msg, int len) 
{
	Packet *p;
  
	p =  Packet::alloc();
	initpkt(p, msg, len);
	return (p);
}


void DiffAppAgent::sendPacket(DiffPacket dp, int len, int dst) {
  Packet *p;
  hdr_ip *iph;
  Message *msg;

  msg = (Message *)dp;
  p = createNsPkt(msg, len); 
  iph = HDR_IP(p);
  iph->dport() = dst;

  // schedule for realistic delay : set to 0 sec for now
  (void)Scheduler::instance().schedule(target_, p, 0.000001);

}


#endif // NS



