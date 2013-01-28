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
  _dropper( SfdDropper( &_packet_queues ) ),
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
  fprintf( stderr,  "SFD: _iter %d, _capacity %f, _qdisc %d , _K %f, _headroom %f \n", _iter, _capacity, _qdisc, _K, _headroom );
  _dropper.set_iter( _iter );
  _scheduler.set_iter( _iter );
  _scheduler.set_qdisc( _qdisc );
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
  double arrival_rate = _rate_estimator.est_flow_arrival_rate( flow_id, now, p );

  /* Estimate fair share */
  auto _fair_share = _rate_estimator.est_fair_share() ;

  /* Estimate total ingress rate to check if the link is congested */
  double total_ingress = _rate_estimator.est_ingress_rate();

  /* Print everything */
  print_stats( now );

  /* Extract protocol (TCP vs UDP) from the header */
  hdr_cmn* hdr  = hdr_cmn::access(p);
  packet_t pkt_type   = hdr->ptype();
  double drop_probability = 0;
  if ( ( pkt_type == PT_CBR or pkt_type == PT_TCP ) and ( total_ingress >= _rate_estimator.est_virtual_egress_rate() ) ) {
    drop_probability = std::max( 0.0 , 1 - _fair_share/arrival_rate );
  }

  /* Toss a coin and drop */
  if ( !_dropper.should_drop( drop_probability ) ) {
    printf( " Time %f : Not dropping packet of type %d , from flow %lu drop_probability is %f\n", now, pkt_type, flow_id, drop_probability );
    enque_packet( p, flow_id );
  } else {
    /* find longest queue  and drop from front*/
    uint64_t drop_flow = flow_id ;
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

  double now = Scheduler::instance().clock();

  if ( _packet_queues.find( current_flow ) != _packet_queues.end() ) {
    if ( !_timestamps.at( current_flow ).empty() ) {
      _timestamps.at( current_flow ).pop();
    }
    Packet *p = _packet_queues.at( current_flow )->deque();
    _rate_estimator.est_flow_service_rate( current_flow, now, p );
    print_stats( now );
    return p;
  } else {
    print_stats( now );
    return 0; /* empty */
  }
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
  return ( iph->flowid() );
}

void SFD::print_stats( double now )
{
  /* Queue sizes */
  printf(" Time %f : Q :  ", now );
  std::for_each( _packet_queues.begin(), _packet_queues.end(), [&] ( const std::pair<uint64_t,PacketQueue*> q ) { printf(" %lu %d ", q.first, q.second->length()); } );
  printf("\n");

  /* Arrival, Service, fair share, and ingress rates */
  _rate_estimator.print_rates( now );
}
