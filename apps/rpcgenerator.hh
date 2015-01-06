#ifndef RPCGENERATOR_HH_
#define RPCGENERATOR_HH_

#include <cassert>
#include <vector>
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

/* RPC Generator */
class RpcGenerator : public TclObject {
 public:
  RpcGenerator(const uint32_t & run,
               const double & arrival_rate,
               const std::string & cdf_file);
  RpcGenerator() = delete;
  int command(int argc, const char*const* argv) override;
  double next_flow_time(void) { return flow_arrivals_.next_event_time(); }
  double next_flow_size(void) { return flow_size_dist_.sample(); }
  void map_to_connection(double next_flow_size);
  FullTcpAgent* new_tcp_connection(Node * sender_node = nullptr,
                                       Node * receiver_node = nullptr);
 private:
  std::vector<std::pair<FullTcpAgent*, bool>> connection_pool_;
  PoissonProcess flow_arrivals_;
  EmpVariate flow_size_dist_;
  FlowStartTimer flow_start_timer_;
};

#endif // RPCGENERATOR_HH_
