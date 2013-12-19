#ifndef ONOFFAPP_HH_
#define ONOFFAPP_HH_

#include "app.h"
#include "statcollector.hh"
#include "tcp.h"
#include "expvariate.hh"
#include "empvariate.hh"

/* forward declaration */
class OnOffApp;

/* Executed on OFF->ON transitions */
class AppOnTimer : public TimerHandler {
 public:
  AppOnTimer(OnOffApp* app) : TimerHandler(), app_(app) {}
  virtual void expire(Event *e) override;
 protected:
  OnOffApp* app_;
};

/* Executed on ON->OFF transitions */
class AppOffTimer : public TimerHandler {
 public:
  AppOffTimer(OnOffApp* app) : TimerHandler(), app_(app) {}
  virtual void expire(Event *e) override;
 protected:
  OnOffApp* app_;
};

/* The Actual ON/OFF app */
class OnOffApp : public Application {
 public:
  OnOffApp(std::string t_ontype,
           uint32_t t_id,
           uint32_t t_run,
           uint32_t t_pkt_size,
           uint32_t t_hdr_size,
           double   on_average,
           double   off_time_avg,
           TcpAgent * tcp_handle,
           bool reset);
  OnOffApp() = delete; /* Delete the default constructor */
  void recv_ack(Packet* ack) override; /* Called inside TcpAgent::recv() */
  void resume(void) override; /* Called when the TcpAgent is done transmitting for the first time */
  void turn_on(void);    /* Called when the flow starts */
  void turn_off(void);   /* Called when the flow ends */
  int command(int argc, const char*const* argv) override;

 private:
  /* ontype, unique ID, packet, and header size */
  const enum {BYTE_BASED, TIME_BASED, EMPIRICAL} ontype_;
  const uint32_t sender_id_;
  const uint32_t pkt_size_;
  const uint32_t hdr_size_;

  /* Maintain statistics */
  double   total_on_time_ {0.0};
  StatCollector stat_collector_ {StatCollector()};

  /* Book-keeping and current state */
  double   laststart_ {0.0};
  enum {ON, OFF} state_ {OFF};

  /* Random variates, use names to mirror Keith's names in Remy */
  const uint32_t run_; /* Read Section 25.1.1 of ns-2 manual */
  ExpVariate start_distribution_; /* When to start next? */
  ExpVariate stop_distribution_;  /* When to stop next? */
  EmpVariate emp_stop_distribution_; /* Allman's CDF */

  /* Store either current on_duration or flow_size */
  union CurrentFlow {
    double on_duration;
    uint32_t flow_size;
    CurrentFlow() { memset(this, 0, sizeof(CurrentFlow)); }
  };
  CurrentFlow current_flow_;

  /* Handle to TCP to reset to IW for every new flow */
  TcpAgent* tcp_handle_;
  const bool do_i_reset_;

  /* Timers */
  AppOnTimer  on_timer_;
  AppOffTimer off_timer_;

  /* Track number of calls to stats */
  uint32_t calls_ {0};
};

#endif // ONOFFAPP_HH_
