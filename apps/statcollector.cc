#include <assert.h>
#include "packet.h"
#include "statcollector.hh"
#include "tcp.h"

StatCollector::StatCollector()
    : num_samples_(0),
      cumulative_rtt_(0.0),
      cumulative_pkts_(0),
      last_ack_(-1) {}
 
void StatCollector::add_sample(Packet* pkt) {
  /* Get headers */
  hdr_cmn *cmn_hdr = hdr_cmn::access(pkt);
  hdr_tcp *tcph = hdr_tcp::access(pkt);

  /* Assert that packet is an ACK */
  assert (cmn_hdr->ptype() == PT_ACK);

  /* Get size of packet, iff it's a new ack */
  int32_t current_ack = tcph->seqno();
  assert (current_ack >= last_ack_); /* ACKs are cumulative */
  if (current_ack > last_ack_) {
    cumulative_pkts_ += (current_ack - last_ack_);
    last_ack_ = current_ack;
  }

  /* Get RTT */
  cumulative_rtt_ += Scheduler::instance().clock() - tcph->ts_echo();
  num_samples_++;
}

void StatCollector::output_stats(double on_duration, uint32_t flow_id,
                                 uint32_t payload_size) {
  assert(on_duration != 0);
//  assert(num_samples_ != 0);
  fprintf(stderr,
          "%u: tp=%f mbps, del=%f ms, on=%f secs, samples=%d, inorder=%d\n",
          flow_id,
          (cumulative_pkts_ * 8.0 * payload_size)/ (1.0e6 * on_duration),
          (num_samples_ != 0) ? (cumulative_rtt_ * 1000.0) / num_samples_ : -1.0,
          on_duration,
          num_samples_,
          cumulative_pkts_);
}
