/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * webtraf.h
 * Copyright (C) 1999 by the University of Southern California
 * $Id: webtraf.h,v 1.19 2011/10/01 22:00:14 tom_henderson Exp $
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
// Incorporating Polly's web traffic module into the PagePool framework
//
// XXX This has nothing to do with the HttpApp classes. Because we are 
// only interested in traffic pattern here, we do not want to be bothered 
// with the burden of transmitting HTTP headers, etc. 
//
// $Header: /cvsroot/nsnam/ns-2/webcache/webtraf.h,v 1.19 2011/10/01 22:00:14 tom_henderson Exp $

#ifndef ns_webtraf_h
#define ns_webtraf_h

#include <map>
#include "ranvar.h"
#include "random.h"
#include "timer-handler.h"

#include "lib/bsd-list.h"
#include "node.h"
#include "tcp.h"
#include "tcp-sink.h"
#include "pagepool.h"
#include "webserver.h"

const int WEBTRAF_DEFAULT_OBJ_PER_PAGE = 1;

class WebTrafPool;

class WebTrafSession : public TimerHandler {
public: 
	WebTrafSession(WebTrafPool *mgr, Node *src, int np, int id, int ftcp_, int recycle_p);
	virtual ~WebTrafSession();

	// Queried by individual pages/objects
	inline RandomVariable*& interPage() { return rvInterPage_; }
	inline RandomVariable*& pageSize() { return rvPageSize_; }
	inline RandomVariable*& interObj() { return rvInterObj_; }
	inline RandomVariable*& objSize() { return rvObjSize_; }

	void donePage(void* ClntData);
	void launchReq(void* ClntData, int obj, int size);
	inline int id() const { return id_; }
	inline WebTrafPool* mgr() { return mgr_; }
	inline void set_interPageOption(int option) { interPageOption_ = option; }

	static int LASTPAGE_;

private:
	virtual void expire(Event *e = 0);
	virtual void handle(Event *e);

	RandomVariable *rvInterPage_, *rvPageSize_, *rvInterObj_, *rvObjSize_;
	WebTrafPool* mgr_;
	Node* src_;		// One Web client (source of request) per session
	int nPage_, curPage_, donePage_;
	int id_, interPageOption_;
	
	// fulltcp support
	int fulltcp_;
	// Reuse of page level attributes support
	int recycle_page_;
};

class WebServer;
class WebTrafPool : public PagePool {
public: 
	WebTrafPool(); 
	virtual ~WebTrafPool(); 

	Node* picksrc();
	Node* pickdst();
	inline void doneSession(int idx) { 
		assert((idx>=0) && (idx<nSession_) && (session_[idx]!=NULL));
		if (isdebug())
			printf("deleted session %d\n", idx);
		delete session_[idx];
		session_[idx] = NULL; 
	}

	void launchReq(Node*, void*, int, int);
	void launchResp(int, Node*, Node*, Agent*, Agent*, int, int);
	inline int nTcp() { return nTcp_; }
	inline int nSink() { return nSink_; }
	inline int isdebug() { return debug_; }

	virtual void delay_bind_init_all();
	virtual int delay_bind_dispatch(const char*, const char*, TclObject*);

	// pick end points for a new TCP connection
	void pick_ep(TcpAgent**, Agent**);
	TcpAgent* picktcp();
	TcpSink* picksink();
	// Given sever's node id, find server
	int find_server(int);
	void donePage (int);

protected:
	virtual int command(int argc, const char*const* argv);

	

	// Session management: fixed number of sessions, fixed number
	// of pages per session
	int nSession_;
	WebTrafSession** session_; 

	// Web servers
	int nServer_;
	//Node** server_;
	// keep the finish time of the last job (M/G/1)
	WebServer *server_;

	// Browsers
	int nClient_;
	Node** client_;

	// Debo added this
	int asimflag_;

	// TCP agent pool management
	struct AgentListElem {
		AgentListElem(Agent* a) : agt_(a) {
			link.le_next = NULL;
			link.le_prev = NULL;
		}
		Agent* agt_;
		LIST_ENTRY(AgentListElem) link;
	};
	LIST_HEAD(AgentList, AgentListElem);
	inline void insertAgent(AgentList* l, Agent *a) {
		AgentListElem *e = new AgentListElem(a);
		LIST_INSERT_HEAD(l, e, link);
	}
	inline Agent* detachHead(AgentList* l) {
		AgentListElem *e = l->lh_first;
		if (e == NULL)
			return NULL;
		Agent *a = e->agt_;
		LIST_REMOVE(e, link);
		delete e;
		return a;
	}
	int nTcp_, nSink_;
	int dbTcp_a, dbTcp_r, dbTcp_cr;
	AgentList tcpPool_;	/* TCP agent pool */
	AgentList sinkPool_;	/* TCP sink pool */

	// Helper methods
	inline int lookup_rv(RandomVariable*& rv, const char* name) {
		if (rv != NULL)
			Tcl::instance().evalf("delete %s", rv->name());
		rv = (RandomVariable*)lookup_obj(name);
		return rv ? (TCL_OK) : (TCL_ERROR);
	}
	int debug_;
	// fulltcp support
	int fulltcp_;
	// Reuse of page level attributes support
	int recycle_page_;

	int dont_recycle_; // Do not recycle tcp agents

	std::map<int, void *> pages_;  // Hold pointers to web pages
};

#endif // ns_webtraf_h


