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
    int user_id;       /* unique user_id */

    /* Underlying FIFO */
    PacketQueue* _packet_queue;

    /* Random dropper */
    SfdDropper _dropper;

    /* Rate Estimator */
    SfdRateEstimator _rate_estimator;

  public :
    SFD();
    int command(int argc, const char*const* argv) override;

    /* print stats  */
    void print_stats( double now );

    /* inherited functions from queue */
    virtual void enque( Packet *p ) override;
    virtual Packet* deque() override;
    virtual bool empty() const { return (_packet_queue->byteLength() == 0); }
};

#endif
