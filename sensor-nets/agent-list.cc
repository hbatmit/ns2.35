// Author: Satish Kumar, kkumar@isi.edu

extern "C" {
#include <stdarg.h>
#include <float.h>
};

#include "agent-list.h"

AgentList* AgentList::instance_;

static class AgentListClass:public TclClass
{
  public:
  AgentListClass ():TclClass ("AgentList")
  {
  }
  TclObject *create (int, const char *const *)
  {
    return (new AgentList ());
  }
} class_agent_list;




int 
AgentList::command (int argc, const char *const *argv)
{
  if(argc == 3) {
    if (strcasecmp(argv[1], "num_agents") == 0) {
      assert(num_agents_ == 0);

      num_agents_ = atoi(argv[2]);
      
      agents_ = new void*[num_agents_];
      bzero((char*) agents_, sizeof(void*) * num_agents_);
      
      instance_ = this;
      
      return TCL_OK;
    }
  }
  return (TclObject::command(argc, argv));
}





void
AgentList::AddAgent(nsaddr_t node_addr, void *a) {
  assert(agents_);
  assert(node_addr < num_agents_);
  agents_[node_addr] = a;
}


