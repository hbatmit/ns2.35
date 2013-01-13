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
/* Ported from CMU/Monarch's code*/

/*
  $Id: tora_neighbor.cc,v 1.2 1999/08/12 21:12:34 yaxu Exp $
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
   TORANeighbor Class Functions
   ====================================================================== */
TORANeighbor::TORANeighbor(nsaddr_t id, Agent *a) : height(id)
{
	index = id;  
	lnk_stat = LINK_UN;
	time_act = Scheduler::instance().clock();

        agent = (toraAgent*) a;
}       


void
TORANeighbor::update_link_status(Height *h)
{
#ifdef LOGGING
        int t = lnk_stat;
#endif
	if(height.isNull())
		lnk_stat = LINK_UN;
	else if(h->isNull())
		lnk_stat = LINK_DN;
	else if(height.compare(h) > 0)
		lnk_stat = LINK_DN;
	else if(height.compare(h) < 0)
		lnk_stat = LINK_UP;
#ifdef LOGGING
        if(t != lnk_stat) {
                agent->log_nb_state_change(this);
        }
#endif
}

void
TORANeighbor::dump()
{
	fprintf(stdout, "\t\tNEIG: %d, Link Status: %d, Time Active: %f\n",
		index, lnk_stat, time_act);
	fprintf(stdout, "\t\t\ttau: %f, oid: %d, r: %d, delta: %d, id: %d\n",
		height.tau, height.oid, height.r, height.delta, height.id);
}

