#include "common/packet.h"
#include "link/fcfs-scheduler.h"
#include "link/fcfs-tx-timer.h"

/* Constructor */
FcfsTxTimer::FcfsTxTimer(FcfsScheduler * fcfs_sched)
    : TimerHandler(),
      fcfs_sched_(fcfs_sched) {}

void FcfsTxTimer::expire(Event *e) {
    fcfs_sched_->transmit_pkt();
}
