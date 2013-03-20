#include <float.h>
#include <assert.h>
#include <algorithm>
#include "prop-fair-scheduler.h"
#include "link/pf-sched-timer.h"
#include "link/pf-tx-timer.h"

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
      sched_timer_(new PFSchedTimer(this, slot_duration_)) {
  bind("num_users_", &num_users_);
  bind("slot_duration_", &slot_duration_);
  sched_timer_ = new PFSchedTimer(this, slot_duration_);
  assert(num_users_ > 0);
  user_queues_ = std::vector< Queue*>(num_users_);
  user_links_  = std::vector<LinkDelay*>(num_users_);
  mean_achieved_rates_ = std::vector<double>(num_users_);
  link_rates_  = std::vector<double>(num_users_);
  rate_generators_ = std::vector<RateGen>(num_users_);
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
  if ((tx_timer_->status() == TimerHandler::TIMER_PENDING) or
      (tx_timer_->status() == TimerHandler::TIMER_HANDLING)) {
    printf(" Cancelling Pending tx timers @%f, recving \n", Scheduler::instance().clock() );
    tx_timer_->cancel();
  }

  /* generate new rates, assume immediate feedback */
  generate_new_rates();
  chosen_user_ = pick_user_to_schedule();
  printf(" chosen_user_ is %d @ %f \n", chosen_user_, Scheduler::instance().clock() );
  if (chosen_user_==-1) return;

  /* Get one packet from chosen user */
  Packet *p = user_queues_.at(chosen_user_)->deque();

  /* Get queue_handler */
  auto queue_handler = &user_queues_.at(chosen_user_)->qh_;

  /* Get transmission time */
  double txt = user_links_.at(chosen_user_)->txtime(p);

  /* Get scheduler instance */
  Scheduler& s = Scheduler::instance();

  /* Check if packet txtime spills over into the next time slot. If so, slice it */
  if(txt+Scheduler::instance().clock() > current_slot_ + slot_duration_) {
    printf(" PFScheduler::expire, Chosen_user %d, slicing %f bits \n",
            chosen_user_,
            (current_slot_+ slot_duration_ - Scheduler::instance().clock()) * 
             user_links_.at(chosen_user_)->bandwidth() );
    printf(" PFScheduler::expire, Chosen_user %d, remaining %f bits \n",
            chosen_user_,
            user_links_.at(chosen_user_)->bandwidth()*txt -
            ((current_slot_+ slot_duration_ - Scheduler::instance().clock()) * 
             user_links_.at(chosen_user_)->bandwidth()) );
    /* TODO */
  } else {
    /* Send packet onward */
    user_links_.at(chosen_user_)->recv(p, queue_handler);
    /* Log */
    printf(" PFScheduler::expire, Chosen_user %d, recving %f bits @ %f \n",
           chosen_user_,
           user_links_.at(chosen_user_)->bandwidth()*txt,
           Scheduler::instance().clock());
    /* schedule next packet transmission */
    tx_timer_->resched(txt);
  }

  /* In any case, update mean_achieved_rates_ */
  update_mean_achieved_rates( chosen_user_ );
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
