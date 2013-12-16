#ifndef EXP_VARIATE_HH_
#define EXP_VARIATE_HH_

#include <stdint.h>
#include "rng.h"

/*
   Random exponential variate with a specific seed,
   done the ns-2 way (Section 25.1.1. of
   the manual)
 */
class ExpVariate {
 public:
  ExpVariate(double average, uint32_t t_run);
  double sample(void);
 private:
  uint32_t run_;
  const double average_;
  RNG* rng_;
};

#endif // EXP_VARIATE_HH_
