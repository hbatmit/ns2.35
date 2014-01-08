/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Super-rational TCP congestion control.
 * Keith Winstein, Hari Balakrishnan (MIT CSAIL & Wireless@MIT).
 * January 2013.
 */

#include <algorithm>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <string>
#include "exception.hh"
#include "tcp-rational.h"

void TcpRttTimer::update_rtt_timer( const double & new_rtt )
{
	assert( new_rtt > 0 );
	if (not first_measurement_done_) {
		srtt_ = new_rtt;
		rtt_var_ = new_rtt / 2.0;
		rto_ = srtt_ + std::max( G, K * rtt_var_ );
		first_measurement_done_ = true;
	} else {
		rtt_var_ = ( 1 - beta ) * rtt_var_ + beta * abs( srtt_ - new_rtt );
		srtt_ = ( 1 - alpha ) * srtt_ + alpha * new_rtt;
		rto_ = srtt_ + std::max( G, K * rtt_var_ );
		rto_ = std::max( 0.2, rto_ );
	}
}

void TcpRttTimer::expire(Event *e) {
	/* Ensure there is something outstanding */
	rat_->assert_outstanding();

	/* RFC6298 5.4: Retransmit earliest outstanding: last_ack_ + 1 */
	rat_->retx_earliest_outstanding();

	/* RFC6298 5.5: Backoff rto_ */
	rto_ = rto_ * 2.0;

	/* RFC6298 5.6: Set timer */
	resched( rto_ );

	/* RFC6298 5.7: N/A */
};

RationalTcpAgent::RationalTcpAgent()
	: _whiskers( NULL ),
	  _memory(),
	  _intersend_time( 0.0 ),
	  transmission_map_(),
	  retx_pending_(),
	  largest_oo_ts_( -1.0 ),
	  largest_oo_ack_( -1 ),
	  transmitted_pkts_( 0 ),
	  rational_rtx_timer_( this ),
	  _last_send_time( 0 ),
	  count_bytes_acked_( 0 )
{
	try {
		bind_bool("count_bytes_acked_", &count_bytes_acked_);

		/* get whisker filename */
		const char *filename = getenv( "WHISKERS" );
		fprintf( stderr, "WHISKERS file: %s\n", filename );	
		if ( !filename ) {
			throw Exception("RationalTcpAgent::RationalTcpAgent", "RemyTCP: Missing WHISKERS environment variable.");
		}

		/* open file */
		int fd = open( filename, O_RDONLY );
		if ( fd < 0 ) {
			throw Exception("RationalTcpAgent::RationalTcpAgent open() call");
		}

		/* parse whisker definition */
		RemyBuffers::WhiskerTree tree;
		if ( !tree.ParseFromFileDescriptor( fd ) ) {
			throw Exception("RationalTcpAgent::RationalTcpAgent", "RemyTCP: Could not parse whiskers in " + std::string(filename) );
		}

		/* close file */
		if ( ::close( fd ) < 0 ) {
			throw Exception("RationalTcpAgent::RationalTcpAgent close() call");
		}

		/* store whiskers */
		_whiskers = new WhiskerTree( tree );
		fid_ = 0;
        } catch (const Exception & e) {
		e.perror();
		exit(1);
        }
}

RationalTcpAgent::~RationalTcpAgent()
{
	if ( _whiskers ) {
		delete _whiskers;
	}
}

void
RationalTcpAgent::delay_bind_init_all()
{
	delay_bind_init_one("_intersend_time");
	TcpAgent::delay_bind_init_all();
        reset();
}

int
RationalTcpAgent::delay_bind_dispatch(const char *varName, const char *localName, 
				   TclObject *tracer)
{
        if (delay_bind(varName, localName, "tracewhisk_", &tracewhisk_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "_intersend_time", &_intersend_time, tracer)) return TCL_OK;
        return TcpAgent::delay_bind_dispatch(varName, localName, tracer);
}


static class RationalTcpClass : public TclClass {
public:
	RationalTcpClass() : TclClass("Agent/TCP/Rational") {}
	TclObject* create(int, const char*const*) {
		return (new RationalTcpAgent());
	}
} class_rational_tcp;

/*
 * initial_window() is called in a few different places in tcp.cc.
 * This function overrides the default. 
 */
double
RationalTcpAgent::initial_window()
{
	_memory.reset();
	update_cwnd_and_pacing();
	return cwnd_;
}

void 
RationalTcpAgent::send_helper(int maxburst) 
{
	/* 
	 * If there is still data to be sent and there is space in the
	 * window, set a timer to schedule the next burst.
	 */
	assert( burstsnd_timer_.status() != TIMER_PENDING );

	/* schedule wakeup */
	auto num_lost_or_received = (largest_oo_ts_ != -1.0) ? transmission_map_.at( largest_oo_ts_ ).second : 0.0;
	if (transmission_map_.size() < num_lost_or_received + window()) {
		if ( ( not retx_pending_.empty() ) or ( t_seqno_ < curseq_ ) ) {
			const double now( Scheduler::instance().clock() );
			const double time_since_last_send( now - _last_send_time );
			const double wait_time( _intersend_time - time_since_last_send );
			assert( wait_time > 0 );
			burstsnd_timer_.resched( wait_time );
		}
	}
}

void RationalTcpAgent::recv(Packet* pkt, Handler *h) {
	hdr_tcp *tcph = hdr_tcp::access(pkt);
	hdr_cmn *cmn_hdr = hdr_cmn::access(pkt);
	
	// Check if ACK is valid.  Suggestion by Mark Allman. 
	assert (cmn_hdr->ptype() == PT_ACK);
	assert (tcph->seqno() >= last_ack_);

	// Update last_ack_ if receiver received next in-order packet
	if ( tcph->seqno() > last_ack_ ) {
		/* RFC 6298 5.3: Acknowledges new data */
		rational_rtx_timer_.resched( rational_rtx_timer_.rto() );
		last_ack_ = tcph->seqno();
	}

	// Calculate srtt and friends
	rational_rtx_timer_.update_rtt_timer( Scheduler::instance().clock() - tcph->ts_echo() );

	/* Received ts has to be greater than the largest seen so far */
	/* Assumption: No reordering */
	assert( tcph->ts_echo() > largest_oo_ts_ );
	largest_oo_ts_ = tcph->ts_echo();

	/* Look at incoming ts */
	int oo_acked_seqno = transmission_map_.at( tcph->ts_echo() ).first;

	/* Compare it to largest_oo_ack_ to fast retransmit missing packets */
	if ( oo_acked_seqno > largest_oo_ack_ ) {
		for (int i = largest_oo_ack_ + 1; i < oo_acked_seqno; i++) {
			retx_pending_.push( i );
		}
	}
	largest_oo_ack_ = max( largest_oo_ack_, oo_acked_seqno );

	/* Update congestion state regardless */
	update_congestion_state(pkt);

	/* RFC 6298: 5.2 No more outstanding packets */
	if( t_seqno_ == last_ack_ or
            ( t_seqno_ - 1 == last_ack_ and t_seqno_ == curseq_ ) or /* ontype bytes overshoot */
            ( t_seqno_ - 1 == last_ack_ and curseq_ == maxseq_ ) ) { /* ontype time overshoot */
		rational_rtx_timer_.force_cancel();
	}

	if (app_) app_->recv_ack(pkt); /* ANIRUDH: Callback to app */
	Packet::free(pkt);
}

void RationalTcpAgent::send_much(int force, int reason, int maxburst)
{
	//assert (burstsnd_timer_.status() != TIMER_PENDING);
	if (burstsnd_timer_.status() == TIMER_PENDING ) {
		return;
	}

	auto num_lost_or_received = (largest_oo_ts_ != -1.0) ? transmission_map_.at( largest_oo_ts_ ).second : 0.0;
	if (transmission_map_.size() < num_lost_or_received + window()) {
		if ( not retx_pending_.empty() ) {
			output(retx_pending_.front(), TCP_REASON_DUPACK );
			retx_pending_.pop();
		}
		else if ( t_seqno_ < curseq_ ) {
			output(t_seqno_, reason);
			t_seqno_ ++ ;
		}
	}
	send_helper(maxburst);
}

void RationalTcpAgent::output( int seqno, int reason )
{
	/* Cannot have two transmissions at the same time */
	assert( transmission_map_.find( Scheduler::instance().clock() ) == transmission_map_.end() );
	transmission_map_[ Scheduler::instance().clock() ] = make_pair( seqno, ++transmitted_pkts_ );
	assert( transmission_map_.size() == transmitted_pkts_ );
	_last_send_time = Scheduler::instance().clock();
	TcpAgent::output( seqno, reason );
	if ( rational_rtx_timer_.status() != TIMER_PENDING ) {
		/* RFC 6298: 5.1 : Set timer if it isn't already running */
		rational_rtx_timer_.resched( rational_rtx_timer_.rto() );
	}
}

/*
 * Update congestion state appropriately.
 */
void
RationalTcpAgent::update_congestion_state(Packet *pkt) 
{
	double now = Scheduler::instance().clock();
	hdr_tcp *tcph = hdr_tcp::access(pkt);

	double timestep = 1000.0;

	unsigned int src = hdr_ip::access(pkt)->saddr();
	unsigned int flow_id = hdr_ip::access(pkt)->flowid();
	update_memory( RemyPacket( src, flow_id, timestep * tcph->ts_echo(), timestep * now ), fid_ );

	update_cwnd_and_pacing();

	/* TODO: Figure out what this is: if the connection is done, call finish() */
	assert( highest_ack_ < curseq_-1);
}

void 
RationalTcpAgent::update_memory( const RemyPacket & packet, const unsigned int flow_id )
{
	std::vector< RemyPacket > packets( 1, packet );
	_memory.packets_received( packets, flow_id );
}

void
RationalTcpAgent::update_cwnd_and_pacing( void )
{
	const Whisker & current_whisker( _whiskers->use_whisker( _memory ) );

	unsigned int new_cwnd = current_whisker.window( (unsigned int)cwnd_ );

	cwnd_ = new_cwnd;

	double timestep_inverse = .001;

	_intersend_time = timestep_inverse * current_whisker.intersend();
	double _print_intersend = _intersend_time;

	const double now( Scheduler::instance().clock() );
	const double time_since_last_send( now - _last_send_time );
	const double wait_time( _intersend_time - time_since_last_send );

	if (tracewhisk_) {
		fprintf( stderr, "%g: %s whisker %s newcwnd: %u newintersend: %f\n", now, _memory.str().c_str(), current_whisker.str().c_str(), new_cwnd, _print_intersend );
	}

	burstsnd_timer_.resched( wait_time <= 0 ? 0 : wait_time );
}

void
RationalTcpAgent::timeout_nonrtx( int tno )
{
	assert ( tno != TCP_TIMER_DELSND );
	if ( tno == TCP_TIMER_BURSTSND ) {
		send_much( 1, TCP_REASON_TIMEOUT, maxburst_ );
	}
}

void 
RationalTcpAgent::traceVar(TracedVar *v)
{
	#define TCP_WRK_SIZE 512
	if (!channel_)
		return;

	double curtime;
	Scheduler& s = Scheduler::instance();
	char wrk[TCP_WRK_SIZE];

	curtime = &s ? s.clock() : 0;
	
	if (v == &_intersend_time) {
		snprintf(wrk, TCP_WRK_SIZE,
			 "%-8.5f %-2d %-2d %-2d %-2d %s %-6.6f\n",
			 curtime, addr(), port(), daddr(), dport(),
			 v->name(), double(*((TracedDouble*) v))); 
		(void)Tcl_Write(channel_, wrk, -1);
	} else {
		TcpAgent::traceVar(v);
	}

}
void RationalTcpAgent::reset_to_iw(void)
{
	cwnd_ = 0;
	_last_send_time = 0;
	_intersend_time = 0;
	initial_window();
	fid_++;
}

int RationalTcpAgent::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "reset_to_iw") == 0) {
			/* Clear all state except for the sequence numbers */
			reset_to_iw();
			return (TCL_OK);
		}
	}
	return (TcpAgent::command(argc, argv));
}
