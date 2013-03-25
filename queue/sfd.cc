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
  if (argc == 3) {
    if (!strcmp(argv[1], "user_id")) {
      user_id=atoi(argv[2]);
      printf("Setting user_id to %d \n", user_id);
      return TCL_OK;
    }
  }
  return LinkAwareQueue::command(argc, argv);
}

SFD::SFD() :
  LinkAwareQueue(),
  _packet_queue( new PacketQueue() ),
  _dropper(),
  _rate_estimator( 0.0, 0.0 )
{
  bind("_iter", &_iter );
  bind("_qdisc", &_qdisc );
  bind("_K", &_K );
  bind("_headroom", &_headroom );
  fprintf( stderr,  "SFD: _iter %d, _qdisc %d , _K %f, _headroom %f \n", _iter, _qdisc, _K, _headroom );
  _dropper.set_iter( _iter );
  _rate_estimator = SfdRateEstimator( _K, _headroom );
}

void SFD::enque(Packet *p)
{
  /* Implements pure virtual function Queue::enque() */

  /* Estimate arrival rate with an EWMA filter */
  double now = Scheduler::instance().clock();
  double arrival_rate = _rate_estimator.est_flow_arrival_rate( user_id, now, p );

  /* Estimate current link rate with an EWMA filter. */
  printf("User id is %d \n", user_id);
  auto current_link_rate = _rate_estimator.est_flow_link_rate(user_id, now, _link->bandwidth());

  /* Divide Avg. link rate by # of active flows to get fair share */
  auto _fair_share = current_link_rate/_scheduler->num_users();

  /* Print everything */
  print_stats( now );

  /* Compute drop_probability */
  double drop_probability = (arrival_rate == 0) ? 0 : std::max( 0.0 , 1 - _fair_share/arrival_rate );

  /* Toss a coin and drop */
  if ( !_dropper.should_drop( drop_probability ) ) {
    printf( " Time %f : Not dropping packet, from flow %u drop_probability is %f\n", now, user_id, drop_probability );
    _packet_queue->enque( p );
  } else {
    /* Drop from front of the same queue */
    printf( " Time %f : Dropping packet, from flow %u drop_probability is %f\n", now, user_id, drop_probability );
    _packet_queue->enque(p);
    Packet* head = _packet_queue->deque();
    if (head != 0 ) {
        drop( head );
    }
  }
}

Packet* SFD::deque()
{
  /* Implements pure virtual function Queue::deque() */
  double now = Scheduler::instance().clock();
  Packet *p = _packet_queue->deque();
  _rate_estimator.est_flow_service_rate( user_id, now, p );
  print_stats( now );
  return p;
}

void SFD::print_stats( double now )
{
  /* Queue sizes */
  printf(" Time %f : Q :  ", now );
  printf(" %u %d ", user_id, _packet_queue->length());
  printf("\n");

  /* Arrival, Service, fair share, and ingress rates */
  _rate_estimator.print_rates( now );
}
