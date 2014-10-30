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

#include "rate-limit-strategy.h"

RateLimitStrategy::RateLimitStrategy(double rate, int ptype, double share, double estimate) {
  
  target_rate_ = rate;
  reset_time_ = Scheduler::instance().clock();
  
  ptype_ = ptype;
  ptype_share_ = share;
  
//  if (debug_)
//    printf("TB: Starting a token bucket at %g with rate %g bps\n", Scheduler::instance().clock(), rate);
  rateEstimator_ = new RateEstimator(estimate);
  rateLimiter_ = new TokenBucketRateLimiter();
  
  ptypeRateEstimator_ = new RateEstimator();
  ptypeRateLimiter_ = new TokenBucketRateLimiter();
  
  ptypeLog_ = new PacketTypeLog(this);
}

double
RateLimitStrategy::process(Packet *p, int mine, int lowDemand) {
  
  rateEstimator_->estimateRate(p);
  ptypeLog_->log(p);
  
  hdr_cmn* hdr = HDR_CMN(p);
    
  int dropped = 0;
  if (hdr->ptype() == (u_int)ptype_) {
    ptypeRateEstimator_->estimateRate(p);
    dropped = ptypeRateLimiter_->rateLimit(p, ptypeRateEstimator_->estRate_, 
					   target_rate_*ptype_share_, mine, lowDemand);
  }

  if (dropped) return 1;
  
  dropped = rateLimiter_->rateLimit(p, rateEstimator_->estRate_, target_rate_, mine, lowDemand);
  return dropped;
}

void
RateLimitStrategy::restrictPacketType(int type, double share, double actual) {
  
  if (type == ptype_) {
    return;
  }
  
  printf("RLSt: Restricting type %d to %g hogging=%g at %g \n",  type, share, actual, 
	 Scheduler::instance().clock());
  ptype_ = type;
  ptype_share_ = share;
  ptypeRateEstimator_->estRate_ = rateEstimator_->estRate_*actual;
}

double 
RateLimitStrategy::getDropRate() {
  double inRate = rateEstimator_->estRate_;
  double dropRate = 0;
  if (inRate > 0) {
	dropRate = (inRate - target_rate_)/inRate;
  }  
  if (dropRate < 0) dropRate=0;
  return dropRate;
}

double 
RateLimitStrategy::getArrivalRate() {
  return rateEstimator_->estRate_;
}

void
RateLimitStrategy::reset() {
  rateLimiter_->reset();
  ptypeRateLimiter_->reset();
  rateEstimator_->reset(); 
  ptypeRateEstimator_->reset();
}

RateLimitStrategy::~RateLimitStrategy() {
  delete(rateLimiter_);
  delete(ptypeRateLimiter_);
  delete(rateEstimator_);
  delete(ptypeRateEstimator_);
  delete(ptypeLog_);
}

//########################### PacketTypeLog Methods #####################

PacketTypeLog::PacketTypeLog(RateLimitStrategy * rlst) {
  count_=0;
  for (int i=0; i<MAX_PACKET_TYPES; i++) {
    typeCount_[i]=0;
  }
  rlStrategy_ = rlst;
  
  resched(PACKET_TYPE_TIMER);
}

void
PacketTypeLog::log(Packet *p) {
  hdr_cmn* hdr = HDR_CMN(p);
  int type = hdr->ptype();
  
  int index = mapTypeToIndex(type);
  //count packets instead of bytes
  typeCount_[index]++;
  count_++;
}

void
PacketTypeLog::expire(Event * ) {

  //printf("PTTimer Expiry at %g\n", Scheduler::instance().clock());

  if (!count_) {
    resched(PACKET_TYPE_TIMER);
    return;
  }
    
  for (int i=0; i<MAX_PACKET_TYPES; i++) {
    if (typeCount_[i]!=0) {
      double actualShare = ((double)typeCount_[i])/count_;
      int type = mapIndexToType(i);
      double maxShare = mapTypeToShare(type);
      if (actualShare>maxShare) {
	//right now only one type can be restricted.
	rlStrategy_->restrictPacketType(type, maxShare, actualShare);
	resched(PACKET_TYPE_TIMER);
	return;
      }
    }
  }
  
  //stop restricting if you were.
  rlStrategy_->restrictPacketType(-1, 1, 1);
  resched(PACKET_TYPE_TIMER);
  return;
}
  
int 
PacketTypeLog::mapTypeToIndex(int type) {
  
  switch (type) {
  case PT_PING: return 0;
  case PT_UDP: return 1;
  case PT_CBR: return 2;
  default: return MAX_PACKET_TYPES-1;
  }
}

int 
PacketTypeLog::mapIndexToType(int index) {
  switch (index) {
  case 0: return PT_PING;
  case 1: return PT_UDP;
  case 2: return PT_CBR;
  case MAX_PACKET_TYPES-1: return -1;
  default: printf("PTLog: invalid index\n"); exit(-1);
  }
}

//returning 1.0 means don't limit traffic of that type.
double
PacketTypeLog::mapTypeToShare(int type) {
  switch (type) {
  case PT_PING: return 1.0;
  case PT_UDP: return 1.0;
  case PT_CBR: return 1.0;
  default: return 1;
  }
}

PacketTypeLog::~PacketTypeLog() {
  cancel();
}

//########################### TokenBucketRateLimiter Methods ###################

TokenBucketRateLimiter::TokenBucketRateLimiter() {
  
  bucket_depth_ = DEFAULT_BUCKET_DEPTH;
  tbucket_ = bucket_depth_;
  total_passed_ = 0.0;
  total_dropped_ = 0.0;
  time_last_token_ = Scheduler::instance().clock();
}

int 
TokenBucketRateLimiter::rateLimit(Packet * p, double , double targetRate, int mine, int lowDemand) {

  hdr_cmn* hdr = HDR_CMN(p);
  double now = Scheduler::instance().clock();
  double time_elapsed = now - time_last_token_;
  
  //printf("TB: now = %g last_sent = %g elapsed = %g\n", now, time_last_token_, time_elapsed);
  tbucket_ += (time_elapsed * targetRate)/8.0;
  time_last_token_ = now;

  if (tbucket_ > bucket_depth_)
    tbucket_ = bucket_depth_;      /* never overflow */

  //printf("TB: tbucket_ = %g pktSize = %g\n", tbucket_, (double)hdr->size_);
  //printf("TB: in = %g out = %g\n", total_passed_, total_dropped_);

  if ((double)hdr->size_ < tbucket_ || (mine && lowDemand) ) {
    tbucket_ -= hdr->size_;
    total_passed_ += (double) hdr->size_;
    //printf("Passed Packet in Rate Limiter\n");
    return 0;
  } 
  else {
    total_dropped_ += (double) hdr->size_;
    //    printf("Dropped Packet in Rate Limiter\n");
    return 1;
  }
}

// double 
// TokenBucketRateLimiter::getDropRate() { 
//   if (getArrivals() == 0) 
//     return 0; 
//   else 
//     return total_dropped_/(total_passed_+total_dropped_);
// }
  
void
TokenBucketRateLimiter::reset() {
  total_dropped_ = 0;
  total_passed_ = 0;
  tbucket_ = 0;
  time_last_token_ = Scheduler::instance().clock();
}

