#include <stdio.h>

#include "whisker.hh"

using namespace std;

Whisker::Whisker( const RemyBuffers::Whisker & dna )
  : _window_increment( dna.window_increment() ),
    _window_multiple( dna.window_multiple() ),
    _intersend( dna.intersend() ),
    _domain( dna.domain() )
{
}

string Whisker::str( void ) const
{
  char tmp[ 256 ];
  snprintf( tmp, 256, "{%s} => (win: %d + (%f * win) intersend: %.2f ms)",
            _domain.str().c_str(), _window_increment, _window_multiple, _intersend );
  return tmp;
}
