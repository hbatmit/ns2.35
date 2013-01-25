/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Super-rational TCP congestion control.
 * Keith Winstein, Hari Balakrishnan (MIT CSAIL & Wireless@MIT).
 * January 2013.
 */

#include "tcp-rational.h"

static class RationalTcpClass : public TclClass {
public:
	RationalTcpClass() : TclClass("Agent/TCP/Rational") {}
	TclObject* create(int, const char*const*) {
		return (new RationalTcpAgent());
	}
} class_rational_tcp;

static class RationalRenoTcpClass : public TclClass {
public:
	RationalRenoTcpClass() : TclClass("Agent/TCP/Reno/Rational") {}
	TclObject* create(int, const char*const*) {
		return (new RationalRenoTcpAgent());
	}
} class_rational_reno_tcp;

static class RationalNewRenoTcpClass : public TclClass {
public:
	RationalNewRenoTcpClass() : TclClass("Agent/TCP/Newreno/Rational") {}
	TclObject* create(int, const char*const*) {
		return (new RationalNewRenoTcpAgent());
	}
} class_rational_newreno_tcp;

/*
 * initial_window() is called in a few different places in tcp.cc.
 * This function overrides the default. 
 */
double
RationalTcpAgent::initial_window()
{
	// This is just a copy of the relevant parts of the default TCP.
	if (wnd_init_option_ == 1) {
		return (wnd_init_);
	} else {
		// do iw according to Internet draft
 		if (size_ <= 1095) {
			return (4.0);
	 	} else if (size_ < 2190) {
			return (3.0);
		} else {
			return (2.0);
		}
	}
}

void 
RationalTcpAgent::send_helper(int maxburst) 
{
	/* 
	 * If there is still data to be sent and there is space in the
	 * window, set a timer to schedule the next burst. Note that
	 * we wouldn't get here if TCP_TIMER_BURSTSEND were pending,
	 * so we do not need an explicit check here.
	 */
	if (t_seqno_ <= highest_ack_ + window() && t_seqno_ < curseq_) {
		burstsnd_timer_.resched(t_srtt_*maxburst_/window());
	}
}

/* 
 * Connection has been idle for some time. 
 */
void
RationalTcpAgent::send_idle_helper() 
{

}

/*
 * recv_newack_helper(pkt) is called from TcpAgent::recv() in tcp.cc when a
 * new cumulative ACK arrives.
 * Process a new ACK: update SRTT, make sure to call newack() of the parent 
 * class, and, most importantly, update cwnd according to the model.
 * This function overrides the default.
 */
void
RationalTcpAgent::recv_newack_helper(Packet *pkt) 
{
	double now = Scheduler::instance().clock();
	hdr_tcp *tcph = hdr_tcp::access(pkt);
	double last_rtt = Scheduler::instance().clock() - tcph->ts_echo();
	double g = 1/8; /* gain used for smoothing rtt */
	double h = 1/4; /* gain used for smoothing rttvar */
	double delta;
	int ackcount, i;

	double last_ack_time_ = now;
	/*
	 * If we are counting the actual amount of data acked, ackcount >= 1.
	 * Otherwise, ackcount=1 just as in standard TCP.
	 */
	if (count_bytes_acked_) {
		ackcount = tcph->seqno() - last_ack_;
	} else {
		ackcount = 1;
	}
	newack(pkt);		// updates RTT to set RTO properly, etc.
	maxseq_ = ::max(maxseq_, highest_ack_);
	update_cwnd(last_rtt);
	/* if the connection is done, call finish() */
	if ((highest_ack_ >= curseq_-1) && !closed_) {
		closed_ = 1;
		finish();
	}
}

/*
 * A simple function to update cwnd using last_rtt; provided only for reference.
 * This function is highly conservative about delay, slowing down the rate 
 * aggressively when delay rises.
 */
void 
RationalTcpAgent::update_cwnd(double last_rtt)
{
	double srtt =  (t_srtt_ >> T_SRTT_BITS) * tcp_tick_;
	if (last_rtt > 2*srtt) {
		// reduce cwnd 
		double extratime = last_rtt - srtt;
		double extra = (cwnd_/srtt)*extratime;
		printf("Reducing cwnd from %.2f ", (double)cwnd_);
		cwnd_ -= (cwnd_/srtt)*extratime/2;
		if (cwnd_ < 1) {
			cwnd_ = 1;
		}
		printf("to %.2f srtt %.3f last_rtt %.3g\n", (double)cwnd_, (double)srtt, (double)last_rtt);
	} else {
		opencwnd();
	}
}
