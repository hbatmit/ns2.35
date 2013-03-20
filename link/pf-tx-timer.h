#ifndef LINK_PF_TX_TIMER_H
#define LINK_PF_TX_TIMER_H

#include "common/timer-handler.h"
#include "link/prop-fair-scheduler.h"

/*
   PFTxTimer inherits from TimerHandler
   to schedule the next packet transmission.
 */
class PFTxTimer : public TimerHandler {
 public:
  /* Constructor */
  PFTxTimer(PFScheduler * pf_sched);

 protected:
  virtual void expire(Event * e) override;
  PFScheduler * pf_sched_;
};

#endif
