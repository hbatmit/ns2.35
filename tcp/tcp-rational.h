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
	double _intersend_time;

public:
	RationalTcpAgent();
	~RationalTcpAgent();

	/* helper functions */
	virtual void send_helper(int maxburst);
	virtual void send_idle_helper();
	virtual void recv_newack_helper(Packet* pkt);
	virtual double initial_window();
	virtual void update_memory( const RemyPacket packet );
	virtual void timeout_nonrtx( int tno );
	virtual void output( int seqno, int reason ) { _last_send_time = Scheduler::instance().clock(); TcpAgent::output( seqno, reason ); }
	virtual void update_cwnd_and_pacing( void );

protected:
	virtual void delay_bind_init_all();
	virtual int delay_bind_dispatch(const char *varName, const char *localName, TclObject *tracer);
	int tracewhisk_;	// trace whiskers?
	double _last_send_time;
	int count_bytes_acked_;
};

/* 
 * Rational TCP with Reno.
 */

class RationalRenoTcpAgent : public virtual RenoTcpAgent, public RationalTcpAgent {
public:
	RationalRenoTcpAgent() : RenoTcpAgent(), RationalTcpAgent() {}

	/* helper functions */
	virtual void send_helper(int maxburst) {RationalTcpAgent::send_helper(maxburst);}
	virtual void send_idle_helper() {RationalTcpAgent::send_idle_helper();}
	virtual void recv_newack_helper(Packet* pkt) {RationalTcpAgent::recv_newack_helper(pkt);}
	virtual double initial_window() {return RationalTcpAgent::initial_window();}
	virtual void update_memory( const RemyPacket packet ) {RationalTcpAgent::update_memory(packet);}
	virtual void output( int seqno, int reason ) { _last_send_time = Scheduler::instance().clock(); RenoTcpAgent::output( seqno, reason ); }
};

/* 
 * Rational TCP with NewReno.
 */
class RationalNewRenoTcpAgent : public virtual NewRenoTcpAgent, public RationalTcpAgent {
public:
	RationalNewRenoTcpAgent() : NewRenoTcpAgent(), RationalTcpAgent() {}

	/* helper functions */
	virtual void send_helper(int maxburst) {RationalTcpAgent::send_helper(maxburst);}
	virtual void send_idle_helper() {RationalTcpAgent::send_idle_helper();}
	virtual void recv_newack_helper(Packet* pkt) {RationalTcpAgent::recv_newack_helper(pkt);}
	virtual double initial_window() {return RationalTcpAgent::initial_window();}
	virtual void update_memory( const RemyPacket packet ) {RationalTcpAgent::update_memory(packet);}
	virtual void output( int seqno, int reason ) { _last_send_time = Scheduler::instance().clock(); NewRenoTcpAgent::output( seqno, reason ); }
};

#endif
