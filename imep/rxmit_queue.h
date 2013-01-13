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
   rexmit_queue.h
   $Id: rxmit_queue.h,v 1.4 2002/05/06 22:23:15 difa Exp $
   */

#ifndef imep_rexmit_queue_h
#define imep_rexmit_queue_h

#include <packet.h>
#include "lib/bsd-list.h"

typedef double Time;

struct rexent {
  double rexmit_at;
  int rexmits_left;
  Packet *p;
  LIST_ENTRY(struct rexent) next;
};

LIST_HEAD(rexent_head, rexent);

class ReXmitQ;

class ReXmitQIter {
  friend class ReXmitQ;
  
public:
  inline Packet * next() {
    if (0 == iter) return 0;
    struct rexent *t = iter;
    iter = iter->next.le_next;
    return t->p;
  }
  
private:
  ReXmitQIter(rexent *r) : iter(r) {};
  struct rexent * iter;
};

class ReXmitQ {
public:
  ReXmitQ();
  
  void insert(Time rxat, Packet *p, int num_rexmits);
  void peekHead(Time *rxat, Packet **pp, int *rexmits_left);
  void removeHead();
  void remove(Packet *p);
  inline ReXmitQIter iter() {
    return ReXmitQIter(head.lh_first); 
  }

private:
  rexent_head head;
};

#endif
