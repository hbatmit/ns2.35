/*
 * Copyright (C) 2007 
 * Mercedes-Benz Research & Development North America, Inc. and
 * University of Karlsruhe (TH)
 * All rights reserved.
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
 * For further information see: 
 * http://dsn.tm.uni-karlsruhe.de/english/Overhaul_NS-2.php
 */



#include <iostream>
#include "random.h"
#include "pbc.h"
int hdr_pbc::offset_;

/**********************TCL Classes***************************/
static class PBCHeaderClass : public PacketHeaderClass {
public:
	PBCHeaderClass() : PacketHeaderClass("PacketHeader/PBC",
					      sizeof(hdr_pbc)) {
		bind_offset(&hdr_pbc::offset_);
	}
} class_pbchdr;


static class PBCClass : public TclClass {
public:
	PBCClass() : TclClass("Agent/PBC") {}
	TclObject* create(int, const char*const*) {
		return (new PBCAgent());
	}
} class_pbc;

/**********************PBC Agent****Constructor*************************/
PBCAgent::PBCAgent() : Agent(PT_PBC), timer(this)
{
  bind("payloadSize", &size);
  bind("periodicBroadcastVariance", &msgVariance);
  bind("periodicBroadcastInterval", &msgInterval);
  bind("modulationScheme",&modulationScheme);
  periodicBroadcast = false;
}

PBCAgent::~PBCAgent()
{

}

/**********************PBC Agent****Member functions *************************/
void PBCAgent::singleBroadcast()
{
  Packet* pkt = allocpkt();
  hdr_cmn *cmnhdr = hdr_cmn::access(pkt);
  hdr_pbc* pbchdr = hdr_pbc::access(pkt);
  hdr_ip*  iphdr  = hdr_ip::access(pkt);

  cmnhdr->next_hop() = IP_BROADCAST;

  cmnhdr->size()    = size;
  iphdr->src_.addr_ = here_.addr_;  //LL will fill MAC addresses in the MAC header
  iphdr->dst_.addr_ = IP_BROADCAST;
  iphdr->dst_.port_ = this->port();
  pbchdr->send_time	= Scheduler::instance().clock();

  switch (modulationScheme)
    {
    case BPSK:   cmnhdr->mod_scheme_ = BPSK;break;
    case QPSK:   cmnhdr->mod_scheme_ = QPSK;break;
    case QAM16:  cmnhdr->mod_scheme_ = QAM16;break;
    case QAM64:  cmnhdr->mod_scheme_ = QAM64;break;
    default :
      cmnhdr->mod_scheme_ = BPSK;
    }
  send(pkt,0);
}


void PBCAgent::singleUnicast(int addr)
{

  Packet* pkt = allocpkt();
  hdr_cmn *cmnhdr = hdr_cmn::access(pkt);
  hdr_pbc* pbchdr = hdr_pbc::access(pkt);
  hdr_ip*  iphdr  = hdr_ip::access(pkt);


  cmnhdr->addr_type() = NS_AF_ILINK;
  cmnhdr->next_hop()  = (u_int32_t)(addr);
  cmnhdr->size()      = size; 
  iphdr->src_.addr_ = here_.addr_;  //MAC will fill this address
  iphdr->dst_.addr_ = (u_int32_t)(addr);
  iphdr->dst_.port_ = this->port();

  pbchdr->send_time 	= Scheduler::instance().clock();

  switch (modulationScheme)
    {
    case BPSK:   cmnhdr->mod_scheme_ = BPSK;break;
    case QPSK:   cmnhdr->mod_scheme_ = QPSK;break;
    case QAM16:  cmnhdr->mod_scheme_ = QAM16;break;
    case QAM64:  cmnhdr->mod_scheme_ = QAM64;break;
    default :
      cmnhdr->mod_scheme_ = BPSK;
    }
  send(pkt,0);
}



void PBCAgent::recv(Packet* pkt, Handler*)
{
	Packet::free(pkt);
}

/*************************PBC Timer *******************/
/****asyn_start() start() stop() setPeriod()***********/
void
PBCTimer::start(void)
{
  if(!started)
    {
      Scheduler &s = Scheduler::instance();
      started = 1;
      variance = agent->msgVariance;
      period = agent->msgInterval;
      double offset = Random::uniform(0.0,period);
      s.schedule(this, &intr, offset);
    }
}


void PBCTimer::stop(void)
{
  	Scheduler &s = Scheduler::instance();
  	if(started)
  	{
  		s.cancel(&intr);
  	}
  	started = 0;
}

void PBCTimer::setVariance(double v)
{
  if(v >= 0) variance = v;
}

void PBCTimer::setPeriod(double p)
{
  if(p >= 0) period = p;
}

void PBCTimer::handle(Event *)
{
	agent->singleBroadcast();
	if(agent->periodicBroadcast)
	  {
	    Scheduler &s = Scheduler::instance();
	    double t = period - variance + Random::uniform(variance*2);
	    s.schedule(this, &intr, t>0.0 ? t : 0.0);
	  }
}


/*************************************************************************/


int PBCAgent::command(int argc, const char*const* argv)
{
	if (argc == 2) 
	{
		if (strcmp(argv[1], "singleBroadcast") == 0)
		{
			singleBroadcast();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "stop") == 0)
		{
		  timer.stop();
		  return (TCL_OK);
		}
	}
	if (argc == 3)
	{
		if(strcmp(argv[1],"unicast")==0)
		  {
		    int addr =atoi(argv[2]);
		    singleUnicast(addr);
		    return TCL_OK;
		  }
		if(strcmp(argv[1],"PeriodicBroadcast") ==0)
		{
		  if (strcmp(argv[2],"ON") == 0 )
		    {
		      periodicBroadcast = true;
		      timer.start();
		      return TCL_OK;
		    }
		  if(strcmp(argv[2],"OFF") ==0 )
		    {
		      periodicBroadcast = false;
		      timer.stop();
		      return TCL_OK;
		    }
		}


	}
	// If the command hasn't been processed by PBCAgent()::command,
	// call the command() function for the base class
	return (Agent::command(argc, argv));
}
