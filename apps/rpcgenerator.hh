#ifndef RPCGENERATOR_HH_
#define RPCGENERATOR_HH_

#include <vector>
#include "tcp-full.h"
#include "empvariate.hh"

class PoissonProcess {


};

/* RPC Generator */
class RpcGenerator : public TclObject {
 public:
  RpcGenerator();
  int command(int argc, const char*const* argv) override;
 private:
  std::vector<std::pair<FullTcpAgent*, bool>> connection_pool_;
  PoissonProcess flow_arrivals_;
  EmpVariate flow_size_dist_;
};

#endif // RPCGENERATOR_HH_
