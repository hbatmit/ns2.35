#include <cassert>
#include <string>
#include "ezio.hh"
#include "config.h"
#include "rpcgenerator.hh"

using namespace std;

void FlowStartTimer::expire(Event *e) {
  auto next_flow_size = rpc_generator_->next_flow_size();
  rpc_generator_->map_to_connection(next_flow_size);
  resched(rpc_generator_->next_flow_time());
}

RpcGenerator::RpcGenerator(const uint32_t & run,
                           const double & arrival_rate,
                           const std::string & cdf_file,
                           Node* t_sender_node,
                           Node* t_receiver_node)
    : connection_pool_(),
      flow_arrivals_(arrival_rate, run),
      flow_size_dist_(cdf_file, run),
      flow_start_timer_(this),
      sender_node_(t_sender_node),
      receiver_node_(t_receiver_node) {
  /* Determine time to schedule first flow */
  assert(flow_arrivals_.last_event_time() == 0);
  flow_start_timer_.sched(flow_arrivals_.next_event_time());
}

static class RpcGeneratorClass : public TclClass {
 public:
  RpcGeneratorClass() : TclClass("RpcGenerator") {}
  TclObject* create(int argc, const char*const* argv) {
    assert(argc == 9);

    /* Get arguments */
    uint32_t run = myatoi(argv[4]);
    assert(run > 0);

    double flow_rate = myatod(argv[5]);
    assert(flow_rate > 0.0);

    std::string cdf_file = argv[6];
    assert(cdf_file != "");

    Node* sender_node = dynamic_cast<Node*>(TclObject::lookup(argv[7]));
    assert(sender_node != nullptr);

    Node* receiver_node = dynamic_cast<Node*>(TclObject::lookup(argv[8]));
    assert(receiver_node != nullptr);

    return new RpcGenerator(run,
                            flow_rate,
                            cdf_file,
                            sender_node,
                            receiver_node);
  }
} class_rpc_generator;

int RpcGenerator::command(int argc, const char*const* argv) {
  return TclObject::command(argc, argv);
}

FullTcpAgent* RpcGenerator::new_tcp_connection() {
   // Create agents in Tcl
   Tcl & tcl = Tcl::instance();

   tcl.evalf("set sender_tcp [new Agent/TCP/FullTcp/Sack]");
   auto sender_tcp   = dynamic_cast<FullTcpAgent*>(tcl.lookup(tcl.result()));
   assert(sender_tcp != nullptr);

   tcl.evalf("set receiver_tcp [new Agent/TCP/FullTcp/Sack]");
   auto receiver_tcp = dynamic_cast<FullTcpAgent*>(tcl.lookup(tcl.result()));
   assert(receiver_tcp != nullptr);

   // Attach agents to nodes
   assert(sender_node_ != nullptr);
   assert(receiver_node_ != nullptr);
   tcl.evalf ("%s attach %s", sender_node_->name(), sender_tcp->name());
   tcl.evalf ("%s attach %s", receiver_node_->name(), receiver_tcp->name());

   // setup connection
   tcl.evalf ("%s listen", receiver_tcp->name());
   tcl.evalf ("[Simulator instance] connect %s %s",
              sender_tcp->name(),
              receiver_tcp->name());

   return sender_tcp;
}

void RpcGenerator::map_to_connection(const uint32_t & next_flow_size) {
  if (connection_pool_.empty()) {
    auto new_connection = new_tcp_connection();
    connection_pool_.push_back(make_pair(new_connection, true));
    new_connection->signal_on_empty() = TRUE;
    new_connection->advanceby(next_flow_size);
    return;
  }

  /* go through the pool now */
  auto it = connection_pool_.begin();
  for (it = connection_pool_.begin(); it != connection_pool_.end(); it++) {
    if (it->second == false) {
      it->first->signal_on_empty() = TRUE;
      it->first->advanceby(next_flow_size);
      it->second = true;
      break;
    }
  }

  /* No idle connections */
  if (it == connection_pool_.end()) {
    auto new_connection = new_tcp_connection();
    connection_pool_.push_back(make_pair(new_connection, true));
    new_connection->signal_on_empty() = TRUE;
    new_connection->advanceby(next_flow_size);
  }
}
