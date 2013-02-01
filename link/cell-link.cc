#include "cell-link.h"
#include <algorithm>
#include <float.h>

static class CellLinkClass : public TclClass {
  public :
    CellLinkClass() : TclClass("CellLink") {}
    TclObject* create(int argc, const char*const* argv) {
      if ( argc != 6 ) {
        printf( " Invalid number of args to CellLink \n" );
        exit(-1);
      }
      return ( new CellLink( atoi(argv[4]), atoi(argv[5]) ) );
    }
} class_cell_link;

CellLink::CellLink( uint32_t num_users, uint32_t iteration_number ) :
  _num_users( num_users ),
  _current_rates( std::vector<double>( _num_users ) ),
  _average_rates( std::vector<double>( _num_users ) ),
  _rate_generators( std::vector<RNG*>( _num_users, new RNG() ) ),
  _iter( iteration_number )
{
  bind( "TIME_SLOT_DURATION", &TIME_SLOT_DURATION );
  bind( "EWMA_SLOTS", &EWMA_SLOTS );
  printf( "CellLink: TIME_SLOT_DURATION %f, EWMA_SLOTS %d , num_user %u, iter %u\n", TIME_SLOT_DURATION, EWMA_SLOTS, _num_users, _iter );
  fflush( stdout );
  auto advance_substream = [&] ( RNG *r )
                           { for ( uint32_t i=1; i < _iter ; i++ ) r->reset_next_substream();};
  std::for_each( _rate_generators.begin(), _rate_generators.end(), advance_substream );
}

void CellLink::tick()
{
  _current_slot++;
  generate_new_rates();
}

uint32_t CellLink::pick_user_to_schedule()
{
  std::vector<double> normalized_rates( _current_rates.size() );
  std::transform( _current_rates.begin(), _current_rates.end(),
                  _average_rates.begin(), normalized_rates.begin(),
                  [&] ( const double & rate, const double & average)
                  { return (average != 0 ) ? rate/average :  DBL_MAX ; } );
                  /* If average is zero, schedule preferentially */
  return std::distance( normalized_rates.begin(),
                        std::max_element( normalized_rates.begin(), normalized_rates.end() ) );
}

void CellLink::generate_new_rates()
{
  /* For now, generate new rates uniformly from allowed rates
   * TODO: fix this to exponential or random walk
   * */
  auto rate_generator = [&] ( RNG *r ) { return ALLOWED_RATES[ r->uniform( (int)ALLOWED_RATES.size() )]; };
  std::transform( _rate_generators.begin(), _rate_generators.end(),
                  _current_rates.begin(),
                  rate_generator );
}

bool CellLink::time_to_revise()
{
  /* For now, schedule on all slots and make slots
     big enough. TODO: Move to variable slots as in
     Page 211 of Tse and Viswanath. */
  return true;
}

std::vector<double> CellLink::get_current_rates()
{
  return _current_rates;
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

  /* Tick to get next set of rates */
  tick();
}
