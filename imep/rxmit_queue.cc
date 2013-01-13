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

/* -*- c++ -*-
   rexmit_queue.cc
   $Id: rxmit_queue.cc,v 1.4 2000/08/18 18:34:02 haoboy Exp $
   */

#include <assert.h>

#include "packet.h"
#include "imep/rxmit_queue.h"

ReXmitQ::ReXmitQ()
{
  LIST_INIT(&head)
}
  
void 
ReXmitQ::insert(Time rxat, Packet *p, int num_rexmits)
{
  struct rexent *r = new rexent;
  r->rexmit_at = rxat;
  r->p = p;
  r->rexmits_left = num_rexmits;

  struct rexent *i;

  if (NULL == head.lh_first || rxat < head.lh_first->rexmit_at) 
    {
      LIST_INSERT_HEAD(&head, r, next);
      return;
    }

  for (i = head.lh_first ; i != NULL ; i = i->next.le_next )
    {
      if (rxat < i->rexmit_at) 
	{
	  LIST_INSERT_BEFORE(i, r, next);
	  return;
	}
      if (NULL == i->next.le_next)
	{
	  LIST_INSERT_AFTER(i, r, next);
	  return;
	}
    }
}

void
ReXmitQ::peekHead(Time *rxat, Packet **pp, int *rexmits_left)
{
  struct rexent *i;
  i = head.lh_first;
  if (NULL == i) {
    *rxat = -1; *pp = NULL; *rexmits_left = -1;
    return;
  }
  *rxat = i->rexmit_at;
  *pp = i->p; 
  *rexmits_left = i->rexmits_left;
}

void 
ReXmitQ::removeHead()
{
  struct rexent *i;
  i = head.lh_first;
  if (NULL == i) return;
  LIST_REMOVE(i, next);
  delete i;
}

void 
ReXmitQ::remove(Packet *p)
{
  struct rexent *i;
  for (i = head.lh_first ; i != NULL ; i = i->next.le_next )
    {
      if (p == i->p)
	{
	  LIST_REMOVE(i, next);
	  delete i;
	  return;
	}
    }
}
