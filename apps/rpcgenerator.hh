#ifndef RPCGENERATOR_HH_
#define RPCGENERATOR_HH_

#include <cassert>
#include <vector>
#include <map>
#include "node.h"
#include "tcp-full.h"
#include "empvariate.hh"
#include "expvariate.hh"

class RpcGenerator;

class FlowStartTimer : public TimerHandler {
 public:
  FlowStartTimer(RpcGenerator* t_rpc_generator)
    : TimerHandler(),
      rpc_generator_(t_rpc_generator) {}
  virtual void expire(Event *e) override;
 protected:
  RpcGenerator* rpc_generator_;
};

class PoissonProcess {
 public:
  PoissonProcess(const double & event_rate,
                 const uint32_t & run)
    : inter_arrivals_(1.0 / event_rate, run) {};
  double next_event_time(void) { return last_event_ += inter_arrivals_.sample(); }
  double last_event_time(void) const { return last_event_; }
 private:
  ExpVariate inter_arrivals_;
  double last_event_ {0.0};
};

struct FlowStat {
 public:
  double flow_start_time {-1.0};
  double flow_end_time {-1.0};
  uint32_t flow_size {0};
};

/* RPC Generator */
class RpcGenerator : public TclObject {
 public:
  RpcGenerator(const uint32_t & run,
               const double & arrival_rate,
               const std::string & cdf_file,
               Node* t_sender_node,
               Node* t_receiver_node);
  RpcGenerator() = delete;
  int command(int argc, const char*const* argv) override;
  double next_flow_time(void) { return flow_arrivals_.next_event_time(); }
  uint32_t next_flow_size(void) { return flow_size_dist_.sample(); }
  void map_to_connection(const uint32_t & next_flow_size);
  void dump_fcts();
 private:
  FullTcpAgent* new_tcp_connection();
  void pin_flow_to_connection(FullTcpAgent* connection,
                              const uint32_t & flow_size);
  std::map<FullTcpAgent*, bool> connection_pool_;
  PoissonProcess flow_arrivals_;
  EmpVariate flow_size_dist_;
  FlowStartTimer flow_start_timer_;
  Node* sender_node_;
  Node* receiver_node_;
  std::map<const FullTcpAgent*, std::vector<FlowStat>> flow_stats_;
};

#endif // RPCGENERATOR_HH_
