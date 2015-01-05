#include <cassert>
#include <string>
#include "ezio.hh"
#include "config.h"
#include "rpcgenerator.hh"

void FlowStartTimer::expire(Event *e) {
  auto next_flow_size = rpc_generator_->next_flow_size();
  rpc_generator_->map_to_connection(next_flow_size);
  resched(rpc_generator_->next_flow_time());
}

RpcGenerator::RpcGenerator(const uint32_t & run,
                           const double & arrival_rate,
                           const std::string & cdf_file)
    : connection_pool_(),
      flow_arrivals_(arrival_rate, run),
      flow_size_dist_(cdf_file, run),
      flow_start_timer_(this) {
  /* Determine time to schedule first flow */
  assert(flow_arrivals_.last_event_time() == 0);
  flow_start_timer_.sched(flow_arrivals_.next_event_time());
}

static class RpcGeneratorClass : public TclClass {
 public:
  RpcGeneratorClass() : TclClass("RpcGenerator") {}
  TclObject* create(int argc, const char*const* argv) {
    assert(argc == 7);

    /* Get arguments */
    uint32_t run = myatoi(argv[4]);
    assert(run > 0);

    double flow_rate = myatod(argv[5]);
    assert(flow_rate > 0.0);

    std::string cdf_file = argv[6];
    assert(cdf_file != "");

    return new RpcGenerator(run,
                            flow_rate,
                            cdf_file);
  }
} class_rpc_generator;

int RpcGenerator::command(int argc, const char*const* argv) {
  return TclObject::command(argc, argv);
}
