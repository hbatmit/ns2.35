/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * Copyright (C) 1997 by the University of Southern California
 * $Id: sessionhelper.cc,v 1.22 2005/09/18 23:33:31 tomh Exp $
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
/* 
 * Contributed by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
 *
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/common/sessionhelper.cc,v 1.22 2005/09/18 23:33:31 tomh Exp $ (USC/ISI)";
#endif

#include "config.h"
#include "tclcl.h"
#include "ip.h"
#include "packet.h"
#include "connector.h"
#include "errmodel.h"

//Definitions for special reference count events
class RcEvent : public Event {
public:
	Packet* packet_;
	Handler* real_handler_;
};

class RcHandler : public Handler {
public:
	void handle(Event* event);
} rc_handler;

void RcHandler::handle(Event* e)
{
	RcEvent* rc = (RcEvent*)e;
	rc->real_handler_->handle(rc->packet_);
	delete rc;
}

struct dstobj {
	double bw;
	double delay;
	double prev_arrival;
	int ttl;
	int dropped;
	nsaddr_t addr;
	NsObject *obj;
	dstobj *next;
};

struct rcv_depobj {
	dstobj *obj;
	rcv_depobj *next;
};


struct loss_depobj {
	ErrorModel *obj;
	loss_depobj *loss_dep;
	rcv_depobj *rcv_dep;
	loss_depobj *next;
};

class SessionHelper : public Connector {
public:
	SessionHelper();
	int command(int, const char*const*);
	void recv(Packet*, Handler*);
protected:
	void get_dropped(loss_depobj*, Packet*);
	void mark_dropped(loss_depobj*);
	void clear_dropped();
	dstobj* find_dstobj(NsObject*);
	void delete_dstobj(NsObject*);
	loss_depobj* find_loss_depobj(ErrorModel*);
	void show_dstobj();
	void show_loss_depobj(loss_depobj*);
	nsaddr_t src_;
	dstobj *dstobj_;
	loss_depobj *loss_dependency_;
	int ndst_;
	int rc_; //enable reference count
};

static class SessionHelperClass : public TclClass {
public:
	SessionHelperClass() : TclClass("SessionHelper") {}
	TclObject* create(int, const char*const*) {
		return (new SessionHelper());
	}
} class_sessionhelper;

SessionHelper::SessionHelper() : dstobj_(0), ndst_(0), rc_(0)
{
	bind("rc_", &rc_);
	loss_dependency_ = new loss_depobj;
	loss_dependency_->obj = 0;
	loss_dependency_->loss_dep = 0;
	loss_dependency_->rcv_dep = 0;
	loss_dependency_->next = 0;
}

void SessionHelper::recv(Packet* pkt, Handler*)
{
	Scheduler& s = Scheduler::instance();
	hdr_cmn* th = hdr_cmn::access(pkt);
	hdr_ip* iph = hdr_ip::access(pkt);
	double tmp_arrival;

	//printf ("src %d,  size %d, iface %d\n", src_, th->size(), th->iface());
	clear_dropped();

	get_dropped(loss_dependency_->loss_dep, pkt);

	for (dstobj *tmpdst = dstobj_; tmpdst; tmpdst = tmpdst->next) {
		int ttl;
		if (tmpdst->dropped 
		    || (ttl = iph->ttl() - tmpdst->ttl) <= 0)
			continue;
		
		if (tmpdst->bw == 0) {
			tmp_arrival = tmpdst->delay;
		} else {
			tmp_arrival = th->size()*8/tmpdst->bw + tmpdst->delay;
		}
		if (tmpdst->prev_arrival >= tmp_arrival) {
			tmp_arrival = tmpdst->prev_arrival + 0.000001;
			/* Assume 1 ns process delay; just to maintain the causality */
		}
		tmpdst->prev_arrival = tmp_arrival;
		if (rc_) {
			// reference count
			//s.rc_schedule(tmpdst->obj, pkt, tmp_arrival);
			RcEvent* rc = new RcEvent;
			rc->packet_ = pkt->refcopy();
			rc->real_handler_ = tmpdst->obj;
			s.schedule(&rc_handler, rc, tmp_arrival);
		} else {
			Packet* tmppkt = pkt->copy();
			hdr_ip* tmpiph = hdr_ip::access(tmppkt);
			tmpiph->ttl() = ttl;
			s.schedule(tmpdst->obj, tmppkt, tmp_arrival);
		}
	}
	
	Packet::free(pkt);
}

void SessionHelper::get_dropped(loss_depobj* loss_dep, Packet* pkt)
{
	if (loss_dep != 0) 
		if (loss_dep->obj != 0) {
			if (loss_dep->obj->corrupt(pkt)) {
				mark_dropped(loss_dep);
			} else {
				get_dropped(loss_dep->loss_dep, pkt);
			}
			get_dropped(loss_dep->next, pkt);
		}
}

void SessionHelper::mark_dropped(loss_depobj* loss_dep)
{
	if (loss_dep != 0) {
		rcv_depobj *tmprcv_dep = loss_dep->rcv_dep;
		loss_depobj *tmploss_dep = loss_dep->loss_dep;

		while (tmprcv_dep != 0) {
			tmprcv_dep->obj->dropped = 1;
			tmprcv_dep = tmprcv_dep->next;
		}

		while (tmploss_dep != 0) {
			mark_dropped(tmploss_dep);
			tmploss_dep = tmploss_dep->next;
		}
	}
}

void SessionHelper::clear_dropped()
{
	dstobj *tmpdst = dstobj_;
	while (tmpdst != 0) {
		tmpdst->dropped = 0;
		tmpdst = tmpdst->next;
	}
}

dstobj* SessionHelper::find_dstobj(NsObject* obj) {
	dstobj *tmpdst = dstobj_;
	while (tmpdst != 0) {
		if (tmpdst->obj == obj) return (tmpdst);
		tmpdst = tmpdst->next;
	}
	return 0;
}

loss_depobj* SessionHelper::find_loss_depobj(ErrorModel* err) {
	struct stackobj {
		loss_depobj *loss_obj;
		stackobj *next;
	};

	if (!loss_dependency_) return 0;

	stackobj *top = new stackobj;
	top->loss_obj = loss_dependency_;
	top->next = 0;

	while (top != 0) {
		if (top->loss_obj->obj == err) {
			loss_depobj *tmp_loss_obj = top->loss_obj;
			while (top != 0) {
				stackobj *befreed = top;
				top = top->next;
				free(befreed);
			}
			return (tmp_loss_obj);
		}
		loss_depobj *tmploss = top->loss_obj->loss_dep;
		stackobj *befreed = top;
		top = top->next;
		free(befreed);
		while (tmploss != 0) {
			stackobj *new_element = new stackobj;
			new_element->loss_obj = tmploss;
			new_element->next = top;
			top = new_element;
			tmploss = tmploss->next;
		}
	}
	return 0;
}

void SessionHelper::show_dstobj() {
	dstobj *tmpdst = dstobj_;
	while (tmpdst != 0) {
		printf("bw:%.2f, delay:%.2f, ttl:%d, dropped:%d, addr:%d, obj:%s\n", tmpdst->bw, tmpdst->delay, tmpdst->ttl, tmpdst->dropped, tmpdst->addr, tmpdst->obj->name());
		tmpdst = tmpdst->next;
	}
}

void SessionHelper::delete_dstobj(NsObject *obj) {
	dstobj *tmpdst = dstobj_;
	dstobj *tmpprev = 0;

	while (tmpdst != 0) {
		if (tmpdst->obj == obj) {
			if (tmpprev == 0) dstobj_ = tmpdst->next;
			else tmpprev->next = tmpdst->next;
			free(tmpdst);
			return;
		}
		tmpprev = tmpdst;
		tmpdst = tmpdst->next;
	}
}

void SessionHelper::show_loss_depobj(loss_depobj *loss_obj) {

	loss_depobj *tmploss = loss_obj->loss_dep;
	rcv_depobj *tmprcv = loss_obj->rcv_dep;

	while (tmprcv != 0) {
		printf("%d ", tmprcv->obj->addr);
		tmprcv = tmprcv->next;
	}
	while (tmploss != 0) {
		printf("(%s: ", tmploss->obj->name());
		show_loss_depobj(tmploss);
		tmploss = tmploss->next;
	}

	if (loss_obj == loss_dependency_) {
		printf("\n");
	} else {
		printf(")");
	}
}


int SessionHelper::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "list-mbr") == 0) {
			dstobj *tmp = dstobj_;
			while (tmp != 0) {
				tcl.resultf("%s %s", tcl.result(),
					    tmp->obj->name());
				tmp = tmp->next;
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "show-loss-depobj") == 0) {
			show_loss_depobj(loss_dependency_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "show-dstobj") == 0) {
			show_dstobj();
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "set-node") == 0) {
			int src = atoi(argv[2]);
			src_ = src;
			//printf("set node %d\n", src_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "update-loss-top") == 0) {
			loss_depobj *tmploss = (loss_depobj*)(atol(argv[2]));
			tmploss->next = loss_dependency_->loss_dep;
			loss_dependency_->loss_dep = tmploss;
			return (TCL_OK);
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "update-loss-rcv") == 0) {
			ErrorModel *tmperr = (ErrorModel*)TclObject::lookup(argv[2]);
			NsObject *tmpobj = (NsObject*)TclObject::lookup(argv[3]);
			//printf("errmodel %s, agent %s\n", tmperr->name(), tmpobj->name());
			loss_depobj *tmploss = find_loss_depobj(tmperr);
			//printf ("%d, loss_dependency_ %d\n", tmploss, loss_dependency_);
			if (!tmploss) {
				tmploss = new loss_depobj;
				tmploss->obj = tmperr;
				tmploss->next = 0;
				tmploss->rcv_dep = 0;
				tmploss->loss_dep = 0;
				tcl.resultf("%ld", tmploss);
			} else {
				tcl.result("0");
			}
			rcv_depobj *tmprcv = new rcv_depobj;
			tmprcv->obj = find_dstobj(tmpobj);
			tmprcv->next = tmploss->rcv_dep;
			tmploss->rcv_dep = tmprcv;
			return (TCL_OK);
		}
		if (strcmp(argv[1], "update-loss-loss") == 0) {
			ErrorModel *tmperrparent = (ErrorModel*)TclObject::lookup(argv[2]);
			loss_depobj *tmplossparent = find_loss_depobj(tmperrparent);
			loss_depobj *tmplosschild = (loss_depobj*)(atol(argv[3]));
			if (!tmplossparent) {
				tmplossparent = new loss_depobj;
				tmplossparent->obj = tmperrparent;
				tmplossparent->next = 0;
				tmplossparent->loss_dep = 0;
				tmplossparent->rcv_dep = 0;
				tcl.resultf("%ld", tmplossparent);
			} else {
				tcl.result("0");
			}
			tmplosschild->next = tmplossparent->loss_dep;
			tmplossparent->loss_dep = tmplosschild;
			return (TCL_OK);
		}
		if (strcmp(argv[1], "delete-dst") == 0) {
			int tmpaddr = atoi(argv[2]);
			//NsObject *tmpobj = (NsObject*)TclObject::lookup(argv[3]);
			printf ("addr = %d\n", tmpaddr);
			return (TCL_OK);
		}
	} else if (argc == 7) {
		if (strcmp(argv[1], "add-dst") == 0) {
			dstobj *tmp = new dstobj;
			tmp->bw = atof(argv[2]);
			tmp->delay = atof(argv[3]);
			tmp->prev_arrival = 0;
			tmp->ttl = atoi(argv[4]);
			tmp->addr = atoi(argv[5]);
			tmp->obj = (NsObject*)TclObject::lookup(argv[6]);
			//printf ("addr = %d, argv3 = %s, obj = %d, ttl=%d\n", tmp->addr, argv[3], tmp->obj, tmp->ttl);
			tmp->next = dstobj_;
			dstobj_ = tmp;
			ndst_ += 1;
			return (TCL_OK);
		}
	}
	return (Connector::command(argc, argv));
}

