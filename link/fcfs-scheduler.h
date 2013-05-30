#ifndef LINK_FCFS_SCHEDULER_H
#define LINK_FCFS_SCHEDULER_H

#include <stdint.h>
#include "link/ensemble-scheduler.h"

/* Forward Declaration */
class FcfsTxTimer;

/*
   FcfsScheduler implements an
   ensemble Fcfs scheduler on top
   of an ensemble of links
 */
class FcfsScheduler : public EnsembleScheduler {
 public:
  /* FALLBACK_INTERVAL for tx_timer */
  static constexpr double FALLBACK_INTERVAL = 0.001;

  /* Constructor */
  FcfsScheduler(double rate_est_time_constant, uint32_t num_users, double feedback_delay);

  /* pick next user to scheduler */
  virtual uint32_t pick_user_to_schedule(void) const;

  /* Service rate of scheduler */
  virtual double get_service_rate(uint32_t user_id) override { return user_service_rate_est_.at(user_id).ser_rate(); }

  /* Tcl interface : add links, and queues */
  virtual int command(int argc, const char*const* argv) override;

  /* Transmit packet */
  void transmit_pkt();

  /* Timers */
  FcfsTxTimer* tx_timer_;

 private:
  /* per user service rates */
  std::vector<FlowStats> user_service_rate_est_;
};

#endif
