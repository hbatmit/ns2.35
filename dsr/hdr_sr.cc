
/*
 * hdr_sr.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: hdr_sr.cc,v 1.6 2005/08/25 18:58:04 johnh Exp $
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
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
// Ported from CMU/Monarch's code, appropriate copyright applies.  
/* -*- c++ -*-
   hdr_sr.cc

   source route header
*/

#include <stdio.h>
#include "hdr_sr.h"

int hdr_sr::offset_;

static class SRHeaderClass : public PacketHeaderClass {
public:
	SRHeaderClass() : PacketHeaderClass("PacketHeader/SR",
					     sizeof(hdr_sr)) {
		offset(&hdr_sr::offset_);

#ifdef DSR_CONST_HDR_SZ
		fprintf(stderr,"WARNING: DSR treating all source route headers\n"
			"as having length %d. this should be used only to estimate effect\n"
			"of no longer needing a src rt in each packet\n",SR_HDR_SZ);
#endif

	}
#if 0
	void export_offsets() {
		field_offset("valid_", OFFSET(hdr_sr, valid_));
		field_offset("num_addrs_", OFFSET(hdr_sr, num_addrs_));
		field_offset("cur_addr_", OFFSET(hdr_sr, cur_addr_));
	}
#endif
} class_SRhdr;

char *
hdr_sr::dump()
{
  static char buf[100];
  dump(buf);
  return (buf);
}

void
hdr_sr::dump(char *buf)
{
  char *ptr = buf;
  *ptr++ = '[';
  for (int i = 0; i < num_addrs_; i++)
    {
      ptr += sprintf(ptr, "%s%d ",
		     (i == cur_addr_) ? "|" : "",
		     addrs()[i].addr);
    }
  *ptr++ = ']';
  *ptr = '\0';
}
