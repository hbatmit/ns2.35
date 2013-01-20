/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Super-rational TCP congestion control.
 * Keith Winstein, Hari Balakrishnan (MIT CSAIL & Wireless@MIT).
 * January 2013.
 */

#include "tcp-rational.h"

static class TcpRationalClass : public TclClass {
public:
	TcpRationalClass() : TclClass("Agent/TCP/Rational") {}
	TclObject* create(int, const char*const*) {
		return (new TcpRationalAgent());
	}
} class_tcprational;

static class RenoTcpRationalClass : public TclClass {
public:
	RenoTcpRationalClass() : TclClass("Agent/TCP/Reno/Rational") {}
	TclObject* create(int, const char*const*) {
		return (new RenoTcpRationalAgent());
	}
} class_reno_tcp_rational;	

static class NewRenoTcpRationalClass : public TclClass {
public:
	NewRenoTcpRationalClass() : TclClass("Agent/TCP/Newreno/Rational") {}
	TclObject* create(int, const char*const*) {
		return (new NewRenoTcpRationalAgent());
	}
} class_newreno_tcp_rational;	

/*
 * initial_window() is called in a few different places in tcp.cc.
 * This function overrides the default. 
 */
double
TcpRationalAgent::initial_window()
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
TcpRationalAgent::send_helper(int maxburst) 
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
TcpRationalAgent::send_idle_helper() 
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
TcpRationalAgent::recv_newack_helper(Packet *pkt) 
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
	if (count_bytes_acked_)
		ackcount = tcph->seqno() - last_ack_;
	else
		ackcount = 1;
	newack(pkt);		// updates RTT to set RTO properly, etc.
	maxseq_ = max(maxseq_, highest_ack_);
	update_cwnd();
	/* if the connection is done, call finish() */
	if ((highest_ack_ >= curseq_-1) && !closed_) {
		closed_ = 1;
		finish();
	}
}

void 
TcpRationalAgent::update_cwnd()
{
	cwnd_ = 100;		// fix this to be the right computation!
}
