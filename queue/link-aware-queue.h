#ifndef LINK_AWARE_QUEUE_HH
#define LINK_AWARE_QUEUE_HH

#include "queue.h"
#include "link/cell-link.h"
#include <map>
#include <vector>

class CellLink;

class LinkAwareQueue : public Queue {
  protected :
    const CellLink* _link; /* Link in front of the queue */

  public :
    LinkAwareQueue() : Queue() {};

    int command( int argc, const char*const* argv );

    virtual std::vector<uint64_t> backlogged_flowids( void ) const = 0;

};

#endif
