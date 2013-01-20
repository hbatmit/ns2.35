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

/* TCPRational with Tahoe */
class TcpRationalAgent : public virtual TcpAgent {
public:
	TcpRationalAgent() : count_bytes_acked_(0)
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
 * TCPRational with Reno.
 */

class RenoTcpRationalAgent : public virtual RenoTcpAgent, public TcpRationalAgent {
public:
	RenoTcpRationalAgent() : RenoTcpAgent(), TcpRationalAgent() {}

	/* helper functions */
	virtual void send_helper(int maxburst) {TcpRationalAgent::send_helper(maxburst);}
	virtual void send_idle_helper() {TcpRationalAgent::send_idle_helper();}
	virtual void recv_newack_helper(Packet* pkt) {TcpRationalAgent::recv_newack_helper(pkt);}
	virtual double initial_window() {return TcpRationalAgent::initial_window();}
	virtual void update_cwnd() {TcpRationalAgent::update_cwnd();}
};

/* 
 * TCPRational with NewReno.
 */
class NewRenoTcpRationalAgent : public virtual NewRenoTcpAgent, public TcpRationalAgent {
public:
	NewRenoTcpRationalAgent() : NewRenoTcpAgent(), TcpRationalAgent() {}

	/* helper functions */
	virtual void send_helper(int maxburst) {TcpRationalAgent::send_helper(maxburst);}
	virtual void send_idle_helper() {TcpRationalAgent::send_idle_helper();}
	virtual void recv_newack_helper(Packet* pkt) {TcpRationalAgent::recv_newack_helper(pkt);}
	virtual double initial_window() {return TcpRationalAgent::initial_window();}
	virtual void update_cwnd() {TcpRationalAgent::update_cwnd();}
};

#endif
