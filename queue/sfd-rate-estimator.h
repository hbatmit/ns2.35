#ifndef SFD_RATE_ESTIMATOR_HH
#define SFD_RATE_ESTIMATOR_HH

#include <map>
#include <stdint.h>
#include <math.h>
#include "flow-stats.h"
#include "queue.h"

class SfdRateEstimator {
  /*
     Wrapper class for all Rate Estimators
     in SFD. In particular, it estimates
     * Per flow arrival rate
     * Per flow service rate
     * Total ingress rate
     * Fair share rate of link
   */
  private :
    /* Per flow stats for rate estimation (borrowed from CSFQ) */
    std::map<uint64_t,FlowStats> _flow_stats;
    double  _K;        /* default : 200 ms */

    /* init. flow data structures */
    void init_flow_stats( uint32_t flow_id );

  public :
    /* Constructor */
    SfdRateEstimator( double _K );

    /* Per flow arrival rate */
    double est_flow_arrival_rate( uint64_t flow_id, double now, Packet* p );

    /* Per flow service rate */
    double est_flow_service_rate( uint64_t flow_id, double now, Packet* p );

    /* Per flow link rate */
    double est_flow_link_rate( uint64_t flow_id, double now, double current_link_rate );

    /* Total ingress rate */
    double est_ingress_rate() const;

    /* Print all rates */
    void print_rates( double now ) const;

};

#endif
