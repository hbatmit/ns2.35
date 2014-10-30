/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *  
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linking this file statically or dynamically with other modules is making
 * a combined work based on this file.  Thus, the terms and conditions of
 * the GNU General Public License cover the whole combination.
 *
 * In addition, as a special exception, the copyright holders of this file
 * give you permission to combine this file with free software programs or
 * libraries that are released under the GNU LGPL and with code included in
 * the standard release of ns-2 under the Apache 2.0 license or under
 * otherwise-compatible licenses with advertising requirements (or modified
 * versions of such code, with unchanged license).  You may copy and
 * distribute such a system following the terms of the GNU GPL for this
 * file and the licenses of the other code concerned, provided that you
 * include the source code of that other code when and as the GNU GPL
 * requires distribution of source code.
 *
 * Note that people who make modified versions of this file are not
 * obligated to grant this special exception for their modified versions;
 * it is their choice whether to do so.  The GNU General Public License
 * gives permission to release a modified version without this exception;
 * this exception also makes it possible to release a modified version
 * which carries forward this exception.
 *
 * This file contributed by Suchitra Raman <sraman@parc.xerox.com>, June 1997.
 */

#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "config.h"
#include "tclcl.h"
#include "srm-topo.h"
#include "scheduler.h"

#define SRM_DEBUG


Topology::Topology(int nn, int src) : idx_(nn), src_(src)
{
	node_ = new SrmNode[nn];
	for (int i = 0; i < nn; i ++) 
		node_[i].id(i);

	bind("src_", &src_);
	bind("delay_", &delay_);
	bind("D_", &D_);
	bind("frac_", &frac_);
	/* D_ (ms) is the delay of node 1 from node 0 */

	bind("det_", &det_);
	bind("rtt_est_", &rtt_est_);
	bind("rand_", &rand_);
}

Topology::~Topology() {
	delete [] node_;
}

int Topology::command(int argc, const char*const* argv) 
{
	//Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "flood") == 0) {
			flood(0, atoi(argv[2]));
			return (TCL_OK);
		}
	}
	return (TclObject::command(argc, argv));
}

SrmNode* Topology::node(int nn) 
{ 
	if (nn < 0 || nn >= idx_) return 0;
	return &(node_[nn]); 
}


static class LineClass : public TclClass {
public:
	LineClass() : TclClass("Topology/Line") {} 
	TclObject* create(int, const char*const* argv) {
		int nn = atoi(argv[4]);
		return (new Line(nn, 0));
	}
} class_line_topo;


double Line::backoff(int dst) 
{
	double rtt;

	if (topology->rtt_estimated() == 1)
		rtt = delay(dst, 0);
	else rtt = D_ * frac_;
	double b;
	double sqroot = sqrt(rtt);
	double r = Random::uniform(0.0, 1.0);

	switch(c2func_) {
	case LOG :
		b = rtt * (det_ +  rand_ * r * log(rtt)/log(D_));
		break;
	case SQRT :
		b = rtt * (det_ +  rand_ * r * sqroot/sqrt(D_));
		break;
	case LINEAR :
		b = rtt * (det_ +  rand_ * r * rtt/D_);
		break;
	case CONSTANT :
		b = rtt * (det_ +  rand_ * r * 1.);
		break;
	default:
		// quiet compiler
		b = 0.0;
		assert (false);
		break;
	}
	return b;
}

#ifdef SRM_BIMODAL
double Line::backoff(int dst) 
{
	double rtt;
	if (topology->rtt_estimated() == 1) 
		rtt = delay(dst, 0);
	else rtt = D_ * frac_;

	double rbackoff;
	double p = Random::uniform(0.0, 1.0);
	int bin = 0;
	int copies = c_;
	int size = topology->idx();
	
	if (p <= copies * 1./size) {
		rbackoff = Random::uniform(0.0, alpha_);
		bin = 0;
	} else {
		rbackoff = Random::uniform(beta_, 1.0 + beta_);
		bin = 1;
	}

	return (rtt * (det_ + rand_ * rbackoff));
}
#endif

Interface_List* Line::oif(int node, int iif=SRM_NOIF)
{
	//int oif;
	Interface_List *ilist = new Interface_List;

	/* If there are no more nodes downstream, return -1 */
	if ((iif <= node && node >= idx_ - 1) ||  (iif > node && node == 0))
		return 0;

	if (iif == SRM_NOIF) 
	{
		ilist->append(node + 1);
		ilist->append(node - 1);
	} else if (iif <= node) 
		ilist->append(node + 1);
	else 
		ilist->append(node - 1);
	return (ilist);
}

double Line::delay(int src, int dst) 
{
	if (src == 0 || dst == 0) 
		return D_ + delay_ * abs(dst - src - 1);
	else
		return delay_ * abs(dst - src);
}


double Star::delay(int src, int dst) 
{
	if (src == 0 || dst == 0) 
		return D_;
	else
		return delay_;
}

/* 
 * Very simple now, can only send to direct ancestor/descendant 
 */
double BTree::delay(int src, int dst)
{
	double src_ht = 0;
	double dst_ht = 0;
	
	if (src > 0) 
		src_ht = 1 + floor(log(src)/log(2));
	if (dst > 0)
		dst_ht = 1 + floor(log(dst)/log(2));
	
	if (src == 0 || dst == 0) 
		return D_ + delay_ * abs(int (dst_ht - src_ht - 1));
	else 
		return delay_ * abs(int (dst_ht - src_ht));
}	

/* 
 * There is always an outbound interface. We should not 
 * start a flood() from a leaf node. 
 * Change it for other topologies. 
 */
void Line::flood(int start, int seqno)
{
	SRM_Event *se = new SRM_Event(seqno, SRM_DATA, start);
	node_[start].send(se);
}

/*
 * BTree -
 */

static class BTreeClass : public TclClass {
public:
	BTreeClass() : TclClass("Topology/BTree") {} 
	TclObject* create(int, const char*const* argv) {
		int nn = atoi(argv[4]);
		return (new BTree(nn, 0));
	}
} class_btree_topo;


/* 
 * The ranges of the different distributions 
 * must be normalized to 1. 
 */
double BTree::backoff(int id) 
{
	double rtt;
	if (topology->rtt_estimated())
		rtt = delay(id, 0);
	else rtt = D_ * frac_; 

	double r = Random::uniform(0.0, 1.0);
	double b;
	double sqroot = sqrt(rtt);
	double D = topology->D();

	switch(c2func_) {
	case LOG :
		b = rtt * (det_ +  rand_ * r * log(rtt)/log(D));
		break;
	case SQRT :
		b = rtt * (det_ +  rand_ * r * sqroot/sqrt(D));
		break;
	case LINEAR :
		b = rtt * (det_ +  rand_ * r * rtt/D);
		break;
	case CONSTANT :
		b = rtt * (det_ +  rand_ * r * 1.);
		break;
	default:
		// quiet compiler
		b = 0.0;
		assert (false);
		break;
	}
	return b;
}

Interface_List* BTree::oif(int node, int iif=SRM_NOIF)
{
	//int oif;
	Interface_List *ilist = new Interface_List;
	/* 
	 * If the packet comes from the source, 
	 * there is only 1 outgoing link.
	 * We make the assumption that source = 0 always (YUCK!)
	 */
	if (iif == SRM_NOIF) {
		if (node == 0) {
			ilist->append(1);
		}
		else {
			ilist->append(2 * node + 1);
			ilist->append(2 * node);
			int k = (int)floor(.5 * node);
			ilist->append(k);
		}
	} else if (iif <= node) {
		ilist->append(2 * node + 1);
		ilist->append(2 * node);
	}

	// Packet came along 2n + 1 or 2n + 2
	else if (iif == 2 * node + 1) {
		if (node == 0)
			return ilist;
		
		ilist->append(2 * node);
		ilist->append((int) floor(.5 * node));
	} else if (iif == 2 * node) {
		ilist->append(2 * node + 1);
		ilist->append((int)floor(.5 * node));
	}
	return (ilist);
}

/*
 * Star -
 */

static class StarClass : public TclClass {
public:
	StarClass() : TclClass("Topology/Star") {} 
	TclObject* create(int, const char*const* argv) {
		int nn = atoi(argv[4]);
		return (new Star(nn, 0));
	}
} class_star_topo;


/* 
 * SrmNode -
 */

void SrmNode::dump_packet(SRM_Event *e) 
{
#ifdef SRM_DEBUG
	tprintf(("(type %d) (in %d) -- @ %d --> \n", e->type(), e->iif(), id_));
#endif
}

/* This forwards an event by making multiple copies of the
 * event 'e'. Does NOT free event 'e'. 
 */
void SrmNode::send(SRM_Event *e) 
{
	/* 
	 * Copy the packet and send it over to 
	 * all the outbound interfaces 
	 */
	int nn;
	Scheduler& s = Scheduler::instance();
	SRM_Event *copy;
	Interface *p;
	Interface_List *ilist = topology->oif(id_, e->iif());
	if (e->iif() < 0)
		e->iif(id_);

	if (ilist) {
		for (p=ilist->head_; p; p=p->next_) {
			nn = p->in_;
			int t = e->type();
			//int i = id_; 
			int snum = e->seqno();
			SrmNode *next = topology->node(nn);
			if (next) {
				copy = new SRM_Event(snum, t, id_);
				s.schedule(next, copy,
					   topology->delay(id_, nn));	 
			}
		}
	}
	delete ilist;
	delete e;
}


/* 
 * Demux the two types of events depending on 
 * the type field.
 */ 
void SrmNode::handle(Event* event)
{
	SRM_Event *srm_event = (SRM_Event *) event;
	int type  = srm_event->type();
	int seqno = srm_event->seqno();
	//int iif   = srm_event->iif();
	//Scheduler& s = Scheduler::instance();

	switch (type) {
	case SRM_DATA :
		if (seqno != expected_) 
			sched_nack(seqno);
		expected_ = seqno;
		break;

	case SRM_PENDING_RREQ :
		tprintf(("Fired RREQ (node %d)\n", id_));
		remove(seqno, SRM_NO_SUPPRESS);
		srm_event->type(SRM_RREQ);
		break;

	case SRM_RREQ :

		remove(seqno, SRM_SUPPRESS);
		break;

	default :
		tprintf(("panic: node(%d) Unexpected type %d\n", id_, type));
		return;
	}
#ifdef SRM_STAR
	if (type == SRM_RREQ || type == SRM_DATA) {
		delete srm_event;
		return;
	}
#endif
	send(srm_event);	
	return;
}

/* 
 * XXX Should take two sequence numbers. The iif field is a dummy. We should be
 * careful not to use it for NACKs 
 */
void SrmNode::sched_nack(int seqno) 
{
	double backoff;
	Scheduler& s = Scheduler::instance();
	SRM_Event *event = new SRM_Event(seqno, 
					 SRM_PENDING_RREQ, SRM_NOIF);
	append(event);

	backoff = topology->backoff(id_);
//	tprintf(("node(%d) schd rrq after %f s\n", id_, backoff));
	s.schedule(this, event, backoff);
}

void SrmNode::append(SRM_Event *e) 
{
	SRM_Request *req = new SRM_Request(e);
	req->next_ = pending_;
	pending_ = req;
}

/* If an event identical to e exists, cancel it */
void SrmNode::remove(int seqno, int flag)
{
	SRM_Request *curr, *prev;
	SRM_Event *ev;

	if (!pending_)
		return;
	for (curr=pending_, prev=0; curr; curr=curr->next_) 
	{
		ev = curr->event_;
		if (ev->seqno() == seqno) {
			if (!prev) 
				pending_ = curr->next_;
			else {
				prev->next_ = curr->next_;
				prev = curr;
			}
			if (flag == SRM_SUPPRESS) {
				curr->cancel_timer();
			}
			delete curr;
		}
	}
}


SRM_Event::SRM_Event(SRM_Event *e) 
{
	seqno_ = e->seqno();
	type_  = e->type();
	iif_   = e->iif();
}

SRM_Request::~SRM_Request() {}

void SRM_Request::cancel_timer() 
{
	if (event_) {
		Scheduler& s = Scheduler::instance();
		s.cancel(event_);
		// xxx: should event be free'd?  possible memory leak.
	}
}

void Interface_List::append(int in) 
{
	Interface *i = new Interface(in);
	i->next_ = head_;
	head_ = i;
}

Interface_List::~Interface_List() 
{
	Interface *p, *next;
	for (p=head_; p; p=next)
	{
		next = p->next_;
		delete p;
	}
}

void BTree::flood(int start, int seqno)
{
	SRM_Event *se = new SRM_Event(seqno, SRM_DATA, SRM_NOIF);
	node_[start].send(se);
}

void Star::flood(int start, int seqno)
{
	SRM_Event *se = new SRM_Event(seqno, SRM_DATA, start);
	node_[start].send(se);
}

Interface_List* Star::oif(int node, int iif=SRM_NOIF)
{
	int i;
	Interface_List *ilist = new Interface_List;

	/* Make a list with every node except myself */
	for (i = 0; i < topology->idx(); i ++)
	{
		if (i != node && i != iif) {
			ilist->append(i);
		}
	}

	return (ilist);
}

/* 
 * Distance of source to cluster = D_
 */
double Star::backoff(int id)
{
	double rtt = topology->delay(id, 0);
	double rbackoff;
	double p = Random::uniform(0.0, 1.0);
	static int bin = 0;
	int copies = c_;
	int size = topology->idx();
	
	if (p <= copies * 1./size) {
		rbackoff = Random::uniform(0.0, alpha_);
		bin ++;
	} else {
		rbackoff = Random::uniform(beta_, 1.0 + beta_);
	}
	return (rtt * (det_ + rand_ * rbackoff));
}


#ifdef SRM_BIMODAL
double BTree::backoff(int dst) 
{
	double height = floor(log(dst)/log(2));

	double D = topology->D();
	double rtt = delay_ * height + D;
	double p = Random::uniform(0.0, 1.0);
	double rbackoff;

	static int bin = 0;
	int copies = c_;
	int size = topology->idx();
	
	if (p <= copies * 1./size) {
		rbackoff = Random::uniform(0.0, alpha_);
		bin ++;
	} else {
		rbackoff = Random::uniform(1.0 + alpha_, 2.0);
	}
	return (rtt * (det_ + rand_ * rbackoff));
}
#endif
