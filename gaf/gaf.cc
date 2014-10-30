
/*
 * gaf.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: gaf.cc,v 1.8 2010/05/11 04:53:03 tom_henderson Exp $
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


#include <template.h>
#include <gaf/gaf.h>
#include <random.h>
#include <address.h>
#include <mobilenode.h>
#include <god.h>
#include <phy.h>
#include <wireless-phy.h>
#include <energy-model.h>



int hdr_gaf::offset_;
static class GAFHeaderClass : public PacketHeaderClass {
public:
	GAFHeaderClass() : PacketHeaderClass("PacketHeader/GAF",
					     sizeof(hdr_gaf)) {
		bind_offset(&hdr_gaf::offset_);
	}
} class_gafhdr;

static class GAFAgentClass : public TclClass {
public:
	GAFAgentClass() : TclClass("Agent/GAF") {}
	TclObject* create(int , const char*const* argv) {
		return (new GAFAgent((nsaddr_t) atoi(argv[4])));
	}
} class_gafagent;

static class GAFPartnerClass : public TclClass {
public:
        GAFPartnerClass() : TclClass("GAFPartner") {}
        TclObject* create(int, const char*const*) {
                return (new GAFPartner());
        }
} class_gafparnter;


static inline double 
gafjitter (double max, int be_random_)
{
  return (be_random_ ? Random::uniform(max) : max);
}


GAFAgent::GAFAgent(nsaddr_t id) : Agent(PT_GAF), beacon_(1), randomflag_(1), timer_(this), stimer_(this), dtimer_(this), maxttl_(5), state_(GAF_FREE),leader_settime_(0),adapt_mobility_(0)
{
        double x = 0.0, y = 0.0, z = 0.0;

	seqno_ = -1;
	nid_ = id;
	thisnode = Node::get_node_by_address(nid_); 
	// gid_ = nid_; // temporary setting, MUST BE RESET

	// grid caculation
	// no need to update location becasue getLoc will do it

	((MobileNode *)thisnode)->getLoc(&x, &y, &z);
	gid_ = God::instance()->getMyGrid(x,y);
        
	if (gid_ < 0) {
	    printf("fatal error: node is outside topography\n");
	}
}

void GAFAgent::recv(Packet* p, Handler *)
{
	hdr_gaf *gafh = hdr_gaf::access(p);

	switch (gafh->type_) {

	case GAF_DISCOVER:

	  if (state_ != GAF_SLEEP)
	      processDiscoveryMsg(p);
	  Packet::free(p);
	  break;

	default:
	  Packet::free(p);
	  break;
	}
}


void GAFAgent::processDiscoveryMsg(Packet* p)
{
	struct DiscoveryMsg emsg;
	u_int32_t dst;
	unsigned char *w = p->accessdata ();
	double x = 0.0, y = 0.0, z = 0.0;
	int ttl;


	dst = *(w++);
	dst = dst << 8 | *(w++);
        dst = dst << 8 | *(w++);
        dst = dst << 8 | *(w++);
	
	emsg.gid = dst;
	
	dst = *(w++);
        dst = dst << 8 | *(w++);
        dst = dst << 8 | *(w++);
        dst = dst << 8 | *(w++);

	emsg.nid = dst;

	dst = *(w++);
        dst = dst << 8 | *(w++);
        dst = dst << 8 | *(w++);
        dst = dst << 8 | *(w++);
	
	emsg.state = dst;

	dst = *(w++);
        dst = dst << 8 | *(w++);
        dst = dst << 8 | *(w++);
        dst = dst << 8 | *(w++);
	
	emsg.ttl = dst;
	
	dst = *(w++);
        dst = dst << 8 | *(w++);
        dst = dst << 8 | *(w++);
        dst = dst << 8 | *(w++);
	
	emsg.stime = dst;


	// first, check if this node has changed its grid
	((MobileNode *)thisnode)->getLoc(&x, &y, &z);
        gid_  = God::instance()->getMyGrid(x,y);
	
	// If the msg is not from my grid, ignore it
	if (((u_int32_t)gid_) != (u_int32_t)emsg.gid) return;


	switch (emsg.state) {

	case GAF_LEADER:

	  // I receives a "whoami" msg from the leader in this grid
	  // I am supposed to discard if I am in GAF_LEADER
	  // state too, or put myself into sleep if I am in GAF_FREE
	  
	  switch (state_) {
	  case GAF_LEADER:

	    ttl = (int)(leader_settime_ - NOW);
	    if (ttl < 0) ttl = 0;

    	    if ( ((u_int32_t)ttl) > (u_int32_t) emsg.ttl) {
    	          //supress the partner
    	          send_discovery();
   		  return; 
    	    
	    } else {
    	          if (((u_int32_t)ttl) == emsg.ttl && (u_int32_t)nid_ < emsg.nid) {
    	              send_discovery();
    		      return;
		  }
		  // from LEADER to SLEEP, cancel my timer
		  stimer_.force_cancel();
		  leader_settime_ = 0;

		  // turn off my self
		  schedule_wakeup(emsg);
	    }

	    break;
	  
	  case GAF_FREE:
	    schedule_wakeup(emsg);

	    break;
	  default:
	    break;

	  }

	  break;
	case GAF_FREE:
	  if (state_ == GAF_FREE) {
	      if ((ttl = (int)myttl()) > MIN_LIFETIME) {
    	         ttl = ttl/2;
    	      }
    
    	      if ( ttl > (int)emsg.ttl) {
    	          //supress other node
    	          send_discovery();
   		  return; 
    	      } else {
    	          if ((u_int32_t)ttl == emsg.ttl && (u_int32_t)nid_ < emsg.nid) {
    	              send_discovery();
    		      return;
    	          }
    
		  schedule_wakeup(emsg);

	      }    	    
    	  }
	  
	  if (state_ == GAF_LEADER) {
	      send_discovery();
	  }

	  break;
	default:
	  printf("%d gets wrong discovery msg\n",nid_ );;
	  break;
	}

}

void GAFAgent::schedule_wakeup(struct DiscoveryMsg emsg) {

  int waketime;
  waketime = emsg.ttl;
 
  // control whether using mobility adaption
  if (adapt_mobility_ > 0 ) {
      if (emsg.stime < emsg.ttl) waketime = emsg.stime;
  }

  // node does not go switch to sleep if the lifetime
  // it senses is less than MIN_SWITCHTIME

  if (waketime > MIN_TURNOFFTIME ) { 
     node_off();
     dtimer_.resched(Random::uniform(waketime/2, waketime)); 
  }
}

double GAFAgent::myttl()
{
  double ce,maxp;
  Phy *phyp; 
  double ttl; 
  
  ce = (thisnode->energy_model())->energy();

  phyp = (thisnode->ifhead()).lh_first;

  assert (phyp != 0);
  maxp = ((WirelessPhy *)phyp)->getPtconsume();
  ttl = ce/maxp;
  
  return ttl;

}

// timeout process for discovery phase
void GAFAgent::timeout(GafMsgType msgt)
{
  
  int ttl;

//printf ("Node (%d %d) get signal %d at %f\n",nid_, gid_, msgt, Scheduler::instance().clock());

    switch (msgt) {
    case GAF_DISCOVER:

      switch (state_) {
      case GAF_SLEEP:
	break;
       
      case GAF_FREE:

	if ((ttl = (int)myttl()) > MIN_LIFETIME) {
                ttl = (int) ttl/2;
        }
        
	leader_settime_ = (int) (ttl + NOW);  

	// schdule to tell me that I can switch after ttl

	stimer_.resched(ttl); 
	
	setGAFstate(GAF_LEADER);

	send_discovery();

	//printf ("Node (%d %d) becomes a leader at %f\n",nid_, gid_, Scheduler::instance().clock());

         timer_.resched(Random::uniform(MAX_DISCOVERY_TIME-1,MAX_DISCOVERY_TIME));
	
	// fall through
         break; 

      case GAF_LEADER:

	send_discovery();

      	timer_.resched(Random::uniform(MAX_DISCOVERY_TIME-1,MAX_DISCOVERY_TIME));
	break;
      default:
	break;

      }

      break;

    case GAF_SELECT:
        switch (state_) {

	  case GAF_LEADER:

	    // I just finish my LEADER role, put myself into FREE
	    // state so that I have chance to go sleep
	    // put myself into FREE does not hurt anything

	    //printf("Node (%d %d) go BACK to FREE from LEADER at %f\n",nid_,gid_,Scheduler::instance().clock());

	    duty_timeout();

	    leader_settime_ = 0;

	    break;

	  case GAF_FREE:
	  case GAF_SLEEP:
	    break;
	  default:
	    break;
        }
	break;
    case GAF_DUTY:
        duty_timeout();
        break;
    default:
        printf("Wrong GAF msg time!\n");
    }

}

// adaptive fidelity timeout

void GAFAgent::duty_timeout()
{
    double x=0.0, y=0.0, z=0.0;

    // find where I am
    
    ((MobileNode *)thisnode)->getLoc(&x, &y, &z);
    gid_ = God::instance()->getMyGrid(x,y);


    // wake up myself
    node_on();
    
    // send discovery first to try to find whether
    // there is a leader around me

    send_discovery();

    // schedule the discovery timer randomly
    // can wait longer to get a chance to be replaced

    timer_.resched(gafjitter(GAF_NONSTART_JITTER, 1));

}

int GAFAgent::command(int argc, const char*const* argv)
{
        if (argc == 2) {
	  if (strcmp (argv[1], "start-gaf") == 0) {
	    // schedule the discovery timer randomly
	    timer_.resched(gafjitter(GAF_STARTUP_JITTER, 1));
	    // schedule the select phase after certain time
	    // of discovery msg exchange, as fast as possible
	    // stimer_.resched(Random::uniform(GAF_LEADER_JITTER,GAF_LEADER_JITTER+1));	

	    return (TCL_OK); 
	  }
	      
	}
	if (argc == 3) {
	    if (strcmp(argv[1], "adapt-mobility") == 0) {
			adapt_mobility_ = atoi(argv[2]);
			//timer_.resched(Random::uniform(0, beacon_));
			return TCL_OK;
	    }
	
	    if (strcmp(argv[1], "maxttl") == 0) {
		maxttl_ = atoi(argv[2]);
		return TCL_OK;
	    }

	}
	return (Agent::command(argc, argv));
}

void GAFAgent::send_discovery()
{
        Packet *p = allocpkt();
	double x=0.0, y=0.0, z=0.0;

	hdr_gaf *h = hdr_gaf::access(p);
	//hdr_ip *iph = hdr_ip::access(p);

	h->type_ = GAF_DISCOVER;
	h->seqno_ = ++seqno_;

	// update my grid infomation
	
	((MobileNode *)thisnode)->getLoc(&x, &y, &z);
	gid_ = God::instance()->getMyGrid(x,y);
    
	makeUpDiscoveryMsg(p);

	send(p,0);
}

void
GAFAgent::makeUpDiscoveryMsg(Packet *p)
{
  hdr_ip *iph = hdr_ip::access(p);
  hdr_cmn *hdrc = hdr_cmn::access(p);
  u_int32_t ttl,stime;
  unsigned char *walk;
  double gridsize, speed;

  // fill up the header
  hdrc->next_hop_ = IP_BROADCAST;
  hdrc->addr_type_ = NS_AF_INET;
  iph->daddr() = IP_BROADCAST << Address::instance().nodeshift();
  iph->dport() = 254;

  // hdrc->direction() = hdr_cmn::DOWN;

  // fill up the data

  p->allocdata(sizeof(DiscoveryMsg));
  walk = p->accessdata ();
  hdrc->size_ = sizeof(DiscoveryMsg) + IP_HDR_LEN; // Existence Msg + IP

  *(walk++) = gid_ >> 24;
  *(walk++) = (gid_ >> 16) & 0xFF;
  *(walk++) = (gid_ >> 8) & 0xFF;
  *(walk++) = (gid_ >> 0) & 0xFF;
  *(walk++) = nid_ >> 24;
  *(walk++) = (nid_ >> 16) & 0xFF;
  *(walk++) = (nid_ >> 8) & 0xFF;
  *(walk++) = (nid_ >> 0) & 0xFF;

  // access my state

  *(walk++) = state_ >> 24;
  *(walk++) = (state_ >> 16) & 0xFF;
  *(walk++) = (state_ >> 8) & 0xFF;
  *(walk++) = (state_ >> 0) & 0xFF;  


  // ttl tells the receiver that how much longer the sender can
  // survive, cut it into half for the purpose of load balance

  if (state_ == GAF_LEADER) {
      // must send real msg because I am the leader
      	if (leader_settime_ < NOW)
		ttl = 0;
	else
		ttl = leader_settime_ - NOW;
  } else {

      if ((ttl = (u_int32_t)myttl()) > MIN_LIFETIME) {
          ttl = (u_int32_t) ttl/2;
      }

  }
  *(walk++) = ttl >> 24;
  *(walk++) = (ttl >> 16) & 0xFF;
  *(walk++) = (ttl >> 8) & 0xFF;
  *(walk++) = (ttl >> 0) & 0xFF;
  

  // fill my possible time of leaving this grid
  
  
  speed = ((MobileNode*)thisnode)->speed();

  if (speed == 0) {
    // make stime big enough
    stime = 2*ttl;
  } else {
    gridsize = God::instance()->getMyGridSize();
    stime = (u_int32_t) (gridsize/speed);
  }

  *(walk++) = stime >> 24;
  *(walk++) = (stime >> 16) & 0xFF;
  *(walk++) = (stime >> 8) & 0xFF;
  *(walk++) = (stime >> 0) & 0xFF;

  //printf("Node %d send out Exitence msg gid/nid %d %d\n", nid_, gid_, nid_);

}


void
GAFAgent::node_off()
{
    Phy *p;
    EnergyModel *em;

    //printf ("Node (%d %d) goes SLEEP from %d at %f\n",nid_, gid_, state_, Scheduler::instance().clock());

	
    // if I am in the data transfer state, do not turn off

    // if (state_ == GAF_TRAFFIC) return;

    // set node state
    em = thisnode->energy_model();
    em->node_on() = false;
    //((MobileNode *)thisnode->energy_model())->node_on() = false;

    // notify phy
    p  = (thisnode->ifhead()).lh_first;
    if (p) {
	((WirelessPhy *)p)->node_off();
    }
    // change agent state
    setGAFstate(GAF_SLEEP);
}

void
GAFAgent::node_on()
{
    Phy *p;
    EnergyModel* em;

    // set node state

    em = thisnode->energy_model();
    em->node_on() = true;

    //(MobileNode *)thisnode->energy_model()->node_on() = true;

    // notify phy

    p = (thisnode->ifhead()).lh_first;
    if (p) {
        ((WirelessPhy *)p)->node_on();
    }
    
    setGAFstate(GAF_FREE);
}

void 
GAFAgent::setGAFstate(GafNodeState gs)
{
  //printf("Node (%d %d) changes state from %d to %d at %f\n", nid_, gid_, state_,gs,Scheduler::instance().clock());

   state_ = gs;
}

GAFPartner::GAFPartner() : Connector(), gafagent_(1),mask_(0xffffffff),
        shift_(8)
{
        bind("addr_", (int*)&(here_.addr_));
        bind("port_", (int*)&(here_.port_));
        bind("shift_", &shift_);
        bind("mask_", &mask_);
}

void GAFPartner::recv(Packet* p, Handler *h)
{
        hdr_ip* hdr = hdr_ip::access(p);
	hdr_cmn *hdrc = hdr_cmn::access(p);

	/* handle PT_GAF packet only */
	/* convert the dst ip if it is -1, change it into node's */
	/* own ip address */	
	if ( hdrc->ptype() == PT_GAF ) {
	  if (gafagent_ == 1) {
	    if (((u_int32_t)hdr->daddr()) == IP_BROADCAST) {
		hdr->daddr() = here_.addr_;
	    }	    
	  } else {
	    /* if gafagent is not installed, drop the packet */
	    drop (p);
	    return;
	  }
	}
	
	target_->recv(p,h);


}

int GAFPartner::command(int argc, const char*const* argv)
{
        if (argc == 3) {
	  if (strcmp (argv[1], "set-gafagent") == 0) {
	    gafagent_ = atoi(argv[2]);
	    return (TCL_OK); 
	  }
	}
	return Connector::command(argc, argv); 
}

void GAFDiscoverTimer::expire(Event *) {
  a_->timeout(GAF_DISCOVER);
}

void GAFSelectTimer::expire(Event *) {
  a_->timeout(GAF_SELECT);
}

void GAFDutyTimer::expire(Event *) {
  a_->timeout(GAF_DUTY);
}




