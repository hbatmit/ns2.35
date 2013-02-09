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
  _chosen_flow( 0 )
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
  _chosen_flow = pick_user_to_schedule();
  update_average_rates( _chosen_flow );
}

uint32_t CellLink::pick_user_to_schedule()
{
  std::vector<double> normalized_rates( _current_rates.size() );
  std::transform( _current_rates.begin(), _current_rates.end(),
                 _average_rates.begin(), normalized_rates.begin(),
                 [&] ( const double & rate, const double & average)
                 { return (average != 0 ) ? rate/average : DBL_MAX ; } );
  return std::distance( normalized_rates.begin(),
                       std::max_element( normalized_rates.begin(), normalized_rates.end() ) );
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
  printf( " Chosen flow is %lu \n", _chosen_flow );
  /* Get flow_id from packet */
  hdr_ip *iph=hdr_ip::access( p );
  auto flow_id = iph->flowid();

  printf(" Time %f CellLink::recv on flow %lu, _chosen_flow is %lu \n", Scheduler::instance().clock(), flow_id, _chosen_flow );
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
        printf("Average rate is %f \n", _average_rates[ i ] );
      _average_rates[ i ] = ( 1.0 - 1.0/EWMA_SLOTS ) * _average_rates[ i ];
    }
  }
}

uint64_t CellLink::prop_fair_scheduler( void )
{
  return _chosen_flow;
}
