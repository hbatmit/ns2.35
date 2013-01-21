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

/* Rational TCP with Tahoe */
class RationalTcpAgent : public virtual TcpAgent {
public:
	RationalTcpAgent() : count_bytes_acked_(0)
	{
		bind_bool("count_bytes_acked_", &count_bytes_acked_);
	}
	/* helper functions */
	virtual void send_helper(int maxburst);
	virtual void send_idle_helper();
	virtual void recv_newack_helper(Packet* pkt);
	virtual double initial_window();
	virtual void update_cwnd();
protected:
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
	virtual void update_cwnd() {RationalTcpAgent::update_cwnd();}
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
	virtual void update_cwnd() {RationalTcpAgent::update_cwnd();}
};

#endif
