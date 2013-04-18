#ifndef ENSEMBLE_AWARE_QUEUE_HH
#define ENSEMBLE_AWARE_QUEUE_HH

#include <map>
#include <vector>
#include "queue.h"
#include "link/ensemble-scheduler.h"

class EnsembleAwareQueue : public Queue {
  protected :
    EnsembleScheduler* _scheduler; /* Cellular scheduler */

  public :
    EnsembleAwareQueue() : Queue() {};

    int command( int argc, const char*const* argv );

};

#endif
