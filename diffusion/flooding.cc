
/*
 * flooding.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: flooding.cc,v 1.11 2011/10/02 22:32:34 tom_henderson Exp $
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
/* flooding.cc : Chalermek Intanagonwiwat (USC/ISI)  05/18/99   */
/****************************************************************/

// Share api with diffusion and omnicient multicast
// Using diffusion packet header 

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
#include "flooding.h"
#include "iflist.h"
#include "hash_table.h"
#include "arp.h"
#include "mac.h"
#include "ll.h"
#include "dsr/path.h"
#include "god.h"

static class FloodingClass : public TclClass {
public:
  FloodingClass() : TclClass("Agent/Flooding") {}
  TclObject* create(int , const char*const* ) {
    return(new FloodingAgent());
  }
} class_flooding;



FloodingAgent::FloodingAgent() : Agent(PT_DIFF)
{
  // Initialize variables.

  pk_count = 0;
  target_ = 0;
  node = NULL;
  tracetarget = NULL;
}


void FloodingAgent::recv(Packet* packet, Handler*)
{
  hdr_cdiff* dfh = HDR_CDIFF(packet);

  // Packet Hash Table is used to keep info about experienced pkts.

  Pkt_Hash_Entry *hashPtr= PktTable.GetHash(dfh->sender_id, dfh->pk_num);

     // Received this packet before ?

     if (hashPtr != NULL) {
       Packet::free(packet);        
       return;
     }

     // Never receive it before ? Put in hash table.

     PktTable.put_in_hash(dfh);

     // Take action for a new pkt.
       
     ConsiderNew(packet);     
}


void FloodingAgent::ConsiderNew(Packet *pkt)
{
  hdr_cdiff* dfh = HDR_CDIFF(pkt);
  unsigned char msg_type = dfh->mess_type;
  unsigned int dtype = dfh->data_type;

  Agent_List *agentPtr;
  PrvCurPtr  RetVal;
  nsaddr_t   from_nodeID;

  Packet *gen_pkt;
  hdr_cdiff *gen_dfh;

  switch (msg_type) {
    case INTEREST : 

      // Check if it comes from sink agent of this node
      // If so we have to keep it in sink list 

      from_nodeID = (dfh->sender_id).addr_;


      if (THIS_NODE == from_nodeID) {       

	// It's from a sink on this node.
	// Is it already in list ?

	RetVal = INTF_FIND(routing_table[dtype].sink, dfh->sender_id);
	if (RetVal.cur == NULL) {            
	  // No, it's not.
	  agentPtr = new Agent_List;
	  AGT_ADDR(agentPtr) = dfh->sender_id;
	  INTF_INSERT(routing_table[dtype].sink, agentPtr);

	  God::instance()->AddSink(dtype, THIS_NODE);
	}	
      }
	  
      Packet::free(pkt);
      return;


    case DATA_READY :

      // put source_agent in source list of routing table

      agentPtr = new Agent_List;
      AGT_ADDR(agentPtr) = dfh->sender_id;
      agentPtr->next = routing_table[dtype].source;
      routing_table[dtype].source = agentPtr;

      God::instance()->AddSource(dtype, (dfh->sender_id).addr_);
	
      gen_pkt = prepare_message(dtype, dfh->sender_id, DATA_REQUEST);
      gen_dfh = HDR_CDIFF(gen_pkt);
      gen_dfh->report_rate = ORIGINAL;
      send_to_dmux(gen_pkt, 0);
      Packet::free(pkt);
      return;

 
    case DATA :

      DataForSink(pkt);

      MACprepare(pkt);
      MACsend(pkt, JITTER*Random::uniform(1.0));
      return;


    default : 
      
      Packet::free(pkt);        
      break;
  }
}


void FloodingAgent::reset()
{
  PktTable.reset();

  for (int i=0; i<MAX_DATA_TYPE; i++) {
    routing_table[i].reset();
  }
}


void FloodingAgent::Terminate() 
{
#ifdef DEBUG_OUTPUT
	printf("node %d: remaining energy %f, initial energy %f\n", THIS_NODE, 
	       node->energy_model()->energy(), 
	       node->energy_model()->initialenergy() );
#endif
}


void FloodingAgent::StopSource()
{
  Agent_List *cur;

  for (int i=0; i<MAX_DATA_TYPE; i++) {
    for (cur=routing_table[i].source; cur!=NULL; cur=AGENT_NEXT(cur) ) {
      SEND_MESSAGE(i, AGT_ADDR(cur), DATA_STOP);
    }
  }
}


Packet * FloodingAgent:: create_packet()
{
  Packet *pkt = allocpkt();

  if (pkt==NULL) return NULL;

  hdr_cmn*  cmh = HDR_CMN(pkt);
  cmh->size() = 36;

  hdr_cdiff* dfh = HDR_CDIFF(pkt);
  dfh->ts_ = NOW;
  return pkt;
}


Packet *FloodingAgent::prepare_message(unsigned int dtype, ns_addr_t to_addr, 
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


void FloodingAgent::MACprepare(Packet *pkt)
{
  hdr_cdiff* dfh = HDR_CDIFF(pkt);
  hdr_cmn* cmh = HDR_CMN(pkt);
  hdr_ip*  iph = HDR_IP(pkt);

  dfh->forward_agent_id = here_; 

  cmh->xmit_failure_ = 0;
  cmh->next_hop() = MAC_BROADCAST;
  cmh->addr_type() = NS_AF_ILINK;
  cmh->direction() = hdr_cmn::DOWN;

  iph->src_ = here_;
  iph->dst_.addr_ = MAC_BROADCAST;
  iph->dst_.port_ = ROUTING_PORT;

  dfh->num_next = 1;
  dfh->next_nodes[0] = MAC_BROADCAST;
}


void FloodingAgent::MACsend(Packet *pkt, Time delay)
{
  hdr_cmn*  cmh = HDR_CMN(pkt);
  hdr_cdiff* dfh = HDR_CDIFF(pkt);

  if (dfh->mess_type == DATA)
    cmh->size() = (God::instance()->data_pkt_size) + 4*(dfh->num_next - 1);
  else
    cmh->size() = 36 + 4*(dfh->num_next -1);

  Scheduler::instance().schedule(ll, pkt, delay);
}


void FloodingAgent::DataForSink(Packet *pkt)
{
  hdr_cdiff     *dfh  = HDR_CDIFF(pkt);
  unsigned int dtype = dfh->data_type;
  Agent_List   *cur_agent;
  Packet       *cur_pkt;
  hdr_cdiff     *cur_dfh;
  hdr_ip       *cur_iph;


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


void FloodingAgent::trace (char *fmt,...)
{
  va_list ap;

  if (!tracetarget)
    return;

  va_start (ap, fmt);
  vsprintf (tracetarget->pt_->buffer(), fmt, ap);
  tracetarget->pt_->dump ();
  va_end (ap);
}



int FloodingAgent::command(int argc, const char*const* argv)
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

  } else if (argc == 3) {

    if (strcasecmp(argv[1], "on-node")==0) {
      node = (Node *)tcl.lookup(argv[2]);
      return TCL_OK;
    }

    if (strcasecmp(argv[1], "add-ll") == 0) {

      TclObject *obj;

      if ( (obj = TclObject::lookup(argv[2])) == 0) {
	fprintf(stderr, "Flooding Node: %d lookup of %s failed\n", THIS_NODE,
		argv[2]);
	return TCL_ERROR;
      }
      ll = (NsObject *) obj;

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
	fprintf(stderr, "Flooding Node: %d lookup of %s failed\n", THIS_NODE,
		argv[2]);
	return TCL_ERROR;
      }
      port_dmux = (NsObject *) obj;
      return TCL_OK;
    }

  } 

  return Agent::command(argc, argv);
}


// Some methods for Flooding Entry

void Flooding_Entry::reset()
{
    clear_agentlist(source);
    clear_agentlist(sink);
    source = NULL;
    sink = NULL;
}

void Flooding_Entry::clear_agentlist(Agent_List *list)
{
  Agent_List *cur=list;
  Agent_List *temp = NULL;

  while (cur != NULL) {
    temp = AGENT_NEXT(cur);
    delete cur;
    cur = temp;
  }
}





