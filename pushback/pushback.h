/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 2000  International Computer Science Institute
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by ACIRI, the AT&T 
 *      Center for Internet Research at ICSI (the International Computer
 *      Science Institute).
 * 4. Neither the name of ACIRI nor of ICSI may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICSI AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ICSI OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef ns_pushback_h
#define ns_pushback_h

#include "config.h"
#include "packet.h"
#include "ip.h"
#include "agent.h"
#include "address.h"
#include "node.h"
#include "route.h"
#include "timer-handler.h"

#include "pushback-constants.h"
#include "pushback-message.h"

class PushbackQueue;
class PushbackEventList;
class IdentStruct;
class RateLimitSession;

struct hdr_pushback {
  
  PushbackMessage * msg_;
  
  static int offset_;
  inline static int& offset() {
    return offset_;
  }
  inline static hdr_pushback * access(Packet *p) {
    return (hdr_pushback *) p->access(offset_);
  }
};

// a structure to store details of each link on the node.
struct queue_rec {

  PushbackQueue * pbq_;             //pointer to the queue object
  IdentStruct * idTree_;              // this queues prefix tree

  // other required variables go here

};

class PushbackEvent;
class PushbackTimer;
  
class PushbackAgent : public Agent {

 public:
  PushbackAgent();
  int command(int argc, const char*const* argv);
  void reportDrop(int qid, Packet * p);
  void timeout(PushbackEvent * event);
  void identifyAggregate(int qid, double arrRate, double linkBW);
  void resetDropLog(int qid);
  void recv(Packet *p, Handler *);
  void calculateLowerBound(int qid, double arrRate);

  int last_index_;
  int verbose_;
  int intResult_;
  int debugLevel;
  char prnMsg[500];  //hopefully long enough
  
  static int mergerAccept(int count, int bits, int bitsDiff);
  void printMsg(char *msg, int msgLevel);

  Node * node_;

protected:
  int enable_pushback_;
  queue_rec queue_list_[MAX_QUEUES];
  double requiredLimit_;
  

  RouteLogic * rtLogic_;
  PushbackTimer * timer_;
  
  void initialUpdate(RateLimitSession * rls);
  void pushbackCheck(RateLimitSession * rls);
  void pushbackStatus(RateLimitSession* rls);
  void pushbackRefresh(int qid);
  void pushbackCancel(RateLimitSession* rls);

  void processPushbackRequest(PushbackRequestMessage * msg);
  void processPushbackStatus(PushbackStatusMessage *msg);
  void processPushbackRefresh(PushbackRefreshMessage *msg);
  void processPushbackCancel(PushbackCancelMessage *msg);

  void refreshUpstreamLimits(RateLimitSession * rls);
  int getQID(int sender);
  int checkQID(int qid);
  void sendMsg(PushbackMessage * msg);
};



class PushbackEvent {

 public:
  double time_;
  int eventID_;
  RateLimitSession * rls_;
  int qid_;
  
  PushbackEvent * next_;
  
  PushbackEvent(double delay, int eventID, RateLimitSession * rls) {
    time_ = Scheduler::instance().clock() + delay;
    eventID_ = eventID;
    rls_ = rls;
    qid_= -1; //rls->localQID_;
    next_=NULL;
  }

  PushbackEvent(double delay, int eventID, int qid) {
    time_ = Scheduler::instance().clock() + delay;
    eventID_ = eventID;
    rls_=NULL;
    qid_=qid;
    next_=NULL;
  }
  
  void setSucc(PushbackEvent * event) {
    next_=event;
  }

  static char * type(PushbackEvent * event) {
    switch (event->eventID_) {
    case PUSHBACK_CHECK_EVENT: return "CHECK";
    case PUSHBACK_REFRESH_EVENT: return "REFRESH";
    case PUSHBACK_STATUS_EVENT: return "STATUS";
    case INITIAL_UPDATE_EVENT: return "INITIAL UPDATE";
    default: return "UNKNOWN";
    }
  }
};

class PushbackTimer: public TimerHandler {
  
 public: 
  PushbackEvent * firstEvent_;
  
  PushbackTimer(PushbackAgent * agent): firstEvent_(NULL) { agent_ = agent;}
  void insert(PushbackEvent * event);
  void cancelStatus(RateLimitSession * rls);
  void removeEvents(RateLimitSession * rls);
  int containsRefresh(int qid);

 protected:
  PushbackAgent * agent_;
  virtual void expire(Event *e);
  void schedule();
  void sanityCheck();
};

#endif 
