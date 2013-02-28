#ifndef LINK_RATE_GEN_H
#define LINK_RATE_GEN_H

#include <vector>
#include "rng.h"

/* Auxillary class :
   Generate rates for each user from a set of rates
 */

class RateGen {
  public:
    RNG* rng_;
    std::vector<double> allowed_rates_;
    RateGen(std::vector<double> allowed_rates);
    RateGen( void );
};

#endif
