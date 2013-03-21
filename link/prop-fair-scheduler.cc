#include <float.h>
#include <assert.h>
#include <algorithm>
#include "prop-fair-scheduler.h"
#include "link/pf-sched-timer.h"
#include "link/pf-tx-timer.h"
#include "common/agent.h"

static class PFSchedulerClass : public TclClass {
 public :
  PFSchedulerClass() : TclClass("PFScheduler") {}
  TclObject* create(int argc, const char*const* argv) {
    return (new PFScheduler());
  }
} class_prop_fair;

PFScheduler::PFScheduler()
    : current_slot_(0.0),
      slot_duration_(0.0),
      num_users_(0),
      chosen_user_(0),
      user_queues_( std::vector<Queue*> () ),
      user_links_( std::vector<LinkDelay*> () ),
      mean_achieved_rates_( std::vector<double> () ),
      link_rates_( std::vector<double> () ),
      rate_generators_( std::vector<RateGen> () ),
      tx_timer_(new PFTxTimer(this)),
      sched_timer_(new PFSchedTimer(this, slot_duration_)),
      abeyance_(std::vector<Packet*>()) {
  bind("num_users_", &num_users_);
  bind("slot_duration_", &slot_duration_);
  sched_timer_ = new PFSchedTimer(this, slot_duration_);
  assert(num_users_ > 0);
  user_queues_ = std::vector< Queue*>(num_users_);
  user_links_  = std::vector<LinkDelay*>(num_users_);
  mean_achieved_rates_ = std::vector<double>(num_users_);
  link_rates_  = std::vector<double>(num_users_);
  rate_generators_ = std::vector<RateGen>(num_users_);
  abeyance_ = std::vector<Packet*>(num_users_);
  for ( uint32_t i=0; i < num_users_; i++ ) {
    rate_generators_.at( i ) = RateGen ( ALLOWED_RATES.at( i ) );
    link_rates_.at( i )=0.0;
    mean_achieved_rates_.at( i )=0.0;
  }
}

uint32_t PFScheduler::pick_user_to_schedule(void) const {
  /* First get the backlogged users */
  std::vector<uint32_t> backlogged_users = get_backlogged_users();

  /* Normalize rates */
  std::vector<double> normalized_rates( link_rates_.size() );
  std::transform(link_rates_.begin(), link_rates_.end(),
                 mean_achieved_rates_.begin(), normalized_rates.begin(),
                 [&] (const double & rate, const double & average)
                 { return (average != 0 ) ? rate/average : DBL_MAX ; } );

  /* Pick the highest normalized rates amongst them */
  auto it = std::max_element(backlogged_users.begin(), backlogged_users.end(),
                             [&] (const uint64_t &f1, const uint64_t &f2)
                             { return normalized_rates.at( f1 ) < normalized_rates.at( f2 );});

  return (it!=backlogged_users.end()) ? *it : (uint64_t)-1;

}

int PFScheduler::command(int argc, const char*const* argv) {
  if (argc == 2) {
    if ( strcmp(argv[1], "activate-link-scheduler" ) == 0 ) {
      sched_timer_->resched( slot_duration_ );
      tick();
      return TCL_OK;
    }
  }
  if(argc == 4) {
    if(!strcmp(argv[1],"attach-queue")) {
      Queue* queue = (Queue*) TclObject::lookup(argv[ 2 ]);
      uint32_t user_id = atoi(argv[ 3 ]);
      user_queues_.at(user_id) = queue;
      abeyance_.at(user_id) = nullptr;
      /* ensure blocked queues */
      assert(user_queues_.at(user_id)->blocked());
      return TCL_OK;
    }
    if(!strcmp(argv[1],"attach-link")) {
      LinkDelay* link = (LinkDelay*) TclObject::lookup(argv[ 2 ]);
      uint32_t user_id = atoi(argv[ 3 ]);
      user_links_.at( user_id ) = link;
      return TCL_OK;
    }
  }
  return TclObject::command( argc, argv );
}

void PFScheduler::tick(void) {
  /* Update current slot */
  current_slot_=Scheduler::instance().clock();

  /* cancel old transmission timer if required */
  if ((tx_timer_->status() == TIMER_PENDING) or (tx_timer_->status() == TimerHandler::TIMER_HANDLING)) {
    printf(" Cancelling Pending tx timers @%f, recving \n", Scheduler::instance().clock() );
    tx_timer_->cancel();
  }

  /* generate new rates, assume immediate feedback */
  generate_new_rates();
  chosen_user_ = pick_user_to_schedule();
  printf(" chosen_user_ is %d @ %f \n", chosen_user_, Scheduler::instance().clock() );

  /* Update mean_achieved_rates_ */
  update_mean_achieved_rates( chosen_user_ );

  /* Transmit and reschedule yourself if required */
  PFScheduler::transmit_pkt(this, this->tx_timer_);
}

void PFScheduler::generate_new_rates(void) {
  /* For now, generate new rates uniformly from allowed rates */
  /* By using these directly, we are assuming perfect information */
  auto rate_generator = [&] ( RateGen r )
                        { auto rnd_index = r.rng_->uniform((int)r.allowed_rates_.size());
                          return r.allowed_rates_[ rnd_index ]; };
  std::transform(rate_generators_.begin(), rate_generators_.end(),
                 link_rates_.begin(),
                 rate_generator );

}

void PFScheduler::update_mean_achieved_rates(uint32_t scheduled_user) {
  for ( uint32_t i=0; i < mean_achieved_rates_.size(); i++ ) {
    if ( i == scheduled_user ) {
      printf(" Time %f Scheduled user is %d \n", Scheduler::instance().clock(), i);
      mean_achieved_rates_.at(i) = ( 1.0 - 1.0/EWMA_SLOTS ) * mean_achieved_rates_.at(i) + ( 1.0/EWMA_SLOTS ) * link_rates_.at(i);
    } else {
      mean_achieved_rates_.at(i) = ( 1.0 - 1.0/EWMA_SLOTS ) * mean_achieved_rates_.at(i);
    }
  }
}

std::vector<uint32_t> PFScheduler::get_backlogged_users(void) const {
  std::vector<uint32_t> backlogged_user_list;
  for ( uint32_t i=0; i < num_users_; i++ ) {
    if ( !((user_queues_.at(i))->empty()) ) {
      backlogged_user_list.push_back(i); 
    } else {
      printf(" User_queue is empty at %d \n", i );
    }
  }
  return backlogged_user_list;
}

void PFScheduler::transmit_pkt(PFScheduler* pf_sched, PFTxTimer* tx_timer ) {
  /* Get chosen user */
  uint32_t chosen_user = pf_sched->chosen_user_;

  /* If no one was scheduled, return */
  if (chosen_user==(uint32_t)-1) return;

  /* Get one packet from chosen user */
  /* First check abeyance_ */
  Packet* p = pf_sched->abeyance_.at(chosen_user);
  if (p==nullptr) {
    /* Now check the main queue */
    p = pf_sched->user_queues_.at(chosen_user)->deque();
    if (p!=nullptr) slice_and_transmit(pf_sched, tx_timer, p, chosen_user, true);
  } else {
    slice_and_transmit(pf_sched, tx_timer, p, chosen_user, false);
  }
}

void PFScheduler::slice_and_transmit(PFScheduler* pf_sched, PFTxTimer* tx_timer, Packet *p, uint32_t chosen_user, bool transmit) {
  /* Get queue_handler */
  auto queue_handler = &pf_sched->user_queues_.at(chosen_user)->qh_;

  /* Get transmission time */
  double txt = pf_sched->user_links_.at(chosen_user)->txtime(p);

  /* Check if packet txtime spills over into the next time slot. If so, slice it */
  if(txt+Scheduler::instance().clock() > pf_sched->current_slot_ + pf_sched->slot_duration_) {
    printf(" PFTxTimer::expire, Chosen_user %d, slicing %f bits \n",
            chosen_user,
            (pf_sched->current_slot_+ pf_sched->slot_duration_ - Scheduler::instance().clock()) * 
            pf_sched->link_rates_.at(chosen_user));

    auto remaining_bits = pf_sched->link_rates_.at(chosen_user)*txt -
                          ((pf_sched->current_slot_+ pf_sched->slot_duration_ - Scheduler::instance().clock())
                           * pf_sched->link_rates_.at(chosen_user));

    /* TODO Actually send sliced packet  */
    Agent dummy(PT_CBR);
    Packet* p = dummy.allocpkt(remaining_bits / 8);
    hdr_cmn::access(p)->size() = remaining_bits / 8;
    pf_sched->abeyance_.at(chosen_user) = p;
  } else {
    /* Send packet onward */
    if(transmit) pf_sched->user_links_.at(chosen_user)->recv(p, queue_handler);
    /* Log */
    printf(" PFScheduler::expire, Chosen_user %d, recving %f bits @ %f \n",
           chosen_user,
           pf_sched->link_rates_.at(chosen_user)*txt,
           Scheduler::instance().clock());

    /* Delete old abeyance if required */
    delete (pf_sched->abeyance_.at(chosen_user));
    pf_sched->abeyance_.at(chosen_user) = nullptr;

    /* schedule next packet transmission */
    tx_timer->resched(txt);
  }
}
