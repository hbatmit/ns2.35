
/*
 * webserver.cc
 * Copyright (C) 1999 by the University of Southern California
 * $Id: webserver.cc,v 1.8 2011/10/01 22:00:14 tom_henderson Exp $
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
#include "webserver.h"

void
WebServer::WebServer_init(WebTrafPool *webpool) {
  web_pool_ = webpool;
  
  // clean busy flag
  busy_ = 0;
  
  // Initialize function flag:
  // 0: there's no server processing delay
  // 1: server processing delay from FCFS scheduling policy
  // 2: server processing delay from STF scheduling policy
  set_mode(0);
  
  // Initialize server processing raste
  set_rate(1);
  
  // initialize the job queue
  head = tail = NULL;
  queue_size_ = 0;
  queue_limit_ = 0;

  //cancel();
}

// Set server processing rate
void WebServer::set_rate(double s_rate) {
  rate_ = s_rate;
}

// Set server function mode
void WebServer::set_mode(int s_mode) {
  mode_ = s_mode;
}

// Set the limit for job queue
void WebServer::set_queue_limit(int limit) {
  queue_limit_ = limit;
}


// Return server's node id
int WebServer::get_nid() {
  return(node->nodeid());
}

// Assign node to server
void WebServer::set_node(Node *n) {
  node = n;
}
// Return server's node
Node* WebServer::get_node() {
  return(node);
}

double WebServer::job_arrival(int obj_id, Node *clnt, Agent *tcp, Agent *snk, int size, int pid) {
  // There's no server processing delay
  if (! mode_) {
    web_pool_->launchResp(obj_id, node, clnt, tcp, snk, size, pid);

    return 1;
  }

  //printf("%d %d\n", queue_limit_, queue_size_);
  if (!queue_limit_ || queue_size_ < queue_limit_) {
    // Insert the new job to the job queue
    job_s *new_job = new(job_s);
    new_job->obj_id = obj_id;
    new_job->clnt = clnt;
    new_job->tcp = tcp;
    new_job->snk = snk;
    new_job->size = size;
    new_job->pid = pid;
    new_job->next = NULL; 

    // always insert the new job to the tail.
    if (tail)
      tail->next = new_job;
    else
      head = new_job;
    tail = new_job;

    queue_size_++;
  } else {
    // drop the incoming job
    //printf("server drop job\n");
    return 0;
  }

  // Schedule the dequeue time when there's no job being processed
  if (!busy_) 
    schedule_next_job();
  
  return 1;
}


double WebServer::job_departure() {
  if (head) {
    web_pool_->launchResp(head->obj_id, node, head->clnt, head->tcp, head->snk, head->size, head->pid);
    
    // delete the first job
    job_s *p = head;
    if (head->next)
      head = head->next;
    else 
      head = tail = NULL;
    
    delete(p);
    queue_size_--;
  }
  
  // Schedule next job
  schedule_next_job();
  return 0;
}

void WebServer::schedule_next_job() {
  job_s *p, *q;
  
  if (head) {
    // do shortest task first scheduling
    if (mode_ == STF_DELAY) { 
      // find the shortest task
      p = q = head;
      while (q) {
	if (p->size > q->size)
	  p = q;
	
	q = q->next;
      }
      
      // exchange the queue head with the shortest job
      int obj_id = p->obj_id;
      Node *clnt = p->clnt;
      int size = p->size;
      int pid = p->pid;
      
      p->obj_id = head->obj_id;
      p->clnt = head->clnt;
      p->size = head->size;
      p->pid = head->pid;

      head->obj_id = obj_id;
      head->clnt = clnt;
      head->size = size;
      head->pid = pid;
    }
    
    // Schedule the processing timer
    double delay_ = head->size / rate_;
    resched(delay_);
    busy_ = 1;
    //	printf("%d, %f, %f\n", head->size, rate_, delay_);
  } else
    busy_ = 0;
}

// Processing finished
void WebServer::expire(Event *) {
  job_departure();
}
