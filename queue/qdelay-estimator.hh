#ifndef QDELAY_ESTIMATOR_HH
#define QDELAY_ESTIMATOR_HH

#include<queue>
#include "common/packet.h"
#include "queue/queue.h"

/* Estimate future delays of packets in queue */
class QdelayEstimator
{
private:
	const double get_ts(const Packet* p);
	const uint32_t get_seq(const Packet* p);
	const double get_service_time(const Packet* p);

	std::queue<const Packet*> _pkts;
	double _link_rate;
	std::vector<double> _delays;

public:
	QdelayEstimator(PacketQueue* pkts, double link_rate);
	
	std::vector<double> estimate_delays(double now);
};

#endif
