/*
 * Codel - The Controlled-Delay Active Queue Management algorithm
 * Copyright (C) 2011-2012 Kathleen Nichols <nichols@pollere.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * Alternatively, provided that this notice is retained in full, this
 * software may be distributed under the terms of the GNU General
 * Public License ("GPL") version 2, in which case the provisions of the
 * GPL apply INSTEAD OF those given above.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ns_codel_h
#define ns_codel_h

#include "queue.h"
#include <stdlib.h>
#include "agent.h"
#include "template.h"
#include "trace.h"

// we need a multi-valued return and C doesn't help
struct dodequeResult { Packet* p; int ok_to_drop; };

class CoDelQueue : public Queue {
  public:   
    CoDelQueue();
  protected:
    // Stuff specific to the CoDel algorithm
    void enque(Packet* pkt);
    Packet* deque();

    // Static state (user supplied parameters)
    double target_;         // target queue size (in time, same units as clock)
    double interval_;       // width of moving time window over which to compute min

    // Dynamic state used by algorithm
    double first_above_time_; // when we went (or will go) continuously above
                              // target for interval
    double drop_next_;      // time to drop next packet (or when we dropped last)
    int count_;             // how many drops we've done since the last time
                            // we entered dropping state.
    int dropping_;          // = 1 if in dropping state.
    int maxpacket_;         // largest packet we've seen so far (this should be
                            // the link's MTU but that's not available in NS)

    // NS-specific junk
    int command(int argc, const char*const* argv);
    void reset();
    void trace(TracedVar*); // routine to write trace records

    PacketQueue *q_;        // underlying FIFO queue
    Tcl_Channel tchan_;     // place to write trace records
    TracedInt curq_;        // current qlen seen by arrivals
    TracedDouble d_exp_;    // delay seen by most recently dequeued packet

  private:
    double control_law(double);
    dodequeResult dodeque();
};

#endif
