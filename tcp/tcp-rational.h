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

/* Rational TCP with Tahoe */
class RationalTcpAgent : public virtual TcpAgent {
private:
	const WhiskerTree *_whiskers;
	Memory _memory;
	TracedDouble _intersend_time;
	std::map<double, std::pair<int32_t, uint32_t>> transmission_map_;
	std::queue<int32_t> retx_pending_;
	double largest_acked_ts_;
	int largest_acked_seqno_;
	unsigned int transmitted_pkts_;

public:
	RationalTcpAgent();
	~RationalTcpAgent();

	/* helper functions, overriden from TcpAgent */
	void send_much(int force, int reason, int maxburst) override;
	void send_helper(int maxburst) override;
	void recv(Packet* p, Handler *h) override;
	double initial_window() override;
	void update_memory( const RemyPacket packet, const unsigned int flow_id );
	void timeout( int tno ) override { return timeout_nonrtx( tno ); }
	void timeout_nonrtx( int tno ) override;
	void output( int seqno, int reason ) override;
	void update_cwnd_and_pacing( void );
	void update_congestion_state(Packet* pkt);
	int command(int argc, const char*const* argv) override;
	void reset_to_iw(void) override;

protected:
	void delay_bind_init_all() override;
	int delay_bind_dispatch(const char *varName, const char *localName, TclObject *tracer) override;
	void traceVar(TracedVar *v) override;
	int tracewhisk_;	// trace whiskers?
	double _last_send_time;
	int count_bytes_acked_;
};

#endif
