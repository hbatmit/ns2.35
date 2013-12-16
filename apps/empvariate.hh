#ifndef EMP_VARIATE_HH_
#define EMP_VARIATE_HH_

#include <stdint.h>
#include <array>
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
  static const std::array<uint32_t, 92607> flowarray;

 private:
  uint32_t run_;
  RNG* rng_;
};

#endif // EMP_VARIATE_HH_
