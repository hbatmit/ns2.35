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
    std::map<uint64_t,PacketQueue*> const * _packet_queues;
    RNG* _pkt_dropper;
    RNG* _queue_picker;
    int _iter;

  public :
    /* constructor */
    SfdDropper( std::map<uint64_t,PacketQueue*>* packet_queues );

    /* Pick one of the longest queues,
       breaking ties at random */
    uint64_t longest_queue( void );

    /* Calc drop_probability using fair share equation */
    double drop_probability( double fair_share, double arrival_rate );

    /* Set seed */
    void set_iter( int iter );

    /* Toss a biased coin */
    bool should_drop( double prob );

};

#endif
