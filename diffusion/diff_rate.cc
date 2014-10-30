
/*
 * diff_rate.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: diff_rate.cc,v 1.8 2005/08/25 18:58:03 johnh Exp $
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
/* diff_rate.cc : Chalermek Intanagonwiwat (USC/ISI)  05/18/99  */
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
#include "diff_rate.h"

extern char *MsgStr[];

static class DiffusionRateClass : public TclClass {
public:
  DiffusionRateClass() : TclClass("Agent/Diffusion/RateGradient") {}
  TclObject* create(int , const char*const* ) {
    return(new DiffusionRate());
  }
} class_diffusion_rate;


void GradientTimer::expire(Event *)
{
  a_->GradientTimeOut();
}


void NegativeReinforceTimer::expire(Event *)
{
  a_->NegReinfTimeOut();
}


DiffusionRate::DiffusionRate() : DiffusionAgent()
{
  DUP_SUP_ = true;

  sub_type_ = BCAST_SUB;
  org_type_ = UNICAST_ORG;
  pos_type_ = POS_ALL;
  pos_node_type_ = INTM_POS;
  neg_win_type_ = NEG_TIMER;
  neg_thr_type_ = NEG_ABSOLUTE;
  neg_max_type_ = NEG_FIXED_MAX;

  num_not_send_bcast_data = 0;
  num_data_bcast_send = 0;
  num_data_bcast_rcv = 0;
  num_neg_bcast_send = 0;
  num_neg_bcast_rcv = 0;
}


void DiffusionRate::recv(Packet* packet, Handler*)
{
  hdr_cdiff* dfh = HDR_CDIFF(packet);

  // Packet Hash Table is used to keep info about experienced pkts.

  Pkt_Hash_Entry *hashPtr= PktTable.GetHash(dfh->sender_id, dfh->pk_num);


#ifdef DEBUG_RATE
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
     // Check for dupplicate data at application
     
     if (DUP_SUP_ == true) {  
       
       if (dfh->mess_type == DATA) {
	 if (DataTable.GetHash(dfh->attr) != NULL) {
	   consider_old(packet);
	   return;
	 } else {
	   DataTable.PutInHash(dfh->attr);
	 }
	 
       }
     } 
       
     consider_new(packet);     
}


void DiffusionRate::consider_old(Packet *pkt)
{
  hdr_cdiff* dfh = HDR_CDIFF(pkt);
  hdr_cmn* cmh = HDR_CMN(pkt);
  unsigned char msg_type = dfh->mess_type;
  unsigned int dtype = dfh->data_type;

  switch (msg_type) {
    case INTEREST :
      InterestHandle(pkt);
      return;

    case DATA: 

      if (cmh->next_hop_ == (nsaddr_t)MAC_BROADCAST) {
	num_data_bcast_rcv++;
      }

      if (dfh->report_rate == ORIGINAL) {
	  routing_table[dtype].CntOldOrg(dfh->forward_agent_id);
      }
      Packet::free(pkt);
      return;
    
    case NEG_REINFORCE:
      if (cmh->next_hop_ == (nsaddr_t)MAC_BROADCAST) {
	num_neg_bcast_rcv++;
      }
      break;
      
    default : 
      Packet::free(pkt);        
      break;
  }
}


void DiffusionRate::consider_new(Packet *pkt)
{
  hdr_cdiff* dfh = HDR_CDIFF(pkt);
  hdr_cmn * cmh = HDR_CMN(pkt);
  unsigned char msg_type = dfh->mess_type;
  unsigned int dtype = dfh->data_type;

  Agent_List *agentPtr;
  Packet *gen_pkt;
  hdr_cdiff *gen_dfh;

  switch (msg_type) {
    case INTEREST : 
      InterestHandle(pkt);
      return;

    case POS_REINFORCE :
      if ( POS_REINF_ == false ) {
	printf("Hey, we are not in pos_reinf mode.\n");
	Packet::free(pkt);
	exit(-1);
      }

      ProcessPosReinf(pkt);
      return;

    case NEG_REINFORCE :  
      if (cmh->next_hop_ == (nsaddr_t)MAC_BROADCAST) {
	num_neg_bcast_rcv++;
      } else {
	routing_table[dtype].CntNeg(dfh->forward_agent_id);
      }

      if (NEG_REINF_ == false) {
	printf("Hey, we are not in neg_reinf mode.\n");
	Packet::free(pkt);
	exit(-1);
      }

      ProcessNegReinf(pkt);
      return;

    case DATA_READY :

      // put source_agent in source list of routing table

      agentPtr = new Agent_List;
      AGT_ADDR(agentPtr) = dfh->sender_id;
      agentPtr->next = routing_table[dtype].source;
      routing_table[dtype].source = agentPtr;

      God::instance()->AddSource(dtype, (dfh->sender_id).addr_);
	/*
	printf("DF %d received DATA_READY (%d, %d, %d) at time %lf\n",
	       THIS_NODE, dfh->sender_id.addr_, dfh->sender_id.port_,
	       dfh->pk_num, NOW);
	*/

      if (routing_table[dtype].active != NULL ||
	    routing_table[dtype].sink != NULL) {
	gen_pkt = prepare_message(dtype, dfh->sender_id, DATA_REQUEST);
	gen_dfh = HDR_CDIFF(gen_pkt);
	gen_dfh->report_rate = SUB_SAMPLED;
	send_to_dmux(gen_pkt, 0);

	  /*
	  printf("DF %d sent DATA_REQUEST (%d, %d, %d) at time %lf\n",
		 THIS_NODE, gen_dfh->sender_id.addr_, gen_dfh->sender_id.port_,
		 gen_dfh->pk_num, NOW);
	  */
      }
	  
      Packet::free(pkt);
      return;

    case DATA :

      if (cmh->next_hop_ == (nsaddr_t)MAC_BROADCAST) {
	num_data_bcast_rcv++;
      }

      DataForSink(pkt);

      if (dfh->report_rate == SUB_SAMPLED) {
       	  routing_table[dtype].CntNewSub(dfh->forward_agent_id);
	  FwdData(pkt);
	  return;
      }

      if (dfh->report_rate == ORIGINAL) {
	  routing_table[dtype].new_org_counter++;
	  routing_table[dtype].CntNewOrg(dfh->forward_agent_id);
	  FwdData(pkt);

	  if (neg_win_type_ == NEG_COUNTER) {
	    CheckNegCounter(dtype);
	    return;
	  }
      }
      return;

    default : 
      Packet::free(pkt);        
      break;
  }
}


void DiffusionRate::CheckNegCounter(int dtype)
{
  if (neg_max_type_ == NEG_FIXED_MAX) {
	  if (routing_table[dtype].new_org_counter >= MAX_NEG_COUNTER
	      && NEG_REINF_ == true) {
	    GenNeg(dtype);
	    routing_table[dtype].new_org_counter = 0;
	    routing_table[dtype].ClrAllNewOrg();
	    routing_table[dtype].ClrAllOldOrg();
	  }
	  return;
  }

  if (neg_max_type_ == NEG_SCALE_MAX) {
	  if (routing_table[dtype].new_org_counter >= 
	      PER_IIF * routing_table[dtype].num_iif
	      && NEG_REINF_ == true) {
	    GenNeg(dtype);
	    routing_table[dtype].new_org_counter = 0;
	    routing_table[dtype].ClrAllNewOrg();
	    routing_table[dtype].ClrAllOldOrg();
	  }
	  return;
  }
}


void DiffusionRate::InterestHandle(Packet *pkt)
{
  hdr_cdiff *dfh = HDR_CDIFF(pkt);
  unsigned int dtype = dfh->data_type;
  Agent_List *agentPtr;

  nsaddr_t from_nodeID;
  PrvCurPtr RetVal;
  Out_List *OutPtr;


   if (dfh->ts_ + INTEREST_TIMEOUT < NOW) {
      Packet::free(pkt);
      return;
   }

   // Check if it comes from sink agent of this node
   // If so we have to keep it in sink list 

   from_nodeID = (dfh->sender_id).addr_;

   if (THIS_NODE == from_nodeID) {       // From sink agent on the same node.

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
   } else {                                    // From different node.

     // If we have gradient for the forwarder.
	
     RetVal = INTF_FIND(routing_table[dtype].active, dfh->forward_agent_id);
     if (RetVal.cur == NULL) {
       OutPtr = new Out_List;
       AGT_ADDR(OutPtr) = dfh->forward_agent_id;
       GRADIENT(OutPtr) = dfh->report_rate;
       GRAD_TMOUT(OutPtr) = dfh->ts_ + INTEREST_TIMEOUT;
       INTF_INSERT(routing_table[dtype].active, OutPtr);
       routing_table[dtype].num_active ++;	  
     } else {
      GRAD_TMOUT(RetVal.cur) = max(GRAD_TMOUT(RetVal.cur),
				   dfh->ts_ + INTEREST_TIMEOUT);
     }

   }

   if (NOW > routing_table[dtype].last_fwd_time + INTEREST_PERIODIC) {
     if (routing_table[dtype].ExistOriginalGradient() == true) {
       DataReqAll(dtype, ORIGINAL);
     } else {
       DataReqAll(dtype, SUB_SAMPLED);
     }
     routing_table[dtype].last_fwd_time = NOW;
     MACprepare(pkt, MAC_BROADCAST, NS_AF_ILINK, 0);
     MACsend(pkt, JITTER*Random::uniform(1.0));
     overhead++;

#ifdef DEBUG_RATE
     hdr_cmn *cmh = HDR_CMN(pkt);
     printf("DF node %x will send %s (%x, %x, %d) to %x\n",
	    THIS_NODE, MsgStr[dfh->mess_type], (dfh->sender_id).addr_, 
	    (dfh->sender_id).port_, dfh->pk_num, cmh->next_hop());
#endif


     return;
   }

   Packet::free(pkt);
   return;
}


void DiffusionRate::NegReinfTimeOut()
{
  for (int i=0; i<MAX_DATA_TYPE; i++) {
    GenNeg(i);
    routing_table[i].new_org_counter = 0;
    routing_table[i].ClrAllNewOrg();
    routing_table[i].ClrAllOldOrg();
  }
  
  neg_reinf_timer->resched(NEG_CHECK);
}


void DiffusionRate::GradientTimeOut()
{
  int i;
  Agent_List *cur_out, **prv_out;
 
  for (i=0; i<MAX_DATA_TYPE; i++) {
    for (cur_out = routing_table[i].active, 
	 prv_out = (Agent_List **)&routing_table[i].active; 
	 cur_out != NULL; ) {
      
      if (NOW > GRAD_TMOUT(cur_out)) {
	INTF_REMOVE(prv_out, cur_out);
	routing_table[i].num_active -- ;
	cur_out = *prv_out;
      }
      else {
	prv_out = &(cur_out->next);
	cur_out = cur_out->next;
      }
      
    }
  }
  
  gradient_timer->resched(INTEREST_TIMEOUT);
}


bool DiffusionRate::FwdSubsample(Packet *pkt)
{
  hdr_cdiff *dfh = HDR_CDIFF(pkt);
  Out_List *cur_out;
  Packet   *cur_pkt;
  hdr_cdiff *cur_dfh;
  hdr_ip   *cur_iph;
  unsigned int dtype = dfh->data_type;

    if (routing_table[dtype].num_active <= 0) {   // Won't forward
      num_not_send_bcast_data++;
      return false;
    } 

    // Will forward
      
    num_data_bcast_send++;

    if (sub_type_ == BCAST_SUB) {
	MACprepare(pkt, MAC_BROADCAST, NS_AF_ILINK, 0);
	MACsend(pkt, JITTER*Random::uniform(1.0));

#ifdef DEBUG_RATE 
	hdr_cmn *cmh = HDR_CMN(pkt);
	printf("DF node %x will send %s (%x, %x, %d) to %x\n",
	   THIS_NODE, MsgStr[dfh->mess_type], (dfh->sender_id).addr_,
	   (dfh->sender_id).port_, dfh->pk_num, cmh->next_hop());
#endif // DEBUG_RATE

	return true;
    }

    if (sub_type_ == UNICAST_SUB) {
	for (cur_out = routing_table[dtype].active; cur_out!= NULL; 
	   cur_out = OUT_NEXT(cur_out)) {

	  cur_pkt       = pkt->copy();
	  cur_iph       = HDR_IP(cur_pkt);
	  cur_iph->dst_ = AGT_ADDR(cur_out);

	  cur_dfh       = HDR_CDIFF(cur_pkt);
	  cur_dfh->forward_agent_id = here_;
	  cur_dfh->num_next = 1;
	  cur_dfh->next_nodes[0] = NODE_ADDR(cur_out);

	  MACprepare(cur_pkt, NODE_ADDR(cur_out), NS_AF_INET, 
		 MAC_RETRY_);
	  MACsend(cur_pkt, 0);      

#ifdef DEBUG_RATE
	  cur_cmh = HDR_CMN(cur_pkt);
	  printf("DF node %x will send %s (%x, %x, %d) to %x\n",
	     THIS_NODE, MsgStr[cur_dfh->mess_type], 
	     (cur_dfh->sender_id).addr_, (cur_dfh->sender_id).port_, 
	     cur_dfh->pk_num, cur_cmh->next_hop());
#endif // DEBUG_RATE

	} // endfor
	
	return true;
    }   // endif unicast sub

    return false;
}


void DiffusionRate::TriggerPosReinf(Packet *pkt, ns_addr_t forward_agent)
{
  hdr_cdiff *dfh = HDR_CDIFF(pkt);
  unsigned int dtype = dfh->data_type;
  nsaddr_t forwarder_node = forward_agent.addr_;

  if (pos_node_type_ == INTM_POS) {
    if (routing_table[dtype].sink != NULL ||
	routing_table[dtype].ExistOriginalGradient() == true) {
      DataReqAll(dtype, ORIGINAL);
      if (THIS_NODE != forwarder_node) {
       PosReinf(dtype, forwarder_node, dfh->sender_id,
		      dfh->pk_num);
       routing_table[dtype].CntPosSend(forward_agent);
       routing_table[dtype].ClrNewSub(forward_agent);
      } 
    }
    return;
  }
 

  if (pos_node_type_ == END_POS) {
    if (routing_table[dtype].sink != NULL) {
      DataReqAll(dtype, ORIGINAL);
      if (THIS_NODE != forwarder_node) {
       PosReinf(dtype, forwarder_node, dfh->sender_id,
		      dfh->pk_num);
       routing_table[dtype].CntPosSend(forward_agent);
       routing_table[dtype].ClrNewSub(forward_agent);
      } 
    }

    return;
  }
}


void DiffusionRate::FwdOriginal(Packet *pkt)
{
  hdr_cdiff *dfh = HDR_CDIFF(pkt);
  unsigned int dtype = dfh->data_type;
  Out_List *cur_out;
  Packet   *cur_pkt;
  hdr_cdiff *cur_dfh;
  hdr_ip   *cur_iph;
 
  if (org_type_ == BCAST_ORG) {
    MACprepare(pkt, MAC_BROADCAST, NS_AF_ILINK, 0);
    MACsend(pkt, JITTER*Random::uniform(1.0));

#ifdef DEBUG_RATE 
    hdr_cmn *cmh = HDR_CMN(pkt);
    printf("DF node %x will send %s (%x, %x, %d) to %x\n",
	   THIS_NODE, MsgStr[dfh->mess_type], (dfh->sender_id).addr_,
	   (dfh->sender_id).port_, dfh->pk_num, cmh->next_hop());
#endif  // DEBUG_RATE

    return;
  }

  if (org_type_ == UNICAST_ORG) {
    for (cur_out = routing_table[dtype].active; cur_out!= NULL; 
       cur_out = OUT_NEXT(cur_out)) {
      if (GRADIENT(cur_out) == ORIGINAL) {

	cur_pkt       = pkt->copy();
	cur_iph       = HDR_IP(cur_pkt);
	cur_iph->dst_ = AGT_ADDR(cur_out);

	cur_dfh       = HDR_CDIFF(cur_pkt);
	cur_dfh->forward_agent_id = here_;
	cur_dfh->num_next = 1;
	cur_dfh->next_nodes[0] = NODE_ADDR(cur_out);

	cur_out->num_data_send++;

	MACprepare(cur_pkt, NODE_ADDR(cur_out), NS_AF_INET, 
		 MAC_RETRY_);
	MACsend(cur_pkt, 0);      
 
#ifdef DEBUG_RATE
	cur_cmh = HDR_CMN(cur_pkt);
	printf("DF node %x will send %s (%x, %x, %d) to %x\n",
	     THIS_NODE, MsgStr[cur_dfh->mess_type], 
	     (cur_dfh->sender_id).addr_, (cur_dfh->sender_id).port_, 
	     cur_dfh->pk_num, cur_cmh->next_hop());
#endif // DEBUG_RATE

      } // endif
    }   // endfor

    Packet::free(pkt);
    return; 
  }    // endif unicast original
}


void DiffusionRate::FwdData(Packet *pkt)
{
  hdr_cdiff *dfh = HDR_CDIFF(pkt);
  unsigned int dtype = dfh->data_type;
  nsaddr_t forwarder_node;
  ns_addr_t forward_agent;
  bool forward_flag;

  forwarder_node = (dfh->forward_agent_id).addr_;
  forward_agent = dfh->forward_agent_id;

  if (dfh->report_rate == SUB_SAMPLED) {
    forward_flag = FwdSubsample(pkt);
    TriggerPosReinf(pkt, forward_agent);

    if (forward_flag == false) {
      Packet::free(pkt);
    }
    return;
  } 

  // Then, report rate is ORIGINAL here.

  if (routing_table[dtype].ExistOriginalGradient() == false
      && routing_table[dtype].sink == NULL) {
    
    if (THIS_NODE != forwarder_node && NEG_REINF_ == true) {
      BcastNeg(dtype);
      routing_table[dtype].new_org_counter = 0;
      routing_table[dtype].ClrAllNewOrg();
      routing_table[dtype].ClrAllOldOrg();
   }
    Packet::free(pkt);
    return;
  }

  if (routing_table[dtype].ExistOriginalGradient() == false) {
    Packet::free(pkt);
    return;
  }

  FwdOriginal(pkt);
}


void DiffusionRate::DataReqAll(unsigned int dtype, int report_rate)
{
  Agent_List *cur_agent;
  Packet *pkt;
  hdr_cdiff *dfh;

  for (cur_agent=routing_table[dtype].source; cur_agent != NULL; 
       cur_agent = AGENT_NEXT(cur_agent) ) {
    pkt = prepare_message(dtype, AGT_ADDR(cur_agent), DATA_REQUEST);
    dfh = HDR_CDIFF(pkt);
    dfh->report_rate = report_rate;
    send_to_dmux(pkt, 0);
  }
}


void DiffusionRate::GenNeg(int dtype)
{
  In_List *cur;

  if (neg_thr_type_ == NEG_ABSOLUTE) {
    for (cur= routing_table[dtype].iif; cur != NULL; cur= IN_NEXT(cur)) {
      if (NEW_ORG_RECV(cur) <= 0 && OLD_ORG_RECV(cur) > MAX_DUP_DATA) {
	UcastNeg(dtype, AGT_ADDR(cur));
	cur->num_neg_send++;
      }
    }
    return;
  }

  if (neg_thr_type_ == NEG_RELATIVE) {
    int most= routing_table[dtype].MostRecvOrg();

    for (cur= routing_table[dtype].iif; cur != NULL; cur= IN_NEXT(cur)) {
      if (OLD_ORG_RECV(cur) > MAX_DUP_DATA &&
	  NEW_ORG_RECV(cur) <= NEG_MIN_RATIO*most) {
	UcastNeg(dtype, AGT_ADDR(cur));
	cur->num_neg_send++;
      }
    }
    return;
  }
}


void DiffusionRate::BcastNeg(int dtype)
{
    ns_addr_t bcast_addr;
    bcast_addr.addr_ = MAC_BROADCAST;
    bcast_addr.port_ = ROUTING_PORT;

    Packet *pkt=prepare_message(dtype, bcast_addr, NEG_REINFORCE);

      MACprepare(pkt, MAC_BROADCAST, NS_AF_ILINK, 0);
      MACsend(pkt, 0);
      overhead++;
      num_neg_bcast_send++;

#ifdef DEBUG_RATE
      hdr_cdiff *dfh = HDR_CDIFF(pkt);
      hdr_cmn *cmh = HDR_CMN(pkt);
      printf("DF node %d will send %s (%x, %x, %d) to %x\n",
	     THIS_NODE, MsgStr[dfh->mess_type], (dfh->sender_id).addr_, 
	     (dfh->sender_id).port_, dfh->pk_num, cmh->next_hop());
#endif // DEBUG_RATE

}


void DiffusionRate::UcastNeg(int dtype, ns_addr_t to)
{
      Packet *pkt=prepare_message(dtype, to, NEG_REINFORCE);
      MACprepare(pkt, to.addr_, NS_AF_INET, 0);
      MACsend(pkt, 0);
      overhead++;

#ifdef DEBUG_RATE
      hdr_cdiff *dfh = HDR_CDIFF(pkt);
      hdr_cmn *cmh = HDR_CMN(pkt);
      printf("DF node %d will send %s (%x, %x, %d) to %x\n",
	     THIS_NODE, MsgStr[dfh->mess_type], (dfh->sender_id).addr_, 
	     (dfh->sender_id).port_, dfh->pk_num, cmh->next_hop());
#endif

}


void DiffusionRate::ProcessNegReinf(Packet *pkt)
{
  hdr_cdiff *dfh = HDR_CDIFF(pkt);
  unsigned int dtype = dfh->data_type;
  Out_List *cur_out;
  PrvCurPtr RetVal;

  RetVal=INTF_FIND(routing_table[dtype].active, dfh->forward_agent_id);

  if (RetVal.cur == NULL) {
    Packet::free(pkt);
    return;
  }

  cur_out = (Out_List *)(RetVal.cur);
  if (GRADIENT(cur_out) == SUB_SAMPLED) {
      Packet::free(pkt);
      return;
  }
    
  GRADIENT(cur_out) = SUB_SAMPLED;

  if (routing_table[dtype].ExistOriginalGradient() == false && 
      routing_table[dtype].sink == NULL) {

    DataReqAll(dtype, SUB_SAMPLED);    
    
    if (NEG_REINF_ == true) {
      BcastNeg(dtype);
      routing_table[dtype].new_org_counter = 0;
      routing_table[dtype].ClrAllNewOrg();
      routing_table[dtype].ClrAllOldOrg();
    }
  }

  Packet::free(pkt);
}


void DiffusionRate::ProcessPosReinf(Packet *pkt)
{
  hdr_cdiff *dfh= HDR_CDIFF(pkt);
  unsigned int dtype = dfh->data_type;
  Out_List *cur_out, *OutPtr;
  PrvCurPtr RetVal;

  RetVal=INTF_FIND(routing_table[dtype].active, dfh->forward_agent_id);

  if (RetVal.cur != NULL) {
    cur_out = (Out_List *)(RetVal.cur);
    GRADIENT(cur_out) = ORIGINAL;
    GRAD_TMOUT(RetVal.cur) = max(GRAD_TMOUT(RetVal.cur),
				   dfh->ts_ + INTEREST_TIMEOUT);
    NUM_POS_RECV(cur_out)++;
  } else {

      OutPtr = new Out_List;
      AGT_ADDR(OutPtr) = dfh->forward_agent_id;
      GRADIENT(OutPtr) = dfh->report_rate;
      GRAD_TMOUT(OutPtr) = dfh->ts_ + INTEREST_TIMEOUT;
      INTF_INSERT(routing_table[dtype].active, OutPtr);
      routing_table[dtype].num_active ++;	  
      NUM_POS_RECV(OutPtr)++;
  }
  
  DataReqAll(dtype, ORIGINAL);


  Pkt_Hash_Entry *hashPtr;
  nsaddr_t next_node;
  In_List *recent_in;
  In_List  *cur;

  switch(pos_type_) {

    case POS_HASH: 
      hashPtr=PktTable.GetHash(dfh->info.sender, dfh->info.seq);
      if (hashPtr == NULL) {
	perror("Hey! I've never seen that packet before.\n");
	Packet::free(pkt);
	exit(-1);
      }

      next_node = (hashPtr->forwarder_id).addr_;
      if (next_node == THIS_NODE) {
	Packet::free(pkt);
	return;
      }

      PosReinf(dtype, hashPtr->forwarder_id.addr_, dfh->info.sender,
		 dfh->info.seq);
      routing_table[dtype].CntPosSend(hashPtr->forwarder_id);
      routing_table[dtype].ClrNewSub(hashPtr->forwarder_id);

#ifdef DEBUG_RATE
      printf("DF node %d will send %s to %x\n",
	 THIS_NODE, MsgStr[dfh->mess_type], hashPtr->forwarder_id.addr_);
#endif // DEBUG_RATE

      Packet::free(pkt);
      return;


  case POS_LAST:
    recent_in = routing_table[dtype].MostRecentIn();
    if (recent_in == NULL) {
      Packet::free(pkt);
      return;
    }

    next_node = NODE_ADDR(recent_in);
    if (next_node == THIS_NODE) {
      Packet::free(pkt);
      return;
    }

    PosReinf(dtype, NODE_ADDR(recent_in), dfh->info.sender, dfh->info.seq);
    routing_table[dtype].CntPosSend(AGT_ADDR(recent_in));
    routing_table[dtype].ClrNewSub(AGT_ADDR(recent_in));

#ifdef DEBUG_RATE
    printf("DF node %d will send %s to %x\n",
	 THIS_NODE, MsgStr[dfh->mess_type], NODE_ADDR(recent_in));
#endif // DEBUG_RATE

    Packet::free(pkt);
    return;


  case POS_ALL:
    for (cur = routing_table[dtype].iif; cur!=NULL; cur = IN_NEXT(cur)) {

      if (NEW_SUB_RECV(cur) <= 0) {
	continue;
      }		
	       
      next_node = NODE_ADDR(cur);

      if (next_node == THIS_NODE) {
	continue;
      }

      PosReinf(dtype, NODE_ADDR(cur), dfh->info.sender, dfh->info.seq);
      routing_table[dtype].CntPosSend(AGT_ADDR(cur));
      routing_table[dtype].ClrNewSub(AGT_ADDR(cur));

#ifdef DEBUG_RATE
      printf("DF node %d will send %s to %x\n",
	 THIS_NODE, MsgStr[dfh->mess_type], NODE_ADDR(cur));
#endif // DEBUG_RATE

    }
    Packet::free(pkt);
    return;

  default: 
    Packet::free(pkt);
    return;
  }
}


void DiffusionRate::PosReinf(int dtype, nsaddr_t to_node, 
		     ns_addr_t info_sender, unsigned int info_seq)
{
  ns_addr_t to_agent_addr;
  to_agent_addr.addr_ = to_node;
  to_agent_addr.port_ = ROUTING_PORT;

  Packet *pkt=prepare_message(dtype, to_agent_addr, POS_REINFORCE);
  hdr_cdiff *dfh = HDR_CDIFF(pkt);

      dfh->report_rate = ORIGINAL;
      dfh->info.sender = info_sender;
      dfh->info.seq = info_seq;
      
      MACprepare(pkt, to_node, NS_AF_INET, 1);
      MACsend(pkt, 0);
      overhead++;

#ifdef DEBUG_RATE
      hdr_cmn *cmh = HDR_CMN(pkt);
      printf("DF node %d will send %s (%x, %x, %d) to %x\n",
	     THIS_NODE, MsgStr[dfh->mess_type], (dfh->sender_id).addr_,
	     (dfh->sender_id).port_, dfh->pk_num, cmh->next_hop());
#endif

}


void DiffusionRate::Start()
{
  DiffusionAgent::Start();

  gradient_timer = new GradientTimer(this);
  gradient_timer->resched(INTEREST_TIMEOUT);

  if ( neg_win_type_ == NEG_TIMER && NEG_REINF_ == true) {
    neg_reinf_timer = new NegativeReinforceTimer(this);
    neg_reinf_timer->resched(NEG_CHECK);
  }
}


void DiffusionRate::reset()
{
  DiffusionAgent::reset();
  DataTable.reset();
}


void DiffusionRate::Print_IOlist()
{
  Out_List *cur_out;
  In_List *cur_in;
  int     i;

  for (i=0; i<1; i++) {
    printf("Node %d DATA TYPE %d: send bcast data %d, not send  %d, rcv %d\n", 
	   THIS_NODE, i, num_data_bcast_send, num_not_send_bcast_data,
	   num_data_bcast_rcv);
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
      printf("Node %d recv new sub %d,new org %d,old org %d:send neg %d pos %d\n", 
	     THIS_NODE, TOTAL_NEW_SUB_RECV(cur_in), TOTAL_NEW_ORG_RECV(cur_in),
	     TOTAL_OLD_ORG_RECV(cur_in), NUM_NEG_SEND(cur_in), 
	     NUM_POS_SEND(cur_in));
    }

  }
}


int DiffusionRate::command(int argc, const char*const*argv) 
{
  if (argc == 2) {
    if (strcasecmp(argv[1], "enable-suppression") == 0) {
      DUP_SUP_ = true;
      return TCL_OK;
    }

    if (strcasecmp(argv[1], "disable-suppression") == 0) {
      DUP_SUP_ = false;
      return TCL_OK;
    }

  }

  else if (argc == 3) {
    
    if (strcasecmp(argv[1], "set-sub-tx-type") == 0 ) {
      sub_type_ = ParseSubType(argv[2]);
      return TCL_OK;
    }

    if (strcasecmp(argv[1], "set-org-tx-type") == 0 ) {
      org_type_ = ParseOrgType(argv[2]);
      return TCL_OK;
    }

    if (strcasecmp(argv[1], "set-pos-type") == 0 ) {
      pos_type_ = ParsePosType(argv[2]);
      return TCL_OK;
    }

    if (strcasecmp(argv[1], "set-pos-node-type") == 0 ) {
      pos_node_type_ = ParsePosNodeType(argv[2]);
      return TCL_OK;
    }

    if (strcasecmp(argv[1], "set-neg-win-type") == 0 ) {
      neg_win_type_ = ParseNegWinType(argv[2]);
      return TCL_OK;
    }

    if (strcasecmp(argv[1], "set-neg-thr-type") == 0 ) {
      neg_thr_type_ = ParseNegThrType(argv[2]);
      return TCL_OK;
    }
    
    if (strcasecmp(argv[1], "set-neg-max-type") == 0 ) {
      neg_max_type_ = ParseNegMaxType(argv[2]);
      return TCL_OK;
    }
  }


  return DiffusionAgent::command(argc, argv);
}



sub_t ParseSubType(const char* str)
{
  if (strcasecmp(str, "BROADCAST") == 0) {
    return BCAST_SUB;
  }

  if (strcasecmp(str, "UNICAST") == 0) {
    return UNICAST_SUB;
  }

  fprintf(stderr,"ParseSubType Error -- Only BROADCAST or UNICAST\n");
  exit(-1);
}



org_t ParseOrgType(const char* str)
{
  if (strcasecmp(str, "BROADCAST") == 0) {
    return BCAST_ORG;
  }

  if (strcasecmp(str, "UNICAST") == 0) {
    return UNICAST_ORG;
  }

  fprintf(stderr,"ParseOrgType Error -- Only BROADCAST or UNICAST\n");
  exit(-1);
}


pos_t ParsePosType(const char* str)
{
  if (strcasecmp(str, "HASH") == 0) {
    return POS_HASH;
  }

  if (strcasecmp(str, "LAST") == 0) {
    return POS_LAST;
  }

  if (strcasecmp(str, "ALL") == 0) {
    return POS_ALL;
  }

  fprintf(stderr,"ParsePosType Error -- Only HASH, LAST, or ALL\n");
  exit(-1);
}


pos_ndt ParsePosNodeType(const char* str)
{
  if (strcasecmp(str, "END") == 0) {
    return END_POS;
  }

  if (strcasecmp(str, "INTM") == 0) {
    return INTM_POS;
  }

  fprintf(stderr,"ParsePosNodeType Error -- Only END or INTM\n");
  exit(-1);
}


neg_wint ParseNegWinType(const char* str)
{
  if (strcasecmp(str, "COUNTER") == 0) {
    return NEG_COUNTER;
  }

  if (strcasecmp(str, "TIMER") == 0) {
    return NEG_TIMER;
  }

  fprintf(stderr,"ParseNegWinType Error -- Only COUNTER or TIMER\n");
  exit(-1);
}


neg_tht ParseNegThrType(const char* str)
{
  if (strcasecmp(str, "ABSOLUTE") == 0) {
    return NEG_ABSOLUTE;
  }

  if (strcasecmp(str, "RELATIVE") == 0) {
    return NEG_RELATIVE;
  }

  fprintf(stderr,"ParseNegThrType Error -- Only ABSOLUTE or RELATIVE\n");
  exit(-1);
}


neg_mxt ParseNegMaxType(const char* str)
{
  if (strcasecmp(str, "FIXED") == 0) {
    return NEG_FIXED_MAX;
  }

  if (strcasecmp(str, "SCALE") == 0) {
    return NEG_SCALE_MAX;
  }

  fprintf(stderr,"ParseNegMaxType Error -- Only FIXED or SCALE\n");
  exit(-1);
}





