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

#include "tcp-rational.h"

RationalTcpAgent::RationalTcpAgent()
	: _whiskers( NULL ),
	  _memory(),
	  _intersend_time( 0.0 ),
	  _last_send_time( 0 ),
	  count_bytes_acked_( 0 )
{
	bind_bool("count_bytes_acked_", &count_bytes_acked_);

	/* get whisker filename */
	const char *filename = getenv( "WHISKERS" );
	if ( !filename ) {
		fprintf( stderr, "RemyTCP: Missing WHISKERS environment variable.\n" );
		throw 1;
	}

	/* open file */
	int fd = open( filename, O_RDONLY );
	if ( fd < 0 ) {
		perror( "open" );
		throw 1;
	}

	/* parse whisker definition */
	RemyBuffers::WhiskerTree tree;
	if ( !tree.ParseFromFileDescriptor( fd ) ) {
		fprintf( stderr, "RemyTCP: Could not parse whiskers in \"%s\".\n", filename );
		throw 1;
	}

	/* close file */
	if ( ::close( fd ) < 0 ) {
		perror( "close" );
		throw 1;
	}

	/* store whiskers */
	_whiskers = new WhiskerTree( tree );
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
	_memory.reset();
	cwnd_ = 0;
	update_cwnd_and_pacing();
	return cwnd_;
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

	/* schedule wakeup */
	if ( t_seqno_ <= highest_ack_ + window() && t_seqno_ < curseq_ ) {
		const double now( Scheduler::instance().clock() );
		const double time_since_last_send( now - _last_send_time );
		const double wait_time( _intersend_time - time_since_last_send );
		if ( wait_time <= 0 ) {
			//			burstsnd_timer_.resched( 0 );
			return;
		} else {
			burstsnd_timer_.resched( wait_time );
		}
	}
}

/* 
 * Connection has been idle for some time. 
 */
void
RationalTcpAgent::send_idle_helper() 
{
	const double now( Scheduler::instance().clock() );

	if ( now - _last_send_time > 0.2 ) {
		/* timeout */
		initial_window();
	}

	/* we want to pace each packet */
	maxburst_ = 1;

	const double time_since_last_send( now - _last_send_time );
	const double wait_time( _intersend_time - time_since_last_send );

	if ( wait_time <= .00000001 ) {
		return;
	} else {
		burstsnd_timer_.resched( wait_time );
	}
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

	int timestep = 1000;

	update_memory( RemyPacket( timestep * tcph->ts_echo(), timestep * now ) );
	update_cwnd_and_pacing();
	/* if the connection is done, call finish() */
	if ((highest_ack_ >= curseq_-1) && !closed_) {
		closed_ = 1;
		finish();
	}
}

void 
RationalTcpAgent::update_memory( const RemyPacket packet )
{
	std::vector< RemyPacket > packets( 1, packet );
	_memory.packets_received( packets );
}

void
RationalTcpAgent::update_cwnd_and_pacing( void )
{
	const Whisker & current_whisker( _whiskers->use_whisker( _memory ) );

	unsigned int new_cwnd = current_whisker.window( (unsigned int)cwnd_ );

	if ( new_cwnd > 16384 ) {
		new_cwnd = 16384;
	}

	cwnd_ = new_cwnd;
	double old_intersend_time = _intersend_time;

	double timestep_inverse = .001;

	_intersend_time = timestep_inverse * current_whisker.intersend();
	double _print_intersend = _intersend_time;

	const double now( Scheduler::instance().clock() );
	const double time_since_last_send( now - _last_send_time );
	const double wait_time( _intersend_time - time_since_last_send );
	const double old_wait_time( old_intersend_time - time_since_last_send );

	if (tracewhisk_) {
		fprintf( stderr, "%g: %s whisker %s newcwnd: %u newintersend: %f\n", now, _memory.str().c_str(), current_whisker.str().c_str(), new_cwnd, _print_intersend );
	}

	if ( wait_time < old_wait_time ) {
		if ( wait_time <= 0 ) {
			burstsnd_timer_.resched( 0 );
		} else {
			burstsnd_timer_.resched( wait_time );
		}
	}
}

void
RationalTcpAgent::timeout_nonrtx( int tno )
{
	if ( tno == TCP_TIMER_DELSND ) {
		send_much( 1, TCP_REASON_TIMEOUT, maxburst_ );
	} else if ( tno == TCP_TIMER_BURSTSND ) {
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
