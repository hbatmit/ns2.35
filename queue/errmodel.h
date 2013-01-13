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
 * @(#) $Header: /cvsroot/nsnam/ns-2/queue/errmodel.h,v 1.50 2005/04/26 18:56:35 haldar Exp $ (UCB)
 */

#ifndef ns_errmodel_h
#define ns_errmodel_h

#include "connector.h"
#include "timer-handler.h"
#include "ranvar.h"
#include "packet.h"
#include "basetrace.h"

enum ErrorUnit { EU_TIME=0, EU_BYTE, EU_PKT, EU_BIT };
#define EU_NAMES "time", "byte", "pkt", "bit"
#define STR2EU(s) (!strcmp(s,"time") ? EU_TIME : (!strcmp(s,"byte") ? EU_BYTE : (!strcmp(s, "bit") ? EU_BIT : EU_PKT)))

enum StTypeUnit {ST_TIME=0, ST_PKT };
#define ST_NAMES "time", "pkt"
#define STR2ST(s) (!strcmp(s,"time") ? ST_TIME : ST_PKT)

#define EM_GOOD	1
#define EM_BAD	2


/* 
 * Basic object for error models.  This can be used unchanged by error 
 * models that are characterized by a single parameter, the rate of errors 
 * (or equivalently, the mean duration/spacing between errors).  Currently,
 * this includes the uniform and exponentially-distributed models.
 */
class ErrorModel : public Connector {
public:
	ErrorModel();
	virtual void recv(Packet*, Handler*);
	virtual void reset();
	virtual int corrupt(Packet*);
	inline double rate() { return rate_; }
	inline ErrorUnit unit() { return unit_; }
	
protected:
	virtual int command(int argc, const char*const* argv);
	int CorruptPkt(Packet*);
	int CorruptTime(Packet*);
	int CorruptByte(Packet*);
	int CorruptBit(Packet*);
	double PktLength(Packet*);
	double * ComputeBitErrProb(int);

	// event-tracing
	virtual void trace_event(char *eventtype);
	EventTrace *et_;
	
	int enable_;		// true if this error module is turned on
	int markecn_;		// mark ecn instead of dropping on corruption?
	int delay_pkt_;		// delay packet instead of dropping
	int firstTime_;		// to not corrupt first packet in byte model
	ErrorUnit unit_;	// error unit in pkts, bytes, or time
	double rate_;		// uniform error rate in pkt or byte
	double delay_;		// time to delay packet
	double bandwidth_;	// bandwidth of the link
	RandomVariable *ranvar_;// the underlying random variate generator

	int FECstrength_;       // indicate how many corrupted bits are corrected
	int datapktsize_;
	int cntrlpktsize_;
	double *cntrlprb_;
	double *dataprb_;
	Event intr_;		// set callback to queue
	
};

class TwoStateErrorModel;
/* Timer for Errormodels using time to change states */
class TwoStateErrModelTimer : public TimerHandler {
public:
	TwoStateErrModelTimer(TwoStateErrorModel *a, void (TwoStateErrorModel::*call_back)())
		: a_(a), call_back_(call_back) {};
protected:
	virtual void expire (Event *e);
	TwoStateErrorModel *a_;
	void (TwoStateErrorModel::*call_back_)();
};

class TwoStateErrorModel : public ErrorModel {
	friend class ComplexTwoStateErrorModel;
public:
	TwoStateErrorModel();
	virtual int corrupt(Packet*);
	void setunit(ErrorUnit unit) {unit_ = unit;}
protected:
	int command(int argc, const char*const* argv);
	virtual int corruptPkt(Packet* p);
	virtual int corruptTime(Packet* p);
	virtual void checkUnit();
	void transitionState();
	int state_;		// state: 0=error-free, 1=error
	double remainLen_;	// remaining length of the current state
	RandomVariable *ranvar_[2]; // ranvar staying length for each state
	TwoStateErrModelTimer*  twoStateTimer_;
};

class ComplexTwoStateErrorModel : public TwoStateErrorModel {
public:
	ComplexTwoStateErrorModel();
	~ComplexTwoStateErrorModel();
protected:
	int command(int argc, const char*const* argv);
	virtual int corruptPkt(Packet* p);
	virtual int corruptTime(Packet* p);
	TwoStateErrorModel*  em_[2];
};

class MultiStateErrorModel : public ErrorModel {
public:
	MultiStateErrorModel();
	virtual int corrupt(Packet*);
protected:
	int command(int argc, const char*const* argv);
	int sttype_;            // type of state trans: 1: 'pkt' prob, 0: 'time'
	int texpired_;          // timed-state expired?
	double curperiod_;      // the duration of the current state
	double prevTime_;       // the last transition time of current state
	ErrorModel* em_;	// current error model to use
};


/* error model that reads a loss trace (instead of a math/computed model) */
class TraceErrorModel : public ErrorModel {
public:
	TraceErrorModel();
	virtual int match(Packet* p);
	virtual int corrupt(Packet* p);
protected:
	double loss_;
	double good_;
};


/*
 * periodic packet drops (drop every nth packet we see)
 * this can be conveniently combined with a flow-based classifier
 * to achieve drops in particular flows
 */
class PeriodicErrorModel : public ErrorModel {
public:
	PeriodicErrorModel();
	virtual int corrupt(Packet*);
protected:
	int cnt_;
	double period_;
	double offset_;
	double burstlen_;
	double last_time_;
	double first_time_;
	int default_drop_;	// 0 for regular, 1 to drop all
				//   but last pkt in period_
};


/*
 * List error model: specify which packets to drop in tcl
 */
class ListErrorModel : public ErrorModel {
public:
	ListErrorModel() : cnt_(0), droplist_(NULL),
		dropcnt_(0), cur_(0) { }
	~ListErrorModel() { if (droplist_) delete droplist_; }
	virtual int corrupt(Packet*);
	int command(int argc, const char*const* argv);
protected:
	int parse_droplist(int argc, const char *const*);
	static int nextval(const char*&p);
	static int intcomp(const void*, const void*);		// for qsort
	int cnt_;	/* cnt of pkts/bytes we've seen */
	int* droplist_;	/* array of pkt/byte #s to affect */
	int dropcnt_;	/* # entries in droplist_ total */
	int cur_;	/* current index into droplist_ */
};

/* For Selective packet drop */
class SelectErrorModel : public ErrorModel {
public:
	SelectErrorModel();
	virtual int corrupt(Packet*);
protected:
	int command(int argc, const char*const* argv);
	packet_t pkt_type_;
	int drop_cycle_;
	int drop_offset_;
};


/* error model for multicast routing,... now inherits from trace.. later
may make them separate and use pointer/containment.. etc */
class MrouteErrorModel : public TraceErrorModel {
public:
	MrouteErrorModel();
	virtual int match(Packet* p);
	inline int maxtype() { return sizeof(msg_type); }
protected:
	int command(int argc, const char*const* argv);
	char msg_type[15]; /* to which to copy the message code (e.g.
			    *  "prune","join"). It's size is the same
			    * as type_ in prune.h [also returned by maxtype.]
			    */
};

class Classifier;

class ErrorModule : public Connector {
public:
	ErrorModule() : classifier_(0) {}
protected:
	int command(int, const char*const*);
	void recv(Packet*, Handler*);
	Classifier* classifier_;
};

#ifdef HAVE_STL //pgm code uses STL

// PGM error model
class PGMErrorModel : public ErrorModel {
public:
        PGMErrorModel();
        virtual int corrupt(Packet*);

protected:
        int ndrops_;
        int command(int argc, const char*const* argv);
        int pgm_type_;
        int drop_cycle_;
        int drop_offset_;

	int count_;
};

#endif//HAVE_STL

// LMS error model
class LMSErrorModel : public ErrorModel {
public:
        LMSErrorModel();
        virtual int corrupt(Packet*);
 
protected:
	int ndrops_;
	int command(int argc, const char*const* argv);
	packet_t pkt_type_;
	int	drop_cycle_;
	int	drop_offset_;
	int	off_rtp_;
	int	off_lms_;
};




#endif 
