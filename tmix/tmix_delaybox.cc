/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * Copyright 2007, Old Dominion University 
 * Copyright 2007, University of North Carolina at Chapel Hill
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
 *
 * M.C. Weigle, P. Adurthi, F. Hernandez-Campos, K. Jeffay, and F.D. Smith, 
 * Tmix: A Tool for Generating Realistic Application Workloads in ns-2, 
 * ACM Computer Communication Review, July 2006, Vol 36, No 3, pp. 67-76.

 * Contact: Michele Weigle (mweigle@cs.odu.edu)
 * http://www.cs.odu.edu/inets/Tmix
 */

#include <tclcl.h>
#include "tmix_delaybox.h"

/*::::::::::::::::: Tmix_DelayBox NODE :::::::::::::::::::::::::::::::::::::*/

static class Tmix_DelayBoxNodeClass : public TclClass {
public:
        Tmix_DelayBoxNodeClass() : TclClass("Node/Tmix_DelayBox") {};
	TclObject* create(int, const char*const*) {
		return (new Tmix_DelayBoxNode);
	}
} class_Tmix_DelayBoxnode;

Tmix_DelayBoxNode::Tmix_DelayBoxNode() : Node()
{
	classifier_ = NULL;
}

Tmix_DelayBoxNode::~Tmix_DelayBoxNode()
{
	delete classifier_;
}

int Tmix_DelayBoxNode::command(int argc, const char*const* argv) {
	if (argc == 5) {
		if (!strcmp (argv[1], "output-delay")) {
			if (!rttfp_)
				return (TCL_ERROR);
			classifier_->output_delay(atoi(argv[2]), 
						  atoi(argv[3]), 
						  atoi(argv[4]), rttfp_);
			return (TCL_OK);
		}
		else if (!strcmp(argv[1],"set-cvfile")) {
			cvecfp_ = fopen (argv[2], "r");
			if (cvecfp_) {
				classifier_->create_flow_table(argv[3], argv[4], cvecfp_);
                                return (TCL_OK);
			}
                        else
                                return (TCL_ERROR);
                }                
	}
	else if (argc == 3) {
		if (!strcmp (argv[1], "set-debug")) {
			classifier_->set_debug(atoi (argv[2]));
			return (TCL_OK);
		}
		else if (!strcmp (argv[1], "set-delay-file")) {
			rttfp_ = fopen (argv[2], "w");
			if (rttfp_) {
				classifier_->setfp(rttfp_);
				return (TCL_OK);
			}
			else
				return (TCL_ERROR);
		}
	}
	else if(argc == 2) {   
		if (!strcmp (argv[1], "list-flows")) {
			classifier_->list_flows();
			return (TCL_OK);
		}
		else if (!strcmp (argv[1], "create-classifier")) {
			Tcl& tcl = Tcl::instance();
			tcl.evalf ("%s entry", name());
			classifier_ = (Tmix_DelayBoxClassifier*) lookup_obj 
				(tcl.result());
			return (TCL_OK);
		}
		else if (!strcmp (argv[1], "close-delayfile")) {
			if (rttfp_) 
				fclose(rttfp_);
			return (TCL_OK);
		}
		else if (!strcmp (argv[1], "set-lossless")) {
			classifier_->set_lossless();
			return (TCL_OK);
		}
	}
	return (Node::command(argc, argv));
}

/*::::::::::::::::: Tmix_DelayBox CLASSIFIER ::::::::::::::::::::::::::::::::*/

static class Tmix_DelayBoxClassifierClass : public TclClass {
public:
	Tmix_DelayBoxClassifierClass() : TclClass("Classifier/Tmix_DelayBox") {}
	TclObject* create (int, const char*const*) {
		return (new Tmix_DelayBoxClassifier);
	}
} class_Tmix_DelayBoxclassifier;

Tmix_DelayBoxClassifier::~Tmix_DelayBoxClassifier()
{
	/* delete the flow table */
	map<DelayBoxPair, DelayBoxFlow*>::iterator flow;
	for (flow = flows_.begin(); flow != flows_.end(); flow++) {
		delete flow->second->queue_;
		flow->second->timer_->force_cancel();
		delete flow->second->timer_;
		delete flow->second;
		flows_.erase(flow);
	}
}

void Tmix_DelayBoxClassifier::create_flow_table (const char* src, 
						 const char *dst, FILE* fp)
					  
{
    char cvec[100];
    int fid = 0;
    unsigned long us_delay;
    double delay, fwdloss, revloss, linkspd; 
    DelayBoxPair* pair;
    DelayBoxQueue* q;
    DelayBoxTimer* timer;
    DelayBoxFlow* flow;

    linkspd = 0;
    while(!feof(fp)) {
	    memset(cvec,'\0',100);
	    fgets (cvec, 100, fp);
	    if (cvec[0] == '#'){
		    /* skip comments */
		    continue;
	    }
	    if (cvec[0] == 'S' || cvec[0] == 'C') {
		    /* start of new connection */
		    fid++;
		    continue;
	    }
	    if (cvec[0] == 'r') {
		    /* RTT */
		    sscanf (cvec, "%*c %lu\n", &us_delay);
		    delay = (double) us_delay / 1000000;
		    continue;
	    }
	    if (cvec[0] == 'l') {
		    /* lossrate */
		    sscanf (cvec, "%*c %lf %lf\n", &fwdloss, &revloss);

		    /* lossrate is final thing tmix_delaybox needs, so
		       create new flows */
		    pair = new DelayBoxPair(atoi(src), atoi(dst), fid);
		    q = new DelayBoxQueue();
		    timer = new DelayBoxTimer(this, atoi(src), atoi(dst), fid);
		    flow = new DelayBoxFlow(delay/2, fwdloss, linkspd, q,timer);
		    flows_[*pair] = flow;
		    delete pair;

		    pair = new DelayBoxPair(atoi(dst), atoi(src), fid);
		    q = new DelayBoxQueue();
		    timer = new DelayBoxTimer(this, atoi(dst), atoi(src), fid);
		    flow = new DelayBoxFlow(delay/2, revloss, linkspd, q,timer);
		    flows_[*pair] = flow;
		    delete pair;
	    }
    }
    fclose (fp);
}

void Tmix_DelayBoxClassifier::recv (Packet* p, Handler* )
{
	DelayBoxFlow* flow;
	double delay;

	/* for printing debugging statements */
	hdr_tcp *tcph = hdr_tcp::access(p);
	hdr_cmn *ch = hdr_cmn::access(p); 
	hdr_ip *iph = hdr_ip::access(p);

	int src = iph->src_.addr_;
	int dst = iph->dst_.addr_;
	int fid = iph->flowid();

	/* lookup flow in flow table */
        DelayBoxPair pair(src, dst, fid);
	map<DelayBoxPair, DelayBoxFlow*>::iterator flow_iter = flows_.begin();
        flow_iter= flows_.find(pair);
     
	if (flow_iter == flows_.end()) {
		/* flow not found in table */
		if (debug_ > 3) {
			char str[50];
			packet_string (str, tcph, iph, ch->size());
			fprintf (stderr, "FWD at %f> %s - NO DELAY\n",
				 now(), str);
		}
		forward_packet(p); 
		return;
	}
                
	/* flow found in the table */
	flow = flow_iter->second;
         
	delay = flow->delay_;
	double loss_rate = flow->loss_;

	/* determine if this packet be dropped */
	double num = Random::uniform (0.0,1.0);
	double time = now();
           
	if (!lossless_ && loss_rate > 0 && num <= loss_rate) {
		/* num is between 0 and loss_rate_, so drop this packet */
		if (debug_ > 0) {
			char str[50] = "";
			packet_string (str, tcph, iph, ch->size());
			fprintf (stderr, "Class %s>  %s DROPPED at %f\n", 
				 name(), str, time);
		}
		Packet::free(p);
		return;
	}

	/* enqueue p */
	double time_to_send = flow->queue_->add(p, time + delay, 0);
	
	if (debug_ > 4) {
		char str[50] = "";
		packet_string (str, tcph, iph, ch->size());
		fprintf (stderr, "  Class %s/Queue %d> %s -> Q at %f\n", 
			 name(), iph->flowid(), str, time);	
		flow->queue_->dumplist();
	}

	/* set timer for next time (time_to_recv) */
	if (flow->queue_->oneitem()) {  
		/* only 1 element is present in the queue */
		time = now();
                flow->timer_->sched(time_to_send - time); // schedule the timer 
		if (debug_ > 4) {
			fprintf (stderr, "     set sched for %fs\n",
				 time_to_send - time);
		}
	}
}
