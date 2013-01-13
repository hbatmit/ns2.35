// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- 

/*
 * Copyright (C) 2000 by the University of Southern California
 * $Id: simulator.cc,v 1.9 2010/03/08 05:54:49 tom_henderson Exp $
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

#include "simulator.h"
#include "node.h"
#include "address.h"
#include "object.h"

//class ParentNode;

static class SimulatorClass : public TclClass {
public:
	SimulatorClass() : TclClass("Simulator") {}
	TclObject* create(int , const char*const* ) {
		return (new Simulator);
	}
} simulator_class;

Simulator* Simulator::instance_;

int Simulator::command(int argc, const char*const* argv) {
	Tcl& tcl = Tcl::instance();
	if ((instance_ == 0) || (instance_ != this))
		instance_ = this;
	if (argc == 3) {
		if (strcmp(argv[1], "populate-flat-classifiers") == 0) {
			nn_ = atoi(argv[2]);
			populate_flat_classifiers();
			return TCL_OK;
		}
		if (strcmp(argv[1], "populate-hier-classifiers") == 0) {
			nn_ = atoi(argv[2]);
			populate_hier_classifiers();
			return TCL_OK;
		}
		if (strcmp(argv[1], "get-routelogic") == 0) {
			rtobject_ = (RouteLogic *)(TclObject::lookup(argv[2]));
			if (rtobject_ == NULL) {
				tcl.add_errorf("Wrong rtobject name %s", argv[2]);
				return TCL_ERROR;
			}
			return TCL_OK;
		}
		if (strcmp(argv[1], "mac-type") == 0) {
			if (strlen(argv[2]) >= SMALL_LEN) {
				tcl.add_errorf("Length of mac-type name must be < %d", SMALL_LEN);
				return TCL_ERROR;
			}
			strcpy(macType_, argv[2]);
			return TCL_OK;
		}
	}
	if (argc == 4) {
		if (strcmp(argv[1], "add-node") == 0) {
			Node *node = (Node *)(TclObject::lookup(argv[2]));
			if (node == NULL) {
				tcl.add_errorf("Wrong object name %s",argv[2]);
				return TCL_ERROR;
			}
			int id = atoi(argv[3]);
			add_node(node, id);
			return TCL_OK;
		} else if (strcmp(argv[1], "add-lannode") == 0) {
			LanNode *node = (LanNode *)(TclObject::lookup(argv[2]));
			if (node == NULL) {
				tcl.add_errorf("Wrong object name %s",argv[2]);
				return TCL_ERROR;
		  }
			int id = atoi(argv[3]);
			add_node(node, id);
			return TCL_OK;
		} else if (strcmp(argv[1], "add-abslan-node") == 0) {
			AbsLanNode *node = (AbsLanNode *)(TclObject::lookup(argv[2]));
			if (node == NULL) {
				tcl.add_errorf("Wrong object name %s",argv[2]);
				return TCL_ERROR;
			}
			int id = atoi(argv[3]);
			add_node(node, id);
			return TCL_OK;
		} else if (strcmp(argv[1], "add-broadcast-node") == 0) {
			BroadcastNode *node = (BroadcastNode *)(TclObject::lookup(argv[2]));
			if (node == NULL) {
				tcl.add_errorf("Wrong object name %s",argv[2]);
				return TCL_ERROR;
			}
			int id = atoi(argv[3]);
			add_node(node, id);
			return TCL_OK;
		} 
	}
	return (TclObject::command(argc, argv));
}

void Simulator::add_node(ParentNode *node, int id) {
	if (nodelist_ == NULL) 
		nodelist_ = new ParentNode*[SMALL_LEN]; 
	check(id);
	nodelist_[id] = node;
}

void Simulator::alloc(int n) {
        size_ = n;
	nodelist_ = new ParentNode*[n];
	for (int i=0; i<n; i++)
		nodelist_[i] = NULL;
}

// check if enough room in nodelist_
void Simulator::check(int n) {
        if (n < size_)
		return;
	ParentNode **old  = nodelist_;
	int osize = size_;
	int m = osize;
	if (m == 0)
		m = SMALL_LEN;
	while (m <= n)
		m <<= 1;
	alloc(m);
	for (int i=0; i < osize; i++) 
		nodelist_[i] = old[i];
	delete [] old;
}

void Simulator::populate_flat_classifiers() {
	// Set up each classifer (aka node) to act as a router.
	// Point each classifer table to the link object that
	// in turns points to the right node.
	char tmp[SMALL_LEN];
	if (nodelist_ == NULL)
		return;

	// Updating nodelist_ (total no of connected nodes)
	// size since size_ maybe smaller than nn_ (total no of nodes)
	check(nn_);    
	for (int i=0; i<nn_; i++) {
		if (nodelist_[i] == NULL) {
			i++; 
			continue;
		}
		nodelist_[i]->set_table_size(nn_);
		for (int j=0; j<nn_; j++) {
			if (i != j) {
				int nh = -1;
				nh = rtobject_->lookup_flat(i, j);
				if (nh >= 0) {
					NsObject *l_head = get_link_head(nodelist_[i], nh);
					sprintf(tmp, "%d", j);
					nodelist_[i]->add_route(tmp, l_head);
				}
			}  
		}
	}
}


void Simulator::populate_hier_classifiers() {
	// Set up each classifer (aka node) to act as a router.
	// Point each classifer table to the link object that
	// in turns points to the right node.
	int n_addr, levels, nh;
	int addr[TINY_LEN];
	char a[SMALL_LEN];
	// update the size of nodelist with nn_
	check(nn_);
	for (int i=0; i<nn_; i++) {
		if (nodelist_[i] == NULL) {
			i++; 
			continue;
		}
		n_addr = nodelist_[i]->address();
		char *addr_str = Address::instance().
			print_nodeaddr(n_addr);
		levels = Address::instance().levels_;
		int k;
		for (k=1; k <= levels; k++) 
			addr[k-1] = Address::instance().hier_addr(n_addr, k);
		for (k=1; k <= levels; k++) {
			int csize = rtobject_->elements_in_level(addr, k);
			nodelist_[i]->set_table_size(k, csize);
			
			char *prefix = NULL;
			if (k > 1)
				prefix = append_addr(k, addr);
			for (int m=0; m < csize; m++) {
				if (m == addr[k-1])
					continue;
				nh = -1;
				if (k > 1) {
					sprintf(a, "%s%d", prefix,m);
				} else
					sprintf(a, "%d", m);
				rtobject_->lookup_hier(addr_str, a, nh);
				if (nh == -1)
					continue;
				int n_id = node_id_by_addr(nh);
				if (n_id >= 0) {
					NsObject *l_head = get_link_head(nodelist_[i], n_id);
					nodelist_[i]->add_route(a, l_head);
				}	
			}
			if (prefix)
				delete [] prefix;
		}
		delete [] addr_str;
	}
}


int Simulator::node_id_by_addr(int address) {
	for (int i=0; i<nn_; i++) {
		if(nodelist_[i]->address() == address)
			return (nodelist_[i]->nodeid());
	}
	return -1;
}

char *Simulator::append_addr(int level, int *addr) {
	if (level > 1) {
		char tmp[TINY_LEN], a[SMALL_LEN];
		char *str;
		a[0] = '\0';
		for (int i=2; i<= level; i++) {
			sprintf(tmp, "%d.",addr[i-2]);
			strcat(a, tmp);
		}

		// To fix the bug of writing beyond the end of 
		// allocated memory for hierarchical addresses (xuanc, 1/14/02)
		// contributed by Joerg Diederich <dieder@ibr.cs.tu-bs.de>
		str = new char[strlen(a) + 1];
		strcpy(str, a);
		return (str);
	}
	return NULL;
}


NsObject* Simulator::get_link_head(ParentNode *node, int nh) {
	Tcl& tcl = Tcl::instance();
	tcl.evalf("[Simulator instance] get-link-head %d %d",
		  node->nodeid(), nh);
	NsObject *l_head = (NsObject *)TclObject::lookup(tcl.result());
	return l_head;
}

