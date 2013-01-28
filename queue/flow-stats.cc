#include "flow-stats.h"

FlowStats::FlowStats() :
  _last_arrival( 0.0 ),
  _last_service( 0.0 ),
  _acc_pkt_size( 0.0 ),
  _flow_arrival_rate( 0.0 ),
  _flow_service_rate( 0.0 ),
  _last_drop_prob( 0.0 )
{}
