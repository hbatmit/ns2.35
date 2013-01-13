
/* The AODV code developed by the CMU/MONARCH group was optimized
 * and tuned by Samir Das (UTSA) and Mahesh Marina (UTSA). The 
 * work was partially done in Sun Microsystems.
 * 
 * The original CMU copyright is below. 
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

*/

/* Ported into VINT ns by Ya Xu, Sept. 1999 */

#ifndef __rtable_h__
#define __rtable_h__

#include <assert.h>
#include <sys/types.h>

#include "config.h"
#include "scheduler.h"
#include "lib/bsd-list.h"

#define CURRENT_TIME    Scheduler::instance().clock()
#define INFINITY2        0xff

/* =====================================================================
   Neighbor Cache Entry
   ===================================================================== */
class Neighbor {
        friend class AODV;
        friend class rt_entry;
 public:
        Neighbor(u_int32_t a) { nb_addr = a; }

 protected:
        LIST_ENTRY(Neighbor) nb_link;
        nsaddr_t        nb_addr;
        double          nb_expire;      // ALLOWED_HELLO_LOSS * HELLO_INTERVAL
};

LIST_HEAD(ncache, Neighbor);


/* =====================================================================
   Route Table Entry
   ===================================================================== */

class rt_entry {
        friend class rttable;
        friend class AODV;
	friend class LocalRepairTimer;
 public:
        rt_entry();
        ~rt_entry();

        void            nb_insert(nsaddr_t id);
        Neighbor*       nb_lookup(nsaddr_t id);

 protected:
        LIST_ENTRY(rt_entry) rt_link;
        nsaddr_t        rt_dst;
        u_int32_t       rt_seqno;
        nsaddr_t        rt_nexthop;     // next hop IP address

        double          rt_expire;      // when entry expires
        u_int16_t       rt_hops;        // hop count
        u_int8_t        rt_flags;
#define RTF_DOWN 0
#define RTF_UP 1
#define RTF_IN_REPAIR 2

        /*
         *  Must receive 4 errors within 3 seconds in order to mark
         *  the route down.
         */
        u_int8_t        rt_errors;      // error count
        double          rt_error_time;
#define MAX_RT_ERROR            4       // errors
#define MAX_RT_ERROR_TIME       3       // seconds

        double          rt_req_timeout;         // when I can send another req
        u_int8_t        rt_req_cnt;             // number of route requests
        int 		rt_req_last_ttl;        // last ttl value used

#define MAX_HISTORY	3
	double 		rt_disc_latency[MAX_HISTORY];
	char 		hist_indx;
				// last few route discovery latencies
	// double 		rt_length [MAX_HISTORY];
				// last few route lengths

        /*
         * a list of neighbors that are using this route.
         */
        ncache          rt_nblist;
// Mahesh - 09/11/99
// This counter indicates whether route replies are sent back
// for this destination, If so, how many?
	int error_propagate_counter;

};


/* =====================================================================
   The Routing Table
   ===================================================================== */
class rttable {
 public:
	rttable() { LIST_INIT(&rthead); }

        rt_entry*       head() { return rthead.lh_first; }

        rt_entry*       rt_lookup(nsaddr_t id);
        void            rt_delete(nsaddr_t id);
        rt_entry*       rt_add(nsaddr_t id);
 private:
        LIST_HEAD(ncache, rt_entry) rthead;
};

#endif /* __rtable_h__ */
