#include <fstream>
#include "empvariate.hh"
EmpVariate::EmpVariate(std::string cdf_file, uint32_t t_run)
    : run_(t_run),
      rng_(RNG::defaultrng()) {
  /* Read CDF file into flow array */
  std::ifstream in_file(cdf_file);
  uint32_t flow_size;
  while (in_file >> flow_size) {
    flow_array_.push_back(flow_size);
  }

  /* Reset until the appropriate run */
  for (uint32_t i = 1; i <= run_; i++) {
    rng_->reset_next_substream();
  }
}

uint32_t EmpVariate::sample(void) {
  return flow_array_.at(int(rng_->uniform(0, flow_array_.size() - 1)));
}
