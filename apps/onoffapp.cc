#include <cmath>
#include "exception.hh"
#include "ezio.hh"
#include "onoffapp.hh"

using namespace std;

OnOffApp::OnOffApp(string str_ontype,
                   uint32_t t_id,
                   uint32_t t_pkt_size,
                   uint32_t t_hdr_size,
                   uint32_t t_run,
                   double   on_average,
                   double   off_time_avg,
                   TcpAgent* t_tcp_handle,
                   bool reset)
    : ontype_(str_ontype == "bytes" ? BYTE_BASED :
              str_ontype == "time"  ? TIME_BASED :
              str_ontype == "flowcdf" ? EMPIRICAL:
              throw Exception("OnOffApp constructor", "Invalid string for ontype")),
      sender_id_(t_id),
      pkt_size_(t_pkt_size),
      hdr_size_(t_hdr_size),
      run_(t_run),
      start_distribution_(off_time_avg, run_),
      stop_distribution_(on_average, run_),
      emp_stop_distribution_(run_),
      tcp_handle_(t_tcp_handle),
      do_i_reset_(reset),
      start_timer_(this),
      stop_timer_(this)
{
  start_timer_.sched(start_distribution_.sample());
}

void OnOffApp::start_send() {
  if (ontype_ == BYTE_BASED) {
    current_flow_.flow_size = stop_distribution_.sample();
  } else if (ontype_ == TIME_BASED) {
    current_flow_.on_duration = stop_distribution_.sample();
  } else if (ontype_ == EMPIRICAL) {
    current_flow_.flow_size = emp_stop_distribution_.sample();
  }

  laststart_ = Scheduler::instance().clock();
  state_ = ON;
  if (do_i_reset_) {
    tcp_handle_->reset_to_iw();
  }

  if (ontype_ == BYTE_BASED or ontype_ == EMPIRICAL) {
    sentinel_ += ceil(double(current_flow_.flow_size) / double(pkt_size_));
    /* TODO: Handle the Vegas kludge somehow */
    tcp_handle_->advanceby(ceil(double(current_flow_.flow_size) / double(pkt_size_)));
  } else if (ontype_ == TIME_BASED) {
    tcp_handle_->send(-1);
    stop_timer_.sched(current_flow_.on_duration);
  }
}

void OnOffApp::recv_ack(Packet* ack) {
  /* Measure RTT and other statistics */
  stat_collector_.add_sample(ack);
//  fprintf(stderr, "ack %d, sentinel_ %d\n",
//                  hdr_tcp::access(ack)->seqno(),
//                  sentinel_);
  if (hdr_tcp::access(ack)->seqno() >= sentinel_ and (ontype_ == BYTE_BASED or ontype_ == EMPIRICAL)) {
      assert(hdr_tcp::access(ack)->seqno() == sentinel_);
      turn_off();
  }
}

void OnOffApp::turn_off(void) {
  if (ontype_ == BYTE_BASED or ontype_ == EMPIRICAL) {
     assert(tcp_handle_->ack() == sentinel_);
  } else if (ontype_ == TIME_BASED) {
     assert(Scheduler::instance().clock() == (laststart_ + current_flow_.on_duration));
     tcp_handle_->advanceto(0); /* Src quench kludge */
  }

  state_ = OFF;
  total_on_time_ += (Scheduler::instance().clock() - laststart_);

  if (do_i_reset_) {
    tcp_handle_->reset_to_iw();
  }
  fprintf(stderr, "Turned off at %f, turning on at %f\n", Scheduler::instance().clock(),
                  Scheduler::instance().clock() + start_distribution_.sample());
  start_timer_.sched(start_distribution_.sample());
}

static class OnOffClass : public TclClass {
 public:
  OnOffClass() : TclClass("Application/OnOff") {}
  TclObject* create(int argc, const char*const* argv) {
    try {
      if (argc != 13) {
        throw Exception("Application/OnOff mirror constructor", "too few args");
      } else {
        return new OnOffApp(string(argv[4]),
                            myatoi(argv[5]),
                            myatoi(argv[6]),
                            myatoi(argv[7]),
                            myatoi(argv[8]),
                            myatof(argv[9]),
                            myatof(argv[10]),
                            reinterpret_cast<TcpAgent *>(TclObject::lookup(argv[11])),
                            myatoi(argv[12]));
      }
    } catch (const Exception & e) {
      e.perror();
      exit(EXIT_FAILURE);
    }
  }
} class_on_off_app;

int OnOffApp::command(int argc, const char*const* argv) {
  if (strcmp(argv[1], "stats") == 0) {
    assert(++calls_ < 2);
    total_on_time_ += (state_ == OFF) ? 0 : (Scheduler::instance().clock() - laststart_);
    stat_collector_.output_stats(total_on_time_, sender_id_, pkt_size_ + hdr_size_);
    return TCL_OK;
  }
  return Application::command(argc, argv);
}

void AppStartTimer::expire(Event* e) {
  app_->start_send();
}

void AppStopTimer::expire(Event* e) {
  app_->turn_off();
}
