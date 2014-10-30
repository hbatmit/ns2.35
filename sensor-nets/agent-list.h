// Satish Kumar, kkumar@isi.edu

#ifndef agent_list_h_
#define agent_list_h_

#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <iomanip.h>
#include <assert.h>
#include <tclcl.h>
#include <trace.h>
#include <rng.h>
#include <agent.h>

class AgentList : public TclObject {
public:
  AgentList() {
    agents_ = NULL;
    num_agents_ = 0;
  }

  virtual int command(int argc, const char * const * argv);
  static AgentList* instance() {assert(instance_); return instance_; }
  void AddAgent(nsaddr_t node_addr, void *a);
  void* GetAgent(nsaddr_t node_addr) {
    assert(num_agents_ > node_addr);
    return(agents_[node_addr]);
  }
  
private:
  void **agents_;
  int num_agents_;
  static AgentList* instance_;
};


#endif
