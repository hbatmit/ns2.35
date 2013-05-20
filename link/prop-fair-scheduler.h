#ifndef LINK_PROP_FAIR_SCHEDULER_H
#define LINK_PROP_FAIR_SCHEDULER_H

#include <stdint.h>
#include <vector>
#include <string>
#include "common/agent.h"
#include "common/ewma-estimator.h"
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
  PFScheduler(uint32_t    t_num_users,
              double      t_feedback_delay,
              double      t_slot_duration,
              uint32_t    t_ewma_slots,
              double      t_alpha,
              std::string t_sub_qdisc);

  /* pick next user to schedule */
  virtual uint32_t pick_user_to_schedule(void) const override; 

  /* Service rate of scheduler */
  virtual double get_service_rate(uint32_t user_id) override { return mean_achieved_rates_.at(user_id); }

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

  /* Get delay of head of line packet */
  double hol_delay(uint32_t user_id) const;
  static constexpr double FLOW_EST_TIME_CONSTANT = 0.2;

 private:
  /* update mean achieved rates */
  void update_mean_achieved_rates(uint32_t scheduled_user);

  /* Get latest delay estimate */
  double get_delay(uint32_t user_id) const { return delay_ewma_.at(user_id).get_delay();}

  /* slot duration */
  const double slot_duration_;

  /* ewma_slots */
  const uint32_t ewma_slots_;

  /* Max weight scheduling parameter alpha */
  const double alpha_;

  /* Sub qdisc */
  const std::string sub_qdisc_;

  /* chosen user */
  uint32_t chosen_user_;

  /* Slotted EWMA of per user achieved rates */
  std::vector<double> mean_achieved_rates_;

  /* EWMA of per user delays */
  std::vector<FlowStats> delay_ewma_;

  /* Timers */
  PFTxTimer* tx_timer_;
  PFSchedTimer* sched_timer_;

  /* Vector of abeyant packets */
  std::vector<Packet*> abeyance_;

  /* Arrival timestamp of hol packets */
  std::vector<double> hol_ts_;

  /* Slicing Agent */
  Agent slicing_agent_;
};

#endif
