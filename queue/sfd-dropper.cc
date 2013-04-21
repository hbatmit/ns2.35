#include "sfd-dropper.h"
#include <algorithm>

SfdDropper::SfdDropper( uint32_t iter ) :
  _pkt_dropper( new RNG() ),
  _iter ( iter )
{
  assert( iter != 0 );
  for ( int i=1; i < _iter; i++ ) {
    _pkt_dropper->reset_next_substream();
  }
}

double SfdDropper::drop_probability( double fair_share, double arrival_rate )
{
  return std::max( 0.0, 1 - fair_share/arrival_rate );
}

bool SfdDropper::should_drop( double prob )
{
  return _pkt_dropper->next_double() <= prob ;
}
