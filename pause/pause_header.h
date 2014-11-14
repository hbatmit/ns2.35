#ifndef PAUSE_HEADER_H
#define PAUSE_HEADER_H

#include <vector>
#include "common/packet.h"

struct hdr_pause {
  static const int ETH_CTRL_FRAME_SIZE = 64;
  static const size_t NUM_ETH_CLASS = 8;

  uint16_t class_pause_durations_[NUM_ETH_CLASS];
  bool class_enable_vector_[NUM_ETH_CLASS];

  /* Packet header access functions */
  static int offset_;
  inline static int& offset() { return offset_; }
  inline static hdr_pause* access(const Packet* p) {
    return reinterpret_cast<hdr_pause*>(p->access(offset_));
  }

  /* Default ctor */
  hdr_pause() = delete;

  /* Fill in fields */
  static void fill_in(Packet* p,
                      const std::vector<uint16_t> & s_class_pause_durations,
                      const std::vector<bool> & s_class_enable_vector);
};
  
#endif
