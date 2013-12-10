#ifndef MEMORY_HH
#define MEMORY_HH

#include <vector>
#include <string>

#include "packet.hh"
#include "dna.pb.h"

class Memory {
public:
  typedef double DataType;

private:
  DataType _rec_send_ewma;
  DataType _rec_rec_ewma;
  DataType _rtt_ratio;
  DataType _slow_rec_rec_ewma;

  double _last_tick_sent;
  double _last_tick_received;
  double _min_rtt;

public:
  Memory()
    : _rec_send_ewma( 0 ),
      _rec_rec_ewma( 0 ),
      _rtt_ratio( 0.0 ),
      _slow_rec_rec_ewma( 0.0 ),
      _last_tick_sent( 0 ),
      _last_tick_received( 0 ),
      _min_rtt( 0 )
  {}

  void reset( void ) { _rec_send_ewma = _rec_rec_ewma = _rtt_ratio = _slow_rec_rec_ewma = _last_tick_sent = _last_tick_received = _min_rtt = 0; }

  void packet_sent( const RemyPacket & packet __attribute((unused)) ) {}
  void packets_received( const std::vector< RemyPacket > & packets, const unsigned int flow_id );
  void advance_to( const unsigned int tickno __attribute((unused)) ) {}

  bool operator>=( const Memory & other ) const { return (_rec_send_ewma >= other._rec_send_ewma) && (_rec_rec_ewma >= other._rec_rec_ewma) && (_rtt_ratio >= other._rtt_ratio) && (_slow_rec_rec_ewma >= other._slow_rec_rec_ewma); }
  bool operator<( const Memory & other ) const { return (_rec_send_ewma < other._rec_send_ewma) && (_rec_rec_ewma < other._rec_rec_ewma) && (_rtt_ratio < other._rtt_ratio) && (_slow_rec_rec_ewma < other._slow_rec_rec_ewma); }
  bool operator==( const Memory & other ) const { return (_rec_send_ewma == other._rec_send_ewma) && (_rec_rec_ewma == _rec_rec_ewma) && (_rtt_ratio == other._rtt_ratio) && (_slow_rec_rec_ewma == other._slow_rec_rec_ewma); }

  Memory( const RemyBuffers::Memory & dna );

  std::string str( void ) const;
};

#endif
