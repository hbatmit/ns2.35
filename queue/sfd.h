#ifndef ns_sfd_h
#define ns_sfd_h

#include <string.h>
#include "queue.h"
#include <map>
#include <list>
#include <queue>
#include "rng.h"

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
    double _last_drop_prob;
    FlowStats();
};

class SFD : public Queue {
  private :
    /* Random dropper, one for the whole queue, pick _iter^{th} substream */
    RNG* _dropper ;
    int _iter;

    /* Underlying per flow FIFOs & incoming timestamps */
    std::map<uint64_t,PacketQueue*> _packet_queues;
    double _capacity;
    std::map<uint64_t,std::queue<uint64_t>> _timestamps;

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
    int command(int argc, const char*const* argv); 

    /* inherited functions from queue */
    void enque( Packet *p );
    Packet* deque();
};

#endif
