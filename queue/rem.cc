/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1990-1997 Regents of the University of California.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
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
 *   
 * ns-2 code written by: Sanjeewa Athuraliya (sanjeewa@caltech.edu)
 *                       Caltech, Pasadena,
 *			 CA 91125
 *                       Victor Li
 *                       University of Melbourne, Parkville
 *                       Vic., Australia.
 *
 * Ref: Active Queue Management (REM), IEEE Network, May/June 2001.
 *      http://netlab.caltech.edu
 */

#include <math.h>
#include <sys/types.h>
#include "config.h"
#include "template.h"
#include "random.h"
#include "flags.h"
#include "delay.h"
#include "rem.h"
#include <iostream>

static class REMClass : public TclClass {
public:
	REMClass() : TclClass("Queue/REM") {}
	TclObject* create(int, const char*const*) {
		return (new REMQueue);
	}
} class_rem;


void REMQueue :: set_update_timer()
{
	rem_timer_.resched(remp_.p_updtime);
}

void REMQueue :: timeout()
{
	//do price update
	run_updaterule();
	set_update_timer();
}


void REMTimer :: expire (Event *) {
	 a_->timeout();
}



REMQueue::REMQueue() : link_(NULL), tchan_(0), rem_timer_(this) 
{
	bind("gamma_", &remp_.p_gamma);
	bind("phi_", &remp_.p_phi);
	bind("inw_", &remp_.p_inw);
	bind("mean_pktsize_", &remp_.p_pktsize); 
	bind("pupdtime_", &remp_.p_updtime); 
	bind("pbo_", &remp_.p_bo); 
	bind("prob_", &remv_.v_prob);		    // dropping probability
	bind("curq_", &curq_);			    // current queue size
	bind("pmark_", &pmark_);      	//number of packets being marked 
	bind_bool("markpkts_", &markpkts_); /* Whether to mark or drop?  Default is drop */
	bind_bool("qib_", &qib_); /* queue in bytes? */ 

	q_ = new PacketQueue();			    // underlying queue
	pq_ = q_;
	reset();

#ifdef notdef
print_remp();
print_remv();
#endif

}

void REMQueue::reset()
{
	/*
	 * Compute the "packet time constant" if we know the
	 * link bandwidth.  The ptc is the max number of 
	 * pkts per second which can be placed on the link.
	 * The link bw is given in bits/sec, so scale psize
	 * accordingly.
	 */
	 
	if (link_)
		remp_.p_ptc = link_->bandwidth() / (8.0 * remp_.p_pktsize);

	remv_.v_pl = 0.0;
	remv_.v_prob = 0.0;
	remv_.v_in = 0.0;
 	remv_.v_ave = 0.0;
	remv_.v_count = 0.0;

	remv_.v_pl1 = 0.0;
	remv_.v_pl2 = 0.0;
	remv_.v_in1 = 0.0;
	remv_.v_in2 = 0.0;
	pmark_ = 0.0;
	bcount_ = 0; 
	
	//Queue::reset();
	set_update_timer();
}

/*
 * Compute the average input rate, the price and the marking prob..
 */
void REMQueue::run_updaterule()
{

	double in, in_avg, nqueued, pl, pr;

        // link price, the congestion measure

	pl = remv_.v_pl;

	// in is the number of bytes (if qib_ is true) or packets (otherwise)
	// arriving at the link (input rate) during one update time interval

	in = remv_.v_count;
       
	// in_avg is the low pss filtered input rate
	// which is in bytes if qib_ is true and in packets otherwise.

	in_avg = remv_.v_ave;
  
	in_avg *= (1.0 - remp_.p_inw);
	
	if (qib_) {
		in_avg += remp_.p_inw*in/remp_.p_pktsize; 
		nqueued = bcount_/remp_.p_pktsize; 
        }
	else {
		in_avg += remp_.p_inw*in;
		nqueued = q_ -> length();
	}


	// c measures the maximum number of packets that 
	// could be sent during one update interval.

	double c  = remp_.p_updtime*remp_.p_ptc;

	pl = pl + remp_.p_gamma*( in_avg + 0.1*(nqueued-remp_.p_bo) - c );

	if ( pl < 0.0) {
	    pl = 0.0;
	}

	double pow1 = pow (remp_.p_phi, -pl);
	pr = 1.0-pow1;


	remv_.v_count = 0.0;
	remv_.v_ave = in_avg;
	remv_.v_pl = pl;
	remv_.v_prob = pr;
}


/*
 * Return the next packet in the queue for transmission.
 */
Packet* REMQueue::deque() 
{
	Packet *p = q_->deque();
	if (p != 0) {
		bcount_ -= hdr_cmn::access(p)->size ();
	}
	if (markpkts_) {
		double u = Random::uniform();
		if (p!=0) {
			double pro = remv_.v_prob;
			if (qib_) {
				int size = hdr_cmn::access(p)->size ();
				pro = remv_.v_prob*size/remp_.p_pktsize; 
			}
   		if ( u <= pro ) {
				hdr_flags* hf = hdr_flags::access(p);
				if(hf->ect() == 1) { 
					hf->ce() = 1; 
					pmark_++;
				}
			}
		}
	}
	double qlen = qib_ ? bcount_ : q_->length();
	curq_ = (int) qlen;
	return (p);
}

/*
 * Receive a new packet arriving at the queue.
 * The packet is dropped if the maximum queue size is exceeded.
 */

void REMQueue::enque(Packet* pkt)
{
	hdr_cmn* ch = hdr_cmn::access(pkt);
	double qlen; 

	if (qib_) {
		remv_.v_count += ch->size();
	}
	else {
		++remv_.v_count;
	}

	double qlim = qib_ ? (qlim_*remp_.p_pktsize) : qlim_ ;

	q_ -> enque(pkt);
	bcount_ += ch->size();

	qlen = qib_ ? bcount_ : q_->length();

	if (qlen >= qlim) {
		q_->remove(pkt);
		bcount_ -= ch->size();
		drop(pkt);
	}
	else  {
		if (!markpkts_) {
			double u = Random::uniform(); 
			double pro = remv_.v_prob;
			if (qib_) {
				pro = remv_.v_prob*ch->size()/remp_.p_pktsize; 
			}
			if ( u <= pro ) {
				q_->remove(pkt);
				bcount_ -= ch->size();
				drop(pkt);
			}		    
		}
	}
	qlen = qib_ ? bcount_ : q_->length();
	curq_ = (int) qlen;
}


int REMQueue::command(int argc, const char*const* argv)
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
				tcl.resultf("REM: trace: can't attach %s for writing", id);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
		// tell REM about link stats
		if (strcmp(argv[1], "link") == 0) {
			LinkDelay* del = (LinkDelay*)TclObject::lookup(argv[2]);
			if (del == 0) {
				tcl.resultf("REM: no LinkDelay object %s",
					argv[2]);
				return(TCL_ERROR);
			}
			// set ptc now
			link_ = del;
			remp_.p_ptc = link_->bandwidth() /
				(8. * remp_.p_pktsize);

			return (TCL_OK);
		}
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

/*
 * Routine called by TracedVar facility when variables change values.
 * Currently used to trace values of avg queue size, drop probability,
 * and the instantaneous queue size seen by arriving packets.
 * Note that the tracing of each var must be enabled in tcl to work.
 */

void
REMQueue::trace(TracedVar* v)
{
	char wrk[500];
	const char *p;

	if (((p = strstr(v->name(), "ave")) == NULL) &&
	    ((p = strstr(v->name(), "prob")) == NULL) &&
	    ((p = strstr(v->name(), "curq")) == NULL)) {
		fprintf(stderr, "REM:unknown trace var %s\n",
			v->name());
		return;
	}

	if (tchan_) {
		int n;
		double t = Scheduler::instance().clock();
		if (*p == 'c') {
			sprintf(wrk, "Q %g %d", t, int(*((TracedInt*) v)));
		} else {
			sprintf(wrk, "%c %g %g", *p, t,
				double(*((TracedDouble*) v)));
		}
		n = strlen(wrk);
		wrk[n] = '\n'; 
		wrk[n+1] = 0;
		(void)Tcl_Write(tchan_, wrk, n+1);
	}
	return; 
}

/* for debugging help */
void REMQueue::print_remp()
{
	printf("=========\n");
}

void REMQueue::print_remv()
{
	printf("=========\n");
}
