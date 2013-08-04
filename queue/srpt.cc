#include "srpt.h"
#include <stdint.h>
#include <algorithm>
#include <float.h>

static class SrptClass : public TclClass {
  public:
    SrptClass() : TclClass("Queue/Srpt") {}
    TclObject* create(int, const char*const*) {
      return (new Srpt());
    }
} class_sfd;

Srpt::Srpt() : pkt_queues_(std::map<uint64_t,PacketQueue*>()) {}

int Srpt::command(int argc, const char*const* argv) {
  return Queue::command(argc, argv);
}

void Srpt::enque(Packet *p) {
  /* Implements pure virtual function Queue::enque() */
  uint64_t flow_id = hash( p );
  enque_packet(p, flow_id);
  return;
}

Packet* Srpt::deque() {
  /* Implements pure virtual function Queue::deque() */
  uint64_t chosen_flow = srpt();

  if ( pkt_queues_.find(chosen_flow) != pkt_queues_.end() ) {
    Packet *p = pkt_queues_.at(chosen_flow)->deque();
    return p;
  } else {
    return 0; /* empty */
  }
}

void Srpt::enque_packet( Packet* p, uint64_t flow_id ) {
  if ( pkt_queues_.find(flow_id)  != pkt_queues_.end() ) {
    pkt_queues_.at(flow_id)->enque(p);
  } else {
    pkt_queues_[flow_id] = new PacketQueue();
    pkt_queues_.at(flow_id)->enque(p);
  }
}

uint64_t Srpt::hash(Packet* pkt) {
  hdr_ip *iph=hdr_ip::access(pkt);
  return iph->flowid();
}

uint64_t Srpt::srpt() {
  typedef std::pair<uint64_t,PacketQueue*> FlowQ;
  auto flow_compare = [&] (const FlowQ & q1, const FlowQ &q2 )
                       { if (q1.second->length() == 0) return false;
                         else if(q2.second->length() == 0) return true;
                         else return hdr_ip::access(q1.second->head())->prio() < hdr_ip::access(q2.second->head())->prio() ; };
  return std::min_element(pkt_queues_.begin(), pkt_queues_.end(), flow_compare) -> first;
}
