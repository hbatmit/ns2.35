#include "queue/link-aware-queue.h"

int LinkAwareQueue::command( int argc, const char*const* argv )
{
  if(argc==3) {
    if(!strcmp(argv[1],"attach-link")) {
      _link=(LinkDelay*) TclObject::lookup( argv[2] );
      return TCL_OK;
    }
    if(!strcmp(argv[1],"attach-sched")) {
      _scheduler=(PFScheduler*) TclObject::lookup( argv[2] );
      return TCL_OK;
    }
  }
  return Queue::command(argc, argv);
}
