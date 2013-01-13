
/*
 * Copyright (C) 2000 by the University of Southern California
 * $Id: mip-reg.cc,v 1.13 2007/01/01 17:38:41 mweigle Exp $
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

// $Header: /cvsroot/nsnam/ns-2/mobile/mip-reg.cc,v 1.13 2007/01/01 17:38:41 mweigle Exp $

/*
 * Copyright (c) Sun Microsystems, Inc. 1998 All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Sun Microsystems, Inc.
 *
 * 4. The name of the Sun Microsystems, Inc nor may not be used to endorse or
 *      promote products derived from this software without specific prior
 *      written permission.
 *
 * SUN MICROSYSTEMS MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS
 * SOFTWARE FOR ANY PARTICULAR PURPOSE.  The software is provided "as is"
 * without express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this software.
 */

// #ident "@(#)mip-reg.cc  1.4     98/08/30 SMI"

#include <template.h>
#include <mip.h>
#include <random.h>
#include <address.h>
#include <mobilenode.h>
#if defined (SOLARIS_MIN_MAX)
#include <config.h>     /* for MIN/MAX (if Solaris) */
#else
#include <sys/param.h>  /* for MIN/MAX */
#endif

#define AGENT_ADS_SIZE		48
#define REG_REQUEST_SIZE	52

int hdr_mip::offset_;
static class MIPHeaderClass : public PacketHeaderClass {
public:
	MIPHeaderClass() : PacketHeaderClass("PacketHeader/MIP",
					     sizeof(hdr_mip)) {
		bind_offset(&hdr_mip::offset_);
	}
} class_miphdr;

static class MIPBSAgentClass : public TclClass {
public:
	MIPBSAgentClass() : TclClass("Agent/MIPBS") {}
	TclObject* create(int, const char*const*) {
		return (new MIPBSAgent());
	}
} class_mipbsagent;

MIPBSAgent::MIPBSAgent() : Agent(PT_UDP), beacon_(1.0), 
  bcast_target_(0), ragent_(0), timer_(this), adlftm_(~0)
{
	bind("adSize_", &size_);
	//bind("shift_", &shift_);
	//bind("mask_", &mask_);
	bind("ad_lifetime_", &adlftm_);
	size_ = AGENT_ADS_SIZE;
	seqno_ = -1;
}

void MIPBSAgent::recv(Packet* p, Handler *)
{
	Tcl& tcl = Tcl::instance();
	const char *objname = NULL;
	NsObject *obj = NULL;
	hdr_mip *miph = hdr_mip::access(p);
	hdr_ip *iph = hdr_ip::access(p);
	hdr_cmn *ch = hdr_cmn::access(p);
	int nodeaddr = Address::instance().get_nodeaddr(addr());
	
	switch (miph->type_) {
	case MIPT_REG_REQUEST:
	  //if (miph->ha_ == (addr_ >> shift_ & mask_)) {
	  if (miph->ha_ == (Address::instance().get_nodeaddr(addr()))){
	    if (miph->ha_ == miph->coa_) { // back home
	      tcl.evalf("%s clear-reg %d", name_,
			miph->haddr_);
	    }
	    else {
	      tcl.evalf("%s encap-route %d %d %lf", name_,
			miph->haddr_, miph->coa_,
			miph->lifetime_);
	    }
	    iph->dst() = iph->src();
	    miph->type_ = MIPT_REG_REPLY;
	  }
	  else {
	    //iph->dst() = iph->dst() & ~(~(nsaddr_t)0 << shift_) | (miph->ha_ & mask_) << shift_;
	    iph->daddr() = miph->ha_;
	    iph->dport() = 0;
	  }
	  iph->saddr() = addr();
	  iph->sport() = port();
	  // by now should be back to normal route
	  // if dst is the mobile
	  // also initialise forward counter to 0. otherwise routing
	  // agent is going to think pkt is looping and drop it!!
	  ch->num_forwards() = 0;
	  
	  send(p, 0);
	  break;
	case MIPT_REG_REPLY:
	  //assert(miph->coa_ == (addr_ >> shift_ & mask_));

	  assert(miph->coa_ == nodeaddr);
	  tcl.evalf("%s get-link %d %d", name_, nodeaddr, miph->haddr_);
	  //
	  // XXX hacking mobileip. all this should go away
	  // when mobileIP for sun-wired model is no longer reqd.
	  //
	  obj = (NsObject*)tcl.lookup(objname = tcl.result());
	  if (strlen(objname) == 0)
	    objname = "XXX";
	  tcl.evalf("%s decap-route %d %s %lf", name_, miph->haddr_,
		    objname, miph->lifetime_);
	  
	  iph->src() = iph->dst();
	  //iph->dst() = iph->dst() & ~(~(nsaddr_t)0 << shift_) |(miph->haddr_ & mask_) << shift_;
	  iph->daddr() = miph->haddr_;
	  iph->dport() = 0;
	  if (obj == NULL)
	    obj = ragent_;
	  obj->recv(p, (Handler*)0);
	  break;
	case MIPT_SOL:
	  //tcl.evalf("%s get-link %d %d", name_, addr_ >> shift_ & mask_,miph->haddr_);
	  tcl.evalf("%s get-link %d %d",name_,nodeaddr,miph->haddr_);
	  send_ads(miph->haddr_, (NsObject*)tcl.lookup(tcl.result()));
	  Packet::free(p);
	  break;
	default:
	  Packet::free(p);
	  break;
	}
}

void MIPBSAgent::timeout(int )
{
	send_ads();
	timer_.resched(beacon_);
}

int MIPBSAgent::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "beacon-period") == 0) {
			beacon_ = atof(argv[2]);
			timer_.resched(Random::uniform(0, beacon_));
			return TCL_OK;
		}
		if (strcmp(argv[1], "bcast-target") == 0) {
			bcast_target_ = (NsObject *)TclObject::lookup(argv[2]);
			return TCL_OK;
		}
		if (strcmp(argv[1], "ragent") == 0) {
		  ragent_ = (NsObject *)TclObject::lookup(argv[2]);
			return TCL_OK;
		}
	}
	return (Agent::command(argc, argv));
}

void MIPBSAgent::send_ads(int dst, NsObject *target)
{
	Packet *p = allocpkt();
	hdr_mip *h = hdr_mip::access(p);
	hdr_ip *iph = hdr_ip::access(p);
	h->haddr_ = h->ha_ = -1;
	//h->coa_ = addr_ >> shift_ & mask_;
	h->coa_ = Address::instance().get_nodeaddr(addr());
	h->type_ = MIPT_ADS;
	h->lifetime_ = adlftm_;
	h->seqno_ = ++seqno_;
	if (dst != -1) {
	  iph->daddr() = dst;
	  iph->dport() = 0;
	}
	else {
	  // if bcast pkt
	  sendOutBCastPkt(p);
	}
	if (target == NULL) {
	  if (bcast_target_) bcast_target_->recv(p, (Handler*) 0);
	  else if (target_) target_->recv(p, (Handler*) 0);
	  else Packet::free(p); // drop; may log in future code
	}
	else target->recv(p, (Handler*)0);
}

void
MIPBSAgent::sendOutBCastPkt(Packet *p)
{
  hdr_ip *iph = hdr_ip::access(p);
  hdr_cmn *hdrc = hdr_cmn::access(p);
  hdrc->next_hop_ = IP_BROADCAST;
  hdrc->addr_type_ = NS_AF_INET;
  iph->daddr() = IP_BROADCAST;
  iph->dport() = 0;
}

void AgtListTimer::expire(Event *) {
	a_->timeout(MIP_TIMER_AGTLIST);
}

static class MIPMHAgentClass : public TclClass {
public:
	MIPMHAgentClass() : TclClass("Agent/MIPMH") {}
	TclObject* create(int, const char*const*) {
		return (new MIPMHAgent());
	}
} class_mipmhagent;

MIPMHAgent::MIPMHAgent() : Agent(PT_UDP), ha_(-1), coa_(-1),
	beacon_(1.0),bcast_target_(0),agts_(0),rtx_timer_(this), 
	agtlist_timer_(this),reglftm_(~0),adlftm_(0.0), node_ (0)
{
	bind("home_agent_", &ha_);
	bind("rreqSize_", &size_);
	bind("reg_rtx_", &reg_rtx_);
	bind("reg_lifetime_", &reglftm_);
	size_ = REG_REQUEST_SIZE;
	seqno_ = -1;
}

void MIPMHAgent::recv(Packet* p, Handler *)
{
	Tcl& tcl = Tcl::instance();
	hdr_mip *miph = hdr_mip::access(p);
	switch (miph->type_) {
	case MIPT_REG_REPLY:
		if (miph->coa_ != coa_) break; // not pending
		tcl.evalf("%s update-reg %d", name_, coa_);
		if (rtx_timer_.status() == TIMER_PENDING)
			rtx_timer_.cancel();
		break;
	case MIPT_ADS:
	  {
	    AgentList **ppagts = &agts_, *ptr;
	    while (*ppagts) {
	      if ((*ppagts)->node_ == miph->coa_) break;
	      ppagts = &(*ppagts)->next_;
	    }
	    if (*ppagts) {
	      ptr = *ppagts;
	      *ppagts = ptr->next_;
	      ptr->expire_time_ = beacon_ +
		Scheduler::instance().clock();
	      ptr->lifetime_ = miph->lifetime_;
	      ptr->next_ = agts_;
	      agts_ = ptr;
	      if (coa_ == miph->coa_) {
		seqno_++;
		reg();
	      }
	    }
	    else { // new ads
	      ptr = new AgentList;
	      ptr->node_ = miph->coa_;
	      ptr->expire_time_ = beacon_ +
		Scheduler::instance().clock();
	      ptr->lifetime_ = miph->lifetime_;
	      ptr->next_ = agts_;
	      agts_ = ptr;
	      coa_ = miph->coa_;
	  
	      // The MHagent now should update the Mobilenode
	      // about the changed coa_ : node updates its 
	      // base-station to new coa_ accordingly.
	      if(node_)
		node_->set_base_stn(coa_);
	      
	      adlftm_ = miph->lifetime_;
	      seqno_++;
	      reg();
	    }
	  }
	  break;
	default:
	  break;
	}
	Packet::free(p);
}

void MIPMHAgent::timeout(int tno)
{
	switch (tno) {
	case MIP_TIMER_SIMPLE:
		reg();
		break;
	case MIP_TIMER_AGTLIST:
		{
			double now = Scheduler::instance().clock();
			AgentList **ppagts = &agts_, *ptr;
			int coalost = 0;
			while (*ppagts) {
				if ((*ppagts)->expire_time_ < now) {
					ptr = *ppagts;
					*ppagts = ptr->next_;
					if (ptr->node_ == coa_) {
						coa_ = -1;
						coalost = 1;
					}
					delete ptr;
				}
				else ppagts = &(*ppagts)->next_;
			}
			agtlist_timer_.resched(beacon_);
			if (coalost) {
				seqno_++;
				reg();
			}
		}
		break;
	default:
		break;
	}
}

int MIPMHAgent::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "beacon-period") == 0) {
			beacon_ = atof(argv[2]);
			timeout(MIP_TIMER_AGTLIST);
			agtlist_timer_.resched(beacon_);
			rtx_timer_.resched(Random::uniform(0, beacon_));
			return TCL_OK;
		}
		else if (strcmp(argv[1], "bcast-target") == 0) {
			bcast_target_ = (NsObject *)TclObject::lookup(argv[2]);
			return TCL_OK;
		}
		else if (strcmp (argv[1], "node") == 0) {
		  node_ = (MobileNode*)TclObject::lookup(argv[2]);
		  if (node_ == 0) {
		    fprintf (stderr, "%s: %s lookup of %s failed\n", __FILE__, argv[1], argv[2]);
		    return TCL_ERROR;
		  }
		  return TCL_OK;
		}
	}
	// later: agent solicitation (now done!), start of simulation, ...
	return (Agent::command(argc, argv));
}

void MIPMHAgent::reg()
{
	rtx_timer_.resched(reg_rtx_);
	if (agts_ == 0) {
		send_sols();
		return;
	}
	if (coa_ < 0) {
		coa_ = agts_->node_;
		adlftm_ = agts_->lifetime_;
	}
	Tcl& tcl = Tcl::instance();
	Packet *p = allocpkt();
	hdr_ip *iph = hdr_ip::access(p);
	//iph->dst() = iph->dst() & ~(~(nsaddr_t)0 << shift_) | (coa_ & mask_) << shift_;
	iph->daddr() = coa_;
	iph->dport() = 0;
	hdr_mip *h = hdr_mip::access(p);
	//h->haddr_ = addr_ >> shift_ & mask_;
	h->haddr_ = Address::instance().get_nodeaddr(addr());
	h->ha_ = ha_;
	h->coa_ = coa_;
	h->type_ = MIPT_REG_REQUEST;
	h->lifetime_ = MIN(reglftm_, adlftm_);
	h->seqno_ = seqno_;
	tcl.evalf("%s get-link %d %d", name_, h->haddr_, coa_);
	NsObject *target = (NsObject *)tcl.lookup(tcl.result());
	if (target != NULL)
	  ((NsObject *)tcl.lookup(tcl.result()))->recv(p, (Handler*) 0);
	else
	  send(p, 0);
}

void MIPMHAgent::send_sols()
{
	Packet *p = allocpkt();
	hdr_mip *h = hdr_mip::access(p);
	h->coa_ = -1;
	//h->haddr_ = addr_ >> shift_ & mask_;
	h->haddr_ = Address::instance().get_nodeaddr(addr());
	h->ha_ = ha_;
	h->type_ = MIPT_SOL;
	h->lifetime_ = reglftm_;
	h->seqno_ = seqno_;
	sendOutBCastPkt(p);
	if (bcast_target_) 
		bcast_target_->recv(p, (Handler*) 0);
	else if (target_) 
		target_->recv(p, (Handler*) 0);
	else 
		Packet::free(p); // drop; may log in future code
}


void MIPMHAgent::sendOutBCastPkt(Packet *p)
{
	hdr_ip *iph = hdr_ip::access(p);
	hdr_cmn *hdrc = hdr_cmn::access(p);
	hdrc->next_hop_ = IP_BROADCAST;
	hdrc->addr_type_ = NS_AF_INET;
	iph->daddr() = IP_BROADCAST;
	iph->dport() = 0;
}
