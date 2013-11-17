/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Super-rational TCP congestion control.
 * Keith Winstein, Hari Balakrishnan (MIT CSAIL & Wireless@MIT).
 * January 2013.
 */

#ifndef ns_tcp_rational_h
#define ns_tcp_rational_h

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
	//	double _intersend_time;
	TracedDouble _intersend_time;

public:
	RationalTcpAgent();
	~RationalTcpAgent();

	/* helper functions, overriden from TcpAgent */
	void send_helper(int maxburst) override;
	void send_idle_helper() override;
	void recv_newack_helper(Packet* pkt) override;
	double initial_window() override;
	void update_memory( const RemyPacket packet );
	void timeout_nonrtx( int tno ) override;
	void output( int seqno, int reason ) override { _last_send_time = Scheduler::instance().clock(); TcpAgent::output( seqno, reason ); }
	void update_cwnd_and_pacing( void );

protected:
	void delay_bind_init_all() override;
	int delay_bind_dispatch(const char *varName, const char *localName, TclObject *tracer) override;
	void traceVar(TracedVar *v) override;
	int tracewhisk_;	// trace whiskers?
	double _last_send_time;
	int count_bytes_acked_;
};

#endif
