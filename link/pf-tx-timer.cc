#include "link/pf-tx-timer.h"
#include "common/packet.h"

/* Constructor */
PFTxTimer::PFTxTimer(PFScheduler * pf_sched)
    : TimerHandler(),
      pf_sched_(pf_sched) {}

void PFTxTimer::expire(Event *e) {
    pf_sched_->transmit_pkt();
}
