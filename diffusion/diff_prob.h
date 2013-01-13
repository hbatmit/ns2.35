
/*
 * diff_prob.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: diff_prob.h,v 1.9 2005/08/25 18:58:03 johnh Exp $
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

/****************************************************************/
/* diff_prob.h : Chalermek Intanagonwiwat (USC/ISI)  08/16/99   */
/****************************************************************/

// Important Note: Work still in progress !!! Major improvement is needed.

#ifndef ns_diff_prob_h
#define ns_diff_prob_h

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <float.h>
#include <stdlib.h>

#include <tcl.h>

#include "diff_header.h"
#include "agent.h"
#include "tclcl.h"
#include "ip.h"
#include "config.h"
#include "packet.h"
#include "trace.h"
#include "random.h"
#include "classifier.h"
#include "node.h"
#include "iflist.h"
#include "hash_table.h"
#include "arp.h"
#include "mac.h"
#include "ll.h"
#include "dsr/path.h"
#include "routing_table.h"
#include "diffusion.h"


//#define DEBUG_PROB

#define BACKTRACK_            false
#define ENERGY_CHECK          0.05         // (sec) between energy checks
#define INTEREST_DELAY        0.05         // (sec) bw receive and forward
#define MAX_REINFORCE_COUNTER 10           // (pkts) bw pos reinf

class DiffusionProb;

class EnergyTimer : public TimerHandler {
public:
	EnergyTimer(DiffusionProb *a, Node *b) : TimerHandler() { 
		a_ = a; 
		node_ = b;
		init_eng_ = node_->energy_model()->energy();
		threshold_ = init_eng_ / 2;
	}
	virtual void expire(Event *e);
protected:
	DiffusionProb *a_;
	Node *node_;
	double init_eng_;
	double threshold_;
};


class InterestTimer : public TimerHandler {
public:
  InterestTimer(DiffusionProb *a, Pkt_Hash_Entry *hashPtr, Packet *pkt) : 
    TimerHandler() 
  { 
      a_ = a; 
      hashPtr_ = hashPtr;
      pkt_ = pkt;
  }

  virtual ~InterestTimer() {
    if (pkt_ != NULL) 
      Packet::free(pkt_);
  }

  virtual void expire(Event *e);
protected:
  DiffusionProb *a_;
  Pkt_Hash_Entry *hashPtr_;
  Packet *pkt_;
};



class DiffusionProb : public DiffusionAgent {
 public:
  DiffusionProb();
  void recv(Packet*, Handler*);

 protected:

  int num_neg_bcast_send;
  int num_neg_bcast_rcv;

  EnergyTimer *energy_timer;
  bool is_low_power;

  void Start();
  void consider_old(Packet *);
  void consider_new(Packet *);
  void add_outlist(unsigned int, From_List *);
  void data_request_all(unsigned int dtype);

  void CreateIOList(Pkt_Hash_Entry *, unsigned int);
  void UpdateIOList(From_List *, unsigned int);
  void Print_IOlist();

  void CalGradient(unsigned int);
  void IncGradient(unsigned int, ns_addr_t);
  void DecGradient(unsigned int, ns_addr_t);

  void ForwardData(Packet *);
  void ForwardTxFailed(Packet *);
  void ReTxData(Packet *);

  void GenPosReinf(unsigned int);
  void FwdPosReinf(unsigned int, Packet *);
  void InterfaceDown(int, ns_addr_t);
  void SendInhibit(int);
  void SendNegReinf();

  void InterestPropagate(Packet *pkt, Pkt_Hash_Entry *hashPtr);
  void xmitFailed(Packet *pkt);

  friend class InterestTimer;
  friend class EnergyTimer;
};

#endif
