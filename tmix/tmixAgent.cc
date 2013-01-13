/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * Copyright 2007, Old Dominion University 
 * Copyright 2007, University of North Carolina at Chapel Hill
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright 
 * notice, this list of conditions and the following disclaimer in the 
 * documentation and/or other materials provided with the distribution.
 *    3. The name of the author may not be used to endorse or promote 
 * products derived from this software without specific prior written 
 * permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * M.C. Weigle, P. Adurthi, F. Hernandez-Campos, K. Jeffay, and F.D. Smith, 
 * Tmix: A Tool for Generating Realistic Application Workloads in ns-2, 
 * ACM Computer Communication Review, July 2006, Vol 36, No 3, pp. 67-76.
 *
 * Contact: Michele Weigle (mweigle@cs.odu.edu)
 * http://www.cs.odu.edu/inets/Tmix
 */

#include "tmixAgent.h"
#include "tmix.h"
#include "node.h"
#include <tclcl.h>
#include "tcp-full.h"
#include "diffusion/diff_sink.h"


void TmixAgent::attachApp(Application * app) {
  agent->attachApp(app);
  dynamic_cast<TmixApp*>(app)->set_agent(agent);
}

/********************************
 * OneWayAgent
 ********************************/

TmixOneWayAgent::TmixOneWayAgent(Tmix * t, char* tcptype, char* sinktype) : 
	TmixAgent(t) {
  // Setup required configuration which is generally done in setup_connection 
  Tcl& tcl = Tcl::instance();

  tcl.evalf("%s alloc-agent %s", tmix->name(), tcptype);
  agent = (Agent*) lookup_obj(tcl.result());
	
  tcl.evalf("%s alloc-sink %s", tmix->name(), sinktype);
  sink = (Agent*) lookup_obj(tcl.result());
  type = ONE_WAY;
}

void TmixOneWayAgent::attachToNode(Node * node) {
  Tcl& tcl = Tcl::instance();
  // agent
  tcl.evalf ("%s attach %s", node->name(), name());
  // sink
  tcl.evalf ("%s attach %s", node->name(), sinkName());
}

void TmixOneWayAgent::attachApp(Application* app) {
  agent->attachApp(app);
  sink->attachApp(app);
	
  dynamic_cast<TmixApp*>(app)->set_agent(agent);
  dynamic_cast<TmixApp*>(app)->set_sink(sink);
}

void TmixOneWayAgent::configureTcp(Tmix* tmixInstance, int window, int mss) {
  Tcl& tcl = Tcl::instance();

  tcl.evalf ("%s configure-source %s %d %d %d", tmixInstance->name(), 
	     agent->name(), tmixInstance->get_total(), window, mss);
  tcl.evalf ("%s configure-sink %s", tmixInstance->name(), sink->name());
}

void TmixOneWayAgent::reset() {
  sink->attachApp(NULL);
  if(TcpAgent* a = dynamic_cast<TcpAgent*>(agent)) {
    a->reset();
  }
  if(TcpAgent* s = dynamic_cast<TcpAgent*>(sink)) {
    s->reset();
  }
}

void TmixOneWayAgent::connect(TmixAgent * peer) {
  Tcl& tcl = Tcl::instance();
	
  tcl.evalf ("set ns [Simulator instance]");
  tcl.evalf ("$ns connect %s %s", name(), 
	     (dynamic_cast<TmixOneWayAgent*>(peer))->sinkName());
  tcl.evalf("$ns connect %s %s", peer->name(), this->sinkName());
}

TmixOneWayAgent::~TmixOneWayAgent() { delete agent; delete sink; }

/********************************
 * FullAgent
 ********************************/
TmixFullAgent::TmixFullAgent(Tmix * t, char* tcptype) : TmixAgent(t) {
  // pass t so we know what kind of full agent to instantiate
  Tcl& tcl = Tcl::instance();

  tcl.evalf("%s alloc-tcp %s", tmix->name(), tcptype);
  agent = (Agent*) lookup_obj(tcl.result());
  type = FULL;
}

void TmixFullAgent::reset() {
  dynamic_cast<FullTcpAgent*>(agent)->reset();
}

void TmixFullAgent::attachToNode(Node * node) {
  Tcl& tcl = Tcl::instance();
	
  tcl.evalf ("%s attach %s", node->name(), 
	     name());
}
	
void TmixFullAgent::configureTcp(Tmix* tmixInstance, int window, int mss) {
  Tcl& tcl = Tcl::instance();
		
  // note that for fulltcp init_mss == acc_mss
  tcl.evalf ("%s setup-tcp %s %d %d %d", tmixInstance->name(), agent->name(), 
	     tmixInstance->get_total(), window, mss);
}

void TmixFullAgent::connect(TmixAgent * peer) {
  Tcl& tcl = Tcl::instance();
	
  tcl.evalf ("set ns [Simulator instance]");
  tcl.evalf ("$ns connect %s %s", name(), peer->name());
}

TmixFullAgent::~TmixFullAgent() { }

TmixAgent * agentFactory(Tmix * tmix, char* tcptype, char* sinktype) {
  int type = tmix->getAgentType();
  if (type == FULL) {
    return new TmixFullAgent(tmix, tcptype);
  }
  else if (type == ONE_WAY) {
    return new TmixOneWayAgent(tmix, tcptype, sinktype);
  }
  return NULL;
}
