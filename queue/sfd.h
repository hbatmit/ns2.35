#ifndef ns_sfd_h
#define ns_sfd_h

#include <float.h>
#include <string.h>
#include "ensemble-aware-queue.h"
#include <map>
#include <list>
#include <queue>
#include <string>
#include "rng.h"
#include "common/flow-stats.h"
#include "queue/sfd-dropper.h"


/*
 * Stochastic Fair Dropping : Variation of AFD
 * where drop rates for TCP are set in accordance
 * with the TCP loss equation.
 */

class SFD : public EnsembleAwareQueue {
  private :
    /* Dropping disciplines */
    void draconian_dropping(double now);
    void time_based_dropping(double now, double arrival_rate);

    /* Helper function */
    void drop_if_not_empty(double now);

    /* Tcl accessible SFD parameters */
    const double  _headroom; /* default : 0.05 */
    const uint32_t _iter;    /* random seed */
    const uint32_t _user_id;  /* unique user_id */
    const double _time_constant;    /* time constant for arrival rate est; used in drop prob */
    const std::string _drop_type; /* draconian vs time-based dropping */

    /* Internal state */
    TracedDouble _last_drop_time;       /* time at which we last dropped a packet */
    TracedDouble _arr_rate_at_drop;     /* arrival rate at last drop */
    TracedDouble _current_arr_rate;     /* current arrival rate */ 

    /* Underlying FIFO */
    PacketQueue* _packet_queue;

    /* Random dropper */
    SfdDropper _dropper;

    /* Arrival Rate Estimator */
    FlowStats _user_arrival_rate_est;

  public :
    SFD(double user_arrival_rate_time_constant, double headroom, uint32_t iter, uint32_t user_id, std::string drop_type);
    int command(int argc, const char*const* argv) override;

    /* print stats  */
    void print_stats( double now );

    /* inherited functions from queue */
    virtual void enque( Packet *p ) override;
    virtual Packet* deque() override;
    virtual bool empty() const override { return (_packet_queue->byteLength() == 0); }
    virtual double get_hol() const override { return (empty()) ? DBL_MAX : hdr_cmn::access(_packet_queue->head())->timestamp(); }
    virtual double get_arrival_rate() override { return _user_arrival_rate_est.arr_rate(); }
    virtual int length() const override { return _packet_queue->length(); }
    virtual int byteLength() const override { return _packet_queue->byteLength(); }
    virtual Packet* get_head() const override { return _packet_queue->head(); }

};

#endif
