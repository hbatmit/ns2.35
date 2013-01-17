#include "cell-link.h"
#include <algorithm>

static class CellLinkClass : public TclClass {
  public :
    CellLinkClass() : TclClass("CellLink") {}
    TclObject* create(int, const char*const*) {
      return (new CellLink( 0, 0 ));
    }
} class_cell_link;

int CellLink::command(int argc, const char*const* argv)
{
  if ( argc == 2 ) {
    if ( !strcmp( argv[1], "tick" ) ) {
      tick();
      return TCL_OK;
    }
  }
  return TclObject::command(argc,argv);
}

CellLink::CellLink( uint32_t num_users, uint32_t iteration_number ) :
  _num_users( num_users ),
  _current_user( uint32_t (-1) ),
  _current_rates( std::vector<double>( _num_users ) ),
  _average_rates( std::vector<double>( _num_users ) ),
  _rate_generators( std::vector<RNG*> ( _rate_generators ) ),
  _iter( iteration_number )
{
  bind( "_iter", &_iter );
  bind( "_num_users", &_num_users );
  auto advance_substream = [&] ( RNG *r ) 
                           { for ( uint32_t i=1; i < _iter ; i++ ) r->reset_next_substream();};
  std::for_each( _rate_generators.begin(), _rate_generators.end(), advance_substream );
}

void CellLink::tick()
{
  _current_slot++;
  generate_new_rates();
  bool schedule_now = time_to_revise();
  if (schedule_now) {
    _current_user = pick_user_to_schedule();
    Tcl& tcl = Tcl::instance();
    tcl.evalf("link_handle set bandwidth_ %f", _current_rates.at( _current_user ) );
    update_average_rates( _current_user );
  } else {
    update_average_rates( (uint32_t) -1 );
  }
}

uint32_t CellLink::pick_user_to_schedule()
{
  std::vector<double> normalized_rates( _current_rates.size() );
  std::transform( _current_rates.begin(), _current_rates.end(),
                  _average_rates.begin(), normalized_rates.begin(),
                  [&] ( const double & rate, const double & average)
                  { return rate/average ; } );
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

void CellLink::update_average_rates( uint32_t scheduled_user )
{
  for ( uint32_t i=0; i < _average_rates.size(); i++ ) {
    if ( i == scheduled_user ) {
     _average_rates[ i ] = ( 1.0 - 1.0/EWMA_SLOTS ) * _average_rates[ i ] + ( 1.0/ EWMA_SLOTS ) * _current_rates[ i ];
    } else {
     _average_rates[ i ] = ( 1.0 - 1.0/EWMA_SLOTS ) * _average_rates[ i ];
    }
  }
}

uint32_t CellLink::get_current_user()
{
  return _current_user;
}
