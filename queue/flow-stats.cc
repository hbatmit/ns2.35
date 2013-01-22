#include "flow-stats.h"

FlowStats::FlowStats() :
  _last_arrival( 0.0 ),
  _acc_pkt_size( 0.0 ),
  _flow_rate( 0.0 )
{}
