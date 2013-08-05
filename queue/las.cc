#include <stdint.h>
#include <float.h>
#include <algorithm>
#include <utility>
#include "queue/las.h"

static class LasClass : public TclClass {
  public:
    LasClass() : TclClass("Queue/Las") {}
    TclObject* create(int, const char*const*) {
      return (new Las());
    }
} class_sfd;

Las::Las()
    : pkt_queues_(std::map<uint64_t, PacketQueue*>()),
      attained_service_(std::map<uint64_t, uint32_t>())
{}

int Las::command(int argc, const char*const* argv) {
  return Queue::command(argc, argv);
}

void Las::enque(Packet *p) {
  /* Implements pure virtual function Queue::enque() */
  uint64_t flow_id = hash(p);
  enque_packet(p, flow_id);
  return;
}

Packet* Las::deque() {
  /* Implements pure virtual function Queue::deque() */
  uint64_t chosen_flow = las();

  if ( pkt_queues_.find(chosen_flow) != pkt_queues_.end() ) {
    Packet *p = pkt_queues_.at(chosen_flow)->deque();
    if (p != nullptr) {
      attained_service_.at(chosen_flow) += hdr_cmn::access(p)->size();
    }
    if (pkt_queues_.at(chosen_flow)->length() == 0) { /* Queue is empty now */
      attained_service_.at(chosen_flow) = 0; /* reset flow counter */
    }
    return p;
  } else {
    return 0; /* empty */
  }
}

void Las::enque_packet(Packet* p, uint64_t flow_id) {
  if ( pkt_queues_.find(flow_id)  != pkt_queues_.end() ) {
    pkt_queues_.at(flow_id)->enque(p);
  } else {
    pkt_queues_[flow_id] = new PacketQueue();
    attained_service_[flow_id] = 0; /* New flow, never seen before */
    pkt_queues_.at(flow_id)->enque(p);
  }
}

uint64_t Las::hash(Packet* pkt) {
  hdr_ip *iph = hdr_ip::access(pkt);
  return iph->flowid();
}

uint64_t Las::las() {
  typedef std::pair<uint64_t, uint32_t> FlowServices;
  auto flow_compare = [&] (const FlowServices & q1, const FlowServices &q2 )
                      { if (pkt_queues_.at(q1.first)->length() == 0) return false;
                        else if (pkt_queues_.at(q2.first)->length() == 0) return true;
                        else return q1.second < q2.second ; };
  return std::min_element(attained_service_.begin(), attained_service_.end(), flow_compare) -> first;
}
