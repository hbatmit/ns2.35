/*
 * Copyright (c) 2003  International Computer Science Institute
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
 *      This product includes software developed by ICIR, the ICSI
 *      Center for Internet Research (ICSI: the International Computer
 *      Science Institute).
 * 4. Neither the name of ICIR nor of ICSI may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICSI AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ICSI OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "delayer.h"

Delayer::Delayer() : Connector(), last_sent(-100000), alloc_int(NULL), 
alloc_len(NULL),  alloc_free(1), at_(*this), 
spike_int(NULL), spike_len(NULL), spike_free(1),
st_(*this), target_free(1), th_ (*this), prev_h_(NULL), pkt_(NULL) {
}
 
void Delayer::recv(Packet* p, Handler* h) {
	double now = Scheduler::instance().clock(); 		

	if (pkt_ != NULL) {
		printf("delayer not empty!\n");
		exit(1);
 	}

	prev_h_ = h;
	pkt_ = p;

        // if queue has been empty then trigger channel allocation delay
        if (alloc_len && now - last_sent > alloc_int->value()) {        
                        alloc_free = 0;
                        at_.resched(alloc_len->value());
                        return;
        }
	try_send();
}

void Delayer::try_send() {
	double now = Scheduler::instance().clock(); 		
	
	if (debug_)
	        printf("now %f last_sent %f alloc_free %d target_free %d, spike_free %d\n", 
			now, last_sent, alloc_free, target_free, spike_free);

	if (!target_free || !alloc_free || !spike_free)
		return;

	if (pkt_ && target_) {
                target_->recv(pkt_, &th_);
		last_sent = Scheduler::instance().clock(); 		
		target_free = 0;
		pkt_ = NULL;
	}
	if (prev_h_)
		prev_h_->handle(&e);
}

int Delayer::command(int argc, const char*const* argv) {
        // explicitly block the queue to model a delay spike
        if (argc == 2 && !strcmp(argv[1],"block") 
	&& !spike_len 
	) {
	        spike_free = 0;
        	return (TCL_OK);
        }
        if (argc == 2 && !strcmp(argv[1],"unblock") 
	//	&& !spike_len
	) {
                spike_free = 1;
                try_send();
                return (TCL_OK);
        }
        // set distributions for channel allocation delay
        if (argc == 4 && !strcmp(argv[1],"alloc")) {
                alloc_int = (RandomVariable *)TclObject::lookup(argv[2]);
                alloc_len = (RandomVariable *)TclObject::lookup(argv[3]);
                return (TCL_OK);
        }
        // set distributions for delay spikes
        if (argc == 4 && !strcmp(argv[1],"spike")) {
                spike_int = (RandomVariable *)TclObject::lookup(argv[2]);
                spike_len = (RandomVariable *)TclObject::lookup(argv[3]);
		st_.sched(getNextSpikeInt());
                return (TCL_OK);
        }
        return Connector::command(argc, argv);
}

void SpikeTimer::expire(Event*) {
//	printf("SpikeHandler, spike_free %d\n", delayer_.getSpike());
  	if (delayer_.getSpike()) {
           	delayer_.takeSpike();
               	resched(delayer_.getNextSpikeLen());
              	return;
   	}
   	delayer_.freeSpike();
   	resched(delayer_.getNextSpikeInt());
   	delayer_.try_send();
}

void AllocTimer::expire(Event*) {
	delayer_.freeAlloc(); 
	delayer_.try_send();
//	printf("AllocHandler\n");
}

void TargetHandler::handle(Event*) {
	delayer_.freeTarget(); 
	delayer_.try_send();
//	printf("TargetHandler\n");
}

