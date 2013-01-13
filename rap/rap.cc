/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * rap.cc
 * Copyright (C) 1997 by the University of Southern California
 * $Id: rap.cc,v 1.14 2005/09/18 23:33:34 tomh Exp $
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

//
// rap.cc 
//      Code for the 'RAP Source' Agent Class
//
// Author: 
//   Mohit Talwar (mohit@catarina.usc.edu)
//
// $Header: /cvsroot/nsnam/ns-2/rap/rap.cc,v 1.14 2005/09/18 23:33:34 tomh Exp $

#include "rap.h"

int hdr_rap::offset_; // static offset of RAP header

static class RapHeaderClass : public PacketHeaderClass 
{
public:
	RapHeaderClass() : PacketHeaderClass("PacketHeader/RAP", 
					     sizeof(hdr_rap)) {
		bind_offset(&hdr_rap::offset_);
	}
} class_raphdr;


static class RapClass : public TclClass 
{
public:
	RapClass() : TclClass("Agent/RAP") {}
	TclObject* create(int, const char*const*) 
		{ return (new RapAgent()); }
} class_rap;


void IpgTimer::expire(Event *)
{ 
	a_->timeout(RAP_IPG_TIMEOUT); 
}

void RttTimer::expire(Event *)
{ 
	a_->timeout(RAP_RTT_TIMEOUT); 
}

//----------------------------------------------------------------------
// EqualSeqno
//      Compare TransHistory Entries on the seqno field.
//
//      "i1", "i2" are the TransHistory entries to be compared.
//----------------------------------------------------------------------
 
int EqualSeqno(void *i1, void *i2)
{ 
	return (((TransHistoryEntry *) i1)->seqno == 
		((TransHistoryEntry *) i2)->seqno);
}

//----------------------------------------------------------------------
// RapAgent::RapAgent
//	Initialize the RAP agent.
//      Bind variables which have to be accessed in both Tcl and C++.
//      Initializes time values.
//----------------------------------------------------------------------

RapAgent::RapAgent() : Agent(PT_RAP_DATA), ipgTimer_(this), rttTimer_(this),
 	seqno_(0), sessionLossCount_(0), curseq_(0), ipg_(2.0), srtt_(2.0), 
	timeout_(2.0), lastRecv_(0), lastMiss_(0), prevRecv_(0), dctr_(0), 
	flags_(0), fixIpg_(0)
{
	bind("packetSize_", &size_);	// Default 512
	bind("seqno_", &seqno_);	// Default 0
	bind("sessionLossCount_", &sessionLossCount_); // Default 0

	bind("ipg_", &ipg_);		// Default 2 seconds
	bind("beta_", &beta_);	// Default 0.5
	bind("alpha_", &alpha_);	// Default 1.0

	bind("srtt_", &srtt_);	// Default 2 seconds
	bind("variance_", &variance_);// Default 0
	bind("delta_", &delta_);	// Default 0.5
	bind("mu_", &mu_);		// Default 1.2
	bind("phi_", &phi_);		// Default 4

	bind("timeout_", &timeout_);	// Default 2 seconds

	bind("overhead_", &overhead_); // Default 0

	bind("useFineGrain_", &useFineGrain_); // Default FALSE
	bind("kfrtt_", &kfrtt_);	// Default 0.9
	bind("kxrtt_", &kxrtt_);	// Default 0.01

	bind("debugEnable_", &debugEnable_);	// Default FALSE

	bind("rap_base_hdr_size_", &rap_base_hdr_size_);

	bind("dpthresh_", &dpthresh_);

	frtt_ = xrtt_ = srtt_;
}

// Cancel all our timers before we quit
RapAgent::~RapAgent()
{
//  	fprintf(stderr, "%g: rap agent %s(%d) stops.\n", 
//  		Scheduler::instance().clock(), name(), addr());
//  	Tcl::instance().eval("[Simulator instance] flush-trace");
	stop();
}

//----------------------------------------------------------------------
// RapAgent::UpdateTimeValues
//      Update the values for srtt_, variance_ and timeout_ based on
//      the "sampleRtt". Use Jacobson/Karl's algorithm.
//      
//	"sampleRtt" is the sample  round trip time obtained from the 
//      current ack packet.
//----------------------------------------------------------------------

void RapAgent::UpdateTimeValues(double sampleRtt)
{ 
	double diff;
	static int initial = TRUE;

	if (initial) {
		frtt_ = xrtt_ = srtt_ = sampleRtt; // First sample, no history
		variance_ = 0;
		initial = FALSE;
	}

	diff = sampleRtt - srtt_;
	srtt_ += delta_ * diff;
      
	diff = (diff < 0) ? diff * -1 : diff; // Take mod
	variance_ += delta_ * (diff - variance_);

	timeout_ = mu_ * srtt_ + phi_ * variance_;

	if (useFineGrain_) {
		frtt_ = (1 - kfrtt_) * frtt_ + kfrtt_ * sampleRtt;
		xrtt_ = (1 - kxrtt_) * xrtt_ + kxrtt_ * sampleRtt;
	}

	double debugSrtt = srtt_;	// $%#& stoopid compiler
	Debug(debugEnable_, logfile_, 
	      "- srtt updated to %f\n", debugSrtt);
}

void RapAgent::start()
{
	if (debugEnable_)
		logfile_ = DebugEnable(this->addr() >> 
				       Address::instance().NodeShift_[1]);
	else
		// Should initialize it regardless of whether it'll be used.
		logfile_ = NULL;
	Debug(debugEnable_, logfile_, "%.3f: RAP start.\n", 
	      Scheduler::instance().clock());

	flags_ = flags_ & ~RF_STOP;
	startTime_ = Scheduler::instance().clock();
	RttTimeout();		// Decreases initial IPG
	IpgTimeout();
}

// Used by a sink to listen to incoming packets
void RapAgent::listen()
{
	if (debugEnable_)
		logfile_ = DebugEnable(this->addr() >> 
				       Address::instance().NodeShift_[1]);
}

void RapAgent::stop()
{
	Debug(debugEnable_, logfile_, 
	      "%.3f: RAP stop.\n", Scheduler::instance().clock());
			
	// Cancel the timer only when there is one
	if (ipgTimer_.status() == TIMER_PENDING)
		ipgTimer_.cancel();  
	if (rttTimer_.status() == TIMER_PENDING)
		rttTimer_.cancel();

	stopTime_ = Scheduler::instance().clock();
	int debugSeqno = seqno_;
	Debug(debugEnable_, logfile_, 
	      "- numPackets %d, totalTime %f\n", 
	      debugSeqno, stopTime_ - startTime_);
	flags_ |= RF_STOP;
}

//----------------------------------------------------------------------
// RapAgent::command
//      Called when a Tcl command for the RAP agent is executed.
//      Two commands are supported
//         $rapsource start
//         $rapsource stop
//----------------------------------------------------------------------
int RapAgent::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "start") == 0) {
			start();
			// return TCL_OK, so the calling function knows that 
			// the command has been processed
			return (TCL_OK);
		} else if (strcmp(argv[1], "stop") == 0) {
			stop();
			return (TCL_OK);
		} else if (strcmp(argv[1], "listen") == 0) {
			listen();
			return (TCL_OK);
		}
	} else if (argc == 3) {
                if (strcmp(argv[1], "advanceby") == 0) {
                        advanceby(atoi(argv[2]));
                        return (TCL_OK);
                }
	}


	// If the command hasn't been processed by RapAgent()::command,
	// call the command() function for the base class
	return (Agent::command(argc, argv));
}

//----------------------------------------------------------------------
// RapAgent::SendPacket
//      Called in IpgTimeout().
//      Create a packet, increase seqno_, send the packet out.
//----------------------------------------------------------------------

void RapAgent::SendPacket(int nbytes, AppData *data)
{
	TransHistoryEntry *pktInfo;
	Packet *pkt;

	type_ = PT_RAP_DATA;
	if (data)
		pkt = allocpkt(data->size()); 
	else 
		pkt = allocpkt();

	// Fill in RAP headers
	hdr_rap* hdr = hdr_rap::access(pkt);
	hdr->seqno() = ++seqno_;	// Start counting from 1;
	hdr->lastRecv = hdr->lastMiss = hdr->prevRecv = 0; // Ignore @ sender
	hdr->flags() = RH_DATA;
	if (data) {
		hdr->size() = data->size();
		pkt->setdata(data);
	} else {
		hdr->size() = size_;
	}
	// XXX Simply set packet size to the given ADU's nominal size. 
	// Make sure that the size is reasonable!!
	hdr_cmn *ch = hdr_cmn::access(pkt);
	ch->size() = nbytes; 

	send(pkt, 0);
	pktInfo = new TransHistoryEntry(seqno_);
	transmissionHistory_.SetInsert((void *) pktInfo, EqualSeqno);
	int debugSeqno = seqno_;
	Debug(debugEnable_, logfile_, 
	      "- packet %d sent\n", debugSeqno);
}

//----------------------------------------------------------------------
// RapAgent::recv
//      Called when a packet is received.
//      Should be of type PT_RAP_ACK.
//----------------------------------------------------------------------

void RapAgent::recv(Packet* pkt, Handler*)
{
	Debug(debugEnable_, logfile_, 
	      "%.3f: RAP packet received.\n", Scheduler::instance().clock());

	hdr_rap* hdr = hdr_rap::access(pkt); // Access RAP header

	switch (hdr->flags()) { 
	case RH_DATA:
		UpdateLastHole(hdr->seqno());
		SendAck(hdr->seqno());
		if ((pkt->datalen() > 0) && app_) 
			// We do have user data, process it
			app_->process_data(pkt->datalen(), pkt->userdata());
		break;
	case RH_ACK:
		RecvAck(hdr);
		break;
	default:
		fprintf(stderr, 
			"RAP agent %s received a packet with unknown flags %x",
			name(), hdr->flags());
		break;
	} 
	Packet::free(pkt);		// Discard the packet
}

//----------------------------------------------------------------------
// RapAgent::RecvAck
//      Called when an Ack is received.
//      
//      "header" is the RAP header of the Ack.
//----------------------------------------------------------------------

void RapAgent::RecvAck(hdr_rap *ackHeader)
{
	double sampleRtt;
	TransHistoryEntry *old, key(ackHeader->seqno_);

	assert(ackHeader->seqno_ > 0);

	Debug(debugEnable_, logfile_, 
	      "- ack %d\n", ackHeader->seqno_);

	old = (TransHistoryEntry *) 
		transmissionHistory_.SetRemove((void *) &key, EqualSeqno);

	if (old != NULL) {
		Debug(debugEnable_, logfile_, 
		      "- found in transmission history\n");
		assert((old->status == RAP_SENT) || (old->status == RAP_INACTIVE));

		// Get sample rtt		
		sampleRtt = key.departureTime - old->departureTime; 

		UpdateTimeValues(sampleRtt);
		
		delete old;
	}
  
	if (!anyack()) {
		flags_ |= RF_ANYACK;
		ipg_ = srtt_;
	}
  
	if (LossDetection(RAP_ACK_BASED, ackHeader))
		LossHandler();

	// XXX We only stop by sequence number when we are in 
	// "counting sequence number" mode.   -- haoboy
	if (counting_pkt() && (ackHeader->seqno_ >= curseq_)) 
		finish();
}

//----------------------------------------------------------------------
// RapAgent::timeout
//      Called when a timer fires.
//
//      "type" is the type of Timeout event
//----------------------------------------------------------------------

void RapAgent::timeout(int type)
{
	if (type == RAP_IPG_TIMEOUT)
		IpgTimeout();
	else if (type == RAP_RTT_TIMEOUT)
		RttTimeout();
	else
		assert(FALSE);
}

//----------------------------------------------------------------------
// RapAgent::IpgTimeout
//      Called when the ipgTimer_ fires.
//----------------------------------------------------------------------

void RapAgent::IpgTimeout()
{
	double waitPeriod;		// Time before next transmission

	Debug(debugEnable_, logfile_, 
	      "%.3f: IPG Timeout.\n", Scheduler::instance().clock());

	if (LossDetection(RAP_TIMER_BASED))
		LossHandler();
	else if (!counting_pkt()) {
		if (app_) {
			int nbytes;
			AppData* data = app_->get_data(nbytes);
			// Missing data in application. What should we do??
			// For now, simply schedule the next SendPacket(). 
			// If the application has nothing to send, it'll stop 
			// the rap agent later on. 
			if (data != NULL) {
				SendPacket(nbytes, data);
				dctr_++;
			}
		} else {
			// If RAP doesn't have application, just go ahead and 
			// send packet
			SendPacket(size_);
			dctr_++;
		}
	} else if (seqno_ < curseq_) {
			SendPacket(size_);
			dctr_++;
	}

	// XXX If we only bound IPG in DecreaseIpg(), the thresholding will 
	// happen immediately because DecreaseIpg() isn't called immediately. 
	// So we do it here. 
	if (fixIpg_ != 0)
		ipg_ = fixIpg_;

	if (useFineGrain_)
		waitPeriod = frtt_ / xrtt_ * ipg_;
	else 
		waitPeriod = ipg_;
	// By this point, we may have been stopped by applications above
	// Thus, do not reschedule a timer if we are stopped. 
	if (!is_stopped())
		ipgTimer_.resched(waitPeriod + Random::uniform(overhead_));
}
  
//----------------------------------------------------------------------
// RapAgent::RttTimeout
//      Called when the rttTimer_ fires.
//      Decrease IPG. Restart rttTimer_.
//----------------------------------------------------------------------

void RapAgent::RttTimeout()
{
	Debug(debugEnable_, logfile_, 
	      "%.3f: RTT Timeout.\n", Scheduler::instance().clock());

	// During the past srtt_, we are supposed to send out srtt_/ipg_
	// packets. If we sent less than that, we may not increase rate
	if (100*dctr_*(ipg_/srtt_) >= dpthresh_)
		DecreaseIpg();		// Additive increase in rate
	else 
		Debug(debugEnable_, logfile_, 
		      "- %f Cannot increase rate due to insufficient data.\n",
		      Scheduler::instance().clock());
	dctr_ = 0;

	double debugIpg = ipg_ + overhead_ / 2;
	Debug(debugEnable_, logfile_, 
	      "- ipg decreased at %.3f to %f\n", 
	      Scheduler::instance().clock(), debugIpg);

	rttTimer_.resched(srtt_);
}

//----------------------------------------------------------------------
// RapAgent::LossDetection
//      Called in ipgTimeout (RAP_TIMER_BASED) 
//          or in RecvAck (RAP_ACK_BASED).
//
// Returns:
//      TRUE if loss detected, FALSE otherwise.
//
//      "ackHeader" is the RAP header of the received Ack (PT_RAP_ACK).
//----------------------------------------------------------------------

static double currentTime;
static hdr_rap *ackHdr;
static RapAgent *rapAgent; 
static int numLosses;

int EqualStatus(void *i1, void *i2)
{ 
	return (((TransHistoryEntry *) i1)->status == 
		((TransHistoryEntry *) i2)->status);
}

void DestroyTransHistoryEntry(long item)
{
	TransHistoryEntry *entry = (TransHistoryEntry *) item;

	Debug(rapAgent->GetDebugFlag(), rapAgent->GetLogfile(),
	      "- purged seq num %d\n", entry->seqno);

	delete entry;
}

void TimerLostPacket(long item)
{
	TransHistoryEntry *entry = (TransHistoryEntry *) item;

	if ((entry->departureTime + rapAgent->GetTimeout()) <= currentTime) {
		// ~ Packets lost in RAP session
		rapAgent->IncrementLossCount(); 

		// Ignore cluster losses
		if (entry->status != RAP_INACTIVE) {
			assert(entry->status == RAP_SENT);

			numLosses++;
			Debug(rapAgent->GetDebugFlag(), rapAgent->GetLogfile(),
			      "- timerlost seq num %d , last sent %d\n", 
			      entry->seqno, rapAgent->GetSeqno());
		}
		entry->status = RAP_PURGED; 
	}
}

void AckLostPacket(long item)
{
	TransHistoryEntry *entry = (TransHistoryEntry *) item;

	int seqno, lastRecv, lastMiss, prevRecv;

	seqno = entry->seqno;
	lastRecv = ackHdr->lastRecv;
	lastMiss = ackHdr->lastMiss;
	prevRecv = ackHdr->prevRecv;

	if (seqno <= lastRecv) {
		if ((seqno > lastMiss) || (seqno == prevRecv))
			entry->status = RAP_PURGED; // Was Received, now purge
		else if ((lastRecv - seqno) >= 3) {
			// ~ Packets lost in RAP session
			rapAgent->IncrementLossCount(); 
			
			if (entry->status != RAP_INACTIVE) {
				assert(entry->status == RAP_SENT);

				numLosses++;
				Debug(rapAgent->GetDebugFlag(), 
				      rapAgent->GetLogfile(),
				      "- acklost seqno %d , last sent %d\n", 
				      seqno, rapAgent->GetSeqno());
			}
			// Was Lost, purge from history
			entry->status = RAP_PURGED; 
		}
	}
}

int RapAgent::LossDetection(RapLossType type, hdr_rap *ackHeader)
{
	TransHistoryEntry key(0, RAP_PURGED);

	currentTime = key.departureTime;
	ackHdr = ackHeader;
	rapAgent = this;
	numLosses = 0;

	switch(type) {
	case RAP_TIMER_BASED:
		transmissionHistory_.Mapcar(TimerLostPacket);
		break;
		
	case RAP_ACK_BASED:
		transmissionHistory_.Mapcar(AckLostPacket);
		break;

	default:
		assert(FALSE);
	}

	Debug(debugEnable_, logfile_, 
	      "- %d losses detected\n", numLosses); 

	Debug(debugEnable_, logfile_, 
	      "- history size %d\n", transmissionHistory_.Size());
  
	transmissionHistory_.Purge((void *) &key, 
				   EqualStatus, // Purge PURGED packets
				   DestroyTransHistoryEntry);

	Debug(debugEnable_, logfile_, 
	      "- history size %d\n", transmissionHistory_.Size());

	if (numLosses)
		return TRUE;
	else
		return FALSE;
}

//----------------------------------------------------------------------
// RapAgent::LossHandler
//      Called when loss detected.
//      Increase IPG. Mark packets INACTIVE. Reschedule rttTimer_.
//----------------------------------------------------------------------

void MarkInactive(long item)
{
	TransHistoryEntry *entry = (TransHistoryEntry *) item;

	entry->status = RAP_INACTIVE;
}

void RapAgent::LossHandler()
{
	IncreaseIpg();		// Multiplicative decrease in rate

	double debugIpg = ipg_ + overhead_ / 2;
	Debug(debugEnable_, logfile_, 
	      "- ipg increased at %.3f to %f\n", 
	      Scheduler::instance().clock(), debugIpg);

	transmissionHistory_.Mapcar(MarkInactive);
	Debug(debugEnable_, logfile_, 
	      "- window full packets marked inactive\n");

	rttTimer_.resched(srtt_);
}

//----------------------------------------------------------------------
// RapAgent::SendAck
//      Create an ack packet, set fields, send the packet out.
//
//      "seqNum" is the sequence number of the packet being acked.
//----------------------------------------------------------------------

void RapAgent::SendAck(int seqNum)
{
	type_ = PT_RAP_ACK;
	Packet* pkt = allocpkt();	// Create a new packet
	hdr_rap* hdr = hdr_rap::access(pkt);   // Access header

	hdr->seqno() = seqNum;
	hdr->flags() = RH_ACK;

	hdr->lastRecv = lastRecv_;
	hdr->lastMiss = lastMiss_;
	hdr->prevRecv = prevRecv_;

	hdr_cmn *ch = hdr_cmn::access(pkt);
	ch->size() = rap_base_hdr_size_;

	send(pkt, 0);
	Debug(debugEnable_, logfile_, 
	      "- ack sent %u [%u %u %u]\n", 
	      seqNum, lastRecv_, lastMiss_, prevRecv_);
}

//----------------------------------------------------------------------
// RapSinkAgent::UpdateLastHole
//      Update the last hole in sequence number space at the receiver.
//      
//	"seqNum" is the sequence number of the data packet received.
//----------------------------------------------------------------------

void RapAgent::UpdateLastHole(int seqNum)
{
	assert(seqNum > 0);

	if (seqNum > (lastRecv_ + 1)) {
		prevRecv_ = lastRecv_;
		lastRecv_ = seqNum;
		lastMiss_ = seqNum - 1;
		return;
	}

	if (seqNum == (lastRecv_ + 1)) {
		lastRecv_ = seqNum;
		return;
	}
	
	if ((lastMiss_ < seqNum) && (seqNum <= lastRecv_)) // Duplicate
		return;

	if (seqNum == lastMiss_) {
		if ((prevRecv_ + 1) == seqNum) // Hole filled
			prevRecv_ = lastMiss_ = 0;
		else
			lastMiss_--;
		
		return;
	}
	
	if ((prevRecv_ < seqNum) && (seqNum < lastMiss_)) {
		prevRecv_ = seqNum;
		return;
	}

	assert(seqNum <= prevRecv_);	// Pretty late...
}


// take pkt count
void RapAgent::advanceby(int delta)
{
	flags_ |= RF_COUNTPKT;
        curseq_ = delta;
	start();
}

void RapAgent::finish()
{
	stop();
	Tcl::instance().evalf("%s done", this->name());
}

