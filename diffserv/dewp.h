
/*
 * dewp.h
 * Copyright (C) 1999 by the University of Southern California
 * $Id: dewp.h,v 1.2 2005/08/25 18:58:03 johnh Exp $
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

//
// dewp.h (Early worm propagation detection)
//   by Xuan Chen (xuanc@isi.edu), USC/ISI

#ifndef DEWP_H
#define DEWP_H

#include "packet.h"
#include "dsPolicy.h"

#define DT_INV 2
#define DB_INV DT_INV
#define P_LEN 65536 

// list to store addresses
struct AddrEntry {
  unsigned int addr;
  struct AddrEntry *next;
};

// List for blocked port
struct BPEntry {
  int s, b;
  int anum, last_anum, avg_anum;
  double last_time;
  struct AddrEntry *aset;
};

// Eearly worm propagation detection policy
class DEWPPolicy : public Policy {
 public:
  DEWPPolicy();
  ~DEWPPolicy();

  void init(double);
  
  // Metering and policing methods:
  void applyMeter(policyTableEntry *policy, Packet *pkt);
  int applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *pkt);

  //  make packet drop decisions
  int dropPacket(Packet *);
  // detect if there is an alarm triggered
  void detect(Packet *pkt);
  
  // couple in- and out-bound queues
  void couple(DEWPPolicy *);

  //protected:
  DEWPPolicy *cdewp;

  double dport_list[P_LEN];
  static BPEntry *bport_list;

 private:
  // The current time
  double now;

  // commom paramters
  // detection interval
  static double dt_inv_;
  // change tolerance
  static double beta;
  // gain
  static double alpha;
  // absolute threshold to filter out some small values (num/s)
  static int anum_th;
};

//int *DEWPPolicy::bport_list = NULL;

#endif
