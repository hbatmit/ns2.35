#include "link/pf-tx-timer.h"
#include "common/packet.h"

/* Constructor */
PFTxTimer::PFTxTimer(PFScheduler * pf_sched)
    : TimerHandler(),
      pf_sched_(pf_sched) {}

void PFTxTimer::expire(Event *e) {
  /* Get chosen user */
  auto chosen_user = pf_sched_->chosen_user_;

  /* Get queue_handler */
  auto queue_handler = &pf_sched_->user_queues_.at(chosen_user)->qh_;

  /* Get one packet from chosen user */
  Packet *p = pf_sched_->user_queues_.at(chosen_user)->deque();

  /* If p is nullptr return */
  if (p==nullptr) {
    return;
  }

  /* Get transmission time */
  double txt = pf_sched_->user_links_.at(chosen_user)->txtime(p);

  /* Check if packet txtime spills over into the next time slot. If so, slice it */
  if(txt+Scheduler::instance().clock() > pf_sched_->current_slot_ + pf_sched_->slot_duration_) {
    printf(" PFTxTimer::expire, Chosen_user %d, slicing %f bits \n",
            pf_sched_->chosen_user_,
            (pf_sched_->current_slot_+ pf_sched_->slot_duration_ - Scheduler::instance().clock()) * 
            pf_sched_->user_links_.at(chosen_user)->bandwidth() );
    printf(" PFTxTimer::expire, Chosen_user %d, remaining %f bits \n",
            pf_sched_->chosen_user_,
            pf_sched_->user_links_.at(chosen_user)->bandwidth()*txt -
            ((pf_sched_->current_slot_+ pf_sched_->slot_duration_ - Scheduler::instance().clock()) * 
            pf_sched_->user_links_.at(chosen_user)->bandwidth()) );

    /* TODO */
  } else {
    /* Send packet onward */
    pf_sched_->user_links_.at(chosen_user)->recv(p, queue_handler);
    /* Log */
    printf(" PFScheduler::expire, Chosen_user %d, recving %f bits @ %f \n",
           chosen_user,
           pf_sched_->user_links_.at(chosen_user)->bandwidth()*txt,
           Scheduler::instance().clock());

    /* schedule next packet transmission */
    resched(txt);
  }
}
