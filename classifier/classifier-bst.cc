/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * Copyright (C) 1999 by the University of Southern California
 * $Id: classifier-bst.cc,v 1.15 2005/08/25 18:58:01 johnh Exp $
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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/classifier/classifier-bst.cc,v 1.15 2005/08/25 18:58:01 johnh Exp $";
#endif

#include <assert.h>
#include <stdlib.h>

#include "config.h"
#include "packet.h"
#include "ip.h"
#include "classifier.h"
#include "classifier-mcast.h"
#include "address.h"
#include "trace.h"
#include "ump.h"

struct upstream_info {
	int dst;
	int node_id;
	char* oiflink;
	struct upstream_info* next;
};

class MCastBSTClassifier : public MCastClassifier {
public:
	MCastBSTClassifier();
	~MCastBSTClassifier();
	static const char STARSYM[]; //"source" field for shared trees
protected:
	virtual int classify(Packet * p);
	upstream_info *oif2RP_;
	int32_t node_id_;

	void insert_upstream_info(int dst);
	virtual void recv(Packet *p, Handler *h);
	void upstream_add(int dst, char *oif2RP, int node_id);
	upstream_info* upstream_find(int dst);
};


const char MCastBSTClassifier::STARSYM[]= "x"; //"source" field for shared trees

static class MCastBSTClassifierClass : public TclClass {
public:
	MCastBSTClassifierClass() : TclClass("Classifier/Multicast/BST") {}
	TclObject* create(int, const char*const*) {
		return (new MCastBSTClassifier());
	}
} class_mcast_bidir_classifier;

MCastBSTClassifier::MCastBSTClassifier()
{
	oif2RP_ = NULL;
	node_id_ = -1;
}

MCastBSTClassifier::~MCastBSTClassifier()
{
	clearAll();
}

int MCastBSTClassifier::classify(Packet *pkt)
{
	hdr_cmn* h = hdr_cmn::access(pkt);
	hdr_ip* ih = hdr_ip::access(pkt);

	nsaddr_t src = ih->saddr(); /*XXX*/
	nsaddr_t dst = ih->daddr();

	int iface = h->iface();
	Tcl& tcl = Tcl::instance();

	hashnode* p = lookup(src, dst, iface);
	//printf("%s, src %d, dst %d, iface %d, p %d\n", name(), src, dst, iface, p);
 	if (p == 0)
 	        p = lookup_star(dst, iface);
		
	if (p == 0) {
		if ((p = lookup(src, dst)) == 0)
			p = lookup_star(dst);
		if (p == 0) {
  			// Didn't find an entry.
			tcl.evalf("%s new-group %ld %ld %d cache-miss", 
				  name(), src, dst, iface);
			// XXX see McastProto.tcl for the return values 0 -
			// once, 1 - twice 
			//printf("cache-miss result= %s\n", tcl.result());
			int res= atoi(tcl.result());

			if (res)
				insert_upstream_info(dst);
			return (res)? Classifier::TWICE : Classifier::ONCE;
		}
		if (p->iif == ANY_IFACE.value()) // || iface == UNKN_IFACE.value())
			return p->slot;

		tcl.evalf("%s new-group %ld %ld %d wrong-iif", 
			  name(), src, dst, iface);
		//printf("wrong-iif result= %s\n", tcl.result());
		int res= atoi(tcl.result());
		return (res)? Classifier::TWICE : Classifier::ONCE;
	}
	return p->slot;
}


void MCastBSTClassifier::recv(Packet *p, Handler *h) 
{
	hdr_cmn* h_cmn = hdr_cmn::access(p);
	hdr_ip* ih = hdr_ip::access(p);
 	hdr_ump* ump = hdr_ump::access(p);
	nsaddr_t dst = ih->daddr();
	Tcl& tcl = Tcl::instance();
	upstream_info *u_info;

	if (node_id_ == -1) {
		tcl.evalf("[%s set node_] id", name());
		sscanf(tcl.result(), "%d", &node_id_);
	} // if

	nsaddr_t src = ih->saddr(); /*XXX*/

	NsObject *node = find(p);
	if (node == NULL) {
		Packet::free(p);
		return;
	} // if

	if ((node_id_ != src) && (ump->isSet) && (ump->umpID_ != node_id_)) {
		// If this node is not the next hop upstream router,
		// and if there are no receivers connected to it, then 
		// we should immediately drop the packet before we
		// trigger a chace miss
		int rpf_iface;

		tcl.evalf("%s check-rpf-link %d %d", name(), node_id_, dst);
		sscanf(tcl.result(), "%d", &rpf_iface);
		if (rpf_iface != h_cmn->iface()) {
			// The RPF check has failed so we have to drop
			// the packet.  Otherwise, we will create
			// duplicate packets on the net.
  			Packet::free(p);
  			return;

			// The following code demonstrates how we
			// could generate duplicates if RPF check is
			// ignord.  Do not reset the UMP and then we
			// will have even more duplicates.  Remember
			// to comment out the previous two lines

//  			ih->flowid()++;
		} else 
			ump->isSet = 0;
	} // if

	if (src == node_id_) {
		memset(ump, 0, sizeof(hdr_ump)); // Initialize UMP to 0
		// We need to set the UMP option.  We need to find the
		// next hop router to the source and place it in the
		// UMP option.
		u_info = upstream_find(dst);
		if (!u_info) {
			printf("Error: Mcast info does not exist\n");
			exit(0);
		} // if
		ump->isSet = 1;
		ump->umpID_ = node_id_;
	} // if

// 	if (ump->isSet) {
// 		if (ump->umpID_ != node_id_) {
// 			// By now we are sure that the packet has
// 			// arrived on an RPF link.  So the UMP portion 
// 			// of the packet should be cleared before it
// 			// is sent downstream
// 			ump->isSet = 0;
// 		} // if
// 	} // if

	if (ump->isSet) {
		u_info = upstream_find(dst);
		if (node_id_ == u_info->node_id)
			// If the next hop is the node itself, then we
			// are at the RP.
			ump->isSet = 0;
		else {
			ump->umpID_ = u_info->node_id;
			ump->oif = strdup(u_info->oiflink);
		} // else
	} else {
		int match;
		
		tcl.evalf("%s match-BST-iif %d %d", name(),
			  h_cmn->iface(), dst);  
		sscanf(tcl.result(), "%d", (int *)&match);
		if (!match) {
			Packet::free(p);
			return;
		} // else
	} // else

	node->recv(p, h);
} // MCastBSTClassifier::recv

void MCastBSTClassifier::upstream_add(int dst, char *link, int node_id)
{
	struct upstream_info *next,
		*current;

	if (oif2RP_) {
		next = oif2RP_;
		do {
			current = next;
			if (current->dst == dst) {
				free(current->oiflink);
				current->oiflink = strdup(link);
				current->node_id = node_id;
				return;
			} // if
			next = current->next;
		} while (next);
		next = new(struct upstream_info);
		next->dst = dst;
		next->oiflink = strdup(link);
		next->node_id = node_id;
		current->next = next;
	} else {
		oif2RP_ = new(struct upstream_info);
		oif2RP_->dst = dst;
		oif2RP_->oiflink = strdup(link);
		oif2RP_->node_id = node_id;
		oif2RP_->next = NULL;
	} // else
} // MCastBSTClassifier::upstream_add

upstream_info* MCastBSTClassifier::upstream_find(int dst)
{
	upstream_info *index;

	index = oif2RP_;
	while (index) {
		if (index->dst == dst)
			return index;
		index = index->next;
	} // while
	return NULL;
} // MCastBSTClassifier::upstream_find

void MCastBSTClassifier::insert_upstream_info(int dst) 
{
	char temp_str[100];
	int nodeID;
	Tcl& tcl = Tcl::instance();

	tcl.evalf("%s info class", name());
	sscanf(tcl.result(), "%s", temp_str);
	tcl.evalf("%s upstream-link %d", name(), dst);
	sscanf(tcl.result(), "%s %d", temp_str, &nodeID);
	upstream_add(dst, temp_str, nodeID);
} // MCastBSTClassifier::insert_upstream_info

