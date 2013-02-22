#include "cell-link.h"
#include <algorithm>
#include <float.h>
#include "queue/link-aware-queue.h"

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
    if ( strcmp(argv[1], "activate-link-scheduler" ) == 0 ) {
      _activate_link_scheduler=true;
      resched(  _slot_duration );
      return TCL_OK;
    }
  }
  if(argc==3) {
    if(!strcmp(argv[1],"attach-queue")) {
      _link_aware_queue = (LinkAwareQueue*) TclObject::lookup( argv[2] );
      return TCL_OK;
    }
  }
  return LinkDelay::command( argc, argv );
}

CellLink::CellLink() :
  _rate_generators( std::vector<RateGen>() ),
  _bits_dequeued( 0 ),
  chosen_flow( 0 ),
  _activate_link_scheduler( false ),
  _link_aware_queue( nullptr )
{
  bind( "_iter",&_iter );
  bind( "_num_users", &_num_users );
  bind( "_slot_duration", &_slot_duration );
  printf( "CellLink: _num_users %u, iter %u\n", _num_users, _iter );
  fflush( stdout );
  assert( _num_users > 0 );
  assert( _iter > 0 );
  for ( uint32_t i=0; i < _num_users; i++ ) {
    _rate_generators.push_back( RateGen ( ALLOWED_RATES[ i ] ) );
    _current_link_rates.push_back( 0 );
    _average_rates.push_back( 0 );
  }
  auto advance_substream = [&] ( RateGen r )
                           { for ( uint32_t i=1; i < _iter ; i++ ) r._rng->reset_next_substream();};
  std::for_each( _rate_generators.begin(), _rate_generators.end(), advance_substream );
}

void CellLink::tick()
{
  generate_new_rates();
  chosen_flow = pick_user_to_schedule();
  update_average_rates( chosen_flow );
}

uint64_t CellLink::pick_user_to_schedule() const
{
  /* First get the backlogged queues */
  std::vector<uint64_t> backlogged_flows = _link_aware_queue->backlogged_flowids();

  /* Normalize rates */
  std::vector<double> normalized_rates( _current_link_rates.size() );
  std::transform( _current_link_rates.begin(), _current_link_rates.end(),
                  _average_rates.begin(), normalized_rates.begin(),
                  [&] ( const double & rate, const double & average)
                  { return (average != 0 ) ? rate/average : DBL_MAX ; } );

  /* Pick the highest normalized rates amongst them */
  auto it = std::max_element( backlogged_flows.begin(), backlogged_flows.end(),
                              [&] (const uint64_t &f1, const uint64_t &f2)
                              { return normalized_rates.at( f1 ) < normalized_rates.at( f2 );});

  return (it!=backlogged_flows.end()) ? *it : (uint64_t)-1;
}

void CellLink::generate_new_rates()
{
  /* For now, generate new rates uniformly from allowed rates */
  auto rate_generator = [&] ( RateGen r )
                        { return r._allowed_rates[ r._rng->uniform( (int)r._allowed_rates.size() ) ]; };
  std::transform( _rate_generators.begin(), _rate_generators.end(),
                  _current_link_rates.begin(),
                  rate_generator );
}

void CellLink::recv( Packet* p, Handler* h )
{
  /* Get flow_id from packet */
  hdr_ip *iph=hdr_ip::access( p );
  uint32_t flow_id = iph->flowid();

  /* Find transmission rate for this flow */
  auto tx_rate = _current_link_rates.at( flow_id );

  /* Schedule transmission */
  Scheduler& s = Scheduler::instance();
  double tx_time = 8. * hdr_cmn::access(p)->size() / tx_rate ;

  /* If you are using the link's scheduler, assert correctness */
  if (_activate_link_scheduler) {
    assert( flow_id == chosen_flow );
    if (tx_time + Scheduler::instance().clock() > _last_time + _slot_duration) {
      /* TODO:slice packet */;
    }
  }

  /* Actually schedule */
  s.schedule(target_, p, tx_time + delay_ ); /* Propagation delay */
  s.schedule(h, &intr_,  tx_time ); /* Transmission delay */
  _bits_dequeued += (8 * hdr_cmn::access(p)->size());
}

void CellLink::update_average_rates( uint32_t scheduled_user )
{
  for ( uint32_t i=0; i < _average_rates.size(); i++ ) {
    if ( i == scheduled_user ) {
        printf(" Time %f Scheduled user is %d \n", Scheduler::instance().clock(), i );
      _average_rates[ i ] = ( 1.0 - 1.0/EWMA_SLOTS ) * _average_rates[ i ] + ( 1.0/EWMA_SLOTS ) * _current_link_rates[ i ];
    } else {
      _average_rates[ i ] = ( 1.0 - 1.0/EWMA_SLOTS ) * _average_rates[ i ];
    }
  }
}

void CellLink::expire( Event *e )
{
  /* TimerHandler::expire */
  tick();
  resched( _slot_duration );
  _last_time = Scheduler::instance().clock();
}
