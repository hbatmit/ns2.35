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

#include <tclcl.h>
#include "lib/bsd-list.h"
#include "random.h"
#include "tcp-full.h"
#include "packmime_HTTP.h"

#define MbPS2BPS_FACTOR 125000

/*::::::::::::::::::::::::::::::: PACKMIME ::::::::::::::::::::::::::::::::::*/

static class PackMimeHTTPClass : public TclClass {
public:
	PackMimeHTTPClass() : TclClass("PackMimeHTTP") {}
	TclObject* create(int, const char*const*) {
		return (new PackMimeHTTP);
	}
} class_packmime;

PackMimeHTTP::PackMimeHTTP() :
	TclObject(), timer_(this), connection_interval_(0), 
	next_client_ind_(0), next_server_ind_(0), total_nodes_(0),
	current_node_(0), outfp_(NULL), fileszfp_(NULL), 
	samplesfp_(NULL), rate_(0), segsize_(0), segsperack_(0),
	interval_(0), ID_(-1), run_(0), debug_(0), 
	cur_pairs_(0), warmup_(0), http_1_1_(false), 
	use_pm_persist_rspsz_(true), use_pm_persist_reqsz_(true),
	active_connections_(0), total_connections_(-1), running_(0), 
	flowarrive_rv_(NULL), reqsize_rv_(NULL), rspsize_rv_(NULL), 
	persist_rspsize_rv_(NULL), persistent_rv_(NULL), num_pages_rv_(NULL),
	single_obj_rv_(NULL), objs_per_page_rv_(NULL), time_btwn_pages_rv_(NULL),
	time_btwn_objs_rv_(NULL), server_delay_rv_(NULL), 
	flowarrive_rng_(NULL), reqsize_rng_(NULL), rspsize_rng_(NULL), 
	persist_rspsize_rng_(NULL), persistent_rng_(NULL), num_pages_rng_(NULL), 
	single_obj_rng_(NULL), objs_per_page_rng_(NULL), time_btwn_pages_rng_(NULL),
	time_btwn_objs_rng_(NULL), server_delay_rng_(NULL)
{
	int i;

	strcpy (tcptype_, "Reno");

	for (i=0; i<MAX_NODES; i++) {
		server_[i] = NULL;
	}
	for (i=0; i<MAX_NODES; i++) {
		client_[i] = NULL;
	}
}

PackMimeHTTP::~PackMimeHTTP()
{
	Tcl& tcl = Tcl::instance();

	// output stats
	if (debug_ > 0) {
		fprintf (stderr, "total connections created: %d ", 
			 total_connections_+1);
		fprintf (stderr, "in pool: %d  active: %d\n", 
			 (int) serverAppPool_.size(), 
			 (int) serverAppActive_.size());
	}
	
	// delete timer
	timer_.force_cancel();

	// delete active clients in the pool
	map<string, PackMimeHTTPClientApp*>::iterator ca_iter;
	for (ca_iter = clientAppActive_.begin(); 
	     ca_iter != clientAppActive_.end(); ca_iter++) {
		ca_iter->second->stop();
		tcl.evalf ("delete %s", ca_iter->second->name());
		clientAppActive_.erase (ca_iter);
	}

	// delete active servers in the pool
	map<string, PackMimeHTTPServerApp*>::iterator sa_iter;
	for (sa_iter = serverAppActive_.begin(); 
	     sa_iter != serverAppActive_.end(); sa_iter++) {
		sa_iter->second->stop();
		tcl.evalf ("delete %s", sa_iter->second->name());
		serverAppActive_.erase (sa_iter);
	}

	// delete clients in pool
	PackMimeHTTPClientApp* cli;
	while (!clientAppPool_.empty()) {
		cli = clientAppPool_.front();
		cli->stop();
		tcl.evalf ("delete %s", cli->name());
		clientAppPool_.pop();
	}

	// delete servers in pool
	PackMimeHTTPServerApp* srv;
	while (!serverAppPool_.empty()) {
		srv = serverAppPool_.front();
		srv->stop();
		tcl.evalf ("delete %s", srv->name());
		serverAppPool_.pop();
	}

	// delete agents in the pool
	FullTcpAgent* tcp;
	while (!tcpPool_.empty()) {
		tcp = tcpPool_.front();
		tcl.evalf ("delete %s", tcp->name());
		tcpPool_.pop();
	}
	
	// delete RNGs and Random Variables
	cleanup();

	// close output files
	if (outfp_)
		fclose(outfp_);
 	if (fileszfp_)
 		fclose(fileszfp_);
 	if (samplesfp_)
 		fclose(samplesfp_);
}

FullTcpAgent* PackMimeHTTP::picktcp()
{
	FullTcpAgent* a;
	Tcl& tcl = Tcl::instance();

	if (tcpPool_.empty()) {
		tcl.evalf ("%s alloc-tcp %s", name(), tcptype_);
		a = (FullTcpAgent*) lookup_obj (tcl.result());
		if (a == NULL) {
			fprintf (stderr, "Failed to allocate a TCP agent\n");
			abort();
		}
		if (debug_ > 1) {
			fprintf (stderr, 
				 "\tflow %d created new TCPAgent %s\n",
				 total_connections_, a->name());
		}
	} else {
		a = tcpPool_.front(); 	// grab top of the queue
		tcpPool_.pop();         // remove top from queue

		if (debug_ > 1) {
			fprintf (stderr, "\tflow %d got TCPAgent %s", 
				 total_connections_, a->name());
			fprintf (stderr, " from pool (%d in pool)\n",
				 (int) tcpPool_.size());
		}
	}

	return a;
}

PackMimeHTTPServerApp* PackMimeHTTP::pickServerApp()
{
	PackMimeHTTPServerApp* a;

	if (serverAppPool_.empty()) {
		Tcl& tcl = Tcl::instance();
		tcl.evalf ("%s alloc-server-app", name());
		a = (PackMimeHTTPServerApp*) lookup_obj (tcl.result());
		if (a == NULL) {
			fprintf (stderr, 
				"Failed to allocate a PackMimeHTTP server app\n");
			abort();
		}
		if (debug_ > 1)
			fprintf (stderr, "\tflow %d created new ",
				 total_connections_);
	} else {
		a = serverAppPool_.front();   // grab top of the queue
		serverAppPool_.pop();         // remove top from queue

		if (debug_ > 1)
			fprintf (stderr, "\tflow %d got ", 
				 total_connections_);
	}

	// initialize server app
	a->set_id(total_connections_);
	a->set_mgr(this);

	if (debug_ > 1)
		fprintf (stderr, "ServerApp %s (%d in pool, %d active)\n",
			 a->name(), (int) serverAppPool_.size(), 
			 (int) serverAppActive_.size()+1);

	return a;
}

PackMimeHTTPClientApp* PackMimeHTTP::pickClientApp()
{
	PackMimeHTTPClientApp* a;

	if (clientAppPool_.empty()) {
		Tcl& tcl = Tcl::instance();
		tcl.evalf ("%s alloc-client-app", name());
		a = (PackMimeHTTPClientApp*) lookup_obj (tcl.result());
		if (a == NULL) {
			fprintf (stderr, 
				"Failed to allocate a PackMimeHTTP client app\n");
			abort();
		}
		if (debug_ > 1)
			fprintf (stderr, "\tflow %d created new ",
				 total_connections_);
	} else {
		a = clientAppPool_.front();   // grab top of the queue
		clientAppPool_.pop();         // remove top from queue

		if (debug_ > 1)
			fprintf (stderr, "\tflow %d got ", 
				 total_connections_);
	}

	// initialize client app
	a->set_id(total_connections_);
	a->set_mgr(this);

	if (debug_ > 1)
		fprintf (stderr, "ClientApp %s (%d in pool, %d active)\n",
			 a->name(), (int) clientAppPool_.size(), 
			 (int) clientAppActive_.size()+1);

	return a;
}

void PackMimeHTTP::recycle(FullTcpAgent* agent)
{
	/*
	 * Problem is that server apps are "done" before client apps,
	 * so server agents and apps get recycled and re-used before
	 * client apps & agents that were attached to them are
	 *
	 */
	if (agent == NULL) {
		fprintf (stderr, "recycle> agent is null\n");
		return;
	}

	// reinitialize FullTcp agent
	agent->reset();

	// add to the inactive agent pool
	tcpPool_.push (agent);

	if (debug_ > 2) {
		fprintf (stderr, "\tTCPAgent %s moved to pool ", 
			 agent->name());
		fprintf (stderr, "(%d in pool)\n", (int) tcpPool_.size());
	}
}

void PackMimeHTTP::recycle(PackMimeHTTPClientApp* app)
{
	if (app == NULL)
		return;

	// find the client app in the active pool
	map<string, PackMimeHTTPClientApp*>::iterator ca_iter = 
		clientAppActive_.find(app->get_agent_name());
	if (ca_iter == clientAppActive_.end()) 
		return;

	// remove the client app from the active pool
	clientAppActive_.erase(ca_iter);

	// insert the client app into the inactive pool
	clientAppPool_.push (app);

	if (debug_ > 2) {
		fprintf (stderr, "\tClientApp %s (%d) moved to pool ", 
			 app->name(), app->get_id());
		fprintf (stderr, "(%d in pool, %d active)\n", 
			 (int) clientAppPool_.size(), 
			 (int) clientAppActive_.size());
	}

	// recycle app
	app->recycle();
}

void PackMimeHTTP::recycle(PackMimeHTTPServerApp* app)
{
	if (app == NULL)
		return;

	// find the server app in the active pool
	map<string, PackMimeHTTPServerApp*>::iterator sa_iter = 
		serverAppActive_.find(app->get_agent_name());
	if (sa_iter == serverAppActive_.end()) 
		return;

	// remove the server app from the active pool
	serverAppActive_.erase(sa_iter);

	// insert the server app into the inactive pool
	serverAppPool_.push (app);

	if (debug_ > 2) {
		fprintf (stderr, "\tServerApp %s (%d) moved to pool ",
			 app->name(), app->get_id());
		fprintf (stderr, "(%d in pool, %d active)\n", 
			 (int) serverAppPool_.size(), 
			 (int) serverAppActive_.size());
	}

	// recycle app
	app->recycle();
}

double PackMimeHTTP::connection_interval() 
{
	return connection_interval_;
}

/* HTTP 1.0 functions */
int PackMimeHTTP::get_reqsize() 
{
	return (int) (reqsize_rv_->value());
}

int PackMimeHTTP::get_rspsize() 
{
	return (int) (rspsize_rv_->value());
}

double PackMimeHTTP::get_server_delay() 
{ 
	return server_delay_rv_->value();
}

/* HTTP 1.1 functions */
bool PackMimeHTTP::is_persistent()
{
	double val = persistent_rv_->value();
	if (val == 0) {
		return false;
	} else {
		return true;
	}
}

int PackMimeHTTP::get_num_pages()
{
	return (int) ceil(num_pages_rv_->value());
}

int PackMimeHTTP::get_num_objs (int pages)
{
	int p_singleobj = 0;
	int objs = 1;

	if (pages > 1) {
		// find probabilty there's only one obj in this page
		p_singleobj = (int) single_obj_rv_->value();
	}
	if (p_singleobj == 0) {
		objs = (int) ceil(objs_per_page_rv_->value());	
		if (objs == 1) {
			// should be at least 2 objs at this point
			objs++;
		}
	}
	return objs;
}

double PackMimeHTTP::get_reqgap (int page, int obj)
{
	double val;

	if (page == 0 && obj == 0) {
		// first request
		val = 0;
	}
	else if (page != 0 && obj == 0) {
		// main page (between-page requests)
		val = time_btwn_pages_rv_->value();
	}
	else {
		// embedded objects (within-page requests)
		val = time_btwn_objs_rv_->value();
	}
	return val;
}

int PackMimeHTTP::adjust_persist_rspsz()
{
	return (int) persist_rspsize_rv_->value();
}

void PackMimeHTTP::reset_persist_rspsz()
{
	persist_rspsize_rv_->reset_loc_scale();
}

void PackMimeHTTP::incr_pairs()
/*
 * Keep track of the number of req/rsp pairs 
 */
{
	cur_pairs_++;
}

void PackMimeHTTP::setup_connection()
/*
 * Setup a new connection, including creation of Agents and Apps
 */
{
	Tcl& tcl = Tcl::instance();

	// incr count of connections
	active_connections_++;
	total_connections_++;

	if (debug_ > 1) {
		fprintf (stderr, 
		 "\nPackMimeHTTP %s> new flow %d  total active: %d at %f\n",
		 name(), total_connections_, active_connections_, now());
	}

	// pick tcp agent for client and server
	FullTcpAgent* ctcp = picktcp();
	FullTcpAgent* stcp = picktcp();

	// rotate through nodes assigning connections
	current_node_++;
	if (current_node_ >= total_nodes_)
		current_node_ = 0;

	// attach agents to nodes (server_ client_)
	tcl.evalf ("%s attach %s", server_[current_node_]->name(), 
		   stcp->name());
	tcl.evalf ("%s attach %s", client_[current_node_]->name(), 
		   ctcp->name());

	// set TCP options
	tcl.evalf ("%s setup-tcp %s %d", name(), stcp->name(), 
		   total_connections_);
	tcl.evalf ("%s setup-tcp %s %d", name(), ctcp->name(),
		   total_connections_);

	// setup connection between client and server
	tcl.evalf ("set ns [Simulator instance]");
	tcl.evalf ("$ns connect %s %s", ctcp->name(), stcp->name());
	tcl.evalf ("%s listen", stcp->name());

	// create PackMimeHTTPApps
	PackMimeHTTPClientApp* client_app = pickClientApp();
	PackMimeHTTPServerApp* server_app = pickServerApp();

	// attach TCPs to PackMimeHTTPApps
	ctcp->attachApp ((Application*) client_app);
	stcp->attachApp ((Application*) server_app);
	client_app->set_server(server_app);
	client_app->set_agent(ctcp);
	server_app->set_agent(stcp);

	// put apps in active list
	clientAppActive_[ctcp->name()] = client_app;
	serverAppActive_[stcp->name()] = server_app;

	// start PackMimeHTTPApps
	client_app->start();
	server_app->start();

	// set time for next connection to start
	connection_interval_ = flowarrive_rv_->value();
}

void PackMimeHTTP::start()
{            
	int i;

	// make sure that we have the same number of server nodes
	// and client nodes
	if (next_client_ind_ != next_server_ind_) {
		fprintf (stderr, "Error: %d clients and %d servers", 
			 next_client_ind_, next_server_ind_);
		exit(-1);
	}
	total_nodes_ = next_client_ind_;

	running_ = 1;

	// initialize PackMimeHTTP random variables
	if (flowarrive_rv_ == NULL) {
		flowarrive_rng_ = (RNG*) new RNG();
		// select proper substream
		for (i=0; i<run_; i++) {
			flowarrive_rng_->reset_next_substream();
		}
		flowarrive_rv_ = (PackMimeHTTPFlowArriveRandomVariable*) new
			PackMimeHTTPFlowArriveRandomVariable (rate_,
							      flowarrive_rng_);
		if (debug_ > 1) {
			fprintf (stderr, "Created FlowArrive RNG and RV\n");
		}
	}
	if (reqsize_rv_ == NULL) {
		reqsize_rng_ = (RNG*) new RNG();
		// select proper substream
		for (i=0; i<run_; i++) {
			reqsize_rng_->reset_next_substream();
		}
		reqsize_rv_ = (PackMimeHTTPFileSizeRandomVariable*) new
			PackMimeHTTPFileSizeRandomVariable (rate_, 
							PACKMIME_REQ_SIZE,
							    reqsize_rng_);
		if (debug_ > 1) {
			fprintf (stderr, "Created ReqSize RNG and RV\n");
		}
	}
	if (rspsize_rv_ == NULL) {
		rspsize_rng_ = (RNG*) new RNG();
		// select proper substream
		for (i=0; i<run_; i++) {
			rspsize_rng_->reset_next_substream();
		}
		rspsize_rv_ = (PackMimeHTTPFileSizeRandomVariable*) new
			PackMimeHTTPFileSizeRandomVariable (rate_, 
							PACKMIME_RSP_SIZE,
							    rspsize_rng_);
		if (debug_ > 1) {
			fprintf (stderr, "Created RspSize RNG and RV\n");
		}
	}
	if (server_delay_rv_ == NULL) {
		server_delay_rng_ = (RNG*) new RNG();
		// select proper substream
		for (i=0; i<run_; i++) {
			server_delay_rng_->reset_next_substream();
		}
		server_delay_rv_ = (PackMimeHTTPServerDelayRandomVariable*) new 
			PackMimeHTTPServerDelayRandomVariable 
			(PackMimeHTTPServerDelayRandomVariable::SERVER_DELAY_SHAPE, 
			 PackMimeHTTPServerDelayRandomVariable::SERVER_DELAY_SCALE, 
			 server_delay_rng_);
		if (debug_ > 1) {
			fprintf (stderr, "Created ServerDelay RNG and RV\n");
		}
	}
	if (persist_rspsize_rv_ == NULL) {
		persist_rspsize_rng_ = (RNG*) new RNG();
		// select proper substream
		for (i=0; i<run_; i++) {
			persist_rspsize_rng_->reset_next_substream();
		}
		persist_rspsize_rv_ = (PackMimeHTTPPersistRspSizeRandomVariable*) new 
			PackMimeHTTPPersistRspSizeRandomVariable(persist_rspsize_rng_);
		if (debug_ > 1) {
			fprintf (stderr, "Created Persistent RspSz RNG and RV\n");
		}
	}
	if (persistent_rv_ == NULL) {
		persistent_rng_ = (RNG*) new RNG();
		// select proper substream
		for (i=0; i<run_; i++) {
			persistent_rng_->reset_next_substream();
		}
		persistent_rv_ = (PackMimeHTTPPersistentRandomVariable*) new 
			PackMimeHTTPPersistentRandomVariable
			(PackMimeHTTPPersistentRandomVariable::P_PERSISTENT,
			 persistent_rng_);
		if (debug_ > 1) {
			fprintf (stderr, "Created Persistent RNG and RV\n");
		}
	}
	if (num_pages_rv_ == NULL) {
		num_pages_rng_ = (RNG*) new RNG();
		// select proper substream
		for (i=0; i<run_; i++) {
			num_pages_rng_->reset_next_substream();
		}
		num_pages_rv_ = (PackMimeHTTPNumPagesRandomVariable*) new 
			PackMimeHTTPNumPagesRandomVariable
			(PackMimeHTTPNumPagesRandomVariable::P_1PAGE,
			 PackMimeHTTPNumPagesRandomVariable::SHAPE_NPAGE,
			 PackMimeHTTPNumPagesRandomVariable::SCALE_NPAGE,
			 num_pages_rng_);
		if (debug_ > 1) {
			fprintf (stderr, "Created Number of Pages RNG and RV\n");
		}
	}
	if (single_obj_rv_ == NULL) {
		single_obj_rng_ = (RNG*) new RNG();
		// select proper substream
		for (i=0; i<run_; i++) {
			single_obj_rng_->reset_next_substream();
		}
		single_obj_rv_ = (PackMimeHTTPSingleObjRandomVariable*) new 
			PackMimeHTTPSingleObjRandomVariable
			(PackMimeHTTPSingleObjRandomVariable::P_1TRANSFER,
			 single_obj_rng_);
		if (debug_ > 1) {
			fprintf (stderr, "Created Single Objects RNG and RV\n");
		}
	}
	if (objs_per_page_rv_ == NULL) {
		objs_per_page_rng_ = (RNG*) new RNG();
		// select proper substream
		for (i=0; i<run_; i++) {
			objs_per_page_rng_->reset_next_substream();
		}
		objs_per_page_rv_ = (PackMimeHTTPObjsPerPageRandomVariable*) new 
			PackMimeHTTPObjsPerPageRandomVariable
			(PackMimeHTTPObjsPerPageRandomVariable::SHAPE_NTRANSFER,
			 PackMimeHTTPObjsPerPageRandomVariable::SCALE_NTRANSFER,
			 objs_per_page_rng_);
		if (debug_ > 1) {
			fprintf (stderr, "Created Objects Per Page RNG and RV\n");
		}
	}
	if (time_btwn_pages_rv_ == NULL) {
		time_btwn_pages_rng_ = (RNG*) new RNG();
		// select proper substream
		for (i=0; i<run_; i++) {
			time_btwn_pages_rng_->reset_next_substream();
		}
		time_btwn_pages_rv_ = (PackMimeHTTPTimeBtwnPagesRandomVariable*) new 
			PackMimeHTTPTimeBtwnPagesRandomVariable(time_btwn_pages_rng_);
		if (debug_ > 1) {
			fprintf (stderr, "Created Time Btwn Pages RNG and RV\n");
		}
	}
	if (time_btwn_objs_rv_ == NULL) {
		time_btwn_objs_rng_ = (RNG*) new RNG();
		// select proper substream
		for (i=0; i<run_; i++) {
			time_btwn_objs_rng_->reset_next_substream();
		}
		time_btwn_objs_rv_ = (PackMimeHTTPTimeBtwnObjsRandomVariable*) new 
			PackMimeHTTPTimeBtwnObjsRandomVariable(time_btwn_objs_rng_);
		if (debug_ > 1) {
			fprintf (stderr, "Created Time Btwn Objs RNG and RV\n");
		}
	}

	// schedule first connection
	timer_.sched (0);
}

void PackMimeHTTP::stop()
{
	running_ = 0;
}

void PackMimeHTTP::cleanup()
{
	// delete all 'new'ed variables
	if (reqsize_rv_ != NULL) {
		delete reqsize_rv_;
	}
	if (rspsize_rv_ != NULL) {
		delete rspsize_rv_;
	}
	if (persist_rspsize_rv_ != NULL) {
		delete persist_rspsize_rv_;
	}
	if (flowarrive_rv_ != NULL) {
		delete flowarrive_rv_;
	}
	if (server_delay_rv_ != NULL) {
		delete server_delay_rv_;
	}
	if (persistent_rv_ != NULL) {
		delete persistent_rv_;
	}
	if (num_pages_rv_ != NULL) {
		delete num_pages_rv_;
	}
	if (single_obj_rv_ != NULL) {
		delete single_obj_rv_;
	}
	if (objs_per_page_rv_ != NULL) {
		delete objs_per_page_rv_;
	}
	if (time_btwn_pages_rv_ != NULL) {
		delete time_btwn_pages_rv_;
	}
	if (time_btwn_objs_rv_ != NULL) {
		delete time_btwn_objs_rv_;
	}
	if (flowarrive_rng_ != NULL) {
		delete flowarrive_rng_;
	}
	if (reqsize_rng_ != NULL) {
		delete reqsize_rng_;
	}
	if (rspsize_rng_ != NULL) {
		delete rspsize_rng_;
	}
	if (server_delay_rng_ != NULL) {
		delete server_delay_rng_;
	}
	if (persistent_rng_ != NULL) {
		delete persistent_rng_;
	}
	if (num_pages_rng_ != NULL) {
		delete num_pages_rng_;
	}
	if (single_obj_rng_ != NULL) {
		delete single_obj_rng_;
	}
	if (objs_per_page_rng_ != NULL) {
		delete objs_per_page_rng_;
	}
	if (time_btwn_pages_rng_ != NULL) {
		delete time_btwn_pages_rng_;
	}
	if (time_btwn_objs_rng_ != NULL) {
		delete time_btwn_objs_rng_;
	}
}

int PackMimeHTTP::command(int argc, const char*const* argv) {
	if (argc == 2) {
		if (!strcmp (argv[1], "start")) {
			start();
			return (TCL_OK);
		}
		else if (!strcmp (argv[1], "stop")) {
			stop();
			return (TCL_OK);
		}
		else if ((!strcmp (argv[1], "set-1.1")) ||
			 (!strcmp (argv[1], "set-http-1.1"))) {
			http_1_1_ = true;
			return (TCL_OK);
		}
		else if (!strcmp (argv[1], "active-connections")) {
			if (outfp_) {
				fprintf (outfp_, "%d ", active_connections_);
				fflush (outfp_);
			}
			else
				fprintf (stderr, "%d ", active_connections_);
			return (TCL_OK);
		}
		else if (!strcmp (argv[1], "total-connections")) {
			fprintf (stderr, "%d ", total_connections_);
			return (TCL_OK);
		}
		else if (strcmp (argv[1], "get-pairs") == 0) {
			Tcl &tcl = Tcl::instance();
			tcl.resultf("%d", cur_pairs_);
			return (TCL_OK);
		}
		else if (!strcmp (argv[1], "no-pm-persistent-reqsz")) {
			use_pm_persist_reqsz_ = false;
			return (TCL_OK);
		}
		else if (!strcmp (argv[1], "no-pm-persistent-rspsz")) {
			use_pm_persist_rspsz_ = false;
			return (TCL_OK);
		}
	}
	else if (argc == 3) {
		if ((!strcmp (argv[1], "set-client")) 
		    || (!strcmp (argv[1], "client"))) {
			if (next_client_ind_ >= MAX_NODES) {
				return (TCL_ERROR);
			}
			client_[next_client_ind_] = (Node*) 
				lookup_obj(argv[2]);
			if (client_[next_client_ind_] == NULL) {
				return (TCL_ERROR);
			}
			next_client_ind_++;
			return (TCL_OK);			
		}
		else if ((!strcmp (argv[1], "set-server")) || 
			 (!strcmp (argv[1], "server"))) {
			if (next_server_ind_ >= MAX_NODES) {
				return (TCL_ERROR);
			}
			server_[next_server_ind_] = (Node*) 
				lookup_obj(argv[2]);
			if (server_[next_server_ind_] == NULL)
				return (TCL_ERROR);
			next_server_ind_++;
			return (TCL_OK);			
		}
		else if (!strcmp (argv[1], "set-TCP")) {
			strcpy (tcptype_, argv[2]);
			return (TCL_OK);
		}
		else if ((!strcmp (argv[1], "set-rate")) ||
			 (!strcmp (argv[1], "rate"))) {
			rate_ = (double) atof (argv[2]);
			return (TCL_OK);
		}
		else if (!strcmp (argv[1], "set-outfile")) {
			outfp_ = fopen (argv[2], "w");
			if (outfp_)
				return (TCL_OK);
			else 
				return (TCL_ERROR);
		}
		else if (!strcmp (argv[1], "set-filesz-outfile")) {
 			fileszfp_ = fopen (argv[2], "w");
 			if (fileszfp_) 
 				return (TCL_OK);
 			else 
 				return (TCL_ERROR);
 		}
 		else if (!strcmp (argv[1], "set-samples-outfile")) {
 			samplesfp_ = fopen (argv[2], "w");
 			if (samplesfp_) 
 				return (TCL_OK);
 			else 
 				return (TCL_ERROR);
 		}
		else if (strcmp (argv[1], "set-req_size") == 0) {
			int res = lookup_rv (reqsize_rv_, argv[2]);
			if (res == TCL_ERROR) {
				fprintf (stderr, "Invalid req size ");
				fprintf (stderr, "random variable\n");
				cleanup();
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
		else if (strcmp (argv[1], "set-rsp_size") == 0) {
			int res = lookup_rv (rspsize_rv_, argv[2]);
			if (res == TCL_ERROR) {
				fprintf (stderr, "Invalid rsp size ");
				fprintf (stderr, "random variable\n");
				cleanup();
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
		else if (strcmp (argv[1], "set-flow_arrive") == 0) {
			int res = lookup_rv (flowarrive_rv_, argv[2]);
			if (res == TCL_ERROR) {
				fprintf (stderr,"Invalid flow arrive ");
				fprintf (stderr, "random variable\n");
				cleanup();
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
		else if (strcmp (argv[1], "set-server_delay") == 0) {
			int res = lookup_rv (server_delay_rv_, argv[2]);
			if (res == TCL_ERROR) {
				fprintf (stderr,"Invalid server delay ");
				fprintf (stderr, "random variable\n");
				cleanup();
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
		else if (!strcmp (argv[1], "set-prob_persistent")) {
			int res = lookup_rv (persistent_rv_, argv[2]);
			if (res == TCL_ERROR) {
				fprintf (stderr,"Invalid percent persistent ");
				fprintf (stderr, "random variable\n");
				cleanup();
				return (TCL_ERROR);
			}
			return (TCL_OK);	
		}
		else if (!strcmp (argv[1], "set-num_pages")) {
			int res = lookup_rv (num_pages_rv_, argv[2]);
			if (res == TCL_ERROR) {
				fprintf (stderr,"Invalid number of pages ");
				fprintf (stderr, "random variable\n");
				cleanup();
				return (TCL_ERROR);
			}
			return (TCL_OK);	
		}
		else if (!strcmp (argv[1], "set-prob_single_obj")) {
			int res = lookup_rv (single_obj_rv_, argv[2]);
			if (res == TCL_ERROR) {
				fprintf (stderr,"Invalid probability single obj ");
				fprintf (stderr, "random variable\n");
				cleanup();
				return (TCL_ERROR);
			}
			return (TCL_OK);	
		}
		else if (!strcmp (argv[1], "set-objs_per_page")) {
			int res = lookup_rv (objs_per_page_rv_, argv[2]);
			if (res == TCL_ERROR) {
				fprintf (stderr,"Invalid objects per page ");
				fprintf (stderr, "random variable\n");
				cleanup();
				return (TCL_ERROR);
			}
			return (TCL_OK);	
		}
		else if (!strcmp (argv[1], "set-time_btwn_pages")) {
			int res = lookup_rv (time_btwn_pages_rv_, argv[2]);
			if (res == TCL_ERROR) {
				fprintf (stderr,"Invalid time between pages ");
				fprintf (stderr, "random variable\n");
				cleanup();
				return (TCL_ERROR);
			}
			return (TCL_OK);	
		}
		else if (!strcmp (argv[1], "set-time_btwn_objs")) {
			int res = lookup_rv (time_btwn_objs_rv_, argv[2]);
			if (res == TCL_ERROR) {
				fprintf (stderr,"Invalid time between objs ");
				fprintf (stderr, "random variable\n");
				cleanup();
				return (TCL_ERROR);
			}
			return (TCL_OK);	
		}
		else if (strcmp (argv[1], "set-ID") == 0) {
			ID_ = (int) atoi (argv[2]);
			return (TCL_OK);
		}
		else if (strcmp (argv[1], "set-run") == 0) {
			run_ = (int) atoi (argv[2]);
			return (TCL_OK);
		}
		else if (strcmp (argv[1], "set-debug") == 0) {
			debug_ = (int) atoi (argv[2]);
			return (TCL_OK);
		}
		else if (strcmp (argv[1], "set-warmup") == 0) {
			warmup_ = (int) atoi (argv[2]);
			return (TCL_OK);
		}
		else if (strcmp (argv[1], "recycle") == 0) {
			FullTcpAgent* tcp = (FullTcpAgent*) 
				lookup_obj(argv[2]);
			
			/*
			 * Need to wait to recycle server until client
			 * is done.  Fortunately, client has
			 * a handle to the server.
			 */

			// find client app associated with this agent
			map<string, PackMimeHTTPClientApp*>::iterator ca_iter = 
				clientAppActive_.find(tcp->name());
			if (ca_iter == clientAppActive_.end()) {
				// this isn't a client app, but a server app
				return (TCL_OK);
			}

			PackMimeHTTPClientApp* ca = ca_iter->second;
			PackMimeHTTPServerApp* sa = ca->get_server();
			FullTcpAgent* stcp = (FullTcpAgent*) 
				lookup_obj(sa->get_agent_name());

			if (debug_ > 1)
			      fprintf (stderr, "client %s (%d)> DONE at %f\n",
					 ca->name(), ca->get_id(), now());

			// remove apps from active pools and put 
			// in inactive pools
			recycle ((FullTcpAgent*) tcp);
			recycle ((FullTcpAgent*) stcp);
			recycle (ca);
			recycle (sa);

			active_connections_--;   // one less active conn
			return (TCL_OK);
		}
	}
	return TclObject::command(argc, argv);
}


/*:::::::::::::::::::::::::::: PACKMIMETIMER ::::::::::::::::::::::::::::::::*/

void PackMimeHTTPTimer::expire(Event* = 0) 
{

}

void PackMimeHTTPTimer::handle(Event* e)
{
	if (!mgr_->running()) {
		if (mgr_->get_active() != 0) {
			TimerHandler::handle(e);
			sched (1);              // check back in 1 second...
		}
		return;
	}

	// setup new connection
	mgr_->setup_connection();

	TimerHandler::handle(e);

	// schedule time for next connection
	if (mgr_->debug() > 1) {
		fprintf (stderr, "\tnext connection scheduled in %f s...\n", 
			 mgr_->connection_interval());
	}
	sched (mgr_->connection_interval());
}


/*:::::::::::::::::::::::::: PACKMIMECLIENTAPP ::::::::::::::::::::::::::::::*/

void PackMimeHTTPClientAppTimer::expire(Event* = 0)
{
}

void PackMimeHTTPClientAppTimer::handle(Event* e)
{
	t_->timeout();
	TimerHandler::handle(e);
}

static class PackMimeHTTPClientAppClass : public TclClass {
public:
	PackMimeHTTPClientAppClass() : TclClass("Application/PackMimeHTTP/Client") {}
	TclObject* create(int, const char*const*) {
		return (new PackMimeHTTPClientApp);
	}
} class_app_packmime_client;

PackMimeHTTPClientApp::~PackMimeHTTPClientApp()
{
	Tcl& tcl = Tcl::instance();
	if (agent_ != NULL) {
		tcl.evalf ("delete %s", agent_->name());
	}
	if (reqsize_array_ != NULL) {
		delete []reqsize_array_;
	}
	if (rspsize_array_ != NULL) {
		delete []rspsize_array_;
	}
	if (reqgap_array_ != NULL) {
		delete []reqgap_array_;
	}
}

void PackMimeHTTPClientApp::start()
{
	running_ = 1;
	timer_.sched(0);   // send data now
}

void PackMimeHTTPClientApp::stop()
{
	running_ = 0;
}

void PackMimeHTTPClientApp::recycle()
{
	id_ = 0;
	rspsize_ = 0;
	totalbytes_ = 0;
	agent_ = NULL;
	server_ = NULL;
	reqs_ = 0;
	array_ind_ = 0;
	if (reqsize_array_ != NULL) {
		delete []reqsize_array_;
	}
	if (rspsize_array_ != NULL) {
		delete []rspsize_array_;
	}
	if (reqgap_array_ != NULL) {
		delete []reqgap_array_;
	}
	reqsize_array_ = NULL;
	rspsize_array_ = NULL;
	reqgap_array_ = NULL;
}

void PackMimeHTTPClientApp::timeout()
{
	/* Time to generate a new request */
	
	int i, j, pages, objs, ind;
	int* objs_per_page;
	int reqsz, rspsz;

	if (!running_) {
		return;
	}

	/*
	 * Client should get reqsize and rspsize so that for the same
	 * seed, there will be the same request/response pairs.
	 */

	if (mgr_->using_http_1_1() && reqs_ == 0) {
		// determine if this connection is persistent
		persistent_ = mgr_->is_persistent();
	}
	
	if (persistent_ && reqs_ == 0) {
		// need to sample the request gaps and file sizes
		
		// get number of pages in this connection
		pages = mgr_->get_num_pages();
		objs_per_page = new int[pages];
		for (i=0; i<pages; i++) {
			// get number of objects on this page
			objs = mgr_->get_num_objs(pages);
			objs_per_page[i] = objs;
			reqs_ += objs;
		}
		
		// allocate space for request gaps and file sizes
		if (reqgap_array_ == NULL) {
			reqgap_array_ = new double[reqs_];
		}
		if (reqsize_array_ == NULL) {
			reqsize_array_ = new int[reqs_];
		}
		if (rspsize_array_ == NULL) {
			rspsize_array_ = new int[reqs_];
		}
		
		// fill the arrays
		ind = 0;
		reqsz = mgr_->get_reqsize();
		rspsz = mgr_->get_rspsize();
		for (i=0; i<pages; i++) {
			for (j=0; j<objs_per_page[i]; j++) {
				// sample inter-request time
				reqgap_array_[ind] = mgr_->get_reqgap(i,j);
				// all requests are same size
				reqsize_array_[ind] = reqsz;
				if (!mgr_->use_pm_persist_reqsz() && ind > 0) {
					reqsize_array_[ind] = mgr_->get_reqsize();
				}
				// all responses start out as same size
				rspsize_array_[ind] = rspsz;
				// for non-PM rspsz, choose new response size
				if (!mgr_->use_pm_persist_rspsz() && ind > 0) {
					rspsize_array_[ind] = mgr_->get_rspsize();
				}
				ind++;
			}
		}
		
		if (mgr_->use_pm_persist_rspsz()) {
			// adjust response sizes
			if (reqs_ > 1 && 
			    rspsz > 
			    PackMimeHTTPPersistRspSizeRandomVariable::FSIZE_CACHE_CUTOFF) {
				for (i=1; i<reqs_; i++) {
					// leave rspsize_array_[0] alone
					rspsize_array_[i] = 
						mgr_->adjust_persist_rspsz();
				}
			}
			mgr_->reset_persist_rspsz();
		}

		array_ind_ = 0;
		delete [] objs_per_page;
	}

	if (persistent_) {
		// get the current request size
		reqsize_ = reqsize_array_[array_ind_];
		// get the current response size
		rspsize_ = rspsize_array_[array_ind_++];		
	}
	else {
		reqs_ = 1;

		// get request size
		reqsize_ = mgr_->get_reqsize();
		// get response size
		rspsize_ = mgr_->get_rspsize();
	}
	
	// check for 0-byte request or response
	if (reqsize_ == 0) 
		reqsize_ = 1;
	if (rspsize_ == 0) 
		rspsize_ = 1;

	server_->set_reqs(reqs_);
	if (reqs_ == 1 || array_ind_ == reqs_) {
		server_->set_last_req();
	}
	server_->set_reqsize(reqsize_);
	server_->set_rspsize(rspsize_);

	// save time of request
	time_of_req_ = mgr_->now();
	
	// dump request size and response size
 	FILE* fp = mgr_->get_samplesfp();
 	if (fp) {
 		char* nodeaddr = 
 			Address::instance().print_nodeaddr(agent_->daddr());
 		char* portaddr = 
 			Address::instance().print_nodeaddr(agent_->dport());
 		fprintf (fp, "%-11.6f %-10d %-10d %s.%-6s\n",
 			 time_of_req_, reqsize_, rspsize_, nodeaddr, portaddr);
 		fflush (fp);
 	}

	// send request
	agent_->sendmsg(reqsize_);
	if (mgr_->debug() > 1)
		fprintf (stderr, "client %s (%d)> sent %d-byte req (%d-byte rsp) at %f\n",
			 name(), id_, reqsize_, rspsize_, time_of_req_);
}

void PackMimeHTTPClientApp::recv(int bytes)
{
	// we've received a packet
	if (mgr_->debug() > 3)
		fprintf (stderr, "client %s (%d)> received %d bytes at %f\n", 
			 name(), id_, bytes, mgr_->now());

	totalbytes_ += bytes;

	if (totalbytes_ == rspsize_) {
		// we've received all packets from server
		if (mgr_->debug() > 1) {
			fprintf (stderr, "client %s (%d)> received ",
				 name(), id_);
			fprintf (stderr, "total of %d bytes at %f\n", 
				 rspsize_, mgr_->now());
		}
		
		if (reqs_ == 1 || array_ind_ == reqs_) {
			// either only 1 request or we've sent all requests
			stop();
		}
		else {
			// schedule next request time
			timer_.sched(reqgap_array_[array_ind_]);
		}

		if (time_of_req_ >= mgr_->get_warmup()) {
			// if we're dumping output, dump now
			double now = mgr_->now();
			FILE* fp = mgr_->get_outfp();
			char* dst_nodeaddr = 
			    Address::instance().print_nodeaddr(agent_->daddr());
			char* dst_portaddr = 
			    Address::instance().print_nodeaddr(agent_->dport());
			if (fp) {
				fprintf (fp, "%-11.6f %-10d %-10d %-10.3f %s.%-6s %-7d\n",
					 now, reqsize_, rspsize_, 
					 (now - time_of_req_) * 1000.0, 
					 dst_nodeaddr, dst_portaddr,
					 mgr_->get_active());
				fflush (fp);
			}
		}

		// increment number of pairs completed
		mgr_->incr_pairs();

		// reset totalbytes
		totalbytes_ = 0;
	}
}


/*:::::::::::::::::::::::::: PACKMIMESERVERAPP ::::::::::::::::::::::::::::::*/

void PackMimeHTTPServerAppTimer::expire(Event* = 0)
{
}

void PackMimeHTTPServerAppTimer::handle(Event* e)
{
	t_->timeout();
	TimerHandler::handle(e);
}

static class PackMimeHTTPServerAppClass : public TclClass {
public:
	PackMimeHTTPServerAppClass() : TclClass("Application/PackMimeHTTP/Server") {}
	TclObject* create(int, const char*const*) {
		return (new PackMimeHTTPServerApp);
	}
} class_app_packmime_server;

PackMimeHTTPServerApp::~PackMimeHTTPServerApp()
{
	Tcl& tcl = Tcl::instance();
	if (agent_ != NULL) {
		tcl.evalf ("delete %s", agent_->name());
	}
}

void PackMimeHTTPServerApp::timeout()
{
	if (!running_) {
		return;
	}

	if (mgr_->debug() > 1) {
		fprintf(stderr,"server %s (%d)> sent %d-byte response at %f\n",
			name(), id_, rspsize_, mgr_->now());
	}

	// dump all request size and response sizes
 	double now = mgr_->now();
 	FILE* fp = mgr_->get_fileszfp();
 	if (fp) {
 		char* nodeaddr = 
 			Address::instance().print_nodeaddr(agent_->addr());
 		char* portaddr = 
 			Address::instance().print_nodeaddr(agent_->port());
		fprintf (fp, "%-11.6f %-10d %-10d %s.%-6s\n",
 			 now, reqsize_, rspsize_, nodeaddr, portaddr);
 		fflush (fp);
 	}

	// send response
	if (reqs_ == 1 || lastreq_) {
		// this is the last message
		agent_->sendmsg(rspsize_, "MSG_EOF");
		stop();
	}
	else {
		agent_->sendmsg(rspsize_);
	}
}

void PackMimeHTTPServerApp::stop()
{
	running_ = 0;
}

void PackMimeHTTPServerApp::recycle()
{
	id_ = 0;
	reqsize_ = 0;
	rspsize_ = 0;
	totalbytes_ = 0;
	agent_ = NULL;
	reqs_ = 0;
	lastreq_ = false;
}


void PackMimeHTTPServerApp::recv(int bytes)
{
	double delay;

	if (!running_) {
		return;
	}

	totalbytes_ += bytes;

	// we've received a packet
	if (mgr_->debug() > 3)
		fprintf (stderr, "server %s (%d)> received %d bytes at %f\n", 
			 name(), id_, bytes, mgr_->now());

	if (totalbytes_ == reqsize_) {
		// generate waiting time
		delay = mgr_->get_server_delay();
		if (mgr_->debug() > 1) {
			fprintf (stderr, 
				 "server %s (%d)> received total of %d bytes from client\n",
				 name(), id_, reqsize_);
			fprintf (stderr, 
				 "server %s (%d)> waiting %f s before responding at %f\n", 
				 name(), id_, delay, mgr_->now());
		}
		
		// send data after waiting server delay seconds
		timer_.sched(delay);   

		// reset totalbytes_
		totalbytes_ = 0;
	}
}
