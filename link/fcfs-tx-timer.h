#ifndef LINK_FCFS_TX_TIMER_H
#define LINK_FCFS_TX_TIMER_H

#include "common/timer-handler.h"
#include "link/fcfs-scheduler.h"

/*
  FcfsTxTimer inherits from TimerHandler
  to schedule the next packet transmission.
 */
class FcfsTxTimer : public TimerHandler {
 public:
  /* Constructor */
  FcfsTxTimer(FcfsScheduler * fcfs_sched);

 protected:
  virtual void expire(Event* e) override;
  FcfsScheduler * fcfs_sched_;
};

#endif
