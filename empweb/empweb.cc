/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * empweb.cc
 * Copyright (C) 2001 by the University of Southern California
 * $Id: empweb.cc,v 1.21 2010/03/08 05:54:50 tom_henderson Exp $
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
// Empirical Web traffic model that simulates Web traffic based on a set of
// CDF (Cumulative Distribution Function) data derived from live tcpdump trace
// The structure of this file is largely borrowed from webtraf.cc
//
// $Header: /cvsroot/nsnam/ns-2/empweb/empweb.cc,v 1.21 2010/03/08 05:54:50 tom_henderson Exp $

#include <tclcl.h>

#include "empweb.h"


// Data structures that are specific to this web traffic model and 
// should not be used outside this file.
//
// - EmpWebTrafPage
// - EmpWebTrafObject

class EmpWebPage : public TimerHandler {
public:
	EmpWebPage(int id, EmpWebTrafSession* sess, int nObj, Node* dst, int svrId) :
	persistOption_(0), id_(id), sess_(sess), nObj_(nObj), curObj_(0), doneObj_(0), dst_(dst), svrId_(svrId) {}
	virtual ~EmpWebPage() {}

	inline void start() {
		// Call expire() and schedule the next one if needed
		status_ = TIMER_PENDING;
		handle(&event_);
	}
	inline int id() const { return id_; }
	inline int svrId() const { return svrId_; }
	Node* dst() { return dst_; }

	void doneObject() {
		if (sess_->mgr()->isdebug())
	   		printf("doneObject: %g done=%d total=%d \n", Scheduler::instance().clock(), doneObj_, nObj_);

	 	if (++doneObj_ >= nObj_) {
	   		printf("doneObject: %g %d %d \n", Scheduler::instance().clock(), doneObj_, nObj_);
		        sess_->donePage((void*)this);
//		}
//		sched(sess_->interObj()->value());
	 	} else if (persistOption_) {
				sched(sess_->interObj()->value());
		}
		
	}
        inline int curObj() const { return curObj_; }
	inline int doneObj() const { return doneObj_; }
		
	inline void set_persistOption(int opt) { persistOption_ = opt; }
	int persistOption_ ;  //0: http1.0  1: http1.1 ; use http1.0 as default

private:
	virtual void expire(Event* = 0) {
		// Launch a request. Make sure size is not 0!
		if (curObj_ >= nObj_) 
			return;
		sess_->launchReq(this, LASTOBJ_++, 
				 (int)ceil(sess_->objSize()->value()),
				 (int)ceil(sess_->reqSize()->value()), sess_->id(), persistOption_);
		if (sess_->mgr()->isdebug())
			printf("expire: Session %d launched page %d obj %d nObj %d \n",
			       sess_->id(), id_, curObj_, nObj_);
	}
	virtual void handle(Event *e) {
		if (sess_->mgr()->isdebug())
			printf("handle: Session %d launched page %d obj %d\n",
			       sess_->id(), id_, curObj_);

		TimerHandler::handle(e);
		curObj_++;
		if (!persistOption_) {
			if (curObj_ <  nObj_) sched(sess_->interObj()->value());
		}
	}
	int id_;
	EmpWebTrafSession* sess_;
	int nObj_, curObj_;
	int doneObj_;
	Node* dst_;
	int svrId_ ;
	static int LASTOBJ_;
};

int EmpWebPage::LASTOBJ_ = 1;

int EmpWebTrafSession::LASTPAGE_ = 1;

int EmpWebTrafPool::LASTFLOW_ = 1;


// XXX Must delete this after all pages are done!!
EmpWebTrafSession::~EmpWebTrafSession() 
{
	if (donePage_ != curPage_) {
		fprintf(stderr, "done pages %d != all pages %d\n",
			donePage_, curPage_);
		abort();
	}
	if (status_ != TIMER_IDLE) {
		fprintf(stderr, "EmpWebTrafSession must be idle when deleted.\n");
		abort();
	}
/*	
	if (rvInterPage_ != NULL)
		Tcl::instance().evalf("delete %s", rvInterPage_->name());
	if (rvPageSize_ != NULL)
		Tcl::instance().evalf("delete %s", rvPageSize_->name());
	if (rvInterObj_ != NULL)
		Tcl::instance().evalf("delete %s", rvInterObj_->name());
	if (rvObjSize_ != NULL)
		Tcl::instance().evalf("delete %s", rvObjSize_->name());
	if (rvReqSize_ != NULL)
		Tcl::instance().evalf("delete %s", rvReqSize_->name());
	if (rvPersistSel_ != NULL)
		Tcl::instance().evalf("delete %s", rvPersistSel_->name());
	if (rvServerSel_ != NULL)
		Tcl::instance().evalf("delete %s", rvServerSel_->name());

*/

}

void EmpWebTrafSession::donePage(void* ClntData) 
{
	EmpWebPage* pg = (EmpWebPage*)ClntData;
	if (mgr_->isdebug()) 
		printf("Session %d done page %d\n", id_, pg->id());
		
	if (pg->doneObj() != pg->curObj()) {
	        fprintf(stderr, "done objects %d != all objects %d\n",
	                pg->doneObj(), pg->curObj());
	                abort();
	}
	

	//for HTTP1.1 persistent-connection
        if (pg->persistOption_) {
	  	if (!fulltcp_) { 
        	//recycle TCP connection
			mgr_->recycleTcp(ctcp_);
			mgr_->recycleTcp(stcp_);
			mgr_->recycleSink(csnk_);
			mgr_->recycleSink(ssnk_);
		} else {
			Tcl::instance().evalf("%s disconnect-full %s %s %s %s",
			        mgr_->name(),
				src_->name(), pg->dst()->name(),
			        ctcp_->name(),  stcp_->name());
		}
	}

	delete pg;

	// If all pages are done, tell my parent to delete myself
	if (++donePage_ >= nPage_) {
	    mgr_->doneSession(id_);
        } else if (interPageOption_) {
		sched(rvInterPage_->value());
		// printf("donePage: %g %d %d\n", Scheduler::instance().clock(), donePage_, curPage_);
	}
}

// Launch the current page
void EmpWebTrafSession::expire(Event *)
{
	// Pick destination for this page
	//temporary hack for isi traffic
	int n;

        if (clientIdx_ < mgr()->nClientL_) n = 0 ; //ISI server
	else
           n = int(ceil(serverSel()->value()));


        assert((n >= 0) && (n < mgr()->nSrc_));
        Node* dst = mgr()->server_[n];


	// Make sure page size is not 0!
	EmpWebPage* pg = new EmpWebPage(LASTPAGE_++, this, 
				  (int)ceil(rvPageSize_->value()), dst, n);

        //each page either use persistent or non-persistent connection
	int opt = (int)ceil(this->persistSel()->value());
        pg->set_persistOption(opt);

	if (mgr_->isdebug())
		printf("Session %d starting page %d, curpage %d \n", 
		       id_, LASTPAGE_-1, curPage_);

        if (pg->persistOption_) { //for HTTP1.1 persistent-connection

                mgr_->LASTFLOW_++;

	 	int wins = int(ceil(serverWin()->value()));
	    	int winc = int(ceil(clientWin()->value()));
	 	int window = (wins >= winc) ? wins : winc;

	 	int m = int(ceil(mtu()->value()));

	  	// Choose source and dest TCP agents for both source and destination
		if (fulltcp_) {
	  		ctcp_ = mgr_->picktcp(window,m);
	  		stcp_ = mgr_->picktcp(window,m);

			Tcl::instance().evalf("%s connect-full %s %s %s %s",
			        mgr_->name(),
				src_->name(), pg->dst()->name(),
			        ctcp_->name(),  stcp_->name());

                } else {
	  		ctcp_ = mgr_->picktcp(window,m);
	  		stcp_ = mgr_->picktcp(window,m);
	  		csnk_ = mgr_->picksink();
	  		ssnk_ = mgr_->picksink();
		}

//		Tcl::instance().evalf("%s set-fid %d %s %s",                             		mgr_->name(), mgr_->LASTFLOW_-1, ctcp_->name(), stcp_->name());
		Tcl::instance().evalf("%s set-fid %d %s %s",                             		mgr_->name(), mgr_->color_, ctcp_->name(), stcp_->name());

	}

	pg->start();
}

void EmpWebTrafSession::handle(Event *e)
{
	// If I haven't scheduled all my pages, do the next one
	TimerHandler::handle(e);
	++curPage_;
	// XXX Notice before each page is done, it will schedule itself 
	// one more time, this makes sure that this session will not be
	// deleted after the above call. Thus the following code will not
	// be executed in the context of a deleted object. 
	if (!interPageOption_) {
		if (curPage_ < nPage_) {
			sched(rvInterPage_->value());
			// printf("schedule: %g %d %d\n", Scheduler::instance().clock(), donePage_, curPage_);
		}
	}
}

// Launch a request for a particular object
void EmpWebTrafSession::launchReq(void* ClntData, int obj, int size, int reqSize, int , int persist)
{

  	TcpAgent* ctcp;
  	TcpAgent* stcp;
  	TcpSink* csnk;
  	TcpSink* ssnk;

	EmpWebPage* pg = (EmpWebPage*)ClntData;

        if (persist) { //for HTTP1.1 persistent-connection
		if (mgr_->isdebug()) {
			printf("HTTP1.1\n");
		}

		// use the same connection
		ctcp = ctcp_;
		stcp = stcp_;
		if (fulltcp_) {
			csnk = 0;
			ssnk = 0;
		} else {
	  		csnk = csnk_;
	  		ssnk = ssnk_;
		}

        } else { //for HTTP1.0 non-consistent connection
		if (mgr_->isdebug()) {
			printf("HTTP1.0\n");
		}
	
		mgr_->LASTFLOW_++;

	 	int wins = int(ceil(serverWin()->value()));
	    	int winc = int(ceil(clientWin()->value()));
	 	int window = (wins >= winc) ? wins : winc;

	 	int m = int(ceil(mtu()->value()));

	  	// Choose source and dest TCP agents for both source and destination

		ctcp = mgr_->picktcp(window,m);
		stcp = mgr_->picktcp(window,m);
		Tcl::instance().evalf("%s set-fid %d %s %s",
				      mgr_->name(), mgr_->color_, ctcp->name(), stcp->name());
		if (fulltcp_) {
			csnk = 0;
			ssnk = 0;
		} else {
	  		csnk = mgr_->picksink();
	  		ssnk = mgr_->picksink();
		}
	}

	// Setup new TCP connection and launch request
	// size and reqSize are in the unit of bytes in fulltcp mode
	// but in the unit of packet in halftcp mode
	if (fulltcp_) {
		Tcl::instance().evalf("%s launch-req-full %d %d %s %s %s %s %d %d %d %d",
                              mgr_->name(), obj, pg->id(), 
			      src_->name(), pg->dst()->name(),
			      ctcp->name(),  
			      stcp->name(),  
			      size, reqSize, ClntData,persist);
	} else {
		assert (csnk != 0 && ssnk != 0);
		Tcl::instance().evalf("%s launch-req %d %d %s %s %s %s %s %s %d %d %d %d",
                              mgr_->name(), obj, pg->id(), 
			      src_->name(), pg->dst()->name(),
			      ctcp->name(), csnk->name(), 
			      stcp->name(), ssnk->name(), 
			      size, reqSize, ClntData,
			      persist);
	}

	if (mgr_->isdebug()) {
	 	printf("size=%d  obj=%d  page=%d  sess=%d  %g src=%d dst=%d\n", size, obj, pg->id(), id_, Scheduler::instance().clock(), src_->address(), pg->dst()->address());
	}
}


static class EmpWebTrafPoolClass : public TclClass {
public:
        EmpWebTrafPoolClass() : TclClass("PagePool/EmpWebTraf") {}
        TclObject* create(int, const char*const*) {
		return (new EmpWebTrafPool());
	}
} class_empwebtrafpool;

EmpWebTrafPool::~EmpWebTrafPool()
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

void EmpWebTrafPool::delay_bind_init_all()
{
	delay_bind_init_one("debug_");
	PagePool::delay_bind_init_all();
}

int EmpWebTrafPool::delay_bind_dispatch(const char *varName,const char *localName,
				     TclObject *tracer)
{
	if (delay_bind_bool(varName, localName, "debug_", &debug_, tracer)) 
		return TCL_OK;
	return PagePool::delay_bind_dispatch(varName, localName, tracer);
}

EmpWebTrafPool::EmpWebTrafPool() : 
	concurrentSess_(0), nSrc_(0), server_(NULL), session_(NULL), nClient_(0), client_(NULL), nTcp_(0), nSink_(0), fulltcp_(0)
{
        bind("fulltcp_", &fulltcp_);

	LIST_INIT(&tcpPool_);
	LIST_INIT(&sinkPool_);
}

TcpAgent* EmpWebTrafPool::picktcp(int win, int mtu)
{


	TcpAgent* a = (TcpAgent*)detachHead(&tcpPool_);
	if (a == NULL) {
		Tcl& tcl = Tcl::instance();
		tcl.evalf("%s alloc-tcp %d %d", name(), win, mtu);
		a = (TcpAgent*)lookup_obj(tcl.result());
		if (a == NULL) {
			fprintf(stderr, "Failed to allocate a TCP agent\n");
			abort();
		}
	} else 
		nTcp_--;
	return a;
}

TcpSink* EmpWebTrafPool::picksink()
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

void EmpWebTrafPool::recycleTcp(Agent* a)
{
	if (fulltcp_) {
		delete a;
	} else {

		if (a == NULL) {
			fprintf(stderr, "Failed to recycle TCP agent\n");
			abort();
		}
		nTcp_++;
		insertAgent(&tcpPool_, a);
	}
}

void EmpWebTrafPool::recycleSink(Agent* a)
{
	if (fulltcp_) {
		delete a;
	} else {

		if (a == NULL) {
			fprintf(stderr, "Failed to recycle Sink agent\n");
			abort();
		}
		nSink_++;
		insertAgent(&sinkPool_, a);
	}
}

int EmpWebTrafPool::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "set-num-session") == 0) {
			if (session_ != NULL) {
				for (int i = 0; i < nSession_; i++) 
					delete session_[i];
				delete []session_;
			}
			nSession_ = atoi(argv[2]);
			session_ = new EmpWebTrafSession*[nSession_];
			memset(session_, 0, sizeof(EmpWebTrafSession*)*nSession_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "set-num-server-lan") == 0) {
			nSrcL_ = atoi(argv[2]);
			if (nSrcL_ >  nSrc_) {
				fprintf(stderr, "Wrong server index %d\n", nSrcL_);
				return TCL_ERROR;
			}
			return (TCL_OK);
		} else if (strcmp(argv[1], "set-num-remote-client") == 0) {
			nClientL_ = atoi(argv[2]);
			if (nClientL_ > nClient_) {
				fprintf(stderr, "Wrong client index %d\n", nClientL_);
				return TCL_ERROR;
			}
			return (TCL_OK);
		} else if (strcmp(argv[1], "set-num-server") == 0) {
			nSrc_ = atoi(argv[2]);
			if (server_ != NULL) 
				delete []server_;
			server_ = new Node*[nSrc_];
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
					EmpWebTrafSession* p = session_[i];
					p->set_interPageOption(option);
				}
			}
			return (TCL_OK);
		} else if (strcmp(argv[1], "doneObj") == 0) {
		        EmpWebPage* p = (EmpWebPage*)atol(argv[2]);
			
			p->doneObject();
                
			return (TCL_OK);
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "set-server") == 0) {
			Node* cli = (Node*)lookup_obj(argv[3]);
			if (cli == NULL)
				return (TCL_ERROR);
			int nc = atoi(argv[2]);
			if (nc >= nSrc_) {
				fprintf(stderr, "Wrong server index %d\n", nc);
				return TCL_ERROR;
			}
			server_[nc] = cli;
			return (TCL_OK);
		} else if (strcmp(argv[1], "set-client") == 0) {
			Node* s = (Node*)lookup_obj(argv[3]);
			if (s == NULL)
				return (TCL_ERROR);
			int n = atoi(argv[2]);
			if (n >= nClient_) {
				fprintf(stderr, "Wrong client index %d\n", n);
				return TCL_ERROR;
			}
			client_[n] = s;
			return (TCL_OK);
		} else if (strcmp(argv[1], "recycle") == 0) {
			// <obj> recycle <tcp> <sink>
			//
			// Recycle a TCP source/sink pair
			Agent* tcp = (Agent*)lookup_obj(argv[2]);
			Agent* snk = (Agent*)lookup_obj(argv[3]);
			if ((tcp == NULL) || (snk == NULL))
				return (TCL_ERROR);
			// XXX TBA: recycle tcp agents
			if (fulltcp_) {
				delete tcp;
				delete snk;
			} else {
				nTcp_++, nSink_++;
				insertAgent(&tcpPool_, tcp);
				insertAgent(&sinkPool_, snk);
			}
			return (TCL_OK);
		}
	} else if (argc == 17) {
		if (strcmp(argv[1], "create-session") == 0) {
			// <obj> create-session <session_index>
			//   <pages_per_sess> <launch_time>
			//   <inter_page_rv> <page_size_rv>
			//   <inter_obj_rv> <obj_size_rv>
			//   <req_size_rv> <persist_sel_rv> <server_sel_rv>
			//   <client_win_rv> <server_win_rv> <mtu_rv>
			//   <inbound/outbound flag> <color>
			int n = atoi(argv[2]);
			if ((n < 0)||(n >= nSession_)||(session_[n] != NULL)) {
				fprintf(stderr,"Invalid session index %d\n",n);
				return (TCL_ERROR);
			}
			int npg = (int)strtod(argv[3], NULL);
			double lt = strtod(argv[4], NULL);

			int flip = atoi(argv[15]);
			if ((flip < 0)||(flip > 1)) {
				fprintf(stderr,"Invalid I/O flag %d\n",flip);
				return (TCL_ERROR);
			}

                        //for SPAWAR demo
			color_ = atoi(argv[16]);

                        int cl;
			if (flip == 1) 
                          cl = int(floor(Random::uniform(0, nClientL_)));
			else
                          cl = int(floor(Random::uniform(nClientL_, nClient_)));

                        assert((cl >= 0) && (cl < nClient_));
                        Node* c=client_[cl];

			EmpWebTrafSession* p = 
				new EmpWebTrafSession(this, c, npg, n, nSrc_, cl,fulltcp_);

			int res = lookup_rv(p->interPage(), argv[5]);
			res = (res == TCL_OK) ? 
				lookup_rv(p->pageSize(), argv[6]) : TCL_ERROR;
			res = (res == TCL_OK) ? 
				lookup_rv(p->interObj(), argv[7]) : TCL_ERROR;
			res = (res == TCL_OK) ? 
				lookup_rv(p->objSize(), argv[8]) : TCL_ERROR;
			res = (res == TCL_OK) ? 
				lookup_rv(p->reqSize(), argv[9]) : TCL_ERROR;
			res = (res == TCL_OK) ? 
				lookup_rv(p->persistSel(), argv[10]) : TCL_ERROR;
			res = (res == TCL_OK) ? 
				lookup_rv(p->serverSel(), argv[11]) : TCL_ERROR;
			res = (res == TCL_OK) ? 
				lookup_rv(p->serverWin(), argv[12]) : TCL_ERROR;
			res = (res == TCL_OK) ? 
				lookup_rv(p->clientWin(), argv[13]) : TCL_ERROR;
			res = (res == TCL_OK) ? 
				lookup_rv(p->mtu(), argv[14]) : TCL_ERROR;
			if (res == TCL_ERROR) {
				delete p;
				fprintf(stderr, "Invalid random variable\n");
				return (TCL_ERROR);
			}
			p->sched(lt);
			session_[n] = p;

                           
			return (TCL_OK);
		}
	}
	return PagePool::command(argc, argv);
}



EmpWebTrafSession::EmpWebTrafSession(EmpWebTrafPool *mgr, Node *src, int np, int id, int , int cl, int ftcp_) : 
	rvInterPage_(NULL), rvPageSize_(NULL),
	rvInterObj_(NULL), rvObjSize_(NULL), 
	rvReqSize_(NULL), rvPersistSel_(NULL), rvServerSel_(NULL),
	rvServerWin_(NULL), rvClientWin_(NULL),
	rvMtu_(NULL),
	mgr_(mgr), src_(src), nPage_(np), curPage_(0), donePage_(0),
	id_(id), clientIdx_(cl), fulltcp_(0), interPageOption_(1)
{
	fulltcp_ = ftcp_;	
}
