
/*
 * Copyright (C) 2004-2005 by the University of Southern California
 * $Id: diffrtg.cc,v 1.16 2005/09/13 20:47:34 johnh Exp $
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
// Diffusion Routing Agent - a wrapper class for core diffusion agent, ported from SCADDS's directed diffusion software. --Padma, nov 2001.

#ifdef NS_DIFFUSION

#include "diffrtg.h"
#include "address.h"
#include "scheduler.h"
#include "diffagent.h"


static class DiffRoutingAgentClass : public TclClass {
public:
	DiffRoutingAgentClass() : TclClass("Agent/DiffusionRouting") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc == 5)
			return(new DiffRoutingAgent(atoi(argv[4])));
		
		fprintf(stderr, "Insufficient number of args for creating DiffRtgAgent");
		return (NULL);
	}
} class_diffusion_routing_agent;


void LocalApp::sendPacket(DiffPacket msg, int len, int dst) {
  agent_->sendPacket(msg, len, dst); 
}


DiffPacket LocalApp::recvPacket(int fd) {
  DiffPacket p;
  
  fprintf(stderr, "This function should not get called; call DiffRoutingAgent::recv(Packet *, Handler *) instead\n\n");
  exit(1);
  return (p);  // to keep the compiler happy
}


void LinkLayerAbs::sendPacket(DiffPacket dp, int len, int dst) {
  Packet *p;
  hdr_cmn *ch;
  hdr_ip *iph;
  Message *msg;
  
  msg = (Message *)dp;
  p = agent_->createNsPkt(msg, len, dst); 
  iph = HDR_IP(p);
  ch = HDR_CMN(p);
  iph->saddr() = agent_->addr();
  iph->sport() = agent_->port();    //RT_PORT;
  iph->daddr() = msg->next_hop_;  // Use diffusion next_hop_
  iph->dport() = agent_->port();    //RT_PORT;
  ch->next_hop_ = msg->next_hop_;  // populate nexthop in cmn hdr
  agent_->send(p, 0);

}


DiffPacket LinkLayerAbs::recvPacket(int fd) {
  DiffPacket p;
  
  fprintf(stderr, "This function should not get called; call DiffRoutingAgent::recv(Packet *, Handler *) instead\n\n");
  exit(1);
  return (p);  // to keep the compiler happy
}



DiffRoutingAgent::DiffRoutingAgent(int nodeid) : Agent(PT_DIFF) {
	agent_ = new DiffusionCoreAgent(this, nodeid);

}


void DiffRoutingAgent::sendPacket(DiffPacket dp, int len, int dst) {
	Packet *p;
	hdr_ip *iph;
	Message *msg;

	msg = (Message *)dp;
	p = createNsPkt(msg, len, dst); 
	iph = HDR_IP(p);
	iph->dport() = dst;
	
	// schedule for a realistic delay : 0 sec for now
	(void)Scheduler::instance().schedule(port_dmux(), p, 0.000001);
	
}

void
DiffRoutingAgent::initpkt(Packet* p, Message* msg, int len)
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
	iph->sport() = port(); // RT_PORT
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
DiffRoutingAgent::createNsPkt(Message *msg, int len, int dst) {
	Packet *p;
	
	p = Packet::alloc();
	initpkt(p, msg, len);
	return p;
}

void DiffRoutingAgent::recv(Packet *p, Handler *h) {
	Message *msg;
	DiffusionData *diffdata;
	
	diffdata = (DiffusionData *)(p->userdata());
	msg = diffdata->data();
	
	agent_->recvMessage(msg);
	
	//delete msg;
	Packet::free(p);
}

int DiffRoutingAgent::command(int argc, const char*const* argv) {
	if (argc == 2) {
		if (strcasecmp(argv[1], "start")==0) {
			//start();
			//eq = new DiffusionCoreEQ(this);
			//eq->eq_new();
			// Add timers to the eventQueue
			//eq->eq_addAfter(NEIGHBORS_TIMER, NULL, NEIGHBORS_DELAY);
			//eq->eq_addAfter(FILTER_TIMER, NULL, FILTER_DELAY);
			
			return TCL_OK;
		}
		
	}
	else if (argc == 3) {
		if (strcasecmp(argv[1], "addr") == 0) {
			addr_ = (Address::instance().str2addr(argv[2]));
			return TCL_OK;
		}
		if (strcasecmp(argv[1], "stop-time")==0) {
			// add stop-event which when fired dumps statistical data
			// at end of ns simulation
			TimerCallback *callback;
			callback = new DiffusionStopTimer(agent_);
			agent_->timers_manager_->addTimer(atoi(argv[2])*1000, callback);
			return TCL_OK;
		}
		TclObject *obj;
		if ((obj = TclObject::lookup (argv[2])) == 0) {
			fprintf(stderr, "_Diffusion Node_ %s lookup of %s failed\n", argv[1], argv[2]);
			return TCL_ERROR;
		}
		if (strcasecmp(argv[1], "port-dmux") == 0) {
			port_dmux_ = (PortClassifier *)obj;
			return TCL_OK;
		}
		if (strcasecmp(argv[1], "add-ll") == 0) {
			target_ = (LL*)obj;
			return TCL_OK;
		}
		if (strcasecmp(argv[1], "tracetarget") == 0) {
			tracetarget_ = (Trace *)obj;
			return TCL_OK;
		}
	}
	return Agent::command(argc, argv);
}


void DiffRoutingAgent::trace(char *fmt, ...) {
	va_list ap;
	if (!tracetarget_)
		return;
	
	va_start (ap, fmt);
	vsprintf (tracetarget_->pt_->buffer (), fmt, ap);
	tracetarget_->pt_->dump ();
	va_end (ap);
}
	

#endif // NS
