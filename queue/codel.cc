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

#include <math.h>
#include <sys/types.h>
#include "config.h"
#include "template.h"
#include "random.h"
#include "flags.h"
#include "delay.h"
#include "codel.h"

static class CoDelClass : public TclClass {
  public:
    CoDelClass() : TclClass("Queue/CoDel") {}
    TclObject* create(int, const char*const*) {
        return (new CoDelQueue);
    }
} class_codel;

CoDelQueue::CoDelQueue() : tchan_(0)
{
    bind("interval_", &interval_);
    bind("target_", &target_);  // target min delay in clock ticks
    bind("curq_", &curq_);      // current queue size in bytes
    bind("d_exp_", &d_exp_);    // current delay experienced in clock ticks
    q_ = new PacketQueue();     // underlying queue
    pq_ = q_;
    reset();
}

void CoDelQueue::reset()
{
    curq_ = 0;
    d_exp_ = 0.;
    dropping_ = 0;
    first_above_time_ = -1;
    maxpacket_ = 256;
    count_ = 0;
    Queue::reset();
}

// Add a new packet to the queue.  The packet is dropped if the maximum queue
// size in pkts is exceeded. Otherwise just add a timestamp so dequeue can
// compute the sojourn time (all the work is done in the deque).

void CoDelQueue::enque(Packet* pkt)
{
    if(q_->length() >= qlim_) {
        // tail drop
        drop(pkt);
    } else {
        HDR_CMN(pkt)->ts_ = Scheduler::instance().clock();
        q_->enque(pkt);
    } 
}

// return the time of the next drop relative to 't'
double CoDelQueue::control_law(double t)
{
    return t + interval_ / sqrt(count_);
}

// Internal routine to dequeue a packet. All the delay and min tracking
// is done here to make sure it's done consistently on every dequeue.
dodequeResult CoDelQueue::dodeque()
{
    double now = Scheduler::instance().clock();
    dodequeResult r = { NULL, 0 };

    r.p = q_->deque();
    if (r.p == NULL) {
        curq_ = 0;
        first_above_time_ = 0;
    } else {
        // d_exp_ and curq_ are ns2 'traced variables' that allow the dynamic
        // queue behavior that drives CoDel to be captured in a trace file for
        // diagnostics and analysis.  d_exp_ is the sojourn time and curq_ is
        // the current q size in bytes.
        d_exp_ = now - HDR_CMN(r.p)->ts_;
        curq_ = q_->byteLength();

        if (maxpacket_ < HDR_CMN(r.p)->size_)
            // keep track of the max packet size.
            maxpacket_ = HDR_CMN(r.p)->size_;

        // To span a large range of bandwidths, CoDel essentially runs two
        // different AQMs in parallel. One is sojourn-time-based and takes
        // effect when target_ is larger than the time it takes to send a
        // TCP MSS packet. The 1st term of the "if" does this.
        // The other is backlog-based and takes effect when the time to send an
        // MSS packet is >= target_. The goal here is to keep the output link
        // utilization high by never allowing the queue to get smaller than
        // the amount that arrives in a typical interarrival time (one MSS-sized
        // packet arriving spaced by the amount of time it takes to send such
        // a packet on the bottleneck). The 2nd term of the "if" does this.
        if (d_exp_ < target_ || curq_ <= maxpacket_) {
            // went below - stay below for at least interval
            first_above_time_ = 0;
        } else {
            if (first_above_time_ == 0) {
                // just went above from below. if still above at first_above_time,
                // will say it’s ok to drop
                first_above_time_ = now + interval_;
            } else if (now >= first_above_time_) {
                r.ok_to_drop = 1;
            }
        }
    }
    return r;
}

// All of the work of CoDel is done here. There are two branches: In packet
// dropping state (meaning that the queue sojourn time has gone above target
// and hasn’t come down yet) check if it’s time to leave or if it’s time for
// the next drop(s). If not in dropping state, decide if it’s time to enter it
// and do the initial drop.

Packet* CoDelQueue::deque()
{
    double now = Scheduler::instance().clock();;
    dodequeResult r = dodeque();

    if (dropping_) {
        if (! r.ok_to_drop) {
            // sojourn time below target - leave dropping state
            dropping_ = 0;
        }
        // It’s time for the next drop. Drop the current packet and dequeue
        // the next.  If the dequeue doesn't take us out of dropping state,
        // schedule the next drop. A large backlog might result in drop
        // rates so high that the next drop should happen now, hence the
        // ‘while’ loop.
        while (now >= drop_next_ && dropping_) {
            drop(r.p);
            ++count_;
            r = dodeque();
            if (! r.ok_to_drop) {
                // leave dropping state
                dropping_ = 0;
            } else {
                // schedule the next drop.
                drop_next_ = control_law(drop_next_);
            }
        }

    // If we get here we’re not in dropping state. 'ok_to_drop' means that the
    // sojourn time has been above target for interval so enter dropping state.
    } else if (r.ok_to_drop) {
        drop(r.p);
        r = dodeque();
        dropping_ = 1;

        // If min went above target close to when it last went below,
        // assume that the drop rate that controlled the queue on the
        // last cycle is a good starting point to control it now.
        count_ = (count_ > 1 && now - drop_next_ < 16*interval_)? count_ - 1 : 1;
        drop_next_ = control_law(now);
    }
    return (r.p);
}

int CoDelQueue::command(int argc, const char*const* argv)
{
    Tcl& tcl = Tcl::instance();
    if (argc == 2) {
        if (strcmp(argv[1], "reset") == 0) {
            reset();
            return (TCL_OK);
        }
    } else if (argc == 3) {
        // attach a file for variable tracing
        if (strcmp(argv[1], "attach") == 0) {
            int mode;
            const char* id = argv[2];
            tchan_ = Tcl_GetChannel(tcl.interp(), (char*)id, &mode);
            if (tchan_ == 0) {
                tcl.resultf("CoDel trace: can't attach %s for writing", id);
                return (TCL_ERROR);
            }
            return (TCL_OK);
        }
        // connect CoDel to the underlying queue
        if (!strcmp(argv[1], "packetqueue-attach")) {
            delete q_;
            if (!(q_ = (PacketQueue*) TclObject::lookup(argv[2])))
                return (TCL_ERROR);
            else {
                pq_ = q_;
                return (TCL_OK);
            }
        }
    }
    return (Queue::command(argc, argv));
}

// Routine called by TracedVar facility when variables change values.
// Note that the tracing of each var must be enabled in tcl to work.
void
CoDelQueue::trace(TracedVar* v)
{
    const char *p;

    if (((p = strstr(v->name(), "curq")) == NULL) &&
        ((p = strstr(v->name(), "d_exp")) == NULL) ) {
        fprintf(stderr, "CoDel: unknown trace var %s\n", v->name());
        return;
    }
    if (tchan_) {
        char wrk[500];
        double t = Scheduler::instance().clock();
        if(*p == 'c') {
            sprintf(wrk, "c %g %d", t, int(*((TracedInt*) v)));
        } else if(*p == 'd') {
            sprintf(wrk, "d %g %g %d %g", t, double(*((TracedDouble*) v)), count_,
                    count_? control_law(0.)*1000.:0.);
        }
        int n = strlen(wrk);
        wrk[n] = '\n'; 
        wrk[n+1] = 0;
        (void)Tcl_Write(tchan_, wrk, n+1);
    }
}
