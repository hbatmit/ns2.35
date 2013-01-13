
/*
 * flooding.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: flooding.h,v 1.4 2005/08/25 18:58:03 johnh Exp $
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
/* flooding.h : Chalermek Intanagonwiwat (USC/ISI)  08/16/99    */
/****************************************************************/

// Share api with diffusion and omnicient multicast
// Using diffusion packet header


#ifndef ns_flooding_h
#define ns_flooding_h

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


#define THIS_NODE             here_.addr_
#define JITTER                0.11       // (sec) to jitter broadcast

#define SEND_MESSAGE(x,y,z)  send_to_dmux(prepare_message(x,y,z), 0)


// Flooding Entry

class Flooding_Entry {
 public:
  Agent_List *source;
  Agent_List *sink;

  Flooding_Entry() { 
    source       = NULL;
    sink         = NULL;
  }

 void reset();
 void clear_agentlist(Agent_List *);
};


// Flooding Agent

class FloodingAgent : public Agent {
 public:
  FloodingAgent();
  int command(int argc, const char*const* argv);
  void recv(Packet*, Handler*);

  Flooding_Entry routing_table[MAX_DATA_TYPE];

 protected:
  int pk_count;
  Pkt_Hash_Table PktTable;

  Node *node;
  Trace *tracetarget;       // Trace Target
  NsObject *ll;  
  NsObject *port_dmux;

  inline void send_to_dmux(Packet *pkt, Handler *h) { 
    port_dmux->recv(pkt, h); 
  }

  void Terminate();
  void reset();
  void ConsiderNew(Packet *pkt);
 
  Packet *create_packet();
  Packet *prepare_message(unsigned int dtype, ns_addr_t to_addr, int msg_type);

  void DataForSink(Packet *pkt);
  void StopSource();
  void MACprepare(Packet *pkt);
  void MACsend(Packet *pkt, Time delay=0);

  void trace(char *fmt,...);

};



#endif




