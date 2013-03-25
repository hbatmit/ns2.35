#include "sfd-dropper.h"
#include <algorithm>

SfdDropper::SfdDropper() :
  _pkt_dropper( new RNG() ),
  _iter ( 0 )
{}

double SfdDropper::drop_probability( double fair_share, double arrival_rate )
{
  return std::max( 0.0, 1 - fair_share/arrival_rate );
}

void SfdDropper::set_iter( int iter )
{
  if ( iter == 0 ) {
    printf( " Iter can't be 0 \n" );
    exit(-5);
  }
  _iter = iter;
  for ( int i=1; i < _iter; i++ ) {
    _pkt_dropper->reset_next_substream();
  }
}

bool SfdDropper::should_drop( double prob )
{
  return _pkt_dropper->next_double() <= prob ;
}
