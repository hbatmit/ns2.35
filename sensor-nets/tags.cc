// Author: Satish Kumar, kkumar@isi.edu



extern "C" {
#include <stdarg.h>
#include <float.h>
};


#include "tags.h"
#include "random.h"
#include <string.h>


#define MAX_P1 10
#define MAX_P2 10

// Split into 10 rectangles at each level. Have two levels for now.


static class TagDbaseClass:public TclClass
{
  public:
  TagDbaseClass ():TclClass ("TagDbase")
  {
  }
  TclObject *create (int, const char *const *)
  {
    return (new tags_database ());
  }
} class_tags_database;




void tags_database::
trace (char *fmt,...)
{
  va_list ap; // Define a variable ap that will refer to each argument in turn

  if (!tracetarget_)
    return;

  // Initializes ap to first argument
  va_start (ap, fmt); 
  // Prints the elements in turn
  vsprintf (tracetarget_->buffer (), fmt, ap); 
  tracetarget_->dump ();
  // Does the necessary clean-up before returning
  va_end (ap); 
}




int 
tags_database::command (int argc, const char *const *argv)
{
  if (argc == 7)
    {
      if (strcmp (argv[1], "create_database") == 0)
        {
	  double x_min = atof(argv[2]);
	  double x_max = atof(argv[3]);
	  double y_min = atof(argv[4]);
	  double y_max = atof(argv[5]);
	  int num_tags = atoi(argv[6]);
	  
	  num_tags_ = num_tags;
	  sensed_tag_list_ = new int[num_tags];
	  freq_qry_tag_list_ = new int[num_tags];
	  create_tags_database(x_min,x_max,y_min,y_max,num_tags);
          return (TCL_OK);
        }
    }
  else if (argc == 3)
    {
      if (strcasecmp (argv[1], "tracetarget") == 0)
        {
          TclObject *obj;
	  if ((obj = TclObject::lookup (argv[2])) == 0)
            {
              fprintf (stderr, "%s: %s lookup of %s failed\n", __FILE__, argv[1],
                       argv[2]);
              return TCL_ERROR;
            }
          tracetarget_ = (Trace *) obj;
          return TCL_OK;
        }
    }

  return (TclObject::command(argc, argv));
}




void
tags_database::create_tags_database(double x_min, double x_max, double y_min, double y_max, int num_tags)
{
  dbase_node *dbnode;
  int i;

  assert((x_min <= x_max) && (y_min <= y_max));  

  // Creating the data structures for storing the tags information
  tags_db_ = new dbase_node(x_min, x_max, y_min, y_max);

  // Creates child nodes for tags_db_ and partitions x and y ranges
  add_level(x_min, x_max, y_min, y_max, tags_db_);
  
  // Creates child nodes for each of tags_db_'s children
  for(i = 0; i < NUM_RECTANGLES; ++i) {
    dbnode = tags_db_->list_node_[i];
    assert((dbnode->x_min_ <= dbnode->x_max_) && (dbnode->y_min_ <= dbnode->y_max_));
    add_level(dbnode->x_min_, dbnode->x_max_, dbnode->y_min_, dbnode->y_max_, dbnode);
  }

  // Creating the tags and adding them to the database
  tag *newtag = new tag();
  i = 0;
  int p1 = 1, p2 = 1, p3 = 1;
  int max_p1 = MAX_P1, max_p2 = MAX_P2;
  int max_p3 = num_tags/(MAX_P1 * MAX_P2);

  while(i < num_tags) {
    newtag->x_ = x_min + rn_->uniform(x_max-x_min);
    newtag->y_ = y_min + rn_->uniform(y_max-y_min);
    newtag->obj_name_ = (int)((p1 * pow(2,24)) + (p2 * pow(2,16)) + p3) ;
    ++p3;

    if(p3 == max_p3) {
      p3 = 1;
      ++p2;
      if(p2 == max_p2) {
	p2 = 1;
	++p1;
	if(p1 == max_p1) {
	  break;
	}
      }
    }

    Addtag(newtag);

    //    printf("Added object %d.%d.%d at (%f,%f)\n",(newtag->obj_name_ >> 24) & 0xFF,(newtag->obj_name_ >> 16) & 0xFF,(newtag->obj_name_) & 0xFFFF,newtag->x_,newtag->y_);
    ++i;
  }
  delete newtag;
}



void
tags_database::add_level(double x_min, double x_max, double y_min, double y_max, dbase_node *dbnode)
{
  double x1, x2, y1, y2, x_partition_size;
  int i;

// Just partitioning along x-range for now
  x_partition_size = (x_max-x_min)/NUM_RECTANGLES;
  x2 = x_min;
  y1 = y_min;
  y2 = y_max;
  for(i = 0; i < NUM_RECTANGLES; ++i) {
    x1 = x2;                    // x_min for partition
    x2 = x1 + x_partition_size; // x_max for partition
    
    // The last partition should cover the remaining ranges
    if (i == (NUM_RECTANGLES - 1)) {
      x2 = x_max;
    }
    dbnode->list_node_[i] = new dbase_node(x1,x2,y1,y2);
  }
}



void
tags_database::Addtag(const tag* tag_)
{
  dbase_node *dbnode = tags_db_;
  int i, found = FALSE;
  tag *new_tag;

  while(dbnode->list_node_[0] != NULL) {
    for(i = 0; i < NUM_RECTANGLES; ++i) {
      if(((dbnode->list_node_[i])->x_min_ <= tag_->x_) && ((dbnode->list_node_[i])->x_max_ >= tag_->x_)) {
	found = TRUE;
	break;
      }
    }
    assert(found);
    dbnode = dbnode->list_node_[i];
  }
  
  if(dbnode->tags_list_ == NULL) {
    dbnode->tags_list_ = new tag;
    new_tag = dbnode->tags_list_;
  }
  else {
    new_tag = dbnode->tags_list_;
    while(new_tag->next_ != NULL) {
      new_tag = new_tag->next_;
    }
    new_tag->next_ = new tag;
    new_tag = new_tag->next_;
  }

  // tags do not have any attributes for now
  new_tag->x_ = tag_->x_;
  new_tag->y_ = tag_->y_;
  new_tag->obj_name_ = tag_->obj_name_;
}



void
tags_database::Deletetag(const tag *tag_)
{
  dbase_node *dbnode = tags_db_;
  int i, found = FALSE;
  tag *old_tag;

  while(dbnode->list_node_[0] != NULL) {
    for(i = 0; i < NUM_RECTANGLES; ++i) {
      if(((dbnode->list_node_[i])->x_min_ <= tag_->x_) && ((dbnode->list_node_[i])->x_max_ >= tag_->x_)) {
	found = TRUE;
	break;
      }
    }
    assert(found);
    dbnode = dbnode->list_node_[i];
  }
  
  assert(dbnode->tags_list_ != NULL);

  //old_tag will point to the tag entry that is to be deleted
  found = FALSE;
  old_tag = dbnode->tags_list_;
  tag *prev_tag = old_tag; // Should have previous tag in list
  while(old_tag != NULL) {
    if ((old_tag->x_== tag_->x_) && (old_tag->y_ == tag_->y_) && (old_tag->obj_name_ == tag_->obj_name_)) {
      found = TRUE;
      break;
    }
    prev_tag = old_tag;
    old_tag = old_tag->next_;
  }
  
  assert(found);
  prev_tag->next_ = old_tag->next_;
  delete old_tag;
}



compr_taglist *
tags_database::Gettags(double x, double y, double r)
{
  dbase_node *dbnode = tags_db_;
  compr_taglist *tptr;
  int i, found = FALSE;
  vtags_ = NULL;
  
  // This interior node should have child nodes
  //  assert(dbnode->list_node_[0] != NULL);
  search_tags_dbase(x,y,r,dbnode);

  tptr = vtags_;
  while(tptr) {
    found = FALSE;
    for(i = 0; i < num_sensed_tags_; ++i) {
      if(tptr->obj_name_ == sensed_tag_list_[i]) {
	found = TRUE;
	break;
      }
    }
    if(!found) {
      //      int r = Random::uniform(4);

      // 20 % of objects stored in frequently queried objects list; others
      // in sensed_tag_list_;
      //      if(r == 0) {
      //	freq_qry_tag_list_[num_freq_qry_tags_] = tptr->obj_name_;
      //	++num_freq_qry_tags_;   	
      //      }
      //      else {
	sensed_tag_list_[num_sensed_tags_] = tptr->obj_name_;
	++num_sensed_tags_;
	//      }
    }
    tptr = tptr->next_;
  }
  assert(num_sensed_tags_ <= num_tags_);
  return(vtags_);
}


int
tags_database::get_random_tag()
{

  // Objects in freq_qry_tags_ are queried 10 times as often as those in
  // sensed_tag_list_
  //  int r = Random::uniform(10);
  //  if(r == 0) {
    int i = rn_->uniform(num_sensed_tags_);
    return(sensed_tag_list_[i]);
  //  }
  //  else {
  //    int i = rn_->uniform(num_freq_qry_tags_);
  //    return(freq_qry_tag_list_[i]);
    //  }
}




void
tags_database::search_tags_dbase(double x, double y, double r, dbase_node *dbnode)
{
  int i, found = FALSE, removed_tag = 0;
  compr_taglist **apt_tags;
  tag *prev_tag, *next_tag;
  tag *dbase_tags;
  dbase_node *child_dbnode;

  // If this is a leaf interior node, lookup the taglist for appropriate tags
  if(dbnode->list_node_[0] == NULL) {

    apt_tags = &vtags_;
    while((*apt_tags) != NULL)
      apt_tags = &((*apt_tags)->next_);
    
    dbase_tags = dbnode->tags_list_;
    prev_tag = dbase_tags;
    while(dbase_tags) {
      removed_tag = 0;
      double xpos = (dbase_tags->x_ - x) * (dbase_tags->x_ - x);
      double ypos = (dbase_tags->y_ - y) * (dbase_tags->y_ - y);

      if((xpos + ypos) < (r*r)) {
	*apt_tags = new compr_taglist;
	(*apt_tags)->obj_name_ = dbase_tags->obj_name_;
	apt_tags = &((*apt_tags)->next_);

	// Delete tag from the list so that only one sensor observes 
	// a particular tag
	removed_tag = 1;
	if(prev_tag == dbase_tags) {
	  dbnode->tags_list_ = dbase_tags->next_;
	  prev_tag = dbase_tags->next_;
	}
	else
	  prev_tag->next_ = dbase_tags->next_;

	next_tag = dbase_tags->next_;
	delete dbase_tags;
      }
      if(!removed_tag) {
	prev_tag = dbase_tags;
	dbase_tags = dbase_tags->next_;
      }
      else {
	dbase_tags = next_tag;
      }
    }
    return;
  }
  
  for(i = 0; i < NUM_RECTANGLES; ++i) {
    found = FALSE;

    // Check if x-r is in the x-range of this node
    if(((dbnode->list_node_[i])->x_min_ <= (x - r)) && ((dbnode->list_node_[i])->x_max_ >= (x - r)))
      found = TRUE;

    // Check if x+r is in the x-range of this node
    if(((dbnode->list_node_[i])->x_min_ <= (x + r)) && ((dbnode->list_node_[i])->x_max_ >= (x + r)))
      found = TRUE;

    // Check if the range (x-r,x+r) covers the x-range of this node
    if(((dbnode->list_node_[i])->x_min_ >= (x - r)) && ((dbnode->list_node_[i])->x_max_ <= (x + r)))
      found = TRUE;
    if(found) {
      child_dbnode = dbnode->list_node_[i];
      search_tags_dbase(x,y,r,child_dbnode);
    }
  }
}




