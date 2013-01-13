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
   imep_stats.h
   $Id: imep_stats.h,v 1.2 1999/08/12 21:17:24 yaxu Exp $

   stats collected by the IMEP code
   */

#ifndef _cmu_imep_imep_stats_h
#define _cmu_imep_imep_stats_h

struct ImepStat {

  int new_in_adjacency;
  int new_neighbor;
  int delete_neighbor1;
  int delete_neighbor2;
  int delete_neighbor3;

  int qry_objs_created;
  int upd_objs_created;
  int clr_objs_created;

  int qry_objs_recvd;
  int upd_objs_recvd;
  int clr_objs_recvd;

  int num_objects_created;
  int sum_response_list_sz;
  int num_object_pkts_created;
  int num_object_pkts_recvd;
  int num_recvd_from_queue;

  int num_rexmitable_pkts;
  int num_rexmitable_fully_acked;
  int num_rexmitable_retired;
  int sum_rexmitable_retired_response_sz;
  int num_rexmits;

  int num_holes_created;
  int num_holes_retired;
  int num_unexpected_acks;
  int num_reseqq_drops;

  int num_out_of_window_objs;
  int num_out_of_order_objs;
  int num_in_order_objs;
};

#endif
