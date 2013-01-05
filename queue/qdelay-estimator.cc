#include "qdelay-estimator.hh"
#include <assert.h>
#include <stdio.h>

QdelayEstimator::QdelayEstimator( const std::queue<Packet> & pkts, uint64_t service_time) :
	_pkts( pkts),
	_service_time( service_time ),
	_delays( std::vector<uint64_t> () )
{
	assert( !_pkts.empty() );
}

std::vector<uint64_t> QdelayEstimator::estimate_delays( uint64_t current_tick )
{
	/* Base case(W_0) in Lindley's recurrence */
	Packet hol_pkt = _pkts.front();
	_pkts.pop();
	int64_t current_delay = current_tick - hol_pkt._tick + _service_time;
	_delays.push_back( current_delay );
	Packet previous_pkt = hol_pkt;
	fprintf( stderr, "Lindley : seqnum %lu delay %ld\n", hol_pkt._seq_num, current_delay);

	/* Apply Lindley's recurrence using constant service time */
	while ( !_pkts.empty() ) {
		Packet current_pkt = _pkts.front();
		_pkts.pop();
		int64_t inter_arrival_time = current_pkt._tick - previous_pkt._tick;
		assert( inter_arrival_time >= 0 );
		current_delay = std::max( (int64_t) (current_delay + _service_time - inter_arrival_time), (int64_t) 0 );
		assert( current_delay >= 0 );
		_delays.push_back( current_delay );
		fprintf( stderr, "Lindley : seqnum %lu delay %ld\n", current_pkt._seq_num, current_delay);
		previous_pkt = current_pkt ;
		}

	/* return delays */
	return _delays;
}
