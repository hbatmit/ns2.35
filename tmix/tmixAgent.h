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

#ifndef ns_tmix_agent_h
#define ns_tmix_agent_h

#define FULL 0
#define ONE_WAY 1

#include "tmix.h"
#include "node.h"
#include <tclcl.h>
#include "app.h"

class Tmix;
class TmixAgent : public TclObject {
public:
  TmixAgent(Tmix * t) : TclObject(), agent(NULL), tmix(t) {}
  inline const char* name() { return agent->name(); }
  inline void sendmsg(int bytes) { agent->sendmsg(bytes); agent->clr_closed(); }
  inline nsaddr_t& port() { return agent->port(); }
  inline nsaddr_t& addr() { return agent->addr(); }
  inline Agent* getAgent() { return agent; }
  inline int getType() { return type; }
  inline int is_closed() { return agent->is_closed(); }
  inline double getTimeInPool(double now) {return (now - time_entered_pool);}
  inline bool inPoolFor1s(double now) {return (now-time_entered_pool > 1);}
  inline void setPoolTime (double now) {time_entered_pool = now;}
  virtual void attachApp(Application* app);
  virtual inline Agent* getSink() { return NULL; }
	
  virtual void attachToNode(Node * node) = 0;
  virtual void configureTcp(Tmix * tmixInstance, int window, int mss) = 0;
  virtual void connect(TmixAgent * peer) = 0;
  virtual void reset() = 0;

  virtual ~TmixAgent() { delete agent; }

  TclObject* lookup_obj(const char* name) {
    TclObject* obj = Tcl::instance().lookup(name);
    if (obj == NULL)
      fprintf(stderr, "Bad object name %s\n", name);
    return obj;
  }

protected:
  Agent* agent;
  int type;
  double time_entered_pool;    /* time agent entered pool - to ensure agents
				  aren't re-used until they've been in pool
				  for at least 1 second */
  Tmix * tmix;
};

class TmixOneWayAgent : public TmixAgent {
public:
  TmixOneWayAgent(Tmix * t, char* tcptype, char* sinktype);
  void attachToNode(Node * node);
  void configureTcp(Tmix * tmixInstance, int window, int mss);
  void connect(TmixAgent * peer);
  void attachApp(Application * app);
  void reset();

  inline Agent* getSink() const { return sink; }
  inline const char* sinkName() const { return sink->name(); }
	
  ~TmixOneWayAgent();
private:
  Agent* sink;
};

class TmixFullAgent : public TmixAgent {
public:
  TmixFullAgent(Tmix * t, char* tcptype);
  void attachToNode(Node * node);
  void configureTcp(Tmix * tmixInstance, int window, int mss);
  void connect(TmixAgent * peer);
  void reset();

  ~TmixFullAgent();
};

TmixAgent * agentFactory(Tmix * tmix, char* tcptype, char* sinktype);

#endif
