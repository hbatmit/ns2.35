
/*
 * dewp.cc
 * Copyright (C) 1999 by the University of Southern California
 * $Id: dewp.cc,v 1.3 2010/03/08 05:54:49 tom_henderson Exp $
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

#include "ip.h"
#include "tcp.h"
#include "tcp-full.h"
#include "random.h"

#include "dewp.h"
BPEntry *DEWPPolicy::bport_list = NULL;
// Initialize parameters
double DEWPPolicy::dt_inv_ = DT_INV;
double DEWPPolicy::beta = 0.5;
double DEWPPolicy::alpha = 0.125;
// num per second
int DEWPPolicy::anum_th = 50;

// EW Policy: deal with queueing stuffs.
//Constructor.  
DEWPPolicy::DEWPPolicy() : Policy() {
  // Initialize detectors
  cdewp = NULL;

  bport_list = new BPEntry[P_LEN];
  for (int i = 0; i < P_LEN; i++) {
    dport_list[i] = 0;
    bport_list[i].s = bport_list[i].b = 0;
    bport_list[i].anum = bport_list[i].last_anum = bport_list[i].avg_anum = 0;
    bport_list[i].last_time = 0;
    bport_list[i].aset = NULL;
  }
}

//Deconstructor.
DEWPPolicy::~DEWPPolicy(){
  if (cdewp)
    free(cdewp);

  for (int i = 0; i < P_LEN; i++) {
    if (bport_list[i].aset) {
      AddrEntry * p = bport_list[i].aset;
      AddrEntry * q;
      while (p) {
        q = p->next;
        free(p);
        p = q;
      }
    }
  }
}

// Initialize the DEWP parameters
void DEWPPolicy::init(double dt_inv) {
  dt_inv_ = dt_inv;
  //printf("%f\n", dt_inv_);
}

// DEWP meter: do nothing.
//  measurement is done in policer: we need to know whether the packet is
//    dropped or not.
void DEWPPolicy::applyMeter(policyTableEntry *, Packet *pkt) {
  hdr_ip* iph = hdr_ip::access(pkt);

  dport_list[iph->dport()] = Scheduler::instance().clock();

  return;
}

// DEWP Policer
//  1. do measurement: P: both arrival and departure; B: only departure
//  2. make packet drop decisions
int DEWPPolicy::applyPolicer(policyTableEntry *, policerTableEntry *policer, Packet * pkt) {
  //printf("enter applyPolicer ");

  // can't count/penalize ACKs:
  //   with resp: may cause inaccurate calculation with TSW(??)
  //   with req:  may cause resp retransmission.
  // just pass them through
  hdr_ip* iph = hdr_ip::access(pkt);
  detect(pkt);
  if (bport_list[iph->dport()].b) {
    //printf("downgrade!\n");	
    return(policer->downgrade1);
  } else {
    //printf("initial!\n");	
    return(policer->initialCodePt);
  }
}

// detect if there is alarm triggered
void DEWPPolicy::detect(Packet *pkt) {
  // it is not for outbound traffic
  if (!cdewp)
    return;

  // get the current time
  now = Scheduler::instance().clock();

  // get IP header
  hdr_ip* iph = hdr_ip::access(pkt);
  int dport = iph->dport();
  unsigned int daddr = iph->daddr();

  // use dport matching to find suspects
  if (dport_list[dport] > 0 && (cdewp->dport_list[dport]) > 0 &&
      now - dport_list[dport]< dt_inv_ &&
      now - cdewp->dport_list[dport] < dt_inv_) {
	if (bport_list[dport].s == 0) {
      printf("S %.2f %d\n", now, dport);
      bport_list[dport].s = 1;
    }
  }

  // count outbound traffic only
  if (bport_list[dport].s == 1 && daddr > 0) {
    AddrEntry * p = bport_list[dport].aset; 
    
    while (p) {
      if (p->addr == daddr) {
        break;
      }
      p = p->next;
    }
    
    if (!p) {
      AddrEntry * new_addr = new AddrEntry;
      
      new_addr->addr = daddr;
      new_addr->next = NULL;
      
      if (bport_list[dport].aset) {
        new_addr->next = bport_list[dport].aset;
      };
      bport_list[dport].aset = new_addr;
    }
    bport_list[dport].anum++;
   
    /* debug purpose only
    p = bport_list[dport].aset;
    printf("[%d] ", dport);
    while (p) {
      printf("%u ", p->addr);
      p = p->next;
    }
    printf("\n");
    */

    if (now - bport_list[dport].last_time > dt_inv_) {
          //printf("DT %f %d %d %d\n", now, dport, bport_list[dport].anum, bport_list[dport].avg_anum);
      if (bport_list[dport].anum > anum_th * dt_inv_ &&
          bport_list[dport].avg_anum > 0 &&
          bport_list[dport].anum > (1 + beta) * bport_list[dport].avg_anum) {
        if (bport_list[dport].b == 0) {
          bport_list[dport].b = 1;
          printf("B %.2f %d %d %d\n", now, dport, bport_list[dport].anum, bport_list[dport].avg_anum);
        }
      }
      
      bport_list[dport].avg_anum = (int) (alpha * bport_list[dport].avg_anum + 
                                       (1 - alpha) * bport_list[dport].anum);
      if (bport_list[dport].avg_anum == 0 && bport_list[dport].anum > 0)
      	bport_list[dport].avg_anum = bport_list[dport].anum;
      bport_list[dport].last_anum = bport_list[dport].anum;
      bport_list[dport].anum = 0;
      bport_list[dport].last_time = now;
    }
    
  }
  
}

//  make packet drop decisions
int DEWPPolicy::dropPacket(Packet *) {
 
  return(0);
}

// couple DEWP detector
void DEWPPolicy::couple(DEWPPolicy *ewpc) {
  cdewp = ewpc;
}

// End of DEWP
