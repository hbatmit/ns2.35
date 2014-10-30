#include "rate-estimator.h"
#include <math.h>

RateEstimator::RateEstimator(): k_(0.1), estRate_(0.0), bytesArr_(0.0), temp_size_(0) {
  prevTime_ = Scheduler::instance().clock();
  reset_time_ = Scheduler::instance().clock();
}

RateEstimator::RateEstimator(double estimate): k_(0.1), bytesArr_(0.0), temp_size_(0) {
  prevTime_ = Scheduler::instance().clock();
  reset_time_ = Scheduler::instance().clock();
  estRate_ = estimate;
}

void
RateEstimator::estimateRate(Packet * pkt) {

  hdr_cmn* hdr  = HDR_CMN(pkt);
  bytesArr_+= hdr->size();
  int pktSize   = hdr->size() << 3; /* length of the packet in bits */
  
  double now = Scheduler::instance().clock();
  double timeGap = ( now - prevTime_);

  if (timeGap == 0) {
    temp_size_ += pktSize;
    return;
  }
  else {
    pktSize+= temp_size_;
    temp_size_ = 0;
  }
	
  prevTime_ = now;
  estRate_ = (1 - exp(-timeGap/k_))*((double)pktSize)/timeGap + exp(-timeGap/k_)*estRate_;
}

  
void
RateEstimator::reset() {
  reset_time_ = Scheduler::instance().clock();
  bytesArr_ = 0;
//shoule the rate estimate be reset?
  //  estRate_=0.0;
  //  prevTime_= Scheduler::instance().clock();
  // temp_size_=0;

}
