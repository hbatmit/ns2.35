// Author: Satish Kumar, kkumar@isi.edu

#ifndef tag_h_
#define tag_h_

#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <iomanip.h>
#include <assert.h>
#include <tclcl.h>
#include <trace.h>
#include <rng.h>

#define NUM_RECTANGLES 10 // Divide into 10 rectangles at each level
#define TRUE 1
#define FALSE 0


// Structure used for caching at agents
class TagCache {
public:
  int obj_name_;
  int origin_time_;
  double X_;
  double Y_;
};



// Object to hold attribute value pairs
class attribs {
  char *name;
  char *value;
  attribs *next_;
};


// Tag object definition
class tag {
public:
  tag() { next_ = NULL;}
  double x_;           // x-coordinate
  double y_;           // y-coordinate
  int obj_name_;  // Hierarchical object name
  attribs *attributes_; // <attribute, value> pairs
  tag *next_;		// Used to attach tags to a list
};
  

// List of tags in 2-dimensional space (for now)
class dbase_node {
public:
  dbase_node (double x_min, double x_max, double y_min, double y_max) {
    assert ((x_min <= x_max) && (y_min <= y_max));
    x_min_ = x_min;
    x_max_ = x_max;
    y_min_ = y_min;
    y_max_ = y_max;
    tags_list_ = NULL;

    for(int i = 0; i < NUM_RECTANGLES; ++i) {
      list_node_[i] = NULL;
    }
    
  };
  
  double x_min_;
  double x_max_;
  double y_min_;
  double y_max_;

  // Divide into NUM_RECTANGLES rectangles at each level
  dbase_node *list_node_[NUM_RECTANGLES]; 

  tag *tags_list_;				
};


// A compressed list returned to the node. Excluded attributes for now.
class compr_taglist {
public:
  compr_taglist() { next_ = NULL;}
  int obj_name_;
  compr_taglist *next_;
};



class tags_database : public TclObject {
public:
  tags_database() : tags_db_(NULL) { 
     num_tags_ = 0;
     num_sensed_tags_ = 0;
     sensed_tag_list_ = NULL;
     num_freq_qry_tags_= 0;   
     freq_qry_tag_list_ = NULL;
     rn_ = new RNG;
  }

  ~tags_database() {
    // Need to add deletion of tree as well
    delete[] sensed_tag_list_;
  }

  virtual int command(int argc, const char * const * argv);

  void create_tags_database(double x_min, double x_max, double y_min, double y_max, int num_tags);
  void Addtag(const tag *tag_);
  void Deletetag(const tag *tag_);
  compr_taglist *Gettags(double x, double y, double r); // Returns all tags 
                             // within a circle centered at (x,y) and radius r
  Trace *tracetarget_;       // Trace target
  void trace(char *fmt,...);				  
  int get_random_tag();

protected:
  dbase_node *tags_db_;  // interior node
  int num_tags_;         // total number of tags in database
  int num_sensed_tags_;  // number of tags sensed by nodes
  int *sensed_tag_list_; // list of all tags sensed by nodes

  int num_freq_qry_tags_;   // number of frequently queried tags	      
  int *freq_qry_tag_list_;  // tags that will be frequently queried

  RNG *rn_;
  compr_taglist *vtags_; //used to store returned tag list after search    
  void add_level(double x_min, double x_max, double y_min, double y_max, dbase_node *dbnode);				       
  void search_tags_dbase(double x, double y, double r, dbase_node *dbnode);
};



#endif



