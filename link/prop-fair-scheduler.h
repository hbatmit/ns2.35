#ifndef LINK_PROP_FAIR_SCHEDULER_H
#define LINK_PROP_FAIR_SCHEDULER_H

#include <stdint.h>
#include <vector>
#include "common/agent.h"
#include "link/ensemble-scheduler.h"

/* Forward declarations */
class PFSchedTimer;
class PFTxTimer;

/* 
   PFScheduler implements an ensemble
   proportionally fair scheduler on top 
   of an ensemble of links.
 */
class PFScheduler : public EnsembleScheduler {
 public:
  /* Constructor */
  PFScheduler(uint32_t num_users,
              double feedback_delay,
              double slot_duration,
              uint32_t ewma_slots);

  /* pick next user to schedule */
  virtual uint32_t pick_user_to_schedule(void) const override; 

  /* Tcl interface : activate scheduler */
  virtual int command(int argc, const char*const* argv) override;

  /* last time something was scheduled */
  double current_slot_;

  /* tick every slot_duration_ */
  void tick(void);

  /* Transmit packet */
  void transmit_pkt();

  /* Helper function, transmit after slicing (if reqd) */
  void slice_and_transmit(Packet *p, uint32_t chosen_user);

 private:
  /* update mean achieved rates */
  void update_mean_achieved_rates(uint32_t scheduled_user);

  /* slot duration */
  const double slot_duration_;

  /* ewma_slots */
  const uint32_t ewma_slots_;

  /* chosen user */
  uint32_t chosen_user_;

  /* per user mean achieved rates */
  std::vector<double> mean_achieved_rates_;

  /* Timers */
  PFTxTimer* tx_timer_;
  PFSchedTimer* sched_timer_;

  /* Vector of abeyant packets */
  std::vector<Packet*> abeyance_;

  /* Slicing Agent */
  Agent slicing_agent_;
};

#endif
