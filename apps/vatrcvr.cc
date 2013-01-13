/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linking this file statically or dynamically with other modules is making
 * a combined work based on this file.  Thus, the terms and conditions of
 * the GNU General Public License cover the whole combination.
 *
 * In addition, as a special exception, the copyright holders of this file
 * give you permission to combine this file with free software programs or
 * libraries that are released under the GNU LGPL and with code included in
 * the standard release of ns-2 under the Apache 2.0 license or under
 * otherwise-compatible licenses with advertising requirements (or modified
 * versions of such code, with unchanged license).  You may copy and
 * distribute such a system following the terms of the GNU GPL for this
 * file and the licenses of the other code concerned, provided that you
 * include the source code of that other code when and as the GNU GPL
 * requires distribution of source code.
 *
 * Note that people who make modified versions of this file are not
 * obligated to grant this special exception for their modified versions;
 * it is their choice whether to do so.  The GNU General Public License
 * gives permission to release a modified version without this exception;
 * this exception also makes it possible to release a modified version
 * which carries forward this exception.
 */

/*
 * Copyright (c) 1995 The Regents of the University of California.
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
 *      This product includes software developed by the Network Research
 *      Group at Lawrence Berkeley National Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*/

#ifndef lint
static const char  rcsid[] =
"@(#) $Header: /cvsroot/nsnam/ns-2/apps/vatrcvr.cc,v 1.7 2005/08/26 05:05:28 tomh Exp $";
#endif

#ifndef WIN32
#include <sys/time.h>
#endif
#include "agent.h"
#include "rtp.h"
#include "adaptive-receiver.h"
#include "vat.h"

//Most of this code is taken from the decoder.cc file of the publicly available
//vat code with some minor midifications.

class VatRcvr : public AdaptiveRcvr {
public:
	VatRcvr();
protected:
	int adapt(Packet *pkt, u_int32_t time);
	void count(int statno);
	u_int32_t hostoffset_;
	int32_t var_; //variance in  this host's interarrival time
	u_int32_t playout_; // playout delay (in media units)
	int maxdel_;
	int block_size_;
	int lecture_mode_;
	u_int32_t lastrecv_;
	u_int32_t predicted_drop_;
	int delvar_;
	/*XXX*/
#define MAXSTAT 16
	struct statcntr {
		const char* name;
		u_int cnt;
	} stat_[MAXSTAT];
	int nstat_;
};


inline void VatRcvr::count(int statno)
{
	++stat_[statno].cnt;
}


inline int absdiff(int x, int y)
{
	register int r = y - x;
	return (r < 0? -r : r);
}

static inline int newoffset(
	int nvar,
	int playout,
	int maxdel, int mindel, int lecture)
{
	register int offset = nvar;
	if (offset > maxdel)
		offset = maxdel;
	register int diff = playout - offset;

	if (diff > 0) {
		// offset going down: in LectureMode, drop at most
		// one frametime per talkspurt.  In ConferenceMode,
		// drop at most 1/2 of difference.
		if (lecture) {
			if (diff > FRAMESIZE) {
				if (playout > (maxdel * 3) / 4 &&
				    diff > 10 * FRAMESIZE)
					diff = 5 * FRAMESIZE;
				else
					diff = FRAMESIZE;
			}
		} else
			diff >>= 1;
		offset = playout - diff;
	} else if (-diff > maxdel) {
		// offset going way up: only allow 3/4 of max.
		offset = (maxdel * 3) / 4;
	}
	if (offset > (maxdel * 7) / 8)
		offset = (maxdel * 7) / 8;
	else if (offset < mindel)
		offset = mindel;
	return (offset);
}



int VatRcvr::adapt(Packet *pkt, u_int32_t local_clock)
{
	hdr_cmn* ch = hdr_cmn::access(pkt);
	register u_int32_t tstamp = (int)ch->timestamp();
	register int hoff = (int)hostoffset_;
	register int offset = (tstamp + hoff - local_clock) &~ 3;
	hdr_rtp *rh = hdr_rtp::access(pkt);
	int new_ts = rh->flags() & RTP_M ;

	//struct timeval tv;
	//static long int last;
	
	/*    printf("%1d %10d %10d\n", new_ts ? 1: 0, tstamp, local_clock);*/
	
	/* printf("%u\n", tstamp); */
	
	//gettimeofday(&tv, NULL);
	//last = tv.tv_usec - last;
	//if (last < 0)
	//	last += 1000000;
	/*
	  printf("%u %u %u ==> %d\n", tstamp, hoff, local_clock, offset);
	  printf("%u %d\n", tv.tv_usec, last);
	  */
	//last = tv.tv_usec;
	/* printf("%u ==> %d\n", local_clock, offset); */
	
	if (hoff == 0 || new_ts) {
		/*  if (hoff == 0) {  */
		
		/* printf("TS: var = %d playback = %d", var_ >> (VAR_FILTER + 3), 
		   playout_ >> (PLAYO_FILTER + 3)); */
		/*
		 * start of new talk spurt --
		 * use accumulated variance to compute new offset if
		 * this would make a significant change.  We change if
		 *  - the variance is currently 'small', or
		 *  - the change would be a least a packet time
		 */
		register int nvar = var_ >> (VAR_FILTER - VAR_MULT);
		offset = playout_ >> PLAYO_FILTER;
		if (nvar < 3*FRAMESIZE || absdiff(nvar, offset) >= FRAMESIZE) {
			offset = newoffset(nvar, offset, maxdel_, 
					   block_size_, lecture_mode_);
			/*
			 * assume that a talk spurt starts with TALK_LEAD
			 * samples of history & subtract them off if possible.
			 */
			
			/* CHANGED THIS PART OF VAT CODE AS WELL */
			if (new_ts) {
				offset -= 4 * FRAMESIZE;
				if (offset < block_size_)
					offset = block_size_;
				
			}
		}
		hostoffset_ = local_clock - tstamp + offset;
		/* printf(" new playback = %d\n", offset >> 3); */
	} else if (offset < 0 || offset > maxdel_) {
		/* printf("LP: late by %d var = %d playback = %d", (0 - (offset >> 3)), 
		   var_ >> (VAR_FILTER + 3), playout_ >> (PLAYO_FILTER + 3)); */
		/*
		 * packet out of range -- if last packet also out of
		 * range or if the delay would increase, resync.
		 */
		if (offset < 0 || predicted_drop_ == tstamp) {
			offset = newoffset(var_ >> (VAR_FILTER - VAR_MULT),
					   playout_ >> PLAYO_FILTER,
					   maxdel_, block_size_,
					   lecture_mode_);
			hostoffset_ = local_clock - tstamp + offset;
		} else {
			/* printf("late packet\n"); */
			predicted_drop_ = tstamp + block_size_;
			lastrecv_ = tstamp - local_clock;
			count(STAT_LATE);
			return (-1);
		}
		/* printf(" new playback = %d\n", offset >> 3); */
	} else {
		// packet in range, update interarrival var. est.
		register int nvar = var_;
		register int off = tstamp - local_clock - lastrecv_;
		/* printf("offset =  %3d", off >> 3);  */
		if (off < 0)
			off = -off;
		off -= (nvar >> VAR_FILTER);
		var_ = nvar + off;
		/* printf(" var =  %3d", var_ >> 8);  */
	}
	lastrecv_ = tstamp - local_clock;
	register u_int avgplay = playout_;
	playout_ = avgplay + (offset - (avgplay >> PLAYO_FILTER));
	/* printf(" offset = %3d avg playout = %3d", offset >> 3, playout_ >> 8);  */
	offset &= ~3;
	delvar_ = var_ >> VAR_FILTER;
	/* printf("\n"); */
	return (offset);
	
}



static class VatRcvrClass : public TclClass {
public:
	VatRcvrClass() : TclClass("Agent/VatRcvr") {}
	TclObject* create(int, const char*const*) {
		return (new VatRcvr());

	}
} class_vat_rcvr;


VatRcvr::VatRcvr() : 
	hostoffset_(0),
	var_(INITIAL_OFFSET << (VAR_FILTER - VAR_MULT)),
	playout_(INITIAL_OFFSET << PLAYO_FILTER),
	maxdel_(8000*6),
	block_size_(FRAMESIZE),
	lecture_mode_(0),
	lastrecv_(0),
	predicted_drop_(~0),
	nstat_(0)
{

	for (int i = 0; i < MAXSTAT; ++i) {
		stat_[i].name = 0;
		stat_[i].cnt = 0;
	}
	stat_[STAT_LATE].name = "Late-Pkts";
	nstat_ = 1;
}
