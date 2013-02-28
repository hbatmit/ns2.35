#ifndef LINK_PROP_FAIR_SCHEDULER_H
#define LINK_PROP_FAIR_SCHEDULER_H

#include <stdint.h>
#include <vector>
#include "common/timer-handler.h"
#include "queue/queue.h"
#include "link/delay.h"
#include "rate-gen.h"
#include "cdma-rates.h"

/* 
   PropFair implements an ensemble
   proportionally fair scheduler on top 
   of an ensemble of links.

   It inherits from TimerHandler to fire off
   a scheduling function periodically to pick
   the next user to transmit.
 */

class PropFair : public TimerHandler, public TclObject {
 public:
  /* Constructor */
  PropFair();

  /* pick next user to schedule */
  uint32_t pick_user_to_schedule(void) const; 

  /* Tcl interface : add links, and queues */
  virtual int command(int argc, const char*const* argv) override;

  /* EWMA_SLOTS */
  static const uint32_t EWMA_SLOTS = 100;

 private:
  /* tick every slot_duration_ */
  void tick(void);

  /* generate new rates, assume perfect information */
  void generate_new_rates(void);

  /* update mean achieved rates */
  void update_mean_achieved_rates(uint32_t scheduled_user);

  /* get backlogged users */
  std::vector<uint32_t> get_backlogged_users(void) const;

  /* expire function from TimerHandler */
  virtual void expire(Event *e) override;

  /* slot duration */
  double slot_duration_;

  /* number of users */
  uint32_t num_users_;

  /* chosen user */
  uint32_t chosen_user_;

  /* last time something was scheduled */
  double last_time_;

  /* per user queues */
  std::vector<const Queue *> user_queues_;

  /* per user links */
  std::vector<const LinkDelay *> user_links_;

  /* per user mean achieved rates */
  std::vector<double> mean_achieved_rates_;

  /* per user instantaneous link rates */
  std::vector<double> link_rates_;

  /* per user rate generators */
  std::vector<RateGen> rate_generators_;
};

#endif
