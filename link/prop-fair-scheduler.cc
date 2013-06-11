#include <float.h>
#include <assert.h>
#include <algorithm>
#include "prop-fair-scheduler.h"
#include "link/pf-sched-timer.h"
#include "link/pf-tx-timer.h"
#include "common/agent.h"
#include "link/cell_header.h"

static class PFSchedulerClass : public TclClass {
 public :
  PFSchedulerClass() : TclClass("PFScheduler") {}
  TclObject* create(int argc, const char*const* argv) {
    return (new PFScheduler(atof(argv[4]),
                            atoi(argv[5]),
                            atof(argv[6]),
                            atof(argv[7]),
                            atoi(argv[8]),
                            atof(argv[9]),
                            std::string(argv[10])));
  }
} class_prop_fair;

PFScheduler::PFScheduler(double      t_rate_est_time_constant,
                         uint32_t    t_num_users,
                         double      t_feedback_delay,
                         double      t_slot_duration,
                         uint32_t    t_ewma_slots,
                         double      t_alpha,
                         std::string t_sub_qdisc)
    : EnsembleScheduler(t_rate_est_time_constant, t_num_users, t_feedback_delay),
      current_slot_(0.0),
      slot_duration_(t_slot_duration),
      ewma_slots_(t_ewma_slots),
      alpha_(t_alpha),
      sub_qdisc_(t_sub_qdisc),
      chosen_user_(0),
      mean_achieved_rates_(std::vector<double> (num_users_, 0.0)),
      user_delay_est_(std::vector<FlowStats> (num_users_, FlowStats(t_rate_est_time_constant))),
      tx_timer_(new PFTxTimer(this)),
      sched_timer_(new PFSchedTimer(this, slot_duration_)),
      abeyance_(std::vector<Packet*> (num_users_, nullptr)),
      hol_ts_(std::vector<double> (num_users_, 0.0)),
      slicing_agent_(Agent(PT_CELLULAR)) {
  fprintf(stderr, "PFScheduler params slot_duration_ %f, ewma_slots_ %u, alpha_ %f \n",
                  slot_duration_, ewma_slots_, alpha_);
}

int PFScheduler::command(int argc, const char*const* argv) {
  if (argc == 2) {
    if ( strcmp(argv[1], "activate-link-scheduler" ) == 0 ) {
      sched_timer_->resched( slot_duration_ );
      /* generate rates to start with */
      update_link_rate_estimate();
      tick();
      return TCL_OK;
    }
  }
  return EnsembleScheduler::command(argc, argv);
}

uint32_t PFScheduler::pick_user_to_schedule(void) const {
  /* First get the feasible users */
  std::vector<uint32_t> feasible_users = get_feasible_users();

  /* Check if there are additional abeyant users */
  for (uint32_t i=0; i < num_users_; i++) {
    if ((abeyance_.at(i) != nullptr) and (get_instantaneous_rate(i) != 0)) {
      if(std::find(feasible_users.begin(), feasible_users.end(), i) == feasible_users.end()) {
        /* printf("Adding one more abeyant user : %d \n", i); */
        feasible_users.push_back(i);
      }
    }
  }

  /* Get instantaneous rates */
  std::vector<double> instantaneous_rates(num_users_);
  for (uint32_t i=0; i < num_users_; i++) {
    instantaneous_rates.at(i) = get_instantaneous_rate(i);
  }

  /* Normalize rates */
  std::vector<double> normalized_rates( num_users_ );

  std::transform(instantaneous_rates.begin(), instantaneous_rates.end(),
                 mean_achieved_rates_.begin(), normalized_rates.begin(),
                 [&] (const double & instantanoeus, const double & average)
                 { auto norm = (average != 0 ) ? instantanoeus/average : DBL_MAX;/* printf("Norm is %f \n", norm); */ return norm;} );

  /* Pick the highest normalized rates amongst them */
  auto it = std::max_element(feasible_users.begin(), feasible_users.end(),
                             [&] (const uint32_t &f1, const uint32_t &f2)
                             { if (sub_qdisc_ == "propfair") {
                                 if (normalized_rates.at(f1) == DBL_MAX) {
                                   return false; /* f1 > f2, so f1 < f2 is false */
                                 } else if (normalized_rates.at(f2) == DBL_MAX) {
                                   return true;  /* f2 > f1, so f1 < f2 is true */
                                 } else {
                                   return normalized_rates.at(f1) < normalized_rates.at(f2);
                                 }
                               } else {
                                 assert(false);
                                 return false;
                               }
                             }
                            );
  return (it!=feasible_users.end()) ? *it : (uint64_t)-1;

}

void PFScheduler::tick(void) {
  /* Update current slot */
  current_slot_=Scheduler::instance().clock();

  /* cancel old transmission timer if required */
  if ((tx_timer_->status() == TIMER_PENDING) or (tx_timer_->status() == TimerHandler::TIMER_HANDLING)) {
    tx_timer_->cancel();
  }

  /* Update link rates */
  update_link_rate_estimate();
  chosen_user_ = pick_user_to_schedule();

  /* Update mean_achieved_rates_ */
  update_mean_achieved_rates( chosen_user_ );

  /* Transmit and reschedule yourself if required */
  transmit_pkt();
}

void PFScheduler::update_mean_achieved_rates(uint32_t scheduled_user) {
  for ( uint32_t i=0; i < mean_achieved_rates_.size(); i++ ) {
    if ( i == scheduled_user ) {
      /* printf(" Time %f Scheduled user is %d \n", Scheduler::instance().clock(), i); */
      mean_achieved_rates_.at(i) = ( 1.0 - 1.0/ewma_slots_ ) * mean_achieved_rates_.at(i) + ( 1.0/ewma_slots_ ) * get_instantaneous_rate(i);
    } else {
      mean_achieved_rates_.at(i) = ( 1.0 - 1.0/ewma_slots_ ) * mean_achieved_rates_.at(i);
    }
  }
}

void PFScheduler::transmit_pkt() {
  /* Get chosen user */
  uint32_t chosen_user = chosen_user_;

  /* If no one was scheduled, return */
  if (chosen_user == (uint32_t)-1) {
    busy_ = false;
    return;
  }

  /* If link rate is zero, reset PF slot boundary right now
     and see if someone else can be scheduled */
  if (user_links_.at(chosen_user)->bandwidth() == 0) {
    sched_timer_->resched(0.0);
    busy_ = false;
    return;
  }

  /* Get one packet from chosen user */
  /* First check abeyance_ */
  Packet* p = abeyance_.at(chosen_user);
  if (p==nullptr) {
    /* Now check the main queue */
    /* Get arrival ts of hol packet */
    //hol_ts_.at(chosen_user) = user_queues_.at(chosen_user)->get_hol();
    p = user_queues_.at(chosen_user)->deque();
    if (p!=nullptr) {
      slice_and_transmit(p, chosen_user);
    } else {
      /* We ran out of packets, move on to someone else */
      busy_ = false;
      sched_timer_->resched(0.0);
    }
  } else {
    slice_and_transmit(p, chosen_user);
  }
}

double PFScheduler::transmission_time(Packet* p, uint32_t chosen_user) {
  double bandwidth = user_links_.at(chosen_user)->bandwidth();
  if (hdr_cmn::access(p)->ptype() == PT_CELLULAR) {
    /* sliced packet */
    return hdr_cellular::access(p)->bit_size()/bandwidth;
  } else {
    /* whole packet */
    return (8. * hdr_cmn::access(p)->size() /bandwidth);
  }
}

void PFScheduler::slice_and_transmit(Packet *p, uint32_t chosen_user) {
  /* Get queue_handler */
  auto queue_handler = &user_queues_.at(chosen_user)->qh_;

  /* Get transmission time */
  double txt = transmission_time(p, chosen_user);

  /* Check if packet txtime spills over into the next time slot. If so, slice it */
  if (txt+Scheduler::instance().clock() > current_slot_ + slot_duration_) {
    auto sliced_bits =(current_slot_+ slot_duration_ - Scheduler::instance().clock())
                      * user_links_.at(chosen_user)->bandwidth();
//    printf(" PFTxTimer::expire, Chosen_user %d, slicing %f bits \n", chosen_user, sliced_bits);

    /* Slice packet */
    Packet* sliced_pkt = hdr_cellular::slice(p, floor(sliced_bits/8), sliced_bits);

    /* Find remnants of the packet */
    Packet *remnants = p->copy();
    auto remaining_bytes=hdr_cmn::access(p)->size()-hdr_cmn::access(sliced_pkt)->size();

    /* Accurately compute remaining bits */
    double remaining_bits = 0.0;
    if (hdr_cmn::access(p)->ptype() == PT_CELLULAR) {
      remaining_bits = hdr_cellular::access(p)->bit_size() - sliced_bits;
    } else {
      remaining_bits = hdr_cmn::access(p)->size()*8.0 - sliced_bits;
    }

    /* Fill in the fields of remnant packet to match sliced packet */
    hdr_cellular::fill_in(remnants, remaining_bytes, remaining_bits, true,
                          hdr_cellular::access(sliced_pkt)->tunneled_type_,
                          hdr_cellular::access(sliced_pkt)->original_size_);
    
    /* Free up old packet because it's been segmented */
    Packet::free(p);

    /* Send slice and put remnants in abeyance */
    user_links_.at(chosen_user)->recv(sliced_pkt, queue_handler);
    busy_ = true;
    abeyance_.at(chosen_user) = remnants;

  } else {
    /* Send packet onward */
    user_links_.at(chosen_user)->recv(p, queue_handler);
    busy_ = true;
    auto current_delay = Scheduler::instance().clock() + txt - hol_ts_.at(chosen_user);
    assert(current_delay > 0);
    user_delay_est_.at(chosen_user).est_delay(Scheduler::instance().clock(), current_delay);

    /* Log */
//    printf(" PFScheduler::expire, Chosen_user %d, recving %f bits @ %f \n",
//           chosen_user,
//           user_links_.at(chosen_user)->bandwidth()*txt,
//           Scheduler::instance().clock());

    /* Reset old abeyance */
    abeyance_.at(chosen_user) = nullptr;

    /* schedule next packet transmission */
    tx_timer_->resched(txt);
  }
}

double PFScheduler::hol_delay(uint32_t user_id) const {
  double hol_arrival = 0.0;
  double txtime = 0.0;
  if (abeyance_.at(user_id) != nullptr) {
    assert(hol_ts_.at(user_id)>=0);
    hol_arrival = hol_ts_.at(user_id);
    txtime = (8*hdr_cmn::access(abeyance_.at(user_id))->size())/get_link_rate_estimate(user_id);
  } else {
    assert(user_queues_.at(user_id)->get_hol()>=0);
    hol_arrival = user_queues_.at(user_id)->get_hol();
    txtime = (8*hdr_cmn::access(user_queues_.at(user_id)->get_head())->size())/get_link_rate_estimate(user_id);
  }
  double delay = Scheduler::instance().clock() - hol_arrival + txtime;
  assert(delay > 0);
  return delay;
}

void PFScheduler::reactivate_link() {
  tick();
  sched_timer_->resched(slot_duration_);
}
