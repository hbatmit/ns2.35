/*
 * Copyright (c) 2008, Marcello Caleffi, <marcello.caleffi@unina.it>,
 * http://wpage.unina.it/marcello.caleffi
 *
 * The AOMDV code has been developed at DIET, Department of Electronic
 * and Telecommunication Engineering, University of Naples "Federico II"
 *
 *
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
Copyright (c) 1997, 1998 Carnegie Mellon University.  All Rights
Reserved. 

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The AODV code developed by the CMU/MONARCH group was optimized and tuned by Samir Das and Mahesh Marina, University of Cincinnati. The work was partially done in Sun Microsystems.
*/



#ifndef __aomdv_rtable_h__
#define __aomdv_rtable_h__

#include <assert.h>
#include <sys/types.h>
#include <config.h>
#include <lib/bsd-list.h>
#include <scheduler.h>

#define CURRENT_TIME    Scheduler::instance().clock()
#define INFINITY2        0xff

/*
   AOMDV Neighbor Cache Entry
*/
class AOMDV_Neighbor {
        friend class AOMDV;
        friend class aomdv_rt_entry;
 public:
        AOMDV_Neighbor(u_int32_t a) { nb_addr = a; }

protected:
        LIST_ENTRY(AOMDV_Neighbor) nb_link;
        nsaddr_t        nb_addr;
        double          nb_expire;      // ALLOWED_HELLO_LOSS * HELLO_INTERVAL
};

LIST_HEAD(aomdv_ncache, AOMDV_Neighbor);

 // AOMDV code
/*
   AOMDV Path list data structure
*/
class AOMDV_Path {
        friend class AOMDV;
        friend class aomdv_rt_entry;
 public:
        AOMDV_Path(nsaddr_t nh, u_int16_t h, double expire_time, nsaddr_t lh=0) { 
           nexthop = nh;
           hopcount = h;
           expire = expire_time;
           ts = Scheduler::instance().clock();
           lasthop = lh;
           // CHANGE
           error = false;
           // CHANGE    
        }
	void printPath() { 
	  printf("                        %6d  %6d  %6d\n", nexthop, hopcount, lasthop);
	}
	void printPaths() {
	  AOMDV_Path *p = this;
	  for (; p; p = p->path_link.le_next) {
	    p->printPath();
	  }
	}

 protected:
        LIST_ENTRY(AOMDV_Path) path_link;
        nsaddr_t        nexthop;    // nexthop address
        u_int16_t       hopcount;   // hopcount through this nexthop
        double          expire;     // expiration timeout
        double          ts;         // time when we saw this nexthop
        nsaddr_t        lasthop;    // lasthop address
        // CHANGE
        bool            error;
        // CHANGE
};

LIST_HEAD(aomdv_paths, AOMDV_Path);

/*
   AOMDV Precursor list data structure
*/
class AOMDV_Precursor {
        friend class AOMDV;
        friend class aomdv_rt_entry;
 public:
        AOMDV_Precursor(u_int32_t a) { pc_addr = a; }
		  
 protected:
        LIST_ENTRY(AOMDV_Precursor) pc_link;
        nsaddr_t        pc_addr;	// precursor address
};

LIST_HEAD(aomdv_precursors, AOMDV_Precursor);


/*
  Route Table Entry
*/

class aomdv_rt_entry {
        friend class aomdv_rtable;
        friend class AOMDV;
	friend class LocalRepairTimer;
 public:
        aomdv_rt_entry();
        ~aomdv_rt_entry();

        void            nb_insert(nsaddr_t id);
        AOMDV_Neighbor*  nb_lookup(nsaddr_t id);

 // AOMDV code
        AOMDV_Path*   path_insert(nsaddr_t nexthop, u_int16_t hopcount, double expire_time, nsaddr_t lasthop=0);

        AOMDV_Path*   path_lookup(nsaddr_t id);  // lookup path by nexthop

        AOMDV_Path*   disjoint_path_lookup(nsaddr_t nexthop, nsaddr_t lasthop);
        bool         new_disjoint_path(nsaddr_t nexthop, nsaddr_t lasthop);

        AOMDV_Path*   path_lookup_lasthop(nsaddr_t id);   // lookup path by lasthop
        void         path_delete(nsaddr_t id);           // delete path by nexthop
        void         path_delete(void);                  // delete all paths
        void         path_delete_longest(void);          // delete longest path
        bool         path_empty(void);                   // is the path list empty?
        AOMDV_Path*   path_find(void);                    // find the path that we got first
        AOMDV_Path*   path_findMinHop(void);              // find the shortest path
        u_int16_t    path_get_max_hopcount(void);  
        u_int16_t    path_get_min_hopcount(void);  
        double       path_get_max_expiration_time(void); 
        void         path_purge(void);
        void            pc_insert(nsaddr_t id);
        AOMDV_Precursor* pc_lookup(nsaddr_t id);
        void 		pc_delete(nsaddr_t id);
        void 		pc_delete(void);
        bool 		pc_empty(void);

        double          rt_req_timeout;         // when I can send another req
        u_int8_t        rt_req_cnt;             // number of route requests
	
		  // AOMDV code
        u_int8_t        rt_flags;
 protected:
        LIST_ENTRY(aomdv_rt_entry) rt_link;

        nsaddr_t        rt_dst;
        u_int32_t       rt_seqno;
	/* u_int8_t 	rt_interface; */
 // AOMDV code
        u_int16_t       rt_hops;             // hop count
        u_int16_t       rt_advertised_hops;  // advertised hop count
	int 		rt_last_hop_count;	// last valid hop count
 // AOMDV code
        aomdv_paths      rt_path_list;     // list of paths
        u_int32_t       rt_highest_seqno_heard; 
        int             rt_num_paths_;
	bool rt_error;
        
	/* list of precursors */ 
        aomdv_precursors rt_pclist;
        double          rt_expire;     		// when entry expires

#define RTF_DOWN 0
#define RTF_UP 1
#define RTF_IN_REPAIR 2

        /*
         *  Must receive 4 errors within 3 seconds in order to mark
         *  the route down.
        u_int8_t        rt_errors;      // error count
        double          rt_error_time;
#define MAX_RT_ERROR            4       // errors
#define MAX_RT_ERROR_TIME       3       // seconds
         */

#define MAX_HISTORY	3
	double 		rt_disc_latency[MAX_HISTORY];
	char 		hist_indx;
        int 		rt_req_last_ttl;        // last ttl value used
	// last few route discovery latencies
	// double 		rt_length [MAX_HISTORY];
	// last few route lengths

        /*
         * a list of neighbors that are using this route.
         */
        aomdv_ncache          rt_nblist;
};


/*
  The Routing Table
*/

class aomdv_rtable {
 public:
	aomdv_rtable() { LIST_INIT(&rthead); }

        aomdv_rt_entry*       head() { return rthead.lh_first; }

        aomdv_rt_entry*       rt_add(nsaddr_t id);
        void                 rt_delete(nsaddr_t id);
        aomdv_rt_entry*       rt_lookup(nsaddr_t id);
 // AOMDV code
	void                 rt_dumptable();
	bool                 rt_has_active_route();

 private:
        LIST_HEAD(aomdv_rthead, aomdv_rt_entry) rthead;
};

#endif /* _aomdv__rtable_h__ */
