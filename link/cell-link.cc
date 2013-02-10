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
    if ( strcmp(argv[1], "tick" ) == 0 ) {
      tick();
      return TCL_OK;
    }
  }
  return LinkDelay::command( argc, argv );
}

CellLink::CellLink() :
  _rate_generators( std::vector<RateGen>() ),
  _bits_dequeued( 0 ),
  _chosen_flows( std::vector<uint64_t>() )
{
  bind( "_iter",&_iter );
  bind( "_num_users", &_num_users );
  printf( "CellLink: _num_users %u, iter %u\n", _num_users, _iter );
  fflush( stdout );
  assert( _num_users > 0 );
  assert( _iter > 0 );
  for ( uint32_t i=0; i < _num_users; i++ ) {
    _rate_generators.push_back( RateGen ( ALLOWED_RATES[ i ] ) );
    _current_rates.push_back( 0 );
    _average_rates.push_back( 0 );
  }
  auto advance_substream = [&] ( RateGen r )
                           { for ( uint32_t i=1; i < _iter ; i++ ) r._rng->reset_next_substream();};
  std::for_each( _rate_generators.begin(), _rate_generators.end(), advance_substream );
}

void CellLink::tick()
{
  generate_new_rates();
  _chosen_flows = pick_user_to_schedule();
  update_average_rates( _chosen_flows.at( 0 ) );
}

std::vector<uint64_t> CellLink::pick_user_to_schedule()
{
  std::vector<double> normalized_rates( _current_rates.size() );
  std::transform( _current_rates.begin(), _current_rates.end(),
                 _average_rates.begin(), normalized_rates.begin(),
                 [&] ( const double & rate, const double & average)
                 { return (average != 0 ) ? rate/average : DBL_MAX ; } );

  /* Create a vector of (flow id,normalized rate) tuples */
  std::vector<std::pair<uint64_t,double>> normalized_rate_tuples( _current_rates.size() );
  for ( auto i = 0; i < normalized_rate_tuples.size(); i++ ) {
    normalized_rate_tuples[ i ] = std::make_pair( i, normalized_rates[ i ] );
  }

  /* Sort them in descending order of normalized_rates */
  std::sort( normalized_rate_tuples.begin(), normalized_rate_tuples.end(),
             [&] ( const std::pair<uint64_t,double> & p1, const std::pair<uint64_t,double> & p2 )
             { return p1.second > p2.second; }
           );

  /* Return flow ids alone from sorted tuple vector */
  std::vector<uint64_t> user_priorities( _current_rates.size() );
  std::transform( normalized_rate_tuples.begin(), normalized_rate_tuples.end(), user_priorities.begin(),
                  [&] ( const std::pair<uint64_t,double> &p ) { return p.first; } );
  return user_priorities;
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
}

void CellLink::update_average_rates( uint32_t scheduled_user )
{
  for ( uint32_t i=0; i < _average_rates.size(); i++ ) {
    if ( i == scheduled_user ) {
        printf(" Time %f Scheduled user is %d \n", Scheduler::instance().clock(), i );
      _average_rates[ i ] = ( 1.0 - 1.0/EWMA_SLOTS ) * _average_rates[ i ] + ( 1.0/ EWMA_SLOTS ) * _current_rates[ i ];
    } else {
      _average_rates[ i ] = ( 1.0 - 1.0/EWMA_SLOTS ) * _average_rates[ i ];
    }
  }
}

std::vector<uint64_t> CellLink::prop_fair_scheduler( void )
{
  return _chosen_flows;
}
