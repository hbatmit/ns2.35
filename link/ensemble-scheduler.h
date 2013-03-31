#ifndef LINK_ENSEMBLE_SCHEDULER_H
#define LINK_ENSEMBLE_SCHEDULER_H

#include <stdint.h>
#include <vector>
#include "queue/queue.h"
#include "link/delay.h"
#include "link/rate-gen.h"
#include "link/cdma-rates.h"
#include "queue/sfd-rate-estimator.h"

class EnsembleScheduler : public TclObject {
 public:
  /* Constructor */
  EnsembleScheduler();
  
  /* pick next user to schedule */
  virtual uint32_t pick_user_to_schedule(void) const = 0;

  /* Tcl interface : add links and queues */
  virtual int command(int argc, const char*const* argv) override;

  /* Number of users */
  uint32_t num_users(void) const { return num_users_; }

  /* Aggregate arrival rate */
  double agg_arrival_rate(void) const;

  /* Aggregate total throughput by pf (or other fairness criteria) */
  double agg_pf_throughput(void);

  /* K for rate estimation */
  static constexpr double K = 0.200;
 protected:
  /* number of users */
  uint32_t num_users_;

  /* per user queues */
  std::vector<Queue *> user_queues_;

  /* per user links */
  std::vector<LinkDelay *> user_links_;

  /* per user instantaneous link rates */
  std::vector<double> link_rates_;

  /* aggregate rate estimator */
  SfdRateEstimator agg_rate_estimator_;

  /* get backlogged users */
  std::vector<uint32_t> get_backlogged_users(void) const;

  /* update link rate estimate , model feedback delay and noise here */
  void update_link_rate_estimate(void);
};

#endif
