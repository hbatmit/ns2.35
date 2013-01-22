#include "sfd-dropper.h"
#include <algorithm>

SfdDropper::SfdDropper( std::map<uint64_t,PacketQueue*>* packet_queues ) :
  _packet_queues( packet_queues ),
  _pkt_dropper( new RNG() ),
  _queue_picker( new RNG() ),
  _iter ( 0 )
{}

uint64_t SfdDropper::longest_queue( void )
{
  /* Find maximum queue length */
  typedef std::pair<uint64_t,PacketQueue*> FlowQ;
  auto flow_compare = [&] (const FlowQ & T1, const FlowQ &T2 )
                       { return T1.second->length() < T2.second->length() ; };
  auto max_len=std::max_element( _packet_queues->begin(), _packet_queues->end(), flow_compare )->second->length();

  /* Find all flows that have the maximum queue length */
  std::vector<FlowQ> all_max_flows( _packet_queues->size() );
  auto filter_end =  std::remove_copy_if( _packet_queues->begin(), _packet_queues->end(), all_max_flows.begin(),
                       [&] (const FlowQ &T ) { return T.second->length() != max_len ; } );
  all_max_flows.erase( filter_end, all_max_flows.end() );

  /* Pick one at random */
  auto max_elem = all_max_flows.at( _queue_picker->uniform( (int) all_max_flows.size() ) ).first;
  return max_elem;
}

double SfdDropper::drop_probability( double fair_share, double arrival_rate )
{
  return std::max( 0.0, 1 - fair_share/arrival_rate );
}

void SfdDropper::set_iter( int iter )
{
  if ( iter == 0 ) {
    printf( " Iter can't be 0 \n" );
    exit(-5);
  }
  _iter = iter;
  for ( int i=1; i < _iter; i++ ) {
    _pkt_dropper->reset_next_substream();
    _queue_picker->reset_next_substream();
  }
}

bool SfdDropper::should_drop( double prob )
{
  return _pkt_dropper->next_double() <= prob ;
}
