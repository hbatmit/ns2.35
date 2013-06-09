#ifndef QDELAY_ESTIMATOR_HH
#define QDELAY_ESTIMATOR_HH

#include<queue>
#include "common/packet.h"
#include "queue/queue.h"

/* Estimate future delays of packets in queue */
class QdelayEstimator
{
private:
	double get_ts(Packet p);
        uint32_t get_seq(Packet p);
        double get_service_time(Packet p);

	std::queue<Packet> _pkts;
	double _link_rate;
	std::vector<double> _delays;

public:
	QdelayEstimator(PacketQueue* pkts, double link_rate);
	
	std::vector<double> estimate_delays(double now);
};

#endif
