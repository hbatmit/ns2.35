#ifndef LINK_CELL_HEADER_H
#define LINK_CELL_HEADER_H

#include "tcp/tcp.h"

struct hdr_cellular {
  bool last_fragment_;
 
  /* Packet header access functions */
  static int offset_;
  inline static int& offset() { return offset_; }
  inline static hdr_cellular* access(const Packet* p) { return (hdr_cellular*)p->access(offset_);}

};

static class CellularHeaderClass : public PacketHeaderClass {
 public:
  CellularHeaderClass() : PacketHeaderClass("PacketHeader/CELLULAR", sizeof(hdr_cellular)) {
     bind_offset(&hdr_cellular::offset_);
  }
} class_cellularhdr;

/* Static member of a struct needs to be initialized */
int hdr_cellular::offset_;

#endif
