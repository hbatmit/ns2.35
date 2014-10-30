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

/*
 * Overview
 * --------
 * Node/DelayBox consists of a Classifier/DelayBox that sits
 * in front of the default classifier and delays and/or drops
 * packets.  After a packet has been delayed, Classifier/DelayBox
 * passes the packet on to the default classifier.
 *
 * There are two tables:
 *   1) Rule Table - specified in the simulation script by user
 *                 - gives an outline of how flows between a pair 
 *                   of hosts should be treated (includes delay/loss 
 *                   distributions)
 *                 - entry for each src/dst pair
 *                 - fields are src, dst, delayRV, lossRV, link speed RV
 *
 *   2) Flow Table - created by Node/DelayBox
 *                 - specifies exactly how each flow should be handled
 *                 - values are obtained by sampling from the distributions
 *                   given in the rule table
 *                 - entry for each flow
 *                 - fields are src, dst, flow id, delay, loss, link
 *                   speed, ptr to queue
 *  
 * There are also a set of queues that handle delaying packets.  There
 * is one queue per flow.  Each flow table entry contains a pointer to
 * the head of the flow's queue.
 *
 * Flows are defined as the first SYN of a new flow id to the first
 * FIN received.  Packets after the first FIN that complete the 
 * connection shutdown sequence are NOT delayed or dropped.
 *
 * Use of DelayBox in a Tcl Script
 * -------------------------------
 * Creating Node/DelayBox:    set db [$ns DelayBox]
 *
 * Adding rules:
 * $db add-rule [$n(0) id] [$n(1) id] $client_delay $loss_rate $client_bw
 * $db add-rule [$n(1) id] [$n(0) id] $server_delay $loss_rate $server_bw
 *
 * List current rules:        $db list-rules
 * List current flows:        $db list-flows
 *
 */

#include <tclcl.h>
#include "delaybox.h"
#include "ip.h"
#include "tcp.h"
#include "ranvar.h"

// for packet_string() and recv()
#define TH_FIN  0x01        /* FIN: closing a connection */
#define TH_SYN  0x02        /* SYN: starting a connection */
#define TH_PUSH 0x08        /* PUSH: used here to "deliver" data */
#define TH_ACK  0x10        /* ACK: ack number is valid */

#define MbPS2BPS_FACTOR 125000

TclObject* lookup_obj(const char* name) {
	TclObject* obj = Tcl::instance().lookup(name);
	if (obj == NULL) 
		fprintf(stderr, "Bad object name %s\n", name);
	return obj;
}

/*::::::::::::::::: DELAYBOX PAIR :::::::::::::::::::::::::::::::::::::*/

bool DelayBoxPair::operator< (const DelayBoxPair& p2) const
{
	if (src_ != p2.src_)
		return (src_ < p2.src_);
	else if (dst_ != p2.dst_)
		return (dst_ < p2.dst_);
	else
		return (fid_ < p2.fid_);
}

bool DelayBoxPair::operator== (const DelayBoxPair& p2) const
{
	if (src_ != p2.src_)
		return false;
	else if (dst_ != p2.dst_)
		return false;
	else if ((fid_ == 0) || (p2.fid_ == 0))
		return true;
	else
		return false;
}

DelayBoxPair::DelayBoxPair(const DelayBoxPair& p)
{
	src_ = p.src_;
	dst_ = p.dst_;
	fid_ = p.fid_;
}

const DelayBoxPair& DelayBoxPair::operator=(const DelayBoxPair& p)
{
	if (this != &p) {
		src_ = p.src_;
		dst_ = p.dst_;
		fid_ = p.fid_;
	}
	return (*this);
}

void DelayBoxPair::format (char *str) const
{
	sprintf (str, "%d > %d: %d", src_, dst_, fid_);
}

void DelayBoxPair::format_short(char *str) const
{
	sprintf (str, "%-3d %-3d %-5d", src_, dst_, fid_);
}

/*::::::::::::::::: DELAYBOX FLOW :::::::::::::::::::::::::::::::::::::*/

DelayBoxFlow::DelayBoxFlow(const DelayBoxFlow& f)
{
	delay_ = f.delay_;
	loss_ = f.loss_;
	linkspd_ = f.linkspd_;
	queue_ = f.queue_;
	timer_ = f.timer_;
}

void DelayBoxFlow::format(char *str)
{
	if (linkspd_ > 0)
		sprintf (str, "%8.3f ms delay  %5.2f%% loss  %8.3f Mbps", 
			 delay_ * 1000.0, loss_, linkspd_ / MbPS2BPS_FACTOR);
	else
		sprintf (str, "%8.3f ms delay  %5.2f%% loss", 
			 delay_ * 1000.0, loss_);
}

void DelayBoxFlow::format_delay(char *str)
{
	sprintf (str, "%8.3f", delay_ * 1000.0);
}

/*::::::::::::::::: DELAYBOX NODE :::::::::::::::::::::::::::::::::::::*/

static class DelayBoxNodeClass : public TclClass {
public:
	DelayBoxNodeClass() : TclClass("Node/DelayBox") {};
	TclObject* create(int, const char*const*) {
		return (new DelayBoxNode);
	}
} class_delayboxnode;

DelayBoxNode::DelayBoxNode() : Node()
{
	classifier_ = NULL;
}

DelayBoxNode::~DelayBoxNode()
{
	delete classifier_;
}

int DelayBoxNode::command(int argc, const char*const* argv) {
	if (argc == 7) {
		/* $db add-rule [$n(0) id] [$n(1) id] $delay $loss_rate $bw */
		if (!strcmp (argv[1], "add-rule")) {
			classifier_->add_rule(argv[2], argv[3], argv[4], 
					     argv[5], argv[6]);
			return (TCL_OK);
		}
	}
	if (argc == 6) {
		/* $db add-rule [$n(0) id] [$n(1) id] $delay $loss_rate */
		if (!strcmp (argv[1], "add-rule")) {
			classifier_->add_rule(argv[2], argv[3], argv[4], 
					     argv[5]);
			return (TCL_OK);
		}
	}
	else if (argc == 5) {
		/* $db add-rule [$n(0) id] [$n(1) id] $delay */
		if (!strcmp (argv[1], "add-rule")) {
			classifier_->add_rule(argv[2], argv[3], argv[4]); 
			return (TCL_OK);
		}
		else if (!strcmp (argv[1], "output-delay")) {
			if (!rttfp_)
				return (TCL_ERROR);
			classifier_->output_delay(atoi(argv[2]), 
						  atoi(argv[3]), 
						  atoi(argv[4]), rttfp_);
			return (TCL_OK);
		}
	}
	else if (argc == 3) {
		if (!strcmp (argv[1], "set-debug")) {
			classifier_->set_debug(atoi (argv[2]));
			return (TCL_OK);
		}
		else if (strcmp (argv[1], "set-delay-file") == 0) {
			rttfp_ = fopen (argv[2], "w");
			classifier_->setfp(rttfp_);
			if (rttfp_)
				return (TCL_OK);
			else 
				return (TCL_ERROR);
		}
	}
	else if (argc == 2) {
		if (!strcmp (argv[1], "list-rules")) {
			classifier_->list_rules();
			return (TCL_OK);
		}
		else if (!strcmp (argv[1], "list-flows")) {
			classifier_->list_flows();
			return (TCL_OK);

		}
		else if (!strcmp (argv[1], "create-classifier")) {
			Tcl& tcl = Tcl::instance();
			tcl.evalf ("%s entry", name());
			classifier_ = (DelayBoxClassifier*) lookup_obj 
				(tcl.result());
			return (TCL_OK);
		}
		else if (!strcmp (argv[1], "close-delay-file")) {
			if (rttfp_) 
				fclose(rttfp_);
			return (TCL_OK);
		}
		else if (!strcmp (argv[1], "set-asymmetric")) {
			classifier_->set_asymmetric();
			return TCL_OK;
		}
	}
	return (Node::command(argc, argv));
}

/*::::::::::::::::: DELAYBOX CLASSIFIER ::::::::::::::::::::::::::::::::*/

static class DelayBoxClassifierClass : public TclClass {
public:
	DelayBoxClassifierClass() : TclClass("Classifier/DelayBox") {}
	TclObject* create (int, const char*const*) {
		return (new DelayBoxClassifier);
	}
} class_delayboxclassifier;

DelayBoxClassifier::~DelayBoxClassifier()
{
	// delete the rule table	
	map<DelayBoxPair, DelayBoxRule*>::iterator rule;
	for (rule = rules_.begin(); rule != rules_.end(); rule++) {
		delete rule->second;
		rules_.erase(rule);
	}

	// delete the flow table
	map<DelayBoxPair, DelayBoxFlow*>::iterator flow;
	for (flow = flows_.begin(); flow != flows_.end(); flow++) {
		delete flow->second->queue_;
		flow->second->timer_->force_cancel();
		delete flow->second->timer_;
		delete flow->second;
		flows_.erase(flow);
	}
}

void DelayBoxClassifier::list_rules()
{
	if (rules_.size() == 0) {
		fprintf (stderr, "\nClass %s> Rules table is empty\n", name());
		return;
	}

	map<DelayBoxPair, DelayBoxRule*>::iterator rule;
	char pair_str[50];
	int i=1;

	fprintf (stderr, "\nClass %s> Rules:  (%d elements)\n", name(),
		 (int) rules_.size());
	for (rule = rules_.begin(); rule != rules_.end(); rule++, i++) {
		rule->first.format(pair_str);
		fprintf (stderr, "%4d) %s\n", i, pair_str);
	}
	fprintf (stderr, "\n");
}

void DelayBoxClassifier::list_flows()
{
	if (flows_.size() == 0) {
		fprintf (stderr, "\nClass %s> Flows table is empty\n", name());
		return;
	}

	map<DelayBoxPair, DelayBoxFlow*>::iterator flow;
	char pair_str[50];
	char flow_str[80];
	int i=0;

	fprintf (stderr, "\nClass %s> Flows:   (%d elements)\n", name(),
		 (int) flows_.size());
	for (flow = flows_.begin(); flow != flows_.end(); flow++, i++) {
		flow->first.format(pair_str);
		flow->second->format(flow_str);
		fprintf (stderr, "%4d) %s %s\n", i, pair_str, flow_str);
	}
	fprintf (stderr, "\n");
}

void DelayBoxClassifier::output_delay(int src, int dst, int fid, FILE* fp)
{
	double delay;

	// find delay information for this flow
	DelayBoxPair fwd_pair = DelayBoxPair (src, dst, fid);
	DelayBoxPair rev_pair = DelayBoxPair (dst, src, fid);

	map<DelayBoxPair, DelayBoxFlow*>::iterator fwd_flow = 
		flows_.find(fwd_pair);
	if (fwd_flow == flows_.end())
		return;    // flow not found
	
	map<DelayBoxPair, DelayBoxFlow*>::iterator rev_flow = 
		flows_.find(rev_pair);
	if (rev_flow == flows_.end())
		return;    // flow not found

	// compute delay
	delay = fwd_flow->second->delay_ + rev_flow->second->delay_;

	// output delay
	if (fp) {
		fprintf (fp, "%d  %f\n", fid, delay);
		fflush (fp);
	}
}

void DelayBoxClassifier::add_rule(const char* src, const char* dst, 
				  const char* dly, const char* loss, 
				  const char* linkspd)
/* $db add-rule [$n(0) id] [$n(1) id] $delay $loss_rate $bw */

/*
 * "add-rule" command - create rule struct
 *                    - add rule to rule table
 * 
 * SYN - lookup fid in flow table
 *     - if not there, create element 
 *            - find src/dst in rule table
 *            - sample from RVs
 *            - create queue and timer
 *     - add element to flow table
 *
 * packet - lookup fid in flow table
 *        - if not there, pass to default classifier
 *        - if loss > 0, sample [0:1]
 *        - if not dropped, add to delay Q
 *        - set flow's timer
 *
 * FIN - delay packet
 *     - remove fid from flow table
 *
 * timeout - lookup flow
 *         - remove packet from Q
 *         - pass to default classifier
 */

{
	// parse the arguments
	int source = atoi (src);
	int dest = atoi (dst);
	RandomVariable* delay = (RandomVariable*) lookup_obj (dly);
	RandomVariable* loss_rate = (RandomVariable*) lookup_obj (loss);
	RandomVariable* link_speed = (RandomVariable*) lookup_obj (linkspd);

	// create a new pair
	DelayBoxPair* pair = new DelayBoxPair (source, dest);

	// create a new rule
	DelayBoxRule* rule = new DelayBoxRule (delay, loss_rate, 
					       link_speed);

	// add to the rule table
	rules_[*pair] = rule;
}

void DelayBoxClassifier::add_rule(const char* src, const char* dst, 
				  const char* dly, const char* loss)
/* $db add-rule [$n(0) id] [$n(1) id] $delay $loss_rate */

/*
 * "add-rule" command - create rule struct
 *                    - add rule to rule table
 */
{
	// parse the arguments
	int source = atoi (src);
	int dest = atoi (dst);
	RandomVariable* delay = (RandomVariable*) lookup_obj (dly);
	RandomVariable* loss_rate = (RandomVariable*) lookup_obj (loss);

	// create a new pair
	DelayBoxPair* pair = new DelayBoxPair (source, dest);

	// create a new rule
	DelayBoxRule* rule = new DelayBoxRule (delay, loss_rate);

	// add to the rule table
	rules_[*pair] = rule;
}

void DelayBoxClassifier::add_rule(const char* src, const char* dst, 
				  const char* dly)
/* $db add-rule [$n(0) id] [$n(1) id] $delay */

/*
 * "add-rule" command - create rule struct
 *                    - add rule to rule table
 */
{
	// parse the arguments
	int source = atoi (src);
	int dest = atoi (dst);
	RandomVariable* delay = (RandomVariable*) lookup_obj (dly);

	// create a new pair
	DelayBoxPair* pair = new DelayBoxPair (source, dest);

	// create a new rule
	DelayBoxRule* rule = new DelayBoxRule (delay);

	// add to the rule table
	rules_[*pair] = rule;
}

int DelayBoxClassifier::classify(Packet *) {
	/* 
	 * Everything should be classified into slot 0, which points
	 * to the default classifier.
	 */
	return 0;
}

void DelayBoxClassifier::recv (Packet* p, Handler* )
{
	DelayBoxFlow* flow = NULL;
	double delay = 0.0, loss = 0, linkspd = 0;

	// debugging
	hdr_tcp *tcph = hdr_tcp::access(p);
	hdr_cmn *ch = hdr_cmn::access(p); 
	hdr_ip *iph = hdr_ip::access(p);

	int src = iph->src_.addr_;
	int dst = iph->dst_.addr_;
	int fid = iph->flowid();

	int action = -1;   // 0 - nothing, 1 - add, 2 - add rev dir

	// lookup flow in flow table
	DelayBoxPair pair = DelayBoxPair (src, dst, fid);
	map<DelayBoxPair, DelayBoxFlow*>::iterator flow_iter = 
		flows_.find(pair);

	if (flow_iter == flows_.end()) {
		/*
		 * flow not found in table
		 */
		if ((tcph->flags() & 0x0) == 0x0) {
			// one-way TCP flow
			int size = ch->size();
			int seqno = tcph->seqno();
			packet_t t = ch->ptype();
			const char* name = packet_info.name(t);
			
			if (symmetric_ && size == 40 && seqno == 0 && 
				 !strcmp (name, "ack")) { 
				// effectively a SYN/ACK
				action = 2;
			}
			else if (size == 40 && seqno == 0 
				 && !strcmp(name, "tcp")) { 
				// effectively a SYN
				action = 1;
			} 
			else {
				action = 0;
			}
		}
		else {
			if (symmetric_ && 
			    (tcph->flags() & TH_ACK) == TH_ACK &&
			    (tcph->flags() & TH_SYN) == TH_SYN) {
				/*
				 * this is a SYN/ACK, find flow for dst -> src
				 */
				action = 2;
			}
			else if ((tcph->flags() & TH_SYN) == TH_SYN) {
				/* 
				 * new flow that's not in table
				 */
				action = 1;
			}
			else {
				action = 0;
			}
		}

		if (action == 0) {
			/*
			 * no rule for this flow
			 */
			forward_packet(p);
			return;
		} else if (action == 1) {
			/*
			 * this is a new flow
			 */
			DelayBoxPair rule_pair (src, dst);
			map<DelayBoxPair, DelayBoxRule*>::iterator rule 
				= rules_.find(rule_pair);
			if (rule == rules_.end()) {
				// no rule for src/dst
				DelayBoxPair rev_pair (dst, src);
				rule = rules_.find(rev_pair);
				if (!symmetric_ || rule == rules_.end()) {
					// no rule for dst/src
					forward_packet(p);
					return;
				}
			}

			// sample rules for flow values
			if (rule->second->delay_ != NULL) {
				// to s
				delay = rule->second->delay_->value() / 1000.0;
			}
			if (rule->second->loss_ != NULL) {
				loss = rule->second->loss_->value();
			}
			if (rule->second->linkspd_ != NULL) {
				linkspd = rule->second->linkspd_->value() *
					MbPS2BPS_FACTOR;
			}

			// create new queue and timer
			DelayBoxQueue* q = new DelayBoxQueue();
			DelayBoxTimer* timer = new DelayBoxTimer(this, src, 
								 dst, fid);
			
			// create new flow table entry
			flow = new DelayBoxFlow(delay, loss, linkspd, q, timer);

			// add to flow table (creates a copy)
			flows_[pair] = flow;
			
			// output to file, if required		
			if (rttfp_ != NULL) {
				char str[50] = "";
				pair.format_short(str);
				fprintf (rttfp_, "%s", str);
				flow->format_delay(str);
				fprintf (rttfp_, " %s ms\n", str);
			}
		}
		else if (action == 2) {
			/*
			 * find the other end of this flow
			 */
			DelayBoxPair revpair = DelayBoxPair (dst, src, fid);
			map<DelayBoxPair, DelayBoxFlow*>::iterator flow_iter = 
				flows_.find(revpair);
			if (flow_iter == flows_.end()) {
				// no flow has been set up
				forward_packet(p);
				return;
			}

			// add this direction to the flow table

			// create new queue and timer
			DelayBoxQueue* q = new DelayBoxQueue();
			DelayBoxTimer* timer = new DelayBoxTimer(this, src, 
								 dst, fid);
			
			// create new flow table entry
			flow = new DelayBoxFlow(flow_iter->second->delay_, 
						flow_iter->second->loss_, 
						flow_iter->second->linkspd_, q, 
						timer);

			// add to flow table (creates a copy)
			flows_[pair] = flow;
			
			// output to file, if required		
			if (rttfp_ != NULL) {
				char str[50] = "";
				pair.format_short(str);
				fprintf (rttfp_, "%s", str);
				flow->format_delay(str);
				fprintf (rttfp_, " %s ms\n", str);
			}
		}
	}
	else {
		/*
		 * flow found in the table
		 */
		flow = flow_iter->second;
	}

	delay = flow->delay_;
	double loss_rate = flow->loss_;
	double link_speed = flow->linkspd_;

	// should this packet be dropped?
	double num = Random::uniform (0.0,1.0);

	double time = now();

	if (loss_rate > 0 && num <= loss_rate) {
		// num is between 0 and loss_rate_, so drop this packet
		if (debug_ > 0) {
			char str[50] = "";
			packet_string (str, tcph, iph, ch->size());
			fprintf (stderr, "Class %s>  %s DROPPED at %f\n", 
				 name(), str, time);
		}
		Packet::free(p);
		return;
	}

	// enqueue p
	double time_to_send = flow->queue_->add(p, time + delay, 
						  link_speed);
	
	if (debug_ > 1) {
		char str[50] = "";
		packet_string (str, tcph, iph, ch->size());
		fprintf (stderr, "  Class %s> %s -> Q at %f\n", name(), str, 
			 time);	
		flow->queue_->dumplist();
	}

	// set timer for next time (time_to_recv)
	if (flow->queue_->oneitem()) {
		time = now();
		flow->timer_->sched(time_to_send - time);
		if (debug_ > 1) {
		fprintf (stderr, "     set sched for %fs\n",
			 time_to_send - time);
		}
	}
}

void DelayBoxClassifier::forward_packet (Packet *p)
{
	// pass this packet on to the default classifier
	NsObject* node = find(p);
	if (node == NULL) {
		Packet::free(p);
		return;
	}
	node->recv(p);
}

void DelayBoxClassifier::timeout(int src, int dst, int fid)
{
	double delta;
	char str[50];
	DelayBoxPair pair (src, dst, fid);	

	// look for this flow in the table
	map<DelayBoxPair, DelayBoxFlow*>::iterator flow_iter = 
		flows_.find(pair);
	if (flow_iter == flows_.end()) {
		pair.format(str);
		fprintf (stderr, "Received timeout for flow %s", str);
		fprintf (stderr, " - not in the flow table\n");
		return;
	}
	
	DelayBoxFlow* flow = flow_iter->second;
	Packet *p = flow->queue_->dequeue (&delta);
	if (p == NULL) {
		fprintf (stderr, "nothing to recv...\n");
		return;
	}

	hdr_tcp *tcph = hdr_tcp::access(p);

	if (debug_ > 1) {
		hdr_ip *iph = hdr_ip::access(p);
		hdr_cmn *ch = hdr_cmn::access(p); 
		char tmp_str[50] = "";
		packet_string (tmp_str, tcph, iph, ch->size());
		fprintf (stderr, "  Class %s> %s <- Q at %f\n", name(), 
			 tmp_str, now());
		flow->queue_->dumplist();
	}

	if ((tcph->flags() & TH_FIN) == TH_FIN) {
		if (debug_ > 1) {
			char pairstr[50];
			pair.format_short(pairstr);
			fprintf (stderr, "  Class %s> deleting flow %s\n",
				 name(), pairstr);
		}
		
		// remove this flow from the flow table
		delete flow_iter->second->queue_;
		flow_iter->second->timer_->force_cancel();
		delete flow_iter->second->timer_;
		delete flow_iter->second;
		flows_.erase (flow_iter);

		/* 
		 * This is not needed.  The other side will
		 * send a FIN and it's entry will then
		 * be deleted from the table.
		 * Besides, there was an error that the
		 * FIN wasn't being forwarded in some cases 
		 */

		// find the other side of the flow and remove
		/*
		DelayBoxPair other_side (dst, src, fid);
		map<DelayBoxPair, DelayBoxFlow*>::iterator flow_iter_tmp = 
			flows_.find(other_side);

		if (flow_iter_tmp != flows_.end()) {
			delete flow_iter_tmp->second->queue_;
			flow_iter_tmp->second->timer_->force_cancel();
			delete flow_iter_tmp->second->timer_;
			delete flow_iter_tmp->second;
			flows_.erase (flow_iter_tmp);
		}
		*/
		forward_packet(p);
		return;
	}

	if (!flow->queue_->empty()) {
		if (debug_ > 1) {
			fprintf (stderr, "    setting sched for %fs\n", 
				 delta);
		}
		flow->timer_->resched(delta);
	}

	forward_packet(p);
}

/*::::::::::::::::: DELAYBOX TIMER :::::::::::::::::::::::::::::::::::::*/

void DelayBoxTimer::expire(Event *) 
{
	a_->timeout(src_, dst_, fid_);
}

/*::::::::::::::::: DELAYBOX QUEUE :::::::::::::::::::::::::::::::::::::*/

DelayBoxQueue::~DelayBoxQueue()
{
	clear();
}

void DelayBoxQueue::clear()
{
	pktinfo* p = head_;
	while (head_) {
		p = head_;
		head_= head_->next_;
		if (p->pkt_ != NULL)
			Packet::free(p->pkt_);
		delete p;
	}
	deltasum_ = 0;
	head_ = tail_ = NULL;
	return;
}

double DelayBoxQueue::add(Packet* pkt, double xfer_time, double link_speed)
	/*
         * pkt       - packet to be added to tail of queue
         * xfer_time - time to transfer this packet with no queuing
         *
         * p->delta_ - time between sending the previous packet and 
	 *             this packet
         *
         * deltasum_ - sum of all of deltas in the queue
         *           - the sending time of the first bit of the 
	 *             previous packet
         *
         * All packets in this queue are from the same connection, but
         * due to variable packet sizes, the processing delay won't
         * be the same for all packets.  This means that shorter packets
         * may have to queue behind larger packets before being transferred.
         *
         * We want the transfer time to reflect the time the first bit
         * gets transferred.  So, p->delta_ may include the transfer time
         * of the previous packet.
         *
         * time packet will be xfer'd = deltasum_ + p->delta_
         *
         */
{
	double process_delay = 0;
	int size = 0;

	// create new queue element 
	pktinfo* p = new pktinfo;
	p->next_ = NULL;
	p->pkt_ = pkt;

	// find the processing delay of the previous packet (tail of queue)
	if (tail_ != NULL) {
		hdr_cmn* ch = hdr_cmn::access(tail_->pkt_);
		size = ch->size();
		if (link_speed > 0)
			process_delay = size / link_speed;
	}

	// calculate when this packet should be transferred
	if (xfer_time < (deltasum_ + process_delay)) {
		/* 
		 * This packet has to queue behind previous packets
		 * and will be transferred as soon as last bit of the 
		 * previous packet has been transferred.
		 */
		p->delta_ = process_delay;
	}
	else { 
		/* 
		 * This packet won't be ready to be transferred until after
		 * the last bit of the previous packet has been transferred.
		 * It can be transferred as soon as it's ready.
		 */
		p->delta_ = xfer_time - deltasum_;
	}

	/*
	 * deltasum_ is now the time that the first bit of this packet
	 * will be sent
	 */
	deltasum_ += p->delta_;

	// insert at end of queue
	if (head_ == NULL) {
		head_ = p;
	} 
	else {
		tail_->next_ = p;
	}
	tail_ = p;

	return (p->delta_);
}


/*
 * dequeue -- remove and return head of the queue
 */
Packet* DelayBoxQueue::dequeue(double* resched_time)
{
	double head_delta;

	if (head_ == NULL)
		return NULL;

	pktinfo *ptr = head_;
	Packet* p = ptr->pkt_;

	// save delta value
	head_delta = ptr->delta_;

	// advance the head pointer
	head_ = ptr->next_;        
	ptr->next_ = NULL;
	delete ptr;

	if (head_ == NULL) {
		*resched_time = 0;
		deltasum_ = 0;
		tail_ = NULL;
	}
	else {
		// new head of the queue's delta should be its time to send
		// which is head_->delta + new_head_->delta
		*resched_time = head_->delta_;
		head_->delta_ += head_delta;   // fix the head's delta
	}
	return (p);
}

/*
 * dumplist -- print out list (for debugging)
 */
void DelayBoxQueue::dumplist()
{
	register pktinfo* p = head_;
	Packet *pkt;
	hdr_tcp *tcph;
	hdr_cmn *ch;
	hdr_ip *iph;
	char str[50] = "";

	if (head_ == NULL) {
		fprintf(stderr, "    head_ is NULL\n");
		return;
	}

	while (p != NULL) {
		pkt = p->pkt_;
		tcph = hdr_tcp::access(pkt);
		ch = hdr_cmn::access(pkt);
		iph = hdr_ip::access(pkt);
		packet_string (str, tcph, iph, ch->size());
		fprintf (stderr, "\t%s at %f\n", str, p->delta_);
		p = p->next_;
	}
}

/*
 * Format packet info for output
 */
void packet_string (char* str, hdr_tcp *tcph, hdr_ip* iph, int size)
{
	int datalen = size - tcph->hlen();     // remove header size

	sprintf (str, "(%d > %d: %d)", iph->src_.addr_, iph->dst_.addr_, 
		 iph->flowid());

	/*
	 * In FullTcp, everything is an ACK except 1st SYN
	 */ 
	if ((tcph->flags() & TH_SYN) && (tcph->flags() & TH_ACK)) {
		sprintf (str, "%s SYN #%u (%d) ACK #%u", str, tcph->seqno(), 
			 datalen, tcph->ackno());
	}
	else if (tcph->flags() & TH_SYN) {
		sprintf (str, "%s SYN #%u (%d)", str, tcph->seqno(), datalen);
	}
	else if (tcph->flags() & TH_FIN) {
		sprintf (str, "%s FIN #%u (%d) ACK #%u", str, tcph->seqno(), 
			 datalen, tcph->ackno());
	}
	else if (datalen == 0) {
		// "pure ACK"
		sprintf (str, "%s ACK #%u (%d)", str, tcph->ackno(), datalen);
	}
	else {
		sprintf (str, "%s DATA #%u (%d) ACK #%u", str, tcph->seqno(), 
			 datalen, tcph->ackno());
	}
}
