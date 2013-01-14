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

class TaggedPacket {
  Packet p;
  double _arrived;
  double _delivered;
};

class SFD : public Queue {
  public :
    SFD();
  protected :
    PacketQueue _packet_queue; /* Underlying FIFO */

    /* inherited functions from queue */
    void enque( Packet * );
    Packet* deque();

    /* Maintain per flow stats */
    /* Map from flow_id (after hashing) to flow_history i.e list of Tagged packets */
    std::map<uint32_t,std::list<TaggedPacket>> _flow_stats;
};

#endif
