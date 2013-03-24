#ifndef LINK_PROP_FAIR_SCHEDULER_H
#define LINK_PROP_FAIR_SCHEDULER_H

#include <stdint.h>
#include <vector>
#include "common/timer-handler.h"
#include "queue/queue.h"
#include "link/delay.h"
#include "rate-gen.h"
#include "cdma-rates.h"
#include "common/agent.h"

/* Forward declarations */
class PFSchedTimer;
class PFTxTimer;

/* 
   PFScheduler implements an ensemble
   proportionally fair scheduler on top 
   of an ensemble of links.
 */
class PFScheduler : public TclObject {
 friend class PFTxTimer;
 public:
  /* Constructor */
  PFScheduler();

  /* pick next user to schedule */
  uint32_t pick_user_to_schedule(void) const; 

  /* Tcl interface : add links, and queues */
  virtual int command(int argc, const char*const* argv) override;

  /* EWMA_SLOTS */
  static const uint32_t EWMA_SLOTS = 100;

  /* last time something was scheduled */
  double current_slot_;

  /* tick every slot_duration_ */
  void tick(void);

  /* Transmit packet */
  static void transmit_pkt(PFScheduler* pf_sched, PFTxTimer* tx_timer);

  /* Helper function, transmit after slicing (if reqd) */
  static void slice_and_transmit(PFScheduler* pf_sched, PFTxTimer* tx_timer, Packet *p, uint32_t chosen_user, bool transmit);

 private:
  /* generate new rates, assume perfect information */
  void generate_new_rates(void);

  /* update mean achieved rates */
  void update_mean_achieved_rates(uint32_t scheduled_user);

  /* get backlogged users */
  std::vector<uint32_t> get_backlogged_users(void) const;

  /* slot duration */
  double slot_duration_;

  /* number of users */
  uint32_t num_users_;

  /* chosen user */
  uint32_t chosen_user_;

  /* per user queues */
  std::vector<Queue *> user_queues_;

  /* per user links */
  std::vector<LinkDelay *> user_links_;

  /* per user mean achieved rates */
  std::vector<double> mean_achieved_rates_;

  /* per user instantaneous link rates */
  std::vector<double> link_rates_;

  /* per user rate generators */
  std::vector<RateGen> rate_generators_;

  /* Timers */
  PFTxTimer* tx_timer_;
  PFSchedTimer* sched_timer_;

  /* Vector of abeyant packets */
  std::vector<Packet*> abeyance_;

  /* Slicing Agent */
  Agent slicing_agent_;
};

#endif
