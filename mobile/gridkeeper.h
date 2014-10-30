/*
 * A possible optimizer for wireless simulation
 *
 * No guarantee for making wireless simulation better
 * Use it according to your scenario
 *
 * Ported from Sun's mobility code
 */

#ifndef __gridkeeper_h__
#define __gridkeeper_h__

#include "mobilenode.h"

#define aligngrid(a,b) (((a)==(b))?((b)-1):((a)))


class GridHandler : public Handler {
 public:
  GridHandler();
  void handle(Event *);
};

class GridKeeper : public TclObject {

public:
  GridKeeper();
  ~GridKeeper();
  int command(int argc, const char*const* argv);
  int get_neighbors(MobileNode *mn, MobileNode **output);
  void new_moves(MobileNode *);
  void dump();
  static GridKeeper* instance() { return instance_;}
  int size_;                     /* how many nodes are kept */
protected:

  MobileNode ***grid_;

  int dim_x_;
  int dim_y_;                    /* dimension */
  GridHandler *gh_; 

private:

  static GridKeeper* instance_;

};

class MoveEvent : public Event {
public:
  MoveEvent() : enter_(0), leave_(0), grid_x_(-1), grid_y_(-1) {}
  MobileNode **enter_;	/* grid to enter */
  MobileNode **leave_;	/* grid to leave */
  int grid_x_;
  int grid_y_;
  MobileNode *token_;    /* what node ?*/
};



#endif //gridkeeper_h


