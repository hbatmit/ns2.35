#include "cell-link.h"
#include <algorithm>
#include <float.h>

static class CellLinkClass : public TclClass {
  public :
    CellLinkClass() : TclClass("DelayLink/CellLink") {}
    TclObject* create(int argc, const char*const* argv) {
      return ( new CellLink() );
    }
} class_cell_link;

int CellLink::command(int argc, const char*const* argv)
{
  if (argc == 2) {
    if ( strcmp(argv[1], "total" ) == 0 ) {
      fprintf( stderr, " Total number of bits is %lu \n", _bits_dequeued );
      return TCL_OK;
    }
  }
  return LinkDelay::command( argc, argv );
}

CellLink::CellLink() :
  _rate_generators( std::vector<RateGen>() ),
  _bits_dequeued( 0 ),
  _K( 0.2 ),
  _next_schedule( 0 ),
  _chosen_flow( 0 )
{
  bind( "_iter",&_iter );
  bind( "_num_users", &_num_users );
  printf( "CellLink: _num_users %u, iter %u\n", _num_users, _iter );
  fflush( stdout );
  assert( _num_users > 0 );
  assert( _iter > 0 );
  _current_rates = std::vector<double>( _num_users );
  for ( uint32_t i=0; i < _num_users; i++ ) {
    _rate_generators.push_back( RateGen ( ALLOWED_RATES[ i ] ) );
  }
  auto advance_substream = [&] ( RateGen r )
                           { for ( uint32_t i=1; i < _iter ; i++ ) r._rng->reset_next_substream();};
  std::for_each( _rate_generators.begin(), _rate_generators.end(), advance_substream );

  /* Tick once to start */
  tick();
}

void CellLink::tick()
{
  generate_new_rates();
}

void CellLink::generate_new_rates()
{
  /* For now, generate new rates uniformly from allowed rates
   * TODO: fix this to exponential or random walk
   * */
  auto rate_generator = [&] ( RateGen r ) { return r._allowed_rates[ r._rng->uniform( (int)r._allowed_rates.size() ) ]; };
  std::transform( _rate_generators.begin(), _rate_generators.end(),
                  _current_rates.begin(),
                  rate_generator );
}

void CellLink::recv( Packet* p, Handler* h )
{
  /* Get flow_id from packet */
  hdr_ip *iph=hdr_ip::access( p );
  auto flow_id = iph->flowid();

  /* Find transmission rate for this flow */
  auto tx_rate = _current_rates.at( flow_id );

  /* Schedule transmission */
  Scheduler& s = Scheduler::instance();
  double tx_time = 8. * hdr_cmn::access(p)->size() / tx_rate ;
  s.schedule(target_, p, tx_time + delay_ ); /* Propagation delay */
  s.schedule(h, &intr_,  tx_time ); /* Transmission delay */
  _bits_dequeued += (8 * hdr_cmn::access(p)->size());

  /* Add to _flow_stats if required */
  if ( _flow_stats.find( flow_id ) == _flow_stats.end() ) {
    _flow_stats[ flow_id ] = FlowStats( _K );
  }

  /* Update service rates for ALL flows */
  _flow_stats[ flow_id ].est_service_rate( Scheduler::instance().clock(), p );
  for ( auto f_it = _flow_stats.begin(); f_it != _flow_stats.end(); f_it ++ ) {
    if ( f_it->first != flow_id ) {
      _flow_stats[ f_it->first ].est_service_rate( Scheduler::instance().clock(), nullptr );
    }
  }

  /* Tick to get next set of rates */
  tick();
}

uint64_t CellLink::prop_fair_scheduler()
{
  auto now = Scheduler::instance().clock();
  if ( now < _next_schedule ) {
    return _chosen_flow;
  } else {
    auto current_link_rates = link_rates_as_map();
    std::map<uint64_t,double> avg_service_rates;
    for ( auto f_it = _flow_stats.begin(); f_it != _flow_stats.end(); f_it++ ) {
      avg_service_rates[ f_it->first ] = f_it->second._link_est.get_rate();
    }

    /* normalize rates to the average seen so far */
    std::map<uint64_t,double> normalized_rates;
    for ( auto it = current_link_rates.begin(); it != current_link_rates.end(); it++ ) {
      auto flow = it->first;
      normalized_rates[ flow ] = ( avg_service_rates.at( flow ) != 0 ) ? current_link_rates.at( flow )/avg_service_rates.at( flow ) : DBL_MAX ;
    }

    /* Pick the best proportionally fair rate, if avg is 0, schedule preferentially */
    auto iter = std::max_element( normalized_rates.begin(), normalized_rates.end(),
                                [&] (const std::pair<uint64_t,double> p1, const std::pair<uint64_t,double> p2 ) { return p1.second < p2.second; } );
    _next_schedule+=SCHEDULING_SLOT_SECS;
    return (_chosen_flow = iter->first);
  }
}

std::map<uint64_t,double> CellLink::link_rates_as_map( void )
{
  std::map<uint64_t,FlowStats>::iterator f_it;
  std::map<uint64_t,double> link_speeds;
  for ( f_it = _flow_stats.begin(); f_it != _flow_stats.end(); f_it++ ) {
    link_speeds[ f_it->first ]=_current_rates[ f_it->first ];
  }
  return link_speeds;
}
