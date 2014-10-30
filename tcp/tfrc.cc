/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1999  International Computer Science Institute
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
 *	This product includes software developed by ACIRI, the AT&T 
 *      Center for Internet Research at ICSI (the International Computer
 *      Science Institute).
 * 4. Neither the name of ACIRI nor of ICSI may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICSI AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ICSI OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdlib.h>
#include <sys/types.h>
#include <math.h>
 
#include "tfrc.h"
#include "formula.h"
#include "flags.h"

int hdr_tfrc::offset_;
int hdr_tfrc_ack::offset_;

static class TFRCHeaderClass : public PacketHeaderClass {
public:
	TFRCHeaderClass() : PacketHeaderClass("PacketHeader/TFRC",
					      sizeof(hdr_tfrc)) {
		bind_offset(&hdr_tfrc::offset_);
	}
} class_tfrchdr;

static class TFRC_ACKHeaderClass : public PacketHeaderClass {
public:
	TFRC_ACKHeaderClass() : PacketHeaderClass("PacketHeader/TFRC_ACK",
						  sizeof(hdr_tfrc_ack)) {
		bind_offset(&hdr_tfrc_ack::offset_);
	}
} class_tfrc_ackhdr;

static class TfrcClass : public TclClass {
public:
  	TfrcClass() : TclClass("Agent/TFRC") {}
  	TclObject* create(int, const char*const*) {
    		return (new TfrcAgent());
  	}
} class_tfrc;


TfrcAgent::TfrcAgent() : Agent(PT_TFRC), send_timer_(this), 
	 NoFeedbacktimer_(this), rate_(0), oldrate_(0), maxrate_(0)
{
	bind("packetSize_", &size_);
	bind("rate_", &rate_);
	bind("df_", &df_);
	bind("tcp_tick_", &tcp_tick_);
	bind("ndatapack_", &ndatapack_);
	bind("ndatabytes_", &ndatabytes_);
	bind("true_loss_rate_", &true_loss_rate_);
	bind("srtt_init_", &srtt_init_);
	bind("rttvar_init_", &rttvar_init_);
	bind("rtxcur_init_", &rtxcur_init_);
	bind("rttvar_exp_", &rttvar_exp_);
	bind("T_SRTT_BITS", &T_SRTT_BITS);
	bind("T_RTTVAR_BITS", &T_RTTVAR_BITS);
	bind("InitRate_", &InitRate_);
	bind("overhead_", &overhead_);
	bind("ssmult_", &ssmult_);
	bind("bval_", &bval_);
	bind("ca_", &ca_);
	bind_bool("printStatus_", &printStatus_);
	bind_bool("conservative_", &conservative_);
	bind_bool("ecn_", &ecn_);
	bind("minrto_", &minrto_);
	bind("maxHeavyRounds_", &maxHeavyRounds_);
	bind("SndrType_", &SndrType_); 
	bind("scmult_", &scmult_);
	bind_bool("oldCode_", &oldCode_);
	bind("rate_init_", &rate_init_);
//	bind("standard_", &standard_);
	bind("rate_init_option_", &rate_init_option_);
	bind_bool("slow_increase_", &slow_increase_); 
	bind_bool("voip_", &voip_);
	bind("voip_max_pkt_rate_", &voip_max_pkt_rate_);
	bind("fsize_", &fsize_);
	bind_bool("useHeaders_", &useHeaders_);
	bind_bool("idleFix_", &idleFix_);
	bind("headersize_", &headersize_);
	seqno_ = -1;
	maxseq_ = 0;
	datalimited_ = 0;
	lastlimited_ = 0.0;
	last_pkt_time_ = 0.0;
	bind("maxqueue_", &maxqueue_);
	maxqueue_ = MAXSEQ;
}

/*
 * Must convert bytes into packets. 
 * If nbytes == -1, this corresponds to infinite send.  We approximate
 * infinite by a very large number (MAXSEQ).
 * For simplicity, when bytes are converted to packets, fractional packets 
 * are always rounded up.  
 */
void TfrcAgent::sendmsg(int nbytes, const char* /*flags*/)
{
        if (nbytes == -1 && maxseq_ < MAXSEQ)
		advanceby(MAXSEQ - maxseq_);
        else if (size_ > 0) {
		int npkts = int(nbytes/size_);
		npkts += (nbytes%size_ ? 1 : 0);
		// maxqueue was added by Tom Phelan, to control the
		//   transmit queue from the application.
		if ((maxseq_ - seqno_) < maxqueue_) {
			advanceby(npkts);
		}
	}
}


void TfrcAgent::advanceby(int delta)
{
  	maxseq_ += delta;
	
	if (seqno_ == -1) {
		// if no packets hve been sent so far, call start. 
  		start(); 
	} else if (datalimited_ && maxseq_ > seqno_) {
		// We were data-limited - send a packet now!
		// The old code always waited for a timer to expire!!
		datalimited_ = 0;
		lastlimited_ = Scheduler::instance().clock();
		all_idle_ = 0;
		if (!oldCode_) {
			sendpkt();
		}
	}
} 

int TfrcAgent::command(int argc, const char*const* argv)
{
	if (argc==2) {
		// are we an infinite sender?
		if ( (strcmp(argv[1],"start")==0) && (SndrType_ == 0)) {
			start();
			return TCL_OK;
		}
		if (strcmp(argv[1],"stop")==0) {
			stop();
			return TCL_OK;
		}
	}
  	if ((argc == 3) && (SndrType_ == 1)) {
		// or do we need an FTP type app? 
    	if (strcmp(argv[1], "advance") == 0) {
            	int newseq = atoi(argv[2]);
		// THIS ISN"T USED.
		// newseq: new sequence
		// seqno_: next sequence to be sent
		// maxseq_: max seq_  produced by app so far.
            	if (newseq > maxseq_)
                 	advanceby(newseq - maxseq_);
            	return (TCL_OK);
    	}
    	if (strcmp(argv[1], "advanceby") == 0) {
      		advanceby(atoi(argv[2]));
      		return (TCL_OK);
    		}
	}
	return (Agent::command(argc, argv));
}

void TfrcAgent::start()
{
	seqno_=0;				
	rate_ = InitRate_;
	delta_ = 0;
	oldrate_ = rate_;  
	rate_change_ = SLOW_START;
	UrgentFlag = 1;
	rtt_=0;	 
	sqrtrtt_=1;
	rttcur_=1;
	tzero_ = 0;
	last_change_=0;
	maxrate_ = 0; 
	ss_maxrate_ = 0;
	ndatapack_=0;
	ndatabytes_ = 0;
	true_loss_rate_ = 0;
	active_ = 1; 
	round_id = 0;
	heavyrounds_ = 0;
	t_srtt_ = int(srtt_init_/tcp_tick_) << T_SRTT_BITS;
	t_rttvar_ = int(rttvar_init_/tcp_tick_) << T_RTTVAR_BITS;
	t_rtxcur_ = rtxcur_init_;
	rcvrate = 0 ;
	all_idle_ = 0;

	first_pkt_rcvd = 0 ;
	// send the first packet
	sendpkt();
	// ... at initial rate
	send_timer_.resched(size_/rate_);
	// ... and start timer so we can cut rate 
	// in half if we do not get feedback
	NoFeedbacktimer_.resched(2*size_/rate_); 
}

void TfrcAgent::stop()
{
	active_ = 0;
	if (idleFix_) 
 		datalimited_ = 1;
	send_timer_.force_cancel();
}

void TfrcAgent::nextpkt()
{
	double next = -1;
	double xrate = -1; 

	if (SndrType_ == 0) {
		sendpkt();
	}
	else {
		if (maxseq_ > seqno_) {
			sendpkt();
		} else
			datalimited_ = 1;
			if (debug_) {
			 	double now = Scheduler::instance().clock();
				printf("Time: %5.2f Datalimited now.\n", now);
			}
	}
	
	// If slow_increase_ is set, then during slow start, we increase rate
	// slowly - by amount delta per packet 
	if (slow_increase_ && round_id > 2 && (rate_change_ == SLOW_START) 
		       && (oldrate_+SMALLFLOAT< rate_)) {
		oldrate_ = oldrate_ + delta_;
		xrate = oldrate_;
	} else {
		if (ca_) {
			if (debug_) printf("SQRT: now: %5.2f factor: %5.2f\n", Scheduler::instance().clock(), sqrtrtt_/sqrt(rttcur_));
			xrate = rate_ * sqrtrtt_/sqrt(rttcur_);
		} else
			xrate = rate_;
	}
	if (xrate > SMALLFLOAT) {
		next = size_/xrate;
		if (voip_) {
	  	    	double min_interval = 1.0/voip_max_pkt_rate_;
		    	if (next < min_interval)
				next = min_interval;
		}
		//
		// randomize between next*(1 +/- woverhead_) 
		//
		next = next*(2*overhead_*Random::uniform()-overhead_+1);
		if (next > SMALLFLOAT)
			send_timer_.resched(next);
                else 
			send_timer_.resched(SMALLFLOAT);
	}
}

void TfrcAgent::update_rtt (double tao, double now) 
{
	/* the TCP update */
	t_rtt_ = int((now-tao) /tcp_tick_ + 0.5);
	if (t_rtt_==0) t_rtt_=1;
	if (t_srtt_ != 0) {
		register short rtt_delta;
		rtt_delta = t_rtt_ - (t_srtt_ >> T_SRTT_BITS);    
		if ((t_srtt_ += rtt_delta) <= 0)    
			t_srtt_ = 1;
		if (rtt_delta < 0)
			rtt_delta = -rtt_delta;
	  	rtt_delta -= (t_rttvar_ >> T_RTTVAR_BITS);
	  	if ((t_rttvar_ += rtt_delta) <= 0)  
			t_rttvar_ = 1;
	} else {
		t_srtt_ = t_rtt_ << T_SRTT_BITS;		
		t_rttvar_ = t_rtt_ << (T_RTTVAR_BITS-1);	
	}
	t_rtxcur_ = (((t_rttvar_ << (rttvar_exp_ + (T_SRTT_BITS - T_RTTVAR_BITS))) + t_srtt_)  >> T_SRTT_BITS ) * tcp_tick_;
	tzero_=t_rtxcur_;
 	if (tzero_ < minrto_) 
  		tzero_ = minrto_;

	/* fine grained RTT estimate for use in the equation */
	if (rtt_ > 0) {
		rtt_ = df_*rtt_ + ((1-df_)*(now - tao));
		sqrtrtt_ = df_*sqrtrtt_ + ((1-df_)*sqrt(now - tao));
	} else {
		rtt_ = now - tao;
		sqrtrtt_ = sqrt(now - tao);
	}
	rttcur_ = now - tao;
}

/*
 * Receive a status report from the receiver.
 */
void TfrcAgent::recv(Packet *pkt, Handler *)
{
	double now = Scheduler::instance().clock();
	hdr_tfrc_ack *nck = hdr_tfrc_ack::access(pkt);
	//double ts = nck->timestamp_echo;
	double ts = nck->timestamp_echo + nck->timestamp_offset;
	double rate_since_last_report = nck->rate_since_last_report;
	// double NumFeedback_ = nck->NumFeedback_;
	double flost = nck->flost; 
	int losses = nck->losses;
	true_loss_rate_ = nck->true_loss;

	round_id ++ ;
	UrgentFlag = 0;

	if (round_id > 1 && rate_since_last_report > 0) {
		/* compute the max rate for slow-start as two times rcv rate */ 
		ss_maxrate_ = 2*rate_since_last_report*size_;
		if (conservative_) { 
			if (losses >= 1) {
				/* there was a loss in the most recent RTT */
				if (debug_) printf("time: %5.2f losses: %d rate %5.2f\n", 
				  now, losses, rate_since_last_report);
				maxrate_ = rate_since_last_report*size_;
			} else { 
				/* there was no loss in the most recent RTT */
				maxrate_ = scmult_*rate_since_last_report*size_;
			}
			if (debug_) printf("time: %5.2f losses: %d rate %5.2f maxrate: %5.2f\n", now, losses, rate_since_last_report, maxrate_);
		} else 
			maxrate_ = 2*rate_since_last_report*size_;
	} else {
		ss_maxrate_ = 0;
		maxrate_ = 0; 
	}
		
	/* update the round trip time */
	update_rtt (ts, now);

	/* .. and estimate of fair rate */
	if (voip_ != 1) {
		// From RFC 3714:
		// The voip flow gets to send at the same rate as
		//  a TCP flow with 1460-byte packets.
		fsize_ = size_;
	}
	rcvrate = p_to_b(flost, rtt_, tzero_, fsize_, bval_);
	// rcvrate is in bytes per second, based on fairness with a    
	// TCP connection with the same packet size size_.	      
	if (voip_) {
		// Subtract the bandwidth used by headers.
		double temp = rcvrate*(size_/(1.0*headersize_+size_));
		rcvrate = temp;
	}

	/* if we get no more feedback for some time, cut rate in half */
	double t = 2*rtt_ ; 
	if (t < 2*size_/rate_) 
		t = 2*size_/rate_ ; 
	NoFeedbacktimer_.resched(t);
	
	/* if we are in slow start and we just saw a loss */
	/* then come out of slow start */
	if (first_pkt_rcvd == 0) {
		first_pkt_rcvd = 1 ; 
		slowstart();
		nextpkt();
	}
	else {
		if (rate_change_ == SLOW_START) {
			if (flost > 0) {
				rate_change_ = OUT_OF_SLOW_START;
				oldrate_ = rate_ = rcvrate;
			}
			else {
				slowstart();
				nextpkt();
			}
		}
		else {
			if (rcvrate>rate_) 
				increase_rate(flost);
			else 
				decrease_rate ();		
		}
	}
	if (printStatus_) {
		printf("time: %5.2f rate: %5.2f\n", now, rate_);
		double packetrate = rate_ * rtt_ / size_;
		printf("time: %5.2f packetrate: %5.2f\n", now, packetrate);
		double maxrate = maxrate_ * rtt_ / size_;
		printf("time: %5.2f maxrate: %5.2f\n", now, maxrate);
	}
	Packet::free(pkt);
}

/*
 * Calculate initial sending rate from RFC 3390.
 */
double TfrcAgent::rfc3390(int )
{
        if (size_ <= 1095) {
                return (4.0);
        } else if (size_ < 2190) {
                return (3.0);
        } else {
                return (2.0);
        }
}

/*
 * Used in setting the initial rate.
 * This is from TcpAgent::initial_wnd().
 */
double TfrcAgent::initial_rate()
{
        if (rate_init_option_ == 1) {
		// init_option = 1: static initial rate of rate_init_
                return (rate_init_);
        }
        else if (rate_init_option_ == 2) {
                // do initial rate according to RFC 3390.
		return (rfc3390(size_));
        }
        // XXX what should we return here???
        fprintf(stderr, "Wrong number of rate_init_option_ %d\n",
                rate_init_option_);
        abort();
        return (2.0); // XXX make msvc happy.
}


// ss_maxrate_ = 2*rate_since_last_report*size_;
// rate_: the rate set from the last pass through slowstart()
void TfrcAgent::slowstart () 
{
	double now = Scheduler::instance().clock(); 
	double initrate = initial_rate()*size_/rtt_;
	// If slow_increase_ is set to true, delta is used so that 
	//  the rate increases slowly to new value over an RTT. 
	if (debug_) printf("SlowStart: round_id: %d rate: %5.2f ss_maxrate_: %5.2f\n", round_id, rate_, ss_maxrate_);
	if (round_id <=1 || (round_id == 2 && initial_rate() > 1)) {
		// We don't have a good rate report yet, so keep to  
		//   the initial rate.				     
		oldrate_ = rate_;
		if (rate_ < initrate) rate_ = initrate;
		delta_ = (rate_ - oldrate_)/(rate_*rtt_/size_);
		last_change_=now;
	} else if (ss_maxrate_ > 0) {
		if (idleFix_ && (datalimited_ || lastlimited_ > now - 1.5*rtt_)
			     && ss_maxrate_ < initrate) {
			// Datalimited recently, and maxrate is small.
			// Don't be limited by maxrate to less that initrate.
			oldrate_ = rate_;
			if (rate_ < initrate) rate_ = initrate;
			delta_ = (rate_ - oldrate_)/(rate_*rtt_/size_);
			last_change_=now;
		} else if (rate_ < ss_maxrate_ && 
		                    now - last_change_ > rtt_) {
			// Not limited by maxrate, and time to increase.
			// Multiply the rate by ssmult_, if maxrate allows.
			oldrate_ = rate_;
			if (ssmult_*rate_ > ss_maxrate_) 
				rate_ = ss_maxrate_;
			else rate_ = ssmult_*rate_;
			if (rate_ < size_/rtt_) 
				rate_ = size_/rtt_; 
			delta_ = (rate_ - oldrate_)/(rate_*rtt_/size_);
			last_change_=now;
		} else if (rate_ > ss_maxrate_) {
			// Limited by maxrate.  
			rate_ = oldrate_ = ss_maxrate_/2.0;
			delta_ = 0;
			last_change_=now;
		} 
	} else {
		// If we get here, ss_maxrate <= 0, so the receive rate is 0.
		// We should go back to a very small sending rate!!!
		oldrate_ = rate_;
		rate_ = size_/rtt_; 
		delta_ = 0;
        	last_change_=now;
	}
	if (debug_) printf("SlowStart: now: %5.2f rate: %5.2f delta: %5.2f\n", now, rate_, delta_);
	if (printStatus_) {
		double rate = rate_ * rtt_ / size_;
	  	printf("SlowStart: now: %5.2f rate: %5.2f ss_maxrate: %5.2f delta: %5.2f\n", now, rate, ss_maxrate_, delta_);
	}
}

void TfrcAgent::increase_rate (double )
{               
        double now = Scheduler::instance().clock();
        double maximumrate;

	double mult = (now-last_change_)/rtt_ ;
	if (mult > 2) mult = 2 ;

	rate_ = rate_ + (size_/rtt_)*mult ;
	if (datalimited_ || lastlimited_ > now - 1.5*rtt_) {
		// Modified by Sally on 3/10/2006
		// If the sender has been datalimited, rate should be
		//   at least the initial rate, when increasing rate.
		double init_rate = initial_rate()*size_/rtt_;
	   	maximumrate = (maxrate_>init_rate)?maxrate_:init_rate ;
        } else {
	   	maximumrate = (maxrate_>size_/rtt_)?maxrate_:size_/rtt_ ;
	}
	maximumrate = (maximumrate>rcvrate)?rcvrate:maximumrate;
	rate_ = (rate_ > maximumrate)?maximumrate:rate_ ;
	
        rate_change_ = CONG_AVOID;  
        last_change_ = now;
	heavyrounds_ = 0;
	if (debug_) printf("Increase: now: %5.2f rate: %5.2f lastlimited: %5.2f rtt: %5.2f\n", now, rate_, lastlimited_, rtt_);
	if (printStatus_) {
		double rate = rate_ * rtt_ / size_;
		printf("Increase: now: %5.2f rate: %5.2f lastlimited: %5.2f rtt: %5.2f\n", now, rate, lastlimited_, rtt_);
	}
}       

void TfrcAgent::decrease_rate () 
{
	double now = Scheduler::instance().clock(); 
	rate_ = rcvrate;
	double maximumrate = (maxrate_>size_/rtt_)?maxrate_:size_/rtt_ ;

	// Allow sending rate to be greater than maximumrate
	//   (which is by default twice the receiving rate)
	//   for at most maxHeavyRounds_ rounds.
	if (rate_ > maximumrate)
		heavyrounds_++;
	else
		heavyrounds_ = 0;
	if (heavyrounds_ > maxHeavyRounds_) {
		rate_ = (rate_ > maximumrate)?maximumrate:rate_ ;
	}

	rate_change_ = RATE_DECREASE;
	last_change_ = now;
	if (debug_) printf("Decrease: now: %5.2f rate: %5.2f rtt: %5.2f\n", now, rate_, rtt_);
	if (printStatus_) {
		double rate = rate_ * rtt_ / size_;
		printf("Decrease: now: %5.2f rate: %5.2f rtt: %5.2f\n", now, rate, rtt_);
	}
}

void TfrcAgent::sendpkt()
{
	if (active_) {
		double now = Scheduler::instance().clock(); 
		Packet* p = allocpkt();
		hdr_tfrc *tfrch = hdr_tfrc::access(p);
		hdr_flags* hf = hdr_flags::access(p);
		if (ecn_) {
			hf->ect() = 1;  // ECN-capable transport
		}
		tfrch->seqno=seqno_++;
		tfrch->timestamp=Scheduler::instance().clock();
		tfrch->rtt=rtt_;
		tfrch->tzero=tzero_;
		tfrch->rate=rate_;
		tfrch->psize=size_;
		tfrch->fsize=fsize_;
		tfrch->UrgentFlag=UrgentFlag;
		tfrch->round_id=round_id;
		ndatapack_++;
		ndatabytes_ += size_;
		if (useHeaders_ == true) {
			hdr_cmn::access(p)->size() += headersize_;
		}
		last_pkt_time_ = now;
		send(p, 0);
	}
}


/*
 * RFC 3448:
 * "If the sender has been idle since this nofeedback timer was set and
 * X_recv is less than four packets per round-trip time, then X_recv
 * should not be halved in response to the timer expiration.  This
 * ensures that the allowed sending rate is never reduced to less than
 * two packets per round-trip time as a result of an idle period."
 */
 
void TfrcAgent::reduce_rate_on_no_feedback()
{
	double now = Scheduler::instance().clock();
	if (rate_change_ != SLOW_START)
		// "if" statement added by Sally on 9/25/06,
		// so that an idle report doesn't kick us out of
		// slow-start!
		rate_change_ = RATE_DECREASE; 
	if (oldCode_ || (!all_idle_ && !datalimited_)) {
		// if we are not datalimited
		rate_*=0.5;
	} else if ((datalimited_ || all_idle_) && rate_init_option_ == 1) { 
		// all_idle_: the sender has been datalimited since the 
		//    timer was set
		//  Don't reduce rate below rate_init_ * size_/rtt_.
                if (rate_ > 2.0 * rate_init_ * size_/rtt_ ) {
                        rate_*=0.5;
                } 
	} else if ((datalimited_ || all_idle_) && rate_init_option_ == 2) {
                // Don't reduce rate below the RFC3390 rate.
                if (rate_ > 2.0 * rfc3390(size_) * size_/rtt_ ) {
                        rate_*=0.5;
                } else if ( rate_ > rfc3390(size_) * size_/rtt_ ) {
                        rate_ = rfc3390(size_) * size_/rtt_;
                }
        }
	if (debug_) printf("NO FEEDBACK: time: %5.2f rate: %5.2f all_idle: %d\n", now, rate_, all_idle_);
	UrgentFlag = 1;
	round_id ++ ;
	double t = 2*rtt_ ; 
	// Set the nofeedback timer.
	if (t < 2*size_/rate_) 
		t = 2*size_/rate_ ; 
	NoFeedbacktimer_.resched(t);
	if (datalimited_) {
		all_idle_ = 1;
		if (debug_) printf("Time: %5.2f Datalimited now.\n", now);
	}
	nextpkt();
}

void TfrcSendTimer::expire(Event *) {
  	a_->nextpkt();
}

void TfrcNoFeedbackTimer::expire(Event *) {
	a_->reduce_rate_on_no_feedback ();
	// TODO: if the first (SYN) packet was dropped, then don't use
	//   the larger initial rates from RFC 3390:
        // if (highest_ack_ == -1 && wnd_init_option_ == 2)
	//     wnd_init_option_ = 1;

}
