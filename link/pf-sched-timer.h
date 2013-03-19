#ifndef LINK_PF_SCHED_TIMER_H
#define LINK_PF_SCHED_TIMER_H

#include "common/timer-handler.h"
#include "link/prop-fair-scheduler.h"

/*
   PFSchedTimer inherits from TimerHandler and
   expires every time a new user is to be picked.
 */
class PFSchedTimer : public TimerHandler {
 public:
  /* Constructor */
  PFSchedTimer(PFScheduler * pf_sched, double slot_duration);

 protected:
  virtual void expire(Event * e) override;
  PFScheduler * pf_sched_;
  double slot_duration_;
};

#endif
