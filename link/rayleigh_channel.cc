#include "link/rayleigh_channel.h"
#include "common/scheduler.h"

RayleighChannel::RayleighChannel(double coherence_time, double noise_sigma)
    : iter_(0),
      coherence_time_(coherence_time),
      noise_sigma_(noise_sigma),
      last_channel_update_(Scheduler::instance().clock()),
      last_channel_magnitude_() {}

void RayleighChannel::set_iter(int iter) {
  if ( iter == 0 ) {
    printf(" Iter can't be 0 \n");
    exit(-5);
  }
  iter_ = iter;
  for ( int i = 1; i < iter_; i++ ) {
    channel_generator_->reset_next_substream();
    noise_generator_->reset_next_substream();
  }
}

double RayleighChannel::get_channel(void) {
  if (Scheduler::instance().clock() > last_channel_update_ + coherence_time_) {
    /* First generate a uniform random vairable */
    auto uniform = channel_generator_->next_double();

    /* Update state */
    last_channel_update_    = Scheduler::instance().clock();
    last_channel_magnitude_ = sqrt(-2*log(uniform));
  }
  return last_channel_magnitude_;
}

double RayleighChannel::get_noise(void) {
  /* return AWGN noise */
  return noise_generator_->normal(0.0, noise_sigma_);
}
