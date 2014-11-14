#include <cassert>

#include "pause/pause_header.h"

/* Static member of a struct needs to be initialized */
int hdr_pause::offset_;

static class PauseHeaderClass : public PacketHeaderClass {
 public:
  PauseHeaderClass() : PacketHeaderClass("PacketHeader/PAUSE", sizeof(hdr_pause)) {
     bind_offset(&hdr_pause::offset_);
  }
} class_pausehdr;

void hdr_pause::fill_in(Packet* p,
                        const std::vector<uint16_t> & s_class_pause_durations,
                        const std::vector<bool> & s_class_enable_vector) {
  /* Fill in hdr_cmn fields */
  /* Size of Ethernet Control Frame so that txtime() works correctly */
  hdr_cmn::access(p)->size() = ETH_CTRL_FRAME_SIZE;

  /* Change the packet type so that Classifier::recv() works */
  hdr_cmn::access(p)->ptype() = PT_PAUSE;

  /* Now fill in hdr_pause fields */
  assert(s_class_pause_durations.size() == NUM_ETH_CLASS);
  assert(s_class_enable_vector.size() == NUM_ETH_CLASS);

  /* Fill in 802.1Qbb ON/OFF durations for each of the 8 priorities */
  for (uint8_t i = 0; i < NUM_ETH_CLASS; i++) {
      hdr_pause::access(p)->class_pause_durations_[i] = s_class_pause_durations.at(i);
      hdr_pause::access(p)->class_enable_vector_[i] = s_class_enable_vector.at(i);
  }
}
