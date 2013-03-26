#include <assert.h>
#include <algorithm>
#include "link/ensemble-scheduler.h"

EnsembleScheduler::EnsembleScheduler()
    : num_users_(0),
      user_queues_(std::vector<Queue*>()),
      user_links_(std::vector<LinkDelay*>()),
      link_rates_(std::vector<double>()),
      rate_generators_(std::vector<RateGen>()) {
  bind("num_users_", &num_users_);
  assert(num_users_ > 0);
  user_queues_ = std::vector< Queue*>(num_users_);
  user_links_  = std::vector<LinkDelay*>(num_users_);
  link_rates_  = std::vector<double>(num_users_);
  rate_generators_ = std::vector<RateGen>(num_users_);
  for ( uint32_t i=0; i < num_users_; i++ ) {
    rate_generators_.at( i ) = RateGen ( ALLOWED_RATES.at( i ) );
    link_rates_.at( i )=0.0;
  }
}

int EnsembleScheduler::command(int argc, const char*const* argv) {
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

void EnsembleScheduler::generate_new_rates(void) {
  /* For now, generate new rates uniformly from allowed rates */
  /* By using these directly, we are assuming perfect information */
  auto rate_generator = [&] ( RateGen r )
                        { auto rnd_index = r.rng_->uniform((int)r.allowed_rates_.size());
                          return r.allowed_rates_[ rnd_index ]; };
  std::transform(rate_generators_.begin(), rate_generators_.end(),
                 link_rates_.begin(),
                 rate_generator );

  /* Update user_links_ */
  for (uint32_t i=0; i<link_rates_.size(); i++) {
    user_links_.at(i)->set_bandwidth(link_rates_.at(i));
  }
}

std::vector<uint32_t> EnsembleScheduler::get_backlogged_users(void) const {
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
