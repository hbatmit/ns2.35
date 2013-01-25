#include <assert.h>
#include <math.h>
#include <algorithm>

#include "whisker.hh"

using namespace std;

Whisker::Whisker( const RemyBuffers::Whisker & dna )
  : _window_increment( dna.window_increment() ),
    _window_multiple( dna.window_multiple() ),
    _intersend( dna.intersend() ),
    _domain( dna.domain() )
{
}
