/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
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
/* Ported from CMU/Monarch's code, nov'98 -Padma.*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "rtable.h"

static int rtent_trich(const void *a, const void *b) {
  nsaddr_t ia = ((const rtable_ent *) a)->dst;
  nsaddr_t ib = ((const rtable_ent *) b)->dst;
  if (ia > ib) return 1;
  if (ib > ia) return -1;
  return 0;
}

RoutingTable::RoutingTable() {
  // Let's start with a ten element maxelts.
  elts = 0;
  maxelts = 10;
  rtab = new rtable_ent[maxelts];
}

void
RoutingTable::AddEntry(const rtable_ent &ent)
{
  rtable_ent *it;
  //DEBUG
  assert(ent.metric <= BIG);

  if ((it = (rtable_ent*) bsearch(&ent, rtab, elts, sizeof(rtable_ent), 
				 rtent_trich))) {
    bcopy(&ent,it,sizeof(rtable_ent));
    return;
    /*
    if (it->seqnum < ent.seqnum || it->metric > (ent.metric+em)) {
      bcopy(&ent,it,sizeof(rtable_ent));
      it->metric += em;
      return NEW_ROUTE_SUCCESS_OLDENT;
    } else {
      return NEW_ROUTE_METRIC_TOO_HIGH;
    }
    */
  }

  if (elts == maxelts) {
    rtable_ent *tmp = rtab;
    maxelts *= 2;
    rtab = new rtable_ent[maxelts];
    bcopy(tmp, rtab, elts*sizeof(rtable_ent));
    delete tmp;
  }
  
  int max;
  
  for (max=0;max<elts;max++) {
	  if (ent.dst < rtab[max].dst) {
		  break;
	  }
  }
  // Copy all the further out guys out yet another.
  // bcopy does not seem to quite work on sunos???
  
  //bcopy(&rtab[max], &rtab[max+1], sizeof(rtable_ent)*(elts-max));
  
  //if (elts) {
  int i = elts-1;
  while (i >= max) {
	  rtab[i+1] = rtab[i];
	  i--;
  }
  //}
  bcopy(&ent, &rtab[max], sizeof(rtable_ent));
  elts++;

  return;
}

void 
RoutingTable::InitLoop() {
  ctr = 0;
}

rtable_ent *
RoutingTable::NextLoop() {
  if (ctr >= elts)
    return 0;

  return &rtab[ctr++];
}

// Only valid (duh) as long as no new routes are added
int 
RoutingTable::RemainingLoop() {
  return elts-ctr;
}

rtable_ent *
RoutingTable::GetEntry(nsaddr_t dest) {
  rtable_ent ent;
  ent.dst = dest;
  return (rtable_ent *) bsearch(&ent, rtab, elts, sizeof(rtable_ent), 
				rtent_trich);
}
