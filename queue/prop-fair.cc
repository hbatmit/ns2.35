#include "queue/prop-fair.h"
void PropFair::enque_packet( Packet* p, uint64_t flow_id )
{
  /* Push into flow queue, if it doesn't exist create one. */

  if ( _packet_queues.find( flow_id )  != _packet_queues.end() ) {
    _packet_queues.at( flow_id )->enque( p );
  } else {
    _packet_queues[ flow_id ] = new PacketQueue();
    _packet_queues.at( flow_id )->enque( p );
  }
}

void PropFair::enque(Packet *p)
{
  /* Implements pure virtual function Queue::enque() */

  /* Compute flow_id using hash */
  uint64_t flow_id = hash( p );

  /* Enque appropriately */
  enque_packet( p, flow_id );
}

Packet* PropFair::deque()
{
  /* Ask link for next flow to schedule */
  uint64_t flow_id =  _link->chosen_flow;
  return _packet_queues.at( flow_id )->deque();
}

std::vector<uint64_t> PropFair::backlogged_flowids( void ) const
{
  /* Return all backlogged flows if CellLink asks for it */
  std::vector<uint64_t> backlogged_flows;
  for (auto it=_packet_queues.begin(); it!=_packet_queues.end(); it++) {
    backlogged_flows.push_back( it->first );
  }
  return backlogged_flows;
}

std::map<uint64_t,double> PropFair::get_current_link_rates( void ) const
{
  return std::map<uint64_t,double>();
  /* Dummy, this function is not used anyway */
}
