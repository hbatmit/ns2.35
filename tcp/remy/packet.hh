#ifndef REMYPACKET_HH
#define REMYPACKET_HH

class RemyPacket
{
public:
  double tick_sent, tick_received;

  RemyPacket( const double & s_tick_sent, const double & s_tick_received )
    : tick_sent( s_tick_sent ), tick_received( s_tick_received )
  {}

  RemyPacket( const RemyPacket & other )
    : tick_sent( other.tick_sent ),
      tick_received( other.tick_received )
  {}
};

#endif
