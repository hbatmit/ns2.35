#include "sfd.h"
#include "rng.h"
#include "queue/qdelay-estimator.hh"
#include "common/distribution.hh"
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
      double delay_thresh = atof(strtok(nullptr, " "));
      return new SFD(user_arrival_rate_time_constant, headroom, iter, user_id, drop_type, delay_thresh);
    }
} class_sfd;

void SFD::trace(TracedVar* v)
{
  if (tchan_) {
    double now = Scheduler::instance().clock();
    char print_str[500];
    if (std::string(v->name()) == "_last_drop_time") {
      sprintf(print_str, "user:%d %s %g %g", _user_id, v->name(), now, double(*((TracedDouble*) v)));
    } else if (std::string(v->name()) == "_arr_rate_at_drop") {
      sprintf(print_str, "user:%d %s %g %g", _user_id, v->name(), now, double(*((TracedDouble*) v)));
    } else if (std::string(v->name()) == "_current_arr_rate") {
      sprintf(print_str, "user:%d %s %g %g", _user_id, v->name(), now, double(*((TracedDouble*) v)));
    } else {
      fprintf(stderr, "SFD: unknown trace var %s\n", v->name());
      exit(5);
    } 
    int n = strlen(print_str);
    print_str[n] = '\n'; 
    print_str[n+1] = 0;
    Tcl_Write(tchan_, print_str, n+1);
  } else {
    fprintf(stderr, "Trace file handle is empty ... exiting\n");
    exit(5);
  }
}

int SFD::command(int argc, const char*const* argv)
{
  if (argc == 3) {
    Tcl& tcl = Tcl::instance();
    // attach a file for variable tracing
    if (strcmp(argv[1], "attach") == 0) {
      int mode;
      const char* id = argv[2];
      tchan_ = Tcl_GetChannel(tcl.interp(), (char*)id, &mode);
      if (tchan_ == 0) {
        tcl.resultf("SFD trace: can't attach %s for writing", id);
        return (TCL_ERROR);
      }
      return (TCL_OK);
    }
  }
  return EnsembleAwareQueue::command(argc, argv);
}

SFD::SFD(double user_arrival_rate_time_constant, double headroom,
         uint32_t iter, uint32_t user_id, std::string drop_type, double delay_thresh) :
  EnsembleAwareQueue(),
  _headroom(headroom),
  _iter(iter),
  _user_id(user_id),
  _time_constant(user_arrival_rate_time_constant),
  _drop_type(drop_type),
  _delay_thresh(delay_thresh),
  _last_drop_time(0.0),
  _arr_rate_at_drop(0.0),
  _current_arr_rate(0.0),
  _packet_queue( new PacketQueue() ),
  _dropper(_iter),
  _user_arrival_rate_est(FlowStats(user_arrival_rate_time_constant)),
  _hist_delays(std::vector<DeliveredPacket>()),
  tchan_(0)
{
  bind("_last_drop_time",   &_last_drop_time);
  bind("_arr_rate_at_drop", &_arr_rate_at_drop);
  bind("_current_arr_rate", &_current_arr_rate);
  fprintf( stderr,  "SFD: _iter %d, _K %.3f, _headroom %.2f, user_id %d, drop_type %s delay_thresh %.3f\n",
           _iter, user_arrival_rate_time_constant, _headroom, _user_id, _drop_type.c_str(), _delay_thresh);
}

void SFD::enque(Packet *p)
{
  /* Implements pure virtual function Queue::enque() */

  /* Enque packet, since all dropping is from the head */
  _packet_queue->enque( p );
 
  /* Estimate arrival rate with an EWMA filter */
  double now = Scheduler::instance().clock();
  _current_arr_rate = _user_arrival_rate_est.est_arrival_rate(now, p);

  /* Estimate aggregate arrival rate with an EWMA filter */
  double agg_arrival_rate = _scheduler->update_arrival_rate(now, p);

  /* Get fair share, and take max of fair share and service rate  */
  auto _fair_share = (1-_headroom) * _scheduler->get_fair_share(_user_id);
  //printf("User id is %d, _fair_share is %f \n", user_id, _fair_share);

  /* Print everything */
  //print_stats( now );
  
  double cap = _scheduler->agg_pf_throughput();
  double delay_total = _scheduler->agg_queue_bytes()*8.0/cap + std::max(_time_constant * (agg_arrival_rate - cap) / cap, 0.0);
  double delay_flow  = byteLength()*8.0/_fair_share + std::max(_time_constant * (_current_arr_rate - _fair_share) / _fair_share, 0.0);
  if (delay_total < _delay_thresh || delay_flow < _delay_thresh) {
    return;
  }

  /* We have exceeded the delay threshold (aggregate and for flow). */
  if (_drop_type == "draconian") {
    draconian_dropping(now, _current_arr_rate);
  } else if (_drop_type == "time") {
    time_based_dropping(now, _current_arr_rate);
  } else {
    assert(false);
  }
}

double SFD::get_median_delay(void)
{
  /* Estimate delay of packets in queue */
  QdelayEstimator estimator(_packet_queue, _scheduler->get_fair_share(_user_id));
  
  /* Compute delays from history */
  std::vector<double> historic_delays(_hist_delays.size());
  std::transform(_hist_delays.begin(), _hist_delays.end(), historic_delays.begin(),
                 [&] (const DeliveredPacket & p)
                 { return p.delivered - hdr_cmn::access(&(p.pkt))->timestamp();} );

  /*Estimate both distributions */
  Distribution queue_dist ( estimator.estimate_delays(Scheduler::instance().clock()) );
  Distribution history_dist( historic_delays );

  /* Compose the two */
  Distribution composed = history_dist.compose( queue_dist );

  /* Return median */
  return composed.quantile(0.5);
}

void SFD::time_based_dropping(double now, double current_arrival_rate)
{
  if ((now - _last_drop_time > _time_constant)) {
    /* Compute drop_probability */
    double drop_probability = (now - _last_drop_time) / _time_constant;
    /* Toss a coin and drop */
    if ( _dropper.should_drop( drop_probability ) ) {
      drop_if_not_empty(now, current_arrival_rate);
    }
  }
}

void SFD::draconian_dropping(double now, double current_arrival_rate)
{
  drop_if_not_empty(now, current_arrival_rate);
}

void SFD::drop_if_not_empty(double now, double current_arrival_rate)
{
  if (length() > 1) {
    Packet* head = _packet_queue->deque();
    drop( head );
    /* Drop from front of the same queue */
    _last_drop_time   = now;
    _arr_rate_at_drop = current_arrival_rate;
  }
}

Packet* SFD::deque()
{
  /* Current time */
  double now = Scheduler::instance().clock();
  
  /* Implements pure virtual function Queue::deque() */
  Packet *p = _packet_queue->deque();

  /* Track user delays */
  if (p != nullptr) {
    _hist_delays.push_back(DeliveredPacket(*(p->copy()), now)); 
  } 

  /* purge old delays */
  _hist_delays.erase(std::remove_if(_hist_delays.begin(), _hist_delays.end(),
                                    [&] (const DeliveredPacket & p) { return p.delivered < now - _time_constant; }),
                     _hist_delays.end());
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
