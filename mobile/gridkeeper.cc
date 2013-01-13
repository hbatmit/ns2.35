/*
 * An optimizer for some wireless simulations
 *
 * Helps most when some nodes are mostly stationary.
 * We hope you can share your experience with the gridkeeper with us.
 *
 * Ported from Sun's mobility code
 */

#include "gridkeeper.h"
#include <sys/param.h> /* For MIN/MAX */

static double d2(double x1, double x2, double y1, double y2)
{
  return ((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
}

GridHandler::GridHandler()
{
}

void GridHandler::handle(Event *e)
{
  MoveEvent *me = (MoveEvent *)e;
  MobileNode **pptr;
  MobileNode * token_ = me->token_;

  if ((pptr = me->leave_)) {
    while (*pptr) {
      if ((*pptr)->address() == token_->address()) break;
      pptr = &((*pptr)->next());
    }
    if (!(*pptr)) abort();
    else {
      *pptr = token_->next();
      token_->next() = 0;
    }
    
  }
  if ((pptr = me->enter_)) {
    if (token_->next()) abort();  // can't be in more than one grid
    token_->next() = *pptr;
    *pptr = token_;
  }
  delete me;

  // dump info in the gridkeeper for debug only
  // dump();

}

GridKeeper* GridKeeper::instance_; 

static class GridKeeperClass : public TclClass {
public:
  GridKeeperClass() : TclClass("GridKeeper") {}
  TclObject* create(int, const char*const*) {
      return (new GridKeeper);
  }
} class_grid_keeper;

GridKeeper::GridKeeper() : size_(0),grid_(NULL), dim_x_(0), dim_y_(0)
{
  gh_ = new GridHandler();
}

GridKeeper::~GridKeeper()
{
  int i;
  for (i = 0; i <= dim_x_; i++) {
    delete [] grid_[i];
  }
  delete [] grid_;
}

int GridKeeper::command(int argc, const char*const* argv)
{
  int i, grid_x, grid_y;
  Tcl& tcl = Tcl::instance();
  MobileNode *mn;

  if (argc == 2) {
    if (strcmp(argv[1], "dump") == 0) {
      dump();
      return (TCL_OK);
    }
  }

  if (argc == 3) {
    if (strcmp(argv[1], "addnode") == 0) {
	mn = (MobileNode *)TclObject::lookup(argv[2]);
	grid_x = aligngrid((int)mn->X(), dim_x_);
        grid_y = aligngrid((int)mn->Y(), dim_y_);
	mn->next() = grid_[grid_x][grid_y];
	grid_[grid_x][grid_y] = mn;
	size_++;
        return (TCL_OK);
    }
  }
  if (argc == 4 && strcmp(argv[1], "dimension") == 0) {
    if (instance_ == 0) instance_ = this;

    dim_x_ = strtol(argv[2], (char**)0, 0);
    dim_y_ = strtol(argv[3], (char**)0, 0);

    if (dim_x_ <= 0 || dim_y_ <= 0) {
      tcl.result("illegal grid dimension");
      return (TCL_ERROR);
    }

    grid_ = new MobileNode **[dim_x_];
    for (i = 0; i < dim_x_; i++) {
      grid_[i] = new MobileNode *[dim_y_];
      bzero((void *)grid_[i], sizeof(MobileNode *)*dim_y_);
    }
    return (TCL_OK);
  }
  return (TclObject::command(argc, argv));
}

void GridKeeper::new_moves(MobileNode *mn)
{
  double x = mn->X(), y = mn->Y(); 
  double ss = mn->speed();
  double vx = (mn->dX())*ss, vy = (mn->dY())*ss;

  double endx, endy, pother, tm;
  int i, j, endi, gother, grid_x, grid_y;
  MoveEvent *me;

  Scheduler& s = Scheduler::instance();

  endx = mn->destX();
  endy = mn->destY();

  if (vx > 0) {
    endi = MIN(dim_x_-1, (int)endx);
    for (i = (int)x+1; i <= endi; i++) {
      tm = (i-x)/vx;
      pother = vy*tm + y;
      j = (int)pother;
      me = new MoveEvent;
      if (j == pother && j != 0 && j != dim_y_) {
	if (vy > 0) gother = j - 1;
	else if (vy < 0) gother = j + 1;
	else gother = j;
      }
      else {
	gother = j;
      }
      me->leave_ = &(grid_[aligngrid(i-1, dim_x_)][aligngrid(gother, dim_y_)]);

      me->grid_x_ = grid_x = aligngrid(i, dim_x_);
      me->grid_y_ = grid_y = aligngrid(j, dim_y_);
      me->enter_ = &(grid_[grid_x][grid_y]);
      me->token_ = mn;
      s.schedule(gh_, me, tm);
    }
  }
  else if (vx < 0) {
    endi = (int)endx;
    for (i = (int)x; i > endi; i--) {
      if (i == dim_x_) continue;
      tm = (i-x)/vx;
      pother = vy*tm + y;
      j = (int)pother;
      me = new MoveEvent;
      if (j == pother && j != 0 && j != dim_y_) {
	if (vy > 0) gother = j - 1;
	else if (vy < 0) gother = j + 1;
	else gother = j;
      }
      else {
	gother = j;
      }
      me->leave_ = &grid_[aligngrid(i, dim_x_)][aligngrid(gother, dim_y_)];

      me->grid_x_ = grid_x = aligngrid(i-1, dim_x_);
      me->grid_y_ = grid_y = aligngrid(j, dim_y_);
      me->enter_ = &grid_[grid_x][grid_y];
      me->token_ = mn;
      s.schedule(gh_, me, tm);
    }
  }
  if (vy > 0) {
    endi = MIN(dim_y_-1, (int)endy);
    for (j = (int)y+1; j <= endi; j++) {
      tm = (j-y)/vy;
      pother = vx*tm + x;
      i = (int)pother;
      me = new MoveEvent;
      if (i == pother && i != 0 && i != dim_x_ && vx != 0) continue;
      me->leave_ = &grid_[aligngrid(i, dim_x_)][aligngrid(j-1, dim_y_)];

      me->grid_x_ = grid_x = aligngrid(i, dim_x_);
      me->grid_y_ = grid_y = aligngrid(j, dim_y_);
      me->enter_ = &grid_[grid_x][grid_y];
      me->token_ = mn;

      s.schedule(gh_, me, tm);
    }
  }
  else if (vy < 0) {
    endi = (int)endy;
    for (j = (int)y; j > endi; j--) {
      if (j == dim_y_) continue;
      tm = (j-y)/vy;
      pother = vx*tm + x;
      i = (int)pother;
      me = new MoveEvent;
      if (i == pother && i != 0 && i != dim_x_ && vx != 0) continue;
      me->leave_ = &grid_[aligngrid(i, dim_x_)][aligngrid(j, dim_y_)];

      me->grid_x_ = grid_x = aligngrid(i, dim_x_);
      me->grid_y_ = grid_y = aligngrid(j-1, dim_y_);
      me->enter_ = &grid_[grid_x][grid_y];
      me->token_ = mn;
      s.schedule(gh_, me, tm);
    }
  }

}

int GridKeeper::get_neighbors(MobileNode* mn, MobileNode **output)
{
  int grid_x, grid_y, index = 0, i, j, ulx, uly, lly, adj;
  MobileNode *pgd;
  double mnx, mny, mnr, sqmnr;
  
  mn->update_position();
  mnx = mn->X();
  mny = mn->Y();

  grid_x = aligngrid((int)mn->X(), dim_x_);
  grid_y = aligngrid((int)mn->Y(), dim_y_);

  mnr = mn->radius();
  sqmnr = mnr * mnr;

  adj = (int)ceil(mnr);

  ulx = MIN(dim_x_-1, grid_x + adj);
  uly = MIN(dim_y_-1, grid_y + adj);
  lly = MAX(0, grid_y - adj);

  for (i = MAX(0, grid_x - adj); i <= ulx; i++) {
    for (j = lly; j <= uly; j++) {
      for (pgd = grid_[i][j]; pgd != 0; pgd = pgd->next()) {
	if (mn->address() == pgd->address()) 
		continue;
	pgd->update_position();
	if (d2(pgd->X(), mnx, pgd->Y(), mny) < sqmnr)
	 output[index++] = pgd;
      }
    }
  }

  return index;
}

void GridKeeper::dump()
{
    int i,j;
    MobileNode *pgd;

    for (i = 0; i< dim_x_; i++) {
      for (j = 0; j < dim_y_; j++) {
	if (grid_[i][j] == 0) continue;
	printf("grid[%d][%d]: ",i,j);
	for (pgd = grid_[i][j]; pgd != 0; pgd = pgd->next()) {
	  printf("%d ",pgd->address());
	}
	printf("\n");
      }
    }
    printf("-------------------------------\n");

}


