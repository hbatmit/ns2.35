
/*
 * pgm.h
 * Copyright (C) 2001 by the University of Southern California
 * $Id: pgm.h,v 1.3 2005/08/25 18:58:10 johnh Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */

/*
 * Pragmatic General Multicast (PGM), Reliable Multicast
 *
 * pgm.h
 *
 * This holds the packet header structures, and packet type constants for
 * the PGM implementation.
 *
 * Ryan S. Barnett, 2001
 * rbarnett@catarina.usc.edu
 */


#ifndef PGM_H
#define PGM_H

/*
 * Types for PT_PGM packets.
 */

#define PGM_SPM         0x00
#define PGM_ODATA       0x04
#define PGM_RDATA       0x05
#define PGM_NAK         0x08
#define PGM_NCF         0x0A

// ************************************************************
// Header included with all PGM packets.
// ************************************************************
struct hdr_pgm {
  int type_; // PGM sub-type.

  ns_addr_t tsi_; // The transport session ID (source addr and source port).

  // The sequence number for this PGM packet. It can have differing meaning
  // depending on whether the packet is SPM, ODATA, RDATA, NAK, or NCF.
  int seqno_;

  static int offset_; // Used to locate this packet header.
  inline static int& offset() { return offset_; }
  inline static hdr_pgm* access(const Packet *p) {
    return (hdr_pgm *)p->access(offset_);
  }

};

// ************************************************************
// Header included with PGM SPM-type packets.
// ************************************************************
struct hdr_pgm_spm {
  ns_addr_t spm_path_; // Address of upstream PGM node.

  static int offset_; // Used to locate this packet header.
  inline static int& offset() { return offset_; }
  inline static hdr_pgm_spm* access(const Packet *p) {
    return (hdr_pgm_spm *)p->access(offset_);
  }

};

// ************************************************************
// Header included with PGM NAK-type packets.
// ************************************************************
struct hdr_pgm_nak {
  ns_addr_t source_; // Address of ODATA source.
  ns_addr_t group_; // Address for the multicast group.

  static int offset_; // Used to locate this packet header.
  inline static int& offset() { return offset_; }
  inline static hdr_pgm_nak* access(const Packet *p) {
    return (hdr_pgm_nak *)p->access(offset_);
  }

};

// Quick reference macros to access the packet headers.
#define HDR_PGM(p) (hdr_pgm::access(p))
#define HDR_PGM_SPM(p) (hdr_pgm_spm::access(p))
#define HDR_PGM_NAK(p) (hdr_pgm_nak::access(p))

#endif // PGM_H

