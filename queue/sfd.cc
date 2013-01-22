#include "sfd.h"
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

SFD::SFD( double capacity ) :
  _packet_queues( std::map<uint64_t,PacketQueue*>() ),
  _timestamps( std::map<uint64_t,std::queue<uint64_t>> () ),
  _counter( 0 ),
  _scheduler( &_packet_queues , &_timestamps ),
  _rate_estimator( 0.0, 0.0, 0.0 )
{
  bind("_iter", &_iter );
  bind( "_capacity", &_capacity );
  bind("_qdisc", &_qdisc );
  bind("_K", &_K );
  bind("_headroom", &_headroom );
  /* Restrict advertised capacity right here */
  fprintf( stderr,  "SFD: _iter %d, _capacity %f, _qdisc %d , _K %f, _headroom %f \n", _iter, _capacity, _qdisc, _K, _headroom );
  _dropper = new RNG();
  _queue_picker = new RNG();
  _scheduler.set_iter( _iter );
  _scheduler.set_qdisc( _qdisc );
  for (int i=1; i < _iter ; i++ ) {
    _dropper->reset_next_substream();
    _queue_picker->reset_next_substream();
  }
  _rate_estimator = SfdRateEstimator( _K, _headroom, _capacity );
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
  double arrival_rate = _rate_estimator.est_flow_rate( flow_id, now, p );
  printf( " Time %f : Arrival rate estimate for flow %lu, src:%u, dst:%u is %f \n",
          now, flow_id, (int)(hdr_ip::access(p)->saddr()), (int)(hdr_ip::access(p)->daddr()), arrival_rate);

  /* Estimate fair share */
  auto _fair_share = _rate_estimator.est_fair_share() ;
  printf( " Time %f : Fair share estimate is %f\n", now, _fair_share );

  /* Estimate total ingress rate to check if the link is congested */
  double total_ingress = _rate_estimator.est_ingress_rate();

  /* Extract protocol (TCP vs UDP) from the header */
  hdr_cmn* hdr  = hdr_cmn::access(p);
  packet_t pkt_type   = hdr->ptype();
  double drop_probability = 0;
  if ( ( pkt_type == PT_CBR or pkt_type == PT_TCP ) and ( total_ingress >= _rate_estimator.est_virtual_egress_rate() ) ) {
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
    current_flow = _scheduler.fcfs_scheduler();
  } else if ( _qdisc == QDISC_RAND ) {
    current_flow = _scheduler.random_scheduler();
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

bool SFD::should_drop( double prob )
{
  /* Toss a biased coin */
  return _dropper->next_double() <= prob ;
}
