#ifndef LINK_AWARE_QUEUE_HH
#define LINK_AWARE_QUEUE_HH

#include "queue.h"
#include "cell-link.h"
#include <vector>

class LinkAwareQueue : public Queue {
  protected :
    CellLink* _link; /* Link in front of the queue */

  public :
    /* constructor */
    virtual std::vector<double> get_link_rates() = 0;

};

#endif
