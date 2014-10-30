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
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/routing/route.h,v 1.9 2002/10/09 03:47:01 difa Exp $
 *
 */

#ifndef ns_route_h
#define ns_route_h

#undef INFINITY
#define INFINITY	0x3fff
#define INDEX(i, j, N) ((N) * (i) + (j))

/*** definitions for hierarchical routing support 
right now implemented for 3 levels of hierarchy --> should
be able to extend it for n levels of hierarchy in the future ***/

#define HIER_LEVEL	3
#define N_N_INDEX(i, j, a, b, c)	(((i) * (a+b+c)) + (j))
#define N_C_INDEX(i, j, a, b, c)	(((i) * (a+b+c)) + (a+j))
#define N_D_INDEX(i, j, a, b, c)	(((i) * (a+b+c)) + (a+(b-1)+j))
#define C_C_INDEX(i, j, a, b, c)	(((a+i) * (a+b+c)) + (a+j))
#define C_D_INDEX(i, j, a, b, c)	(((a+i) * (a+b+c)) + (a+(b-1)+j))
#define D_D_INDEX(i, j, a, b, c)	(((a+(b-1)+i) * (a+b+c)) + (a+(b-1)+j))

struct adj_entry {
	double cost;
	void* entry;
};

struct route_entry {
public:
	int next_hop;
	void* entry;
};

class RouteLogic : public TclObject {
public:
	RouteLogic();
	~RouteLogic();
	int command(int argc, const char*const* argv);
	virtual int lookup_flat(char* asrc, char* adst, int&result);
	virtual int lookup_flat(int sid, int did); //added for pushback - ratul
	int lookup_hier(char* asrc, char* adst, int&result);
	static void ns_strtok(char *addr, int *addrstr);
	int elements_in_level (int *addr, int level);
	inline int domains(){ return (D_-1); }
	inline int domain_size(int domain);
	inline int cluster_size(int domain, int cluster);
protected:

	void check(int);
	void alloc(int n);
	void reset(int src, int dst);
	void compute_routes();
	void insert(int src, int dst, double cost);
	adj_entry *adj_;
	route_entry *route_;
	void insert(int src, int dst, double cost, void* entry);
	void reset_all();
	int size_,
		maxnode_;

	/**** Hierarchical routing support ****/

	void hier_check(int index);
	void hier_alloc(int size);
	void hier_init(void);
	void str2address(const char*const* address, int *src, int *dst);
	void get_address(char * target, int next_hop, int index, int d, int size, int *src);
	void hier_insert(int *src, int *dst, int cost);
	void hier_reset(int *src, int *dst);
	void hier_compute();
	void hier_compute_routes(int index, int d);

	/* Debugging print functions */
	void hier_print_hadj();
	void hier_print_route();
	
	int	**hadj_;
	int	**hroute_;
	int	*hsize_;
	int	*cluster_size_;		/* no. of nodes/cluster/domain */
	char	***hconnect_;		/* holds the connectivity info --> address of target */
	int	level_;
	int	*C_;                    /* no. of clusters/domain */
	int	D_,			/* total no. of domains */
		Cmax_;			/* max value of C_ for initialization purpose */
	
};

class RouteLogicAlgo : public RouteLogic {
public:
	int lookup_flat(char* asrc, char* adst, int&result) {
		Tcl& tcl= Tcl::instance();
		tcl.evalf("%s lookup %s %s", name(), asrc, adst);
		result= atoi(tcl.result());
		return TCL_OK;
	}
};
#endif
