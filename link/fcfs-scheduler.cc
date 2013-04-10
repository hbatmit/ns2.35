#include <algorithm>
#include "link/fcfs-scheduler.h"
#include "link/fcfs-tx-timer.h"

static class FcfsSchedulerClass : public TclClass {
 public :
  FcfsSchedulerClass() : TclClass("FcfsScheduler") {}
  TclObject* create(int argc, const char*const* argv) {
    return (new FcfsScheduler());
  }
} class_fcfs;

FcfsScheduler::FcfsScheduler()
    : EnsembleScheduler(),
      tx_timer_(new FcfsTxTimer(this)) {}

uint32_t FcfsScheduler::pick_user_to_schedule(void) const {
  /* First get the backlogged users */
  std::vector<uint32_t> backlogged_users = get_backlogged_users();

  /* Get user timestamps */
  std::vector<double> hol_ts( num_users_ );
  for (uint32_t i=0; i < num_users_ ; i++ ) {
    hol_ts.at(i) = user_queues_.at(i)->get_hol();
  }
  
  /* Pick the earliest ts amongst them */
  auto it = std::min_element(backlogged_users.begin(), backlogged_users.end(),
                             [&] (const uint32_t &f1, const uint32_t &f2)
                             { return hol_ts.at(f1) < hol_ts.at(f2);});

  return (it!=backlogged_users.end()) ? *it : (uint32_t)-1;
}

int FcfsScheduler::command(int argc, const char*const* argv) {
  if (argc == 2) {
    if ( strcmp(argv[1], "activate-link-scheduler" ) == 0 ) {
      tx_timer_->resched( FALLBACK_INTERVAL );
      /* generate rates to start with */
      update_link_rate_estimate();
      return TCL_OK;
    }
  }
  return EnsembleScheduler::command(argc, argv);
}

void FcfsScheduler::transmit_pkt() {
  /* Update link rate estimate */
  update_link_rate_estimate();

  /* Get chosen user */
  uint32_t chosen_user = pick_user_to_schedule();

  /* If no one was scheduled, return */
  if (chosen_user==(uint32_t)-1) {
    tx_timer_->resched(FALLBACK_INTERVAL);
    return;
  }

  /* If link rate is zero, return */
  if (user_links_.at(chosen_user)->bandwidth()==0) {
    tx_timer_->resched(FALLBACK_INTERVAL);
    return;
  }

  /* Get one packet from chosen user */
  Packet *p = user_queues_.at(chosen_user)->deque();
  assert(p!=nullptr);

  /* Get queue_handler */
  auto queue_handler = &user_queues_.at(chosen_user)->qh_;

  /* Get transmission time */
  double txt = user_links_.at(chosen_user)->txtime(p);

  /* Send packet onward */
  user_links_.at(chosen_user)->recv(p, queue_handler);

  /* Log */
//  printf(" FcfsTxTimer::expire, Chosen_user %d, recving %f bits @ %f \n",
//         chosen_user,
//         user_links_.at(chosen_user)->bandwidth()*txt,
//         Scheduler::instance().clock());

  /* schedule next packet transmission */
  tx_timer_->resched(txt);
}
