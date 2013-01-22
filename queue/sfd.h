#ifndef ns_sfd_h
#define ns_sfd_h

#include <string.h>
#include "queue.h"
#include <map>
#include <list>
#include <queue>
#include "rng.h"
#include "flow-stats.h"
#include "sfd-rate-estimator.h"

/*
 * Stochastic Fair Dropping : Variation of AFD
 * where drop rates for TCP are set in accordance
 * with the TCP loss equation.
 */

#define QDISC_FCFS 0
#define QDISC_RAND 1

class SFD : public Queue {
  private :
    /* Queuing discipline */
    int _qdisc;

    /* Tcl accessible SFD parameters */
    double _capacity;
    double  _K; /* default : 200 ms */
    double  _headroom; /* default : 0.05 */

    /* Underlying per flow FIFOs and enque wrapper */
    std::map<uint64_t,PacketQueue*> _packet_queues;
    void enque_packet( Packet* p, uint64_t flow_id );

    /* Random dropping :
       Toss a coin
       Pick one of the longest queues at random
       Drop from front of that queue
     */
    RNG* _dropper ;
    RNG* _queue_picker;
    int _iter;
    uint64_t longest_queue( void );

    /* Random scheduler, one for whole queue, pick _iter^{th} substream for RNG */
    RNG* _rand_scheduler;
    uint64_t random_scheduler( void );

    /* Fcfs scheduler, use timestamps */
    std::map<uint64_t,std::queue<uint64_t>> _timestamps;
    uint64_t fcfs_scheduler( void );
    uint64_t _counter;

    /* Hash from packet to flow */
    uint64_t hash( Packet *p );

    /* Rate Estimator */
    SfdRateEstimator _rate_estimator;

    /* Probabilistic dropping */
    bool should_drop( double prob );

  public :
    SFD( double capacity );
    int command(int argc, const char*const* argv);

    /* inherited functions from queue */
    void enque( Packet *p );
    Packet* deque();
};

#endif
