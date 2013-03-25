/*
 * sfqCodel - The Controlled-Delay Active Queue Management algorithm
 * Copyright (C) 2011-2013 Kathleen Nichols <nichols@pollere.com>
 *
 * Portions of this source code were developed under contract with Cable
 * Television Laboratories, Inc.
 *
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
 *
 * If using this module, add the following lines to tcl/lib/ns-default.tcl:
 *
 *
 *	Queue/sfqCoDel set curq_ 0.0
 *	Queue/sfqCoDel set d_exp_ 0.0
 *	Queue/sfqCoDel set interval_ 0.1
 *	Queue/sfqCoDel set target_ .005
 *	Queue/sfqCoDel set maxbins_ 1024
 *	Queue/sfqCoDel set quantum_ 0
 */

#ifndef ns_sfqcodel_h
#define ns_sfqcodel_h

#include "queue.h"
#include <stdlib.h>
#include "agent.h"
#include "template.h"
#include "trace.h"

#define MAXBINS 1024


// we need a multi-valued return and C doesn't help
struct dodequeResult { Packet* p; int ok_to_drop; };

    struct bindesc {
        PacketQueue *q_;        // underlying FIFO queue
	int index;
        // Dynamic state used by algorithm
        double first_above_time_; // when we went (or will go) continuously above
                                  // target for interval
        double drop_next_;      // time to drop next packet (or when  dropped last)
        int count_;             // how many drops we've done since the last time
                            // we entered dropping state.
        int dropping_;          // = 1 if in dropping state.
	int deficit_;		// for rounding on bytes

        int newflag;
	int on_sched_;
        bindesc* prev;
        bindesc* next;
    } ;

class sfqCoDelQueue : public Queue {
  public:   
    sfqCoDelQueue();

/* The following lines were added by CableLabs for their purposes but
 * require other changes to ns-2 so are commented out for general use
         //adding a length() method to allow for length queries - JRP
    virtual int length() { return curlen_;}  // return number of pkts currently in
                         // queue i.e. total occupancy of all bins

        virtual int byteLength() { return curq_; }  // number of bytes
                         // currently in all bins
*/


  protected:
    // Stuff specific to the CoDel algorithm
    void enque(Packet* pkt);
    Packet* deque();

    bindesc bin_[MAXBINS];
    bindesc* binsched_;
//use these when call dodeque
    double first_above_time_;
    double drop_next_; 
    int count_;
    int dropping_;

    // Static state (user supplied parameters)
    double target_;         // target queue size (in time, same units as clock)
    double interval_;       // width of moving time window over which to compute min

    int maxpacket_;         // largest packet we've seen so far (this should be
                            // the link's MTU but that's not available in NS)
int mtu_max_;
    int curlen_;	    // the total occupancy of all bins in packets
    int maxbinid_;	    // id of bin with the most pkts
    int maxbins_;	    // for tcl override of MAXBINS (can only make smaller)
    int quantum_;	    // for rounding by bytes - set to 1510 in tcl file
    int isolate_;	    // for isolating cbrs. To isolate, set to 1, default 0

    // NS-specific junk
    int command(int argc, const char*const* argv);
    void reset();
    void trace(TracedVar*); // routine to write trace records

    Tcl_Channel tchan_;     // place to write trace records
    TracedInt curq_;        // current qlen in bytes seen by arrivals
    TracedDouble d_exp_;    // delay seen by most recently dequeued packet

  private:
    double control_law(double);
    dodequeResult dodeque(PacketQueue*);
    unsigned int hash(Packet*);
    bindesc* readybin();
    bindesc* removebin(bindesc*);
};

#endif
