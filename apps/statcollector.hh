#ifndef STATCOLLECTOR_HH_
#define STATCOLLECTOR_HH_

class StatCollector {
 public:
  StatCollector();
  void add_sample(Packet *pkt);
  void output_stats(double on_duration, uint32_t flow_id,
                    uint32_t payload_size);

 private:
  uint32_t num_samples_;
  double   cumulative_rtt_;
  uint32_t cumulative_pkts_;
  int32_t  last_ack_;
};

#endif // STATCOLLECTOR_HH_
