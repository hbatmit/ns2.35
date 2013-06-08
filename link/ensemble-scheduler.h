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
  EnsembleScheduler(double rate_est_time_constant, uint32_t num_users, double feedback_delay);

  /* pick next user to schedule */
  virtual uint32_t pick_user_to_schedule(void) const = 0;

  /* Get service rate of EnsembleScheduler, this is different from the service rate of Queue */
  virtual double get_service_rate(uint32_t user_id) = 0;

  /* Tcl interface : add links and queues */
  virtual int command(int argc, const char*const* argv) override;

  /* Number of users */
  uint32_t num_active_users(void) const { return get_feasible_users().size(); }

  /* Update aggregate arrival rate */
  double update_arrival_rate(double now, Packet* p) { return agg_arrival_rate_est_.est_arrival_rate(now, p); }

  /* Aggregate service rate */
  double agg_service_rate(void) const { return agg_service_rate_est_.ser_rate(); }

  /* Aggregate total throughput by pf (or other fairness criteria) */
  double agg_pf_throughput(void);

  /* Aggregate number of bytes across all queues */
  uint32_t agg_queue_bytes(void);

  /* Get fair share of user */
  double get_fair_share(uint32_t user_id);

 protected:
  /* update link rate estimate , model feedback delay and noise */
  void update_link_rate_estimate(void);

  /* get instantaneous link rate */
  double get_instantaneous_rate(uint32_t user_id) const;

  /* Get current link rate estimate after EWMA */
  double get_link_rate_estimate(uint32_t user_id) const { return user_link_rate_est_.at(user_id).link_rate(); }

  /* get feasible users i.e backlogged and non-zero link rate */
  std::vector<uint32_t> get_feasible_users(void) const;

  /* number of users */
  uint32_t num_users_;

  /* feedback delay in secs */
  double feedback_delay_;

  /* per user queues */
  std::vector<Queue *> user_queues_;

  /* per user links */
  std::vector<LinkDelay *> user_links_;

  /* per user current estimate of link rates */
  std::vector<FlowStats> user_link_rate_est_;

  /* Aggregate service rate EWMA */
  FlowStats agg_service_rate_est_;

  /* Aggregate arrival rate EWMA */
  FlowStats agg_arrival_rate_est_;
};

#endif  // LINK_ENSEMBLE_SCHEDULER_H_
