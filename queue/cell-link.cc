#include "cell-link.h"
#include <algorithm>

void CellLink::tick( double now )
{
  _current_slot++;
  generate_new_rates();
  bool schedule_now = time_to_revise();
  if (schedule_now) {
    uint32_t scheduled_user = pick_user_to_schedule();
    Tcl& tcl = Tcl::instance();
    tcl.evalf("link_handle set bandwidth_ %f", _current_rates.at( scheduled_user ) );
    update_average_rates( scheduled_user );
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
  auto rate_generator = [&] ( RNG & r ) { return ALLOWED_RATES[ r.uniform( (int)ALLOWED_RATES.size() )]; };
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
