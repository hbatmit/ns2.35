#include "sfd-rate-estimator.h"
#include <algorithm>
#include <list>
#include <float.h>

SfdRateEstimator::SfdRateEstimator( double K, double headroom ) :
  _K( K ),
  _headroom( headroom )
{}

double SfdRateEstimator::est_flow_link_rate( uint64_t flow_id, double now, double current_link_rate )
{
  init_flow_stats( flow_id );
  return _flow_stats.at( flow_id ).est_link_rate( now, current_link_rate );
}

double SfdRateEstimator::est_flow_arrival_rate( uint64_t flow_id, double now, Packet *p )
{
  init_flow_stats( flow_id );
  return _flow_stats.at( flow_id ).est_arrival_rate( now, p );
}

double SfdRateEstimator::est_flow_service_rate( uint64_t flow_id, double now, Packet *p )
{
  init_flow_stats( flow_id );
  return _flow_stats.at( flow_id ).est_service_rate( now, p );
}

double SfdRateEstimator::est_ingress_rate( void )
{
  typedef std::pair<uint64_t,FlowStats> FlowStatsMap;
  return std::accumulate( _flow_stats.begin(), _flow_stats.end(), 0.0,
                         [&] ( const double &acc, const FlowStatsMap &f2 ) { return acc + f2.second._arr_est.get_rate() ;} );

}

void SfdRateEstimator::print_rates( double now )
{
  typedef std::pair<uint64_t,FlowStats> FlowStatsMap;
  /* Arrival rates */
  printf(" Time %f : A :  ", now );
  std::for_each( _flow_stats.begin() , _flow_stats.end(), [&] ( const FlowStatsMap s ) { printf(" %lu %f ", s.first, s.second._arr_est.get_rate()); } );
  printf("\n");

  /* Service rates */
  printf(" Time %f : S :  ", now );
  std::for_each( _flow_stats.begin() , _flow_stats.end(), [&] ( const FlowStatsMap s ) { printf(" %lu %f ", s.first, s.second._ser_est.get_rate()); } );
  printf("\n");

  /* Fair share of each user */
  printf(" Time %f : F :  ", now );
  std::for_each( _flow_stats.begin() , _flow_stats.end(), [&] ( const FlowStatsMap s ) { printf(" %lu %f ", s.first, s.second._link_est.get_rate() / _flow_stats.size() ); } );
  printf("\n");

  /* Ingress Arrival rate */
  printf(" Time %f : I : %f \n", now, est_ingress_rate() );
}

void SfdRateEstimator::init_flow_stats( uint32_t flow_id  )
{
  if ( _flow_stats.find( flow_id ) == _flow_stats.end() ) {
    printf(" Initializing FlowStats for flow %d \n", flow_id );
    _flow_stats[ flow_id ] = FlowStats( _K );
  }
}
