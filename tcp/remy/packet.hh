#ifndef REMYPACKET_HH
#define REMYPACKET_HH

class RemyPacket
{
public:
  unsigned int tick_sent, tick_received;

  RemyPacket( const unsigned int & s_tick_sent, const unsigned int & s_tick_received )
    : tick_sent( s_tick_sent ), tick_received( s_tick_received )
  {}

  RemyPacket( const RemyPacket & other )
    : tick_sent( other.tick_sent ),
      tick_received( other.tick_received )
  {}
};

#endif
