#ifndef LINK_AWARE_QUEUE_HH
#define LINK_AWARE_QUEUE_HH

#include "queue.h"
#include "cell-link.h"
#include <map>

class LinkAwareQueue : public Queue {
  protected :
    CellLink* _link; /* Link in front of the queue */

  public :
    LinkAwareQueue() : Queue() {};

    virtual std::map<uint64_t,double> get_link_rates() = 0;

    int command( int argc, const char*const* argv );

};

#endif
