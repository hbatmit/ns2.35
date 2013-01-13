
/*
 * diffusion.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: diffusion.cc,v 1.15 2011/10/02 22:32:34 tom_henderson Exp $
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
 * are not obligated to grant this special exception for their * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */

//
// $Header: /cvsroot/nsnam/ns-2/diffusion/diffusion.cc,v 1.15 2011/10/02 22:32:34 tom_henderson Exp $

/****************************************************************/
/* diffusion.cc : Chalermek Intanagonwiwat (USC/ISI)  05/18/99  */
/****************************************************************/

// Important Note: Work still in progress !

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
#include "diffusion.h"
#include "iflist.h"
#include "hash_table.h"
#include "arp.h"
#include "mac.h"
#include "ll.h"
#include "dsr/path.h"
#include "god.h"
#include "routing_table.h"

/* Callback helper */
void XmitFailedCallback(Packet *pkt, void *data)
{
  DiffusionAgent *agent = (DiffusionAgent *)data;  // cast of trust
  agent->xmitFailed(pkt);
}

char *MsgStr[]= {"", "INTEREST", "DATA", "DATA_READY", "DATA_REQUEST",
		"POS_REINFORCE", "NEG_REINFORCE", "INHIBIT", "TX_FAILED",
		"DATA_STOP" };

int hdr_cdiff::offset_;
static class DiffHeaderClass : public PacketHeaderClass {
public:
	DiffHeaderClass() : PacketHeaderClass("PacketHeader/Diffusion", 
					      sizeof(hdr_cdiff)) {
		bind_offset(&hdr_cdiff::offset_);
	}
} class_diffhdr;


static class DiffusionClass : public TclClass {
public:
  DiffusionClass() : TclClass("Agent/Diffusion") {}
  TclObject* create(int , const char*const* ) {
    return(new DiffusionAgent());
  }
} class_diffusion;


void ArpBufferTimer::expire(Event *e)
{
  a_->ArpBufferCheck();
  resched(ARP_BUFFER_CHECK + ARP_BUFFER_CHECK * 
	  (double) ((long) e>>5 & 0xff) /256.0);
}

void SendBufTimer::expire(Event *e)
{
  a_->SendBufferCheck();
  resched(SEND_BUFFER_CHECK + SEND_BUFFER_CHECK * (double) ((long) e>>5 & 0xff)/256.0);
}


void DiffusionAgent::DataForSink(Packet *pkt)
{
  hdr_cdiff     *dfh  = HDR_CDIFF(pkt);
  unsigned int dtype = dfh->data_type;
  Agent_List   *cur_agent;
  Packet       *cur_pkt;
  hdr_cdiff     *cur_dfh;
  hdr_ip       *cur_iph;


  // Give it to sink agents on this node. Don't care if wireless.

  for (cur_agent= (routing_table[dtype]).sink; cur_agent != NULL; 
	   cur_agent= AGENT_NEXT(cur_agent) ) {

      cur_pkt       = pkt->copy();
      cur_iph       = HDR_IP(cur_pkt);
      cur_iph->dst_ = AGT_ADDR(cur_agent);

      cur_dfh       = HDR_CDIFF(cur_pkt);
      cur_dfh->forward_agent_id = here_;
      cur_dfh->num_next = 1;
      cur_dfh->next_nodes[0] = NODE_ADDR(cur_agent);

      send_to_dmux(cur_pkt, 0);
  }
}


Packet *DiffusionAgent::prepare_message(unsigned int dtype, ns_addr_t to_addr, 
				  int msg_type)
{
  Packet *pkt;
  hdr_cdiff *dfh;
  hdr_ip *iph;

    pkt = create_packet();
    dfh = HDR_CDIFF(pkt);
    iph = HDR_IP(pkt);
    
    dfh->mess_type = msg_type;
    dfh->pk_num = pk_count;
    pk_count++;
    dfh->sender_id = here_;
    dfh->data_type = dtype;
    dfh->forward_agent_id = here_;

    dfh->ts_ = NOW;
    dfh->num_next = 1;
    dfh->next_nodes[0] = to_addr.addr_;
    
    iph->src_ = here_;
    iph->dst_ = to_addr;

    return pkt;
}


DiffusionAgent::DiffusionAgent() : Agent(PT_DIFF), arp_buf_timer(this), 
  send_buf_timer(this)
{
  POS_REINF_ = true;
  NEG_REINF_ = true;
  overhead = 0;
  pk_count = 0;
  target_ = 0;

  node = NULL;
  tracetarget = NULL;
}


void DiffusionAgent::recv(Packet* packet, Handler*)
{
  hdr_cdiff* dfh = HDR_CDIFF(packet);

  // Packet Hash Table is used to keep info about experienced pkts.

  Pkt_Hash_Entry *hashPtr= PktTable.GetHash(dfh->sender_id, dfh->pk_num);


     // Received this packet before ?

     if (hashPtr != NULL) {
       consider_old(packet);
       return;
     }

     // Never receive it before ? Put in hash table.

     PktTable.put_in_hash(dfh);

     // Take action for a new pkt.

     consider_new(packet);     
}


void DiffusionAgent::consider_old(Packet *pkt)
{
      Packet::free(pkt);        
}


void DiffusionAgent::consider_new(Packet *pkt)
{
      Packet::free(pkt);        
}


void DiffusionAgent::Terminate() 
{
#ifdef DEBUG_OUTPUT
	printf("Diffusion node %d : terminates (overhead %d)\n", 
	       THIS_NODE, overhead);  
	printf("node %d: remaining energy %f, initial energy %f\n", THIS_NODE, 
	       node->energy_model()->energy(), 
	       node->energy_model()->initialenergy() );
	Print_IOlist();
#endif
}


void DiffusionAgent::Print_IOlist()
{
}


void DiffusionAgent::Start()
{
  arp_buf_timer.sched(ARP_BUFFER_CHECK + ARP_BUFFER_CHECK * 
		      Random::uniform(1.0));	  
  send_buf_timer.sched(SEND_BUFFER_CHECK + SEND_BUFFER_CHECK *
			   Random::uniform(1.0));
}


int DiffusionAgent::command(int argc, const char*const* argv)
{  
  Tcl& tcl =  Tcl::instance();

  if (argc == 2) {

    if (strcasecmp(argv[1], "reset-state")==0) {
      
      reset();
      return TCL_OK;
    }

    if (strcasecmp(argv[1], "reset")==0) {
      
      return Agent::command(argc, argv);
    }

    if (strcasecmp(argv[1], "start")==0) {
      Start();
      return TCL_OK;
    }

    if (strcasecmp(argv[1], "stop")==0) {
      return TCL_OK;
    }

    if (strcasecmp(argv[1], "terminate")==0) {
      Terminate();
      return TCL_OK;
    }

    if (strcasecmp(argv[1], "stop-source")==0) {
      StopSource();
      return TCL_OK;
    }

    if (strcasecmp(argv[1], "enable-pos") == 0) {
      POS_REINF_ = true;
      return TCL_OK;
    }

    if (strcasecmp(argv[1], "enable-neg") == 0) {
      NEG_REINF_ = true;
      return TCL_OK;
    }


    if (strcasecmp(argv[1], "disable-pos") == 0) {
      POS_REINF_ = false;
      return TCL_OK;
    }

    if (strcasecmp(argv[1], "disable-neg") == 0) {
      NEG_REINF_ = false;
      return TCL_OK;
    }

  } else if (argc == 3) {

    if (strcasecmp(argv[1], "on-node")==0) {
      node = (Node *)tcl.lookup(argv[2]);
      return TCL_OK;
    }


    if (strcasecmp(argv[1], "add-ll") == 0) {

      TclObject *obj;

      if ( (obj = TclObject::lookup(argv[2])) == 0) {
	fprintf(stderr, "Diffusion Node: %d lookup of %s failed\n", THIS_NODE,
		argv[2]);
	return TCL_ERROR;
      }
      ll = (NsObject *) obj;

      // What a hack !!!
      arp_table = ((LL *)ll)->arp_table();
      if (arp_table == NULL) 
	return TCL_ERROR;

     return TCL_OK;
    }

    if (strcasecmp (argv[1], "tracetarget") == 0) {
      TclObject *obj;
      if ((obj = TclObject::lookup (argv[2])) == 0) {
	  fprintf (stderr, "%s: %s lookup of %s failed\n", __FILE__, argv[1],
		   argv[2]);
	  return TCL_ERROR;
      }

      tracetarget = (Trace *) obj;
      return TCL_OK;
    }

    if (strcasecmp(argv[1], "port-dmux") == 0) {

      TclObject *obj;

      if ( (obj = TclObject::lookup(argv[2])) == 0) {
	fprintf(stderr, "Diffusion Node: %d lookup of %s failed\n", THIS_NODE,
		argv[2]);
	return TCL_ERROR;
      }
      port_dmux = (NsObject *) obj;
      return TCL_OK;
    }


  } 

  return Agent::command(argc, argv);
}


void DiffusionAgent::StopSource()
{
  Agent_List *cur;

  for (int i=0; i<MAX_DATA_TYPE; i++) {
    for (cur=routing_table[i].source; cur!=NULL; cur=AGENT_NEXT(cur) ) {
      SEND_MESSAGE(i, AGT_ADDR(cur), DATA_STOP);
    }
  }
}


Packet * DiffusionAgent:: create_packet()
{
  Packet *pkt = allocpkt();

  if (pkt==NULL) return NULL;

  hdr_cmn*  cmh = HDR_CMN(pkt);
  cmh->size() = 36;

  hdr_cdiff* dfh = HDR_CDIFF(pkt);
  dfh->ts_ = NOW;
  return pkt;
}


void DiffusionAgent::MACprepare(Packet *pkt, nsaddr_t next_hop, 
				int type, bool lk_dtct)
{
  hdr_cdiff* dfh = HDR_CDIFF(pkt);
  hdr_cmn* cmh = HDR_CMN(pkt);
  hdr_ip*  iph = HDR_IP(pkt);

  dfh->forward_agent_id = here_; 
  if (type == (int) NS_AF_ILINK && next_hop == (nsaddr_t) MAC_BROADCAST) {
      cmh->xmit_failure_ = 0;
      cmh->next_hop() = MAC_BROADCAST;
      cmh->addr_type() = NS_AF_ILINK;
      cmh->direction() = hdr_cmn::DOWN;

      
      iph->src_ = here_;
      iph->dst_.addr_ = next_hop;
      iph->dst_.port_ = ROUTING_PORT;

      dfh->num_next = 1;
      dfh->next_nodes[0] = next_hop;

      return;     
  }  

  if (lk_dtct != 0) {
    cmh->xmit_failure_ = XmitFailedCallback;
    cmh->xmit_failure_data_ = (void *) this;
  }
  else {
    cmh->xmit_failure_ = 0;
  }

  cmh->direction() = hdr_cmn::DOWN;

  cmh->next_hop() = next_hop;
  cmh->addr_type() = type;  

  iph->src_ = here_;
  iph->dst_.addr_ = next_hop;
  iph->dst_.port_ = ROUTING_PORT;
  
  dfh->num_next = 1;
  dfh->next_nodes[0] = next_hop;
}


void DiffusionAgent::MACsend(Packet *pkt, Time delay)
{
  hdr_cmn*  cmh = HDR_CMN(pkt);
  hdr_cdiff* dfh = HDR_CDIFF(pkt);

  if (dfh->mess_type == DATA)
    cmh->size() = (God::instance()->data_pkt_size) + 4*(dfh->num_next - 1);
  else
    cmh->size() = 36 + 4*(dfh->num_next -1);

  Scheduler::instance().schedule(ll, pkt, delay);
}


void DiffusionAgent::xmitFailed(Packet *)
{
  // For future use, if needed.
}


void DiffusionAgent::StickPacketInArpBuffer(Packet *pkt)
{
  Time min = DBL_MAX;
  int  min_index = 0;
  int  c;

  for (c=0; c < ARP_BUF_SIZE; c++) {
    if (arp_buf[c].p == NULL) {
      arp_buf[c].t = NOW;
      arp_buf[c].attempt = 1;
      arp_buf[c].p = pkt;
      return;
    }
    else if (arp_buf[c].t < min) {
      min = arp_buf[c].t;
      min_index = c;
    }
  }

  // Before killing somebody, let him get a last chance to send.

  ARPEntry *llinfo;
  hdr_cmn*  cmh = HDR_CMN(arp_buf[min_index].p);

  llinfo= arp_table->arplookup(cmh->next_hop());

  if (llinfo == 0) {
    // printf("ARP fails. And must give up slot.\n");
      xmitFailed(arp_buf[min_index].p);
  }
  else
      MACsend(arp_buf[min_index].p, 0);


  // The new packet is taking over the slot of the dead guy.

  arp_buf[min_index].t = NOW;
  arp_buf[min_index].attempt = 1;
  arp_buf[min_index].p = pkt;
}


void DiffusionAgent::ArpBufferCheck()
{
  int c;
  ARPEntry *llinfo;
  hdr_cmn*  cmh;

  for (c = 0; c < ARP_BUF_SIZE; c++) {
    if (arp_buf[c].p == NULL)
      continue;
    
    cmh = HDR_CMN(arp_buf[c].p);
    llinfo= arp_table->arplookup(cmh->next_hop());
    if (llinfo != 0) {
      MACsend(arp_buf[c].p, 0);
      arp_buf[c].p = NULL;
      continue;}
    
    if (arp_buf[c].attempt > ARP_MAX_ATTEMPT) {
      // printf("ARP fails. Too many attempts.\n");
      xmitFailed(arp_buf[c].p);
      arp_buf[c].p = NULL;
      continue;
    }

    arp_table->arprequest(THIS_NODE, cmh->next_hop(), (LL *)ll);
    arp_buf[c].attempt ++;
  }
}


void DiffusionAgent::StickPacketInSendBuffer(Packet *p)
{
  Time min = DBL_MAX;
  int min_index = 0;
  int c;

  for (c = 0 ; c < SEND_BUF_SIZE ; c ++)
    if (send_buf[c].p  == NULL)
      {
	send_buf[c].t = NOW;
	send_buf[c].p = p;
	return;
      }
    else if (send_buf[c].t < min)
      {
	min = send_buf[c].t;
	min_index = c;
      }
  
  // Before killing somebody, you'd better give him the last chance.

  hdr_cdiff  *dfh = HDR_CDIFF(send_buf[min_index].p);
  hdr_ip    *iph = HDR_IP(send_buf[min_index].p);
  int dtype = dfh->data_type;

  PrvCurPtr RetVal = INTF_FIND(routing_table[dtype].active, iph->dst_);

  if (RetVal.cur != NULL)   
      MACsend(send_buf[min_index].p, 0);
  else
      Packet::free(send_buf[min_index].p);

  // A new packet is taking over the slot.

  send_buf[min_index].t = Scheduler::instance().clock();
  send_buf[min_index].p = p;
}


void DiffusionAgent::SendBufferCheck()
{
  int c;
  hdr_cdiff *dfh;
  hdr_ip   *iph;
  int dtype;
  PrvCurPtr RetVal;

  for (c = 0; c < SEND_BUF_SIZE; c++) {
    if (send_buf[c].p == NULL)
      continue;
    
    dfh = HDR_CDIFF(send_buf[c].p);
    iph = HDR_IP(send_buf[c].p);
    dtype = dfh->data_type;

    RetVal = INTF_FIND(routing_table[dtype].active, iph->dst_);

    if (RetVal.cur != NULL) {
      MACsend(send_buf[c].p, 0);
      send_buf[c].p = NULL;
      continue;
    }
    
    if (Scheduler::instance().clock() - send_buf[c].t > SEND_TIMEOUT) {
      Packet::free(send_buf[c].p);
      send_buf[c].p = NULL;
      continue;
    }

  }
}


void DiffusionAgent::trace (char *fmt,...)
{
  va_list ap;

  if (!tracetarget)
    return;

  va_start (ap, fmt);
  vsprintf (tracetarget->pt_->buffer (), fmt, ap);
  tracetarget->pt_->dump ();
  va_end (ap);
}


void DiffusionAgent::reset()
{
  PktTable.reset();

  for (int i=0; i<MAX_DATA_TYPE; i++) {
    routing_table[i].reset();
  }
  clear_arp_buf();
  clear_send_buf();
}


void DiffusionAgent::clear_arp_buf()
{
  for (int i=0; i<ARP_BUF_SIZE; i++) {
    arp_buf[i].t = 0;
    arp_buf[i].attempt = 0;
    if (arp_buf[i].p != NULL) 
      Packet::free(arp_buf[i].p);
    arp_buf[i].p = NULL;
  }
}

void DiffusionAgent::clear_send_buf()
{
  for (int i=0; i<SEND_BUF_SIZE; i++) {
    send_buf[i].t = 0;
    if (send_buf[i].p != NULL)
      Packet::free(send_buf[i].p);
    send_buf[i].p = NULL;
  }
}
				    







