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
      std::string drop_type(strtok(nullptr," "));
      return new SFD(user_arrival_rate_time_constant, headroom, iter, user_id, drop_type);
    }
} class_sfd;

int SFD::command(int argc, const char*const* argv)
{
  return EnsembleAwareQueue::command(argc, argv);
}

SFD::SFD(double user_arrival_rate_time_constant, double headroom,
         uint32_t iter, uint32_t user_id, std::string drop_type) :
  EnsembleAwareQueue(),
  _headroom(headroom),
  _iter(iter),
  _user_id(user_id),
  _time_constant(user_arrival_rate_time_constant),
  _drop_type(drop_type),
  _last_drop_time(0.0),
  _packet_queue( new PacketQueue() ),
  _dropper(_iter),
  _user_arrival_rate_est(FlowStats(user_arrival_rate_time_constant))
{
  fprintf( stderr,  "SFD: _iter %d, _K %f, _headroom %f, user_id %d, drop_type %s \n",
           _iter, user_arrival_rate_time_constant, _headroom, _user_id, _drop_type.c_str());
}

void SFD::enque(Packet *p)
{
  /* Implements pure virtual function Queue::enque() */

  /* Enque packet, since all dropping is from the head */
  _packet_queue->enque( p );
 
  /* Estimate arrival rate with an EWMA filter */
  double now = Scheduler::instance().clock();
  double arrival_rate = _user_arrival_rate_est.est_arrival_rate(now, p);

  /* Estimate aggregate arrival rate with an EWMA filter */
  double agg_arrival_rate = _scheduler->update_arrival_rate(now, p);

  /* Get fair share, and take max of fair share and service rate  */
  auto _fair_share = (1-_headroom) * _scheduler->get_fair_share(_user_id);
  //printf("User id is %d, _fair_share is %f \n", user_id, _fair_share);

  /* Print everything */
  //print_stats( now );
  

  /* Check aggregate arrival rate and compare it to aggregate ideal pf throughput */
  if (agg_arrival_rate <= _scheduler->agg_pf_throughput() || arrival_rate <= _fair_share) {
    return;
  }

  /* The arrival rate has exceeded the service rate for the agg and for us */
  if (_drop_type == "draconian") {
    draconian_dropping(now);
  } else if (_drop_type == "time") {
    time_based_dropping(now);
  } else {
    assert(false);
  }
}

void SFD::time_based_dropping(double now)
{
  if ((now - _last_drop_time > _time_constant)) {
    /* Compute drop_probability */
    double drop_probability = (now - _last_drop_time) / _time_constant;
    /* Toss a coin and drop */
    if ( _dropper.should_drop( drop_probability ) ) {
      /* Drop from front of the same queue */
      //  printf( " Time %f : Dropping packet, from flow %u drop_probability is %f\n", now, user_id, drop_probability );
      if (length() > 1) {
	Packet* head = _packet_queue->deque();
	drop( head );
	_last_drop_time = now;
      }
    }
  }
}

void SFD::draconian_dropping(double now)
{
  drop_if_not_empty(now);
}

void SFD::drop_if_not_empty(double now)
{
  if (length() > 1) {
    Packet* head = _packet_queue->deque();
    drop( head );
    _last_drop_time = now;
  }
}

Packet* SFD::deque()
{
  /* Implements pure virtual function Queue::deque() */
  Packet *p = _packet_queue->deque();
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
