

#ifndef sensor_query_h_
#define sensor_query_h_

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
#include <address.h>
#include "tags.h"

class SensorQueryHandler;


class SensorQueryAgent : public Agent {
public:
  SensorQueryAgent();
  virtual int command(int argc, const char * const * argv);

  // Tracing stuff
  Trace *tracetarget_;
  void trace(char* fmt,...); 
  
  // Address of node the agent is attached to
  nsaddr_t myaddr_;

  void startUp();
  void stop();

  double query_interval_;
  void generate_query(int p1, int p2, int p3);

  // Used to schedule query generation
  Event *gen_query_event_;
  SensorQueryHandler *query_handler_;
  //  void handle(Event *);

  void recv(Packet *p, Handler *);

  // Pointer to global tag database
  tags_database *tag_dbase_;

  // set to 1 if node is dead, defaults to 0 
  int node_dead_;
};




class SensorQueryHandler : public Handler {
public:
  SensorQueryHandler(SensorQueryAgent *a) { a_ = a; }
  virtual void handle(Event *e) { a_->generate_query(-1,-1,-1); }
private:
  SensorQueryAgent *a_;
};


#endif

