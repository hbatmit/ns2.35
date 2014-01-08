#include <cassert>
#include "expvariate.hh"

ExpVariate::ExpVariate(double average, uint32_t t_run)
    : run_(t_run),
      average_(average),
      rng_(RNG::defaultrng()) {
  /* Reset until the appropriate run */
  assert( run_ > 0 );
  for (uint32_t i = 1; i <= run_; i++) {
    rng_->reset_next_substream();
  }
}

double ExpVariate::sample(void) {
  return rng_->exponential(average_);
}
