#ifndef WHISKER_HH
#define WHISKER_HH

#include <string>

#include "memoryrange.hh"
#include "dna.pb.h"

class Whisker {
private:
  int _window_increment;
  double _window_multiple;
  double _intersend;

  MemoryRange _domain;

public:
  unsigned int window( const unsigned int previous_window ) const { return std::max( 0, int( previous_window * _window_multiple + _window_increment ) ); }
  const double & intersend( void ) const { return _intersend; }
  const MemoryRange & domain( void ) const { return _domain; }

  Whisker( const RemyBuffers::Whisker & dna );
  std::string str( void ) const;
};

#endif
