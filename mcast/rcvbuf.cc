
/*
 * rcvbuf.cc
 * Copyright (C) 2001 by the University of Southern California
 * $Id: rcvbuf.cc,v 1.2 2005/08/25 18:58:08 johnh Exp $
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

/*
 * Pragmatic General Multicast (PGM), Reliable Multicast
 * Light-Weight Multicast Services (LMS), Reliable Multicast
 *
 * rcvbuf.cc
 *
 * Utility class used by receivers to provide for packet loss detection,
 * and to compute latency statistics for recovered packets.
 *
 * Christos Papadopoulos
 */

#include "config.h"

#include <stdio.h>
#include "rcvbuf.h"

RcvBuffer::RcvBuffer ()
{
	nextpkt_ = 0;
	maxpkt_ = -1;
	duplicates_ = 0;
	delay_sum_ = 0.0;
	max_delay_ = 0.0;
	min_delay_ = 1e6;
	pkts_recovered_ = 0;
	tail_ = gap_ = 0;
}

//
// add pkt to buffer with seqno i, received at time tnow
//
void RcvBuffer::add_pkt (int i, double tnow)
{
	Gap	*g, *pg;

	if (exists_pkt (i))
		{
		duplicates_++;
		return;
		}
	// common case first: got what we expected
	if (i == nextpkt_)
		{
		nextpkt_++;
		if (i == maxpkt_ + 1)
			maxpkt_++;
		return;
		}
	// new gap
	if (i > nextpkt_)
		{
		g = new Gap;
		g->start_ = nextpkt_;
		g->end_ = i-1;
		g->time_ = tnow;
		g->next_ = 0;

		if (!gap_)
			gap_ = tail_ = g;
		else	{
			tail_->next_ = g;
			tail_ = g;
			}
		nextpkt_ = i+1;
		return;
		}

	// i < nextpkt_ (a retransmission)

	pkts_recovered_++;

	// is packet part of the first gap?
	if (gap_->start_ <= i && i <= gap_->end_)
		{
		double	d = tnow - gap_->time_;

		if (d > max_delay_)
			max_delay_ = d;
		if (d < min_delay_)
			min_delay_ = d;
		delay_sum_ += d;
		if (i == maxpkt_ + 1)
			{
			maxpkt_++;
			if (++gap_->start_ > gap_->end_)
				{
				g = gap_;
				gap_ = gap_->next_;
				if (gap_)
					maxpkt_ = gap_->start_ - 1;
				else	{
					maxpkt_ = nextpkt_ - 1;
					tail_ = 0;
					}
				delete g;
				}
			return;
			}
		g = gap_;
		}
	else	{
		double	d;

		// locate gap this packet belongs to
		pg = gap_;
		g = gap_->next_;
		while (!(i >= g->start_ && i <= g->end_))
			{
			pg = g;
			g = g->next_;
			}
		d = tnow - g->time_;
		delay_sum_ += d;
		if (d > max_delay_) max_delay_ = d;
		if (d < min_delay_) min_delay_ = d;

		// first packet in gap
		if (g->start_ == i)
			{
			if (++g->start_ > g->end_)
				{
				pg->next_ = g->next_;
				if (tail_ == g)
					tail_ = pg;
				delete g;
				}
			return;
			}
		}
	// last packet in gap
	if (g->end_ == i)
		{
		g->end_--;
		return;
		}
	// a packet in the middle of gap
	pg = new Gap;
	pg->start_ = i+1;
	pg->end_   = g->end_;
	pg->time_  = g->time_;
	pg->next_  = g->next_;

	g->next_ = pg;
	g->end_  = i-1;
	if (tail_ == g)
		tail_ = pg;
}

//
// Return 0 if packet does not exist in the
// buffer, 1 if it does
//
int RcvBuffer::exists_pkt (int i)
{
	if (i <= maxpkt_)  return 1;
	if (i >= nextpkt_) return 0;

	Gap *g;
	for (g = gap_; g; g = g->next_)
		if (g->start_ <= i && i <= g->end_)
			return 0;
	return 1;
}

void RcvBuffer::print ()
{
	Gap	*g;

	printf ("maxpkt: %d ", maxpkt_);

	for (g = gap_; g; g = g->next_)
		printf ("(%d, %d) ", g->start_, g->end_);

	printf ("nextpkt: %d\n", nextpkt_);
}

