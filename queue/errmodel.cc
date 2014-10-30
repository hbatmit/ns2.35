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
 * Contributed by the Daedalus Research Group, UC Berkeley 
 * (http://daedalus.cs.berkeley.edu)
 *
 * Multi-state error model patches contributed by Jianping Pan 
 * (jpan@bbcr.uwaterloo.ca).
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/queue/errmodel.cc,v 1.84 2010/03/08 05:54:53 tom_henderson Exp $ (UCB)
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/queue/errmodel.cc,v 1.84 2010/03/08 05:54:53 tom_henderson Exp $ (UCB)";
#endif

#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include "packet.h"
#include "flags.h"
#include "mcast_ctrl.h"
#include "errmodel.h"
#include "srm-headers.h"		// to get the hdr_srm structure
#include "classifier.h"

static class ErrorModelClass : public TclClass {
public:
	ErrorModelClass() : TclClass("ErrorModel") {}
	TclObject* create(int, const char*const*) {
		return (new ErrorModel);
	}
} class_errormodel;

static class TwoStateErrorModelClass : public TclClass {
public:
	TwoStateErrorModelClass() : TclClass("ErrorModel/TwoState") {}
	TclObject* create(int, const char*const*) {
		return (new TwoStateErrorModel);
	}
} class_errormodel_twostate;

static class ComplexTwoStateMarkovModelClass : public TclClass {
public:
 	ComplexTwoStateMarkovModelClass() : TclClass("ErrorModel/ComplexTwoStateMarkov") {}
 	TclObject* create(int, const char*const*) {
 		return (new ComplexTwoStateErrorModel);
 	}
} class_errormodel_complextwostatemarkov;


static class MultiStateErrorModelClass : public TclClass {
public:
	MultiStateErrorModelClass() : TclClass("ErrorModel/MultiState") {}
	TclObject* create(int, const char*const*) {
		return (new MultiStateErrorModel);
	}
} class_errormodel_multistate;

static class TraceErrorModelClass : public TclClass {
public:
	TraceErrorModelClass() : TclClass("ErrorModel/Trace") {}
	TclObject* create(int, const char*const*) {
		return (new TraceErrorModel);
	}
} class_traceerrormodel;

static char* eu_names[] = { EU_NAMES };

inline double comb(int n, int k) {
	int i;
	double sum = 1.0;

	for(i = 0; i < k; i++) 
		sum *= (n - i)/(i + 1);
	return sum;
}


ErrorModel::ErrorModel() : et_(0), firstTime_(1), unit_(EU_PKT), ranvar_(0), FECstrength_(1)
{
	bind("enable_", &enable_);
	bind("rate_", &rate_);
	bind("delay_", &delay_);
	bind_bw("bandwidth_", &bandwidth_); // required for EU_TIME
	bind_bool("markecn_", &markecn_);
	bind_bool("delay_pkt_", &delay_pkt_);
	
}

int ErrorModel::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	//ErrorModel *em;
	if (argc == 3) {
		if (strcmp(argv[1], "unit") == 0) {
			unit_ = STR2EU(argv[2]);
			return (TCL_OK);
		} 
		if (strcmp(argv[1], "ranvar") == 0) {
			ranvar_ = (RandomVariable*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "FECstrength") == 0) {
			FECstrength_ = atoi(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "datapktsize") == 0) {
			datapktsize_ = atoi(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "cntrlpktsize") == 0) {
			cntrlpktsize_ = atoi(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "eventtrace") == 0) {
			et_ = (EventTrace *)TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
	} else if (argc == 2) {
		if (strcmp(argv[1], "unit") == 0) {
			tcl.resultf("%s", eu_names[unit_]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "ranvar") == 0) {
			tcl.resultf("%s", ranvar_->name());
			return (TCL_OK);
		} 
		if (strcmp(argv[1], "FECstrength") == 0) {
			tcl.resultf("%d", FECstrength_);
			return (TCL_OK);
		} 
	} 
	return Connector::command(argc, argv);
}

void ErrorModel::reset()
{
	firstTime_ = 1;
}

void ErrorModel::recv(Packet* p, Handler* h)
{
	// 1.  Determine the error by calling corrupt(p)
	// 2.  Set the packet's error flag if it is corrupted
	// 3.  If there is no error,  no drop_ target or markecn is true,
	//	let pkt continue, otherwise hand the corrupted packet to drop_

	hdr_cmn* ch = hdr_cmn::access(p);
	int error = corrupt(p);

	// XXX When we do ECN, the packet is marked but NOT dropped.
	// So we don't resume handler here. 
	if (!markecn_ && !delay_pkt_ && (h && ((error && drop_) || !target_))) {
		// if we drop or there is no target_, then resume handler
		double delay = Random::uniform(8.0 * ch->size() / bandwidth_);
		if (intr_.uid_ <= 0 ) 
			// schedule only if nothing scheduled already
			Scheduler::instance().schedule(h, &intr_, delay);
	} 
	if (error) {
		ch->error() |= error;

		if (markecn_) {
			hdr_flags* hf = hdr_flags::access(p);
			hf->ce() = 1;
		} else if (delay_pkt_) {
			// Delay the packet.
			Scheduler::instance().schedule(target_, p, delay_);
			return;
		} else if (drop_) {
			drop_->recv(p);
			return;
		}
	}

	if (target_) {
	       	target_->recv(p, h);
	}
}

int ErrorModel::corrupt(Packet* p)
{
	hdr_cmn* ch;
	// a temp hack

	ch = HDR_CMN(p);
	
	if (enable_ == 0)
		return 0;
	switch (unit_) {
	case EU_TIME:
		return (CorruptTime(p) != 0);
	case EU_BYTE:
		return (CorruptByte(p) != 0);
	case EU_BIT:
		ch = hdr_cmn::access(p);
		ch->errbitcnt() = CorruptBit(p);
		return (ch->errbitcnt() != 0);
	default:
		return (CorruptPkt(p) != 0);
	}
	return 0;
}

double ErrorModel::PktLength(Packet* p)
{
	//double now;
	if (unit_ == EU_PKT)
		return 1;
	int byte = hdr_cmn::access(p)->size();
	if (unit_ == EU_BYTE)
		return byte;
	if (unit_ == EU_BIT)
		return 8.0 * byte;
	return 8.0 * byte / bandwidth_;
}

double * ErrorModel::ComputeBitErrProb(int size) 
{
	double *dptr;
	int i;

        dptr = (double *)calloc((FECstrength_ + 2), sizeof(double));

        for (i = 0; i < (FECstrength_ + 1) ; i++) 
		dptr[i] = comb(size, i) * pow(rate_, (double)i) * pow(1.0 - rate_, (double)(size - i));

	// Cumulative probability
	for (i = 0; i < FECstrength_ ; i++) 
		dptr[i + 1] += dptr[i];
	
	dptr[FECstrength_ + 1] = 1.0;

	/*	printf("Size = %d\n", size);
	for (i = 0; i <(FECstrength_ + 2); i++)
		printf("Ptr[%d] = %g\n", i, dptr[i]); */

	return dptr;
	
}

int ErrorModel::CorruptPkt(Packet*) 
{
	// if no random var is specified, assume uniform random variable
	double u = ranvar_ ? ranvar_->value() : Random::uniform();
	return (u < rate_);
}

int ErrorModel::CorruptByte(Packet* p)
{
	// compute pkt error rate, assume uniformly distributed byte error
	double per = 1 - pow(1.0 - rate_, PktLength(p));
	double u = ranvar_ ? ranvar_->value() : Random::uniform();
	return (u < per);
}

int ErrorModel::CorruptBit(Packet* p)
{
	double u, *dptr;
	int i;

	if (firstTime_ && FECstrength_) {
		// precompute the probabilies for each bit-error cnts
		cntrlprb_ = ComputeBitErrProb(cntrlpktsize_);
		dataprb_ = ComputeBitErrProb(datapktsize_);

		firstTime_ = 0;
	}	

	u = ranvar_ ? ranvar_->value() : Random::uniform();
	dptr = (hdr_cmn::access(p)->size() >= datapktsize_) ? dataprb_ : cntrlprb_;
        for (i = 0; i < (FECstrength_ + 2); i++)
		if (dptr[i] > u) break;
	return(i);
}

int ErrorModel::CorruptTime(Packet *)
{
	fprintf(stderr, "Warning:  uniform rate error cannot be time-based\n");
	return 0;
}

#if 0
/*
 * Decide whether or not to corrupt this packet, for a continuous 
 * time-based error model.  The main parameter used is errLength,
 * which is the time to the next error, from the last time an error 
 * occured on  the channel.  It is dependent on the random variable 
 * being used  internally.
 *	rate_ is the user-specified mean
 */
int ErrorModel::CorruptTime(Packet *p)
{
	/* 
	 * First get MAC header.  It has the transmission time (txtime)
	 * of the packet in one of it's fields.  Then, get the time
	 * interval [t-txtime, t], where t is the current time.  The
	 * goal is to figure out whether the channel would have
	 * corrupted the packet during that interval. 
	 */
	Scheduler &s = Scheduler::instance();
	double now = s.clock(), rv;
	int numerrs = 0;
	double start = now - hdr_mac::access(p)->txtime();

	while (remainLen_ < start) {
		rv = ranvar_ ? ranvar_->value() : Random::uniform(rate_);
		remainLen_ += rv;
	}

	while (remainLen_ < now) { /* corrupt the packet */
		numerrs++;
		rv = ranvar_ ? ranvar_->value() : Random::uniform(rate_);
		remainLen_ += rv;
	}
	return numerrs;
}
#endif

void ErrorModel::trace_event(char *eventtype)
{
	if (et_ == NULL) return;
	char *wrk = et_->buffer();
	char *nwrk = et_->nbuffer();
	if (wrk != 0)
		sprintf(wrk,
			"E "TIME_FORMAT" ErrModelTimer %p %s",
			et_->round(Scheduler::instance().clock()),   // time
			this,
			eventtype                    // event type
			);
	
	if (nwrk != 0)
		sprintf(nwrk,
			"E -t "TIME_FORMAT" ErrModelTimer %p %s",
			et_->round(Scheduler::instance().clock()),   // time
			this,
			eventtype                    // event type
			);
	et_->trace();
}



/*
 * Two-State:  error-free and error
 */
TwoStateErrorModel::TwoStateErrorModel() : remainLen_(0), twoStateTimer_(NULL)
{
	ranvar_[0] = ranvar_[1] = 0;
}

int TwoStateErrorModel::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (strcmp(argv[1], "ranvar") == 0) {
		int i = atoi(argv[2]);
		if (i < 0 || i > 1) {
			tcl.resultf("%s does not has ranvar_[%d]", name_, i);
			return (TCL_ERROR);
		}
		if (argc == 3) {
			tcl.resultf("%s", ranvar_[i]->name());
			return (TCL_OK);
		}
		if (argc == 4) {
			ranvar_[i] = (RandomVariable*)TclObject::lookup(argv[3]);
			if (ranvar_[0] != 0 && ranvar_[1] != 0)
				checkUnit();
			return (TCL_OK);
		} 
	}
	return ErrorModel::command(argc, argv);
}

int TwoStateErrorModel::corruptPkt(Packet* p)
{
#define ZERO 0.00000
	int error;
	if (firstTime_) {
		firstTime_ = 0;
		state_ = 0;
		remainLen_ = ranvar_[state_]->value();
	}

	// if remainLen_ is outside the range of 0, then error = state_
	error = state_ && (remainLen_ > ZERO);
	remainLen_ -= PktLength(p);

	// state transition until remainLen_ > 0 to covers the packet length
	while (remainLen_ <= ZERO) {
		state_ ^= 1;	// state transition: 0 <-> 1
		remainLen_ += ranvar_[state_]->value();
		error |= state_;
	}
	return error;
}

void TwoStateErrorModel::checkUnit() 
{
	if (unit_ == EU_TIME) {
		// setup timer for keeping states in time
		twoStateTimer_ = new TwoStateErrModelTimer(this, &TwoStateErrorModel::transitionState);
		transitionState();
	}
	
}

void TwoStateErrorModel::transitionState()
{
	char buf[SMALL_LEN];
	
	if (firstTime_) {
		firstTime_ = 0;
		state_ = 0;
		remainLen_ = ranvar_[state_]->value();
		twoStateTimer_->sched(remainLen_);
		sprintf (buf,"STATE %d, DURATION %f",state_,remainLen_);
		trace_event(buf);
		return;
	}
	state_ ^= 1;
	remainLen_ = ranvar_[state_]->value();
	if (state_ == 1 && remainLen_ > 120)
		remainLen_ = 120;
		
	twoStateTimer_->resched(remainLen_);
	sprintf (buf,"STATE %d, DURATION %f",state_,remainLen_);
	trace_event(buf);
	
}



int TwoStateErrorModel::corrupt(Packet* p)
{
	if (unit_ == EU_TIME)
		return corruptTime(p);
	else
		return corruptPkt(p);
}

int TwoStateErrorModel::corruptTime(Packet*)
{
	int error = 0;
	if (state_ == 1)
		error = 1;
	return error;
	
}

ComplexTwoStateErrorModel::ComplexTwoStateErrorModel() 
{
	em_[0] = new TwoStateErrorModel();
	em_[1] = new TwoStateErrorModel();
}

ComplexTwoStateErrorModel::~ComplexTwoStateErrorModel()
{
	delete em_[0];
	delete em_[1];
}

int ComplexTwoStateErrorModel::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "unit") == 0) {
			unit_ = STR2EU(argv[2]);
			em_[0]->setunit(unit_); 
			em_[1]->setunit(unit_);
			return TCL_OK;
		}
		if (strcmp(argv[1], "eventtrace") == 0) {
			EventTrace* et = (EventTrace *)TclObject::lookup(argv[2]);
			em_[0]->et_ = et;
			em_[1]->et_ = et;
			return (TCL_OK);
		}
	}
	else if (argc == 5) {
		if (strcmp(argv[1], "ranvar") == 0) {
			int i = atoi(argv[2]);
			int j = atoi(argv[3]);
			if (i < 0 || i > 1) {
				tcl.add_errorf("%s does not has em_[%d]", name_, i);
				return (TCL_ERROR);
			}
			if (j < 0 || j > 1) {
				tcl.add_errorf("%s does not has ranvar_[%d]", name_, i);
				return (TCL_ERROR);
			}

			em_[i]->ranvar_[j] = (RandomVariable*)TclObject::lookup(argv[4]);
			if (em_[i]->ranvar_[0] != 0 && em_[i]->ranvar_[1] != 0)
				em_[i]->checkUnit();
			return (TCL_OK);
		}
		
	}
	return ErrorModel::command(argc, argv);
}


int ComplexTwoStateErrorModel::corruptTime(Packet*)
{
	int error = 0;
	if (em_[0]->state_ == 1 && em_[1]->state_ == 1) 
		error = 1;
	return error;
}

int ComplexTwoStateErrorModel::corruptPkt(Packet*)
{
	fprintf(stderr, "Error model defined in time; not in packets\n");
	return -1;
	
}



static char * st_names[]={ST_NAMES};

/*
// MultiState ErrorModel:
//   corrupt(pkt) invoke Tcl method "corrupt" to do state transition
//	Tcl corrupt either:
//	   - assign em_, the error-model to be use
//	   - return the status of the packet
//	If em_ is assigned, then invoke em_->corrupt(p)
*/

MultiStateErrorModel::MultiStateErrorModel() : prevTime_(0.0), em_(0)
{
	bind("sttype_", &sttype_);
	bind("texpired_", &texpired_);
	bind("curperiod_", &curperiod_);
}

int MultiStateErrorModel::command(int argc, const char*const* argv)
{

	Tcl& tcl = Tcl::instance();

	if (argc == 3) {
		if (strcmp(argv[1], "error-model") == 0) {
			em_ = (ErrorModel*) TclObject::lookup(argv[2]);
			return TCL_OK;
		}
		if (strcmp(argv[1], "sttype") == 0) {
			sttype_ = STR2ST(argv[2]);
			return TCL_OK;
		}
	} else if (argc == 2) {
		if (strcmp(argv[1], "sttype") == 0) {
			tcl.resultf("%s", st_names[sttype_]);
			return TCL_OK;
		}
		if (strcmp(argv[1], "error-model") == 0) {
			tcl.resultf("%s", (ErrorModel*) em_->name());
			return TCL_OK;
		}
	}
	return ErrorModel::command(argc, argv);
}

int MultiStateErrorModel::corrupt(Packet* p)
{
	int retval;
	double now;
	// static double prevTime_ = 0.0;
	Scheduler & s = Scheduler::instance();

	now = s.clock();

	if (sttype_ == ST_TIME)
		if ((now - prevTime_) >= curperiod_)
			texpired_ = 1;

	Tcl& tcl = Tcl::instance();
	tcl.evalf("%s corrupt", name());

	retval = em_ ? em_->corrupt(p) : atoi(tcl.result());

	if (firstTime_) {
		firstTime_ = 0;
		prevTime_ = s.clock();
		texpired_ = 0;
	}

	return (retval);

}


TraceErrorModel::TraceErrorModel() : loss_(0), good_(123456789)
{
	bind("good_", &good_);
	bind("loss_", &loss_);
}

/* opening and reading the trace file/info is done in OTcl */
int TraceErrorModel::corrupt(Packet* p)
{
	Tcl& tcl = Tcl::instance();
	if (! match(p))
		return 0;
	if ((good_ <= 0) && (loss_ <= 0)) {
		tcl.evalf("%s read",name());
		if (good_ < 0)
			good_ = 123456789;
	}
	if (good_-- > 0)
		return 0;
	return (loss_-- > 0);
}

int TraceErrorModel::match(Packet*)
{
	return 1;
}


/*
 * Periodic ErrorModel
 */
static class PeriodicErrorModelClass : public TclClass {
public:
	PeriodicErrorModelClass() : TclClass("ErrorModel/Periodic") {}
	TclObject* create(int, const char*const*) {
		return (new PeriodicErrorModel);
	}
} class_periodic_error_model;

PeriodicErrorModel::PeriodicErrorModel() : cnt_(0), last_time_(0.0), first_time_(-1.0)
{
	bind("period_", &period_);
	bind("offset_", &offset_);
	bind("burstlen_", &burstlen_);
	bind("default_drop_", &default_drop_);
}

int PeriodicErrorModel::corrupt(Packet* p)
{
	hdr_cmn *ch = hdr_cmn::access(p);
	double now = Scheduler::instance().clock();

	if (unit_ == EU_TIME) {
		if (first_time_ < 0.0) {
			if (now >= offset_) {
				first_time_ = last_time_ = now;
				return 1;
			}
		} else {
			if ((now - last_time_) > period_) {
				last_time_ = now;
				return 1;
			}
			if ((now - last_time_) < burstlen_) {
				return 1;
			}
		}
		return 0;
	}
	cnt_ += (unit_ == EU_PKT) ? 1 : ch->size();
	if (default_drop_) {
		if (int(first_time_) < 0) {
			if (cnt_ >= int(offset_)) {
				last_time_ = first_time_ = 1.0;
				cnt_ = 0;
				return 0;
			}
			return 0;
		} else {
			if (cnt_ >= int(period_)) {
				cnt_ = 0;
				return 0;
			}
		}
		return 1;
	} else {
		if (int(first_time_) < 0) {
			if (cnt_ >= int(offset_)) {
				last_time_ = first_time_ = 1.0;
				cnt_ = 0;
				return 1;
			}
			return 0;
		} else {
			if (cnt_ >= int(period_)) {
				cnt_ = 0;
				return 1;
			}
			if (cnt_ < burstlen_)
				return 1;
		}
		return 0;
	}
}

/*
 * List ErrorModel: specify a list of packets/bytes to drop
 * can be specified in any order
 */
static class ListErrorModelClass : public TclClass {
public:
	ListErrorModelClass() : TclClass("ErrorModel/List") {}
	TclObject* create(int, const char*const*) {
		return (new ListErrorModel);
	}
} class_list_error_model;

int ListErrorModel::corrupt(Packet* p)
{
	/* assumes droplist_ is sorted */
	int rval = 0;	// no drop

	if (unit_ == EU_TIME) {
		fprintf(stderr,
			"ListErrorModel: error, EU_TIME not supported\n");
		return 0;
	}

	if (droplist_ == NULL || dropcnt_ == 0) {
		fprintf(stderr, "warning: ListErrorModel: null drop list\n");
		return 0;
	}

	if (unit_ == EU_PKT) {
//printf("TEST: cur_:%d, dropcnt_:%d, droplist_[cur_]:%d, cnt_:%d\n",
//cur_, dropcnt_, droplist_[cur_], cnt_);
		if ((cur_ < dropcnt_) && droplist_[cur_] == cnt_) {
			rval = 1;
			cur_++;
		}
		cnt_++;

	} else if (unit_ == EU_BYTE) {
		int sz = hdr_cmn::access(p)->size();
		if ((cur_ < dropcnt_) && (cnt_ + sz) >= droplist_[cur_]) {
			rval = 1;
			cur_++;
		}
		cnt_ += sz;
	}

	return (rval);
}

int
ListErrorModel::command(int argc, const char*const* argv)
{
	/*
	 * works for variable args:
	 *	$lem droplist "1 3 4 5"
	 * and
	 *	$lem droplist 1 3 4 5
	 */
	Tcl& tcl = Tcl::instance();
	if (strcmp(argv[1], "droplist") == 0) {
		int cnt;
		if ((cnt = parse_droplist(argc-2, argv + 2)) < 0)
			return (TCL_ERROR);
		tcl.resultf("%u", cnt);
		return(TCL_OK);
	}
	return (ErrorModel::command(argc, argv));
}

int
ListErrorModel::intcomp(const void *p1, const void *p2)
{
	int a = *((int*) p1);
	int b = *((int*) p2);
	return (a - b);
}

/*
 * nextval: find the next value in the string
 *
 * skip white space, update pointer to first non-white-space
 * character.  Return the number of characters in the next
 * token.
 */
int
ListErrorModel::nextval(const char*& p)
{
	while (*p && isspace(*p))
		++p;

	if (!*p) {
		/* end of string */
		return (0);
	}
	const char *q = p;
	while (*q && !isspace(*q))
		++q;
	return (q-p);
}

int
ListErrorModel::parse_droplist(int argc, const char *const* argv)
{

	int cnt = 0;		// counter for argc list
	int spaces = 0;		// counts # of spaces in an argv entry
	int total = 0;		// total entries in the drop list
	int n;			// # of chars in the next drop number
	const char *p;		// ptr into current string

	/*
	 * loop over argc list:  figure out how many numbers
	 * have been specified
	 */
	while (cnt < argc) {
		p = argv[cnt];
		spaces = 0;
		while ((n = nextval(p))) {
			if (!isdigit(*p)) {
				/* problem... */
				fprintf(stderr, "ListErrorModel(%s): parse_droplist: unknown drop specifier starting at >>>%s\n",
					name(), p);
				return (-1);
			}
			++spaces;
			p += n;
		}
		total += spaces;
		cnt++;
	}

	/*
	 * parse the numbers, put them in an array (droplist_)
	 * set dropcnt_ to the total # of drops.  Also, free any
	 * previous drop list.
	 */

	if ((total == 0) || (dropcnt_ > 0 && droplist_ != NULL)) {
		delete[] droplist_;
		droplist_ = NULL;
	}

	if ((dropcnt_ = total) == 0)
		return (0);

	droplist_ = new int[dropcnt_];
	if (droplist_ == NULL) {
		fprintf(stderr,
			"ListErrorModel(%s): no memory for drop list!\n",
			name());
		return (-1);
	}

	int idx = 0;
	cnt = 0;
	while (cnt < argc) {
		p = argv[cnt];
		while ((n = nextval(p))) {
			/*
			 * this depends on atoi(s) returning the
			 * value of the first number in s
			 */
			droplist_[idx++] = atoi(p);
			p += n;
		}
		cnt++;
	}
	qsort(droplist_, dropcnt_, sizeof(int), intcomp);

	/*
	 * sanity check the array, looking for (wrong) dups
	 */
	cnt = 0;
	while (cnt < (dropcnt_ - 1)) {
		if (droplist_[cnt] == droplist_[cnt+1]) {
			fprintf(stderr,
				"ListErrorModel: error: dup %d in list\n",
				droplist_[cnt]);
			total = -1;	/* error */
		}
		++cnt;
	}

	if (total < 0) {
		if (droplist_)
			delete[] droplist_;
		dropcnt_ = 0;
		droplist_ = NULL;
		return (-1);
	}

#ifdef notdef
printf("sorted list:\n");
{
	register i;
	for (i =0; i < dropcnt_; i++) {
		printf("list[%d] = %d\n", i, droplist_[i]);
	}
}
#endif
	return dropcnt_;
}

/***** ***/

static class SelectErrorModelClass : public TclClass {
public:
	SelectErrorModelClass() : TclClass("SelectErrorModel") {}
	TclObject* create(int, const char*const*) {
		return (new SelectErrorModel);
	}
} class_selecterrormodel;

SelectErrorModel::SelectErrorModel()
{
	bind("pkt_type_", (int*)&pkt_type_);
	bind("drop_cycle_", &drop_cycle_);
	bind("drop_offset_", &drop_offset_);
}

int SelectErrorModel::command(int argc, const char*const* argv)
{
	if (strcmp(argv[1], "drop-packet") == 0) {
		pkt_type_ = packet_t(atoi(argv[2]));
		drop_cycle_ = atoi(argv[3]);
		drop_offset_ = atoi(argv[4]);
		return TCL_OK;
	}
	return ErrorModel::command(argc, argv);
}

int SelectErrorModel::corrupt(Packet* p)
{
	if (unit_ == EU_PKT) {
		hdr_cmn *ch = hdr_cmn::access(p);
		// XXX Backward compatibility for cbr agents
		if (ch->ptype() == PT_UDP && pkt_type_ == PT_CBR)
			pkt_type_ = PT_UDP; // "udp" rather than "cbr"
		if (ch->ptype() == pkt_type_ && ch->uid() % drop_cycle_ 
		    == drop_offset_) {
			//printf ("dropping packet type %d, uid %d\n", 
			//	ch->ptype(), ch->uid());
			return 1;
		}
	}
	return 0;
}

/* Error model for srm experiments */
class SRMErrorModel : public SelectErrorModel {
public:
	SRMErrorModel();
	virtual int corrupt(Packet*);
protected:
	int command(int argc, const char*const* argv);
};

static class SRMErrorModelClass : public TclClass {
public:
	SRMErrorModelClass() : TclClass("SRMErrorModel") {}
	TclObject* create(int, const char*const*) {
		return (new SRMErrorModel);
	}
} class_srmerrormodel;

SRMErrorModel::SRMErrorModel()
{
}

int SRMErrorModel::command(int argc, const char*const* argv)
{
	//int ac = 0;
	if (strcmp(argv[1], "drop-packet") == 0) {
		pkt_type_ = packet_t(atoi(argv[2]));
		drop_cycle_ = atoi(argv[3]);
		drop_offset_ = atoi(argv[4]);
		return TCL_OK;
	}
	return ErrorModel::command(argc, argv);
}

int SRMErrorModel::corrupt(Packet* p)
{
	if (unit_ == EU_PKT) {
		hdr_srm *sh = hdr_srm::access(p);
		hdr_cmn *ch = hdr_cmn::access(p);
		// XXX Backward compatibility for cbr agents
		if (ch->ptype()==PT_UDP && pkt_type_==PT_CBR && sh->type() == SRM_DATA)
			pkt_type_ = PT_UDP; // "udp" rather than "cbr"
		if ((ch->ptype() == pkt_type_) && (sh->type() == SRM_DATA) && 
		    (sh->seqnum() % drop_cycle_ == drop_offset_)) {
			//printf ("dropping packet type SRM-DATA, seqno %d\n", 
			//sh->seqnum());
			return 1;
		}
	}
	return 0;
}


static class MrouteErrorModelClass : public TclClass {
public:
	MrouteErrorModelClass() : TclClass("ErrorModel/Trace/Mroute") {}
	TclObject* create(int, const char*const*) {
		return (new MrouteErrorModel);
	}
} class_mrouteerrormodel;
 
MrouteErrorModel::MrouteErrorModel() : TraceErrorModel()
{
}

int MrouteErrorModel::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "drop-packet") == 0) {
			const char* s = argv[2];
			int n = strlen(s);
			if (n >= this->maxtype()) {
				// tcl.result("message type too big");
				return (TCL_ERROR);
			}
			strcpy(msg_type,s);
			return(TCL_OK);
		}
	}
	return TraceErrorModel::command(argc, argv);
}

int MrouteErrorModel::match(Packet* p)
{
	hdr_mcast_ctrl* ph = hdr_mcast_ctrl::access(p);
	int indx = strcspn(ph->type(),"/");
	if (!strncmp(ph->type(),msg_type,indx)) {
		return 1;
	}
	return 0;
}


static class ErrorModuleClass : public TclClass {
public:
	ErrorModuleClass() : TclClass("ErrorModule") {}
	TclObject* create(int, const char*const*) {
		return (new ErrorModule);
	}
} class_errormodule;

void ErrorModule::recv(Packet *p, Handler *h)
{
	classifier_->recv(p, h);
}

int ErrorModule::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "classifier") == 0) {
			if (classifier_)
				tcl.resultf("%s", classifier_->name());
			else
				tcl.resultf("");
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "classifier") == 0) {
			classifier_ = (Classifier*)
				TclObject::lookup(argv[2]);
			if (classifier_ == NULL) {
				tcl.resultf("Couldn't look up classifier %s", argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
	}
	return (Connector::command(argc, argv));
}

#include "config.h"
#ifdef HAVE_STL //pgm uses STL

#include "pgm/pgm.h"

static class PGMErrorModelClass : public TclClass {
public:
       PGMErrorModelClass() : TclClass("PGMErrorModel") {}
        TclObject* create(int, const char*const*) {
                return (new PGMErrorModel);
        }
} class_pgm_errormodel;

PGMErrorModel::PGMErrorModel() : ErrorModel(), pgm_type_(-1), count_(0)
{
        ndrops_ = 0;

        bind("ndrops_", &ndrops_);
}

int PGMErrorModel::command(int argc, const char*const* argv)
{
        if (strcmp(argv[1], "drop-packet") == 0) {
		if (!strcasecmp(argv[2], "SPM")) {
			pgm_type_ = PGM_SPM;
		}
		else if (!strcasecmp(argv[2], "ODATA")) {
			pgm_type_ = PGM_ODATA;
		}
		else if (!strcasecmp(argv[2], "RDATA")) {
			pgm_type_ = PGM_RDATA;
		}
		else if (!strcasecmp(argv[2], "NAK")) {
			pgm_type_ = PGM_NAK;
		}
		else if (!strcasecmp(argv[2], "NCF")) {
			pgm_type_ = PGM_NCF;
		}
		else {
			fprintf(stderr, "PGMErrorModel: drop-packet PGM type \"%s\" unknown.\n", argv[2]);
			return TCL_ERROR;
		}

                drop_cycle_ = atoi(argv[3]);
                drop_offset_ = atoi(argv[4]);
                return TCL_OK;
        }
        return ErrorModel::command(argc, argv);
}

int PGMErrorModel::corrupt(Packet* p)
{
        if (unit_ == EU_PKT) {

                hdr_cmn *ch = HDR_CMN(p);
                hdr_pgm *hp = HDR_PGM(p);

                if ((ch->ptype() == PT_PGM) && (hp->type_ == pgm_type_)) {
			count_++;
			if (count_ % drop_cycle_ == drop_offset_) {
#ifdef PGM_DEBUG
				printf ("DROPPING PGM packet type %d, seqno %d\n", pgm_type_, hp->seqno_);
#endif
				++ndrops_;
				return 1;
			}
		}
        }
        return 0;
}

#endif //HAVE_STL

//
// LMS Error Model
//
#include "rtp.h"                
#include "mcast/lms.h"

static class LMSErrorModelClass : public TclClass {
public:
        LMSErrorModelClass() : TclClass("LMSErrorModel") {}
        TclObject* create(int, const char*const*) {
                return (new LMSErrorModel);
        }
} class_lms_errormodel;
 
 
LMSErrorModel::LMSErrorModel() : ErrorModel()
	{
        ndrops_ = 0;
        bind("ndrops_", &ndrops_);
	}
 
int LMSErrorModel::command(int argc, const char*const* argv)
	{
        if (strcmp(argv[1], "drop-packet") == 0)
		{
                pkt_type_ = packet_t(atoi(argv[2]));
                drop_cycle_ = atoi(argv[3]);
                drop_offset_ = atoi(argv[4]);
                return TCL_OK;
		}
        return ErrorModel::command(argc, argv);
	}

int LMSErrorModel::corrupt(Packet* p)
{
        if (unit_ == EU_PKT)
		{
                hdr_cmn *ch = HDR_CMN(p);
                hdr_lms *lh = HDR_LMS(p);
                hdr_rtp *rh = HDR_RTP(p);
 
                if ((ch->ptype() == pkt_type_) && (lh->type_ != LMS_DMCAST) &&
                    (rh->seqno() % drop_cycle_ == drop_offset_))
			{
#ifdef LMS_DEBUG
printf ("Error Model: DROPPING pkt type %d, seqno %d\n", pkt_type_, rh->seqno());
#endif
                        ++ndrops_;
			return 1;
			}
        	}
	return 0;
}

void TwoStateErrModelTimer::expire(Event *) 
{
	(*a_.*call_back_)();
}

