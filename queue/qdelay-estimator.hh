#ifndef QDELAY_ESTIMATOR_HH
#define QDELAY_ESTIMATOR_HH

#include "../packet.hh"
#include<queue>

/* Estimate future delays of packets in queue */
class QdelayEstimator
{
private:
	std::queue<Packet> _pkts;
	uint64_t _service_time;
	std::vector<uint64_t> _delays;

public:
	QdelayEstimator( const std::queue<Packet> & pkts, uint64_t service_time );
	
	std::vector<uint64_t> estimate_delays( uint64_t current_tick );
};

#endif
