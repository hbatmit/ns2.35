/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/* 
 * Copyright 2002, Statistics Research, Bell Labs, Lucent Technologies and
 * The University of North Carolina at Chapel Hill
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright 
 * notice, this list of conditions and the following disclaimer in the 
 * documentation and/or other materials provided with the distribution.
 *    3. The name of the author may not be used to endorse or promote 
 * products derived from this software without specific prior written 
 * permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Reference
 *     Stochastic Models for Generating Synthetic HTTP Source Traffic 
 *     J. Cao, W.S. Cleveland, Y. Gao, K. Jeffay, F.D. Smith, and M.C. Weigle 
 *     IEEE INFOCOM 2004.
 *
 * Documentation available at http://dirt.cs.unc.edu/packmime/
 * 
 * Contacts: Michele Weigle (mcweigle@cs.unc.edu),
 *           Kevin Jeffay (jeffay@cs.unc.edu)
 */

#ifndef ns_packmime_h
#define ns_packmime_h

#include "timer-handler.h"
#include "app.h"
#include "node.h"
#include "packmime_ranvar.h"
#include <string>
#include <stack>
#include <queue>
#include <map>

#define MAX_NODES 10 

class FullTcpAgent;
class PackMimeHTTP;
class PackMimeHTTPTimer;
class PackMimeHTTPServerApp;
class PackMimeHTTPClientApp;

/*::::::::::::::::::::::::: TIMER HANDLER classes :::::::::::::::::::::::::::*/


class PackMimeHTTPServerAppTimer : public TimerHandler {
 public:
	PackMimeHTTPServerAppTimer(PackMimeHTTPServerApp* t) : TimerHandler(), 
		t_(t) {}
	virtual void handle(Event*);
	virtual void expire(Event*);
 protected:
	PackMimeHTTPServerApp* t_;
};


class PackMimeHTTPClientAppTimer : public TimerHandler {
 public:
	PackMimeHTTPClientAppTimer(PackMimeHTTPClientApp* t) : TimerHandler(), 
		t_(t) {}
	virtual void handle(Event*);
	virtual void expire(Event*);
 protected:
	PackMimeHTTPClientApp* t_;
};

class PackMimeHTTPTimer : public TimerHandler {
public:
	PackMimeHTTPTimer(PackMimeHTTP* mgr) : TimerHandler(), mgr_(mgr) {}
	inline PackMimeHTTP* mgr() {return mgr_;}
protected:
	virtual void handle(Event* e);
	virtual void expire(Event* e);
	PackMimeHTTP* mgr_;	                    // pointer to PackMimeHTTP object
};

/*:::::::::::::::::::::::: PACKMIME APPLICATION Classes ::::::::::::::::::::*/

class PackMimeHTTPClientApp : public Application {
 public:
	PackMimeHTTPClientApp() : Application(), id_(0), running_(0), 
				  persistent_(false),
				  totalbytes_(0), reqsize_(0), rspsize_(0), 
				  reqs_(0), reqsize_array_(NULL), 
				  rspsize_array_(NULL), reqgap_array_(NULL), 
				  array_ind_ (0), time_of_req_(0.0),
				  timer_(this), server_(NULL), mgr_(NULL) {};
	~PackMimeHTTPClientApp();
	void timeout();
	void start();
	void stop();
	void recycle();

	inline void set_server(PackMimeHTTPServerApp* server) {server_ = server;}
	inline const char* get_agent_name() {return agent_->name();}
	inline PackMimeHTTPServerApp* get_server() {return server_;}
	inline void set_agent(Agent* tcp) {agent_ = tcp;}
	inline void set_mgr(PackMimeHTTP* mgr) {mgr_ = mgr;}
	inline void set_id (int id) {id_ = id;}
	inline int get_id () {return id_;}
 protected:
	void recv(int bytes);

	int id_;
	int running_;
	bool persistent_;                // persistent connection?
	int totalbytes_;
	int reqsize_;
	int rspsize_;
	int reqs_;                      // total requests in this connection
	int* reqsize_array_;            // array of request sizes
	int* rspsize_array_;            // array of response sizes
	double* reqgap_array_;          // array of request intervals
	int array_ind_;                 // index into the arrays
	double time_of_req_;
	PackMimeHTTPClientAppTimer timer_;
	PackMimeHTTPServerApp* server_;     // pointer to Server
	PackMimeHTTP* mgr_;                 // pointer to PackMimeHTTP object
};


class PackMimeHTTPServerApp : public Application {
 public:
	PackMimeHTTPServerApp() : Application(), id_(0), running_(0), 
				  reqsize_(0), rspsize_(0), reqs_(0),
				  lastreq_(false), totalbytes_(0), 
				  timer_(this), mgr_(NULL) {};
	~PackMimeHTTPServerApp();
	void timeout();
	void stop();
	inline const char* get_agent_name() {return agent_->name();}
	inline void start() {running_ = 1;}
	inline void set_agent(Agent* tcp) {agent_ = tcp;}
	inline void set_mgr(PackMimeHTTP* mgr) {mgr_ = mgr;}
	inline void set_id (int id) {id_ = id;}
	inline int get_id () {return id_;}
	inline void set_reqsize(int size) {reqsize_ = size;}
	inline void set_rspsize(int size) {rspsize_ = size;}
	inline void set_reqs(int reqs) {reqs_ = reqs;}
	inline void set_last_req() {lastreq_ = true;}
	void recycle();

 protected:
	void recv(int bytes);

	int id_;
	int running_;
	int reqsize_;
	int rspsize_;
	int reqs_;                    // total number of requests
	bool lastreq_;                // is this the last request?
	int totalbytes_;              // total bytes received so far

	PackMimeHTTPServerAppTimer timer_;
	PackMimeHTTP* mgr_;                 // pointer to PackMimeHTTP object
};



/*::::::::::::::::::::::::: class PACKMIME :::::::::::::::::::::::::::::::::*/

class PackMimeHTTP : public TclObject {
 public:
	PackMimeHTTP();
	~PackMimeHTTP();
	void recycle (PackMimeHTTPClientApp*);
	void recycle (PackMimeHTTPServerApp*);
	void setup_connection ();
	void incr_pairs();

	inline double now() {return Scheduler::instance().clock();}
	inline int get_active() {return active_connections_;}
	inline int get_total() {return total_connections_;}
	inline int running() {return running_;}
	inline int debug() {return debug_;}
	inline int get_ID() {return ID_;}
	inline int get_warmup() {return warmup_;}
	inline double get_rate() {return rate_;}
	inline bool using_http_1_1() {return http_1_1_;}
	inline bool use_pm_persist_rspsz() {return use_pm_persist_rspsz_;}
	inline bool use_pm_persist_reqsz() {return use_pm_persist_reqsz_;}

	/* HTTP 1.0 random variable fns */
	double connection_interval();
	int get_reqsize();
	int get_rspsize();
	double get_server_delay();

	/* HTTP 1.1 random variable fns */
	bool is_persistent();
	int get_num_pages();
	int get_num_objs(int pages);
	double get_reqgap (int page, int obj);
	int adjust_persist_rspsz();
	void reset_persist_rspsz();

	inline FILE* get_outfp() {return outfp_;}
	inline FILE* get_fileszfp() {return fileszfp_;}
	inline FILE* get_samplesfp() {return samplesfp_;}

 protected:
	virtual int command (int argc, const char*const* argv);
	void start();
	void stop();
	void cleanup();
	void recycle (FullTcpAgent*);

	FullTcpAgent* picktcp();
	PackMimeHTTPServerApp* pickServerApp();
	PackMimeHTTPClientApp* pickClientApp();	

	PackMimeHTTPTimer timer_;
	double connection_interval_;  // set in setup_connection()

	// variables used to maintain array of server and client nodes
	int next_client_ind_;    
	int next_server_ind_;
	int total_nodes_;
	int current_node_;

	// TCL configurable variables
	Node* server_[MAX_NODES];
	Node* client_[MAX_NODES];
	char tcptype_[20];         // {Reno, Tahoe, NewReno, SACK}
	FILE* outfp_;              // output file for completed pairs
	FILE* fileszfp_;           // output file for requested pairs (@ server)
	FILE* samplesfp_;          // output file for requested pairs (@ client)
	double rate_;              // connections per second
	int segsize_;              // FullTCP max segment size
	int segsperack_;           // = 2 for delayed ACKS
	double interval_;          // delayed ACK interval
	int ID_;                   // PackMimeHTTP cloud ID
	int run_;                  // exp run number (for RNG stream selection)
	int debug_;
	int goal_pairs_;           // req/rsp pairs to allow
	int cur_pairs_;            // number of current req/rsp pairs
	int warmup_;               // warmup interval (s)
	bool http_1_1_;            // use HTTP 1.1?  (default: no)
	bool use_pm_persist_rspsz_; // use PM response sizes for persistent conns (def: yes)
	bool use_pm_persist_reqsz_; // use PM request size rule for persistent conns (def: yes)

	int active_connections_;   // number of active connections
	int total_connections_;    // number of total connections
	int running_;              // start new connections?
	
	// statistics objects
	RandomVariable* flowarrive_rv_;
	RandomVariable* reqsize_rv_;
	RandomVariable* rspsize_rv_;
	PackMimeHTTPPersistRspSizeRandomVariable* persist_rspsize_rv_;
	RandomVariable* persistent_rv_;
	RandomVariable* num_pages_rv_;
	RandomVariable* single_obj_rv_;
	RandomVariable* objs_per_page_rv_;
	RandomVariable* time_btwn_pages_rv_;
	RandomVariable* time_btwn_objs_rv_;
	RandomVariable* server_delay_rv_;

	RNG* flowarrive_rng_;
	RNG* reqsize_rng_;
	RNG* rspsize_rng_;
	RNG* persist_rspsize_rng_;
	RNG* persistent_rng_;
	RNG* num_pages_rng_;
	RNG* single_obj_rng_;
	RNG* objs_per_page_rng_;
	RNG* time_btwn_pages_rng_;
	RNG* time_btwn_objs_rng_;
	RNG* server_delay_rng_;

	// helper methods
	TclObject* lookup_obj(const char* name) {
                TclObject* obj = Tcl::instance().lookup(name);
                if (obj == NULL) 
                        fprintf(stderr, "Bad object name %s\n", name);
                return obj;
        }

	inline int lookup_rv (RandomVariable*& rv, const char* name) {
		if (rv != NULL)
			Tcl::instance().evalf ("delete %s", rv->name());
		rv = (RandomVariable*) lookup_obj (name);
		return rv ? (TCL_OK) : (TCL_ERROR);
	}

	// Agent and App Pools	
	std::queue<FullTcpAgent*> tcpPool_;
	std::queue<PackMimeHTTPClientApp*> clientAppPool_;
	std::queue<PackMimeHTTPServerApp*> serverAppPool_;

	// string = tcpAgent's name
	map<string, PackMimeHTTPClientApp*> clientAppActive_;
	map<string, PackMimeHTTPServerApp*> serverAppActive_;
};

#endif

