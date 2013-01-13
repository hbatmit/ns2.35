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
 *      This product includes software developed by ACIRI, the AT&T
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <math.h>


#include "ip.h"
#include "tcp-rfc793edu.h"
#include "flags.h"

// Original code contributed by Fernando Cela Diaz,
// <fcela@ce.chalmers.se>.
// For more information, see the following:
// URL "http://www.ce.chalmers.se/~fcela/tcp-tour.html"

static class  RFC793eduTcpClass : public TclClass {
public:
	RFC793eduTcpClass() : TclClass("Agent/TCP/RFC793edu") {}
	TclObject* create(int, const char*const*) {
		return (new RFC793eduTcpAgent());
	}
} class_rfc793edu;


RFC793eduTcpAgent::RFC793eduTcpAgent() : TcpAgent() 
{
}

void
RFC793eduTcpAgent::delay_bind_init_all()
{
	delay_bind_init_one("add793expbackoff_");
	delay_bind_init_one("add793jacobsonrtt_");
	delay_bind_init_one("add793fastrtx_");
	delay_bind_init_one("add793slowstart_");
	delay_bind_init_one("add793additiveinc_");
	delay_bind_init_one("add793karnrtt_");
	delay_bind_init_one("add793exponinc_"); 
	delay_bind_init_one("rto_");
	TcpAgent::delay_bind_init_all();
        reset();
}

int
RFC793eduTcpAgent::delay_bind_dispatch(const char *varName, 
				       const char *localName,TclObject *tracer)
{
        if (delay_bind_bool(varName, localName, "add793expbackoff_", 
			    &add793expbackoff_, tracer)) 
		return TCL_OK;
        if (delay_bind_bool(varName, localName, "add793jacobsonrtt_", 
			    &add793jacobsonrtt_, tracer)) 
		return TCL_OK;
        if (delay_bind_bool(varName, localName, "add793fastrtx_", 
			    &add793fastrtx_, tracer)) 
		return TCL_OK;
        if (delay_bind_bool(varName, localName, "add793slowstart_", 
			    &add793slowstart_, tracer)) 
		return TCL_OK;
        if (delay_bind_bool(varName, localName, "add793slowstart_", 
			    &add793slowstart_, tracer)) 
		return TCL_OK;
        if (delay_bind_bool(varName, localName, "add793additiveinc_", 
			    &add793additiveinc_, tracer)) 
		return TCL_OK;
        if (delay_bind_bool(varName, localName, "add793karnrtt_", 
			    &add793karnrtt_, tracer)) 
		return TCL_OK;
        if (delay_bind_bool(varName, localName, "add793exponinc_", 
			    &add793exponinc_, tracer)) 
		return TCL_OK;
        if (delay_bind(varName, localName, "rto_", 
		       &rto_, tracer)) 
		return TCL_OK;
        return TcpAgent::delay_bind_dispatch(varName, localName, tracer);
}

void RFC793eduTcpAgent::reset()
{
  //Reset here protected vars. 

  rto_ = rtxcur_init_ / tcp_tick_;
  TcpAgent::reset();
}


void RFC793eduTcpAgent::rtt_update(double tao)
{
  double now = Scheduler::instance().clock();
  if (ts_option_)
    t_rtt_ = int(tao /tcp_tick_ + 0.5);
  else {
    double sendtime = now - tao;
    sendtime += boot_time_;
    double tickoff = fmod(sendtime, tcp_tick_);
    t_rtt_ = int((tao + tickoff) / tcp_tick_);
  }
  if (t_rtt_ < 1)
    t_rtt_ = 1;
  
      // Jacobson/Karels RTT estimation as implemented in tcp.cc
      //
      // Diference    = SampleRTT - EstimatedRTT
      // EstimatedRTT = EstimatedRTT + (delta * Difference)
      // Deviation    = Deviation + delta * (|Difference| - Deviation)
      // TimeOut      = EstimatedRTT + 4 * Deviation

      // srtt has 3 bits to the right of the binary point
      // rttvar has 2
      if (t_srtt_ != 0) {
	register short delta;
	delta = t_rtt_ - (t_srtt_ >> T_SRTT_BITS);	// d = (m - a0)
	if ((t_srtt_ += delta) <= 0)	// a1 = 7/8 a0 + 1/8 m
	  t_srtt_ = 1;
	if (delta < 0)
	  delta = -delta;
	delta -= (t_rttvar_ >> T_RTTVAR_BITS);
	if ((t_rttvar_ += delta) <= 0)	// var1 = 3/4 var0 + 1/4 |d|
	  t_rttvar_ = 1;
      } else {
	t_srtt_ = t_rtt_ << T_SRTT_BITS;		// srtt = rtt
	t_rttvar_ = t_rtt_ << (T_RTTVAR_BITS-1);	// rttvar = rtt / 2
      }

   if (add793jacobsonrtt_) 
    {
      //
      // Current retransmit value is 
      //    (unscaled) smoothed round trip estimate
      //    plus 2^rttvar_exp_ times (unscaled) rttvar. 
      //
      rto_ = (((t_rttvar_ << (rttvar_exp_ + (T_SRTT_BITS - T_RTTVAR_BITS)))                   + t_srtt_)  >> T_SRTT_BITS );
    }
   else { 
     // RFC 793
     rto_ = (t_srtt_ >> ( T_SRTT_BITS - 1) );
   }
      t_rtxcur_ = rto_  * tcp_tick_;
  return;
}


void RFC793eduTcpAgent::rtt_backoff()
{
  if (add793expbackoff_) {
    // Standard Tahoe Code

	if (t_backoff_ < 64)
		t_backoff_ <<= 1;

	if (t_backoff_ > 8) {
		/*
		 * If backed off this far, clobber the srtt
		 * value, storing it in the mean deviation
		 * instead.
		 */
		t_rttvar_ += (t_srtt_ >> T_SRTT_BITS);
		t_srtt_ = 0;
	}
  }
  // safe paranoia
  else 
    t_backoff_ = 1;
}


void RFC793eduTcpAgent::recv(Packet *pkt, Handler*)
{
	hdr_tcp *tcph = hdr_tcp::access(pkt);
#ifdef notdef
	if (pkt->type_ != PT_ACK) {
		Tcl::instance().evalf("%s error \"received non-ack\"",
				      name());
		Packet::free(pkt);
		return;
	}
#endif
	++nackpack_;
	ts_peer_ = tcph->ts();
	int ecnecho = hdr_flags::access(pkt)->ecnecho();
	if (ecnecho && ecn_)
		ecn(tcph->seqno());
	recv_helper(pkt);
	/* grow cwnd and check if the connection is done */ 
	if (tcph->seqno() > last_ack_) {
		recv_newack_helper(pkt);
		if (last_ack_ == 0 && delay_growth_) { 
			cwnd_ = initial_window();
		}
	} else if (tcph->seqno() == last_ack_) {
                if (hdr_flags::access(pkt)->eln_ && eln_) {
                        tcp_eln(pkt);
                        return;
                }
		// fast retransmission
		if ( (++dupacks_ == numdupacks_) && add793fastrtx_ ) {
			dupack_action();
		}
	}
	Packet::free(pkt);
	/*
	 * Try to send more data.
	 */
	send_much(0, 0, maxburst_);
}



void RFC793eduTcpAgent::opencwnd()
{
        if ( !(add793slowstart_ || add793exponinc_ || add793additiveinc_)) {
	  cwnd_ = wnd_;
        } else if (((cwnd_ < ssthresh_) && (!add793additiveinc_)) 
		   || add793exponinc_)
	  {
		/* slow-start (exponential) */
	        if (add793exponinc_ && (cwnd_ < ssthresh_))
		  cwnd_ = ssthresh_;
		else cwnd_ += 1;
	} else {
		/* linear */
		double f;

		if (add793additiveinc_ && (cwnd_ < ssthresh_))
		  cwnd_ = ssthresh_;
		else {
		  switch (wnd_option_) {
		  case 0:
		    if (++count_ >= cwnd_) {
		      count_ = 0;
		      ++cwnd_;
		    }
		    break;
		    
		  case 1:
			/* This is the standard algorithm. */
		    cwnd_ += 1 / cwnd_;
		    break;
		    
		  case 2:
		    /* These are window increase algorithms
		     * for experimental purposes only. */
		    f = (t_srtt_ >> T_SRTT_BITS) * tcp_tick_;
		    f *= f;
		    f *= wnd_const_;
		    f += fcnt_;
		    if (f > cwnd_) {
		      fcnt_ = 0;
		      ++cwnd_;
		    } else
		      fcnt_ = f;
		    break;
			
		  case 3:
		    f = awnd_;
		    f *= f;
		    f *= wnd_const_;
		    f += fcnt_;
		    if (f > cwnd_) {
		      fcnt_ = 0;
		      ++cwnd_;
		    } else
		      fcnt_ = f;
		    break;
		    
		  case 4:
		    f = awnd_;
		    f *= wnd_const_;
		    f += fcnt_;
		    if (f > cwnd_) {
		      fcnt_ = 0;
		      ++cwnd_;
		    } else
                                fcnt_ = f;
		    break;
		  case 5:
		    f = (t_srtt_ >> T_SRTT_BITS) * tcp_tick_;
		    f *= wnd_const_;
		    f += fcnt_;
		    if (f > cwnd_) {
		      fcnt_ = 0;
		      ++cwnd_;
		    } else
		      fcnt_ = f;
		    break;
		    
		  default:
#ifdef notdef
		    /*XXX*/
		    error("illegal window option %d", wnd_option_);
#endif
		    abort();
		  }
		}
	}
	// if maxcwnd_ is set (nonzero), make it the cwnd limit
	if (maxcwnd_ && (int(cwnd_) > maxcwnd_))
	  cwnd_ = maxcwnd_;
	
	return;
}


void  RFC793eduTcpAgent::output(int seqno, int reason)
{
	int force_set_rtx_timer = 0;
	Packet* p = allocpkt();
	hdr_tcp *tcph = hdr_tcp::access(p);
	hdr_flags* hf = hdr_flags::access(p);
	tcph->seqno() = seqno;
	tcph->ts() = Scheduler::instance().clock();
	tcph->ts_echo() = ts_peer_;
	tcph->reason() = reason;
	if (ecn_) {
		hf->ect() = 1;	// ECN-capable transport
	}
	if (cong_action_) {
		hf->cong_action() = TRUE;  // Congestion action.
		cong_action_ = FALSE;
        }
	/* Check if this is the initial SYN packet. */
	if (seqno == 0) {
		if (syn_) {
			hdr_cmn::access(p)->size() = tcpip_base_hdr_size_;
		}
		if (ecn_) {
			hf->ecnecho() = 1;
//			hf->cong_action() = 1;
			hf->ect() = 0;
		}
	}
        int bytes = hdr_cmn::access(p)->size();

	/* if no outstanding data, be sure to set rtx timer again */
	if (highest_ack_ == maxseq_)
		force_set_rtx_timer = 1;
	/* call helper function to fill in additional fields */
	output_helper(p);

        ++ndatapack_;
        ndatabytes_ += bytes;
	send(p, 0);
	if (seqno == curseq_ && seqno > maxseq_)
		idle();  // Tell application I have sent everything so far
	if (seqno > maxseq_) {
		maxseq_ = seqno;
		if (!rtt_active_) {
			rtt_active_ = 1;			
			if (seqno > rtt_seq_) {
				rtt_seq_ = seqno;			
				rtt_ts_ = Scheduler::instance().clock();
			}
					
		}
	} else {
	  if (!add793karnrtt_) {
	        rtt_active_ = 1;
		rtt_seq_ = seqno;			
		rtt_ts_ = Scheduler::instance().clock();
	  }	
        	++nrexmitpack_;
        	nrexmitbytes_ += bytes;
	}
	if (!(rtx_timer_.status() == TIMER_PENDING) || force_set_rtx_timer)
		/* No timer pending.  Schedule one. */
		set_rtx_timer();
}

void RFC793eduTcpAgent::recv_newack_helper(Packet *pkt) {
	//hdr_tcp *tcph = hdr_tcp::access(pkt);
	newack(pkt);
	if (!ect_ || !hdr_flags::access(pkt)->ecnecho() ||
		(old_ecn_ && ecn_burst_)) 
		/* If "old_ecn", this is not the first ACK carrying ECN-Echo
		 * after a period of ACKs without ECN-Echo.
		 * Therefore, open the congestion window. */
	        opencwnd();
	if (ect_) {
		if (!hdr_flags::access(pkt)->ecnecho())
			ecn_backoff_ = 0;
		if (!ecn_burst_ && hdr_flags::access(pkt)->ecnecho())
			ecn_burst_ = TRUE;
		else if (ecn_burst_ && ! hdr_flags::access(pkt)->ecnecho())
			ecn_burst_ = FALSE;
	}
	if (!ect_ && hdr_flags::access(pkt)->ecnecho() &&
		!hdr_flags::access(pkt)->cong_action())
		ect_ = 1;
	/* if the connection is done, call finish() */
	if ((highest_ack_ >= curseq_-1) && !closed_) {
		closed_ = 1;
		finish();
	}
}

void RFC793eduTcpAgent::newack(Packet* pkt)
{
	double now = Scheduler::instance().clock();
	hdr_tcp *tcph = hdr_tcp::access(pkt);
	/* 
	 * Wouldn't it be better to set the timer *after*
	 * updating the RTT, instead of *before*? 
	 */
	newtimer(pkt);
	dupacks_ = 0;
	last_ack_ = tcph->seqno();
	highest_ack_ = last_ack_;

	if (t_seqno_ < last_ack_ + 1)
		t_seqno_ = last_ack_ + 1;
	/* 
	 * Update RTT only if it's OK to do so from info in the flags header.
	 * This is needed for protocols in which intermediate agents
	 * in the network intersperse acks (e.g., ack-reconstructors) for
	 * various reasons (without violating e2e semantics).
	 */	
	hdr_flags *fh = hdr_flags::access(pkt);
	if (!fh->no_ts_) {
		if (ts_option_)
			rtt_update(now - tcph->ts_echo());

		if (rtt_active_ && tcph->seqno() >= rtt_seq_) {
			if (!ect_ || !ecn_backoff_ || 
				!hdr_flags::access(pkt)->ecnecho()) {
				/* 
				 * Don't end backoff if still in ECN-Echo with
			 	 * a congestion window of 1 packet. 
				 */
				t_backoff_ = 1;
				ecn_backoff_ = 0;
			}
			rtt_active_ = 0;
			if (!ts_option_)
				rtt_update(now - rtt_ts_);
		}
	}
	/* update average window */
	awnd_ *= 1.0 - wnd_th_;
	awnd_ += wnd_th_ * cwnd_;
}

	
