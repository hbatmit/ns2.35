#ifndef SFD_SCHEDULER_HH
#define SFD_SCHEDULER_HH

/*
    All scheduling disciplines for
    front end (post drop ) scheduling
    in SFD
 */
#include <map>
#include <stdint.h>
#include <queue>
#include "queue.h"
#include "rng.h"

#define QDISC_FCFS 0
#define QDISC_RAND 1

class SfdScheduler {
  private :
    /*
     * Constant reference to per flow queues that you are trying to schedule.
     * SfdScheduler only finds which flow to schedule next.
     * The actual scheduling is done by SFD.
     */
    std::map<uint64_t,PacketQueue*> const* _packet_queues;

    /* Pick between Fcfs and Rand */
    int _qdisc;
    int _iter;

    /* FIFO data structures */
    std::map<uint64_t,std::queue<uint64_t>>* _timestamps;

    /* Rand data structures */
    RNG *_rand_scheduler;

  public :
    /* constructor */
    SfdScheduler( std::map<uint64_t,PacketQueue*> * packet_queues,
                  std::map<uint64_t,std::queue<uint64_t>> * timestamps);

    /* Schedule using FCFS */
    uint64_t fcfs_scheduler( void );

    /* Schedule using Rand */
    uint64_t random_scheduler( void );

    /* setters */
    void set_qdisc( int qdisc ) ;
    void set_iter( int iter )   { _iter  = iter;  }
};

#endif
