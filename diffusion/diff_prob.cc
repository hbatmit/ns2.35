
/*
 * diff_prob.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: diff_prob.cc,v 1.10 2005/08/25 18:58:03 johnh Exp $
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
/* diff_prob.cc : Chalermek Intanagonwiwat (USC/ISI)  05/18/99  */
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
#include "diff_prob.h"


static class DiffusionProbClass : public TclClass {
public:
  DiffusionProbClass() : TclClass("Agent/Diffusion/ProbGradient") {}
  TclObject* create(int , const char*const* ) {
    return(new DiffusionProb());
  }
} class_diffusion_probability;



void InterestTimer::expire(Event *)
{
  a_->InterestPropagate(pkt_, hashPtr_);
}


void EnergyTimer::expire(Event *)
{
  if (node_->energy_model()->energy() < threshold_) {    
    if (a_->NEG_REINF_ == true) {
      a_->SendNegReinf();
    }
    threshold_ = threshold_/2;
    a_->is_low_power = true;
  }

  if (threshold_ >= init_eng_/8) 
    resched(ENERGY_CHECK); 
}


DiffusionProb::DiffusionProb() : DiffusionAgent()
{
  is_low_power = false;
  num_neg_bcast_send = 0;
  num_neg_bcast_rcv = 0;
}


void DiffusionProb::recv(Packet* packet, Handler*)
{
  hdr_cdiff* dfh = HDR_CDIFF(packet);

  // Packet Hash Table is used to keep info about experienced pkts.

  Pkt_Hash_Entry *hashPtr= PktTable.GetHash(dfh->sender_id, dfh->pk_num);


#ifdef DEBUG_PROB
     printf("DF node %x recv %s (%x, %x, %d)\n",
	    THIS_NODE, MsgStr[dfh->mess_type], (dfh->sender_id).addr_, 
	    (dfh->sender_id).port_, dfh->pk_num);
#endif


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


void DiffusionProb::consider_old(Packet *pkt)
{
  hdr_cdiff* dfh = HDR_CDIFF(pkt);
  unsigned char msg_type = dfh->mess_type;
  unsigned int dtype = dfh->data_type;

  Pkt_Hash_Entry *hashPtr;
  From_List  *fromPtr;
  nsaddr_t from_nodeID, forward_nodeID;

  switch (msg_type) {
    case INTEREST :

      hashPtr = PktTable.GetHash(dfh->sender_id, dfh->pk_num);

      if (hashPtr->is_forwarded == true) {
	Packet::free(pkt);
	return;
      }

      from_nodeID = (dfh->sender_id).addr_;
      forward_nodeID = (dfh->forward_agent_id).addr_;


      hashPtr->num_from++;
      fromPtr = new From_List;
      AGT_ADDR(fromPtr) = dfh->forward_agent_id;
      fromPtr->rank = hashPtr->num_from;

      if (from_nodeID == forward_nodeID)
	fromPtr->is_sink = true;

      fromPtr->next = hashPtr->from_agent;
      hashPtr->from_agent = fromPtr;

      // Check if this hashPtr has timer, and if lists already exist,
      // to decide whether to create or update Out_List and In_List

      if (hashPtr->timer == NULL) {
	if (hashPtr->has_list==false) {
	  CreateIOList(hashPtr, dtype);
        } 
	else {
	  UpdateIOList(fromPtr, dtype);
	}
      }
      
      Packet::free(pkt);
      break;

    default : 
      Packet::free(pkt);        
      break;
  }
}


void DiffusionProb::consider_new(Packet *pkt)
{
  hdr_cdiff* dfh = HDR_CDIFF(pkt);
  unsigned char msg_type = dfh->mess_type;
  unsigned int dtype = dfh->data_type;

  Pkt_Hash_Entry *hashPtr;
  From_List  *fromPtr;
  Agent_List *agentPtr;
  Agent_List *cur;
  PrvCurPtr  RetVal;
  nsaddr_t   from_nodeID, forward_nodeID;

  int i;

  switch (msg_type) {
    case INTEREST : 

      hashPtr = PktTable.GetHash(dfh->sender_id, dfh->pk_num);

      // Check if it comes from sink agent of this node
      // If so we have to keep it in sink list 

      from_nodeID = (dfh->sender_id).addr_;
      forward_nodeID = (dfh->forward_agent_id).addr_;


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
      else {                             
	
	// It's not from a sink on this node.

	fromPtr = new From_List;
	hashPtr->from_agent = fromPtr;
	hashPtr->num_from = 1;
	AGT_ADDR(fromPtr) = dfh->forward_agent_id;
	fromPtr->rank = 1;
	fromPtr->next = NULL;    

	// Is the forwarder node is a sink node ? 
	// This information is useful when we do negative reinforcement.

	if ( from_nodeID == forward_nodeID )
	  fromPtr->is_sink = true;

      }


      // Do we have to request data ?

      if (routing_table[dtype].source == NULL) {     

	// No, we don't. Better forward the interest.

	hashPtr->timer = new InterestTimer(this, hashPtr, pkt);
	(hashPtr->timer)->sched(INTEREST_DELAY*Random::uniform(1.0));
      } 
      else {
	
	// Since this node has sources,it won't propagate the interest.
	// We need to create Out_List and In_List in routing table [dtype] now.

	CreateIOList(hashPtr, dtype);
       	data_request_all(dtype);

	Packet::free(pkt);
      }
      break;


    case POS_REINFORCE :

      if ( POS_REINF_ == false ) {
	printf("Hey, we are not in pos_reinf mode.\n");
	Packet::free(pkt);
	exit(-1);
      }

      IncGradient(dtype, dfh->forward_agent_id);
      CAL_RANGE(routing_table[dtype].active);

      if (routing_table[dtype].source == NULL) {
	if (is_low_power == false) {
	  FwdPosReinf(dtype, pkt);
	  return;
	}
      } 

      Packet::free(pkt);
      break;


    case NEG_REINFORCE :  

      if (NEG_REINF_ == false) {
	printf("Hey, we are not in neg_reinf mode.\n");
	Packet::free(pkt);
	exit(-1);
      }

      // Negative Reinforcement won't be forwarded.

      num_neg_bcast_rcv++;

      for (i=0; i<MAX_DATA_TYPE; i++) {
	RetVal = INTF_FIND(routing_table[i].active, dfh->sender_id);
	if (RetVal.cur == NULL ) {
	  continue;
	}

	// If it is not a sink, we decrease the gradient.

	if ( IS_SINK(RetVal.cur) == false) {
	  DecGradient(i, dfh->sender_id);
	  CAL_RANGE(routing_table[i].active);
	}
      }

      Packet::free(pkt);
      break;


    case DATA_READY :

      // put source_agent in source list of routing table

      agentPtr = new Agent_List;
      AGT_ADDR(agentPtr) = dfh->sender_id;
      agentPtr->next = routing_table[dtype].source;
      routing_table[dtype].source = agentPtr;

      if (routing_table[dtype].active != NULL ||
	  routing_table[dtype].sink != NULL) {
	SEND_MESSAGE(dtype, dfh->sender_id, DATA_REQUEST);
      }

      Packet::free(pkt);
      break;


    case DATA :

      DataForSink(pkt);

      routing_table[dtype].IncRecvCnt(dfh->forward_agent_id);
      ForwardData(pkt);

      if (routing_table[dtype].counter >= MAX_REINFORCE_COUNTER) {
	if (is_low_power == false) {
	  if (POS_REINF_ == true)
	    GenPosReinf(dtype);
	  return;
	}
	if (routing_table[dtype].sink != NULL) {
	  if (POS_REINF_ == true)
	    GenPosReinf(dtype);
	  return;
	}
      }
      break;


    case INHIBIT :

      if (routing_table[dtype].active == NULL) {
        Packet::free(pkt);
        return;
      }

      RetVal=INTF_FIND(routing_table[dtype].active, dfh->sender_id);
      if (RetVal.cur == NULL){
	Packet::free(pkt);
	return;
      }

      INTF_REMOVE(RetVal.prv, RetVal.cur);
      INTF_INSERT(routing_table[dtype].inactive, RetVal.cur);
      routing_table[dtype].num_active --;
      NORMALIZE(routing_table[dtype].active);
      CAL_RANGE(routing_table[dtype].active);

      if (routing_table[dtype].num_active < 1) {

	// *** You need to stop the source if you have one. *****

	if (routing_table[dtype].source != NULL ) {
	  for (cur=routing_table[dtype].source; cur != NULL; 
	       cur=AGENT_NEXT(cur)) {
	    SEND_MESSAGE(dtype, AGT_ADDR(cur), DATA_STOP);
	  }
	  Packet::free(pkt);
	  return;
	}

	// If not, send inhibit signal upstream

	SendInhibit(dtype);
      }
      
      Packet::free(pkt);
      return;


    case TX_FAILED :

      if (BACKTRACK_ == false) {
	printf("We are not in backtracking mode.\n");
	Packet::free(pkt);
	exit(-1);
      }

      if (routing_table[dtype].active == NULL) {
        ForwardTxFailed(pkt);
        return;
      }

      //      RetVal=INTF_FIND(routing_table[dtype].active, dfh->sender_id);

      RetVal=INTF_FIND(routing_table[dtype].active, dfh->forward_agent_id);

      if (RetVal.cur != NULL){
	INTF_REMOVE(RetVal.prv, RetVal.cur);
	INTF_INSERT(routing_table[dtype].inactive, RetVal.cur);
	routing_table[dtype].num_active --;
	NORMALIZE(routing_table[dtype].active);
	CAL_RANGE(routing_table[dtype].active);
      }

      if (routing_table[dtype].num_active < 1) {
	ForwardTxFailed(pkt);
	return;
      }
      
      ReTxData(pkt);

      Packet::free(pkt);
      return;


    default : 
      
      Packet::free(pkt);        
      break;
  }
}


void DiffusionProb::InterestPropagate(Packet *pkt, 
					       Pkt_Hash_Entry *hashPtr)
{
  hdr_cdiff *dfh = HDR_CDIFF(pkt);
  unsigned int dtype=dfh->data_type;

  CreateIOList(hashPtr, dtype);

  if ( routing_table[dtype].source != NULL ) { 
    data_request_all(dtype);
    Packet::free(pkt);
    return;
  }

  MACprepare(pkt, MAC_BROADCAST, NS_AF_ILINK, 0);
  MACsend(pkt, 0);
  overhead++;
  hashPtr->is_forwarded = true;
}


void DiffusionProb::ForwardData(Packet *pkt)
{
  hdr_cdiff     *dfh  = HDR_CDIFF(pkt);
  unsigned int dtype =dfh->data_type;
  Out_List     *cur_out;
  Packet       *cur_pkt;
  hdr_cdiff     *cur_dfh;
  hdr_ip       *cur_iph;

  cur_out = WHERE_TO_GO(routing_table[dtype].active);

  if (cur_out !=NULL) {

    // Got somewhere to go.

      cur_pkt = pkt;
      cur_iph = HDR_IP(cur_pkt);

      cur_iph->dst_ = AGT_ADDR(cur_out);

      cur_dfh = HDR_CDIFF(cur_pkt);
      cur_dfh->forward_agent_id = here_;
      cur_dfh->num_next = 1;
      cur_dfh->next_nodes[0] = NODE_ADDR(cur_out);

      cur_out->num_data_send++;

#ifdef DEBUG_PROB
      printf("DF node %x will send data (%x, %x, %d) to %x\n",
	       THIS_NODE, (cur_dfh->sender_id).addr_,
	       (cur_dfh->sender_id).port_, cur_dfh->pk_num,
	       AGT_ADDR(cur_out));
#endif

      MACprepare(cur_pkt, NODE_ADDR(cur_out), NS_AF_INET, 1);
      MACsend(cur_pkt, 0);

      return;
  }

  if (routing_table[dtype].sink != NULL) {

    // No where to go but have sinks.

    Packet::free(pkt);
    return;
  }

  // No where to go and no sink.    
  // Check if we have sources on this node.
  // If so, we stop the sources. Otherwise, we generate the transmission
  // failure packet. 

  Agent_List *cur;

  if (routing_table[dtype].source != NULL ) {
    for (cur=routing_table[dtype].source; cur != NULL; cur=AGENT_NEXT(cur)) {

      // DATA_STOP never go out of the node, so don't care if wireless.

      SEND_MESSAGE(dtype, AGT_ADDR(cur), DATA_STOP);
    }
    Packet::free(pkt);
    return;
  }

  // No source, generate the transmission failure packet.

  if (BACKTRACK_ == false) {
    Packet::free(pkt);
    return;
  }

  // YES, we are in backtracking mode so send TX_FAILED to the forwarder.

  cur_pkt = prepare_message(dtype, dfh->forward_agent_id, TX_FAILED);
  cur_dfh = HDR_CDIFF(cur_pkt);
  cur_dfh->info.sender = dfh->sender_id;
  cur_dfh->info.seq = dfh->pk_num;

  hdr_cmn *cmh = HDR_CMN(pkt);
  cur_dfh->info.size = cmh->size_;

  MACprepare(cur_pkt, (dfh->forward_agent_id).addr_, NS_AF_INET, 0);
  MACsend(cur_pkt, 0);

  Packet::free(pkt);
}


void DiffusionProb::ForwardTxFailed(Packet *pkt)
{
  hdr_cdiff *dfh = HDR_CDIFF(pkt);
  hdr_ip   *iph = HDR_IP(pkt);

  dfh->forward_agent_id = here_;

  Pkt_Hash_Entry *hashPtr=PktTable.GetHash(dfh->info.sender, dfh->info.seq);

  if (hashPtr == NULL) {
    Packet::free(pkt);
    return;
  }

  iph->dst_ = hashPtr->forwarder_id;
  dfh->num_next = 1;
  dfh->next_nodes[0] = (hashPtr->forwarder_id).addr_;

  MACprepare(pkt, (hashPtr->forwarder_id).addr_, NS_AF_INET, 0);
  MACsend(pkt, 0);

  overhead++;
}


void DiffusionProb::ReTxData(Packet *pkt)
{
  hdr_cdiff *dfh = HDR_CDIFF(pkt);  
  Pkt_Hash_Entry *hashPtr=PktTable.GetHash(dfh->info.sender, dfh->info.seq);

  // Make sure it has data on its cache.

  if (hashPtr == NULL) {
    printf("No hash for (%x, %x, %d)\n", (dfh->info.sender).addr_, 
	   (dfh->info.sender).port_, dfh->info.seq);
    return;
  }

  int dtype = dfh->data_type;
  Out_List *to_out = WHERE_TO_GO(routing_table[dtype].active);

  if (to_out == NULL) return;

  Packet *rtxPkt = prepare_message(dtype, AGT_ADDR(to_out), DATA);
  hdr_cdiff *rtx_dfh = HDR_CDIFF(rtxPkt);
  hdr_cmn  *rtx_cmh = HDR_CMN(rtxPkt);

  rtx_dfh->sender_id = dfh->info.sender;
  rtx_dfh->pk_num = dfh->info.seq;
  rtx_cmh->size_ = dfh->info.size;
  
  MACprepare(rtxPkt, NODE_ADDR(to_out), NS_AF_INET, 1);
  MACsend(rtxPkt, 0);

  printf("Retransmit (%d,%d,%d)\n",(rtx_dfh->sender_id).addr_, 
	 (rtx_dfh->sender_id).port_, rtx_dfh->pk_num);
}


void DiffusionProb::data_request_all(unsigned int dtype)
{
  Agent_List *cur_agent;

  for (cur_agent=routing_table[dtype].source; cur_agent != NULL; 
       cur_agent = AGENT_NEXT(cur_agent) ) {
    SEND_MESSAGE(dtype, AGT_ADDR(cur_agent), DATA_REQUEST);
  }
}


void DiffusionProb::CreateIOList(Pkt_Hash_Entry *hashPtr, 
					  unsigned int dtype)
{
  From_List *fromPtr;

  // Better clear out all existing IO lists first.

  INTF_FREEALL(routing_table[dtype].active);
  INTF_FREEALL(routing_table[dtype].inactive);
  INTF_FREEALL(routing_table[dtype].iif);
  INTF_FREEALL(routing_table[dtype].down_iif);
  routing_table[dtype].num_active=0;
  routing_table[dtype].counter=0;

  for (fromPtr = hashPtr->from_agent; fromPtr != NULL;
       fromPtr = FROM_NEXT(fromPtr) ) {
    add_outlist(dtype, fromPtr);
  }

  hashPtr->has_list = true;

  CalGradient(dtype);
  CAL_RANGE(routing_table[dtype].active);
}


void DiffusionProb::UpdateIOList(From_List *fromPtr, 
					  unsigned int dtype)
{
  add_outlist(dtype, fromPtr);
  CalGradient(dtype);
  CAL_RANGE(routing_table[dtype].active);
}


void DiffusionProb::add_outlist(unsigned int dtype, From_List *foundPtr)
{
  Out_List *outPtr = new Out_List;

  AGT_ADDR(outPtr) = AGT_ADDR(foundPtr);
  outPtr->rank = foundPtr->rank;
  outPtr->is_sink = foundPtr->is_sink;

  INTF_INSERT(routing_table[dtype].active, outPtr);
  routing_table[dtype].num_active ++;
}


void DiffusionProb::Print_IOlist()
{
  Out_List *cur_out;
  In_List *cur_in;
  int     i;

  for (i=0; i<1; i++) {
    printf("Node %d neg bcast send %d, neg bcast rcv %d\n",
	   THIS_NODE, num_neg_bcast_send, num_neg_bcast_rcv);
    for (cur_out = routing_table[i].active; cur_out != NULL; 
	 cur_out = OUT_NEXT(cur_out) ) {
      printf("DF node %d has oif %d (%f,%d) send data %d recv neg %d pos %d\n", 
           THIS_NODE, NODE_ADDR(cur_out), GRADIENT(cur_out),
           routing_table[i].num_active, NUM_DATA_SEND(cur_out), 
	   NUM_NEG_RECV(cur_out), NUM_POS_RECV(cur_out));
    }

    for (cur_in = routing_table[i].iif; cur_in != NULL;
	 cur_in = IN_NEXT(cur_in) ) {
      printf("Diffusion node %d has iif for %d\n", 
	     THIS_NODE, NODE_ADDR(cur_in));
    }

    for (cur_out = routing_table[i].inactive; cur_out != NULL; 
	 cur_out = OUT_NEXT(cur_out) ) {
      printf("Diffusion node %d has down oif %d (%f, %d) send %d\n", 
           THIS_NODE, NODE_ADDR(cur_out), cur_out->gradient,
           routing_table[i].num_active, cur_out->num_data_send);

    }

    for (cur_in = routing_table[i].down_iif; cur_in != NULL;
	 cur_in = IN_NEXT(cur_in) ) {
      printf("Diffusion node %d has down_iif for %d (recv %d)\n", THIS_NODE, 
	     NODE_ADDR(cur_in), cur_in->total_received);
    }

  }
  
}


void DiffusionProb::CalGradient(unsigned int dtype)
{
  Out_List *cur_out;
  
  for (cur_out = routing_table[dtype].active; cur_out != NULL;
       cur_out = OUT_NEXT(cur_out) ) {
    cur_out->gradient = pow(2, routing_table[dtype].num_active - 
			    cur_out->rank) / 
                       ( pow(2, routing_table[dtype].num_active) - 1);
  }
}



void DiffusionProb::IncGradient(unsigned int dtype, ns_addr_t addr)
{
  Out_List *cur_out;
  PrvCurPtr RetVal;

  RetVal=INTF_FIND(routing_table[dtype].active, addr);

  if (RetVal.cur != NULL) {
    cur_out = (Out_List *)(RetVal.cur);
    GRADIENT(cur_out) = GRADIENT(cur_out) + 0.99;
    NORMALIZE(routing_table[dtype].active);
  }

}


void DiffusionProb::DecGradient(unsigned int dtype, ns_addr_t addr)
{
  Out_List *cur_out;
  PrvCurPtr RetVal;

  RetVal=INTF_FIND(routing_table[dtype].active, addr);

  if (RetVal.cur != NULL) {

    for (cur_out = routing_table[dtype].active; cur_out != NULL;
	 cur_out = OUT_NEXT(cur_out) ) 
      GRADIENT(cur_out) = GRADIENT(cur_out) + 1.0;

    cur_out = (Out_List *)(RetVal.cur);
    GRADIENT(cur_out) = GRADIENT(cur_out) - 1.99;
    if (GRADIENT(cur_out)< 0.0) {
       GRADIENT(cur_out) = 0.0;
    }
    NORMALIZE(routing_table[dtype].active);
  }
}



void DiffusionProb::GenPosReinf(unsigned int dtype)
{
  In_List *cur_in, *max_in;
  Packet *pkt;

  max_in = FIND_MAX_IN(routing_table[dtype].iif);

  if (max_in != NULL) {
   if ( (max_in->total_received - max_in->prev_received) < 
         routing_table[dtype].counter) {

      pkt=prepare_message(dtype, AGT_ADDR(max_in), POS_REINFORCE);

      MACprepare(pkt, NODE_ADDR(max_in), NS_AF_INET, 0);
      MACsend(pkt, 0);

      overhead++;

    }
  }

  routing_table[dtype].counter = 0;
  for (cur_in = routing_table[dtype].iif; cur_in != NULL;
       cur_in = IN_NEXT(cur_in) ) {
    cur_in->prev_received = cur_in->total_received;
  }

}


void DiffusionProb::FwdPosReinf(unsigned int dtype, Packet *pkt)
{
  In_List  *cur_in, *max_in=NULL;
  hdr_ip   *iph = HDR_IP(pkt);
  hdr_cdiff *dfh = HDR_CDIFF(pkt);

  max_in = FIND_MAX_IN(routing_table[dtype].iif);
  if (max_in != NULL) {

    iph->dst_ = AGT_ADDR(max_in);

    dfh->num_next = 1;
    dfh->next_nodes[0] = NODE_ADDR(max_in);
    dfh->forward_agent_id = here_;

    MACprepare(pkt, NODE_ADDR(max_in), NS_AF_INET, 0);
    MACsend(pkt, 0);

    overhead++;
  }
  else {
    Packet::free(pkt);
  }

  routing_table[dtype].counter = 0;
  for (cur_in = routing_table[dtype].iif; cur_in != NULL;
       cur_in = IN_NEXT(cur_in) ) {
    cur_in->prev_received = cur_in->total_received;
  }
}


void DiffusionProb::Start()
{
  DiffusionAgent::Start();

  energy_timer = new EnergyTimer(this, node);
  energy_timer->resched(ENERGY_CHECK + ENERGY_CHECK * Random::uniform(1.0));
}


void DiffusionProb::InterfaceDown(int dtype, ns_addr_t DownDiff)
{
  PrvCurPtr RetVal;

  RetVal = INTF_FIND(routing_table[dtype].iif, DownDiff);

  if (RetVal.cur != NULL) {
    INTF_REMOVE(RetVal.prv, RetVal.cur);
    INTF_INSERT(routing_table[dtype].down_iif, RetVal.cur);
    return;
  }

  RetVal = INTF_FIND(routing_table[dtype].active, DownDiff);
  if (RetVal.cur == NULL)
    return;

  INTF_REMOVE(RetVal.prv, RetVal.cur);
  INTF_INSERT(routing_table[dtype].inactive, RetVal.cur);
  routing_table[dtype].num_active --;
  NORMALIZE(routing_table[dtype].active);
  CAL_RANGE(routing_table[dtype].active);

  if (routing_table[dtype].num_active < 1) {

    // ** If we have a source, stop the source.
    // If not, send inhibit signal upstream. **

    Agent_List *cur;

    if (routing_table[dtype].source != NULL ) {
      for (cur=routing_table[dtype].source; cur != NULL; cur=AGENT_NEXT(cur)) {
	     SEND_MESSAGE(dtype, AGT_ADDR(cur), DATA_STOP);
      }
    }
    else {
      SendInhibit(dtype);
    }
  }
}


void DiffusionProb::SendInhibit(int dtype)
{
  // Wireless. Just use one MAC broadcast.

    ns_addr_t bcast_addr;
    bcast_addr.addr_ = MAC_BROADCAST;
    bcast_addr.port_ = ROUTING_PORT;

    Packet *pkt = prepare_message(dtype, bcast_addr, INHIBIT);
    MACprepare(pkt, MAC_BROADCAST, NS_AF_ILINK, 0);
    MACsend(pkt, 0);
    overhead++;
    return;
}


// For negative reinforcement, we don't care the data type.
// Any neighbor upstream of any data type should be informed.

void DiffusionProb::SendNegReinf()
{
    ns_addr_t bcast_addr;
    bcast_addr.addr_ = MAC_BROADCAST;
    bcast_addr.port_ = ROUTING_PORT;

    Packet *pkt = prepare_message(0, bcast_addr, NEG_REINFORCE);
    MACprepare(pkt, MAC_BROADCAST, NS_AF_ILINK, 0);
    MACsend(pkt, 0);
    overhead++;
    return;
}

