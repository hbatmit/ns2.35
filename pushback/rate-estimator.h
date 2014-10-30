#ifndef ns_rate_estimator_h
#define ns_rate_estimator_h

#include "packet.h"

//copied over from csfq.cc (Stoica)
class RateEstimator {
 public:
  double k_;                    /* averaging interval for rate estimation in seconds*/
  double estRate_;              /* current flow's estimated rate in bps */
  double bytesArr_;
  
  RateEstimator();
  RateEstimator(double estimate);
  void estimateRate(Packet *p);
  void reset();
  
protected:
  int temp_size_;               /* keep track of packets that arrive at the same time */
  double prevTime_;             /* time of last packet arrival */
  double reset_time_;
};
  
  
#endif
