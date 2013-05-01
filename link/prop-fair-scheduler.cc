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
    return (new PFScheduler(atoi(argv[4]),
                            atof(argv[5]),
                            atof(argv[6]),
                            atoi(argv[7])));
  }
} class_prop_fair;

PFScheduler::PFScheduler(uint32_t num_users,
                         double feedback_delay,
                         double slot_duration,
                         uint32_t ewma_slots)
    : EnsembleScheduler(num_users, feedback_delay),
      current_slot_(0.0),
      slot_duration_(slot_duration),
      ewma_slots_(ewma_slots),
      chosen_user_(0),
      mean_achieved_rates_( std::vector<double> () ),
      tx_timer_(new PFTxTimer(this)),
      sched_timer_(new PFSchedTimer(this, slot_duration_)),
      abeyance_(std::vector<Packet*>()),
      slicing_agent_(Agent(PT_CELLULAR)) {
  sched_timer_ = new PFSchedTimer(this, slot_duration_);
  mean_achieved_rates_ = std::vector<double>(num_users_);
  abeyance_ = std::vector<Packet*>(num_users_);
  for ( uint32_t i=0; i < num_users_; i++ ) {
    mean_achieved_rates_.at( i )=0.0;
    abeyance_.at(i) = nullptr;
  }
  fprintf(stderr, "PFScheduler parameters slot_duration_ %f, ewma_slots_ %u \n",
                  slot_duration_, ewma_slots_);
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
    if( (abeyance_.at(i) != nullptr) and (link_rates_.at(i) != 0) ) {
      if(std::find(feasible_users.begin(), feasible_users.end(), i) == feasible_users.end()) {
        /* printf("Adding one more abeyant user : %d \n", i); */
        feasible_users.push_back(i);
      }
    }
  }

  /* Normalize rates */
  std::vector<double> normalized_rates( link_rates_.size() );
  std::transform(link_rates_.begin(), link_rates_.end(),
                 mean_achieved_rates_.begin(), normalized_rates.begin(),
                 [&] (const double & rate, const double & average)
                 { auto norm = (average != 0 ) ? rate/average : DBL_MAX/10000.0 ;/* printf("Norm is %f \n", norm); */ return norm;} );

  /* Pick the highest normalized rates amongst them */
  auto abeyance_len = [&] (const Packet* tmp) { return (tmp != nullptr) ? hdr_cmn::access(tmp)->size() : 0;};
  auto it = std::max_element(feasible_users.begin(), feasible_users.end(),
                             [&] (const uint64_t &f1, const uint64_t &f2)
                             { uint32_t q1 = abeyance_len(abeyance_.at(f1)) + user_queues_.at(f1)->byteLength();
                               uint32_t q2 = abeyance_len(abeyance_.at(f2)) + user_queues_.at(f2)->byteLength();
                               return (normalized_rates.at(f1) * q1) < (normalized_rates.at(f2) * q2) ;});

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
      mean_achieved_rates_.at(i) = ( 1.0 - 1.0/ewma_slots_ ) * mean_achieved_rates_.at(i) + ( 1.0/ewma_slots_ ) * link_rates_.at(i);
    } else {
      mean_achieved_rates_.at(i) = ( 1.0 - 1.0/ewma_slots_ ) * mean_achieved_rates_.at(i);
    }
  }
}

void PFScheduler::transmit_pkt() {
  /* Get chosen user */
  uint32_t chosen_user = chosen_user_;

  /* If no one was scheduled, return */
  if (chosen_user == (uint32_t)-1) return;

  /* If link rate is zero, return */
  if (user_links_.at(chosen_user)->bandwidth() == 0) {
    sched_timer_->resched(0.0);
    return;
  }

  /* Get one packet from chosen user */
  /* First check abeyance_ */
  Packet* p = abeyance_.at(chosen_user);
  if (p==nullptr) {
    /* Now check the main queue */
    p = user_queues_.at(chosen_user)->deque();
    if (p!=nullptr) {
      slice_and_transmit(p, chosen_user);
    } else {
      sched_timer_->resched(0.0);
    }
  } else {
    slice_and_transmit(p, chosen_user);
  }
}

void PFScheduler::slice_and_transmit(Packet *p, uint32_t chosen_user) {
  /* Get queue_handler */
  auto queue_handler = &user_queues_.at(chosen_user)->qh_;

  /* Get transmission time */
  double txt = user_links_.at(chosen_user)->txtime(p);

  /* Check if packet txtime spills over into the next time slot. If so, slice it */
  if(txt+Scheduler::instance().clock() > current_slot_ + slot_duration_) {
    auto sliced_bits =(current_slot_+ slot_duration_ - Scheduler::instance().clock())
                      * user_links_.at(chosen_user)->bandwidth();
//    printf(" PFTxTimer::expire, Chosen_user %d, slicing %f bits \n", chosen_user, sliced_bits);

    /* Slice packet */
    Packet* sliced_pkt = hdr_cellular::slice(p, floor(sliced_bits/8));

    /* Find remnants of the packet */
    Packet *remnants = p->copy();
    auto remaining_bytes=hdr_cmn::access(p)->size()-hdr_cmn::access(sliced_pkt)->size();

    /* Fill in the fields of remnant packet to match sliced packet */
    hdr_cellular::fill_in(remnants, remaining_bytes, true,
                          hdr_cellular::access(sliced_pkt)->tunneled_type_,
                          hdr_cellular::access(sliced_pkt)->original_size_);
    
    /* Send slice and put remnants in abeyance */
    user_links_.at(chosen_user)->recv(sliced_pkt, queue_handler);
    abeyance_.at(chosen_user) = remnants;

  } else {
    /* Send packet onward */
    user_links_.at(chosen_user)->recv(p, queue_handler);

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
