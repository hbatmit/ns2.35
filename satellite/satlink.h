/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1999 Regents of the University of California.
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
 *      This product includes software developed by the MASH Research
 *      Group at the University of California Berkeley.
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
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/satellite/satlink.h,v 1.7 2005/09/21 20:52:47 haldar Exp $
 *
 * Contributed by Tom Henderson, UCB Daedalus Research Group, June 1999
 */

#ifndef ns_satlink_h
#define ns_satlink_h

#include "node.h"
#include "channel.h"
#include "phy.h"
#include "queue.h"
#include "net-interface.h"
#include "timer-handler.h"

#define LINK_HDRSIZE 16 

// Link types
#define LINK_GENERIC 1
#define LINK_GSL_GEO 2
#define LINK_GSL_POLAR 3
#define LINK_GSL_REPEATER 4
#define LINK_GSL 5  
#define LINK_ISL_INTRAPLANE 6
#define LINK_ISL_INTERPLANE 7
#define LINK_ISL_CROSSSEAM 8 

class SatLinkHead;
class SatTrace;
class SatNode;

class SatLL : public LL {
public:
        SatLL() : LL(), arpcache_(-1), arpcachedst_(-1) {}
        virtual void sendDown(Packet* p);
        virtual void sendUp(Packet* p);
        virtual void recv(Packet* p, Handler* h);
	Channel* channel(); // Helper function used for ``ARP''
	SatNode* satnode() {return satnode_; }
protected:
	int command(int argc, const char*const* argv);
	int getRoute(Packet *p);
	SatNode* satnode_;
	// Optimization-- cache the last value of Mac address 
        int arpcache_;  
        int arpcachedst_;
};

///////////////////////////////////////////////////////////////////////

class SatMac;
class MacSendTimer : public TimerHandler {
public:
        MacSendTimer(SatMac *a) : TimerHandler() {a_ = a; }
protected:
        virtual void expire(Event *e);
        SatMac *a_;
};

class MacRecvTimer : public TimerHandler {
public:
        MacRecvTimer(SatMac *a) : TimerHandler() {a_ = a; }
protected:
        virtual void expire(Event *e);
        SatMac *a_;
};

class MacHandlerRcv : public Handler {
public:
        MacHandlerRcv(SatMac* m) : mac_(m) {}
        void handle(Event* e);
protected:
        SatMac* mac_;
};

class SatMac : public Mac {
public:
	SatMac();
	void sendDown(Packet* p);
	void sendUp(Packet *p);
	virtual void send_timer() {}
	virtual void recv_timer() {}

protected:
	int command(int argc, const char*const* argv);
	MacSendTimer send_timer_; 
	MacRecvTimer recv_timer_; 
	SatTrace* drop_trace_; // Trace object for dropped packets in Mac
	int trace_drops_; // OTcl-settable flag for whether we trace drops
	SatTrace* coll_trace_; // Trace object for collisions in Mac
	int trace_collisions_; // OTcl-settable flag for whether we trace coll.
};


class UnslottedAlohaMac : public SatMac {
public:
	UnslottedAlohaMac();
	void sendDown(Packet* p); 
	void sendUp(Packet *p); 
	void send_timer(); 
	void recv_timer(); 
	void end_of_contention(Packet* p);

protected:
	virtual void backoff(double delay=0);
	Packet* snd_pkt_;	// stores packet currently being sent
	Packet* rcv_pkt_;	// stores packet currently being recieved
	MacState tx_state_;	// transmit state (SEND or COLL or IDLE)
	MacState rx_state_;	// receive state (RECV or IDLE)
	int rtx_; 		// # of retransmissions so far
	int rtx_limit_;		// Set in OTcl-- retransmission limit
	double mean_backoff_;	// Set in OTcl-- mean backoff time 
	double send_timeout_;	// Set in OTcl-- time out after this interval
	double end_of_contention_; // Saves time that contention will be over
};

///////////////////////////////////////////////////////////////////////

class SatPhy : public Phy {
 public:
	SatPhy() {}
	void sendDown(Packet *p);
	int sendUp(Packet *p);
 protected:
	int command(int argc, const char*const* argv);
};

class RepeaterPhy : public Phy {
 public:
	RepeaterPhy() {}
	void recv(Packet* p, Handler*);
	void sendDown(Packet *p);
	int sendUp(Packet *) { return 0; } 
 protected:
};

///////////////////////////////////////////////////////////////////////

/*
 * Class SatChannel
 */
class SatChannel : public Channel{
friend class SatRouteObject;
 public:
	SatChannel(void);
	int getId() { return index_; }
	void add_interface(Phy*);
	void remove_interface(Phy*);
	int find_peer_mac_addr(int);

 protected:
	double get_pdelay(Node* tnode, Node* rnode);
 private:

};

///////////////////////////////////////////////////////////////////////

class SatNode;
class ErrorModel;
/*
 * For now, this is the API used in the satellite networking code (hopefully
 * a more general one will follow).
 */
class SatLinkHead : public LinkHead {
public:
	SatLinkHead(); 
	// This builds on the API provided by LinkHead
	// (Note: the following pointers are not part of a generic API and may 
	// be changed in the future
	SatPhy* phy_tx() { return phy_tx_; }
	SatPhy* phy_rx() { return phy_rx_; }
	SatMac* mac() { return mac_; }
	SatLL* satll() { return satll_; }
	Queue* queue() { return queue_; }
	ErrorModel* errmodel() { return errmodel_; }
	int linkup_;
	SatNode* node() { return ((SatNode*) node_); }
	
protected:
	virtual int command(int argc, const char*const* argv); 
	SatPhy* phy_tx_;
	SatPhy* phy_rx_;
	SatMac* mac_;
	SatLL* satll_;
	Queue* queue_;
	ErrorModel* errmodel_;

};

#endif
