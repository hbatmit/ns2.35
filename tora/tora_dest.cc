/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
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
 *      This product includes software developed by the Computer Systems
 *      Engineering Group at Lawrence Berkeley Laboratory.
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
 */
/* Ported from CMU/Monarch's code */
/* 
   $Id: tora_dest.cc,v 1.2 1999/08/12 21:12:29 yaxu Exp $
   */

#include <agent.h>
#include <random.h>
#include <trace.h>

#include <ll.h>
#include <priqueue.h>
#include <tora/tora_packet.h>
#include <tora/tora.h>

#define	CURRENT_TIME	Scheduler::instance().clock()

/* ======================================================================
   TORADest Class Functions
   ====================================================================== */
TORADest::TORADest(nsaddr_t id, Agent *a) :  height(id)
{
	index = id;

	rt_req = 0;
	time_upd = 0.0;

	time_rt_req = 0.0;
	time_tx_qry = 0.0;

	LIST_INIT(&nblist);
	num_active = num_down = num_up = 0;

        agent = (toraAgent*) a;
}


void
TORADest::dump()
{
	TORANeighbor *tn = nb_find_next_hop();

	fprintf(stdout, "\tDEST: %d, RT Req: %x, Time UPD: %f\n",
		index, rt_req, time_upd);
	fprintf(stdout, "\t\ttau: %f, oid: %d, r: %d, delta: %d, id: %d\n",
		height.tau, height.oid, height.r, height.delta, height.id);
	fprintf(stdout, "\t\tActive: %d, Down: %d, Up: %d, Next Hop: %d\n\n",
		num_active, num_down, num_up, tn ? tn->index : -1);

	for(tn = nblist.lh_first; tn; tn = tn->link.le_next)
		tn->dump();
}


/* ====================================================================== */

TORANeighbor*
TORADest::nb_add(nsaddr_t id)
{
	TORANeighbor *tn = new TORANeighbor(id, agent);
	assert(tn);

	LIST_INSERT_HEAD(&nblist, tn, link);

	num_active += 1;
	if(tn->index == index) {
		tn->height.Zero();
		tn->lnk_stat = LINK_DN;
		num_down += 1;
	}

	return tn;
}


int
TORADest::nb_del(nsaddr_t id)
{
	TORANeighbor *tn = nblist.lh_first;

	for( ; tn; tn = tn->link.le_next) {
		if(tn->index == id) {

			num_active -= 1;

			if(tn->lnk_stat == LINK_DN)
				num_down -=1;
			if(tn->lnk_stat == LINK_UP)
				num_up -= 1;


			LIST_REMOVE(tn, link);
			delete tn;

                        /*
                         *  Here's were we decide whether or not to
                         *  do route maintenance.
                         */
                        if(num_down == 0) {
				if(num_up > 0) {
					update_height(CURRENT_TIME,
						      index,
						      0,
						      0,
						      index);
				} else {
					update_height(-1,
						      -1,
						      -1,
						      -1,
						      index);
					// set height to NULL
				}

				agent->logNbDeletedLastDN(this);

				return 1;
				// send an UPDATE packet
			}
			return 0;
		}
	}
        return 0;
}


TORANeighbor*
TORADest::nb_find(nsaddr_t id)
{
	TORANeighbor *tn = nblist.lh_first;
	for( ; tn; tn = tn->link.le_next) {
		if(tn->index == id)
			return tn;
	}
	return 0;
}


TORANeighbor*
TORADest::nb_find_next_hop()
{
	TORANeighbor *tn = nblist.lh_first;
	TORANeighbor *tn_min = 0;

	for( ; tn; tn = tn->link.le_next) {
		if(tn->height.isNull())
                        continue;

                if(tn->lnk_stat != LINK_DN)
                        continue;

                if(tn_min == 0 || tn_min->height.compare(&tn->height) < 0) {
                        tn_min = tn;
                }
	}
	assert(tn_min == 0 || tn_min->lnk_stat == LINK_DN);
	return tn_min;
}


/*
 *  Find a neighbor of minimum height, subject to the constraint
 *  that height.r == R.
 */
TORANeighbor*
TORADest::nb_find_min_height(int R)
{
	TORANeighbor *tn = nblist.lh_first;
	TORANeighbor *tn_min = 0;

	for( ; tn; tn = tn->link.le_next) {
		if(tn->height.r == R) {
			if(tn_min == 0 ||
                           tn_min->height.compare(&tn->height) < 0)
				tn_min = tn;
		}
	}
	return tn_min;
}


/*
 *  Find a neighbor of minimum height, subject to the constraint
 *  that height.(tau,oid,r) == (tau,oid,r) and height != NULL
 */
TORANeighbor*
TORADest::nb_find_min_nonnull_height(Height *h)
{
	TORANeighbor *tn = nblist.lh_first;
	TORANeighbor *tn_min = 0;

	assert(h);

	for( ; tn; tn = tn->link.le_next) {
		if(tn->height.isNull() == 0 &&
		   tn->height.tau == h->tau &&
		   tn->height.oid == h->oid &&
		   tn->height.r == h->r) {
			if(tn_min == 0 ||
                           tn_min->height.compare(&tn->height) < 0)
		       	tn_min = tn;
		}
	}
	return tn_min;
}


/*
 *  Find a neighbor of maximum height, subject to the constraint
 *  that height != NULL.
 */
TORANeighbor*
TORADest::nb_find_max_height()
{
	TORANeighbor *tn = nblist.lh_first;
	TORANeighbor *tn_max = 0;

	for( ; tn; tn = tn->link.le_next) {
		if(tn->height.isNull() == 0) {
			if(tn_max == 0 ||
                           tn_max->height.compare(&tn->height) > 0)
				tn_max = tn;
		}
	}
	return tn_max;
}


// Verify that all neighbors of non-null height have the same reference
// level.
int
TORADest::nb_check_same_ref()
{
	TORANeighbor *tn = nblist.lh_first;
	TORANeighbor *tref = 0;

	for( ; tn; tn = tn->link.le_next) {
		if(tn->height.isNull() == 0) {
			if(tref == 0)
				tref = tn;
			else if(tref->height.tau != tn->height.tau ||
				tref->height.oid != tn->height.oid ||
				tref->height.r != tn->height.r)
				return 0;
		}
	}
	return 1;
}

	
void
TORADest::update_height_nb(TORANeighbor *tn, struct hdr_tora_upd *uh)
{
	Height h(uh->tu_id);

	h.tau = uh->tu_tau;
	h.oid = uh->tu_oid;
	h.r = uh->tu_r;
	h.delta = uh->tu_delta;

	tn->height.update(&h);

	/*
	 *  Update num_down/num_up
	 */
	if(tn->lnk_stat == LINK_DN)
		num_down -= 1;
	if(tn->lnk_stat == LINK_UP)
		num_up -= 1;

	tn->update_link_status(&height);

	/*
	 *  Update num_down/num_up
	 */
	if(tn->lnk_stat == LINK_DN)
		num_down += 1;
	if(tn->lnk_stat == LINK_UP)
		num_up += 1;
}


/*
 *  Updates the height of a destination and then recomputes
 *  all of the link status information for the neighbors.
 */
void
TORADest::update_height(double TAU, nsaddr_t OID, int R, int DELTA, nsaddr_t ID)
{
	TORANeighbor *tn = nblist.lh_first;

	height.tau = TAU;
	height.oid = OID;
	height.r = R;
	height.delta = DELTA;
	height.id = ID;

#ifdef LOGGING
        agent->log_dst_state_change(this);
#endif
	num_active = num_down = num_up = 0;

	for( ; tn; tn = tn->link.le_next) {

		tn->update_link_status(&height);

		num_active += 1;

		if(tn->lnk_stat == LINK_DN)
			num_down += 1;
		if(tn->lnk_stat == LINK_UP)
			num_up += 1;
	}
}

