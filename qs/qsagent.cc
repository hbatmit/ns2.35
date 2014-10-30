
/*
 * qsagent.cc
 * Copyright (C) 2001 by the University of Southern California
 * $Id: qsagent.cc,v 1.9 2010/03/08 05:54:53 tom_henderson Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */

/*
 * Quick Start for TCP and IP.
 * A scheme for transport protocols to dynamically determine initial 
 * congestion window size.
 *
 * http://www.ietf.org/internet-drafts/draft-amit-quick-start-02.ps
 *
 * This implements the Quick Start Agent at each of network element "Agent/QSAgent"
 * qsagent.cc
 *
 * Srikanth Sundarrajan, 2002
 * sundarra@usc.edu
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <float.h>

#include "object.h"
#include "agent.h"
#include "packet.h"
#include "ip.h"
#include "classifier.h"
#include "connector.h"
#include "delay.h"
#include "queue.h"
#include "scheduler.h"
#include "random.h"

#include "hdr_qs.h"
#include "qsagent.h"
#include <fstream>

static class QSAgentClass : public TclClass {
public:
	QSAgentClass() : TclClass("Agent/QSAgent") {}
	TclObject* create(int, const char*const*) {
		return (new QSAgent);
	}
} class_QSAgent;

int QSAgent::rate_function_ = 1;

QSAgent::QSAgent():Agent(PT_TCP), old_classifier_(NULL), qs_enabled_(1), 
    qs_timer_(this) 
{

	prev_int_aggr_ = 0;
	aggr_approval_ = 0;

	bind("qs_enabled_", &qs_enabled_);
	bind("old_classifier_", &old_classifier_);
	bind("state_delay_", &state_delay_);
	bind("alloc_rate_", &alloc_rate_);
	bind("threshold_", &threshold_);
	bind("max_rate_", &max_rate_);
	bind("mss_", &mss_);
	bind("rate_function_", &rate_function_);
	bind("algorithm_", &algorithm_);

	qs_timer_.resched(state_delay_);
  
}

QSAgent::~QSAgent()
{
}

int QSAgent::command(int argc, const char*const* argv)
{
	return (Agent::command(argc,argv));
}

void QSAgent::recv(Packet* packet, Handler*)
{
	double app_rate;
	//double avail_bw, util;
	Classifier * pkt_target;
	Tcl& tcl = Tcl::instance();
	char qname[64], lname[64];

	hdr_qs *qsh =  hdr_qs::access(packet);
	hdr_ip *iph = hdr_ip::access(packet);

	assert (old_classifier_ != 0);

	pkt_target = (Classifier *)TclObject::lookup(old_classifier_->name());

	if (qs_enabled_) {
		if (qsh->flag() == QS_REQUEST && qsh->rate() > 0 && iph->daddr() != addr()) {
			sprintf (qname, "[Simulator instance] get-queue %d %d", addr(), iph->daddr()); 
			tcl.evalc (qname);
			Queue * queue = (Queue *) TclObject::lookup(tcl.result());

			sprintf (lname, "[Simulator instance] get-link %d %d", addr(), iph->daddr()); 
			tcl.evalc (lname);
			LinkDelay * link = (LinkDelay *) TclObject::lookup(tcl.result());

			if (link != NULL && queue != NULL) {
				app_rate = process(link, queue, hdr_qs::rate_to_Bps(qsh->rate()));
				if (app_rate > 0) {    
					qsh->ttl() -= 1;
					qsh->rate() = hdr_qs::Bps_to_rate(app_rate); //update rate
				}
				else {
					qsh->rate() = 0; //disable quick start, not enough bandwidth
					qsh->flag() = QS_DISABLE;
				}
			}
		}
	}
	
	pkt_target->recv(packet, 0);

	return;
  
}


double QSAgent::process(LinkDelay *link, Queue *queue, double ratereq)
{
	double util, avail_bw, app_rate, util_bw;

	// PS: avail_bw is in units of bytes per sec.
	if (algorithm_ == 1) {
		/*
		 */
		util = queue->utilization();
		avail_bw = link->bandwidth() / 8 * (1 - util);
		avail_bw -= (prev_int_aggr_ + aggr_approval_);
		avail_bw *= alloc_rate_;
		app_rate = (avail_bw < ratereq) ? (int) avail_bw : ratereq;
		app_rate = (app_rate < (max_rate_ * 1024)) ?
			app_rate : (max_rate_ * 1024);
		if (app_rate > 0) {
			// add approved to current bucket
			aggr_approval_ += app_rate;
		}
	} else if (algorithm_ == 2) {
		/*
		 * Algorithm 2 checks if the utilized bandwidth is
		 *  less than some fraction (threshold_) of
		 *  the total bandwidth.  If so, the approved rate
		 *  is at most some fraction (alloc_rate_) of the
		 *  link bandwidth.
		 */
		util = queue->utilization();
		util_bw = link->bandwidth() / 8 * util;
		util_bw += (prev_int_aggr_ + aggr_approval_);
		if (util_bw < threshold_ * link->bandwidth() / 8) {
			app_rate = alloc_rate_ * link->bandwidth() / 8;
			if (ratereq < app_rate)
				app_rate = ratereq;
		} else {
			app_rate = 0;
		}
		aggr_approval_ += app_rate;
	} else if (algorithm_ == 3) {
		/*
		 * Algorithm 3 checks if the utilized bandwidth is
		 *  less than some fraction (threshold_) of
		 *  the total bandwidth.  If so, the approved rate
		 *  is at most the allowed allocated bandwidth minus
		 *  the utilized bandwidth.
		 *
		 * Algorithm 3 used queue->peak_utilization() instead of
		 *   queue->utilization().  This looks at the peak
		 *   utilization measures over a sub-interval of a
		 *   larger interval.
		 */
		util = queue->peak_utilization();
		util_bw = link->bandwidth() / 8 * util;
		util_bw += (prev_int_aggr_ + aggr_approval_);
		if (util_bw < threshold_ * link->bandwidth() / 8) {
			app_rate = alloc_rate_ * link->bandwidth() / 8
				     - util_bw;
			if (ratereq < app_rate)
				app_rate = ratereq;
			if (app_rate <  0) 
				app_rate = 0;
		} else {
			app_rate = 0;
		}
		aggr_approval_ += app_rate;
	} else if (algorithm_ == 4) {
		// a broken router: yes to all QS requests
		app_rate = ratereq;
	} else {
		app_rate = 0;
	}

#ifdef QS_DEBUG
		printf("%d: requested = %f KBps, available = %f KBps, approved = %f KBps\n", addr(), ratereq/1024, free_bw/1024, app_rate/1024);
#endif

	return app_rate;
}


void QSTimer::expire(Event *) {
	
	qs_handle_->prev_int_aggr_ = qs_handle_->aggr_approval_;
	qs_handle_->aggr_approval_ = 0;

	this->resched(qs_handle_->state_delay_);

}


/*
Local Variables:
c-basic-offset: 8
End:
*/
