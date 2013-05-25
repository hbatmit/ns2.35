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
	  _internal_clock( Scheduler::instance().clock() ),
	  _last_wakeup( _internal_clock ),
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
		perror( "open WHISKERS" );
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
	update_cwnd_and_pacing();
	_internal_clock = Scheduler::instance().clock();

	/* start timer */
	burstsnd_timer_.resched( 0 );

	return cwnd_;
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

	/*
	fprintf( stderr, "got ack @ %f (acking %d sent @ %f)\n", now * 1000,
		 tcph->seqno(), 1000 * tcph->ts_echo() );
	*/

	newack(pkt);		// updates RTT to set RTO properly, etc.
	maxseq_ = ::max(maxseq_, highest_ack_);
	update_memory( RemyPacket( 1000 * tcph->ts_echo(), 1000 * now ) );
	update_cwnd_and_pacing();
	/* if the connection is done, call finish() */
	if ((highest_ack_ >= curseq_-1) && !closed_) {
		closed_ = 1;
		finish();
	}

	if ( burstsnd_timer_.status() != TIMER_PENDING ) {
		fprintf( stderr, "Starting timer at %f...\n", now * 1000 );
		burstsnd_timer_.sched( 0 );
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

	cwnd_ = 0; /* don't let it send unless we're doing the sending */

	unsigned int new_cwnd = current_whisker.window( (unsigned int)_the_window );

	if ( new_cwnd > INT_MAX/2 ) {
		new_cwnd = INT_MAX/2;
	}

	_the_window = new_cwnd;
	_intersend_time = .001 * current_whisker.intersend();
	double _print_intersend = _intersend_time;
	if (tracewhisk_) {
		fprintf( stderr, "memory: %s falls into whisker %s\n", _memory.str().c_str(), current_whisker.str().c_str() );
		fprintf( stderr, "\t=> cwnd now %u, intersend_time now %f\n", new_cwnd, _print_intersend );
	}
}

void
RationalTcpAgent::timeout_nonrtx( int tno )
{
	if ( tno == TCP_TIMER_BURSTSND ) {
		/* tick */
		double now = Scheduler::instance().clock();

		if ( now - _last_wakeup > .0011 ) {
			initial_window();
			if ( !resetting ) {
				//				fprintf( stderr, "Reset.\n" );
				resetting = true;
			}
		}

		_last_wakeup = now;

		if ( _the_window == 0 ) {
			update_cwnd_and_pacing();
		}
		
		if ( t_seqno_ >= curseq_ ) {
			initial_window();
			if ( !resetting ) {
				//				fprintf( stderr, "Reset.\n" );
				resetting = true;
			}
		} else {
			if ( resetting ) {
				//				fprintf( stderr, "Starting!\n" );
				initial_window();
				resetting = false;
			}
			int realwnd = (_the_window < wnd_ ? _the_window : (int)wnd_);
			while ( t_seqno_ <= highest_ack_ + realwnd ) {
				if ( _internal_clock > now + .001 ) {
					break;
				}

				/*
				fprintf( stderr, "Sending @ %f, intersend = %f, internal_clock = %f\n",
					 now * 1000, _intersend_time * 1000, _internal_clock * 1000 );
				*/

				cwnd_ = 1000000;
				send_much( 1, TCP_REASON_TIMEOUT, 1 );
				cwnd_ = 0;
				_internal_clock += _intersend_time;
			}
		}

		burstsnd_timer_.resched( 0.001 );
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
