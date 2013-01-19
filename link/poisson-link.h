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
    double _bandwidth;
    uint32_t _iter;

  public :
    /* constructor */
    PoissonLink( double _bandwidth, uint32_t iter );

    /* generate next inter arrival time */
    double transmission_time( double num_bits );

    /* override the recv function from LinkDelay */
    virtual void recv( Packet* p, Handler* h );
};

#endif
