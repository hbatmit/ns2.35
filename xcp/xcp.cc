/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
 *
 * Copyright (C) 2004 by USC/ISI
 *               2002 by Dina Katabi
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation, advertising
 * materials, and other materials related to such distribution and use
 * acknowledge that the software was developed by the University of
 * Southern California, Information Sciences Institute.  The name of the
 * University may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef lint
static const char rcsid[] =
"@(#) $Header: /cvsroot/nsnam/ns-2/xcp/xcp.cc,v 1.11 2006/05/30 20:30:30 pradkin Exp $";
#endif


#include "xcp.h"
#include "red.h"

static unsigned int next_router = 0;

static class XCPClass : public TclClass {
public:
	XCPClass() : TclClass("Queue/XCP") {}
	TclObject* create(int, const char*const*) {
		return (new XCPWrapQ);
	}
} class_xcp_queue;

XCPWrapQ::XCPWrapQ() : xcpq_(0), qToDq_(0),  tcp_xcp_on_(0)
{
	// If needed wrrTemp and queueWeight will be reset to more useful
	// values in XCPWrapQ::setVirtualQueues
	for (int i = 0; i<MAX_QNUM; ++i) {
		q_[i] = 0;
		wrrTemp_[i] = 0.0;
		queueWeight_[i]=  1 / MAX_QNUM;
	}
	
	routerId_ = next_router++;

	// XXX Temporary fix XXX
	// set flag to 1 when supporting both tcp and xcp flows; 
	// temporary fix for allocating link BW between xcp and 
	// tcp queues until dynamic queue weights come into effect. 
	// This fix should then go away.
	
	Tcl& tcl = Tcl::instance();
	tcl.evalf("Queue/XCP set tcp_xcp_on_");
	if (strcmp(tcl.result(), "0") != 0)  
		tcp_xcp_on_ = true;              //tcp_xcp_on flag is set
	else
		tcp_xcp_on_ = false;		// Not strictly required with
						// the initializer above.
}
 
void XCPWrapQ::setVirtualQueues() {
	qToDq_ = 0;
	queueWeight_[XCPQ] = 0.5;
	queueWeight_[TCPQ] = 0.5;
	queueWeight_[OTHERQ] = 0;
  
	// setup timers for xcp queue only
	if (xcpq_) {
		xcpq_->routerId(this, routerId_);
		xcpq_->setupTimers();
	}
}


int XCPWrapQ::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();

	if (argc == 2) {
		// for Dina's parking-lot experiment data
		if (strcmp(argv[1], "queue-read-drops") == 0) {
			if (xcpq_) {
				tcl.resultf("%g", xcpq_->totalDrops());
				return (TCL_OK);
			} else {
				tcl.add_errorf("XCP queue is not set\n");
				return TCL_ERROR;
			}
		} 

	}
	
	if (argc == 3) {
		if (strcmp(argv[1], "set-xcpQ") == 0) {

			q_[XCPQ] = xcpq_ = (XCPQueue *)(TclObject::lookup(argv[2]));
			
			if (xcpq_ == NULL) {
				tcl.add_errorf("Wrong xcp virtual queue %s\n", argv[2]);
				return TCL_ERROR;
			}
			setVirtualQueues();
			return TCL_OK;
		}
		else if (strcmp(argv[1], "set-tcpQ") == 0) {
			
			q_[TCPQ] = (XCPQueue *)(TclObject::lookup(argv[2]));
			
			if (q_[TCPQ] == NULL) {
				tcl.add_errorf("Wrong tcp virtual queue %s\n", argv[2]);
				return TCL_ERROR;
			}
			return TCL_OK;
		}
		else if (strcmp(argv[1], "set-otherQ") == 0) {
			
			q_[OTHERQ] = (XCPQueue *)(TclObject::lookup(argv[2]));
			
			if (q_[OTHERQ] == NULL) {
				tcl.add_errorf("Wrong 'other' virtual queue %s\n", argv[2]);
				return TCL_ERROR;
			}
			return TCL_OK;
		}
		else if (strcmp(argv[1], "set-link-capacity") == 0) {
			double link_capacity_bitps = strtod(argv[2], 0);
			if (link_capacity_bitps < 0.0) {
				printf("Error: BW < 0"); 
				exit(1);
			}
			if (tcp_xcp_on_) // divide between xcp and tcp queues
				link_capacity_bitps = link_capacity_bitps * queueWeight_[XCPQ];
			
			xcpq_->setBW(link_capacity_bitps/8.0); 
			return TCL_OK;
		}
    
		else if (strcmp(argv[1], "drop-target") == 0) {
			drop_ = (NsObject*)TclObject::lookup(argv[2]);
			if (drop_ == 0) {
				tcl.resultf("no object %s", argv[2]);
				return (TCL_ERROR);
			}
			for (int n=0; n < MAX_QNUM; n++)
				if (q_[n])
					q_[n]->setDropTarget(drop_);
			
			return (TCL_OK);
		}

		else if (strcmp(argv[1], "attach") == 0) {
			int mode;
			const char* id = argv[2];
			Tcl_Channel queue_trace_file = Tcl_GetChannel(tcl.interp(), (char*)id, &mode);
			if (queue_trace_file == 0) {
				tcl.resultf("queue.cc: trace-drops: can't attach %s for writing", id);
				return (TCL_ERROR);
			}
			xcpq_->setChannel(queue_trace_file); //XXX virtual queues had
							     //better be attached from
							     //tcl in a similar manner
							     //(this way xcpq_ is
							     //singled out).
			
			return (TCL_OK);
		}
    
		else if (strcmp(argv[1], "queue-sample-everyrtt") == 0) {
			double e_rtt = strtod(argv[2],0);
			//printf(" timer at %f \n",e_rtt);
			xcpq_->setEffectiveRtt(e_rtt);
			return (TCL_OK);
		}
    
		else if (strcmp(argv[1], "num-mice") == 0) {
			int nm = atoi(argv[2]);

			xcpq_->setNumMice(nm);
			return (TCL_OK);
		}
	}
	return (Queue::command(argc, argv));
}


void XCPWrapQ::recv(Packet* p, Handler* h)
{
	mark(p);
	Queue::recv(p, h);
}

void XCPWrapQ::addQueueWeights(int queueNum, int weight) {
	if (queueNum < MAX_QNUM)
		queueWeight_[queueNum] = weight;
	else {
		fprintf(stderr, "Queue number is out of range.\n");
	}
}

int XCPWrapQ::queueToDeque()
{
	int i = 0;
  
	if (wrrTemp_[qToDq_] <= 0) {
		qToDq_ = ((qToDq_ + 1) % MAX_QNUM);
		wrrTemp_[qToDq_] = queueWeight_[qToDq_] - 1;
	} else {
		wrrTemp_[qToDq_] = wrrTemp_[qToDq_] -1;
	}
	while ((i < MAX_QNUM) && (q_[qToDq_]->length() == 0)) {
		wrrTemp_[qToDq_] = 0;
		qToDq_ = ((qToDq_ + 1) % MAX_QNUM);
		wrrTemp_[qToDq_] = queueWeight_[qToDq_] - 1;
		i++;
	}
	return (qToDq_);
}

int XCPWrapQ::queueToEnque(int cp)
{
	int n;
	switch (cp) {
	case CP_XCP:
		return (n = XCPQ);
    
	case CP_TCP:
		return (n = TCPQ);
    
	case CP_OTHER:
		return (n = OTHERQ);

	default:
		fprintf(stderr, "Unknown codepoint %d\n", cp);
		exit(1);
	}
}


// Extracts the code point marking from packet header.
int XCPWrapQ::getCodePt(Packet *p) {

	hdr_ip* iph = hdr_ip::access(p);
	return(iph->prio());
}

Packet* XCPWrapQ::deque()
{
	Packet *p = NULL;
	int n = queueToDeque();
	
// Deque a packet from the underlying queue:
	p = q_[n]->deque();
	return(p);
}


void XCPWrapQ::enque(Packet* pkt)
{
	int codePt;
  
	codePt = getCodePt(pkt);
	int n = queueToEnque(codePt);
  
	q_[n]->enque(pkt);
}


void XCPWrapQ::mark(Packet *p) {
  
	int codePt;
	hdr_cmn* cmnh = hdr_cmn::access(p);
	hdr_xcp *xh = hdr_xcp::access(p);
	hdr_ip *iph = hdr_ip::access(p);
	
	if ((codePt = iph->prio_) > 0)
		return;
	
	else {
		if (xh->xcp_enabled_ != hdr_xcp::XCP_DISABLED)
			codePt = CP_XCP;
		else 
			if (cmnh->ptype() == PT_TCP || cmnh->ptype() == PT_ACK)
				codePt = CP_TCP;
			else // for others
				codePt = CP_OTHER;
    
		iph->prio_ = codePt;
	}
}
