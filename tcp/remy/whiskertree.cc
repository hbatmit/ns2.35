#include <assert.h>
#include <math.h>
#include <algorithm>

#include "whiskertree.hh"

using namespace std;

const Whisker & WhiskerTree::use_whisker( const Memory & _memory ) const
{
  const Whisker * ret( whisker( _memory ) );

  if ( !ret ) {
    fprintf( stderr, "ERROR: No whisker found for memory: %s\n", _memory.str().c_str() );
    exit( 1 );
  }

  assert( ret );

  return *ret;
}

const Whisker * WhiskerTree::whisker( const Memory & _memory ) const
{
  if ( !_domain.contains( _memory ) ) {
    return NULL;
  }

  if ( is_leaf() ) {
    return &_leaf[ 0 ];
  }

  /* need to descend */
  for ( auto &x : _children ) {
    auto ret( x.whisker( _memory ) );
    if ( ret ) {
      return ret;
    }
  }

  assert( false );
  return NULL;
}

bool WhiskerTree::is_leaf( void ) const
{
  return !_leaf.empty();
}

WhiskerTree::WhiskerTree( const RemyBuffers::WhiskerTree & dna )
  : _domain( dna.domain() ),
    _children(),
    _leaf()
{
  if ( dna.has_leaf() ) {
    assert( dna.children_size() == 0 );
    _leaf.emplace_back( dna.leaf() );
  } else {
    assert( dna.children_size() > 0 );
    for ( const auto &x : dna.children() ) {
      _children.emplace_back( x );
    }
  }
}
