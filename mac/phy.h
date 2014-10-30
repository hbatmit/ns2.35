/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
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
 *
 * Ported from CMU/Monarch's code, nov'98 -Padma Haldar.
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/mac/phy.h,v 1.7 2000/08/17 00:03:38 haoboy Exp $
 *
 * superclass for all network interfaces
 
 ===================================================================

 * Phy represents the hardware that actually controls the channel
 * access for the node. Phy transmits/receives pkts from the channel
 * to which it is connected. No pkts are buffered at this layer as
 * the decision to send has already been made and the packet is on
 * its way to the "Channel".
 *==================================================================
 */

#ifndef ns_phy_h
#define ns_phy_h

#include <assert.h>
#include "bi-connector.h"
#include "lib/bsd-list.h"

class Phy;
LIST_HEAD(if_head, Phy);

#include "channel.h"
#include "node.h"
#include "mac.h"

class Node;
class LinkHead;
/*--------------------------------------------------------------
  Phy : Base class for all network interfaces used to control
  channel access
 ---------------------------------------------------------------*/

class Phy : public BiConnector {
 public:
	Phy();

	void recv(Packet* p, Handler* h);
	
	virtual void sendDown(Packet *p)=0;
	
	virtual int sendUp(Packet *p)=0;

	inline double  txtime(Packet *p) {
		return (hdr_cmn::access(p)->size() * 8.0) / bandwidth_; }
	inline double txtime(int bytes) {
		return (8.0 * bytes / bandwidth_); }
	virtual double  bittime() const { return 1/bandwidth_; }

	// list of all network interfaces on a channel
	Phy* nextchnl(void) const { return chnl_link_.le_next; }
	inline void insertchnl(struct if_head *head) {
		LIST_INSERT_HEAD(head, this, chnl_link_);
		//channel_ = chnl;
	}
	// list of all network interfaces on a node
	Phy* nextnode(void) const { return node_link_.le_next; }
	inline void insertnode(struct if_head* head) {
		LIST_INSERT_HEAD(head, this, node_link_);
		//node_ = node;
	}
	inline void removechnl() {
		LIST_REMOVE(this, chnl_link_);
	}
	void setchnl (Channel *chnl) { channel_ = chnl; }
	virtual void setnode (Node *node) { node_ = node; }
	virtual Node* node(void) const { return node_; }
 	virtual Channel* channel(void) const {return channel_;}	
	
	virtual void    dump(void) const;
	LinkHead* head() { return head_; }

 protected:
	//void		drop(Packet *p);
	int		command(int argc, const char*const* argv);
	int             index_;
	
	Node* node_;
	LinkHead* head_; // the entry point of this network stack
  
	/*
   * A list of all "network interfaces" on a given channel.
   * Note: a node may have multiple interfaces, each of which
   * is on a different channel.
   */
	LIST_ENTRY(Phy) chnl_link_;

  /*
   * A list of all "network interfaces" for a given node.
   * Each interface is assoicated with exactly one node
   * and one channel.
   */
	LIST_ENTRY(Phy) node_link_;
	
/* ============================================================
     Physical Layer State
   ============================================================ */
	
	
	double bandwidth_;                   // bit rate
	Channel         *channel_;    // the channel for output

};


#endif // ns_phy_h
