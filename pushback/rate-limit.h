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

#ifndef ns_rate_limit_h
#define ns_rate_limit_h

#include "node.h"
#include "packet.h"
#include "route.h"

#include "agg-spec.h"
#include "rate-limit-strategy.h"
#include "logging-data-struct.h"

class RateLimitSession {

 public:
  int pushbackON_;  //whether pushback has been called/propagated on this aggregate 
                    //  if pushback is ON, the node needs to send pushback refresh messages
  int origin_;      //id of the node who requested rate limiting; either itself or 
                    // one hop downstream (send status in latter case).

  //snapshot view taken once pushback is invoked/propagated.
  //used to report status before arrival of status messages from upstream.
  //double snapshotArrivalRate_;
 
  //Queue ID of the rate limiting session
  int localQID_;
  int remoteQID_; //qid downstream
  
  //session ID of rate limiting
  int localID_;   
  int remoteID_;  //id downstream;

  AggSpec * aggSpec_;
  
  //meaningful only for sessions started on this node.
  double lowerBound_;       // the specified lower bound for this aggregate.
  int merged_;              // whether merged or not
  int initialPhase_;        // the initial probability increasing phase.

  //position of this RLS in tree.
  int heightInPTree_; //from leaf.
  int depthInPTree_;  //from root.
  
  double startTime_;
  double expiryTime_;  //time when this rate-limiting should expire;
  double refreshTime_;
  RateLimitSession * next_; //next session in the list;

  RateLimitStrategy * rlStrategy_;
  LoggingDataStruct * logData_;

  //constructor for locally started sessions.
  RateLimitSession(AggSpec * aggSpec, double estimate, int initial,  double limit, int origin, 
		   int locQid, 
		   double delay, double lowerBound, Node *node, RouteLogic * rtLogic_);

  //for sessions started on pushback request.
  RateLimitSession(AggSpec * aggSpec, double limit, int originID,  int locQid, int remQid, 
		   int remoteID, int depth, double delay, double lowerBound, Node *node, RouteLogic * rtLogic_);

  ~RateLimitSession();

  void setSucc(RateLimitSession * session) {next_ =  session;}
  double log(Packet *p, int lowDemand);
  double getDropRate();
  void pushbackOn();
  void refreshed();
  void setAggSpec(AggSpec * aggSpec);
  void setLimit(double limit);
  double getArrivalRateForStatus();
  static RateLimitSession * merge(RateLimitSession *, RateLimitSession *, int bits);
 
 private:
  static double pick4merge(double, double, int);
};


class RateLimitSessionList {

 public:
  RateLimitSession * first_;
  int noSessions_;
  
  int IDCounter_;

  RateLimitSessionList(): first_(NULL), noSessions_(0), IDCounter_(0){};
  int insert(RateLimitSession * session); 
  int filter(Packet * p, int lowDemand);
  int containsAggSpec(AggSpec *);
  RateLimitSession * containsLocalAggSpec(AggSpec * spec, int myID);
  void mergeSessions(int myID);
  int noMySessions(int myID);
  void endSession(RateLimitSession *);
  int getMySubsetCount(int prefix, int bits, int myID);
  void merge(int prefix, int bits, int myID);
  RateLimitSession * getSessionByLocalID(int localID);
  RateLimitSession * getSessionByRemoteID(int remoteID);
  
  //returns number of sessions with sending rate strictly more than this rate.
	int rankRate(int myID, double rate);	
	int rankSession(int myID, RateLimitSession * session);
};


#endif
