
/*
 * webserver.h
 * Copyright (C) 1999 by the University of Southern California
 * $Id: webserver.h,v 1.6 2011/10/01 22:00:14 tom_henderson Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */

//
// Incorporation Polly's web traffic module into the PagePool framework
//
// Simple web server implementation
// Two server scheduling policies supported: fcfs and stf
// Xuan Chen (xuanc@isi.edu)
//

#ifndef ns_webserver_h
#define ns_webserver_h

#include "webtraf.h"

#define NO_DELAY 0
#define FCFS_DELAY 1
#define STF_DELAY 2

// Data structure for incoming jobs (requests)
struct job_s {
  int obj_id;
  Node *clnt;
  Agent *tcp;
  Agent *snk;
  int size;
  int pid;
  job_s *next;
};

class WebTrafPool;

// Data structure for web server
class WebServer : public TimerHandler{
 private:
       void WebServer_init(WebTrafPool *);

 public:
	WebServer(WebTrafPool*pool_) { WebServer_init(pool_); };
	WebServer() { WebServer_init(NULL); };
	
	// Assign node to server
	void set_node(Node *);
	// Return server's node
	Node* get_node();
	// Return server's node id
	int get_nid();

	// Set server processing rate
	void set_rate(double);
	// Set server function mode
	void set_mode(int);
	// Set the limit for job queue
	void set_queue_limit(int);

	// Handling incoming job
	double job_arrival(int, Node *, Agent *, Agent *, int, int);

 private:
	// The web page pool associated with this server
	WebTrafPool *web_pool_;

	// The node associated with this server
	Node *node;

	// Server processing rate KB/s
	double rate_;
	
	// Flag for web server:
	// 0: there's no server processing delay
	// 1: server processing delay from FCFS scheduling policy
	// 2: server processing delay from STF scheduling policy
	int mode_;

	// Job queue
	job_s *head, *tail;
	int queue_size_;
	int queue_limit_;

	virtual void expire(Event *e);

	double job_departure();
	
	// Flag for server status
	int busy_;
	void schedule_next_job();
};
#endif //ns_webserver_h
