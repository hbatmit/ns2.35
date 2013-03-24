#include "link/cell_header.h"

/* Static member of a struct needs to be initialized */
int hdr_cellular::offset_;

static class CellularHeaderClass : public PacketHeaderClass {
 public:
  CellularHeaderClass() : PacketHeaderClass("PacketHeader/CELLULAR", sizeof(hdr_cellular)) {
     bind_offset(&hdr_cellular::offset_);
  }
} class_cellularhdr;
