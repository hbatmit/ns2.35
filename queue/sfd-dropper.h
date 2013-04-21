#ifndef SFD_DROPPER_HH
#define SFD_DROPPER_HH

#include <map>
#include "rng.h"
#include "queue.h"

/* Random dropping :
 * Toss a coin
 * Pick one of the longest queues at random
 * Drop from front of that queue
 */

class SfdDropper {
  private :
    RNG* _pkt_dropper;
    int _iter;

  public :
    /* constructor */
    SfdDropper( uint32_t iter );

    /* Calc drop_probability using fair share equation */
    double drop_probability( double fair_share, double arrival_rate );

    /* Toss a biased coin */
    bool should_drop( double prob );

};

#endif
