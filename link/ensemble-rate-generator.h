#ifndef LINK_ENSEMBLE_RATE_GENERATOR_H
#define LINK_ENSEMBLE_RATE_GENERATOR_H

#include <stdint.h>
#include <queue>
#include <vector>
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
  EnsembleRateGenerator(std::string t_trace_file);

  /* Tcl interface : add links and queues */
  virtual int command(int argc, const char*const* argv) override;

 protected:
  /* TimerHandler interface */
  virtual void expire(Event *e) override;

 private:
  /* Read from a trace file of link rate changes, return number of unique users */
  uint32_t read_link_rate_trace(void);

  /* Schedule next link rate change using TimerHandler */
  void schedule_next_event(void);

  /* Start scheduling in response to command() */
  void init(void);

  /* Queue with all link rate changes */
  std::queue<LinkRateEvent> link_rate_changes_;

  /* Handles to user links */
  std::vector<LinkDelay*> user_links_;

  int num_users_;
  std::string trace_file_;
  LinkRateEvent next_event_;
};

#endif // LINK_ENSEMBLE_RATE_GENERATOR_H
