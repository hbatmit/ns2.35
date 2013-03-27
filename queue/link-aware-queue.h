#ifndef LINK_AWARE_QUEUE_HH
#define LINK_AWARE_QUEUE_HH

#include <map>
#include <vector>
#include "queue.h"
#include "link/delay.h"
#include "link/ensemble-scheduler.h"

class LinkAwareQueue : public Queue {
  protected :
    const LinkDelay* _link; /* Link in front of the queue */
    const EnsembleScheduler* _scheduler; /* Cellular scheduler */

  public :
    LinkAwareQueue() : Queue() {};

    int command( int argc, const char*const* argv );

};

#endif
