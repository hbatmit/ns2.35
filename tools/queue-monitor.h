/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- 
 *
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
 *      This product includes software developed by the MASH Research
 *      Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
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
 * @(#) $Header: /cvsroot/nsnam/ns-2/tools/queue-monitor.h,v 1.26 2005/07/13 03:51:33 tomh Exp $ (UCB)
 */

#ifndef ns_queue_monitor_h
#define ns_queue_monitor_h

#include "config.h"  // for int64_t
#include "integrator.h"
#include "connector.h"
#include "packet.h"
#include "flags.h"

class QueueMonitor : public TclObject {
public: 
	QueueMonitor() : bytesInt_(NULL), pktsInt_(NULL), delaySamp_(NULL),
		size_(0), pkts_(0),
		parrivals_(0), barrivals_(0),
		pdepartures_(0), bdepartures_(0),
		pdrops_(0), pmarks_(0), bdrops_(0), 
			 qs_pkts_(0), qs_bytes_(0), qs_drops_(0),
		keepRTTstats_(0), maxRTT_(1), numRTTs_(0), binsPerSec_(10),
		keepSeqnoStats_(0), maxSeqno_(1000), 
		numSeqnos_(0), SeqnoBinSize_(1),
		srcId_(0), dstId_(0), channel_(0), channel1_(0),
		estimate_rate_(0), 
		k_(0.1), 
		estRate_(0.0),
		temp_size_(0) {
		
		bind("size_", &size_);
		bind("pkts_", &pkts_);
		bind("parrivals_", &parrivals_);
		bind("barrivals_", &barrivals_);
		bind("pdepartures_", &pdepartures_);
		bind("bdepartures_", &bdepartures_);
		bind("pdrops_", &pdrops_);
		bind("pmarks_", &pmarks_);
		bind("bdrops_", &bdrops_);

		bind("qs_pkts_", &qs_pkts_);
		bind("qs_bytes_", &qs_bytes_);
		bind("qs_drops_", &qs_drops_);

		bind("first_pkt_", &first_pkt_);
		bind("last_pkt_", &last_pkt_);

		//for keeping RTT statistics
                bind_bool("keepRTTstats_", &keepRTTstats_);
		bind("maxRTT_", &maxRTT_);
		bind("binsPerSec_", &binsPerSec_);

		//for keeping sequence number statistics
                bind_bool("keepSeqnoStats_", &keepSeqnoStats_);
		bind("maxSeqno_", &maxSeqno_);
		bind("SeqnoBinSize_", &SeqnoBinSize_);

		//variable binding for flow rate estimation
		bind_bool("estimate_rate_", &estimate_rate_);
		bind("k_", &k_);
		bind("prevTime_", &prevTime_);
		bind("startTime_", &startTime_);
 		bind("estRate_", &estRate_);
		
		startTime_ = Scheduler::instance().clock();
		prevTime_  = startTime_;
	};

	int size() const { return (size_); }
	int pkts() const { return (pkts_); }
#if defined(HAVE_INT64)
	int64_t parrivals() const { return (parrivals_); }	
	int64_t barrivals() const { return (barrivals_); }
	int64_t pdepartures() const { return (pdepartures_); }
	int64_t bdepartures() const { return (bdepartures_); }
#else /* no 64-bit integer */
	int parrivals() const { return (parrivals_); }
	int barrivals() const { return (barrivals_); }
	int pdepartures() const { return (pdepartures_); }
	int bdepartures() const { return (bdepartures_); }
#endif
	int pdrops() const { return (pdrops_); }
	int pmarks() const { return (pmarks_); }
	int bdrops() const { return (bdrops_); }

	int qs_pkts() const { return (qs_pkts_); }
	int qs_bytes() const { return (qs_bytes_); }
	int qs_drops() const { return (qs_drops_); }

	double first_pkt() const { return (first_pkt_); }

	void printRTTs();
	void printSeqnos();
	void printStats();
	virtual void in(Packet*);
	virtual void out(Packet*);
	virtual void drop(Packet*);
	virtual void edrop(Packet*) { abort(); }; // not here
	virtual int command(int argc, const char*const* argv);
protected:
	Integrator *bytesInt_;		// q-size integrator (bytes)
	Integrator *pktsInt_;		// q-size integrator (pkts)
	Samples* delaySamp_;		// stat samples of q delay
	int size_;			// current queue size (bytes)
	int pkts_;			// current queue size (packets)
	// aggregate counters bytes/packets
#if defined(HAVE_INT64)
	int64_t parrivals_;
	int64_t barrivals_;
	int64_t pdepartures_;
	int64_t bdepartures_;
#else /* no 64-bit integer */
	int parrivals_;
	int barrivals_;
	int pdepartures_;
	int bdepartures_;
#endif
	int pdrops_;
	int pmarks_;
	int bdrops_;

	int qs_pkts_;			/* Number of Quick-Start packets */
	int qs_bytes_;			/* Number of Quick-Start bytes */
	int qs_drops_;			/* Number of dropped QS packets */

	double first_pkt_;		/* Time of first packet arrival */
	double last_pkt_;		/* Time of last packet arrival */

        int keepRTTstats_;		/* boolean - keeping RTT stats? */
	int maxRTT_;			/* Max RTT to measure, in seconds */
	int numRTTs_;			/* number of RTT measurements */
	int binsPerSec_;		/* number of bins per second */
	int *RTTbins_;			/* Number of RTTs in each bin */

        int keepSeqnoStats_;		/* boolean - keeping Seqno stats? */
	int maxSeqno_;			/* Max Seqno to measure */
	int numSeqnos_;			/* number of Seqno measurements */
	int SeqnoBinSize_;		/* number of Seqnos per bin */
	int *SeqnoBins_;		/* Number of Seqnos in each bin */

	int srcId_;
	int dstId_;
	Tcl_Channel channel_;
	Tcl_Channel channel1_;

	// the estimation of incoming rate using an exponential averaging algorithm due to Stoica
	// hence a lot of this stuff is inspired by csfq.cc(Stoica)
	// put in here so that it can be used to estimate the arrival rate for both whole queues as 
	// well as flows (Flow inherits from EDQueueMonitor);
public:
	int estimate_rate_;           /* boolean - whether rate estimation is on or not */
	double k_;                    /* averaging interval for rate estimation in seconds*/
	double estRate_;              /* current flow's estimated rate in bps */
	double prevTime_;             /* time of last packet arrival */
	double startTime_;            /* time when the flow started */

protected:
	int temp_size_;               /* keep track of packets that arrive at the same time */

	void estimateRate(Packet *p);
	void keepRTTstats(Packet *p);
	void keepSeqnoStats(Packet *p);
};

class SnoopQueue : public Connector {
public: 
	SnoopQueue() : qm_(0) {}
	int command(int argc, const char*const* argv) {
		if (argc == 3) { 
			if (strcmp(argv[1], "set-monitor") == 0) {
				qm_ = (QueueMonitor*)
					TclObject::lookup(argv[2]);
				if (qm_ == NULL)
					return (TCL_ERROR);
				return (TCL_OK);
			}
		}
		return (Connector::command(argc, argv));
	}
 protected:
	QueueMonitor* qm_;
};

class SnoopQueueIn : public SnoopQueue {
public:
	void recv(Packet* p, Handler* h) {
		qm_->in(p);
		send(p, h);
	}
};

class SnoopQueueOut : public SnoopQueue {
public:
	void recv(Packet* p, Handler* h) {
		qm_->out(p);
		send(p, h);
	}
};

class SnoopQueueDrop : public SnoopQueue {
public:
	void recv(Packet* p, Handler* h) {
		qm_->drop(p);
		send(p, h);
	}
};

/* Tagger, Like a normal FlowMonitor, use SnoopQueueTagger
 * to start it.
 * By Yun Wang 
 */
class SnoopQueueTagger : public SnoopQueue {
public:
        void recv(Packet* p, Handler* h) {
                qm_->in(p);
                send(p, h);
        }
};

/*
 * "early drop" QueueMonitor.  Like a normal QueueMonitor,
 * but also supports the notion of "early" drops
 */

/* 
 * The mon* things added to make it work with redpd. 
 * I tried more "elegant" ways -- but mulitple inheritance sucks !!.
 * -ratul
 */
class EDQueueMonitor : public QueueMonitor {
public:
	EDQueueMonitor() : ebdrops_(0), epdrops_(0), mon_ebdrops_(0), mon_epdrops_(0) {
		bind("ebdrops_", &ebdrops_);
		bind("epdrops_", &epdrops_);
		bind("mon_ebdrops_", &mon_ebdrops_);
		bind("mon_epdrops_", &mon_epdrops_);
	}
	void edrop(Packet* p) {
		hdr_cmn* hdr = hdr_cmn::access(p);
		ebdrops_ += hdr->size();
		epdrops_++;
		// remove later - ratul
		// printf("My epdrops = %d\n",epdrops_);
		QueueMonitor::drop(p);
	}
	
	void mon_edrop(Packet *p) {
		hdr_cmn* hdr = hdr_cmn::access(p);
		mon_ebdrops_ += hdr->size();
		mon_epdrops_++;
	
		QueueMonitor::drop(p);
	}
	
	int epdrops() const { return (epdrops_); }
	int ebdrops() const { return (ebdrops_); }
	int mon_epdrops() const { return (mon_epdrops_); }
	int mon_ebdrops() const { return (mon_ebdrops_); }
protected:
	int	ebdrops_;
	int	epdrops_;
	int	mon_ebdrops_;
	int	mon_epdrops_;
};

class SnoopQueueEDrop : public SnoopQueue {
public:
	void recv(Packet* p, Handler* h) {
		qm_->edrop(p);
		send(p, h);
	}
};


#ifndef	MAXFLOW
#define	MAXFLOW	32
#endif

/*
 * a 'QueueMonitorCompat', which is used by the compat
 * code to produce the link statistics available in ns-1
 */

class QueueMonitorCompat : public QueueMonitor {
public:
	QueueMonitorCompat();
	void in(Packet*);
	void out(Packet*);
	void drop(Packet*);
	int command(int argc, const char*const* argv);
protected:
	void	flowstats(int flowid);	/* create a flowstats structure */

	int	pkts_[MAXFLOW];
	int	bytes_[MAXFLOW];
	int	drops_[MAXFLOW];
	Samples *flowstats_[MAXFLOW];
};

#endif

