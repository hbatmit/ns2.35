/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Super-rational TCP congestion control.
 * Keith Winstein, Hari Balakrishnan (MIT CSAIL & Wireless@MIT).
 * January 2013.
 */

#ifndef ns_tcp_rational_h
#define ns_tcp_rational_h

#include <map>
#include <queue>

#include "tcp.h"
#include "ip.h"
#include "flags.h"
#include "random.h"
#include "template.h"

#include "remy/whiskertree.hh"

class RationalTcpAgent;

/* Implementation based on RFC 6298 */
class TcpRttTimer : public TimerHandler {
private:
	double srtt_;
	double rtt_var_;
	double rto_;
	bool first_measurement_done_;
public:
	TcpRttTimer(RationalTcpAgent *rat): srtt_( 0.0 ), rtt_var_( 0.0 ), rto_( 1.0 ), first_measurement_done_( false ), rat_( rat ) {}
	void update_rtt_timer( const double & new_rtt );
	double rto() { return rto_; }
	virtual void expire(Event *e) override;
	const double alpha = 0.125;
	const double beta = 0.25;
	const double G = 0.1;
	const double K = 4.0;
	RationalTcpAgent* rat_;

};

/* Rational TCP with Tahoe */
class RationalTcpAgent : public virtual TcpAgent {
private:
	const WhiskerTree *_whiskers;
	Memory _memory;
	TracedDouble _intersend_time;
	std::map<double, std::pair<int32_t, uint32_t>> transmission_map_;
	std::queue<int32_t> retx_pending_;
	double largest_oo_ts_; /* largest seen out of order timestamp */
	int largest_oo_ack_;   /* largest seen out of order ack */
	unsigned int transmitted_pkts_;
	TcpRttTimer rational_rtx_timer_;

public:
	RationalTcpAgent();
	~RationalTcpAgent();

	/* helper functions, overriden from TcpAgent */
	void send_much(int force, int reason, int maxburst) override;
	void send_helper(int maxburst) override;
	void recv(Packet* p, Handler *h) override;
	double initial_window() override;
	void timeout( int tno ) override { return timeout_nonrtx( tno ); }
	void timeout_nonrtx( int tno ) override;
	void output( int seqno, int reason ) override;
	int command(int argc, const char*const* argv) override;
	void reset_to_iw(void) override;

	/* other utility functions */
	void update_cwnd_and_pacing( void );
	void update_congestion_state(Packet* pkt);
	void update_memory( const RemyPacket & packet, const unsigned int flow_id );
	void assert_outstanding( void ) { assert( t_seqno_ > last_ack_ ); }
	void retx_earliest_outstanding( void ) { output( last_ack_ + 1, TCP_REASON_TIMEOUT ); }

protected:
	void delay_bind_init_all() override;
	int delay_bind_dispatch(const char *varName, const char *localName, TclObject *tracer) override;
	void traceVar(TracedVar *v) override;
	int tracewhisk_;	// trace whiskers?
	double _last_send_time;
	int count_bytes_acked_;
};

#endif
