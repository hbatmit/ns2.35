#ifndef EMP_VARIATE_HH_
#define EMP_VARIATE_HH_

#include <string>
#include <vector>
#include <cstdint>
#include "rng.h"

/*
   Empirical Random variate with a specific seed,
   done the ns-2 way (Section 25.1.1. of
   the manual)
 */
class EmpVariate {
 public:
  EmpVariate(std::string cdf_file, uint32_t t_run);
  uint32_t sample(void);

 private:
  const uint32_t run_;
  RNG* rng_;
  std::vector<uint32_t> flow_array_;
};

#endif // EMP_VARIATE_HH_
