#ifndef FLOW_STATS_HH
#define FLOW_STATS_HH

#include "ewma-estimator.h"
#include "packet.h"

/*
   Book keeping information
   for flow statistics estimation
 */
class FlowStats {
  private :
    double    _first_arrival;
    double    _first_service;
    double    _acc_arrivals;
    double    _acc_services;
    uint32_t  _arrival_count;
    uint32_t  _service_count;

  public :
    EwmaEstimator _arr_est;
    EwmaEstimator _ser_est;
    EwmaEstimator _link_est;
    double _K;

    /* Constructor */
    FlowStats( double K );

    /* Default constructor */
    FlowStats() {};

    /* Estimate arrival rate */
    double est_arrival_rate( double now, Packet* p );

    /* Estimate service rate */
    double est_service_rate( double now, Packet* p );

    /* Estimate link rate */
    double est_link_rate( double now, double current_link_rate );

    /* Get packet size */
    uint32_t get_pkt_size( Packet* p );

    /* Print rates */
    void print_rates(uint32_t flow_id, double now) const;

};

#endif
