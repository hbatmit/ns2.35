#ifndef LINK_ENSEMBLE_RATE_GENERATOR_H
#define LINK_ENSEMBLE_RATE_GENERATOR_H

#include <stdint.h>
#include <queue>
#include <vector>
#include <map>
#include <string>
#include "common/timer-handler.h"
#include "link/delay.h"

/*
  Generate rates from a trace file for
  an ensemble of links.
 */

struct LinkRateEvent {
  double timestamp;
  uint32_t user_id;
  double link_rate;
  LinkRateEvent(double t_timestamp, uint32_t t_user_id, double t_link_rate)
    : timestamp(t_timestamp),
      user_id(t_user_id),
      link_rate(t_link_rate) {}
  LinkRateEvent()
    : timestamp(-1.0),
      user_id((uint32_t)-1),
      link_rate(-1.0) {}
  LinkRateEvent(const LinkRateEvent &e)
    : timestamp(e.timestamp),
      user_id(e.user_id),
      link_rate(e.link_rate) {}
};

class EnsembleRateGenerator : public TimerHandler, public TclObject {
 public:
  /* Constructor */
  EnsembleRateGenerator(std::string t_trace_file, double simulation_time);

  /* Tcl interface : add links and queues */
  virtual int command(int argc, const char*const* argv) override;

 protected:
  /* TimerHandler interface */
  virtual void expire(Event *e) override;

 private:
  /* Read from a trace file of link rate changes, return number of unique users */
  uint32_t read_link_rate_trace(double simulation_time);

  /* Schedule next link rate change using TimerHandler */
  void schedule_next_event(void);

  /* Finalize capacity calculations used simulation_time */
  void finalize_caps(double simulation_time);

  /* Start scheduling in response to command() */
  void init(void);

  /* Queue with all link rate changes */
  std::queue<LinkRateEvent> link_rate_changes_;

  /* Initial rate map */
  std::map<uint32_t,double> initial_rate_map_;

  /* Trace file name */
  std::string trace_file_;

  /* Per user capacity calculations */
  std::map<uint32_t,double> cap_in_bits_; /* Capacity of each user so far in bits */
  std::map<uint32_t,double> last_change_;  /* When was the last time when the rate changed? */
  std::map<uint32_t,double> last_rate_;   /* What was the last rate? */
  std::map<uint32_t,double> cap_in_bps_;  /* Capacity calculated finally in bits per sec */

  /* Number of users */
  int num_users_;

  /* Handles to user links */
  std::vector<LinkDelay*> user_links_;

  /* Next event */
  LinkRateEvent next_event_;
};

#endif // LINK_ENSEMBLE_RATE_GENERATOR_H
