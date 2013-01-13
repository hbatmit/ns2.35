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
   imep_rt.cc
   $Id

   Routing Protocol (or more generally, ULP) Specific Functions
   */

#include <imep/imep.h>
#include <tora/tora_packet.h>
#include <packet.h>

// ======================================================================
// computes the length of a TORA header, copies the header to "dst"
// and returns the length of the header in "length".

int
imepAgent::toraHeaderLength(struct hdr_tora *t)
{
	switch(t->th_type) {
	case TORATYPE_QRY:
		return sizeof(struct hdr_tora_qry);
		break;
	case TORATYPE_UPD:
		return sizeof(struct hdr_tora_upd);
		break;
	case TORATYPE_CLR:
		return sizeof(struct hdr_tora_clr);
		break;
	default:
		abort();
	}
	return 0; /* Make msvc happy */
}

void
imepAgent::toraExtractHeader(Packet *p, char* dst)
{
  struct hdr_tora *t = HDR_TORA(p);
  int length = toraHeaderLength(t);
  bcopy((char*) t, dst, length);

  switch(t->th_type) {
  case TORATYPE_QRY:
    stats.qry_objs_created++;
    break;
  case TORATYPE_UPD:
    stats.upd_objs_created++;
    break;
  case TORATYPE_CLR:
    stats.clr_objs_created++;
    break;
  default:
    abort();
  }
}

void
imepAgent::toraCreateHeader(Packet *p, char *src, int length)
{
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_tora *t = HDR_TORA(p);

  ch->size() = IP_HDR_LEN + length;
  ch->ptype() = PT_TORA;

  bcopy(src, (char*) t, length);

  switch(t->th_type) {
  case TORATYPE_QRY:
    stats.qry_objs_recvd++;
    break;
  case TORATYPE_UPD:
    stats.upd_objs_recvd++;
    break;
  case TORATYPE_CLR:
    stats.clr_objs_recvd++;
    break;
  default:
    abort();
  }
}
