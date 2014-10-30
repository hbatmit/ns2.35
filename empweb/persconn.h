#ifndef ns_conpers_h
#define ns_conpers_h

#include <stdio.h>
#include <limits.h>
#include <tcl.h>
#include <ranvar.h>
#include <tclcl.h>
#include "config.h"

#define IDLE 0
#define INUSE 1
#define NONPERSIST 0
#define PERSIST 1


// Abstract page pool, used for interface only
class PagePool : public TclObject {
public: 
	PagePool() : num_pages_(0), start_time_(INT_MAX), end_time_(INT_MIN) {}
	int num_pages() const { return num_pages_; }
protected:
	virtual int command(int argc, const char*const* argv);
	int num_pages_;
	double start_time_;
	double end_time_;
	int duration_;

	// Helper functions
	TclObject* lookup_obj(const char* name) {
		TclObject* obj = Tcl::instance().lookup(name);
		if (obj == NULL) 
			fprintf(stderr, "Bad object name %s\n", name);
		return obj;
	}
};

class PersConn : public TclObject {
public: 
//	PersConn() : status_(IDLE), pendingReqByte_(0), pendingReplyByte_(0), ctcp_(NULL), csnk_(NULL), client_(NULL), server_(NULL) {}
	PersConn() :  ctcp_(NULL), csnk_(NULL), client_(NULL), server_(NULL) {}
//	inline int getStatus() { return status_ ;}
//	inline void setStatus(int s) { status_ = s ;}
	inline void setDst(int d) { dst_ = d ;}
	inline void setSrc(int s) { src_ = s ;}
	inline int getDst() { return dst_ ;}
	inline int getSrc() { return src_ ;}
	inline TcpAgent* getCTcpAgent() { return ctcp_ ;}
	inline TcpSink* getCTcpSink() { return csnk_ ;}
	inline TcpAgent* getSTcpAgent() { return stcp_ ;}
	inline TcpSink* getSTcpSink() { return ssnk_ ;}
	inline Node* getCNode() { return client_ ;}
	inline Node* getSNode() { return server_ ;}
	inline void setCTcpAgent(TcpAgent* c) { ctcp_ = c;}
	inline void setCTcpSink(TcpSink* c) { csnk_ = c;}
	inline void setSTcpAgent(TcpAgent* s) { stcp_ = s;}
	inline void setSTcpSink(TcpSink* s) { ssnk_ = s;}
	inline void setCNode(Node* c) { client_ = c;}
	inline void setSNode(Node* s) { server_ = s;}

//        int pendingReqByte_ ;
//        int pendingReplyByte_ ;

protected:
//	int status_ ;
	int src_ ;
	int dst_ ;
	TcpAgent* ctcp_ ;
	TcpSink* csnk_ ;
	TcpAgent* stcp_ ;
	TcpSink* ssnk_ ;
	Node* client_ ;
	Node* server_ ;

};

#endif //ns_conpers_h
