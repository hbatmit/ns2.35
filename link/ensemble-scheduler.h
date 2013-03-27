#ifndef LINK_ENSEMBLE_SCHEDULER_H
#define LINK_ENSEMBLE_SCHEDULER_H

#include <stdint.h>
#include <vector>
#include "queue/queue.h"
#include "link/delay.h"
#include "link/rate-gen.h"
#include "link/cdma-rates.h"

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

  /* Aggregate total throughput by pf (or any other fairness criterion) */
  double agg_pf_throughput(void) const;

 protected:
  /* number of users */
  uint32_t num_users_;

  /* per user queues */
  std::vector<Queue *> user_queues_;

  /* per user links */
  std::vector<LinkDelay *> user_links_;

  /* per user instantaneous link rates */
  std::vector<double> link_rates_;

  /* per user rate generators */
  std::vector<RateGen> rate_generators_;

  /* generate new rates, assume perfect information */
  void generate_new_rates(void);

  /* get backlogged users */
  std::vector<uint32_t> get_backlogged_users(void) const;
};

#endif
