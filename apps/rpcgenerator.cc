#include <cassert>
#include <string>
#include "config.h"
#include "rpcgenerator.hh"

RpcGenerator::RpcGenerator() {}

static class RpcGeneratorClass : public TclClass {
 public:
  RpcGeneratorClass() : TclClass("RpcGenerator") {}
  TclObject* create(int argc, const char*const* argv) {
    assert(argc == 4);
    return new RpcGenerator();
  }
} class_rpc_generator;

int RpcGenerator::command(int argc, const char*const* argv) {
  return TclObject::command(argc, argv);
}
