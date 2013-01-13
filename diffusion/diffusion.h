
/*
 * diffusion.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: diffusion.h,v 1.7 2005/08/25 18:58:03 johnh Exp $
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
/* diffusion.h : Chalermek Intanagonwiwat (USC/ISI)  08/16/99   */
/****************************************************************/

// Important Note: Work still in progress ! 

#ifndef ns_diffusion_h
#define ns_diffusion_h

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <float.h>

#include <tcl.h>
#include <stdlib.h>

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


#define THIS_NODE             here_.addr_
#define MAC_RETRY_            0          // 0: NO RETRY
                                         // 1: RETRY
                                         // But, we only have no retry now!
#define JITTER                0.11       // (sec) to jitter broadcast
#define ARP_BUFFER_CHECK      0.03       // (sec) between buffer checks
#define ARP_MAX_ATTEMPT       3          // (times)
#define ARP_BUF_SIZE          64         // (slots)

#define SEND_BUFFER_CHECK     0.03       // (sec) between buffer checks
#define SEND_TIMEOUT          30.0       // (sec) a packet can live in sendbuf
#define SEND_BUF_SIZE         64         // (slots)


#define SEND_MESSAGE(x,y,z)  send_to_dmux(prepare_message(x,y,z), 0)
#define max(a,b) (((a)<(b))?(b):(a))

class DiffusionAgent;


class ArpBufEntry {
 public:
  Time   t;                            // Insertion Time
  int    attempt;                      // number of attempts
  Packet *p;

  ArpBufEntry() { t=0; attempt=0; p=NULL; }
};


class ArpBufferTimer : public TimerHandler {
 public:
  ArpBufferTimer(DiffusionAgent *a) : TimerHandler() { a_ = a; }
  void expire(Event *e);
 protected:
  DiffusionAgent *a_;
};


class SendBufferEntry {
 public:
  Time    t;                          // Insertion Time 
  Packet *p;

  SendBufferEntry() { t=0; p=NULL; }
};


class SendBufTimer : public TimerHandler {
 public:
  SendBufTimer(DiffusionAgent *a) : TimerHandler() { a_ = a; }
  void expire(Event *e);
 protected:
  DiffusionAgent *a_;
};


class DiffusionAgent : public Agent {
 public:
  DiffusionAgent();
  int command(int argc, const char*const* argv);
  void recv(Packet*, Handler*);

  Diff_Routing_Entry  routing_table[MAX_DATA_TYPE];


 protected:

  // Variables start here.

  bool POS_REINF_;
  bool NEG_REINF_;

  int pk_count;
  int overhead;

  Pkt_Hash_Table PktTable;

  Node *node;
  Trace *tracetarget;       // Trace Target
  NsObject *ll;  
  NsObject *port_dmux;
  ARPTable *arp_table;

  ArpBufferTimer     arp_buf_timer;
  ArpBufEntry        arp_buf[ARP_BUF_SIZE];
  SendBufTimer       send_buf_timer;
  SendBufferEntry    send_buf[SEND_BUF_SIZE];

  // Methods start here.

  inline void send_to_dmux(Packet *pkt, Handler *h) { 
    port_dmux->recv(pkt, h); 
  }

  void clear_arp_buf();
  void clear_send_buf();
  void reset();
  void consider_old(Packet *);
  void consider_new(Packet *);
  void Terminate();
  virtual void Start();

  Packet *create_packet();
  Packet *prepare_message(unsigned int dtype, ns_addr_t to_addr, int msg_type);

  virtual void Print_IOlist();
  void DataForSink(Packet *);
  void StopSource();

  void MACprepare(Packet *pkt, nsaddr_t next_hop, int type, 
		  bool lk_dtct);
  void MACsend(Packet *pkt, Time delay=0);
  void xmitFailed(Packet *pkt);

  void StickPacketInArpBuffer(Packet *pkt);
  void ArpBufferCheck();
  void SendBufferCheck();
  void StickPacketInSendBuffer(Packet *p);

  void trace(char *fmt,...);

  friend void XmitFailedCallback(Packet *pkt, void *data);
  friend class ArpBufferTimer;
  friend class SendBufTimer;
};

#endif


