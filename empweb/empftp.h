/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * empftp.h
 * Copyright (C) 2001 by the University of Southern California
 * $Id: empftp.h,v 1.5 2005/08/25 18:58:05 johnh Exp $
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
// Empirical FTP traffic model that simulates FTP traffic based on a set of
// CDF (Cumulative Distribution Function) data derived from live tcpdump trace
// The structure of this file is largely borrowed from empweb.h
//

#ifndef ns_empftp_h
#define ns_empftp_h

#include "ranvar.h"
#include "random.h"
#include "timer-handler.h"

#include "lib/bsd-list.h"
#include "node.h"
#include "tcp.h"
#include "tcp-sink.h"
#include "persconn.h"


class EmpFtpTrafPool;

class EmpFtpTrafSession : public TimerHandler {
public: 
	EmpFtpTrafSession(EmpFtpTrafPool *mgr, int np, int id) : 
		rvInterFile_(NULL), rvFileSize_(NULL),
		rvServerSel_(NULL), 
		rvServerWin_(NULL), rvClientWin_(NULL),
		mgr_(mgr), nFile_(np), curFile_(0), 
		id_(id) {}
	virtual ~EmpFtpTrafSession();

	// Queried by individual pages/objects
	inline EmpiricalRandomVariable*& interFile() { return rvInterFile_; }
	inline EmpiricalRandomVariable*& fileSize() { return rvFileSize_; }
	inline EmpiricalRandomVariable*& serverSel() { return rvServerSel_; }

	inline EmpiricalRandomVariable*& serverWin() { return rvServerWin_; }
	inline EmpiricalRandomVariable*& clientWin() { return rvClientWin_; }

	inline void setServer(Node* s) { src_ = s; }
	inline void setClient(Node* c) { dst_ = c; }

	void sendFile(int obj, int size);
	inline int id() const { return id_; }
	inline EmpFtpTrafPool* mgr() { return mgr_; }

	static int LASTFILE_;

private:
	virtual void expire(Event *e = 0);
	virtual void handle(Event *e);

	EmpiricalRandomVariable *rvInterFile_, *rvFileSize_, *rvServerSel_;
	EmpiricalRandomVariable *rvServerWin_, *rvClientWin_;

	EmpFtpTrafPool* mgr_;
	Node* src_;		
	Node* dst_;		
	int nFile_, curFile_ ;
	int id_;


};

class EmpFtpTrafPool : public PagePool {
public: 
	EmpFtpTrafPool(); 
	virtual ~EmpFtpTrafPool(); 

	inline void doneSession(int idx) { 

		assert((idx>=0) && (idx<nSession_) && (session_[idx]!=NULL));
		if (isdebug()) {
			printf("deleted session %d \n", idx );
                }
		delete session_[idx];
		session_[idx] = NULL; 
	}
	TcpAgent* picktcp(int size);
	TcpSink* picksink();
	inline int nTcp() { return nTcp_; }
	inline int nSink() { return nSink_; }
	inline int isdebug() { return debug_; }

	virtual void delay_bind_init_all();
	virtual int delay_bind_dispatch(const char*, const char*, TclObject*);

	int nSrcL_;
	int nClientL_;

        int color_;

	int nSrc_;
	Node** server_;		/* FTP servers */

protected:
	virtual int command(int argc, const char*const* argv);

	// Session management: fixed number of sessions, fixed number
	// of pages per session
	int nSession_;
	EmpFtpTrafSession** session_; 

	int nClient_;
	Node** client_; 	/* FTP clients */

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
};


#endif // ns_empftp_h
