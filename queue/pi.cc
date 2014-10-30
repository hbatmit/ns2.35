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
 */

/*
 * Based on PI controller described in:
 * C. Hollot, V. Misra, D. Towsley and W. Gong. 
 * On Designing Improved Controllers for AQM Routers
 * Supporting TCP Flows, 
 * INFOCOMM 2001. 
 */

#include <math.h>
#include <sys/types.h>
#include "config.h"
#include "template.h"
#include "random.h"
#include "flags.h"
#include "delay.h"
#include "pi.h"

static class PIClass : public TclClass {
public:
	PIClass() : TclClass("Queue/PI") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc==5) 
			return (new PIQueue(argv[4]));
		else
			return (new PIQueue("Drop"));
	}
} class_pi;


PIQueue::PIQueue(const char * trace) : CalcTimer(this), link_(NULL), q_(NULL),
	qib_(0), de_drop_(NULL), EDTrace(NULL), tchan_(0), curq_(0),
	edp_(), edv_(), first_reset_(1)
{
	if (strlen(trace) >=20) {
		printf("trace type too long - allocate more space to traceType in pi.h and recompile\n");
		exit(0);
	}
	strcpy(traceType, trace);

	bind_bool("bytes_", &edp_.bytes);	    // boolean: use bytes?
	bind_bool("queue_in_bytes_", &qib_);	    // boolean: q in bytes?
	bind("a_", &edp_.a);		
	bind("b_", &edp_.b);	   
	bind("w_", &edp_.w);		  
	bind("qref_", &edp_.qref);		  
	bind("mean_pktsize_", &edp_.mean_pktsize);  // avg pkt size
	bind_bool("setbit_", &edp_.setbit);	    // mark instead of drop
	bind("prob_", &edv_.v_prob);		    // dropping probability
	bind("curq_", &curq_);			    // current queue size
	q_ = new PacketQueue();			    // underlying queue
	pq_ = q_;
	reset();
}

void PIQueue::reset()
{
	//double now = Scheduler::instance().clock();
	/*
	if (qib_ && first_reset_ == 1) {
		edp_.qref = edp_.qref*edp_.mean_pktsize;
	}
	*/
	edv_.count = 0;
	edv_.count_bytes = 0;
	edv_.v_prob = 0;
	edv_.qold = 0;
	curq_ = 0;
	calculate_p();
	Queue::reset();
}


void PIQueue::enque(Packet* pkt)
{
	//double now = Scheduler::instance().clock();
	hdr_cmn* ch = hdr_cmn::access(pkt);
	++edv_.count;
	edv_.count_bytes += ch->size();

	int droptype = DTYPE_NONE;

	int qlen = qib_ ? q_->byteLength() : q_->length();
	curq_ = qlen;	// helps to trace queue during arrival, if enabled

	int qlim = qib_ ? (qlim_ * edp_.mean_pktsize) : qlim_;

	if (qlen >= qlim) {
		droptype = DTYPE_FORCED;
	}
	else {
		if (drop_early(pkt, qlen)) {
			droptype = DTYPE_UNFORCED;
		}
	}

	if (droptype == DTYPE_UNFORCED) {
		Packet *pkt_to_drop = pickPacketForECN(pkt);
		if (pkt_to_drop != pkt) {
			q_->enque(pkt);
			q_->remove(pkt_to_drop);
			pkt = pkt_to_drop; /* XXX okay because pkt is not needed anymore */
		}

		if (de_drop_ != NULL) {
			if (EDTrace != NULL) 
				((Trace *)EDTrace)->recvOnly(pkt);
			de_drop_->recv(pkt);
		}
		else {
			drop(pkt);
		}
	} else {
		q_->enque(pkt);
		if (droptype == DTYPE_FORCED) {
			pkt = pickPacketToDrop();
			q_->remove(pkt);
			drop(pkt);
			edv_.count = 0;
			edv_.count_bytes = 0;
		}
	}
	return;
}

double PIQueue::calculate_p()
{
	//double now = Scheduler::instance().clock();
	double p;
	int qlen = qib_ ? q_->byteLength() : q_->length();
	
	if (qib_) {
		p=edp_.a*(qlen*1.0/edp_.mean_pktsize-edp_.qref)-
			edp_.b*(edv_.qold*1.0/edp_.mean_pktsize-edp_.qref)+
			edv_.v_prob;
	}
	else {
		p=edp_.a*(qlen-edp_.qref)-edp_.b*(edv_.qold-edp_.qref)+edv_.v_prob;
	}
		
	if (p < 0) p = 0;
	if (p > 1) p = 1;
	
	edv_.v_prob = p;
	edv_.qold = qlen;

	CalcTimer.resched(1.0/edp_.w);
	return p;
}

int PIQueue::drop_early(Packet* pkt, int )
{
	//double now = Scheduler::instance().clock();
	hdr_cmn* ch = hdr_cmn::access(pkt);
	double p = edv_.v_prob; 

	if (edp_.bytes) {
		p = p*ch->size()/edp_.mean_pktsize;
		if (p > 1) p = 1; 
	}

	double u = Random::uniform();
	if (u <= p) {
		edv_.count = 0;
		edv_.count_bytes = 0;
		hdr_flags* hf = hdr_flags::access(pickPacketForECN(pkt));
		if (edp_.setbit && hf->ect()) {
			hf->ce() = 1; 	// mark Congestion Experienced bit
			return (0);	// no drop
		} else {
			return (1);	// drop
		}
	}
	return (0);			// no DROP/mark
}

Packet* PIQueue::pickPacketForECN(Packet* pkt)
{
	return pkt; /* pick the packet that just arrived */
}

Packet* PIQueue::pickPacketToDrop() 
{
	int victim;
	victim = q_->length() - 1;
	return(q_->lookup(victim)); 
}

Packet* PIQueue::deque()
{
	Packet *p;
	p = q_->deque();
	curq_ = qib_ ? q_->byteLength() : q_->length(); // helps to trace queue during arrival, if enabled
	return (p);
}

int PIQueue::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "reset") == 0) {
			reset();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "early-drop-target") == 0) {
			if (de_drop_ != NULL)
				tcl.resultf("%s", de_drop_->name());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "edrop-trace") == 0) {
			if (EDTrace != NULL) {
				tcl.resultf("%s", EDTrace->name());
			}
			else {
				tcl.resultf("0");
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "trace-type") == 0) {
			tcl.resultf("%s", traceType);
			return (TCL_OK);
		}
	} 
	else if (argc == 3) {
		// attach a file for variable tracing
		if (strcmp(argv[1], "attach") == 0) {
			int mode;
			const char* id = argv[2];
			tchan_ = Tcl_GetChannel(tcl.interp(), (char*)id, &mode);
			if (tchan_ == 0) {
				tcl.resultf("PI: trace: can't attach %s for writing", id);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
		// tell PI about link stats
		if (strcmp(argv[1], "link") == 0) {
			LinkDelay* del = (LinkDelay*)TclObject::lookup(argv[2]);
			if (del == 0) {
				tcl.resultf("PI: no LinkDelay object %s", argv[2]);
				return(TCL_ERROR);
			}
			link_ = del;
			return (TCL_OK);
		}
		if (strcmp(argv[1], "early-drop-target") == 0) {
			NsObject* p = (NsObject*)TclObject::lookup(argv[2]);
			if (p == 0) {
				tcl.resultf("no object %s", argv[2]);
				return (TCL_ERROR);
			}
			de_drop_ = p;
			return (TCL_OK);
		}
		if (strcmp(argv[1], "edrop-trace") == 0) {
			NsObject * t  = (NsObject *)TclObject::lookup(argv[2]);
			if (t == 0) {
				tcl.resultf("no object %s", argv[2]);
				return (TCL_ERROR);
			}
			EDTrace = t;
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

void PIQueue::trace(TracedVar* v)
{
	char wrk[500];
	const char *p;

	if (((p = strstr(v->name(), "prob")) == NULL) &&
	    ((p = strstr(v->name(), "curq")) == NULL)) {
		fprintf(stderr, "PI:unknown trace var %s\n", v->name());
		return;
	}
	if (tchan_) {
		int n;
		double t = Scheduler::instance().clock();
		// XXX: be compatible with nsv1 PI trace entries
		if (*p == 'c') {
			sprintf(wrk, "Q %g %d", t, int(*((TracedInt*) v)));
		} else {
			sprintf(wrk, "%c %g %g", *p, t, double(*((TracedDouble*) v)));
		}
		n = strlen(wrk);
		wrk[n] = '\n'; 
		wrk[n+1] = 0;
		(void)Tcl_Write(tchan_, wrk, n+1);
	}
	return; 
}

void PICalcTimer::expire(Event *)
{
	a_->calculate_p();
}
