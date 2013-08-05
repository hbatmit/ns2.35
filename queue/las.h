#ifndef QUEUE_LAS_H_
#define QUEUE_LAS_H_

#include <string.h>
#include <map>
#include <list>
#include <queue>
#include "queue/queue.h"
#include "tools/rng.h"

/*
 * LAS: Least Attained Service
 */

class Las : public Queue {
 public :
  Las();
  int command(int argc, const char*const* argv);

  /* inherited functions from queue */
  virtual void enque(Packet *p) override;
  virtual Packet* deque() override;

 private :
  /* Hash from packet to flow */
  uint64_t hash(Packet *p);

  /* Pick the flow with the LAS */
  uint64_t las(void);

  /* Underlying per flow FIFOs and enque wrapper */
  std::map<uint64_t, PacketQueue*> pkt_queues_;
  void enque_packet(Packet* p, uint64_t flow_id);

  /* Maintain attained service for each flow */
  std::map<uint64_t, uint32_t> attained_service_;
};

#endif  // QUEUE_LAS_H_
