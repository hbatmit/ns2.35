#ifndef EMP_VARIATE_HH_
#define EMP_VARIATE_HH_

#include <stdint.h>
#include "rng.h"

/*
   Empirical Random variate with a specific seed,
   done the ns-2 way (Section 25.1.1. of
   the manual)
 */
class EmpVariate {
 public:
  EmpVariate(uint32_t t_run);
  double sample(void);
  static const uint32_t flowarray[92607];

 private:
  uint32_t run_;
  RNG* rng_;
};

#endif // EMP_VARIATE_HH_
