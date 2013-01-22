#include "sfd-scheduler.h"
#include <algorithm>

SfdScheduler::SfdScheduler( std::map<uint64_t,PacketQueue*> * packet_queues,
                            std::map<uint64_t,std::queue<uint64_t>> * timestamps ) :
  _packet_queues( packet_queues ),
  _qdisc( 0 ),
  _iter( 0 ),
  _timestamps( timestamps )
{}

uint64_t SfdScheduler::fcfs_scheduler( void )
{
  /* Implement FIFO by looking at arrival time of HOL pkt */
  typedef std::pair<uint64_t,std::queue<uint64_t>> FlowTs;
  auto flow_compare = [&] (const FlowTs & T1, const FlowTs &T2 )
                       { if (T1.second.empty()) return false;
                         else if(T2.second.empty()) return true;
                         else return T1.second.front() < T2.second.front() ; };
  return std::min_element( _timestamps->begin(), _timestamps->end(), flow_compare ) -> first;
}

uint64_t SfdScheduler::random_scheduler( void )
{
  /* Implement Random scheduler */
  typedef std::pair<uint64_t,PacketQueue*> FlowQ;
  std::vector<FlowQ> available_flows( _packet_queues->size() );
  auto filter_end = std::remove_copy_if( _packet_queues->begin(), _packet_queues->end(),
                                         available_flows.begin(),
                                         [&] (const FlowQ &T) { return T.second->length()==0; } );
  available_flows.erase( filter_end, available_flows.end() );
  return ( available_flows.empty() ) ? ( uint64_t ) -1 :
                                       available_flows.at( _rand_scheduler->uniform( (int)available_flows.size() ) ).first;
}

void SfdScheduler::set_qdisc( int qdisc )
{
  if ( _qdisc != 0 ) {
    printf( " Exit : _qdisc already set \n" );
    exit(-3);
  }
  if ( _iter == 0 ) {
    printf( " _iter is still  zero, exiting \n ");
    exit(-4);
  }
  _qdisc = qdisc;
  if ( _qdisc == QDISC_RAND ) {
    printf( "Using Rand \n");
    _rand_scheduler = new RNG();
    for (int i=1; i < _iter ; i++ ) {
      _rand_scheduler->reset_next_substream();
    }
  } else {
    printf( "Using Fcfs \n");
  }
}
