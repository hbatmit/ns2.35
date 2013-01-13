/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
//

/*
 * webtraf.cc
 * Copyright (C) 1999 by the University of Southern California
 * $Id: webtraf.cc,v 1.31 2011/10/01 22:00:14 tom_henderson Exp $
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
// $Header: /cvsroot/nsnam/ns-2/webcache/webtraf.cc,v 1.31 2011/10/01 22:00:14 tom_henderson Exp $

#include "config.h"
#include <tclcl.h>
#include <iostream>

#include "node.h"
#include "pagepool.h"
#include "webtraf.h"

// Data structures that are specific to this web traffic model and 
// should not be used outside this file.
//
// - WebTrafPage
// - WebTrafObject

class WebPage : public TimerHandler {
public:
	WebPage(int id, WebTrafSession* sess, int nObj, Node* dst) :
		id_(id), sess_(sess), nObj_(nObj), curObj_(0), doneObj_(0),
		dst_(dst) {}
	virtual ~WebPage() {}

	inline void start() {
		// Call expire() and schedule the next one if needed
		status_ = TIMER_PENDING;
		handle(&event_);
	}
	inline int id() const { return id_; }
	Node* dst() { return dst_; }

	void doneObject() {
		if (++doneObj_ >= nObj_) {
			//printf("doneObject: %g %d %d \n", Scheduler::instance().clock(), doneObj_, nObj_);
			sess_->donePage((void*)this);
		}
	}
	inline int curObj() const { return curObj_; }
	inline int doneObj() const { return doneObj_; }

private:
	virtual void expire(Event* = 0) {
		// Launch a request. Make sure size is not 0!
		if (curObj_ >= nObj_) 
			return;
		sess_->launchReq(this, LASTOBJ_++, 
				 (int)ceil(sess_->objSize()->value()));
		if (sess_->mgr()->isdebug())
			printf("Session %d launched page %d obj %d\n",
			       sess_->id(), id_, curObj_);
	}
	virtual void handle(Event *e) {
		// XXX Note when curObj_ == nObj_, we still schedule the timer
		// once, but we do not actually send out requests. This extra
		// schedule is only meant to be a hint to wait for the last
		// request to finish, then we will ask our parent to delete
		// this page.
		// if (curObj_ <= nObj_) {
		//
		// Polly Huang: Wed Nov 21 18:18:51 CET 2001
		// With explicit doneObject() upcalls from the tcl
		// space, we don't need to play this trick anymore.
		if (curObj_ < nObj_) {
			// If this is not the last object, schedule the next 
			// one. Otherwise stop and tell session to delete me.
			TimerHandler::handle(e);
			curObj_++;
			// Kun-chan Lan: Mon Feb 11 10:12:27 PST 2002
			// Don't schedule another one when curObj_ = nObj_
			// otherwise the page might already have been deleted
			// before the next one is up and cause seg fault
			// in the case of larger interObj()->value()
			// sched(sess_->interObj()->value());
			if (curObj_ < nObj_) sched(sess_->interObj()->value());
		}
	}
	int id_;
	WebTrafSession* sess_;
	int nObj_, curObj_, doneObj_;
	Node* dst_;
	static int LASTOBJ_;
};

int WebPage::LASTOBJ_ = 1;

int WebTrafSession::LASTPAGE_ = 1;

// Constructor
WebTrafSession::WebTrafSession(WebTrafPool *mgr, Node *src, int np, int id, int ftcp_, int recycle_p) : 
	rvInterPage_(NULL), rvPageSize_(NULL),
	rvInterObj_(NULL), rvObjSize_(NULL), 
	mgr_(mgr), src_(src), nPage_(np), curPage_(0), donePage_(0),
	id_(id), interPageOption_(1), fulltcp_(0) {
	fulltcp_ = ftcp_;
	recycle_page_ = recycle_p;
}

// XXX Must delete this after all pages are done!!
WebTrafSession::~WebTrafSession() 
{
	if (donePage_ != curPage_) {
		fprintf(stderr, "done pages %d != all pages %d\n",
			donePage_, curPage_);
		abort();
	}
	if (status_ != TIMER_IDLE) {
		fprintf(stderr, "WebTrafSession must be idle when deleted.\n");
		abort();
	}

	// Recycle the objects of page level attributes if needed
	// Reuse these objects may save memory for large simulations--xuanc
	if (recycle_page_) {
		if (rvInterPage_ != NULL)
			Tcl::instance().evalf("delete %s", rvInterPage_->name());
		if (rvPageSize_ != NULL)
			Tcl::instance().evalf("delete %s", rvPageSize_->name());
		if (rvInterObj_ != NULL)
			Tcl::instance().evalf("delete %s", rvInterObj_->name());
		if (rvObjSize_ != NULL)
			Tcl::instance().evalf("delete %s", rvObjSize_->name());
	}
}

void WebTrafSession::donePage(void* ClntData) 
{
	WebPage* pg = (WebPage*)ClntData;
	if (mgr_->isdebug()) 
		printf("Session %d done page %d\n", id_, pg->id());
	if (pg->doneObj() != pg->curObj()) {
		fprintf(stderr, "done objects %d != all objects %d\n",
			pg->doneObj(), pg->curObj());
		abort();
	}
	mgr_->donePage(pg->id());
	delete pg;
	// If all pages are done, tell my parent to delete myself
	//
	if (++donePage_ >= nPage_)
		mgr_->doneSession(id_);
	else if (interPageOption_) {
		// Polly Huang: Wed Nov 21 18:23:30 CET 2001
		// add inter-page time option
		// inter-page time = end of a page to the start of the next
		sched(rvInterPage_->value());
		// printf("donePage: %g %d %d\n", Scheduler::instance().clock(), donePage_, curPage_);
	}
}

// Launch the current page
void WebTrafSession::expire(Event *)
{
	// Pick destination for this page
	Node* dst = mgr_->pickdst();
	// Make sure page size is not 0!
	WebPage* pg = new WebPage(LASTPAGE_++, this, 
				  (int)ceil(rvPageSize_->value()), dst);
	if (mgr_->isdebug())
		printf("Session %d starting page %d, curpage %d\n", 
		       id_, LASTPAGE_-1, curPage_);
	pg->start();
}

void WebTrafSession::handle(Event *e)
{
	// If I haven't scheduled all my pages, do the next one
	TimerHandler::handle(e);
	++curPage_;
	// XXX Notice before each page is done, it will schedule itself 
	// one more time, this makes sure that this session will not be
	// deleted after the above call. Thus the following code will not
	// be executed in the context of a deleted object. 
	//
	// Polly Huang: Wed Nov 21 18:23:30 CET 2001
	// add inter-page time option
	// inter-page time = inter-page-start time
	// If the interPageOption_ is not set, the XXX Notice above applies.
	if (!interPageOption_) {
		if (curPage_ < nPage_) {
			sched(rvInterPage_->value());
			// printf("schedule: %g %d %d\n", Scheduler::instance().clock(), donePage_, curPage_);
		}
	}
}

// Launch a request for a particular object
void WebTrafSession::launchReq(void* ClntData, int obj, int size) {
	mgr_->launchReq(src_, ClntData, obj, size);
}

static class WebTrafPoolClass : public TclClass {
public:
        WebTrafPoolClass() : TclClass("PagePool/WebTraf") { 	

	}
        TclObject* create(int, const char*const*) {
		return (new WebTrafPool());
	}
} class_webtrafpool;

WebTrafPool::~WebTrafPool()
{
	if (session_ != NULL) {
		for (int i = 0; i < nSession_; i++)
			delete session_[i];
		delete []session_;
	}
	if (server_ != NULL)
		delete []server_;
	if (client_ != NULL)
		delete []client_;
	// XXX Destroy tcpPool_ and sinkPool_ ?
}

void WebTrafPool::delay_bind_init_all()
{
	delay_bind_init_one("debug_");
	PagePool::delay_bind_init_all();
}

int WebTrafPool::delay_bind_dispatch(const char *varName,const char *localName,
				     TclObject *tracer)
{
	if (delay_bind_bool(varName, localName, "debug_", &debug_, tracer)) 
		return TCL_OK;
	return PagePool::delay_bind_dispatch(varName, localName, tracer);
}

// By default we use constant request interval and page size
WebTrafPool::WebTrafPool() : 
	session_(NULL), nServer_(0), server_(NULL), nClient_(0), client_(NULL),
	nTcp_(0), nSink_(0), fulltcp_(0), recycle_page_(0)
{
	bind("fulltcp_", &fulltcp_);
	bind("recycle_page_", &recycle_page_);
	bind("dont_recycle_", &dont_recycle_);
	// Debo
	asimflag_=0;
	LIST_INIT(&tcpPool_);
	LIST_INIT(&sinkPool_);
	dbTcp_a = dbTcp_r = dbTcp_cr = 0;
}

TcpAgent* WebTrafPool::picktcp()
{
	TcpAgent* a = (TcpAgent*)detachHead(&tcpPool_);
	if (a == NULL) {
		Tcl& tcl = Tcl::instance();
		tcl.evalf("%s alloc-tcp", name());
		a = (TcpAgent*)lookup_obj(tcl.result());
		if (a == NULL) {
			fprintf(stderr, "Failed to allocate a TCP agent\n");
			abort();
		}
	} else 
		nTcp_--;
	//printf("A# %d\n", dbTcp_a++);
	return a;
}

TcpSink* WebTrafPool::picksink()
{
	TcpSink* a = (TcpSink*)detachHead(&sinkPool_);
	if (a == NULL) {
		Tcl& tcl = Tcl::instance();
		tcl.evalf("%s alloc-tcp-sink", name());
		a = (TcpSink*)lookup_obj(tcl.result());
		if (a == NULL) {
			fprintf(stderr, "Failed to allocate a TCP sink\n");
			abort();
		}
	} else 
		nSink_--;
	return a;
}

Node* WebTrafPool::picksrc() {
	int n = int(floor(Random::uniform(0, nClient_)));
	assert((n >= 0) && (n < nClient_));
	return client_[n];
}

Node* WebTrafPool::pickdst() {
	int n = int(floor(Random::uniform(0, nServer_)));
	assert((n >= 0) && (n < nServer_));
	return(server_[n].get_node());
}

// pick end points for a new TCP connection
void WebTrafPool::pick_ep(TcpAgent** tcp, Agent** snk) {
	// Choose source
	*tcp = picktcp();

	// Choose destination
	if (fulltcp_) {
		*snk = picktcp();
	} else {
		*snk = picksink();
	}
}

// Launch a request for a particular object
void WebTrafPool::launchReq(Node *src_, void* ClntData, int obj, int size) {
	TcpAgent *ctcp;
	Agent *csnk;

	// Allocation new TCP connections for both directions
	pick_ep(&ctcp, &csnk);

	WebPage* pg = (WebPage*)ClntData;
	pages_[pg->id()] = ClntData;

	// Setup TCP connection and done
	Tcl::instance().evalf("%s launch-req %d %d %s %s %s %s %d", 
			      name(), obj, pg->id(),
			      src_->name(), pg->dst()->name(),
			      ctcp->name(), csnk->name(), size);

	// Debug only
	// $numPacket_ $objectId_ $pageId_ $sessionId_ [$ns_ now] src dst
#if 0
	printf("%d \t %d \t %d \t %d \t %g %d %d\n", size, obj, pg->id(), id_,
	       Scheduler::instance().clock(), 
	       src_->address(), pg->dst()->address());
	printf("** Tcp agents %d, Tcp sinks %d\n", nTcp(),nSink());
#endif
}

// Launch a request for a particular object
void WebTrafPool::launchResp(int obj_id, Node *svr_, Node *clnt_, Agent *tcp, Agent* snk, int size, int pid) {

	// Setup TCP connection and done
	Tcl::instance().evalf("%s launch-resp %d %d %s %s %s %s %d", 
			      name(), obj_id, pid, svr_->name(), clnt_->name(),
			      tcp->name(), snk->name(), size);

	// Debug only
	// $numPacket_ $objectId_ $pageId_ $sessionId_ [$ns_ now] src dst
#if 0
	printf("%d \t %d \t %d \t %d \t %g %d %d\n", size, obj, pg->id(), id_,
	       Scheduler::instance().clock(), 
	       src_->address(), pg->dst()->address());
	printf("** Tcp agents %d, Tcp sinks %d\n", nTcp(),nSink());
#endif
}

// Given sever's node id, find server
int WebTrafPool::find_server(int sid) {
	int n = 0;
	while (server_[n].get_nid() != sid && n < nServer_) {
		n++;
	}
	
	return(n);
}

void WebTrafPool::donePage (int pid) {
	pages_.erase(pid);
}
	
int WebTrafPool::command(int argc, const char*const* argv) {

	// Debojyoti Dutta ... for asim
	if (argc == 2){
		if (strcmp(argv[1], "use-asim") == 0) {
			asimflag_ = 1;
			//Tcl::instance().evalf("puts \"Here\"");
			return (TCL_OK);
		} 
	}
	else if (argc == 3) {
		if (strcmp(argv[1], "set-num-session") == 0) {
			if (session_ != NULL) {
				for (int i = 0; i < nSession_; i++) 
					delete session_[i];
				delete []session_;
			}
			nSession_ = atoi(argv[2]);
			session_ = new WebTrafSession*[nSession_];
			memset(session_, 0, sizeof(WebTrafSession*)*nSession_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "set-num-server") == 0) {
			nServer_ = atoi(argv[2]);
			if (server_ != NULL) 
				delete []server_;
			server_ = new WebServer[nServer_];
			for (int i = 0; i < nServer_; i++) {
				server_[i] = WebServer(this);
			};
			return (TCL_OK);
		} else if (strcmp(argv[1], "set-num-client") == 0) {
			nClient_ = atoi(argv[2]);
			if (client_ != NULL) 
				delete []client_;
			client_ = new Node*[nClient_];
			return (TCL_OK);
		} else if (strcmp(argv[1], "set-interPageOption") == 0) {
			int option = atoi(argv[2]);
			if (session_ != NULL) {
				for (int i = 0; i < nSession_; i++) {
					WebTrafSession* p = session_[i];
					p->set_interPageOption(option);
				}
			}
			return (TCL_OK);
		} else if (strcmp(argv[1], "doneObj") == 0) {
			WebPage* p = static_cast<WebPage*> (pages_[atoi(argv[2])]);
			// printf("doneObj for Page id: %d\n", p->id());
			p->doneObject();
			return (TCL_OK);
		} else if (strcmp(argv[1], "set-server-mode") == 0) {
			// <obj> set-server-mode <mode>
			int mode = atoi(argv[2]);
			for (int n = 0; n < nServer_; n++) {
				server_[n].set_mode(mode);

			}
			
			return (TCL_OK);
		} else if (strcmp(argv[1], "set-server-rate") == 0) {
			// <obj> set-server-rate <rate> 
			int rate = atoi(argv[2]);

			for (int n = 0; n < nServer_; n++) {
				server_[n].set_rate(rate);
			}
			
			return (TCL_OK);
		} else if (strcmp(argv[1], "set-server-qlimit") == 0) {
			// <obj> set-server-qlimit <qlimit> 
			int qlimit = atoi(argv[2]);

			for (int n = 0; n < nServer_; n++) {
				server_[n].set_queue_limit(qlimit);
			}
			
			return (TCL_OK);
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "set-server") == 0) {
			Node* s = (Node*)lookup_obj(argv[3]);
			if (s == NULL)
				return (TCL_ERROR);
			int n = atoi(argv[2]);
			if (n >= nServer_) {
				fprintf(stderr, "Wrong server index %d\n", n);
				return TCL_ERROR;
			}
			server_[n].set_node(s);
			return (TCL_OK);
		} else if (strcmp(argv[1], "set-client") == 0) {
			Node* c = (Node*)lookup_obj(argv[3]);
			if (c == NULL)
				return (TCL_ERROR);
			int n = atoi(argv[2]);
			if (n >= nClient_) {
				fprintf(stderr, "Wrong client index %d\n", n);
				return TCL_ERROR;
			}
			client_[n] = c;
			return (TCL_OK);
		} else if (strcmp(argv[1], "recycle") == 0) {
			// <obj> recycle <tcp> <sink>
			//
			// Recycle a TCP source/sink pair
			Agent* tcp = (Agent*)lookup_obj(argv[2]);
			Agent* snk = (Agent*)lookup_obj(argv[3]);
			
			if ((tcp == NULL) || (snk == NULL))
				return (TCL_ERROR);
			
			if (fulltcp_) {
				delete tcp;
				delete snk;
			} else if (!dont_recycle_) {
				// PS: hmm.. who deletes the agents?
				// PS: plain delete doesn't seem to work

				// Recyle both tcp and sink objects
				nTcp_++;
				// XXX TBA: recycle tcp agents
				insertAgent(&tcpPool_, tcp);
				nSink_++;
				insertAgent(&sinkPool_, snk);
				//printf("R# %d\n", dbTcp_r++);
			}

			return (TCL_OK);
		} else if (strcmp(argv[1], "set-server-rate") == 0) {
			// <obj> set_rate <server> <size> 
			int sid = atoi(argv[2]);
			int rate = atoi(argv[3]);

			int n = find_server(sid);
			if (n >= nServer_) 
				return (TCL_ERROR);

			server_[n].set_rate(rate);
			return (TCL_OK);
		} else if (strcmp(argv[1], "set-server-mode") == 0) {
			// <obj> set-mode <server> <mode>
			int sid = atoi(argv[2]);
			int mode = atoi(argv[3]);

			int n = find_server(sid);
			if (n >= nServer_) 
				return (TCL_ERROR);

			server_[n].set_mode(mode);
			return (TCL_OK);
		} else if (strcmp(argv[1], "set-server-qlimit") == 0) {
			// <obj> set-server-qlimit <server> <qlimit>
			int sid = atoi(argv[2]);
			int qlimit = atoi(argv[3]);

			int n = find_server(sid);
			if (n >= nServer_) 
				return (TCL_ERROR);

			server_[n].set_queue_limit(qlimit);
			return (TCL_OK);
		} 
	} else if (argc == 9) {
		if (strcmp(argv[1], "create-session") == 0) {
			// <obj> create-session <session_index>
			//   <pages_per_sess> <launch_time>
			//   <inter_page_rv> <page_size_rv>
			//   <inter_obj_rv> <obj_size_rv>
			int n = atoi(argv[2]);
			if ((n < 0)||(n >= nSession_)||(session_[n] != NULL)) {
				fprintf(stderr,"Invalid session index %d\n",n);
				return (TCL_ERROR);
			}
			int npg = (int)strtod(argv[3], NULL);
			double lt = strtod(argv[4], NULL);
			WebTrafSession* p = 
				new WebTrafSession(this, picksrc(), npg, n, fulltcp_, recycle_page_);
			int res = lookup_rv(p->interPage(), argv[5]);
			res = (res == TCL_OK) ? 
				lookup_rv(p->pageSize(), argv[6]) : TCL_ERROR;
			res = (res == TCL_OK) ? 
				lookup_rv(p->interObj(), argv[7]) : TCL_ERROR;
			res = (res == TCL_OK) ? 
				lookup_rv(p->objSize(), argv[8]) : TCL_ERROR;
			if (res == TCL_ERROR) {
				delete p;
				fprintf(stderr, "Invalid random variable\n");
				return (TCL_ERROR);
			}
			p->sched(lt);
			session_[n] = p;
			
			// Debojyoti added this for asim
			if(asimflag_){										// Asim stuff. Added by Debojyoti Dutta
				// Assumptions exist 
				//Tcl::instance().evalf("puts \"Here\"");
				double lambda = (1/(p->interPage())->avg())/(nServer_*nClient_);
				double mu = ((p->objSize())->value());
				//Tcl::instance().evalf("puts \"Here\"");
				for (int i=0; i<nServer_; i++){
					for(int j=0; j<nClient_; j++){
						// Set up short flows info for asim
						Tcl::instance().evalf("%s add2asim %d %d %lf %lf", this->name(),server_[i].get_nid(),client_[j]->nodeid(),lambda, mu);
					}
				}
				//Tcl::instance().evalf("puts \"Here\""); 
			}
			
			return (TCL_OK);
		} else if (strcmp(argv[1], "job_arrival") == 0) {
			//$self job_arrival $id $clnt $svr $tcp $snk $size $pid
			int obj_id = atoi(argv[2]);
			Node* clnt_ = (Node*)lookup_obj(argv[3]);
			Node* svr_ = (Node*)lookup_obj(argv[4]);
			// TCP source and sink pair
			Agent* tcp = (Agent*)lookup_obj(argv[5]);
			Agent* snk = (Agent*)lookup_obj(argv[6]);
			int size = atoi(argv[7]);
			int pid = atoi(argv[8]);

			int sid = svr_->nodeid();
			int n = find_server(sid);
			if (n >= nServer_) 
				return (TCL_ERROR);
			
			double delay = server_[n].job_arrival(obj_id, clnt_, tcp, snk, size, pid);
				
			Tcl::instance().resultf("%f", delay);
			return (TCL_OK);
		}
	}
	return PagePool::command(argc, argv);
}

