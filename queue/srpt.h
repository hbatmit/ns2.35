#ifndef ns_srpt_h_
#define ns_srpt_h_

#include <string.h>
#include "queue.h"
#include <map>
#include <list>
#include <queue>
#include "rng.h"


/*
 * SRPT: Shortest Remaining Processing Time first
 */

class Srpt : public Queue {
 public :
  Srpt();
  int command(int argc, const char*const* argv);

  /* inherited functions from queue */
  virtual void enque(Packet *p) override;
  virtual Packet* deque() override;

 private :
  /* Underlying per flow FIFOs and enque wrapper */
  std::map<uint64_t, PacketQueue*> pkt_queues_;
  void enque_packet(Packet* p, uint64_t flow_id);

  /* Hash from packet to flow */
  uint64_t hash( Packet *p );

  /* Pick the flow with the SRPT */
  uint64_t srpt(void);
};

#endif
