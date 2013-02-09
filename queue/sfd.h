#ifndef ns_sfd_h
#define ns_sfd_h

#include <string.h>
#include "link-aware-queue.h"
#include <map>
#include <list>
#include <queue>
#include "rng.h"
#include "flow-stats.h"
#include "sfd-rate-estimator.h"
#include "sfd-scheduler.h"
#include "sfd-dropper.h"


/*
 * Stochastic Fair Dropping : Variation of AFD
 * where drop rates for TCP are set in accordance
 * with the TCP loss equation.
 */

class SFD : public LinkAwareQueue {
  private :

    /* Tcl accessible SFD parameters */
    int _qdisc;        /* Queuing discipline */
    double  _K;        /* default : 200 ms */
    double  _headroom; /* default : 0.05 */
    int _iter;         /* random seed */

    /* Underlying per flow FIFOs and enque wrapper */
    std::map<uint64_t,PacketQueue*> _packet_queues;
    void enque_packet( Packet* p, uint64_t flow_id );

    /* Random dropper */
    SfdDropper _dropper;

    /* Scheduler */
    std::map<uint64_t,std::queue<uint64_t>> _timestamps; /* For FCFS */
    uint64_t _counter;
    SfdScheduler _scheduler;

    /* Hash from packet to flow */
    uint64_t hash( Packet *p ) { return ( hdr_ip::access(p) )->flowid(); };

    /* Rate Estimator */
    SfdRateEstimator _rate_estimator;

  public :
    SFD();
    int command(int argc, const char*const* argv);

    /* print stats  */
    void print_stats( double now );

    /* inherited functions from LinkAwareQueue */
    virtual std::map<uint64_t,double> get_link_rates();

    /* inherited functions from queue */
    virtual void enque( Packet *p );
    virtual Packet* deque();
};

#endif
