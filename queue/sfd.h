#ifndef ns_sfd_h
#define ns_sfd_h

#include <string.h>
#include "queue.h"
#include <map>
#include <list>

/*
 * Stochastic Fair Dropping : Variation of AFD 
 * where drop rates for TCP are set in accordance 
 * with the TCP loss equation.
 */

class FlowStats {
  public :
    double _last_arrival ;
    double _acc_pkt_size;
    double _flow_rate;
    FlowStats();
};

class SFD : public Queue {
  private :
    /* Underlying FIFO and link speed */
    PacketQueue *_packet_queue;
    double _capacity;

    /* Hash from packet to flow */
    uint64_t hash( Packet *p );

    /* Per flow stats for rate estimation (borrowed from CSFQ) */
    std::map<uint64_t,FlowStats> _flow_stats;
    static constexpr double  K = 0.2; /* 200 ms */
    double est_flow_rate( uint64_t flow_id, double now, Packet* p );

    /* Fair share rate of link */
    double est_fair_share() ;
    double _fair_share;

    /* Probabilistic dropping */
    bool should_drop( double prob ); 

  public :
    SFD( double capacity );

    /* inherited functions from queue */
    void enque( Packet *p );
    Packet* deque();
};

#endif
