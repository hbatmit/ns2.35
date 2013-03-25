/*
 * sfqCodel - The Controlled-Delay Active Queue Management algorithm
 * with stochastic binning. Inspired by Eric Dumazaet's linux code.
 * This module was put together by Kathleen Nichols.
 * Packets are hashed into a maximum of 1024 bins based on header
 * Bins each have a separate CoDel manager
 * For expediency, this originally combined codel.cc and some aspects of
 * sfq.cc from the ns2 distribution, contributed by Curtis Villamizar, Feb 1997.
 * A notable difference from Eric's code is that this uses packet-by-packet
 * round-robining, which we believe to be more appropriate, but need to test.
 * Van Jacobson contributed the hash to attempt to model linux kernel hash.
 * This is experimental code, for implementation, see Dumazaet's code. 
 *
 * Copyright (C) 2011-2013 Kathleen Nichols <nichols@pollere.com>
*
 * Portions of this source code were developed under contract with Cable
 * Television Laboratories, Inc.
 *
 * Changes made by Kathleen Nichols for Cable Labs 12/2012 - 1/2013
 * tcl lines can be used to set the number of bins smaller (maxbins_)
 * When the total buffer limit is exceeded, can either head or tail drop
 * from the fullest (in packets) bin.
 * This is useful in presence of nonconforming or large flows
 * This version drops from tail but can easily be changed to head to match
 * fq_codel though this has implementation problems in that it requres locks.
 * Added a quantum_ variable which, when non-zero, rounds by that number
 * of bytes instead of per packet
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
 */

#include <math.h>
#include <sys/types.h>
#include "config.h"
#include "template.h"
#include "random.h"
#include "flags.h"
#include "delay.h"
#include "sfqcodel.h"

//#include "docsislink.h"

static class sfqCoDelClass : public TclClass {
  public:
    sfqCoDelClass() : TclClass("Queue/sfqCoDel") {}
    TclObject* create(int, const char*const*) {
        return (new sfqCoDelQueue);
    }
} class_codel;

sfqCoDelQueue::sfqCoDelQueue() :  maxbins_(MAXBINS), quantum_(0), tchan_(0), isolate_(0)
{
    bind("interval_", &interval_);
    bind("target_", &target_);  // target min delay in clock ticks
    bind("curq_", &curq_);      // current queue size in bytes
    bind("d_exp_", &d_exp_);    // current delay experienced in clock ticks
    bind("maxbins_", &maxbins_);    // tcl settable max number of bins
    bind("quantum_", &quantum_);    // tcl settable quantum value for byte rounding
					// if zero, rounds by packets
   if (maxbins_ > MAXBINS)  {
        printf("sfqCoDel: maxbins_ of %d exceeds upper bound of %d", maxbins_, MAXBINS);
	exit(0);
    }

    for(int i=0; i<maxbins_; i++) {
     bin_[i].q_ = new PacketQueue();
     bin_[i].first_above_time_ = -1;
     bin_[i].dropping_ = 0;
     bin_[i].count_ = 0;
     bin_[i].newflag = 0;
     bin_[i].index = i;
     bin_[i].on_sched_ = 0;
    }
    pq_ = bin_[0].q_; //does ns need this?
    reset();
}

void sfqCoDelQueue::reset()
{
    binsched_ = NULL;
    curq_ = 0;
    d_exp_ = 0.;
    maxpacket_ = 256;
    maxbinid_ = 0;
    curlen_ = 0;
    curq_ = 0;
    isolate_ = 0;
    mtu_max_ = maxpacket_;
    Queue::reset();
}

// Add a new packet to the queue. If the entire buffer space is full, drop from
// the largest queue (in packets) first.
// Add a timestamp so dequeue can compute the sojourn time (all the work is done in the deque)
// and enqueue.

void sfqCoDelQueue::enque(Packet* pkt)
{
	// check for full buffer
	if(curlen_ >= qlim_) {
	   //total buffer is full, drop from largest so find maxbin
	   // maxbinid_ is either 0 or the id of the last max
	   // trying to make sure pick a different bin of the same size
	   int m = maxbinid_;
	   for(int j=0; j<maxbins_; j++)
	       if( j != maxbinid_ && bin_[j].on_sched_ && (bin_[j].q_)->length() >= (bin_[m].q_)->length()) {
		   m = j;
	   }
	   maxbinid_ = m;

	   //tail drop from maxbinid_ (use head() for td)
	   Packet* p = (bin_[maxbinid_].q_)->tail();
	   curq_ -= HDR_CMN(p)->size();
	   curlen_--;
	   (bin_[maxbinid_].q_)->remove(p);	//where packet is actually removed
	   drop(p);
	}

	HDR_CMN(pkt)->ts_ = Scheduler::instance().clock();
	curlen_++;
	curq_ += HDR_CMN(pkt)->size();

	// experimental code to isolate packets marked CBR which are
	// all voice packets in our simulations
	unsigned int i;
	if(isolate_) {
	if(HDR_CMN(pkt)->ptype() == PT_CBR)
		i = (maxbins_ -1);
	else
		i = hash(pkt) % (maxbins_ - 1);
	} else
        // Determine which bin to enqueue the packet and add it to that bin.
	// the bin's id comes from a hash of its header
	i = hash(pkt) % maxbins_;
// printf("flow_id %d bin %d\n", (hdr_ip::access(pkt))->flowid(), i);
	(bin_[i].q_)->enque(pkt);

	//if it's the only bin in use, set binsched_
        if(binsched_ == NULL) {
          binsched_ = &(bin_[i]);
          bin_[i].prev = &(bin_[i]);
          bin_[i].next = bin_[i].prev;
          bin_[i].newflag = 1;
          bin_[i].on_sched_ = 1;
	  if(quantum_ > 0)	//if rounding by bytes
             bin_[i].deficit_ = quantum_;
        } else if( bin_[i].on_sched_ == 0) {
	//if bin was not on the schedule, add to the list before continuing
        // bins but after other new bins
          bindesc* b = binsched_;
          while(b->newflag == 1) {
            if(b->next == binsched_) break;
            b = b->next;
          }
          if(b->next == binsched_) {
             //insert at end
             b->next = &(bin_[i]);
             bin_[i].prev = b;
             bin_[i].next = binsched_;
             binsched_->prev = &(bin_[i]);
          } else {
	     //insert in front of b
             bin_[i].next = b;
             bin_[i].prev = b->prev;
             (b->prev)->next = &(bin_[i]);
             b->prev = &(bin_[i]);
	     if(b == binsched_)
		binsched_ = &(bin_[i]);
          }
          bin_[i].newflag = 1;
          bin_[i].on_sched_ = 1;
	  if(quantum_ > 0)	//if rounding by bytes
             bin_[i].deficit_ = quantum_;
	}
}

extern "C" {
/* lookup3.c, by Bob Jenkins, May 2006, Public Domain.  */

//#include <stdint.h>     /* defines uint32_t etc */
//#include <sys/param.h>  /* attempt to define endianness */
//#ifdef linux
//# include <endian.h>    /* attempt to define endianness */
//#endif

/*
 * My best guess at if you are big-endian or little-endian.  This may
 * need adjustment.
 */
#if (defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && \
     __BYTE_ORDER == __LITTLE_ENDIAN) || \
    (defined(i386) || defined(__i386__) || defined(__i486__) || \
     defined(__i586__) || defined(__i686__) || defined(vax) || defined(MIPSEL))
# define HASH_LITTLE_ENDIAN 1
# define HASH_BIG_ENDIAN 0
#elif (defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && \
       __BYTE_ORDER == __BIG_ENDIAN) || \
      (defined(sparc) || defined(POWERPC) || defined(mc68000) || defined(sel))
# define HASH_LITTLE_ENDIAN 0
# define HASH_BIG_ENDIAN 1
#else
# define HASH_LITTLE_ENDIAN 0
# define HASH_BIG_ENDIAN 0
#endif

#define hashsize(n) ((u_int32_t)1<<(n))
#define hashmask(n) (hashsize(n)-1)
#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

#define mix(a,b,c) \
{ \
  a -= c;  a ^= rot(c, 4);  c += b; \
  b -= a;  b ^= rot(a, 6);  a += c; \
  c -= b;  c ^= rot(b, 8);  b += a; \
  a -= c;  a ^= rot(c,16);  c += b; \
  b -= a;  b ^= rot(a,19);  a += c; \
  c -= b;  c ^= rot(b, 4);  b += a; \
}

#define final(a,b,c) \
{ \
  c ^= b; c -= rot(b,14); \
  a ^= c; a -= rot(c,11); \
  b ^= a; b -= rot(a,25); \
  c ^= b; c -= rot(b,16); \
  a ^= c; a -= rot(c,4);  \
  b ^= a; b -= rot(a,14); \
  c ^= b; c -= rot(b,24); \
}

static inline u_int32_t jhash_3words( u_int32_t a, u_int32_t b, u_int32_t c, u_int32_t i) {
    i += 0xdeadbeef + (3 << 2);
    a += i;
    b += i;
    c += i;
    mix(a, b, c);
    final(a, b, c);
    return c;
}

#undef mix
#undef final

}

unsigned int sfqCoDelQueue::hash(Packet* pkt)
{
  hdr_ip* iph = hdr_ip::access(pkt);
  return jhash_3words(iph->daddr(), iph->saddr(),
                      (iph->dport() << 16) | iph->sport(), 0);
}

// return the time of the next drop relative to 't'
double sfqCoDelQueue::control_law(double t)
{
    return t + interval_ / sqrt(count_);
}

// Internal routine to dequeue a packet. All the delay and min tracking
// is done here to make sure it's done consistently on every dequeue.
dodequeResult sfqCoDelQueue::dodeque(PacketQueue* q)
{
    double now = Scheduler::instance().clock();
    dodequeResult r = { NULL, 0 };

    r.p = q->deque();
    if (r.p == NULL) {
	first_above_time_ = 0;
    } else {
        // d_exp_ and curq_ are ns2 'traced variables' that allow the dynamic
        // queue behavior that drives CoDel to be captured in a trace file for
        // diagnostics and analysis.  d_exp_ is the sojourn time and curq_ is
        // the current total q size in bytes.
        d_exp_ = now - HDR_CMN(r.p)->ts_;
        curlen_--;
        curq_ -= HDR_CMN(r.p)->size_;
	maxpacket_ = mtu_max_;
        if (maxpacket_ < HDR_CMN(r.p)->size_)
            // keep track of the max packet size.
            maxpacket_ = HDR_CMN(r.p)->size_;

	mtu_max_ = maxpacket_;
	/* experimental code to work with docsis mac model and not drop
	   packets with pending tokens (see note on bursty macs)
	int tk = DocsisLink::tokens_;
	if(tk > maxpacket_)
	{
                maxpacket_ = tk;
	}
	*/

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
	// Note that we use the overall value of curq_, across all bins
	// Add a test for this queue being empty though

        if (d_exp_ < target_ || curq_ <= maxpacket_ || q->length() == 0) {
            // went below - stay below for at least interval
            first_above_time_ = 0;
        } else {
            if (first_above_time_ == 0) {
                //just went above from below. if still above at first_above_time
                // will say it’s ok to drop
                 first_above_time_ = now + interval_;
            } else if (now >= first_above_time_) {
                r.ok_to_drop = 1;
            }
        }
    }
    return r;
}

// Get the next scheduled bin that has a non-empty queue and return its
// pointer. If it's not binsched_, update binsched_ to be this value.
// A null pointer means there is nothing to send on any bin.

bindesc* sfqCoDelQueue::readybin()
{
    //get the next scheduled bin that has a non-empty queue,
    // set the binsched_ to that bin,
    // if none,  return NULL
    if(binsched_ == NULL) return NULL;

    bindesc* b = binsched_;
    while((b->q_)->length() == 0) {
	b = removebin(b);	//clean up, remove bin from schedule
	if(b == NULL) return b;
    }

    // if get here, b = binsched_ and points to a non-empty queue
    //if rounding by packets, return now
    if(quantum_ == 0)
       return binsched_;

    // guaranteed that at least this bin has a packet. If it has a deficit, it
    // will get more bytes go to end. If it's the only one, it will get sent 
    while(b->deficit_ <= 0) {
	b->deficit_ += quantum_;
	b = b->next;
  	while((b->q_)->length() == 0) {	// find next non-empty bin
         b = b->next;
	}
    }
    binsched_ = b;

    return binsched_;
}

bindesc* sfqCoDelQueue::removebin(bindesc* b) {

      //clean up, remove bin from schedule
      if(b->next == b) {
         //means this was the only bin on the schedule, so empty the schedule
         b->next = NULL;
         b->prev = NULL;
	 b->dropping_ = 0;
	 b->on_sched_ = 0;
         binsched_ = NULL;
      } else {
	 b->dropping_ = 0;
	 b->on_sched_ = 0;
         binsched_ = b->next;
         binsched_->prev = b->prev;
         (b->prev)->next = binsched_;
         b->next = b->prev = NULL;
         b = binsched_;
      }
      return binsched_;
}

// All of the work of CoDel is done here. There are two branches: In packet
// dropping state (meaning that the queue sojourn time has gone above target
// and hasn’t come down yet) check if it’s time to leave or if it’s time for
// the next drop(s). If not in dropping state, decide if it’s time to enter it
// and do the initial drop.

Packet* sfqCoDelQueue::deque()
{
    double now = Scheduler::instance().clock();
    bindesc* b;
    dodequeResult r;

   do {
   //have to check all bins until find a packet (or there are none)
    if( (b = readybin()) == NULL) return NULL;

    //set up to use the same dodeque() as regular CoDel
    b->newflag = 0;
    first_above_time_ = b->first_above_time_;
    drop_next_ = b->drop_next_;
    count_ = b->count_;
    dropping_ = b->dropping_;
    r = dodeque( b->q_ );
    b->newflag = 0;

    //There has to be a packet because readybin() returned a bin
    if(r.p == NULL) printf("sfqCoDelQueue::deque(): error\n");

    if (dropping_) {
        if (! r.ok_to_drop) {
            // sojourn time below target - leave dropping state
	    //    and send this bin's packet
            dropping_ = 0;
        }
        // It’s time for the next drop. Drop the current packet and dequeue
        // the next.  If the dequeue doesn't take us out of dropping state,
        // schedule the next drop. A large backlog might result in drop
        // rates so high that the next drop should happen now, hence the
        // ‘while’ loop.
        while (now >= drop_next_ && dropping_) {
            drop(r.p);
            r = dodeque(b->q_);

	    //if drop emptied queue, it gets to be new on next arrival
	    // and want to move on to next bin to find a packet to send
	    if(r.p == NULL) {
    		b->count_ = count_;
    		b->drop_next_ = drop_next_;
    		b->first_above_time_ = 0;
		removebin(b);
	    }

            if (! r.ok_to_drop) {
                // leave dropping state
                dropping_ = 0;
            } else {
                // schedule the next drop.
                ++count_;
                drop_next_ = control_law(drop_next_);
            }
        }

    // If we get here we’re not in dropping state. 'ok_to_drop' means that the
    // sojourn time has been above target for interval so enter dropping state.
    } else if (r.ok_to_drop) {
        drop(r.p);
        r = dodeque(b->q_);
        dropping_ = 1;

	//if drop emptied bin's queue, it gets to be new on next arrival
	// and want to move on to next bin to find a packet to send
	if(r.p == NULL) {
    		b->count_ = count_;
    		b->drop_next_ = drop_next_;
    		b->first_above_time_ = 0;
		removebin(b);
	}

        // If min went above target close to when it last went below,
        // assume that the drop rate that controlled the queue on the
        // last cycle is a good starting point to control it now.
        count_ = (count_ > 2 && now - drop_next_ < 8*interval_)? count_ - 2 : 1;
        drop_next_ = control_law(now);
    }
   } while (r.p == NULL) ;

    //make sure the bin state gets updated
    if(r.p != NULL) {
	b->count_ = count_;
    	b->first_above_time_ = first_above_time_;
    	b->drop_next_ = drop_next_;
    	b->dropping_ = dropping_;
    }

    if(quantum_) { //for rounding on bytes, don't advance binsched_
       b->deficit_ -= HDR_CMN(r.p)->size();
    } else

    // There's a packet to send so need to advance bin schedule.
    // so that binsched_ will be next one to send (or NULL)
     if(binsched_ != NULL) binsched_ = binsched_->next;

    return (r.p);
}

int sfqCoDelQueue::command(int argc, const char*const* argv)
{
    Tcl& tcl = Tcl::instance();

    if (argc == 2) {
        if (strcmp(argv[1], "reset") == 0) {
            reset();
            return (TCL_OK);
        } else if (strcmp(argv[1], "isolate-cbr") == 0) {
	    isolate_ = 1;
            return (TCL_OK);
        }
    } else if (argc == 3) {
        // attach a file for variable tracing
        if (strcmp(argv[1], "attach") == 0) {
            int mode;
            const char* id = argv[2];
            tchan_ = Tcl_GetChannel(tcl.interp(), (char*)id, &mode);
            if (tchan_ == 0) {
                tcl.resultf("sfqCoDel trace: can't attach %s for writing", id);
                return (TCL_ERROR);
            }
            return (TCL_OK);
        }
        // connect CoDel to the underlying queue
        if (!strcmp(argv[1], "packetqueue-attach")) {
//            delete q_;
//            if (!(q_ = (PacketQueue*) TclObject::lookup(argv[2])))
printf("error in command\n");
                return (TCL_ERROR);
//            else {
//                pq_ = q_;
//                return (TCL_OK);
//            }
        }
    }
    return (Queue::command(argc, argv));
}

// Routine called by TracedVar facility when variables change values.
// Note that the tracing of each var must be enabled in tcl to work.
void
sfqCoDelQueue::trace(TracedVar* v)
{
    const char *p;

    if (((p = strstr(v->name(), "curq")) == NULL) &&
        ((p = strstr(v->name(), "d_exp")) == NULL) ) {
        fprintf(stderr, "sfqCoDel: unknown trace var %s\n", v->name());
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
