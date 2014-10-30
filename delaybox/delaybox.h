/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/* 
 * Copyright 2002, The University of North Carolina at Chapel Hill
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright 
 * notice, this list of conditions and the following disclaimer in the 
 * documentation and/or other materials provided with the distribution.
 *    3. The name of the author may not be used to endorse or promote 
 * products derived from this software without specific prior written 
 * permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Reference
 *     Stochastic Models for Generating Synthetic HTTP Source Traffic 
 *     J. Cao, W.S. Cleveland, Y. Gao, K. Jeffay, F.D. Smith, and M.C. Weigle 
 *     IEEE INFOCOM 2004.
 *
 * Documentation available at http://dirt.cs.unc.edu/delaybox/
 * 
 * Contacts: Michele Weigle (mcweigle@cs.unc.edu),
 *           Kevin Jeffay (jeffay@cs.unc.edu)
 */

#include "classifier.h"
#include "node.h"
#include "ranvar.h"
#include <map>

class DelayBoxClassifier;

void packet_string (char* str, hdr_tcp* tcph, hdr_ip* iph, int size);

/*::::::::::::::::: DELAYBOX PAIR ::::::::::::::::::::::::::::::::*/

class DelayBoxPair {
public:
	DelayBoxPair() {};
	DelayBoxPair(int s, int d, int f=0) : src_(s), dst_(d), fid_(f) {};
	DelayBoxPair(const DelayBoxPair&);
	bool operator< (const DelayBoxPair&) const;
	bool operator==(const DelayBoxPair&) const;
	const DelayBoxPair& operator=(const DelayBoxPair&);
	void format (char*) const;
	void format_short (char*) const;

protected:
	int src_;
	int dst_;
	int fid_;
};

/*::::::::::::::::: DELAYBOX TIMER ::::::::::::::::::::::::::::::::*/

class DelayBoxTimer : public TimerHandler {
public:
	DelayBoxTimer(DelayBoxClassifier *a, int src, int dst, int fid) 
		: TimerHandler(), a_(a), src_(src), dst_(dst), fid_(fid) {};
  
protected:
	virtual void expire(Event *);
	DelayBoxClassifier *a_;
	int src_;
	int dst_;
	int fid_;
};

/*::::::::::::::::: DELAYBOX QUEUE ::::::::::::::::::::::::::::::::*/

class DelayBoxQueue {
	struct pktinfo {
		pktinfo* next_;  // forward link
		Packet* pkt_;    // packet
		double delta_;   // delta from last pkt
	};

public:
	DelayBoxQueue() : head_(NULL), tail_(NULL), deltasum_(0) {}
	~DelayBoxQueue();
	int empty() { return (head_ == NULL); }
	double add(Packet*, double, double);
	Packet* dequeue(double*);
	void clear();
	int oneitem() { return (head_->next_ == NULL); }
	void dumplist();        // for debugging

protected:
	pktinfo* head_;         // head of linked list
	pktinfo* tail_;         // tail of linked list
	double deltasum_;       // sum of deltas in queue
};

/*::::::::::::::::: DELAYBOX RULE ::::::::::::::::::::::::::::::::*/

class DelayBoxRule {
	friend struct DelayBoxClassifier;
public:
	DelayBoxRule(RandomVariable* delay, RandomVariable* loss, 
	     RandomVariable* linkspd) : delay_(delay), loss_(loss),
					linkspd_(linkspd) {};
	DelayBoxRule(RandomVariable* delay, RandomVariable* loss) : 
		delay_(delay), loss_(loss), linkspd_(NULL) {};
	DelayBoxRule(RandomVariable* delay) : 
		delay_(delay), loss_(NULL), linkspd_(NULL) {};

protected:
	RandomVariable* delay_;
	RandomVariable* loss_;
	RandomVariable* linkspd_;
};

/*::::::::::::::::: DELAYBOX FLOW ::::::::::::::::::::::::::::::::*/

class DelayBoxFlow {
	friend struct DelayBoxClassifier;
	friend struct Tmix_DelayBoxClassifier;
public:
	DelayBoxFlow(double delay, double loss, double linkspd, 
		     DelayBoxQueue* q, DelayBoxTimer* timer) : delay_(delay), 
		loss_(loss), linkspd_(linkspd), queue_(q), timer_(timer) {};
	DelayBoxFlow(const DelayBoxFlow& f);
	void format (char*);
	void format_delay (char*);

protected:
	double delay_;
	double loss_;
	double linkspd_;
	DelayBoxQueue* queue_;
	DelayBoxTimer* timer_;
};


/*::::::::::::::::: DELAYBOX CLASSIFIER ::::::::::::::::::::::::::::::::*/

class DelayBoxClassifier : public Classifier {
public:
	DelayBoxClassifier() : Classifier(), debug_(0), rttfp_(NULL),
			       symmetric_(1) {};
	~DelayBoxClassifier();
	inline double now() {return Scheduler::instance().clock();}
	void timeout(int src, int dst, int fid);
	void add_rule (const char*const src, const char*const dst, 
		       const char*const dly, const char*const loss, 
		       const char*const linkspd);
	void add_rule (const char*const src, const char*const dst, 
		       const char*const dly, const char*const loss);
	void add_rule (const char*const src, const char*const dst, 
		       const char*const dly);
	void list_rules();
	void list_flows();
	void output_delay(int src, int dst, int fid, FILE* fp);
	inline void set_debug (int d) { debug_ = d; }
	inline void set_asymmetric(void) { symmetric_ = 0; }
	void setfp (FILE* fp) {rttfp_ = fp;}

protected:
	int classify (Packet *p);
	void forward_packet (Packet *p);
	virtual void recv(Packet *p, Handler *h);

	map<DelayBoxPair, DelayBoxRule*> rules_;
	map<DelayBoxPair, DelayBoxFlow*> flows_;
	int debug_;
	FILE* rttfp_;
	int symmetric_;   // use symmetric delay (same on data/ACK path)
};

/*::::::::::::::::: DELAYBOX NODE :::::::::::::::::::::::::::::::::::::*/

class DelayBoxNode : public Node {
public:
	DelayBoxNode();
	~DelayBoxNode();
	int command(int argc, const char*const* argv);

protected:
	DelayBoxClassifier* classifier_;
	FILE* rttfp_;
};

