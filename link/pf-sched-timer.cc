#include "link/pf-sched-timer.h"
#include "assert.h"

/* Constructor */
PFSchedTimer::PFSchedTimer(PFScheduler * pf_sched, double slot_duration)
    : TimerHandler(),
      pf_sched_(pf_sched),
      slot_duration_(slot_duration) {}

void PFSchedTimer::expire(Event *e) {
  /* TimerHandler::expire */
  pf_sched_->tick();
  resched(slot_duration_);
}
