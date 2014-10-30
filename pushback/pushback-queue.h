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

#ifndef ns_pushback_queue_h
#define ns_pushback_queue_h

#include "red.h"
#include "queue-monitor.h"
#include "timer-handler.h"
#include "pushback-constants.h"

class RateLimitSessionList;
class RateEstimator;

class PushbackQueueTimer;
class PushbackAgent;

class PushbackQueue: public REDQueue {

 public:
  PushbackQueue( const char* const);
  
  virtual void reportDrop(Packet *pkt);
  void enque(Packet *p);
  //  Packet * deque() {return NULL;}

  PushbackAgent * pushback_; 
  int command(int, const char*const*);
  void timeout(int);
  double getBW();
  double getRate();
  double getDropRate();
  int verbose_;

  RateLimitSessionList * rlsList_;
  RateEstimator * rateEstimator_;

 protected: 
  int pushbackID_;
  int src_;
  int dst_;
  int rate_limiting_;
  EDQueueMonitor * qmon_;
  PushbackQueueTimer * timer_;

  NsObject * RLDropTrace_;
};

class PushbackQueueTimer: public TimerHandler {

 public:
  PushbackQueueTimer(PushbackQueue * queue) {queue_ = queue;}

  virtual void
  expire(Event *e);
   
 protected:
  PushbackQueue * queue_;
  
 
};

#endif
