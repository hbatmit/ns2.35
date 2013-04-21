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

  /* Tcl interface : add links and queues */
  virtual int command(int argc, const char*const* argv) override;

  /* Number of users */
  uint32_t num_active_users(void) const { return get_backlogged_users().size(); }

  /* Aggregate arrival rate */
  double agg_arrival_rate(void) const;

  /* Aggregate total throughput by pf (or other fairness criteria) */
  double agg_pf_throughput(void);

  /* K for rate estimation */
  static constexpr double K = 0.200;

  /* update link rate estimate , model feedback delay and noise */
  void update_link_rate_estimate(void);

  double get_link_rate_estimate(int user_id) { return link_rates_.at(user_id); }

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
  std::vector<double> link_rates_;

  /* aggregate rate estimator */
  FlowStats agg_rate_estimator_;

  /* get backlogged users */
  std::vector<uint32_t> get_backlogged_users(void) const;
};

#endif  // LINK_ENSEMBLE_SCHEDULER_H_
