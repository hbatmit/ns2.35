#ifndef LINK_AWARE_QUEUE_HH
#define LINK_AWARE_QUEUE_HH

#include "queue.h"
#include "link/delay.h"
#include <map>
#include <vector>

class CellLink;

class LinkAwareQueue : public Queue {
  protected :
    const LinkDelay* _link; /* Link in front of the queue */

  public :
    LinkAwareQueue() : Queue() {};

    int command( int argc, const char*const* argv );

};

#endif
