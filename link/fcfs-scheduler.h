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
 friend FcfsTxTimer;
 public:
  /* FALLBACK_INTERVAL for tx_timer */
  static constexpr double FALLBACK_INTERVAL = 0.001;

  /* Constructor */
  FcfsScheduler();

  /* pick next user to scheduler */
  virtual uint32_t pick_user_to_schedule(void) const;

  /* Tcl interface : add links, and queues */
  virtual int command(int argc, const char*const* argv) override;

  /* Transmit packet */
  void transmit_pkt();

  /* Timers */
  FcfsTxTimer* tx_timer_;
};

#endif
