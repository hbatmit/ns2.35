#include "sfd.h"
#include "rng.h"
#include <stdint.h>
#include <algorithm>
#include <float.h>

static class SFDClass : public TclClass {
  public:
    SFDClass() : TclClass("Queue/SFD") {}
    TclObject* create(int, const char*const*) {
      return (new SFD);
    }
} class_sfd;

int SFD::command(int argc, const char*const* argv)
{
  return LinkAwareQueue::command(argc, argv);
}

SFD::SFD() :
  LinkAwareQueue(),
  _packet_queues( std::map<uint64_t,PacketQueue*>() ),
  _dropper( SfdDropper( &_packet_queues ) ),
  _timestamps( std::map<uint64_t,std::queue<uint64_t>> () ),
  _counter( 0 ),
  _scheduler( &_packet_queues , &_timestamps ),
  _rate_estimator( 0.0, 0.0 )
{
  bind("_iter", &_iter );
  bind("_qdisc", &_qdisc );
  bind("_K", &_K );
  bind("_headroom", &_headroom );
  fprintf( stderr,  "SFD: _iter %d, _qdisc %d , _K %f, _headroom %f \n", _iter, _qdisc, _K, _headroom );
  _dropper.set_iter( _iter );
  _scheduler.set_iter( _iter );
  _scheduler.set_qdisc( _qdisc );
  _rate_estimator = SfdRateEstimator( _K, _headroom );
}

void SFD::enque(Packet *p)
{
  /* Implements pure virtual function Queue::enque() */

  /* Compute flow_id using hash */
  uint64_t flow_id = hash( p );

  /* Estimate arrival rate */
  double now = Scheduler::instance().clock();
  double arrival_rate = _rate_estimator.est_flow_arrival_rate( flow_id, now, p );

  /* Estimate current EWMA link rate for flow. */
  auto current_link_rates = get_link_rates();

  /* Check if flow has been seen earlier */
  if ( current_link_rates.find( flow_id ) == current_link_rates.end() ) {
    enque_packet( p, flow_id );
    return;
  }

  /* Divide Avg. link rate by # of active flows to get fair share */
  auto _fair_share = ( _rate_estimator.est_flow_link_rate( flow_id, now, current_link_rates.at( flow_id ) ) )/_packet_queues.size();

  /* Print everything */
  print_stats( now );

  /* Extract protocol (TCP vs UDP) from the header */
  hdr_cmn* hdr  = hdr_cmn::access(p);
  packet_t pkt_type   = hdr->ptype();

  /* Compute drop_probability */
  double drop_probability = std::max( 0.0 , 1 - _fair_share/arrival_rate );

  /* Toss a coin and drop */
  if ( !_dropper.should_drop( drop_probability ) ) {
    printf( " Time %f : Not dropping packet of type %d , from flow %lu drop_probability is %f\n", now, pkt_type, flow_id, drop_probability );
    enque_packet( p, flow_id );
  } else {
    /* Drop from front of the same queue */
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

  double now = Scheduler::instance().clock();
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
    Packet *p = _packet_queues.at( current_flow )->deque();
    _rate_estimator.est_flow_service_rate( current_flow, now, p );
    print_stats( now );
    return p;
  } else {
    _rate_estimator.est_flow_service_rate( current_flow, now, nullptr );
    print_stats( now );
    return 0; /* empty */
  }
}

void SFD::enque_packet( Packet* p, uint64_t flow_id )
{
  /* Push into flow queue, if it doesn't exist create one.
     Also push into _timestamps for FCFS */

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

void SFD::print_stats( double now )
{
  /* Queue sizes */
  printf(" Time %f : Q :  ", now );
  std::for_each( _packet_queues.begin(), _packet_queues.end(), [&] ( const std::pair<uint64_t,PacketQueue*> q ) { printf(" %lu %d ", q.first, q.second->length()); } );
  printf("\n");

  /* Arrival, Service, fair share, and ingress rates */
  _rate_estimator.print_rates( now );
}

std::map<uint64_t,double> SFD::get_link_rates( void )
{
  auto link_rates = _link->get_current_rates();
  std::map<uint64_t,PacketQueue*>::iterator q_it;
  std::map<uint64_t,double> link_speeds;

  for ( q_it = _packet_queues.begin(); q_it != _packet_queues.end(); q_it++ ) {
    link_speeds[ q_it->first ]=link_rates[ q_it->first ];
  }
  return link_speeds;
}
