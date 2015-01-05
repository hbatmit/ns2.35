#ifndef RPCGENERATOR_HH_
#define RPCGENERATOR_HH_

#include <vector>
#include "tcp-full.h"
#include "empvariate.hh"
#include "expvariate.hh"

class PoissonProcess {
 public:
  PoissonProcess(const double & event_rate,
                 const uint32_t & run)
    : inter_arrivals_(1.0 / event_rate, run) {};
  double next_event_time(void) { return last_event_ += inter_arrivals_.sample(); }
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
 private:
  std::vector<std::pair<FullTcpAgent*, bool>> connection_pool_;
  PoissonProcess flow_arrivals_;
  EmpVariate flow_size_dist_;
};

#endif // RPCGENERATOR_HH_
