// Author: Satish Kumar, kkumar@isi.edu

#ifndef flood_agent_h_
#define flood_agent_h_

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
#include "landmark.h"

typedef double Time;


class QueryList {
public:
  QueryList() {
    next_ = NULL;
  }
  nsaddr_t src_;
  int obj_name_;
  int origin_time_;
  int num_hops_;
  nsaddr_t last_hop_id_;
  QueryList *next_;
};




class FloodAgent : public Agent {
public:
  FloodAgent();
  virtual int command(int argc, const char * const * argv);
  
protected:
  //  RoutingTable *table_;     // Routing Table

  void startUp();           // Starts off the hierarchy construction protocol
  int seqno_;               // Sequence number to advertise with...
  int myaddr_;              // My address...

  // Periodic advertisements stuff				

  void periodic_callback(Event *e, int level); // method to send periodic advts
  
  PriQueue *ll_queue;       // link level output queue

  void recv(Packet *p, Handler *);

  // Tracing stuff
  void trace(char* fmt,...);       
  Trace *tracetarget_;  // Trace target

  // Pointer to global tag database
  tags_database *tag_dbase_;
  // Local tag list
  compr_taglist *tag_list_;

  // Method returns 1 if query seen before. Otherwise returns 0
  // and adds query info to the list
  QueryList *query_list_;
  int search_queries_list(nsaddr_t src, int obj_name, int origin_time, int num_hops, nsaddr_t last_hop_id);

  nsaddr_t get_next_hop(nsaddr_t src, int obj_name, int origin_time);

  // Mobile node to which agent is attached; Used to get position information
  MobileNode *node_;

  // Debug flag
  int debug_;	  

  // Tag cache info
  int cache_;      // set to 1 to enable caching
  TagCache *tag_cache_;
  int num_cached_items_;
};



#endif
