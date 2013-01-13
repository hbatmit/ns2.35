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
 *	This product includes software developed by the Daedalus Research
 *	Group at the University of California Berkeley.
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
 *
 * Contributed by the Daedalus Research Group, http://daedalus.cs.berkeley.edu
 *
 */

#include "trafgen.h"
#include "agent.h"

// Need to initialize runnin as 0 to avoid the attempt to cancel 
//   unscheduled timer (xuanc 1/14/02)
// Fix contributed by Frank A. Zdarsky <frank.zdarsky@kom.tu-darmstadt.de>
TrafficGenerator::TrafficGenerator() : nextPkttime_(-1), running_(0), timer_(this)
{
}


void TrafficGenerator::start()
{
	init();
	running_ = 1;
	nextPkttime_ = next_interval(size_);
	timer_.resched(nextPkttime_);
	// Enable line below if you want to send immediately upon start
	//timeout(); 
}


void TrafficGenerator::stop()
{
	if (running_)
		timer_.cancel();
	running_ = 0;
}


void TrafficGenerator::timeout()
{
        if (! running_)
		return;

	/* send a packet */
	send(size_);
	/* figure out when to send the next one */
	nextPkttime_ = next_interval(size_);
	/* schedule it */
	if (nextPkttime_ > 0)
		timer_.resched(nextPkttime_);
	else
		running_ = 0;
}


void TrafficTimer::expire(Event *)
{
	tgen_->timeout();
}
