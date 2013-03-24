#ifndef LINK_CELL_HEADER_H
#define LINK_CELL_HEADER_H

#include "common/packet.h"

struct hdr_cellular {
  bool last_fragment_;
  packet_t tunneled_type_;
  int original_size_;

  /* Packet header access functions */
  static int offset_;
  inline static int& offset() { return offset_; }
  inline static hdr_cellular* access(const Packet* p) { return (hdr_cellular*)p->access(offset_);}

  /* Fill in fields */
  static void fill_in(Packet* p,
                      int slice_size,
                      bool last_fragment,
                      packet_t tunneled_type,
                      int original_size);

  /* Slice packet */
  static Packet* slice(Packet *p, int slice_bytes);
};

#endif
