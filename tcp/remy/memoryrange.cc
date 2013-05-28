#include "memoryrange.hh"

using namespace std;

bool MemoryRange::contains( const Memory & query ) const
{
  return (query >= _lower) && (query < _upper);
}

bool MemoryRange::operator==( const MemoryRange & other ) const
{
  return (_lower == other._lower) && (_upper == other._upper); /* ignore median estimator for now */
}

MemoryRange::MemoryRange( const RemyBuffers::MemoryRange & dna )
  : _lower( dna.lower() ),
    _upper( dna.upper() )
{
}

string MemoryRange::str( void ) const
{
  char tmp[ 256 ];
  snprintf( tmp, 256, "(lo: <%s> hi: <%s>)",
            _lower.str().c_str(),
            _upper.str().c_str() );
  return tmp;
}
