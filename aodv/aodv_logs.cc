/*
Copyright (c) 1997, 1998 Carnegie Mellon University.  All Rights
Reserved. 

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The AODV code developed by the CMU/MONARCH group was optimized and tuned by Samir Das and Mahesh Marina, University of Cincinnati. The work was partially done in Sun Microsystems.
 
*/

#include <aodv/aodv.h>
#include <aodv/aodv_packet.h>
#include <ip.h>

#define CURRENT_TIME    Scheduler::instance().clock()

static const int verbose = 0;

/* =====================================================================
   Logging Functions
   ===================================================================== */
void
AODV::log_link_del(nsaddr_t dst)
{
        static int link_del = 0;

        if(! logtarget || ! verbose) return;

        /*
         *  If "god" thinks that these two nodes are still
         *  reachable then this is an erroneous deletion.
         */
        sprintf(logtarget->pt_->buffer(),
                "A %.9f _%d_ deleting LL hop to %d (delete %d is %s)",
                CURRENT_TIME,
                index,
                dst,
                ++link_del,
                God::instance()->hops(index, dst) != 1 ? "VALID" : "INVALID");
        logtarget->pt_->dump();
}


void
AODV::log_link_broke(Packet *p)
{
        static int link_broke = 0;
        struct hdr_cmn *ch = HDR_CMN(p);

        if(! logtarget || ! verbose) return;

        sprintf(logtarget->pt_->buffer(),
                "A %.9f _%d_ LL unable to deliver packet %d to %d (%d) (reason = %d, ifqlen = %d)",
                CURRENT_TIME,
                index,
                ch->uid_,
                ch->next_hop_,
                ++link_broke,
                ch->xmit_reason_,
                ifqueue->length());
	logtarget->pt_->dump();
}

void
AODV::log_link_kept(nsaddr_t dst)
{
        static int link_kept = 0;

        if(! logtarget || ! verbose) return;


        /*
         *  If "god" thinks that these two nodes are now
         *  unreachable, then we are erroneously keeping
         *  a bad route.
         */
        sprintf(logtarget->pt_->buffer(),
                "A %.9f _%d_ keeping LL hop to %d (keep %d is %s)",
                CURRENT_TIME,
                index,
                dst,
                ++link_kept,
                God::instance()->hops(index, dst) == 1 ? "VALID" : "INVALID");
        logtarget->pt_->dump();
}

