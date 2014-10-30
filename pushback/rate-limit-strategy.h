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

#ifndef ns_rate_limit_strategy_h
#define ns_rate_limit_strategy_h

#include "pushback-constants.h"
#include "packet.h"
#include "rate-estimator.h"
#include "timer-handler.h"

//container class for rate limiter
class RateLimiter {
 public:  
  virtual ~RateLimiter () {}
  virtual int rateLimit(Packet * p, double estRate, double targetRate, 
			int mine, int lowDemand)=0;
  virtual void reset()=0;
};

class PacketTypeLog;

class RateLimitStrategy {
 public:
  double target_rate_;          //predefined flow rate in bps 
  double reset_time_;           //time since the aggregate stats are being collected         
  
  RateEstimator * rateEstimator_; 
  RateLimiter * rateLimiter_;

  int ptype_;
  double ptype_share_;
  RateEstimator * ptypeRateEstimator_;
  RateLimiter * ptypeRateLimiter_;

  PacketTypeLog * ptypeLog_;

  RateLimitStrategy(double rate, int ptype, double share, double estimate);
  ~RateLimitStrategy();
  double process(Packet * p, int mine, int lowDemand);
  void restrictPacketType(int type, double share, double actual);
  
  double getDropRate();  
  double getArrivalRate();
  void reset();
};

class PacketTypeLog : public TimerHandler {
  
 public:
  int count_;
  int typeCount_[MAX_PACKET_TYPES];
  RateLimitStrategy * rlStrategy_;
  PacketTypeLog(RateLimitStrategy *);
  virtual ~PacketTypeLog();
  
  void log(Packet *);
  static int mapTypeToIndex(int type);
  static int  mapIndexToType(int index);
  static double mapTypeToShare(int type);
  
 protected:
  void expire(Event *e);
  
};
  
class TokenBucketRateLimiter: public RateLimiter {
 public:
  //the static parameters   
  double bucket_depth_;             //depth of the token bucket in bytes
   
  //the dynamic state
  double tbucket_;                 //number of tokens in the bucket; in bytes
  double time_last_token_;
  double total_passed_;
  double total_dropped_;
  
  TokenBucketRateLimiter();
  virtual ~TokenBucketRateLimiter () {}
  int rateLimit(Packet * p, double estRate, double targetRate, int mine, int lowDemand);
  void reset(); 
};

class PrefDropRateLimiter : public RateLimiter {
 public:
  //state for Pref Drop Implementation Goes Here
  
/*   virtual double process(Packet *p) { */
/*     //todo" stuff here */
/*     return 0; */
/*   } */
};


#endif
