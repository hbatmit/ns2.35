#include "link/pf-tx-timer.h"

/* Constructor */
PFTxTimer::PFTxTimer(PFScheduler * pf_sched)
    : TimerHandler(),
      pf_sched_(pf_sched_) {}

void PFTxTimer::expire(Event *e) {}
