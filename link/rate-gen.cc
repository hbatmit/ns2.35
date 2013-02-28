#include "link/rate-gen.h"

RateGen::RateGen(std::vector<double> allowed_rates) {
  rng_ = new RNG();
  allowed_rates_ = allowed_rates;
}

RateGen::RateGen() {
  rng_ = nullptr;
  allowed_rates_ = std::vector<double>();
}
