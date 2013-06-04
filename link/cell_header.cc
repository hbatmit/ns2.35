#include "link/cell_header.h"

/* Static member of a struct needs to be initialized */
int hdr_cellular::offset_;

static class CellularHeaderClass : public PacketHeaderClass {
 public:
  CellularHeaderClass() : PacketHeaderClass("PacketHeader/CELLULAR", sizeof(hdr_cellular)) {
     bind_offset(&hdr_cellular::offset_);
  }
} class_cellularhdr;

void hdr_cellular::fill_in(Packet* p,
                           int slice_size,
                           double slice_size_bits,
                           bool last_fragment,
                           packet_t tunneled_type,
                           int original_size) {
  /* Fill in hdr_cmn fields */
  /* Shorten the packet so that txtime() works correctly */
  hdr_cmn::access(p)->size() = slice_size;

  /* Change the packet type so that Classifier::recv() works */
  hdr_cmn::access(p)->ptype() = PT_CELLULAR;

  /* Now fill in hdr_cellular fields required for reassembly */
  /* Is this the last fragment ? For classifier::recv() */
  hdr_cellular::access(p)->last_fragment_ = last_fragment;

  /* What is the tunneled type ? For classifier::recv() */
  hdr_cellular::access(p)->tunneled_type_ = tunneled_type;

  /* What was the original packet size ? For classifier::recv() */
  hdr_cellular::access(p)->original_size_ = original_size;

  /* What was the precise size of the slice in bits */
  hdr_cellular::access(p)->size_in_bits_  = slice_size_bits;
}

Packet* hdr_cellular::slice(Packet *p, int slice_bytes, double sliced_bits) {
  /* Slice packet */
  Packet* sliced_pkt = p->copy();

  /* Find tunneled_type and original_size */
  packet_t tunneled_type;
  int original_size;
  if (hdr_cmn::access(p)->ptype() == PT_CELLULAR) { 
    /* It's already been split up, so look "recursively" */
    tunneled_type = hdr_cellular::access(p)->tunneled_type_;
    original_size = hdr_cellular::access(p)->original_size_;

  } else {
    /* Look at the hdr_common fields */
    tunneled_type = hdr_cmn::access(p)->ptype();
    original_size = hdr_cmn::access(p)->size();
  }

  /* Fill in */
  hdr_cellular::fill_in(sliced_pkt, slice_bytes, sliced_bits, false, tunneled_type, original_size);
  return sliced_pkt;
}
