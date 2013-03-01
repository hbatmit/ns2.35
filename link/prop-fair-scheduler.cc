#include <float.h>
#include <assert.h>
#include <algorithm>
#include "prop-fair-scheduler.h"

static class PropFairClass : public TclClass {
 public :
  PropFairClass() : TclClass("PropFair") {}
  TclObject* create(int argc, const char*const* argv) {
    return (new PropFair());
  }
} class_prop_fair;

PropFair::PropFair()
    : slot_duration_(0.0),
      num_users_(0),
      chosen_user_(0),
      last_time_(0.0),
      user_queues_( std::vector<const Queue*> () ),
      user_links_( std::vector<const LinkDelay*> () ),
      mean_achieved_rates_( std::vector<double> () ),
      link_rates_( std::vector<double> () ),
      rate_generators_( std::vector<RateGen> () ) {
  bind("num_users_", &num_users_);
  bind("slot_duration_", &slot_duration_);
  assert(num_users_ > 0);
  user_queues_ = std::vector<const Queue*>(num_users_);
  user_links_  = std::vector<const LinkDelay*>(num_users_);
  mean_achieved_rates_ = std::vector<double>(num_users_);
  link_rates_  = std::vector<double>(num_users_);
  rate_generators_ = std::vector<RateGen>(num_users_);
  for ( uint32_t i=0; i < num_users_; i++ ) {
    rate_generators_.at( i ) = RateGen ( ALLOWED_RATES.at( i ) );
    link_rates_.at( i )=0.0;
    mean_achieved_rates_.at( i )=0.0;
  }
}

uint32_t PropFair::pick_user_to_schedule(void) const {
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

int PropFair::command(int argc, const char*const* argv) {
  if (argc == 2) {
    if ( strcmp(argv[1], "activate-link-scheduler" ) == 0 ) {
      resched( slot_duration_ );
      tick();
      return TCL_OK;
    }
  }
  if(argc == 4) {
    if(!strcmp(argv[1],"attach-queue")) {
      Queue* queue = (Queue*) TclObject::lookup(argv[ 2 ]);
      uint32_t user_id = atoi(argv[ 3 ]);
      user_queues_.at( user_id ) = queue;
      assert(user_queues_.at( user_id )->blocked());
      /* ensure blocked queues */
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

void PropFair::tick(void) {
  generate_new_rates();
  chosen_user_ = pick_user_to_schedule();
  update_mean_achieved_rates( chosen_user_ ); 
}

void PropFair::generate_new_rates(void) {
  /* For now, generate new rates uniformly from allowed rates */
  /* By using these directly, we are assuming perfect information */
  auto rate_generator = [&] ( RateGen r )
                        { auto rnd_index = r.rng_->uniform((int)r.allowed_rates_.size());
                          return r.allowed_rates_[ rnd_index ]; };
  std::transform(rate_generators_.begin(), rate_generators_.end(),
                 link_rates_.begin(),
                 rate_generator );

}

void PropFair::update_mean_achieved_rates(uint32_t scheduled_user) {
  for ( uint32_t i=0; i < mean_achieved_rates_.size(); i++ ) {
    if ( i == scheduled_user ) {
      printf(" Time %f Scheduled user is %d \n", Scheduler::instance().clock(), i);
      mean_achieved_rates_.at(i) = ( 1.0 - 1.0/EWMA_SLOTS ) * mean_achieved_rates_.at(i) + ( 1.0/EWMA_SLOTS ) * link_rates_.at(i);
    } else {
      mean_achieved_rates_.at(i) = ( 1.0 - 1.0/EWMA_SLOTS ) * mean_achieved_rates_.at(i);
    }
  }
}

std::vector<uint32_t> PropFair::get_backlogged_users(void) const {
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

void PropFair::expire(Event *e) {
  /* TimerHandler::expire */
  tick();
  resched( slot_duration_ );
  last_time_ = Scheduler::instance().clock();
}
