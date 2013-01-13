/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * empweb.h
 * Copyright (C) 2001 by the University of Southern California
 * $Id: empweb.h,v 1.20 2010/03/08 05:54:50 tom_henderson Exp $
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
// The structure of this file is largely borrowed from webtraf.h
//
// $Header: /cvsroot/nsnam/ns-2/empweb/empweb.h,v 1.20 2010/03/08 05:54:50 tom_henderson Exp $

#ifndef ns_empweb_h
#define ns_empweb_h

#include "ranvar.h"
#include "random.h"
#include "timer-handler.h"

#include "lib/bsd-list.h"
#include "node.h"
#include "tcp.h"
#include "tcp-sink.h"
#include "persconn.h"

const int WEBTRAF_DEFAULT_OBJ_PER_PAGE = 1;

class EmpWebTrafPool;

class EmpWebTrafSession : public TimerHandler {
public: 
	EmpWebTrafSession(EmpWebTrafPool *mgr, Node *src, int np, int id, int connNum, int cl, int ftcp_);
	virtual ~EmpWebTrafSession();

	// Queried by individual pages/objects
	inline EmpiricalRandomVariable*& interPage() { return rvInterPage_; }
	inline EmpiricalRandomVariable*& pageSize() { return rvPageSize_; }
	inline EmpiricalRandomVariable*& interObj() { return rvInterObj_; }
	inline EmpiricalRandomVariable*& objSize() { return rvObjSize_; }

	inline EmpiricalRandomVariable*& reqSize() { return rvReqSize_; }
	inline EmpiricalRandomVariable*& persistSel() { return rvPersistSel_; }
	inline EmpiricalRandomVariable*& serverSel() { return rvServerSel_; }

	inline EmpiricalRandomVariable*& serverWin() { return rvServerWin_; }
	inline EmpiricalRandomVariable*& clientWin() { return rvClientWin_; }

	inline EmpiricalRandomVariable*& mtu() { return rvMtu_; }

	void donePage(void* ClntData);
	void launchReq(void* ClntData, int obj, int size, int reqSize, int sid, int p);
	inline int id() const { return id_; }
	inline EmpWebTrafPool* mgr() { return mgr_; }

        inline void set_interPageOption(int option) { interPageOption_ = option; }
	 
	static int LASTPAGE_;

private:
	virtual void expire(Event *e = 0);
	virtual void handle(Event *e);

	EmpiricalRandomVariable *rvInterPage_, *rvPageSize_, *rvInterObj_, *rvObjSize_;
	EmpiricalRandomVariable *rvReqSize_, *rvPersistSel_, *rvServerSel_;
	EmpiricalRandomVariable *rvServerWin_, *rvClientWin_;
	EmpiricalRandomVariable *rvMtu_;
	EmpWebTrafPool* mgr_;
	Node* src_;		// One Web client (source of request) per session
	int nPage_, curPage_, donePage_;
	int id_;


        int clientIdx_;

        int fulltcp_;

        int interPageOption_;

	TcpAgent* ctcp_;
        TcpAgent* stcp_;
	TcpSink* csnk_;
	TcpSink* ssnk_;

};

class EmpWebTrafPool : public PagePool {
public: 
	EmpWebTrafPool(); 
	virtual ~EmpWebTrafPool(); 

	inline void startSession() {
	       	concurrentSess_++;
		if (isdebug()) 
			printf("concurrent number of sessions = %d \n", concurrentSess_ );
	}
	inline void doneSession(int idx) { 

		assert((idx>=0) && (idx<nSession_) && (session_[idx]!=NULL));
		if (concurrentSess_ > 0) concurrentSess_--;
		if (isdebug()) {
			printf("deleted session %d \n", idx );
			printf("concurrent number of sessions = %d \n", concurrentSess_ );
                }
		delete session_[idx];
		session_[idx] = NULL; 
	}
	void recycleTcp(Agent* a);
	void recycleSink(Agent* a);
	TcpAgent* picktcp(int size, int mtu);
	TcpSink* picksink();
	inline int nTcp() { return nTcp_; }
	inline int nSink() { return nSink_; }
	inline int isdebug() { return debug_; }

	virtual void delay_bind_init_all();
	virtual int delay_bind_dispatch(const char*, const char*, TclObject*);

	int nSrcL_;
	int nClientL_;

	int concurrentSess_;

 	int color_;

	static int LASTFLOW_;
	int nSrc_;
	Node** server_;		/* Web servers */

protected:
	virtual int command(int argc, const char*const* argv);

	// Session management: fixed number of sessions, fixed number
	// of pages per session
	int nSession_;
	EmpWebTrafSession** session_; 

	int nClient_;
	Node** client_; 	/* Browsers */

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
	AgentList tcpPool_;	/* TCP agent pool */
	AgentList sinkPool_;	/* TCP sink pool */

	// Helper methods
	inline int lookup_rv(EmpiricalRandomVariable*& rv, const char* name) {
		if (rv != NULL)
			Tcl::instance().evalf("delete %s", rv->name());
		rv = (EmpiricalRandomVariable*)lookup_obj(name);
		return rv ? (TCL_OK) : (TCL_ERROR);
	}

	int debug_;

	int fulltcp_;
};


#endif // ns_empweb_h
