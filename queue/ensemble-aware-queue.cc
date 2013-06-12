#include "queue/ensemble-aware-queue.h"

EnsembleAwareQueue::EnsembleAwareQueue(EnsembleScheduler* scheduler) : _scheduler(scheduler) {}

int EnsembleAwareQueue::command( int argc, const char*const* argv )
{
  if(argc==3) {
    if(!strcmp(argv[1],"attach-sched")) {
      _scheduler=(EnsembleScheduler*) TclObject::lookup( argv[2] );
      return TCL_OK;
    }
  }
  return Queue::command(argc, argv);
}
