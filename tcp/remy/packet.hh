#ifndef REMYPACKET_HH
#define REMYPACKET_HH

class RemyPacket
{
public:
  unsigned int src;
  unsigned int flow_id;
  double tick_sent, tick_received;

  RemyPacket( const unsigned int & s_src,
              const unsigned int & s_flow_id,
              const double & s_tick_sent,
              const double & s_tick_received )
    : src( s_src ),
      flow_id( s_flow_id ),
      tick_sent( s_tick_sent ),
      tick_received( s_tick_received )
  {}

  RemyPacket( const RemyPacket & other )
    : src( other.src ),
      flow_id( other.flow_id ),
      tick_sent( other.tick_sent ),
      tick_received( other.tick_received )
  {}
};

#endif
