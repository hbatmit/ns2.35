#include "sfd.h"
#include "rng.h"
#include <stdint.h>
#include <algorithm>
#include <float.h>

static class SFDClass : public TclClass {
  public:
    SFDClass() : TclClass("Queue/SFD") {}
    TclObject* create(int argc, const char*const* argv) {
      std::string q_args(argv[4]);
      char* stripped_str = (char*) q_args.substr(1, q_args.size() - 2).c_str();
      double user_arrival_rate_time_constant = atof(strtok(stripped_str, " "));
      double headroom = atof(strtok(nullptr, " "));
      double iter = atoi(strtok(nullptr," "));
      double user_id = atoi(strtok(nullptr," ")); 
      return new SFD(user_arrival_rate_time_constant, headroom, iter, user_id);
    }
} class_sfd;

int SFD::command(int argc, const char*const* argv)
{
  return EnsembleAwareQueue::command(argc, argv);
}

SFD::SFD(double user_arrival_rate_time_constant, double headroom, uint32_t iter, uint32_t user_id) :
  EnsembleAwareQueue(),
  _headroom(headroom),
  _iter(iter),
  _user_id(user_id),
  _packet_queue( new PacketQueue() ),
  _dropper(_iter),
  _user_arrival_rate_est(FlowStats(user_arrival_rate_time_constant))
{
  fprintf( stderr,  "SFD: _iter %d, _K %f, _headroom %f, user_id %d \n",
           _iter, user_arrival_rate_time_constant, _headroom, _user_id );
}

void SFD::enque(Packet *p)
{
  /* Implements pure virtual function Queue::enque() */

  /* Estimate arrival rate with an EWMA filter */
  double now = Scheduler::instance().clock();
  double arrival_rate = _user_arrival_rate_est.est_arrival_rate(now, p);

  /* Estimate aggregate arrival rate with an EWMA filter */
  double agg_arrival_rate = _scheduler->update_arrival_rate(now, p);

  /* Get fair share, and take max of fair share and service rate  */
  auto _fair_share = (1-_headroom) * _scheduler->get_fair_share(_user_id);
  _fair_share = std::max(_fair_share, _scheduler->get_service_rate(_user_id));
  //printf("User id is %d, _fair_share is %f \n", user_id, _fair_share);

  /* Print everything */
  //print_stats( now );

  /* Compute drop_probability */
  double drop_probability = (arrival_rate < _fair_share) ? 0.0 : 1.0 ;

  /* Check aggregate arrival rate and compare it to aggregate ideal pf throughput */
  bool exceeded_capacity = agg_arrival_rate > std::max(_scheduler->agg_pf_throughput(), _scheduler->agg_service_rate()) ;

  /* Enque packet */
  _packet_queue->enque( p );
 
  /* Toss a coin and drop */
  if ( !_dropper.should_drop( drop_probability ) ) {
   // printf( " Time %f : Not dropping packet, from flow %u drop_probability is %f\n", now, user_id, drop_probability );
  } else if ( !exceeded_capacity ) {
   // printf( " Time %f : Not dropping packet, from flow %u agg ingress %f, less than capacity %f \n", now, user_id, _scheduler->agg_arrival_rate(), _scheduler->agg_pf_throughput() );
  } else {
    /* Drop from front of the same queue */
  //  printf( " Time %f : Dropping packet, from flow %u drop_probability is %f\n", now, user_id, drop_probability );
    if (length() > 1) {
      Packet* head = _packet_queue->deque();
      if (head != 0 ) {
          drop( head );
      }
    }
  }
}

Packet* SFD::deque()
{
  /* Implements pure virtual function Queue::deque() */
  double now = Scheduler::instance().clock();
  Packet *p = _packet_queue->deque();
  //print_stats( now );
  return p;
}

void SFD::print_stats( double now )
{
  /* Queue sizes */
  printf(" Time %f : Q :  ", now );
  printf(" %u %d ", _user_id, _packet_queue->length());
  printf("\n");

  /* Arrival, Service, fair share, and ingress rates */
  _user_arrival_rate_est.print_rates(_user_id, now);
}
