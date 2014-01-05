/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Super-rational TCP congestion control.
 * Keith Winstein, Hari Balakrishnan (MIT CSAIL & Wireless@MIT).
 * January 2013.
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <string>
#include "exception.hh"
#include "tcp-rational.h"

RationalTcpAgent::RationalTcpAgent()
	: _whiskers( NULL ),
	  _memory(),
	  _intersend_time( 0.0 ),
	  transmission_map_(),
	  largest_acked_ts_( -1.0 ),
	  _last_send_time( 0 ),
	  count_bytes_acked_( 0 )
{
	try {
		bind_bool("count_bytes_acked_", &count_bytes_acked_);

		/* get whisker filename */
		const char *filename = getenv( "WHISKERS" );
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
	if ( t_seqno_ <= highest_ack_ + window() && t_seqno_ < curseq_ ) {
		const double now( Scheduler::instance().clock() );
		const double time_since_last_send( now - _last_send_time );
		const double wait_time( _intersend_time - time_since_last_send );
		assert( wait_time > 0 );
		burstsnd_timer_.resched( wait_time );
	}
}

void RationalTcpAgent::recv(Packet* pkt, Handler *h) {
	hdr_tcp *tcph = hdr_tcp::access(pkt);
	hdr_cmn *cmn_hdr = hdr_cmn::access(pkt);
	assert (cmn_hdr->ptype() == PT_ACK);
	// Check if ACK is valid.  Suggestion by Mark Allman. 
	assert (tcph->seqno() >= last_ack_);

	/* Received ts has to be greater than the largest seen so far */
	/* Assumption: No reordering */
	assert( tcph->ts() > largest_acked_ts_ );
	largest_acked_ts_ = tcph->ts_echo();

	if (tcph->seqno() > last_ack_) {
		newack(pkt);		// updates RTT to set RTO properly, etc.
		maxseq_ = ::max(maxseq_, highest_ack_);
		update_congestion_state(pkt);
	} else if (tcph->seqno() == last_ack_) {
		update_congestion_state(pkt);

                /* Fast retransmit */
		if (++dupacks_ == 1 ) {
			printf("%f, Fast retransmit @ ack %d\n", Scheduler::instance().clock(), last_ack_ );
			output(last_ack_ + 1, TCP_REASON_DUPACK);       // from top
		}
	}

	if (app_) app_->recv_ack(pkt); /* ANIRUDH: Callback to app */
	Packet::free(pkt);
}

void RationalTcpAgent::send_much(int force, int reason, int maxburst)
{
	int win = window();

	//assert (burstsnd_timer_.status() != TIMER_PENDING);
	if (burstsnd_timer_.status() == TIMER_PENDING ) {
		return;
	}

	/* We don't need a while, an if will do */
	/* Old code */
///	if (t_seqno_ <= highest_ack_ + win && t_seqno_ < curseq_) {
///		output(t_seqno_, reason);
///		t_seqno_++;
///	}

	/* New code */
	auto num_lost_or_received = (largest_acked_ts_ != -1.0) ? transmission_map_.at( largest_acked_ts_ ).second : 0.0;
	if (transmission_map_.size() < num_lost_or_received + win and t_seqno_ < curseq_ ) {
		output(t_seqno_, reason);
		t_seqno_ ++ ;
	}
	send_helper(maxburst);
}

void RationalTcpAgent::output( int seqno, int reason )
{
	/* Maintain a count of all transmitted packets */
	static unsigned int transmitted_pkts = 0;

	/* Cannot have two transmissions at the same time */
	assert( transmission_map_.find( Scheduler::instance().clock() ) == transmission_map_.end() );
	transmission_map_[ Scheduler::instance().clock() ] = make_pair( seqno, ++transmitted_pkts );
	assert( transmission_map_.size() == transmitted_pkts );
	_last_send_time = Scheduler::instance().clock();
	TcpAgent::output( seqno, reason );
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
RationalTcpAgent::update_memory( const RemyPacket packet, const unsigned int flow_id )
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
