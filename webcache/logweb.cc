/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * logweb.cc
 * Copyright (C) 2001 by the University of Southern California
 * $Id: logweb.cc,v 1.8 2011/10/02 22:32:35 tom_henderson Exp $
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
// Generate web traffic based on HTTP log
// Xuan Chen (xuanc@isi.edu)
//
#include <tclcl.h>

#include "logweb.h"

// Timer to send requests
RequestTimer::RequestTimer(LogWebTrafPool* pool) {
	lwp = pool;
}

void RequestTimer::expire(Event *) {
	//if (e) 
	//	Packet::free((Packet *)e);

	lwp->run();
}


static class LogWebTrafPoolClass : public TclClass {
public:
        LogWebTrafPoolClass() : TclClass("PagePool/WebTraf/Log") {}
        TclObject* create(int, const char*const*) {
		return (new LogWebTrafPool());
	}
} class_logwebtrafpool;

LogWebTrafPool::LogWebTrafPool() {
	num_obj = 0;
	// initialize next request
	next_req.time = 0;
	next_req.client = 0;
	next_req.server = 0;
	next_req.size = 0;

	// initialize request timer
	req_timer = new RequestTimer(this);
	start_t = 0;
}

LogWebTrafPool::~LogWebTrafPool() {
	if (fp)
		fclose(fp);
	if (req_timer) delete req_timer;
}

int LogWebTrafPool::loadLog(const char* filename) {
	fp = fopen(filename, "r");
	if (fp == 0)
		return(0);
	
	return(1);
}

int LogWebTrafPool::start() {
	start_t = Scheduler::instance().clock();
	processLog();
	return(1);
}

int LogWebTrafPool::processLog() {
	int time, client, server, size;

	if (!feof(fp)) {
		fscanf(fp, "%d %d %d %d\n", &time, &client, &server, &size);
		// save information for next request
		next_req.time = time;
		next_req.client = client;
		next_req.server = server;
		next_req.size = size;	

		double now = Scheduler::instance().clock();
		double delay = time + start_t - now;
		req_timer->resched(delay);

		return(1);

	} else 
		return(0);
}

int LogWebTrafPool::run() {
	launchReq(next_req.client, next_req.server, next_req.size);
	processLog();
	return(1);
}

Node* LogWebTrafPool::picksrc(int id) {
	int n = id % nClient_;
	assert((n >= 0) && (n < nClient_));
	return client_[n];
}

Node* LogWebTrafPool::pickdst(int id) {
	int n = id % nServer_;
	assert((n >= 0) && (n < nServer_));
	return server_[n].get_node();
}

int LogWebTrafPool::launchReq(int cid, int sid, int size) {
	TcpAgent *tcp;
	Agent *snk;
	
	// Allocation new TCP connections for both directions
	pick_ep(&tcp, &snk);

	// pick client and server nodes
	Node* client = picksrc(cid);
	Node* server = pickdst(sid);

	int num_pkt = size / 1000 + 1;

	// Setup TCP connection and done
	Tcl::instance().evalf("%s launch-req %d %d %s %s %s %s %d %d", 
			      name(), num_obj, num_obj + 1,
			      client->name(), server->name(),
			      tcp->name(), snk->name(), num_pkt, NULL);

	return(1);
}

int LogWebTrafPool::command(int argc, const char*const* argv) {
	if (argc == 2) {
		if (strcmp(argv[1], "start") == 0) {
			if (start())
				return (TCL_OK);
			else
				return (TCL_ERROR);
			
		}
		
	} else if (argc == 3) {
		if (strcmp(argv[1], "loadLog") == 0) {
			if (loadLog(argv[2]))
				return (TCL_OK);
			else
				return (TCL_ERROR);

		} else if (strcmp(argv[1], "doneObj") == 0) {
			return (TCL_OK);
		} 
	}
	return WebTrafPool::command(argc, argv);
}
