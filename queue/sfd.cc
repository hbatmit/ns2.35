#include "sfd.h"
#include <math.h>
#include "rng.h"
#include <stdint.h>
#include <algorithm>
#include <float.h>

static class SFDClass : public TclClass {
  public:
    SFDClass() : TclClass("Queue/SFD") {}
    TclObject* create(int, const char*const*) {
      return (new SFD(0));
    }
} class_sfd;

int SFD::command(int argc, const char*const* argv) 
{
  if (argc == 3) {
    if (!strcmp(argv[1], "capacity")) {
     _capacity=atof(argv[2]);
     return (TCL_OK);
    }
  }
  return Queue::command(argc, argv);
}

FlowStats::FlowStats() :
  _last_arrival( 0.0 ),
  _acc_pkt_size( 0.0 ),
  _flow_rate( 0.0 )
{}

SFD::SFD( double capacity ) :
  _counter( 0.0 )
{ 
  bind("_iter", &_iter ); 
  bind( "_capacity", &_capacity );
  bind("_qdisc", &_qdisc );
  bind("_K", &_K );
  bind("_headroom", &_headroom );
  /* Restrict advertised capacity right here */
  printf( "Using head_room of %f \n", _headroom );
  _capacity = _capacity * (1 - _headroom);
  fprintf( stderr,  "SFD: _iter %d, _capacity %f, _qdisc %d , _K %f \n", _iter, _capacity, _qdisc, _K );
  _dropper = new RNG();
  _queue_picker = new RNG();
  if ( _qdisc == QDISC_RAND ) {
    printf( "Using Rand \n");
    _rand_scheduler = new RNG();
  } else {
    printf( "Using Fcfs \n");
  }
  for (int i=1; i < _iter ; i++ ) {
    _dropper->reset_next_substream();
    _queue_picker->reset_next_substream();
    if ( _qdisc == QDISC_RAND ) {
     _rand_scheduler->reset_next_substream();
    }
  }
}

void SFD::enque(Packet *p)
{
  /* Implements pure virtual function Queue::enque() */

  /* Compute flow_id using hash */
  uint64_t flow_id = hash( p ); 

  /* If it's an ACK, simply enque */
  if (hdr_cmn::access(p)->ptype() == PT_ACK ) {
   enque_packet(p, flow_id);
   return;
  }

  /* Estimate arrival rate */
  double now = Scheduler::instance().clock();
  double arrival_rate = est_flow_rate( flow_id, now, p );
  printf( " Time %f : Arrival rate estimate for flow %lu, src:%u, dst:%u is %f \n",
          now, flow_id, (int)(hdr_ip::access(p)->saddr()), (int)(hdr_ip::access(p)->daddr()), arrival_rate);

  /* Estimate fair share */
  _fair_share = est_fair_share() ;
  printf( " Time %f : Fair share estimate is %f\n", now, _fair_share );

  /* Check if the link is congested */
  typedef std::pair<uint64_t,FlowStats> FlowStatsMap;
  double total_ingress = std::accumulate( _flow_stats.begin(), _flow_stats.end(), 0.0,
                         [&] ( const double &acc, const FlowStatsMap &f2 ) { return acc + f2.second._flow_rate ;} );

  /* Extract protocol (TCP vs UDP) from the header */
  hdr_cmn* hdr  = hdr_cmn::access(p); 
  packet_t pkt_type   = hdr->ptype();
  double drop_probability = 0;
  if ( ( pkt_type == PT_CBR or pkt_type == PT_TCP ) and ( total_ingress >= _capacity ) ) {
    drop_probability = std::max( 0.0 , 1 - _fair_share/arrival_rate );
  }

  /* Toss a coin and drop */  
  if ( !should_drop( drop_probability ) ) {
    printf( " Time %f : Not dropping packet of type %d , from flow %lu drop_probability is %f\n", now, pkt_type, flow_id, drop_probability );
    enque_packet( p, flow_id );
  } else {
    /* find longest queue  and drop from front*/
    uint64_t drop_flow = longest_queue();
    printf( " Time %f : Dropping packet of type %d, from flow %lu drop_probability is %f\n", now, pkt_type, drop_flow, drop_probability );
    enque_packet( p, flow_id );
    Packet* head = _packet_queues.at( drop_flow )->deque();
    if (head != 0 ) {
        drop( head );
    }
    if ( !_timestamps.at( drop_flow ).empty() ) {
      _timestamps.at( drop_flow ).pop();
    }
  }
}

Packet* SFD::deque()
{
  /* Implements pure virtual function Queue::deque() */
  uint64_t current_flow = (uint64_t)-1;
  if ( _qdisc == QDISC_FCFS ) {
    current_flow = fcfs_scheduler();
  } else if ( _qdisc == QDISC_RAND ) {
    current_flow = random_scheduler();
  } else {
    assert( false );
  }

  if ( _packet_queues.find( current_flow ) != _packet_queues.end() ) {
    if ( !_timestamps.at( current_flow ).empty() ) {
      _timestamps.at( current_flow ).pop();
    }
    return _packet_queues.at( current_flow )->deque();
  } else {
    return 0; /* empty */
  }
}

uint64_t SFD::longest_queue( void )
{
  /* Find maximum queue length */
  typedef std::pair<uint64_t,PacketQueue*> FlowQ;
  auto flow_compare = [&] (const FlowQ & T1, const FlowQ &T2 )
                       { return T1.second->length() < T2.second->length() ; };
  auto max_len=std::max_element( _packet_queues.begin(), _packet_queues.end(), flow_compare )->second->length();

  /* Find all flows that have the maximum queue length */
  std::vector<FlowQ> all_max_flows( _packet_queues.size() );
  auto filter_end =  std::remove_copy_if( _packet_queues.begin(), _packet_queues.end(), all_max_flows.begin(),
                       [&] (const FlowQ &T ) { return T.second->length() != max_len ; } );
  all_max_flows.erase( filter_end, all_max_flows.end() );

  /* Pick one at random */
  auto max_elem = all_max_flows.at( _queue_picker->uniform( (int) all_max_flows.size() ) ).first;
  return max_elem;
}

uint64_t SFD::fcfs_scheduler( void ) 
{
  /* Implement FIFO by looking at arrival time of HOL pkt */
  typedef std::pair<uint64_t,std::queue<uint64_t>> FlowTs;
  auto flow_compare = [&] (const FlowTs & T1, const FlowTs &T2 )
                       { if (T1.second.empty()) return false;
                         else if(T2.second.empty()) return true;
                         else return T1.second.front() < T2.second.front() ; };
  return std::min_element( _timestamps.begin(), _timestamps.end(), flow_compare ) -> first;
}

uint64_t SFD::random_scheduler( void ) 
{
  /* Implement Random scheduler */
  typedef std::pair<uint64_t,PacketQueue*> FlowQ;
  std::vector<FlowQ> available_flows( _packet_queues.size() );
  auto filter_end = std::remove_copy_if( _packet_queues.begin(), _packet_queues.end(),
                                         available_flows.begin(),
                                         [&] (const FlowQ &T) { return T.second->length()==0; } );
  available_flows.erase( filter_end, available_flows.end() );
  return ( available_flows.empty() ) ? ( uint64_t ) -1 :
                                       available_flows.at( _rand_scheduler->uniform( (int)available_flows.size() ) ).first;
}

void SFD::enque_packet( Packet* p, uint64_t flow_id ) 
{
  if ( _packet_queues.find( flow_id )  != _packet_queues.end() ) {
    _packet_queues.at( flow_id )->enque( p );
    _timestamps.at( flow_id ).push( ++_counter );
  } else {
    _packet_queues[ flow_id ] = new PacketQueue();
    _timestamps[ flow_id ] = std::queue<uint64_t>();
    _packet_queues.at( flow_id )->enque( p );
    _timestamps.at( flow_id ).push( ++_counter );
  }
}

uint64_t SFD::hash(Packet* pkt)
{
  hdr_ip *iph=hdr_ip::access(pkt);
  return ( iph->flowid() ); // modulo a large prime
}

double SFD::est_flow_rate( uint64_t flow_id, double now, Packet *p ) 
{
  /* Extract packet length in bits from the header */
  double interarrival_time = now - _flow_stats[ flow_id ]._last_arrival;
  hdr_cmn* hdr  = hdr_cmn::access(p); 
  uint32_t packet_size   = hdr->size() << 3;
 
  /* If you have simultaneous arrivals, coalesce packets */ 
  if ( interarrival_time == 0 ) {
    _flow_stats[ flow_id ]._acc_pkt_size += packet_size;
    if ( _flow_stats[ flow_id ]._flow_rate ) {
      return  _flow_stats[ flow_id ]._flow_rate ;
    } else {
      /* first packet, init. rate */
      return ( _flow_stats[ flow_id ]._flow_rate = _fair_share );
    }
  } else {
    packet_size += _flow_stats[ flow_id ]._acc_pkt_size;
    _flow_stats[ flow_id ]._acc_pkt_size = 0;
  }

  /* Apply EWMA with exponential weight, and update _last_arrival */
  _flow_stats[ flow_id ]._last_arrival = now;
  _flow_stats[ flow_id ]._flow_rate = 
    (1.0 - exp(-interarrival_time/_K))*(double )packet_size/interarrival_time + exp(-interarrival_time/_K)*_flow_stats[ flow_id ]._flow_rate;

  return _flow_stats[ flow_id ]._flow_rate;
}

double SFD::est_fair_share() 
{
  /* Estimate fair share rate based on desired individual rates */
  std::map<uint64_t,double> desired;
  std::map<uint64_t,double> current_share;
  std::map<uint64_t,double> final_share;

  for ( auto it = _flow_stats.begin(); it != _flow_stats.end(); it++ ) {
    auto flow_id = it->first;
    auto flow_rate = it->second._flow_rate;
    desired[ flow_id ] = flow_rate;
    current_share[ flow_id ] = 0;
  }
  auto capacity = _capacity ;

  /* First, determine all the allocations */
  while ( (!current_share.empty()) and (capacity > DBL_MIN ) ) {
    auto total_shares = current_share.size();
    auto unit_share   = capacity / total_shares ;
    std::list<uint64_t> user_list;
    for ( auto it = current_share.begin(); it != current_share.end(); it++ )
      user_list.push_back( it->first );
    for ( auto &user : user_list ) {
      auto fair_share = unit_share ; 
      current_share[ user ] += fair_share;
      if ( current_share[ user ] >= desired[ user ] ) {
        auto spare_capacity = current_share[user] - desired[ user ];
        final_share[ user ] = desired[ user ];
        current_share.erase( current_share.find( user ) );
        capacity = capacity + spare_capacity - fair_share ;
      } else {
        capacity = capacity - fair_share;
      }
    }
  }

  /* finalize allocations */
  for ( auto &user_share : current_share ) {
    final_share[ user_share.first ] = current_share[ user_share.first ];
  }

  /* Determine fair share rate as max over all allocations */
  auto arg_max = std::max_element( final_share.begin(), final_share.end(), 
                 [&] (const std::pair<uint64_t,double> & p1, const std::pair<uint64_t,double> &p2 )
                 { return p1.second < p2.second ;} );
  return arg_max->second ;
}

bool SFD::should_drop( double prob )
{
  /* Toss a biased coin */
  return _dropper->next_double() <= prob ;
}
