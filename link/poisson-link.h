#ifndef POISSON_LINK_HH
#define POISSON_LINK_HH

#include "delay.h"
#include <vector>
#include "rng.h"

/*
 * A link with a Poisson process
 * that dictates packet arrivals.
 * First order model for a fixed
 * rate wireless link.
 */

class PoissonLink : public LinkDelay {
  private :
    RNG* _arrivals;
    double _lambda;
    uint32_t _iter;
    double _next_rx;

  public :
    /* constructor */
    PoissonLink( double lambda, uint32_t iter );

    /* generate next inter arrival time */
    double generate_interarrival( void );

    /* override the recv function from LinkDelay */
    virtual void recv( Packet* p, Handler* h );
};

#endif
