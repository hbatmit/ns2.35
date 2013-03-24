#ifndef LINK_CELL_HEADER_H
#define LINK_CELL_HEADER_H

#include "common/packet.h"

struct hdr_cellular {
  bool last_fragment_;
  packet_t tunneled_type_;

  /* Packet header access functions */
  static int offset_;
  inline static int& offset() { return offset_; }
  inline static hdr_cellular* access(const Packet* p) { return (hdr_cellular*)p->access(offset_);}

};

#endif
