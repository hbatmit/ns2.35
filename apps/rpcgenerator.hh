#ifndef RPCGENERATOR_HH_
#define RPCGENERATOR_HH_

/* RPC Generator */
class RpcGenerator : public TclObject {
 public:
  RpcGenerator();
  int command(int argc, const char*const* argv) override;
};

#endif // RPCGENERATOR_HH_
