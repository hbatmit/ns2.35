/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1991-1994 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and the Network Research Group at
 *	Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Routing code for general topologies based on min-cost routing algorithm in
 * Bertsekas' book.  Written originally by S. Keshav, 7/18/88
 * (his work covered by identical UC Copyright)
 */

#ifndef lint
static const char rcsid[] =
"@(#) $Header: /cvsroot/nsnam/ns-2/routing/route.cc,v 1.40 2010/03/08 05:54:53 tom_henderson Exp $ (LBL)";
#endif

#include <stdlib.h>
#include <assert.h>
#include "config.h"
#include "route.h"
#include "address.h"

class RouteLogicClass : public TclClass {
public:
	RouteLogicClass() : TclClass("RouteLogic") {}
	TclObject* create(int, const char*const*) {
		return (new RouteLogic());
	}
} routelogic_class;

void RouteLogic::reset_all()
{
	delete[] adj_;
	delete[] route_;
	adj_ = 0; 
	route_ = 0;
	size_ = 0;
}

int RouteLogic::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "compute") == 0) {
			if (adj_ == 0)
				return (TCL_OK);
			compute_routes();
			return (TCL_OK);
		} else if (strcmp(argv[1], "hier-compute") == 0) {
			if (hadj_ == 0) {
				return (TCL_OK);
			}
			hier_compute();
			return (TCL_OK);
		} else if (strcmp(argv[1], "hier-print") == 0) {
			hier_print_hadj();
			return (TCL_OK);
		} else if (strcmp(argv[1], "hier-print-route") == 0) {
			hier_print_route();
			return (TCL_OK);
		} else if (strcmp(argv[1], "reset") == 0) {
			reset_all();
			return (TCL_OK);
		}
	} else if (argc > 2) {
		if (strcmp(argv[1], "insert") == 0) {
			int src = atoi(argv[2]) + 1;
			int dst = atoi(argv[3]) + 1;
			if (src <= 0 || dst <= 0) {
				tcl.result("negative node number");
				return (TCL_ERROR);
			}
			double cost = (argc == 5 ? atof(argv[4]) : 1);
			insert(src, dst, cost);
			return (TCL_OK);
		} else if (strcmp(argv[1], "hlevel-is") == 0) {
			level_ = atoi(argv[2]);
			if (level_ < 1) {
				tcl.result("send-hlevel: # hierarchy levels should be non-zero");
				return (TCL_ERROR);
			}
			return (TCL_OK);
		} else if (strcmp(argv[1], "send-num-of-domains") == 0) {
			D_ = atoi(argv[2]) + 1;
			if (D_ <= 1) {
				tcl.result("send-num-of-domains: # domains should be larger than 1");
				return (TCL_ERROR);
			}
			return (TCL_OK);
		} else if (strcmp(argv[1], "send-num-of-clusters") == 0) {
			if (argc != D_ + 1) {
				tcl.resultf("send-num-of-clusters: # of "
					    "clusters %d != domain (%d) + 1\n",
					    argc, D_);
				return (TCL_ERROR);
			}
			C_ = new int[D_];
			int i, j = 2;
			for (i = 1; i < D_; i++) {
				C_[i] = atoi(argv[j]) + 1;
				j++;
			}
			hier_init();
			return (TCL_OK);
		} else if(strcmp(argv[1], "send-num-of-nodes") == 0) {
			int i, j, k=2, Ctotal=0 ;
			for (i=1; i < D_; i++)
				Ctotal = Ctotal + (C_[i]-1);
			if (argc != (Ctotal + 2)) {
				tcl.result("send-hlastdata: # args do not match");
				return (TCL_ERROR);
			}
			for (i=1; i < D_; i++)
				for (j=1; (j < C_[i]); j++) {
					cluster_size_[INDEX(i, j, Cmax_)] = atoi(argv[k]);
					k++;
				}
			return (TCL_OK);
		} else if (strcmp(argv[1], "hier-insert") == 0) {
			if(Cmax_== 0 || D_== 0) {
				tcl.result("Required Hier_data not sent");
				return (TCL_ERROR);
			}
			int i;
			int src_addr[SMALL_LEN], dst_addr[SMALL_LEN];
			str2address(argv, src_addr, dst_addr);
			for (i=0; i < level_; i++)
				if (src_addr[i]<=0 || dst_addr[i]<=0){
					tcl.result ("negative node number");
					return (TCL_ERROR);
				}
			int cost = (argc == 5 ? atoi(argv[4]) : 1);
			hier_insert(src_addr, dst_addr, cost);
			return (TCL_OK);
		} else if (strcmp(argv[1], "hier-reset") == 0) {
			int i;
			int  src_addr[SMALL_LEN], dst_addr[SMALL_LEN];
			
			str2address(argv, src_addr, dst_addr);
			// assuming node-node addresses (instead of 
			// node-cluster or node-domain pair) 
			// are sent for hier_reset  
			for (i=0; i < level_; i++)
				if (src_addr[i]<=0 || dst_addr[i]<=0){
					tcl.result ("negative node number");
					return (TCL_ERROR);
				}
			hier_reset(src_addr, dst_addr);
			return (TCL_OK);
		} else if (strcmp(argv[1], "hier-lookup") == 0) {
			int nh;
			int res = lookup_hier((char*)argv[2], (char*)argv[3],
					      nh);
			return res;
		} else if (strcmp(argv[1], "reset") == 0) {
			int src = atoi(argv[2]) + 1;
			int dst = atoi(argv[3]) + 1;
			if (src <= 0 || dst <= 0) {
				tcl.result("negative node number");
				return (TCL_ERROR);
			}
			reset(src, dst);
			return (TCL_OK);
		} else if (strcmp(argv[1], "lookup") == 0) {
			int nh;
			int res = lookup_flat((char*)argv[2], (char*)argv[3], 
					      nh);
			if (res == TCL_OK)
				tcl.resultf("%d", nh);
			return res;
		}
	}
	return (TclObject::command(argc, argv));
}

// xxx: using references as in this result is bogus---use pointers!
int RouteLogic::lookup_flat(char* asrc, char* adst, int& result) {
	Tcl& tcl = Tcl::instance();
	int src = atoi(asrc) + 1;
	int dst = atoi(adst) + 1;

	if (route_ == 0) {
		// routes are computed only after the simulator is running
		// ($ns run).
		tcl.result("routes not yet computed");
		return (TCL_ERROR);
	}
	if (src >= size_ || dst >= size_) {
		tcl.result("node out of range");
		return (TCL_ERROR);
	}
	result = route_[INDEX(src, dst, size_)].next_hop - 1;
	return TCL_OK;
}

//added for pushback. a method callable from c++ code. 
//probably could have been concocted from already existing methods - ratul
int RouteLogic::lookup_flat(int sid, int did) {
	int src = sid+1;
	int dst = did+1;
	if (route_ == 0) {
		// routes are computed only after the simulator is running
		// ($ns run).
		printf("routes not yet computed\n");
		return (-1);
	}
	if (src >= size_ || dst >= size_) {
		printf("node out of range\n");
		return (-2);
	}
	return route_[INDEX(src, dst, size_)].next_hop - 1;
}

// xxx: using references as in this result is bogus---use pointers!
int RouteLogic::lookup_hier(char* asrc, char* adst, int& result) {
	int i;
	int src[SMALL_LEN], dst[SMALL_LEN];
	Tcl& tcl = Tcl::instance();

	if ( hroute_ == 0) {
		tcl.result("Required Hier_data not sent");
		return TCL_ERROR;
	}
      
	ns_strtok(asrc, src);
	ns_strtok(adst, dst);

	for (i=0; i < level_; i++)
		if (src[i] <= 0) {
			tcl.result("negative src node number");
			return TCL_ERROR;
		}
	if (dst[0] <= 0) {
		tcl.result("negative dst domain number");
		return TCL_ERROR;
	}

	int d = src[0];
	int index = INDEX(src[0], src[1], Cmax_);
	int size = cluster_size_[index];

	if (hsize_[index] == 0) {
		tcl.result("Routes not computed");
		return TCL_ERROR;
	}
	if ((src[0] < D_) || (dst[0] < D_)) {
		if((src[1] < C_[d]) || (dst[1] < C_[dst[0]]))
			if((src[2] <= size) ||
			   (dst[2]<=cluster_size_[INDEX(dst[0],dst[1],Cmax_)]))
			{
				;
			}
	} else { 
		tcl.result("node out of range");
		return TCL_ERROR;
	}
	int next_hop = 0;
	/* if node-domain lookup */
	if (((dst[1] <= 0) && (dst[2] <= 0)) ||
	    (src[0] != dst[0])){
		next_hop = hroute_[index][N_D_INDEX(src[2], dst[0], size, C_[d], D_)];
	}
	/* if node-cluster lookup */
	else if ((dst[2] <= 0) || (src[1] != dst[1])) {
		next_hop = hroute_[index][N_C_INDEX(src[2], dst[1], size, C_[d], D_)];
	}
	/* if node-node lookup */
	else {
		next_hop = hroute_[index][N_N_INDEX(src[2], dst[2], size, C_[d], D_)];	
	}
	
	char target[SMALL_LEN];
	if (next_hop > 0) {
		get_address(target, next_hop, index, d, size, src);
		tcl.result(target);
		result= Address::instance().str2addr(target);
	} else {
		tcl.result("-1");
		result = -1;
	}
	return TCL_OK;
}

RouteLogic::RouteLogic()
{
	size_ = 0;
	adj_ = 0;
	route_ = 0;
	/* additions for hierarchical routing extension */
	C_ = 0;
	D_ = 0;
	Cmax_ = 0;
	level_ = 0;
	hsize_ = 0;
	hadj_ = 0;
	hroute_ = 0;
	hconnect_ = 0;
	cluster_size_ = 0;
}
	
RouteLogic::~RouteLogic()
{
	delete[] adj_;
	delete[] route_;

	for (int i = 0; i < (Cmax_ * D_); i++) {
		for (int j = 0; j < (Cmax_ + D_) * (cluster_size_[i]+1); j++) {
			if (hconnect_[i][j] != NULL)
				delete [] hconnect_[i][j];
		}
		delete [] hconnect_[i];
	}

	for (int n =0; n < (Cmax_ * D_); n++) {
		if (hadj_[n] != NULL)
			delete [] hadj_[n];
		if (hroute_[n] != NULL)
			delete [] hroute_[n];
	}

	delete [] C_;
	delete [] hsize_;
	delete [] cluster_size_;
	delete hadj_;
	delete hroute_;
	delete hconnect_;
}

void RouteLogic::alloc(int n)
{
	size_ = n;
	n *= n;
	adj_ = new adj_entry[n];
	for (int i = 0; i < n; ++i) {
		adj_[i].cost = INFINITY;
		adj_[i].entry = 0;
	}
}

/*
 * Check that we have enough storage in the adjacency array
 * to hold a node numbered "n"
 */
void RouteLogic::check(int n)
{
	if (n < size_)
		return;

	adj_entry* old = adj_;
	int osize = size_;
	int m = osize;
	if (m == 0)
		m = 16;
	while (m <= n)
		m <<= 1;

	alloc(m);
	for (int i = 0; i < osize; ++i) {
		for (int j = 0; j < osize; ++j)
			adj_[INDEX(i, j, m)].cost =old[INDEX(i, j, osize)].cost;
	}
	size_ = m;
	delete[] old;
}

void RouteLogic::insert(int src, int dst, double cost)
{
	check(src);
	check(dst);
	adj_[INDEX(src, dst, size_)].cost = cost;
}
void RouteLogic::insert(int src, int dst, double cost, void* entry_)
{
	check(src);
	check(dst);
	adj_[INDEX(src, dst, size_)].cost = cost;
	adj_[INDEX(src, dst, size_)].entry = entry_;
}

void RouteLogic::reset(int src, int dst)
{
	assert(src < size_);
	assert(dst < size_);
	adj_[INDEX(src, dst, size_)].cost = INFINITY;
}

void RouteLogic::compute_routes()
{
	int n = size_;
	int* parent = new int[n];
	double* hopcnt = new double[n];
#define ADJ(i, j) adj_[INDEX(i, j, size_)].cost
#define ADJ_ENTRY(i, j) adj_[INDEX(i, j, size_)].entry
#define ROUTE(i, j) route_[INDEX(i, j, size_)].next_hop
#define ROUTE_ENTRY(i, j) route_[INDEX(i, j, size_)].entry
	delete[] route_;
	route_ = new route_entry[n * n];
	memset((char *)route_, 0, n * n * sizeof(route_[0]));

	/* do for all the sources */
	int k;
	for (k = 1; k < n; ++k) {
		int v;
		for (v = 0; v < n; v++)
			parent[v] = v;
	
		/* set the route for all neighbours first */
		for (v = 1; v < n; ++v) {
			if (parent[v] != k) {
				hopcnt[v] = ADJ(k, v);
				if (hopcnt[v] != INFINITY) {
					ROUTE(k, v) = v;
					ROUTE_ENTRY(k, v) = ADJ_ENTRY(k, v);
				}
			}
		}
		for (v = 1; v < n; ++v) {
			/*
			 * w is the node that is the nearest to the subtree
			 * that has been routed
			 */
			int o = 0;
			/* XXX */
			hopcnt[0] = INFINITY;
			int w;
			for (w = 1; w < n; w++)
				if (parent[w] != k && hopcnt[w] < hopcnt[o])
					o = w;
			parent[o] = k;
			/*
			 * update distance counts for the nodes that are
			 * adjacent to o
			 */
			if (o == 0)
				continue;
			for (w = 1; w < n; w++) {
				if (parent[w] != k &&
				    hopcnt[o] + ADJ(o, w) < hopcnt[w]) {
					ROUTE(k, w) = ROUTE(k, o);
					ROUTE_ENTRY(k, w) = 
					    ROUTE_ENTRY(k, o);
					hopcnt[w] = hopcnt[o] + ADJ(o, w);
				}
			}
		}
	}
	/*
	 * The route to yourself is yourself.
	 */
	for (k = 1; k < n; ++k) {
		ROUTE(k, k) = k;
		ROUTE_ENTRY(k, k) = 0; // This should not matter
	}

	delete[] hopcnt;
	delete[] parent;
}

/* hierarchical routing support */

/*
 * This function creates adjacency matrix for each cluster at the lowest level
 * of the hierarchy for every node in the cluster, for every other cluster in 
 * the domain, and every other domain. can be extended from 3-level hierarchy 
 * to n-level along similar lines
 */
void RouteLogic::hier_alloc(int i)
{
	hsize_[i] = cluster_size_[i]+ Cmax_+ D_ ;
	hsize_[i] *= hsize_[i];
	hadj_[i] = new int[hsize_[i]];
	hroute_[i] = new int[hsize_[i]];
	hconnect_[i] = new char*[(Cmax_ + D_) * (cluster_size_[i]+1)];
	for (int n = 0; n < hsize_[i]; n++){
		hadj_[i][n] = INFINITY;
		hroute_[i][n] = INFINITY;
	}
}

void RouteLogic::hier_check(int i)
{
	if(hsize_[i] > 0)
		return;
	else
		hier_alloc(i);
}

void RouteLogic::hier_init(void) 
{
	int i;

	for (i = 1; i < D_; i++) {
		Cmax_ = C_[i] > Cmax_ ? C_[i]: Cmax_;
	}
	int arr_size = Cmax_ * D_ ;
	cluster_size_ = new int[arr_size]; 
	hsize_ = new int[arr_size];
	for (i = 0; i < arr_size; i++)
		hsize_[i] = 0;
	hadj_ = new int*[arr_size];
	hroute_ = new int*[arr_size];
	hconnect_ = new char**[arr_size];
}


int RouteLogic::domain_size(int domain) { 
	return (C_[domain+1]-1); 
}
int RouteLogic::cluster_size(int d, int c) {
	d += 1;
	c += 1;
	return (cluster_size_[INDEX(d, c, Cmax_)]);
}

int RouteLogic::elements_in_level(int *addr, int level) {
	if (level == 1)
		return (domains());
	else if (level == 2)
		return (domain_size(addr[0]));
	else if (level == 3) {
		return (cluster_size(addr[0], addr[1]));
	}
	return (-1);
}

void RouteLogic::str2address(const char*const* argv, int *src_addr, int *dst_addr)
{
	char src[SMALL_LEN];
	char dst[SMALL_LEN];

	strcpy(src, argv[2]);
	strcpy(dst, argv[3]);

	ns_strtok(src, src_addr);
	ns_strtok(dst, dst_addr);
}

void RouteLogic::ns_strtok(char *addr, int *addrstr)
{
	int	i;
	char	tmpstr[SMALL_LEN];
	char	*next, *index;
	
	i = 0;
	strcpy(tmpstr, addr);
	next = tmpstr;
	while(*next){
		index = strstr(next, ".");
		if (index != NULL){
			next[index - next] = '\0';
			addrstr[i] = atoi(next) + 1;
			next = index + 1;
			i++;
		}
		else {
			if (*next != '\0') //damn that ending point
				addrstr[i] = atoi(next) + 1;
			break;
		}
	}
}


void RouteLogic::get_address(char *address, int next_hop, int index, int d, 
			     int size, int *src)
{
	if (next_hop <= size) {
		sprintf(address,"%d.%d.%d", src[0]-1, src[1]-1, next_hop-1);
	}
	else if ((next_hop > size) && (next_hop < (size + C_[d]))) {
		int temp = next_hop - size;
		int I = src[2] * (C_[d] + D_) + temp;
		strcpy(address, hconnect_[index][I]);
	}
	else {
		int temp = next_hop - size - (C_[d] - 1);
		int I = src[2] * (C_[d] + D_) + (C_[d] - 1 + temp);
		strcpy(address,hconnect_[index][I]);
	}
}

void RouteLogic::hier_reset(int *src, int *dst)
{
	int i, d, n;
	d = src[0];
	if (src[0] == dst[0])
		if (src[1] == dst[1]) {
			i = INDEX(src[0], src[1], Cmax_);
			n = cluster_size_[i];
			hadj_[i][N_N_INDEX(src[2], dst[2], n, C_[d], D_)] = INFINITY;
		} else {
			for (int y=1; y < C_[d]; y++) { 
				i = INDEX(src[0], y, Cmax_);
				n = cluster_size_[i];
				hadj_[i][C_C_INDEX(src[1], dst[1], n, C_[d], D_)] = INFINITY;
				if (y == src[1])
					hadj_[i][N_C_INDEX(src[2], dst[1], n, C_[d], D_)] = INFINITY; 
			}
		}
	else {
		for (int x=1; x < D_; x++)
			for (int y=1; y < C_[x]; y++) {
				i = INDEX(x, y, Cmax_);
				n = cluster_size_[i];
				hadj_[i][D_D_INDEX(src[0], dst[0], n, C_[x], D_)] = INFINITY;
				if ( x == src[0] ){
					hadj_[i][C_D_INDEX(src[1], dst[0], n, C_[x], D_)] = INFINITY;
					if (y == src[1])
						hadj_[i][N_D_INDEX(src[2], dst[0], n, C_[x], D_)] = INFINITY;
				}
			}
	}
}

void RouteLogic::hier_insert(int *src_addr, int *dst_addr, int cost)
{
	int X1 = src_addr[0];
	int Y1 = src_addr[1];
	int Z1 = src_addr[2];
	int X2 = dst_addr[0];
	int Y2 = dst_addr[1];
	int Z2 = dst_addr[2];
	int n, i;

	if ( X1 == X2)
		if (Y1 == Y2){ 
			/*
			 * For the same domain & cluster 
			 */
			i = INDEX(X1, Y1, Cmax_);
			n = cluster_size_[i];
			hier_check(i);
			hadj_[i][N_N_INDEX(Z1, Z2, n, C_[X1], D_)] = cost;
		} else { 
			/* 
			 * For the same domain but diff clusters 
			 */
			for (int y=1; y < C_[X1]; y++) { /* insert cluster connectivity */
				i = INDEX(X1, y, Cmax_);
				n = cluster_size_[i];
				hier_check(i);
				hadj_[i][C_C_INDEX(Y1, Y2, n, C_[X1], D_)] = cost;

				if (y == Y1) {  /* insert node conn. */
					hadj_[i][N_C_INDEX(Z1, Y2, n, C_[X1], D_)] = cost;
					int I = Z1 * (C_[X1]+ D_) + Y2;
					hconnect_[i][I] = new char[SMALL_LEN];
					sprintf(hconnect_[i][I],"%d.%d.%d",X2-1,Y2-1,Z2-1);
				}
			}
		}
	else { 
		/* 
		 * For different domains 
		 */
		for (int x=1; x < D_; x++) { /* inset domain connectivity */
			for (int y=1; y < C_[x]; y++) {
				i = INDEX(x, y, Cmax_);
				n = cluster_size_[i];
				hier_check(i);
				hadj_[i][D_D_INDEX(X1, X2, n, C_[x], D_)] = cost;
			}
		}
		for (int y=1; y < C_[X1]; y++) { /* insert cluster connectivity */
			i = INDEX(X1, y, Cmax_);
			n = cluster_size_[i];
			hier_check(i);
			hadj_[i][C_D_INDEX(Y1, X2, n, C_[X1], D_)] = cost;
		}
		/* insert node connectivity */
		i = INDEX(X1, Y1, Cmax_);
		n = cluster_size_[i]; 
		hadj_[i][N_D_INDEX(Z1, X2, n, C_[X1], D_)] = cost;
		int I = Z1 * (C_[X1] + D_) + (C_[X1] - 1 + X2);
		hconnect_[i][I] = new char[SMALL_LEN];
		sprintf(hconnect_[i][I],"%d.%d.%d",X2-1,Y2-1,Z2-1);
	}
}

void RouteLogic::hier_compute_routes(int i, int j)
{
	int size = (cluster_size_[i] + C_[j] + D_);
	int n = size ;
	double* hopcnt = new double[n];
#define HADJ(i, j) adj_[INDEX(i, j, size)].cost
#define HROUTE(i, j) route_[INDEX(i, j, size)].next_hop
	delete[] route_;
	route_ = new route_entry[n * n];
	int* parent = new int[n];
	memset((char *)route_, 0, n * n * sizeof(route_[0]));

	/* do for all the sources */
	int k;
	for (k = 1; k < n; ++k) {
		int v;
		for (v = 0; v < n; v++)
			parent[v] = v;

		/* set the route for all neighbours first */
		for (v = 1; v < n; ++v) {
			if (parent[v] != k) {
				hopcnt[v] = HADJ(k, v);
				if (hopcnt[v] != INFINITY)
					HROUTE(k, v) = v;
			}
		}
		for (v = 1; v < n; ++v) {
			/*
			 * w is the node that is the nearest to the subtree
			 * that has been routed
			 */
			int o = 0;
			/* XXX */
			hopcnt[0] = INFINITY;
			int w;
			for (w = 1; w < n; w++)
				if (parent[w] != k && hopcnt[w] < hopcnt[o])
					o = w;
			parent[o] = k;
			/*
			 * update distance counts for the nodes that are
			 * adjacent to o
			 */
			if (o == 0)
				continue;
			for (w = 1; w < n; w++) {
				if (parent[w] != k &&
				    hopcnt[o] + HADJ(o, w) < hopcnt[w]) {
					HROUTE(k, w) = HROUTE(k, o);
					hopcnt[w] = hopcnt[o] + HADJ(o, w);
				}
			}
		}
	}
	/*
	 * The route to yourself is yourself.
	 */
	for (k = 1; k < n; ++k)
		HROUTE(k, k) = k;

	delete[] hopcnt;
	delete[] parent;
}

/* function to check the adjacency matrices created */
void RouteLogic::hier_print_hadj() {
	int i, j, k;

	for (j=1; j < D_; j++) 
		for (k=1; k < C_[j]; k++) {
			i = INDEX(j, k, Cmax_);
			int s = (cluster_size_[i] + C_[j] + D_);
			printf("ADJ MATRIX[%d] for cluster %d.%d :\n",i,j,k);
			int temp = 1;
			printf(" ");
			while(temp < s){
				printf(" %d",temp);
				temp++;
			}
			printf("\n");
			for(int n=1; n < s;n++){
				printf("%d ",n);
				for(int m=1;m < s;m++){
					if(hadj_[i][INDEX(n,m,s)] == INFINITY)
						printf("~ ");
					else
						printf("%d ",hadj_[i][INDEX(n,m,s)]);
				}
				printf("\n");
			}
			printf("\n\n");
		}
}

void RouteLogic::hier_compute()
{
	int i, j, k, m, n;
	for (j=1; j < D_; j++) 
		for (k=1; k < C_[j]; k++) {
			i = INDEX(j, k, Cmax_);
			int s = (cluster_size_[i] + C_[j] + D_);
			adj_ = new adj_entry[(s * s)];
			memset((char *)adj_, 0, s * s * sizeof(adj_[0]));
			for (n=0; n < s; n++)
				for(m=0; m < s; m++)
					adj_[INDEX(n, m, s)].cost = hadj_[i][INDEX(n, m, s)];
			hier_compute_routes(i, j);
	
			for (n=0; n < s; n++)
				for(m=0; m < s; m++)
					hroute_[i][INDEX(n, m, s)] = route_[INDEX(n, m, s)].next_hop;
			delete [] adj_;
		}
}

/*
 * Prints routing table - debugging function
 */
void RouteLogic::hier_print_route()
{
	for (int j=1; j < D_; j++) 
		for (int k=1; k < C_[j]; k++) {
			int i = INDEX(j, k, Cmax_);
			int s = (cluster_size_[i]+C_[j]+D_);
			printf("ROUTE_TABLE[%d] for cluster %d.%d :\n",i,j,k);
			int temp = 1;
			printf(" ");
			while(temp < s){
				printf(" %d",temp);
				temp++;
			}
			printf("\n");
			for(int n=1; n < s; n++){
				printf("%d ",n);
				for(int m=1; m < s; m++)
					printf("%d ",hroute_[i][INDEX(n, m, s)]);
				printf("\n");
			}
			printf("\n\n");
		}
}

static class RouteLogicAlgoClass : public TclClass {
public:
	RouteLogicAlgoClass() : TclClass("RouteLogic/Algorithmic") {}
	TclObject* create(int, const char*const*) {
		return (new RouteLogicAlgo);
	}
} class_routelogic_algo;
