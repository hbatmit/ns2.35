#include "qdelay-estimator.hh"
#include <assert.h>

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
	uint64_t current_delay = current_tick - hol_pkt._tick + _service_time;
	_delays.push_back( current_delay );
	Packet previous_pkt = hol_pkt;

	/* Apply Lindley's recurrence using constant service time */
	while ( !_pkts.empty() ) {
		Packet current_pkt = _pkts.front();
		_pkts.pop();
		uint64_t inter_arrival_time = current_pkt._tick - previous_pkt._tick;
		current_delay = std::max( current_delay + _service_time - inter_arrival_time, (uint64_t) 0 );
		_delays.push_back( current_delay );
		}

	/* return delays */
	return _delays;
}
