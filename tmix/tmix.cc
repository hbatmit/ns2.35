/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/* 
 * Copyright 2007, Old Dominion University
 * Copyright 2007, University of North Carolina at Chapel Hill
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
 *
 * M.C. Weigle, P. Adurthi, F. Hernandez-Campos, K. Jeffay, and F.D. Smith, 
 * Tmix: A Tool for Generating Realistic Application Workloads in ns-2, 
 * ACM Computer Communication Review, July 2006, Vol 36, No 3, pp. 67-76.
 *
 * Contact: Michele Weigle (mweigle@cs.odu.edu)
 * http://www.cs.odu.edu/inets/Tmix
 * 
 * For more information on Tmix and to obtain Tmix tools and 
 * connection vectors, see http://netlab.cs.unc.edu/Tmix
 *
 * Additional work:
 *  DongXia Xu - scaling parameter
 *  David Hayes - prefilling parameters, maximum segment size CV record,
 *                simulation end time, fixes to one-way tcp recycling.
 */

#include <tclcl.h>
#include "lib/bsd-list.h"
#include "tcp-full.h"
#include "tmix.h"
#include "tmixAgent.h"

/*:::::::::::::::::::::::::: ADU class :::::::::::::::::::::::::::*/

void ADU::print()
{
	if (size_ != FIN) {
		fprintf (stderr,"\t%lu B (%g s after send / %g s after recv)\n",
			 size_, get_send_wait_sec(), get_recv_wait_sec());
	}
	else {
		fprintf (stderr, "\tFIN (%g s after send / %g s after recv)\n",
			 get_send_wait_sec(), get_recv_wait_sec());
	}
}

/*:::::::::::::::::::::::: CONNECTION VECTOR class :::::::::::::::::::::::::*/

ConnVector::~ConnVector()
{
	for (vector<ADU*>::iterator i = init_ADU_.begin();
	  i != init_ADU_.end(); ++i) {
		delete *i;
	}
	for (vector<ADU*>::iterator i = acc_ADU_.begin();
	  i != acc_ADU_.end(); ++i) {
		delete *i;
	}
	init_ADU_.clear();
	acc_ADU_.clear();
}

void ConnVector::add_ADU (ADU* adu, bool direction)
{
	if (direction == INITIATOR) {
		init_ADU_.push_back (adu);
	}
	else if (direction == ACCEPTOR) {
		acc_ADU_.push_back (adu);
	}
}

void ConnVector::print()
{
	int i;

	fprintf (stderr, "CV %ld> %f %s (%d:%d) %d init, %d acc\n", global_id_,
		 start_time_, (type_==SEQ)?"SEQ":"CONC", init_win_, acc_win_,
		 init_ADU_count_, acc_ADU_count_);
	
	fprintf (stderr, "  INIT:\n");
	for (i=0; i<(int) init_ADU_.size(); i++) {
		init_ADU_[i]->print();
	}

	fprintf (stderr, "  ACC:\n");
	for (i=0; i<(int) acc_ADU_.size(); i++) {
		acc_ADU_[i]->print();
	}
}

/*::::::::::::::::::::::::: TMIX class :::::::::::::::::::::::::::::::::*/

static class TmixClass : public TclClass {
public:
	TmixClass() : TclClass("Tmix") {}
	TclObject* create(int, const char*const*) {
		return (new Tmix);
	}
} class_Tmix;

Tmix::Tmix() :
	TclObject(), timer_(this), next_init_ind_(0), 
	next_acc_ind_(0), total_nodes_(0), current_node_(0), outfp_(NULL),
	cvfp_(NULL), ID_(-1), run_(0), debug_(0), pkt_size_(1460),
	step_size_(1000), warmup_(0), active_connections_(0), 
	total_connections_(0), total_apps_(0), running_(false), 
	agentType_(FULL), prefill_t_(0), prefill_a_(1), prefill_si_(0), 
	scale_(1), end_(0), fin_time_(1000000), check_oneway_closed_(false)
{
	connections_.clear();
	line[0] = '#';
	strcpy (tcptype_, "Reno");
	strcpy (sinktype_, "default");

	for (int i=0; i<MAX_NODES; i++) {
		acceptor_[i] = NULL;
	}
	for (int i=0; i<MAX_NODES; i++) {
		initiator_[i] = NULL;
	}
}

Tmix::~Tmix()
{
	Tcl& tcl = Tcl::instance();

	/* output stats */
	if (debug_ >= 1) {
		fprintf (stderr, "total connections: %lu / active: %lu  ", 
			 get_total(), get_active());
		fprintf (stderr, "(apps in pool: %d  active: %d)\n", 
			 (int) appPool_.size(), (int) appActive_.size());
	}
	
	/* cancel timer */
	timer_.force_cancel();

	/* delete active apps in the pool */
	map<string, TmixApp*>::iterator iter;
        if(!appActive_.empty()) {
		for (iter = appActive_.begin(); 
		     iter != appActive_.end(); iter++) {
			iter->second->stop();
			tcl.evalf ("delete %s", iter->second->name());
			appActive_.erase (iter);
		}
	}
	appActive_.clear();

	/* delete apps in pool */
	TmixApp* app;
	while (!appPool_.empty()) {
		app = appPool_.front();
		app->stop();
		tcl.evalf ("delete %s", app->name());
		appPool_.pop();
	}

	/* delete agents in the pool */
	TmixAgent* tcp;
	while (!tcpPool_.empty()) {
		tcp = tcpPool_.front();
		tcl.evalf ("delete %s", tcp->name());
 		tcpPool_.pop();
	}

	/* delete connections */
	for (list<ConnVector*>::iterator i = connections_.begin();
	     i != connections_.end(); i++) {
		delete *i;
	}
	connections_.clear();

	/* close output file */
	if (outfp_)
		fclose(outfp_);

	/* close input file, if not already closed */
        if (cvfp_)
		fclose (cvfp_);
}

TmixAgent* Tmix::picktcp()
{
	TmixAgent* a;

	if (! tcpPool_.empty()) {
		/* check to see if oldest agent has been in for 1 second */
		a = tcpPool_.front();
		if (a->inPoolFor1s(now())) {
			tcpPool_.pop();    /* remove top from queue */

			if (debug_ >= 6) {
				fprintf (stderr, "\tflow %lu got TCPAgent %s", 
					 total_connections_, a->name());
				fprintf (stderr, " from pool (%.3f s in pool, %d in pool)\n", a->getTimeInPool(now()), (int) tcpPool_.size());
			}
			return a;
		}
		else if (debug_ >= 6) {
			fprintf (stderr, "\t head of queue, only %.3f s in pool\n", a->getTimeInPool(now()));
		}
	}

	/* Need to create new Agent - 
	   Pool is either empty or no agent has been in > 1 second */
	a = agentFactory(this, tcptype_, sinktype_);
	if (a == NULL) {
		fprintf (stderr, "Failed to allocate a TCP agent\n");
		abort();
	}
	if (debug_ >= 6) {
		fprintf (stderr, 
			 "\tflow %lu created new TCPAgent %s (%d in pool)\n",
			 total_connections_, a->name(), (int) tcpPool_.size());
	}
	return a;
}

TmixApp* Tmix::pickApp()
{
	TmixApp* a;

	if (appPool_.empty()) {
		Tcl& tcl = Tcl::instance();
		tcl.evalf ("%s alloc-app", name());
		a = (TmixApp*) lookup_obj (tcl.result());
		if (a == NULL) {
			fprintf (stderr, 
				 "Failed to allocate a Tmix app\n");
			abort();
		}
		if (debug_ >= 6)
			fprintf (stderr, "\tflow %lu created new ",
				 total_connections_);
	} else {
		a = appPool_.front();   /* grab top of the queue */
		appPool_.pop();         /* remove top from queue */
		if (debug_ >= 6)
			fprintf (stderr, "\tflow %lu got ", 
				 total_connections_);
	}

	/* initialize app */
	total_apps_++;
	a->set_id(total_apps_);
	a->set_mgr(this);

	if (debug_ >= 6)
		fprintf (stderr, "App %s/%lu (%d in pool, %d active)\n",
			 a->name(), a->get_id(), (int) appPool_.size(), 
			 (int) appActive_.size()+1);

	return a;
}

int Tmix::crecycle(Agent* tcp) {
	/* Time to stop the TmixApp and recycle apps and agents */

	/* find app associated with this agent */
	map<string, TmixApp*>::iterator iter = 
		appActive_.find(tcp->name());
	if (iter == appActive_.end()) {
		return (TCL_ERROR);  /* can't find app */
	}
	TmixApp* app = iter->second;

	/* stop the app */
	if (!app->get_running()) {
		return (TCL_OK);  /* already been through crecycle() once */
	}
	app->stop();   /* indicate app is done */
	
	TmixApp* peer_app = app->get_peer();
	ConnVector* cv = app->get_cv();
	TmixAgent* agent = app->get_tmix_agent();
	TmixAgent* peer_agent = peer_app->get_tmix_agent();

	if (debug_ >= 6) {
	fprintf (stderr, 
			 "%f %d.%d -> %d.%d (cv: %lu) CRECYCLE active: %lu total: %lu\n", now(), agent->addr(), agent->port(), peer_agent->addr(), 
		 peer_agent->port(), cv->get_ID(), get_active(), get_total());
	}

	/* check the peer app */
	if (peer_app->get_running()) {
		/* peer is still running */
		if (debug_ >= 3) {
			fprintf (stderr, "App (%s)> stopped, but peer still running\n", app->id_str());
		}
		/* this side will get recycled when peer is done */
		return(TCL_OK); 
	}

	/* decrement active connections */
	decr_active();

	/* both this app and peer app are done */
	if (debug_ >= 2) {
		int agent_addr = agent->addr();
		int agent_port = agent->port();
		int peer_addr = peer_agent->addr();
		int peer_port = peer_agent->port();
		if (agentType_ == ONE_WAY) {
			peer_port+=1;
			fprintf (stderr, "%f %d.%d -> %d.%d  %d.%d -> %d.%d (cv: %lu) FINISHED active: %lu total: %lu\n", now(), peer_addr, agent_port, 
				 agent_addr, peer_port, agent_addr, 
				 agent_port, peer_addr, peer_port,
				 cv->get_ID(), get_active(), get_total());
		} else {
			fprintf (stderr, "%f %d.%d -> %d.%d (cv: %lu) FINISHED active: %lu total: %lu\n", now(), peer_addr, agent_port, agent_addr, 
				 peer_port, cv->get_ID(), get_active(), 
				 get_total());
		}
	}
			
	/* delete the ConnVector and ADUs associated with the apps */
	connections_.remove(cv);
	delete cv; /* ADUs are deleted in ~ConnVector */

	/* recycle everything, remove from active pools and put in 
	   inactive pools */
	recycle (agent);
	recycle (peer_agent);
	recycle (app);
	recycle (peer_app);

	return (TCL_OK);
}

void Tmix::recycle(TmixAgent* agent)
{
	if (agent == NULL) {
		fprintf (stderr, "Tmix::recycle> agent is null\n");
		return;
	}

	/* reinitialize agent */
	agent->reset();

	/* set the time agent entered the pool */
	agent->setPoolTime(now());

	/* add to the inactive agent pool */
	tcpPool_.push (agent);

	if (debug_ >= 6) {
		fprintf (stderr, "\tTCPAgent %s moved to pool ", 
			 agent->name());
		fprintf (stderr, "(%d in pool)\n", (int) tcpPool_.size());
	}
}

void Tmix::recycle(TmixApp* app)
{
	if (app == NULL)
		return;

	/* find the app in the active pool */
	map<string, TmixApp*>::iterator iter = 
		appActive_.find(app->get_agent_name());
	if (iter == appActive_.end()) 
		return;

	/* remove the app from the active pool */
	appActive_.erase(iter);

	/* insert the app into the inactive pool */
	appPool_.push (app);

	if (debug_ >= 6) {
		fprintf (stderr, "\tApp %s (%lu) moved to pool ", 
			 app->name(), app->get_id());
		fprintf (stderr, "(%d in pool, %d active)\n", 
			 (int) appPool_.size(), (int) appActive_.size());
	}

	/* recycle app */
	app->recycle();
}

void Tmix::incr_next_active()
{
	ConnVector* cv;
	int i = 0;

	list<ConnVector*>::iterator iter = next_active_;

	iter++;
	if (iter == connections_.end()) {
		while (!feof (cvfp_) && i < (int) step_size_) {
			/* all connections are started and there are still 
			 * connection vectors in the file, so read a set */
			cv = read_one_cvec();
			if (cv != NULL) {
				connections_.push_back(cv);
			}
			else {
				fprintf (stderr, "cv is null!\n");
			}
			i++;
		}

		if (debug_ >= 1) {
			fprintf (stderr, "Tmix %s> %d connections read\n", 
				 name(), (int) connections_.size());
		}

	}
	next_active_++;
}

void Tmix::setup_connection ()
/*
 * Setup a new connection, including creation of Agents and Apps
 */
{
	ConnVector* cv = get_current_cvec();

	/* pick tcp agent for initiator and acceptor */
	TmixAgent* init_tcp = picktcp();
	TmixAgent* acc_tcp = picktcp();

	/* increment total connections - must be done before 
	   configuring tcp sources (sets flowid) */
	incr_total();

	/* rotate through nodes assigning connections */
	current_node_++;
	if (current_node_ >= total_nodes_)
		current_node_ = 0;

	/* attach agents to nodes (acceptor_ init_) */
	init_tcp->attachToNode(initiator_[current_node_]);
	acc_tcp->attachToNode(acceptor_[current_node_]);

	if (debug_ >= 3) {
		if (agentType_ == FULL) {
			// link node.port to cvec global id
			fprintf(stderr,"%f CVEC-NODE-PORT: %lu %d.%d -> %d.%d\n", now(),  cv->get_ID(), initiator_[current_node_]->nodeid(), init_tcp->port(), 
				acceptor_[current_node_]->nodeid(), 
				acc_tcp->port());
		} else {
			// link node.port to cvec global id
			fprintf(stderr, "%f CVEC-NODE-PORT: %lu %d.%d -> %d.%d  %d.%d -> %d.%d\n", now(),  cv->get_ID(), initiator_[current_node_]->nodeid(), 
				init_tcp->port(), 
				acceptor_[current_node_]->nodeid(), 
				(dynamic_cast<TmixOneWayAgent*>
				 (acc_tcp))->getSink()->port(), 
				acceptor_[current_node_]->nodeid(),
				acc_tcp->port(),
				initiator_[current_node_]->nodeid(),
				(dynamic_cast<TmixOneWayAgent*>
				 (init_tcp))->getSink()->port());
		} 
	}


	/* set TCP options */
	init_tcp->configureTcp(this, cv->get_init_win(), cv->get_init_mss());
	acc_tcp->configureTcp(this, cv->get_acc_win(), cv->get_acc_mss());

	/* connect initiator and acceptor in this way since we may be
         * from an agent as source to an agent as sink instead of from 
	 * a fulltcp agent to another fulltcp agent */
	init_tcp->connect(acc_tcp);

	/* create TmixApps and put in active list */
	TmixApp* init_app = pickApp();
	appActive_[init_tcp->name()] = init_app;

	TmixApp* acc_app = pickApp();
	appActive_[acc_tcp->name()] = acc_app;

	/* attach TCPs to TmixApps */
	init_tcp->attachApp((Application*) init_app);
	init_app->set_agent(init_tcp->getAgent());
	acc_tcp->attachApp((Application*) acc_app);
	acc_app->set_agent(acc_tcp->getAgent());

	/*
	 * Combine this functionality with set_agent
	 */
	init_app->set_tmix_agent(init_tcp);
	acc_app->set_tmix_agent(acc_tcp);

	/* associate these peers with each other */
	init_app->set_peer(acc_app);
        acc_app->set_peer(init_app);

	init_app->set_cv(cv);
	acc_app->set_cv(cv);

	init_app->set_type(INITIATOR);
	acc_app->set_type(ACCEPTOR);

	init_app->set_mss(cv->get_init_mss());
	acc_app->set_mss(cv->get_acc_mss());
        
	/* incr count of connections */
	incr_active();

	if (debug_ >= 2) {
		if (agentType_ == FULL) {
		fprintf (stderr, 
			 "%f %d.%d -> %d.%d (cv: %lu) NEW CONN active: %lu total: %lu\n", now(), initiator_[current_node_]->nodeid(), init_tcp->port(), 
				 acceptor_[current_node_]->nodeid(), 
				 acc_tcp->port(), cv->get_ID(), get_active(), 
				 get_total());
		} else {
		fprintf (stderr, 
			 "%f %d.%d -> %d.%d (cv: %lu) NEW CONN active: %lu total: %lu\n", now(), initiator_[current_node_]->nodeid(), init_tcp->port(), 
				acceptor_[current_node_]->nodeid(), 
			 (dynamic_cast<TmixOneWayAgent*>
			  (acc_tcp))->getSink()->port(), cv->get_ID(), 
			 get_active(), get_total());
		}
	}
        
	/* start TmixApps */
	init_app->start();
	acc_app->start();
}

int fpeek(FILE *stream)
{
    int c;
    c = fgetc(stream);
    ungetc(c,stream);
    return c;
}

#include <iostream>
using namespace std;
ConnVector* Tmix::read_one_cvec() {
  static bool read = false;
  static int cv_file_type = 0;

  if (!read) {
    read = true;
    char local_line[CVEC_LINE_MAX];

    do {
      fgets(local_line, CVEC_LINE_MAX, cvfp_);
    } while (local_line[0] == '#');
    if (local_line[1] == 'E' || local_line[1] == 'O') {
      cv_file_type = CV_V1;
    }
    else {
      cv_file_type = CV_V2;
    }
    fclose(cvfp_);
    cvfp_ = fopen(cvfn_.c_str(), "r");
    if (!cvfp_) {
      cerr << "Error opening connection vector file" << endl;
    }
  }
  if (cv_file_type == CV_V1) {
	  return read_one_cvec_v1();
  }
  else if (cv_file_type == CV_V2) {
	  return read_one_cvec_v2();
  } 
  return 0;
}

#define A	0
#define B	1
#define TA	2
#define TB	3
ConnVector* Tmix::read_one_cvec_v1() {
	/*
	 * Need to remember the last time we got...
	 * if the last time we got was the same type
	 * as the current time we are reading
	 * then we set send_wait and not receive_wat
	 */
	ConnVector* cv=NULL;
	ADU* adu = NULL;

	int init_win, acc_win;
	int init_mss, acc_mss;
	unsigned long long int start_time;
	int init_ADU_count, acc_ADU_count;
	int global_id;

	int int_junk;     /* Stores temporary integer input */
	float float_junk; /* Stores temporary float input */
		
	int last_state = TB;   /* A, B, TA, or TB */

	unsigned long last_time_value = 0;
	unsigned long last_initiator_time_value = 0;
	unsigned long last_acceptor_time_value = 0;
	bool pending_initiator = false; /* Used for CONC cvecs */
	bool pending_acceptor = false;  /* Used for CONC cvecs */

	bool last_direction = INITIATOR; /* Used to add FIN if none in cvec */
		
	bool started_cvec=false;/* Set to true when the cvec read is started */
	bool parse_error=false; /* Set to true when a parse error occurs */	
	
	do {
		/* Skip a blank line, comment line, or empty line */
		if (line == NULL || line[0] == '#' || !strcmp(line,"\n")) {
			continue;
		}
		
		/* Break if a cvec has already been read */
		if (started_cvec && (line[0] == 'S' || line[0] == 'C')) {
			break;
		}
		started_cvec=true;
		
		/*SEQ connection vector construction*/		
		if(line[0] == 'S'){
			if(sscanf(line, "SEQ %llu %u %u %u",
				  &(start_time), &(init_ADU_count),&(int_junk), 
				  &(global_id)) != 4){
				fprintf(stderr, "Parse error on: %s\n", line);
				parse_error=true;
				break;
			}
			cv = new ConnVector(global_id, 
					    (double)start_time/1000000.0, SEQ, 
					    pkt_size_);
		}
		/*CONC connection vector construction*/
		else if(line[0] == 'C'){
			if(sscanf(line, "CONC %llu %u %u %u %u",
				  &(start_time), &(init_ADU_count), 
				  &(acc_ADU_count), &(int_junk),
				  &(global_id)) != 5){
				fprintf(stderr, "Parse error on: %s\n", line);
				parse_error=true;
				break;
			}
			cv = new ConnVector(global_id, 
					    (double)start_time/1000000.0, CONC,
					    init_ADU_count, acc_ADU_count, 
					    pkt_size_);
		}		  
		/*Maximum Segment Size*/
		else if (line[0] == 'm') {
			if(sscanf(line, "m %d %d", &(init_mss), 
				  &(acc_mss)) != 2){
				fprintf(stderr, "Parse error on: %s\n", line);
				parse_error=true;
				break;
			}
			/* set the mss to the greater value for Full TCP*/
			if (agentType_ == FULL) {
				cv->set_mss(max(init_mss,acc_mss));
			}
			/* set the initiator mss and acceptor mss 
			   for one-way TCP*/ 
			else{
				cv->set_init_mss(init_mss);
				cv->set_acc_mss(acc_mss);
			}
		}
		/*Window size*/
		else if(line[0] == 'w'){ 
			if(sscanf(line, "w %d %d", &(init_win), 
				  &(acc_win)) != 2){
				fprintf(stderr, "Parse error on: %s\n", line);
				parse_error=true;
				break;
			}
			cv->set_init_win(init_win);
			cv->set_acc_win(acc_win);
		}
		/*Per flow delay*/
		else if(line[0] == 'r'){ 
			if(sscanf(line, "r %d", &(int_junk)) != 1){
				fprintf(stderr, "Parse error on: %s\n", 
					line);
				parse_error=true;
				break;
			}
		}
		/*Per flow loss*/
		else if(line[0] == 'l'){ 
			if(sscanf(line, "l %f %f", &(float_junk), 
				  &(float_junk)) != 2){
				fprintf(stderr, "Parse error on: %s\n", line);
				parse_error=true;
				break;
			}
		}
		/*Process ADU lines*/
		else{
			unsigned long tmp=0; /* stores size or time */

			if(sscanf(line, "%*s %lu", &tmp) != 1){
				fprintf(stderr, "Parse error on: %s\n", 
					line);
				parse_error=true;
				break;
			}
			/* seq has recv waits */
			/* SEQ related ADU units*/
			if(cv->get_type() == SEQ){		
				if(line[0] == '>'){
					if(last_state == A || 
					   last_state == B){
						fprintf(stderr, 
"Got ADU A after ADU %d\n", last_state);
						parse_error=true;
						break;
					}
					/* Create the ADU */
					if(last_state == TA){
						adu = new ADU(last_time_value, 
							      0, tmp);
					}
					else if (last_state == TB) {
						adu = new ADU(0, 
							      last_time_value, 
							      tmp);
					}
					
					/* Add the ADU */
					cv->add_ADU(adu, INITIATOR);
					last_direction = INITIATOR;

					if (cv->get_type() == SEQ 
					    && tmp != FIN) {
						cv->incr_init_ADU_count();
					}
					last_state = A;

				}else if(line[0] == '<'){
					if(last_state == A || 
					   last_state == B){
						fprintf(stderr, "Got ADU B after ADU %d\n", last_state);
						parse_error=true;
						break;
					}
					/* Create the ADU */
					if(last_state == TB){
						adu = new ADU(last_time_value, 
							      0, tmp);
					}
					else if (last_state == TA) {
						adu = new ADU(0, 
							      last_time_value, 
							      tmp);
					}
					
					/* Add the ADU */
					cv->add_ADU(adu, ACCEPTOR);
					last_direction = ACCEPTOR;

					if (tmp != FIN) {
						cv->incr_acc_ADU_count();
					}

					last_state = B;

				}else if(line[0] == 't'){
					if(last_state == TA || 
					   last_state == TB){
						fprintf(stderr, "Got ADU TA/B after ADU %d\n", last_state);
						parse_error=true;
						break;
					}
					if(last_state == A){
						/* If the time is zero force a 
						   1 micro sec wait */
						if (tmp == 0) 
							last_time_value = 1;
						else
							last_time_value = tmp;

						last_state = TA;

					}else if(last_state == B){
						/* If the time is zero force a 
						   1 micro sec wait */
						if (tmp == 0)
							last_time_value = 1;
						else
							last_time_value = tmp;

						last_state = TB;
					}
				}				
			}
			/* conc has send waits*/
			/* CONC related ADU units*/
			else if(cv->get_type() == CONC){ 
				if(line[0] == 'c' && line[1] == '>'){
					if(last_state == A){
						fprintf(stderr, "Got ADU c> after c>\n");
						parse_error=true;
						break;
					}

					adu = new ADU(last_initiator_time_value,
						      0, tmp);
					cv->add_ADU(adu, INITIATOR);
					last_direction = INITIATOR;

					pending_initiator = false;
					last_state = A;
				}else if(line[0] == 'c' && line[1] == '<'){
					if(last_state == B){
						fprintf(stderr, "Got ADU c< after c<\n");
						parse_error=true;
						break;
					}

					adu = new ADU(last_acceptor_time_value,
						      0, tmp);
					cv->add_ADU(adu, ACCEPTOR);
					last_direction = ACCEPTOR;

					pending_acceptor = false;
					last_state = B;

				}else if(line[0] == 't' && line[1] == '>'){
					if(last_state == TA){
						fprintf(stderr, "Got ADU t> after t>\n");
						parse_error=true;
						break;
					}

					/* If the time is zero force a 
					   1 micro sec wait */
				       	if (tmp == 0) 
						last_initiator_time_value = 1;
					else
						last_initiator_time_value = tmp;
						
					pending_initiator = true;
					last_state = TA;

				}else if(line[0] == 't' && line[1] == '<'){
					if(last_state == TB){
						fprintf(stderr, "Got ADU t< after t<\n");
						parse_error=true;
						break;
					}

					/* If the time is zero force a 
					   1 micro sec wait */	
					if (tmp == 0) 
				       		last_acceptor_time_value = 1;
					else
						last_acceptor_time_value = tmp;

					pending_acceptor = true;
					last_state = TB;
				}
			}
		}
	} while (fgets(line, CVEC_LINE_MAX, cvfp_) != NULL);

	if(parse_error){
		/*delete the cv and return NULL if an error occurred*/
		fprintf(stderr,"error: cvecid=%lu\n", cv->get_ID());
		if (cv != NULL)
			delete cv;
		cv = NULL;
	}

	if (cv == NULL) {
		/*If there was an unexpected error return NULL*/
		return cv;
	}

	/*If no error occured, add the last ADU*/

	/* Add FIN for SEQ connection vector */
	if (last_time_value != 0 ) {
		if (last_state == TA) {
			adu = new ADU(last_time_value, 0, 0);
			cv->add_ADU(adu, INITIATOR);
			last_direction = INITIATOR;
		}
		else if (last_state == TB) {
			adu = new ADU(last_time_value, 0, 0);
			cv->add_ADU(adu, ACCEPTOR);
			last_direction = ACCEPTOR;
		}
	}

	/* Add a FIN for a CONC connection vector */
	if (pending_initiator == true) {
		adu = new ADU(last_initiator_time_value, 0, 0);
		cv->add_ADU(adu, INITIATOR);
		last_direction = INITIATOR;
	}
	if (pending_acceptor == true) {
		adu = new ADU(last_acceptor_time_value, 0, 0);
		cv->add_ADU(adu, ACCEPTOR);
		last_direction = ACCEPTOR;
	}

	/* Was the last ADU a FIN?  If not, we need to add one */
	if (adu->get_size() != 0) {
		adu = (ADU*) new ADU (fin_time_, 0, 0);
		cv->add_ADU (adu, last_direction);
	}

	return cv;
}

#undef A
#undef B
#undef TA
#undef TB

ConnVector*
Tmix::read_one_cvec_v2()
{
	char sym;       /* first token in line - a symbol */
	/* items to read */
	unsigned long start, id, send, recv, size;
	int numinit, numacc, initwin, accwin, init_mss, acc_mss;
	ConnVector* cv;
	ADU* adu;
	bool started_one = false;
	bool last_direction = INITIATOR;

	while (!feof (cvfp_)) {
		/* look at the first character in the line */
		sym = fpeek (cvfp_);

		if (sym == '#') {
			/* skip comments */
			fscanf (cvfp_, "%*[^\n]\n");
			continue;
		}

		/* break if we've already read one cvec */
		if (started_one && (sym == 'S' || sym == 'C')) {
			break;
		}
		started_one = true;

		if (sym == 'S') {
			/* start of new SEQ connection vector */
			fscanf (cvfp_, "%c %lu %*d %*d %lu\n", &sym, &start, 
				&id);
			cv = (ConnVector*) new ConnVector (id, 
 							   (double) 
							   start/1000000.0, 
							   SEQ, pkt_size_); 
		}
		else if (sym == 'C') {
			/* start of new CONC connection vector */
			fscanf (cvfp_, "%c %lu %d %d %*d %lu\n", &sym,
				&start, &numinit, &numacc, &id);
			cv = (ConnVector*) new ConnVector (id, 
							   (double)
							   start/1000000.0, 
							   CONC, numinit, 
							   numacc, pkt_size_); 
		}
		else if (sym == 'm') {
			/* maximum segment size */
			fscanf (cvfp_, "%c %d %d\n", &sym, &init_mss, &acc_mss);
			if (agentType_ == FULL) {
				cv->set_mss(max(init_mss,acc_mss));
			} else {
				cv->set_init_mss(init_mss);
				cv->set_acc_mss(acc_mss);
			}
		}
		else if (sym == 'w') {
			/* window size */
			fscanf (cvfp_, "%c %d %d\n", &sym, &initwin, &accwin);
			cv->set_init_win (initwin);
			cv->set_acc_win (accwin);
		}
		else if (sym == 'I') {
			/* new initiator ADU */
			fscanf (cvfp_, "%c %lu %lu %lu\n", &sym, &send, &recv, 
				&size);
			adu = (ADU*) new ADU (send, recv, size);
			cv->add_ADU (adu, INITIATOR);
			if (cv->get_type() == SEQ && size != FIN) {
				cv->incr_init_ADU_count();
			}
			last_direction = INITIATOR;
		}
		else if (sym == 'A') {
			/* new acceptor ADU */
			fscanf (cvfp_, "%c %lu %lu %lu\n", &sym, &send, &recv, 
				&size);
			adu = (ADU*) new ADU (send, recv, size);
			cv->add_ADU (adu, ACCEPTOR);
			if (cv->get_type() == SEQ && size != FIN) {
				cv->incr_acc_ADU_count();
			}
			last_direction = ACCEPTOR;
		}
		else {
			/* skip the line */
			fscanf (cvfp_, "%*[^\n]\n");
		}
	}

	/* Was the last ADU a FIN?  If not, we need to add one */
	if (adu->get_size() != 0) {
		adu = (ADU*) new ADU (fin_time_, 0, 0);
		cv->add_ADU (adu, last_direction);
		adu = NULL;
	}

	/* return the cv ptr */
	return cv;
}

void Tmix::start()
{            
	/* make sure that there are an equal number of acceptor nodes
           and initiator nodes */
	if (next_init_ind_ != next_acc_ind_) {
		fprintf (stderr, "Error: %d initiators and %d acceptors", 
			 next_init_ind_, next_acc_ind_);
	}
	total_nodes_ = next_init_ind_;
	running_  = true;
	
	/* read from the connection vector file */
	ConnVector* cv;
	int i=0;
	while (!feof (cvfp_) && i < (int) step_size_) {
		/* read in step_size_ ConnVectors and add to list */
		cv = read_one_cvec();
		if (cv != NULL) {
			connections_.push_back(cv);
		}
		else {
			fprintf (stderr, "cv is null!\n");
		}
		i++;
	}
		
	if (debug_ >= 1) {
		fprintf (stderr, "Tmix %s> %d connections read\n", 
			 name(), (int) connections_.size());
	}
	
	/* Start scheduling connections */
	
	/* initialize the iterator to the next active connection */
	init_next_active();
	
	/* schedule the first connection */
	/* The idea behind prefill is to rapidly start up flows at the beginning
	   of the simulation to minimise the time needed for a warmup period. To
	   do this connections starting in [0,prefill_t_] s are started more
	   rapidly in the compressed interval
	   [(prefill_t_ - prefill_si_),prefill_t_] */
	if(get_next_start() < get_prefill_t()){
		timer_.resched(get_next_prefill_start());                  
	} else {
		timer_.resched(get_next_start());
	}
}

void Tmix::stop()
{
	running_ = false;
}

int Tmix::command(int argc, const char*const* argv) {
	if (argc == 2) {
		if (strcmp (argv[1], "start") == 0) {
			start();
			return (TCL_OK);
		}
		else if (strcmp (argv[1], "stop") == 0) {
			stop();
			return (TCL_OK);
		}
		else if (strcmp (argv[1], "active-connections") == 0) {
			if (outfp_) {
				fprintf (outfp_, "%lu ", get_active());
				fflush (outfp_);
			}
			else
				fprintf (stderr, "%lu ", get_active());
			return (TCL_OK);
		}
		else if (strcmp (argv[1], "total-connections") == 0) {
			if (outfp_) {
				fprintf (outfp_, "%lu ", get_total());
				fflush (outfp_);
			}
			else
				fprintf (stderr, "%lu ", get_total());
			return (TCL_OK);
		}
		else if (strcmp(argv[1], "check-oneway-closed")==0) {
			check_oneway_closed_ = true;
			return(TCL_OK);
		}
	}
	else if (argc == 3) {
		if ((!strcmp (argv[1], "set-init")) 
		    || (!strcmp (argv[1], "init"))) {
			if (next_init_ind_ >= MAX_NODES) {
				return (TCL_ERROR);
			}
			initiator_[next_init_ind_] = (Node*) 
				lookup_obj(argv[2]);
			if (initiator_[next_init_ind_] == NULL) {
				return (TCL_ERROR);
			}
			next_init_ind_++;
			return (TCL_OK);			
		}
		else if ((!strcmp (argv[1], "set-acc")) || 
			 (!strcmp (argv[1], "acc"))) {
			if (next_acc_ind_ >= MAX_NODES) {
				return (TCL_ERROR);
			}
			acceptor_[next_acc_ind_] = (Node*) 
				lookup_obj(argv[2]);
			if (acceptor_[next_acc_ind_] == NULL)
				return (TCL_ERROR);
			next_acc_ind_++;
			return (TCL_OK);			
                    
		}
		else if (strcmp (argv[1], "set-outfile") == 0) {
			outfp_ = fopen (argv[2], "a");
			if (outfp_)
				return (TCL_OK);
			else 
				return (TCL_ERROR);
		}
		else if (strcmp (argv[1], "set-cvfile") == 0) {  
			cvfp_ = fopen (argv[2], "r");
			cvfn_ = argv[2];
			if (cvfp_)
				return (TCL_OK);
			else 
				return (TCL_ERROR);
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
		else if (strcmp (argv[1], "set-prefill-t") == 0) {
			prefill_t_ = atof (argv[2]);
			if (prefill_t_ > 0) {
				prefill_a_ = prefill_si_/prefill_t_;
				return (TCL_OK);
			} else if (prefill_t_ < 0) {
				return (TCL_ERROR);
			}
			return (TCL_OK);
		} 
		else if (strcmp (argv[1], "set-prefill-si") == 0) {
			prefill_si_ =  atof (argv[2]);
			/* calculation down twice so either order works */
			if (prefill_t_ > 0)
				prefill_a_ = prefill_si_/prefill_t_;
			return (TCL_OK);
		} 
		else if (strcmp (argv[1], "set-scale") == 0) {
			scale_ = atof (argv[2]);
			return (TCL_OK);
		}               
		else if (strcmp (argv[1], "set-end") == 0) {
			end_ = atof (argv[2]);
			return (TCL_OK);
		}               
		else if (!strcmp (argv[1], "set-TCP")) {
			strcpy (tcptype_, argv[2]);
			return (TCL_OK);
		}
		else if (!strcmp (argv[1], "set-sink")) {
			strcpy (sinktype_, argv[2]);
			return (TCL_OK);
		}
		else if (strcmp(argv[1],"set-agent-type") == 0) {
			if (strcmp(argv[2], "full") == 0) {
				agentType_ = FULL;
			}
			else if (strcmp (argv[2], "one-way") == 0){
				agentType_ = ONE_WAY;
			}
			return (TCL_OK);
			
		}
		else if(strcmp(argv[1],"set-pkt-size")==0) {
			pkt_size_ = atoi(argv[2]);
			return(TCL_OK);
		}
		else if (strcmp(argv[1], "set-fin-time")==0) {
			fin_time_ = atoi(argv[2]);
			return(TCL_OK);
		}
		else if(strcmp(argv[1],"set-step-size")==0) {
			step_size_=atol(argv[2]);
			return(TCL_OK);
		}
		else if (strcmp (argv[1], "recycle") == 0) {
			Agent* tcp = (Agent*) lookup_obj(argv[2]);
			return crecycle(tcp);
		}
	}
	return TclObject::command(argc, argv);
}


/*::::::::::::::::::::::::: TIMER HANDLER classes :::::::::::::::::::::::::::*/

void TmixTimer::expire(Event* = 0) 
{

}

void TmixTimer::handle(Event* e)
{
	double next_start;

	TimerHandler::handle(e);

	/* if stopped, wait for all active connections to finish */
	if (!mgr_->running()) {
		if (mgr_->get_active() != 0) {
			resched (1);              // check back in 1 second...
		}
		return;
	}

	/* setup new connection */
	mgr_->setup_connection();

    	/* increment ptr to next active connection */
	mgr_->incr_next_active();

	if (mgr_->scheduled_all()) {
		/* we've just scheduled the last connection */
		mgr_->stop();
		return;
	}
	
	/* schedule time for next connection */
	/* For prefill, see note in Tmix::start() */
	if(mgr_->get_next_start() < mgr_->get_prefill_t()){
		next_start = mgr_->get_next_prefill_start() - mgr_->now(); 
	} else {
		/* is this past the end of the simulation? */
		if (mgr_->get_end() > 0 && 
		    mgr_->get_next_start() > mgr_->get_end()) {
			mgr_->stop();
			return;
		}
		next_start = mgr_->get_next_start() - mgr_->now();
	}

	if (next_start < 0) {
		/* if we're behind, don't schedule a negative time */
		fprintf (stderr, "Tmix (%s) resched time reset from %f to 0\n",
			 mgr_->name(), next_start);
		next_start = 0;
	}
	if (mgr_->debug() >= 3) {
		fprintf (stderr, "\tnext connection starting in %f s...\n", 
			 next_start);
	}
	resched (next_start);
}


void TmixAppTimer::expire(Event* = 0)
{
}

void TmixAppTimer::handle(Event* e)
{
	TimerHandler::handle(e);  /* resets the timer status to timer_idle */
	t_->timeout();
}

/*:::::::::::::::::::::::: TMIX APPLICATION class ::::::::::::::::::::*/

static class TmixAppClass : public TclClass {
public:
	TmixAppClass() : TclClass("Application/Tmix") {}
	TclObject* create(int, const char*const*) {
		return (new TmixApp);
	}
} class_app_tmix;

TmixApp::~TmixApp()
{
	Tcl& tcl = Tcl::instance();
	if (agent_ != NULL) {
		tcl.evalf ("delete %s", agent_->name());
	}
}

inline void TmixApp::incr_expected_bytes (int bytes)
{
	if (tmixAgent_->getType() == ONE_WAY) {
		/* set expected bytes as multiple of pckts to be sent 
		   to avoid early recycle of apps, agents */
		expected_bytes_ += ((bytes-1)/mss_ + 1) * mss_;
	}
	else {
		expected_bytes_ += bytes;
	}
}

char* TmixApp::id_str()
{
	static char appname[50];
	sprintf (appname, " %s / %lu / %lu / %d.%d ", name(), id_, 
		 get_global_id(), agent_->addr(), agent_->port());
	return appname;
}

bool TmixApp::ADU_empty() 
{
	if (cv_ == NULL) {
		return true;
	}
	if (type_ == INITIATOR) {
		return (cv_->get_init_ADU().empty());
	}
	else {
		return (cv_->get_acc_ADU().empty());
	}
}

bool TmixApp::end_of_ADU() 
{
	if (cv_ == NULL) {
		return true;
	}
	if (type_ == INITIATOR) {
		return ((int) cv_->get_init_ADU().size() == ADU_ind_);
	}
	else {
		return ((int) cv_->get_acc_ADU().size() == ADU_ind_);
	}
}

int TmixApp::size_of_ADU() 
{
	if (type_ == INITIATOR) {
		return ((int) cv_->get_init_ADU().size());
	}
	else {
		return ((int) cv_->get_acc_ADU().size());
	}
}

ADU* TmixApp::get_current_ADU()
{
	if (end_of_ADU() || ADU_empty()) {
		sent_last_ADU_ = true;
		return NULL;
	}
	
	if (type_ == INITIATOR) {
		return (cv_->get_init_ADU()[ADU_ind_]);
	}
	else {
		return (cv_->get_acc_ADU()[ADU_ind_]);
	}
}

bool TmixApp::send_first()
{
	ADU* adu = get_current_ADU();

	if (!adu) {
		return false;
	}

	return (adu->get_recv_wait_sec() == 0);
}

void TmixApp::start()
{
	/* initialize variables */
	running_ = true;
	ADU_ind_ = 0;

	get_agent()->listen();
	if (sink_ != NULL) {
		sink_->listen();
	}
	if (mgr_->debug() >= 3) {
		fprintf (stderr, "\tApp (%s)> listening\n", id_str());
	}

	/* if we send data first (before waiting for recv, send now */
	if (send_first()) {
		timer_.resched(0);      // send data now
	}
}

void TmixApp::stop()
{
	running_ = false;
}

void TmixApp::recycle()
{
	id_ = 0;
	ADU_ind_ = 0;
	cv_ = NULL;
        running_ = false;
	total_bytes_ = 0;
	expected_bytes_ = 0;
	peer_ = NULL;
        mgr_ = NULL;
	agent_ = NULL;
	timer_.force_cancel();
	sent_last_ADU_ = false;
	waiting_to_send_ = false;
}


void TmixApp::timeout()
{ 
	/* look at the next thing to send */
	double send_wait = 0;
	ADU* adu = get_current_ADU();
	if (!adu) {
		if (tmixAgent_->getType() == ONE_WAY && 
		    mgr_->check_oneway_closed()) {
			if (agent_->is_closed()) {
				/* all outstanding packets have been ACK'd */
				mgr_->crecycle(agent_);
			} else {
				timer_.resched (1); /* wait 1 second and check*/
			}
		}
		return;
	}

	waiting_to_send_ = false;

	unsigned long size = adu->get_size();

	if (size == FIN) {
		sent_last_ADU_ = true;

		/* sending FIN, time to end connection */
		if (tmixAgent_->getType() == ONE_WAY) {
			if (mgr_->debug() >= 3) {
				fprintf (stderr, "%f App (%s)> closing connection (FIN)\n", mgr_->now(), id_str());
			}
			if (!mgr_->check_oneway_closed() || 
			    agent_->is_closed()) {
				mgr_->crecycle(agent_);
			} else {
				timer_.resched (1); /* wait 1 second and check*/
			}
		} else {
			// Full-TCP
			if (peer_->sent_last_ADU()) {
				/* close() is not equivalent - 
				   flag only for TCP_FULL*/
				if (mgr_->debug() >= 3) {
					fprintf (stderr,"%f App (%s)> sending FIN (+2 B)\n", mgr_->now(), id_str());
				}
				agent_->sendmsg (2, "MSG_EOF"); 
			}
		}
		return;
	}
	
	/* time to send an ADU */

	/* let the peer know what we're sending */
	peer_->incr_expected_bytes (size);

	if (mgr_->debug() >= 4) {
		fprintf(stderr,"%f App (%s)> sent %lu B ADU (%d expected)\n",
			mgr_->now(), id_str(), size, 
			peer_->get_expected_bytes());
	}
	
	/* need to know if this is the last ADU to send */
	ADU_ind_++;     		/* move to the next ADU */
	adu = get_current_ADU();
	if (adu) {
		send_wait = adu->get_send_wait_sec();
		if (mgr_->get_end() > 0 && 
		    (send_wait + mgr_->now()) > mgr_->get_end()) {
			ADU_ind_ = size_of_ADU(); /* move ADU_ind_ to the end */
			adu = get_current_ADU();
		}

	}
	
	if (end_of_ADU()) {
		/* adu == NULL, time for last ADU */
		sent_last_ADU_ = true;
		if (mgr_->debug() >= 4) {
			fprintf (stderr, "%f App (%s)> sent last ADU", 
				 mgr_->now(), id_str());
		}
		if (peer_->sent_last_ADU()) {
			if (tmixAgent_->getType() == FULL) {
				/* peer is also done, close connection */
				if (mgr_->debug() >= 3) {
					fprintf (stderr,", sending FIN (%ld B)",
						 size);
				}
				agent_->sendmsg (size, "MSG_EOF"); 
			}
		}
		else {
			/* wait for peer to close connection */
			agent_->sendmsg (size);
			if (mgr_->debug() >= 4) {
				fprintf (stderr, "\n");
			}
		}
		return;
	}

	/* was not last ADU, still have more to send */
	agent_->sendmsg (size);
	
	if (!adu) {
		/* double-check to make sure adu not NULL */
		return;
	}
	
	if (send_wait == 0) {
		/* nothing to send yet */
		return;
	}

	/* set timer to send next ADU */
	waiting_to_send_ = true;
	timer_.resched (send_wait);
	if (mgr_->debug() >= 3) {
		fprintf (stderr, "%f App (%s)> sending next ADU in %f s\n", 
			 mgr_->now(), id_str(), send_wait);
	}
}

void TmixApp::recv(int bytes)
{
	double recv_wait;
	ADU* adu;

	if (tmixAgent_->getType() == ONE_WAY) {
		if (bytes == 40) {
			/* this is just a fake SYN */
			return;
		}
		bytes = bytes - 40;
	}

	/* we've received a packet */
	total_bytes_ += bytes;

	if (mgr_->debug() >= 5 && total_bytes_ < expected_bytes_)
		fprintf (stderr, "%f App (%s)> recv %d B (total: %d, expected: %d)\n", mgr_->now(), id_str(), bytes, total_bytes_, expected_bytes_);

	if (total_bytes_ >= expected_bytes_) {
		/* finished receiving ADU from peer */
		if (mgr_->debug() >= 4)
			fprintf (stderr, 
				 "%f App (%s)> recv ADU (total %d B, expected: %d B)\n", mgr_->now(), id_str(), total_bytes_, expected_bytes_);

		if (tmixAgent_->getType() == ONE_WAY) {
			// one-way won't give us the exact number of 
			// expected bytes because of constant pktsz
			total_bytes_ = 0;
		} else {
			total_bytes_ -= expected_bytes_;
		}
		expected_bytes_ = 0;

		if (mgr_->debug() >= 5 && total_bytes_ > 0) {
			fprintf (stderr, 
				 "App (%s)> %d bytes still left\n",
				 id_str(), total_bytes_);
		}
		
		if (tmixAgent_->getType() == ONE_WAY 
		    && sent_last_ADU() && peer_->sent_last_ADU()) {
			/* We've sent our last ADU and peer sent last ADU */
			if (mgr_->debug() >= 4) {
				fprintf (stderr, 
					 "%f App (%s)> sent last ADU, recvd peer's last ADU\n", mgr_->now(), id_str());
			}
			if (!mgr_->check_oneway_closed() || agent_->is_closed()) {
				mgr_->crecycle(agent_);
			} else {
				timer_.resched (1); /* wait 1 second and check*/
			}
			return;
		}

		adu = get_current_ADU();
		if (!adu) {
			return;
		}

		recv_wait = adu->get_recv_wait_sec();
		if (mgr_->get_end() > 0 &&
		    (recv_wait + mgr_->now()) > mgr_->get_end()) {
			ADU_ind_ = size_of_ADU(); /* move ADU_ind_ to the end */
			adu = get_current_ADU();
			if  (tmixAgent_->getType() == ONE_WAY) {
				sent_last_ADU_ = true;
				if (!mgr_->check_oneway_closed() 
				    || agent_->is_closed()) {
					mgr_->crecycle(agent_);
				}
				else {
					/* wait 1 second and check again */
					timer_.resched (1); 
				}
				return;
			}		
		}

		if (recv_wait == 0) {
			/* no action needed because we're not waiting for 
			 * the peer to finish */
			return;
		}
	
		if (peer_->waiting_to_send()) {
			/* no action needed because we're waiting for
			 * the peer to send another ADU */
			return;
		}
		
		/* schedule next send */
		timer_.resched (recv_wait);
		if (mgr_->debug() >= 3) {
			fprintf (stderr, "%f App (%s)> sending next ADU in %f s\n", mgr_->now(), id_str(), recv_wait);
		}
	}
}
