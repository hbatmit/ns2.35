#include <assert.h>
#include <algorithm>
#include "link/ensemble-scheduler.h"

EnsembleScheduler::EnsembleScheduler(double rate_est_time_constant, uint32_t num_users, double feedback_delay)
    : num_users_(num_users),
      feedback_delay_(feedback_delay),
      busy_(false),
      user_queues_(std::vector<Queue*>(num_users_)),
      user_links_(std::vector<LinkDelay*>(num_users_)),
      user_link_rate_est_(std::vector<FlowStats>(num_users_, FlowStats(rate_est_time_constant))),
      agg_service_rate_est_(FlowStats(rate_est_time_constant)),
      agg_arrival_rate_est_(FlowStats(rate_est_time_constant)) {
  assert(num_users_ > 0);
  fprintf( stderr, "EnsembleScheduler parameters num_users_ %d, feedback_delay %f \n",
          num_users_, feedback_delay_);
}

int EnsembleScheduler::command(int argc, const char*const* argv) {
  if (argc == 4) {
    if (!strcmp(argv[1], "attach-queue")) {
      Queue* queue = (Queue*) TclObject::lookup(argv[ 2 ]);
      uint32_t user_id = atoi(argv[ 3 ]);
      user_queues_.at(user_id) = queue;
      /* ensure blocked queues */
      assert(user_queues_.at(user_id)->blocked());
      return TCL_OK;
    }
    if (!strcmp(argv[1], "attach-link")) {
      LinkDelay* link = (LinkDelay*) TclObject::lookup(argv[ 2 ]);
      uint32_t user_id = atoi(argv[ 3 ]);
      user_links_.at(user_id) = link;
      return TCL_OK;
    }
  }
  return TclObject::command( argc, argv );
}

std::vector<uint32_t> EnsembleScheduler::get_feasible_users(void) const {
  std::vector<uint32_t> feasible_user_list;
  for ( uint32_t i = 0; i < num_users_; i++ ) {
    if ( (!(user_queues_.at(i)->empty())) and (get_instantaneous_rate(i) != 0)) {
      feasible_user_list.push_back(i);
    } else {
//      printf(" User_queue is empty at %d \n", i );
    }
  }
  return feasible_user_list;
}

double EnsembleScheduler::agg_pf_throughput(void) {
  /* Aggregate PF throughput, after EWMA */
  double agg_link_rate = 0.0;
  for (uint32_t i = 0; i < num_users_; i++) {
    if (!user_queues_.at(i)->empty()) agg_link_rate += user_link_rate_est_.at(i).link_rate();
  }
  return (agg_link_rate/(num_active_users() == 0 ? 1 : num_active_users()));
}

uint32_t EnsembleScheduler::agg_queue_bytes(void) {
  /* Sum total of all queue sizes */
  int sum_total = 0;
  for (uint32_t i = 0; i < num_users_; i++) {
    sum_total += user_queues_.at(i)->byteLength();
  }
  return sum_total;
}

double EnsembleScheduler::get_instantaneous_rate(uint32_t user_id) const {
  return user_links_.at(user_id)->get_bw_in_past(Scheduler::instance().clock() - feedback_delay_);
}

void EnsembleScheduler::update_link_rate_estimate(void) {
  /* Update link rate estimates, model feedback delay and/or noise here */
  for (uint32_t i = 0; i < num_users_; i++) {
    user_link_rate_est_.at(i).est_link_rate(Scheduler::instance().clock(),
                                    user_links_.at(i)->get_bw_in_past(Scheduler::instance().clock() - feedback_delay_)
                                   );
  }
}

double EnsembleScheduler::get_fair_share(uint32_t user_id) {
  update_link_rate_estimate();
  double current_link_rate = get_link_rate_estimate(user_id);
  return current_link_rate / (num_active_users() == 0 ? 1 : num_active_users());
}
