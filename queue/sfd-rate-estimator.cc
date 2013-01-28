#include "sfd-rate-estimator.h"
#include <algorithm>
#include <list>
#include <float.h>

SfdRateEstimator::SfdRateEstimator( double K, double headroom, double capacity ) :
  _K( K ),
  _headroom( headroom ),
  _capacity( capacity )
{}

double SfdRateEstimator::est_flow_arrival_rate( uint64_t flow_id, double now, Packet *p )
{
  /* If packet is NULL, do nothing */
  if ( p == NULL ) {
    return _flow_stats[ flow_id ]._flow_arrival_rate;
  }

  /* Extract packet length in bits from the header */
  double interarrival_time = now - _flow_stats[ flow_id ]._last_arrival;
  hdr_cmn* hdr  = hdr_cmn::access(p);
  uint32_t packet_size   = hdr->size() << 3;

  /* If you have simultaneous arrivals, coalesce packets */
  if ( interarrival_time == 0 ) {
    _flow_stats[ flow_id ]._acc_pkt_size += packet_size;
    if ( _flow_stats[ flow_id ]._flow_arrival_rate ) {
      return  _flow_stats[ flow_id ]._flow_arrival_rate ;
    } else {
      /* first packet, init. rate */
      return ( _flow_stats[ flow_id ]._flow_arrival_rate = _fair_share );
    }
  } else {
    packet_size += _flow_stats[ flow_id ]._acc_pkt_size;
    _flow_stats[ flow_id ]._acc_pkt_size = 0;
  }

  /* Apply EWMA with exponential weight, and update _last_arrival */
  _flow_stats[ flow_id ]._last_arrival = now;
  _flow_stats[ flow_id ]._flow_arrival_rate =
    (1.0 - exp(-interarrival_time/_K))*(double )packet_size/interarrival_time + exp(-interarrival_time/_K)*_flow_stats[ flow_id ]._flow_arrival_rate;

  return _flow_stats[ flow_id ]._flow_arrival_rate;
}

double SfdRateEstimator::est_flow_service_rate( uint64_t flow_id, double now, Packet *p )
{
  /* If packet is NULL, do nothing */
  if ( p == NULL ) {
    return _flow_stats[ flow_id ]._flow_service_rate;
  }

  /* Extract packet length in bits from the header */
  double interservice_time = now - _flow_stats[ flow_id ]._last_service;
  hdr_cmn* hdr  = hdr_cmn::access(p);
  uint32_t packet_size   = hdr->size() << 3;

  /* If you have simultaneous services, coalesce packets */
  if ( interservice_time == 0 ) {
    _flow_stats[ flow_id ]._acc_pkt_size += packet_size;
    if ( _flow_stats[ flow_id ]._flow_service_rate ) {
      return  _flow_stats[ flow_id ]._flow_service_rate ;
    } else {
      /* first packet, init. rate */
      return ( _flow_stats[ flow_id ]._flow_service_rate = _fair_share );
    }
  } else {
    packet_size += _flow_stats[ flow_id ]._acc_pkt_size;
    _flow_stats[ flow_id ]._acc_pkt_size = 0;
  }

  /* Apply EWMA with exponential weight, and update _last_service */
  _flow_stats[ flow_id ]._last_service = now;
  _flow_stats[ flow_id ]._flow_service_rate =
    (1.0 - exp(-interservice_time/_K))*(double )packet_size/interservice_time + exp(-interservice_time/_K)*_flow_stats[ flow_id ]._flow_service_rate;

  return _flow_stats[ flow_id ]._flow_service_rate;
}

double SfdRateEstimator::est_ingress_rate( void )
{
  /* Check if the link is congested */
  typedef std::pair<uint64_t,FlowStats> FlowStatsMap;
  return std::accumulate( _flow_stats.begin(), _flow_stats.end(), 0.0,
                         [&] ( const double &acc, const FlowStatsMap &f2 ) { return acc + f2.second._flow_arrival_rate ;} );


}

double SfdRateEstimator::est_fair_share( void )
{
  /* Estimate fair share rate based on desired individual rates */
  std::map<uint64_t,double> desired;
  std::map<uint64_t,double> current_share;
  std::map<uint64_t,double> final_share;

  for ( auto it = _flow_stats.begin(); it != _flow_stats.end(); it++ ) {
    auto flow_id = it->first;
    auto flow_rate = it->second._flow_arrival_rate;
    desired[ flow_id ] = flow_rate;
    current_share[ flow_id ] = 0;
  }
  auto capacity = _capacity * ( 1 - _headroom );

  /* First, determine all the allocations */
  while ( (!current_share.empty()) and (capacity > DBL_MIN ) ) {
    auto total_shares = current_share.size();
    auto unit_share   = capacity / total_shares ;
    std::list<uint64_t> user_list;
    for ( auto it = current_share.begin(); it != current_share.end(); it++ )
      user_list.push_back( it->first );
    for ( auto &user : user_list ) {
      auto fair_share = unit_share ;
      current_share[ user ] += fair_share;
      if ( current_share[ user ] >= desired[ user ] ) {
        auto spare_capacity = current_share[user] - desired[ user ];
        final_share[ user ] = desired[ user ];
        current_share.erase( current_share.find( user ) );
        capacity = capacity + spare_capacity - fair_share ;
      } else {
        capacity = capacity - fair_share;
      }
    }
  }

  /* finalize allocations */
  for ( auto &user_share : current_share ) {
    final_share[ user_share.first ] = current_share[ user_share.first ];
  }

  /* Determine fair share rate as max over all allocations */
  auto arg_max = std::max_element( final_share.begin(), final_share.end(),
                 [&] (const std::pair<uint64_t,double> & p1, const std::pair<uint64_t,double> &p2 )
                 { return p1.second < p2.second ;} );
  return ( _fair_share = arg_max->second ) ;
}

double SfdRateEstimator::est_virtual_egress_rate( void )
{
  /* "Virtual" because we multiply the output by headroom */
  return _capacity * ( 1 - _headroom );
  /* TODO : Fix this to dynamically estimate and return later */

}

void SfdRateEstimator::print_rates( double now )
{
  typedef std::pair<uint64_t,FlowStats> FlowStatsMap;
  /* Arrival rates */
  printf(" Time %f : A :  ", now );
  std::for_each( _flow_stats.begin() , _flow_stats.end(), [&] ( const FlowStatsMap s ) { printf(" %lu %f ", s.first, s.second._flow_arrival_rate); } );
  printf("\n");

  /* Service rates */
  printf(" Time %f : S :  ", now );
  std::for_each( _flow_stats.begin() , _flow_stats.end(), [&] ( const FlowStatsMap s ) { printf(" %lu %f ", s.first, s.second._flow_service_rate); } );
  printf("\n");

  /* Fair share of link */
  printf(" Time %f : F : %f \n", now, _fair_share );

  /* Ingress Arrival rate */
  printf(" Time %f : I : %f \n", now, est_ingress_rate() );
}
