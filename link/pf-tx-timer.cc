#include "link/pf-tx-timer.h"
#include "common/packet.h"

/* Constructor */
PFTxTimer::PFTxTimer(PFScheduler * pf_sched)
    : TimerHandler(),
      pf_sched_(pf_sched) {}

void PFTxTimer::expire(Event *e) {
    PFScheduler::transmit_pkt(pf_sched_, pf_sched_->tx_timer_);
}
