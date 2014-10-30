// Author: Satish Kumar, kkumar@isi.edu

#ifndef landmark_h_
#define landmark_h_

#include <agent.h>
#include <ip.h>
#include <delay.h>
#include <scheduler.h>
#include <queue.h>
#include <trace.h>
#include <arp.h>
#include <ll.h>
#include <mac.h>
#include <priqueue.h>
#include <mobilenode.h>
#include "tags.h"
#include "agent-list.h"

typedef double Time;

#define ROUTER_PORT 255
#define QUERY_PORT 0

#define NOT_CHILD 0
#define IS_CHILD 1
#define NOT_POTL_CHILD 2 // Not within appropriate vicinity

#define NO_PARENT 60000

#define OLD_ENTRY 0     // Object already exists in list
#define NEW_ENTRY 1     // New object created in list
#define OLD_MESSAGE 2   // Old information; Object not added to list
#define ENTRY_NOT_FOUND 3 // Object not found
#define NEW_CHILD 4     // New child added
#define OLD_CHILD_TAGS_CHANGED 5

#define TRUE 1
#define FALSE 0

#define MAX_ENERGY 100000
#define MAX_LEVELS 8
#define MAX_CHILDREN 30
#define MAX_DEMOTION_RECORDS 20

#define INITIAL_WAIT_TIME 10.0
#define PERIODIC_WAIT_TIME 60.0
#define LM_STARTUP_JITTER 10.0 // secs to jitter LM's periodic adverts
#define IP_HDR_SIZE 20 // 20 byte IP header as in the Internet
#define WAIT_TIME 2 // 1s for each hop; round-trip is 2
#define MAX_TIMEOUT 200 // Value divided by local population density and 
            // level to compute promotion timeout at node
#define CONST_TIMEOUT 10 // Constant portion of timeout across levels

#define LOW_FREQ_UPDATE 300
#define PROMO_TIMEOUT_MULTIPLES 1 // Used in promotion timer

#define DEMOTION 0    // update pkt should advertise demotion
#define PERIODIC_ADVERTS 1   // periodic update for each level node is at
#define UNICAST_ADVERT_CHILD 2
#define UNICAST_ADVERT_PARENT 3
#define GLOBAL_ADVERT 4 // global advertisement from root LM
#define QUERY_PKT  5        // query/response pkt
#define DIR_QUERY_PKT 6
#define DIR_RESPONSE_PKT 7
#define OBJECT_QUERY_PKT 8
#define OBJECT_RESPONSE_PKT 9
#define HASH_PKT 10
#define HASH_ACK_PKT 11
#define REHASH_PKT 12

#define FLOOD 0
#define UNICAST 1
#define SUPPRESS 2

#define DEMOTION_RATIO 1.3
#define DEMOTION_DIFF  5000

#define NO_NEXT_HOP 50000
#define MAX_CACHE_ITEMS 200

#define NO_GLOBAL_LM 60000


class TagMobilityHandler;
class TagAdvtHandler;

//class TagCache {
//public:
//  int obj_name_;
//  int origin_time_;
//  double X_;
//  double Y_;
//};


// Used in aggregation
class aggreg_taglist {
public:
  aggreg_taglist() { marked_ = 0; next_ = NULL;}
  int obj_name_;
  int marked_;
  aggreg_taglist *next_;
};





class LMAddrs {
public:
  LMAddrs() {
    lmaddr_ = 0;
    num_lm_addrs_ = 0;
  }

  ~LMAddrs() {  
    delete_lm_addrs();
  }

  void add_lm_addr(int64_t lmaddr) {
    int i = 0;
    assert(num_lm_addrs_ >= 0);

    if(num_lm_addrs_) {
       int64_t *tmp_addrs = new int64_t[num_lm_addrs_];
       for(i = 0; i < num_lm_addrs_; ++i) {
          tmp_addrs[i] = lmaddr_[i];
       }
       delete[] lmaddr_;

       lmaddr_ = new int64_t[num_lm_addrs_+1];
       for(i = 0; i < num_lm_addrs_; ++i) {
         lmaddr_[i] = tmp_addrs[i];
       }
       delete[] tmp_addrs;
    }
    else 
      lmaddr_ = new int64_t[num_lm_addrs_+1];

    lmaddr_[num_lm_addrs_] = lmaddr;
    ++num_lm_addrs_;
  }

  void delete_lm_addrs() {
    if(!num_lm_addrs_) return;
    num_lm_addrs_ = 0;
    delete[] lmaddr_;
    lmaddr_ = NULL;
  }

  void get_lm_addrs(int *num_lm_addrs, int64_t **lmaddr) {
    *num_lm_addrs = num_lm_addrs_;
    *lmaddr = NULL;
    if(num_lm_addrs_) {
      *lmaddr = new int64_t[num_lm_addrs_];
      for(int i = 0; i < num_lm_addrs_; ++i) {
          (*lmaddr)[i] = lmaddr_[i];
      }
    }
  }

  int match_lm_addr(int *addr, int level) {
    int i[8];
    int num_addrs = 0;
    int match = FALSE;

    assert(level < 8);

    for(num_addrs = 0; num_addrs < num_lm_addrs_; ++num_addrs) {
       i[7] = (lmaddr_[num_addrs] >> 56) & 0xFF;
       i[6] = (lmaddr_[num_addrs] >> 48) & 0xFF;
       i[5] = (lmaddr_[num_addrs] >> 40) & 0xFF;
       i[4] = (lmaddr_[num_addrs] >> 32) & 0xFF;
       i[3] = (lmaddr_[num_addrs] >> 24) & 0xFF;
       i[2] = (lmaddr_[num_addrs] >> 16) & 0xFF;
       i[1] = (lmaddr_[num_addrs] >> 8) & 0xFF;
       i[0] = (lmaddr_[num_addrs]) & 0xFF;

       match = TRUE;
       for(int j = 7; j >= level; --j) {
          if(addr[j] != i[j]) {
            match = FALSE;
            break;
          }
       }
       if(match == TRUE)
          return(match);
    }
    return(match);
  }

private:
 int64_t *lmaddr_;
 int num_lm_addrs_;
};

// We need a separate record to keep track of old demotion messages so that
// the same msg is not forwarded more than once. We cant use the potl children
// or potl parent list for this because the entry is deleted on demotion.
class RecentMsgRecord {
public:
  RecentMsgRecord() {
    id_ = 60000;
    level_ = -1;
    origin_time_ = 0;
  }
  nsaddr_t id_;
  int level_;
  int origin_time_;
};



class NodeIDList {
public:
  NodeIDList() {
    next_ = NULL;
  }
  nsaddr_t dst_node_;
  nsaddr_t dst_next_hop_;
  int next_hop_level_;
  NodeIDList *next_;
};


class LMNode {
  friend class ParentChildrenList;
  friend class LandmarkAgent;
  friend class PromotionTimer;
public:
  LMNode(nsaddr_t id, nsaddr_t next_hop, int num_hops, int level, int num_children, int energy, int origin_time, double update_time) {
    id_ = id;
    lmaddr_ = new LMAddrs;
    next_hop_ = next_hop;
    child_flag_ = NOT_CHILD;
    num_hops_ = num_hops;
    level_ = level;
    num_children_ = num_children;
    energy_ = energy;
    last_upd_origin_time_ = origin_time;
    last_upd_seqnum_ = -1;
    last_update_rcvd_ = update_time;
    tag_list_ = NULL;
    next_ = NULL;
  }
  
  ~LMNode() {
    compr_taglist *tag_ptr1, *tag_ptr2;
    tag_ptr1 = tag_list_;
    while(tag_ptr1) {
      tag_ptr2 = tag_ptr1;
      tag_ptr1 = tag_ptr1->next_;
      delete tag_ptr2;
    }
    delete lmaddr_;
   }

  nsaddr_t id_;             // ID of this node
  LMAddrs *lmaddr_;       // Landmark addresses of this node		       
  nsaddr_t next_hop_;	    // Next hop to reach this node
  int child_flag_;	    // indicates whether this node is a child
  int num_hops_;            // number of hops away
  int level_;               // level of the child node
  int num_children_;        // Number of children that this node has
  int energy_;              // Energy reserves of the child node
  int last_upd_origin_time_; // Origin time of last update
  int last_upd_seqnum_;      // Seqnum of last upd; used to distinguish
                             // different messages originated at same time
  double last_update_rcvd_; // Time at which last update received
  compr_taglist *tag_list_;  // Tag advertisements of this node
  LMNode *next_;     

  void copy_tag_list(compr_taglist *taglist);
};



class LMEvent : public Event {
public:
  int level_;
  LMEvent(int level) : Event() {level_ = level;}
};


class ParentChildrenList {
  friend class LandmarkAgent;
  friend class LMNode;
  friend class PromotionTimer;
  friend class LMPeriodicAdvtHandler;
public:
  ParentChildrenList(int level, LandmarkAgent *a);
  ~ParentChildrenList() {
    LMNode *node1, *node2;

    node1 = pchildren_;
    while(node1) {
      node2 = node1;
      node1 = node1->next_;
      delete node2;
    }

    node1 = pparent_;
    while(node1) {
      node2 = node1;
      node1 = node1->next_;
      delete node2;
    }

    // Event id > 0 for valid event
    if(periodic_update_event_->uid_) {
       Scheduler &s = Scheduler::instance();
       s.cancel(periodic_update_event_);
    }
    delete mylmaddrs_;
  }

//  void set_lmaddress(int64_t lmaddr) {
//    mylmaddr_ = lmaddr;
//  }
//  int64_t get_lmaddress() {
//    return(mylmaddr_);
//  }

  int level_;          // my level
  LMNode *parent_;    // points to the appropriate object in the pparent_ list
  int num_heard_;     // number of nodes heard at one level lower than 
                      // our level; superset of potential children heard
                      // with the "election" vicinity condition
  int num_children_;   // number of children
  int num_potl_children_; // number of potential children
  int num_pparent_;     // number of potential parents			       
  LMNode *pchildren_;  // list of children and potential children
  LMNode *pparent_;   // list of potential parents			    

  // Periodic advertisement stuff				       
  int seqnum_;          // Sequence number of last advertisement	   
  double last_update_sent_; // Time at which last update was sent 
  double update_period_;    // period between updates		 
  double update_timeout_;   // Updates are deleted after this timeout	 
  Event *periodic_update_event_;  // event used to schedule periodic 
                                    // landmark updates
  LMPeriodicAdvtHandler *periodic_handler_; // handler called by the scheduler

  ParentChildrenList *next_;  // pointer to next list element
  int update_round_;        // To be used for demotion

  // Update/add/delete info abt a potential parent or child
  // Returns 1 if parent or child already present else adds the relevant
  // object and returns 0
  // Deletes the specified parent or child if delete flag is set to 1;
  // should be set to 0 otherwise
  // One method might be enough but have two in case different
  // actions have to be taken for adding parent and child in the future
  int UpdatePotlParent(nsaddr_t id, nsaddr_t next_hop, int num_hops, int level,int num_children, int energy, int origin_time, int delete_flag);
  int UpdatePotlChild(nsaddr_t id, nsaddr_t next_hop, int num_hops, int level, int num_children, int energy, int origin_time, int child_flag, int delete_flag, compr_taglist *taglist);
  void UpdateChildLMAddr(nsaddr_t id, int num_lm_addrs, int64_t *lm_addrs);

  LandmarkAgent *a_; // Agent associated with this object
  compr_taglist *tag_list_; // Aggregated list of tags for each level	     
  int num_tags_;            // Number of tags in tag_list	       

  int adverts_type_;  // Indicates whether adverts should be flooded, unicast
                  // or suppresed when no changes as in a hard-state scheme
  LMAddrs *mylmaddrs_;  // My landmark addresses; 8 bits per level; assume
                            // max of 8 levels; 0 at any level indicates 
                            // unassigned, addrs start from 1 onwards.
};


#define HIER_ADVS 0
#define OBJECT_ADVS 1
#define HIER_AND_OBJECT_ADVS 2

class LandmarkAgent : public Agent {
  friend class LMPeriodicAdvtHandler;
  friend class PromotionTimer;
  friend class ParentChildrenList;
public:
  LandmarkAgent();
  virtual int command(int argc, const char * const * argv);

  //  RoutingTable *table_;     // Routing Table

  // Promotion timer stuff
  PromotionTimer *promo_timer_; // Promotion timer object
  double promo_start_time_;     // Time when the promotion timer was started
  double promo_timeout_;	// Promotion timeout. Same for all levels.
  double promo_timeout_decr_; // Amount by which promotion timer is
                               // decr when another LM's adverts is heard
  int promo_timer_running_;	// indicates that promotion timer is running

  void startUp();           // Starts off the hierarchy construction protocol
  virtual void stop();             // Resets the agent state
  int seqno_;               // Sequence number to advertise with...
  int myaddr_;              // My address...

  // Periodic advertisements stuff				

  virtual void periodic_callback(Event *e, int level); // method to send periodic advts
  
  int highest_level_;       // My highest level in the hierarchy (note
                            // that a LM can be at multiple levels)
  // List of parent and children nodes for each level I'm at. Methods to add
  // and remove parent, child information from this list.
  ParentChildrenList *parent_children_list_;
  void Addparent(const nsaddr_t parent, int level);

  void Addpotentialchild(const nsaddr_t child, int level);

  // pkt_type is one of HIER_ADVS, OBJECT_ADVS, HIER_AND_OBJECT_ADVS
  // action is one of DEMOTION, PERIODIC_ADVERTS, UNICAST_ADVERT_CHILD,
  // UNICAST_ADVERT_PARENT, GLOBAL_ADVERT (from root LM) and QUERY_PKT
  virtual Packet *makeUpdate(ParentChildrenList *pcl, int pkt_type, int action); 
                                  
  int radius(int level); // returns the LM radius for the specified level

  PriQueue *ll_queue;       // link level output queue

  void recv(Packet *p, Handler *);
  virtual void ProcessHierUpdate(Packet *p);
  virtual void ForwardPacket(Packet *p);

  // Prints neighbour information for this node
  void get_nbrinfo();

  // Store a record of recent forwarded demotion msgs
  RecentMsgRecord *recent_demotion_msgs_;
  int num_demotion_msgs_;
  int CheckDemotionMsg(nsaddr_t id, int level, int origin_time);

  // Tracing stuff
  void trace(char* fmt,...);       
  Trace *tracetarget_;  // Trace target

  // Assign landmark addresses to children
  void assign_lmaddress(int64_t *lmaddr, int num_lm_addrs, int root_level);
			     
  // Pointer to global tag database
  tags_database *tag_dbase_;
  compr_taglist *aggregate_taginfo(compr_taglist *unagg_tags, int agg_level, int *num_tags);
  compr_taglist *aggregate_tags(compr_taglist *unagg_tags, int agg_level, int *num_tags);
  NodeIDList *search_tag(int obj_name, int prev_hop_level, int next_hop_level, nsaddr_t last_hop_id, int *num_dst);
  virtual nsaddr_t get_next_hop(nsaddr_t dst, int next_hop_level);

  // Mobile node to which agent is attached; Used to get position information
  MobileNode *node_;

  // Randomness/MAC/logging parameters
  int be_random_;    // set to 1 on initialization

  int num_resched_; // used in rescheduling timers
  int wait_state_;  // used to indicate that the node is waiting to receive
                    // other LM hierarchy messages
  double total_wait_time_; // total time the node has spent in wait state

  // Debug flags
  int debug_;	  
  int qry_debug_;

  // Tag cache info
  int cache_;      // set to 1 to enable caching
  TagCache *tag_cache_;
  int num_cached_items_;

  // Update period info
  double update_period_;
  double update_timeout_;

  // Option to indicate whether updates are to be flooded, unicast or
  // suppressed when no changes occur as in a hard-state based scheme
  int adverts_type_;  

  // Option to indicate whether there is a global LM. We dont need a global
  // LM for all the query direction schemes
  int global_lm_; 

  // ID and level of global LM that this node sees
  nsaddr_t global_lm_id_;
  int global_lm_level_;

  // Indicates whether the node that the agent is attached to is alive or not
  int node_dead_;

  // Random number generator
  RNG *rn_;
  inline double jitter(double max, int be_random_);
  inline double random_timer(double max, int be_random_);

  virtual void GenerateReHashMsg(int64_t lm_addr, double net_change_time) { }

  int num_nbrs_;
  int *nbrs_;

  // Tag mobility stuff
  TagMobilityHandler *tag_mobility_;
  Event *tag_mobility_event_;
  double mobility_period_;

  virtual void MoveTags();
  virtual void AddMobileTag(void *mobile_tag);
  // Pointer to tags that have moved within this sensor's range
  // while the sensor was dead
  compr_taglist *mobile_tags_;

  TagAdvtHandler *tag_advt_handler_;
  Event *tag_advt_event_;
  RNG *tag_rng_;

  void SendChangedTagListUpdate(int our_tag_changed, int level);
  int compare_tag_lists(compr_taglist *tag_list1, int num_tags1, compr_taglist *tag_list2, int num_tags2);
};



class LMPeriodicAdvtHandler : public Handler {
public:
  LMPeriodicAdvtHandler(ParentChildrenList *p) { p_ = p; }
  virtual void handle(Event *e) { (p_->a_)->periodic_callback(e,p_->level_); }
private:
  ParentChildrenList *p_;
};


class PromotionTimer : public TimerHandler {
public:
  PromotionTimer(LandmarkAgent *a) : TimerHandler() { a_ = a;}
protected:
  virtual void expire(Event *e);
  LandmarkAgent *a_;
};


class TagMobilityHandler : public Handler {
public:
  TagMobilityHandler(LandmarkAgent *a) { a_ = a; }
  virtual void handle(Event *) { a_->MoveTags(); }
private:
  LandmarkAgent *a_;
};




class TagAdvtHandler : public Handler {
public:
  TagAdvtHandler(LandmarkAgent *a) { a_ = a; our_tags_changed_ = 1; }
  virtual void handle(Event *) { a_->SendChangedTagListUpdate(our_tags_changed_,0); }
private:
  LandmarkAgent *a_;
  int our_tags_changed_;
};


#endif
