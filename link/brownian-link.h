#ifndef BROWNIAN_LINK_HH
#define BROWNIAN_LINK_HH

/* A link with an inhomogenous Poisson
 * process that dictates packet arrivals.
 * First, sample a rate from a rate distribution.
 * Then, sample a transmission time using that rate.
 */

#include "delay.h"
#include "rng.h"

class BrownianLink : public LinkDelay {
  private :
      RNG* _rate;
      double _min_rate;
      double _max_rate;
      RNG* _arrivals;
      uint32_t _iter;

  public :
      /* constructor */
      BrownianLink( double _min_rate, double _max_rate, uint32_t _iter );

      /* generate next transmission time */
      double transmission_time( double num_bits );

      /* override the recv function from LinkDelay */
      virtual void recv( Packet* p, Handler* h);
};

#endif
