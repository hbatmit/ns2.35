#include <assert.h>
#include <algorithm>
#include "link/ensemble-scheduler.h"

EnsembleScheduler::EnsembleScheduler(uint32_t num_users, double feedback_delay)
    : num_users_(num_users),
      feedback_delay_(feedback_delay),
      user_queues_(std::vector<Queue*>(num_users_)),
      user_links_(std::vector<LinkDelay*>(num_users_)),
      link_rates_(std::vector<double>(num_users_)),
      agg_rate_estimator_(K) {
  assert(num_users_ > 0);
  for ( uint32_t i = 0; i < num_users_; i++ ) {
    link_rates_.at(i)=0.0;
  }
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
    if ( (!(user_queues_.at(i)->empty())) and (link_rates_.at(i) != 0)) {
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
    if (!user_queues_.at(i)->empty()) agg_link_rate += link_rates_.at(i);
  }
  auto pf_allocation = agg_link_rate/(num_active_users() == 0 ? 1 : num_active_users());
  auto now = Scheduler::instance().clock();
  return agg_rate_estimator_.est_link_rate(now, pf_allocation);
}

double EnsembleScheduler::agg_arrival_rate(void) const {
  /* Ask each queue for it's own arrival rate */
  double agg_arrival_rate = 0.0;
  for (uint32_t i = 0; i < num_users_; i++) {
    agg_arrival_rate += user_queues_.at(i)->get_arrival_rate();
  }
  return agg_arrival_rate;
}

double EnsembleScheduler::agg_service_rate(void) const {
  /* Ask each queue for it's own service rate */
  double agg_service_rate = 0.0;
  for (uint32_t i = 0; i < num_users_; i++) {
    agg_service_rate += user_queues_.at(i)->get_service_rate();
  }
  return agg_service_rate;
}
void EnsembleScheduler::update_link_rate_estimate(void) {
  /* Update link rate estimates, model feedback delay and/or noise here */
  for (uint32_t i = 0; i < num_users_; i++) {
    link_rates_.at(i) = user_links_.at(i)->get_bw_in_past(Scheduler::instance().clock() - feedback_delay_);
  }
}
