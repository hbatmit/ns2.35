#ifndef FLOW_STATS_HH
#define FLOW_STATS_HH

/*
   Book keeping information
   for flow statistics estimation
 */
class FlowStats {
  public :
    double _last_arrival;
    double _last_service;
    double _acc_arr_pkt_size;
    double _acc_dep_pkt_size;
    double _flow_arrival_rate;
    double _flow_service_rate;
    double _last_drop_prob;
    FlowStats();
};

#endif
