/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1996 Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by the MASH Research
 * 	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Contributed by Rishi Bhargava <rishi_bhargava@yahoo.com> May, 2001.
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <float.h>

#include "object.h"
#include "agent.h"
#include "trace.h"
#include "packet.h"
#include "scheduler.h"
#include "random.h"

#include "mac.h"
#include "ll.h"
#include "cmu-trace.h"

#include "hdr_src.h"
#include "sragent.h"
#include <fstream>




/*===========================================================================
  SRAgent OTcl linkage
---------------------------------------------------------------------------*/
static class SRAgentClass : public TclClass {
public:
  SRAgentClass() : TclClass("Agent/SRAgent") {}
  TclObject* create(int, const char*const*) {
    return (new SRAgent);
  }
} class_SRAgent;

/*===========================================================================
  SRAgent methods
---------------------------------------------------------------------------*/
SRAgent::SRAgent():Agent(PT_TCP), slot_(0), nslot_(0), maxslot_(-1) 
{
  //  cout << " into the constructor " << endl; 
}

SRAgent::~SRAgent()
{
  delete [] slot_;
}



int
SRAgent::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance(); 	

	if(argc == 2)
	  {
	    if (strcmp(argv[1],"reset") == 0)
	      {
		tcl.resultf("reset done");
		return(TCL_OK);
	      }
	  }
	else if(argc == 3) 
	  {
	    if((strcmp(argv[1], "target") == 0))
	      {
		target_ = (NsObject *) TclObject::lookup(argv[2]);
		if (target_ == 0) {
		  tcl.resultf("no such target %s", argv[2]);
		  return(TCL_ERROR);
		}
		tcl.resultf(" The target is successfully set");
		return(TCL_OK);
	      }
	  }
	else if(argc == 4)
	  {
	    if((strcmp(argv[1], "install_slot") == 0))
	      {
		//cout << " Installing " << argv[2] << " in " << argv[3] << endl;
		NsObject* obj = (NsObject*)TclObject::lookup(argv[2]);
		if (obj == 0) 
		  {
		    tcl.resultf("SRAgent: %s lookup of %s failed\n", argv[1], argv[2]);
		    return TCL_ERROR;
		  }
		else
		  {
		    install(atoi(argv[3]), obj);
		    return TCL_OK;
		  }
		
	      }
	  }
	else
	  {

	    if( (strcmp(argv[1], "install_connection") == 0))
	      {
		h_node *node = (h_node *)malloc(sizeof(h_node));
		h_node *list;
		
		node->route_len = argc - 5;
		for (int tmp=0; tmp < node->route_len; tmp++)
		  node->route[tmp] = atoi(argv[5+tmp]);
		node->tag = atoi(argv[2]);
		
		int hash_val = atoi(argv[2])%10;
		list = table[hash_val];
		node->next = list;
		table[hash_val] = node;
		
		return TCL_OK;
		
	      }
	  }
	return (Agent::command(argc,argv));
}

void SRAgent::install(int slot, NsObject* p)
{
	if (slot >= nslot_)
		alloc(slot);
	slot_[slot] = p;
	if (slot >= maxslot_)
		maxslot_ = slot;
}


void
SRAgent::recv(Packet* packet, Handler*)

{
	//  int i = 0;
	//int n;
  hdr_cmn *cmh =  hdr_cmn::access(packet);
  hdr_src *srh =  hdr_src::access(packet);
  hdr_ip *iph = hdr_ip::access(packet);

  assert(cmh->size() >= 0);
  /* insert the route in the header here , also after this the target
   * should have been set.*/
  if (cmh->src_rt_valid)
    {
      //cout << "src rt packet" << endl;
      
      if (srh->cur_addr_ == srh->num_addrs_)
      	{
	  // supposedly dest reached
	  // I can change the packet type and send it to the entry point once again..... this would set things right. This part of the code is managing things at the receiver also..... finally the packet goes to the right agent and not our src rt agent.
	  //cout << " setting the packet tye as tcp" << endl; 
      	  cmh->src_rt_valid = '\0';
      	  send(packet,0);
	  return;
      	}
      
      int slot_no = srh->addrs[srh->cur_addr_++];
      NsObject *node =  slot_[slot_no];
      //cout << "passing to the next hop: " << slot_no << endl;
      
      if (node == NULL)
	{
	  //cout << "src_rt : null node " << slot_no << " accessed" << endl;
	  //cout << "flow id is " << iph->flowid() << endl;
	  exit(0);
	}
      else
	{
	  //cout << "packet of type" << cmh->ptype() << endl;
	  cmh->src_rt_valid = '1';
	  
	  node->recv(packet, (Handler *)0);
	  
	}
      return;
    }
  else
    {
      // packet from the TCP most probably
      h_node *temp_list;
      //cout << " Packet of type: "<< cmh->ptype();

      srh->cur_addr_ = 1;
      int connect_id = iph->flowid();
      //cout << " The flowid is" << connect_id << endl;
      temp_list = table[connect_id%10];

      while (temp_list && temp_list->tag != connect_id)
	temp_list = temp_list->next;
      if (temp_list == NULL)
	{
	  printf(" The connection id is not in routing tabel\n");
	  exit(0);
	}
      
      srh->num_addrs_ = temp_list->route_len;
      // cout << " total number of hops:" << srh->num_addrs_ << endl;
      
      for (int j = 0; j< srh->num_addrs_; j++ )
	{
	  srh->addrs[j] = temp_list->route[j];
	  // cout << " setting the next hop "<< srh->addrs[j];
	}
      //cout << endl;
      //set the target before starting anything in the tcl code
      
      cmh->src_rt_valid = '1';
      //      cout << "setting the type to be src_rt" << endl; 
      
      send(packet,0);
      return;
    }
  
}

void SRAgent::alloc(int slot)
{
	NsObject** old = slot_;
	int n = nslot_;
	if (old == NULL)
		nslot_ = 32;
	while (nslot_ <= slot)
		nslot_ <<= 1;
	slot_ = new NsObject*[nslot_];
	memset(slot_, 0, nslot_ * sizeof(NsObject*));
	for (int i = 0; i < n; ++i)
		slot_[i] = old[i];
	delete [] old;
}
