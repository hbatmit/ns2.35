#ifndef ns_pushback_message_h
#define ns_pushback_message_h

#define PUSHBACK_REQUEST_MSG     1 
#define PUSHBACK_REFRESH_MSG     2
#define PUSHBACK_STATUS_MSG      3
#define PUSHBACK_CANCEL_MSG      4

class AggSpec;

class PushbackMessage {
 public:
  int msgID_;
  int senderID_;
  int targetID_;

  //id details of the downstream PBA.
  int qid_;
  int rlsID_;

  void set(int msg, int sender, int dest, int qid, int rlsID) {
    msgID_ = msg;
    senderID_ = sender;
    targetID_ = dest;
    qid_ = qid;
    rlsID_=rlsID;
  }

  static char * type(PushbackMessage * msg) {
    switch (msg->msgID_) {
    case PUSHBACK_REQUEST_MSG: return "REQUEST";
    case PUSHBACK_REFRESH_MSG: return "REFRESH";
    case PUSHBACK_STATUS_MSG: return "STATUS";
    case PUSHBACK_CANCEL_MSG: return "CANCEL";
    default: return "UNKNOWN";
    }
  }
};


class PushbackRequestMessage : public PushbackMessage {
 public:
  AggSpec * aggSpec_;  
  double limit_;
  int depth_;          //depth of the sender. 

  PushbackRequestMessage(int sender, int dest, int qid, int rlsID, 
			 AggSpec * aggSpec, double limit, int depth) {
    set(PUSHBACK_REQUEST_MSG, sender, dest, qid, rlsID);
    aggSpec_ = aggSpec;
    limit_=limit;
    depth_ = depth;
  }
};

class PushbackStatusMessage : public PushbackMessage {
 public:
  double arrivalRate_;   //in bps as always.
  int height_;           //height of sender.
 
  PushbackStatusMessage(int sender, int dest, int qid, int rlsID, 
			double arrivalRate, int height) {
    set(PUSHBACK_STATUS_MSG, sender, dest, qid, rlsID);
    arrivalRate_ = arrivalRate;
    height_=height;
  }
};

class PushbackRefreshMessage : public PushbackMessage {
 public:
  AggSpec * aggSpec_;
  double limit_;
  
  PushbackRefreshMessage(int sender, int dest, int qid, int rlsID, 
			 AggSpec * aggSpec, double newLimit) {
    set(PUSHBACK_REFRESH_MSG, sender, dest, qid, rlsID);
    aggSpec_= aggSpec;
    limit_=newLimit;
  }
};

class PushbackCancelMessage : public PushbackMessage {
 public:
  
  PushbackCancelMessage(int sender, int dest, int qid, int rlsID) {
    set(PUSHBACK_CANCEL_MSG, sender, dest, qid, rlsID);
  }
};

#endif

