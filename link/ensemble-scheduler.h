#ifndef LINK_ENSEMBLE_SCHEDULER_H_
#define LINK_ENSEMBLE_SCHEDULER_H_

#include <stdint.h>
#include <vector>
#include "queue/queue.h"
#include "common/flow-stats.h"
#include "link/delay.h"
#include "link/rate-gen.h"

class EnsembleScheduler : public TclObject {
 public:
  /* Constructor */
  EnsembleScheduler(uint32_t num_users, double feedback_delay);

  /* pick next user to schedule */
  virtual uint32_t pick_user_to_schedule(void) const = 0;

  /* Get service rate of EnsembleScheduler, this is different from the service rate of Queue */
  virtual double get_service_rate(uint32_t user_id) = 0;

  /* Tcl interface : add links and queues */
  virtual int command(int argc, const char*const* argv) override;

  /* Number of users */
  uint32_t num_active_users(void) const { return get_feasible_users().size(); }

  /* Aggregate arrival rate */
  double agg_arrival_rate(void);

  /* Aggregate service rate */
  double agg_service_rate(void);

  /* Aggregate total throughput by pf (or other fairness criteria) */
  double agg_pf_throughput(void);

  /* K for rate estimation */
  static constexpr double FLOW_ESTIMATOR_K = 0.200;

  /* update link rate estimate , model feedback delay and noise */
  void update_link_rate_estimate(void);

  /* Get current link rate estimate after EWMA */
  double get_link_rate_estimate(uint32_t user_id) const { return link_rates_.at(user_id).link_rate(); }

 protected:
  /* number of users */
  uint32_t num_users_;

  /* feedback delay in secs */
  double feedback_delay_;

  /* per user queues */
  std::vector<Queue *> user_queues_;

  /* per user links */
  std::vector<LinkDelay *> user_links_;

  /* per user current estimate of link rates */
  std::vector<FlowStats> link_rates_;

  /* aggregate rate estimator */
  FlowStats agg_rate_estimator_;

  /* get feasible users i.e backlogged and non-zero link rate */
  std::vector<uint32_t> get_feasible_users(void) const;
};

#endif  // LINK_ENSEMBLE_SCHEDULER_H_
