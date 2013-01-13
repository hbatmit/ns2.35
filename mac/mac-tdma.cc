// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-

/*
 * mac-tdma.cc
 * Copyright (C) 1999 by the University of Southern California
 * $Id: mac-tdma.cc,v 1.18 2011/10/02 22:32:34 tom_henderson Exp $
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

//
// $Header: /cvsroot/nsnam/ns-2/mac/mac-tdma.cc,v 1.18 2011/10/02 22:32:34 tom_henderson Exp $
//
// mac-tdma.cc
// by Xuan Chen (xuanc@isi.edu), ISI/USC
//
// Preamble TDMA MAC layer for single hop.
// Centralized slot assignment computing.


#include "delay.h"
#include "connector.h"
#include "packet.h"
#include "random.h"

// #define DEBUG

//#include <debug.h>

#include "arp.h"
#include "ll.h"
#include "mac.h"
#include "mac-tdma.h"
#include "wireless-phy.h"
#include "cmu-trace.h"

#include <stddef.h>

#define SET_RX_STATE(x)			\
{					\
	rx_state_ = (x);			\
}

#define SET_TX_STATE(x)				\
{						\
	tx_state_ = (x);				\
}

/* Phy specs from 802.11 */
static PHY_MIB PMIB = {
	DSSS_CWMin, DSSS_CWMax, DSSS_SlotTime, DSSS_CCATime,
	DSSS_RxTxTurnaroundTime, DSSS_SIFSTime, DSSS_PreambleLength,
	DSSS_PLCPHeaderLength
};	

/* Timers */
void MacTdmaTimer::start(Packet *p, double time)
{
	Scheduler &s = Scheduler::instance();
	assert(busy_ == 0);
  
	busy_ = 1;
	paused_ = 0;
	stime = s.clock();
	rtime = time;
	assert(rtime >= 0.0);
  
	s.schedule(this, p, rtime);
}

void MacTdmaTimer::stop(Packet *p) 
{
	Scheduler &s = Scheduler::instance();
	assert(busy_);
  
	if(paused_ == 0)
		s.cancel((Event *)p);

	// Should free the packet p.
	Packet::free(p);
  
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;
}

/* Slot timer for TDMA scheduling. */
void SlotTdmaTimer::handle(Event *e)
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;
  
	mac->slotHandler(e);
}

/* Receive Timer */
void RxPktTdmaTimer::handle(Event *e) 
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;
  
	mac->recvHandler(e);
}

/* Send Timer */
void TxPktTdmaTimer::handle(Event *e) 
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;
	
	mac->sendHandler(e);
}

/* ======================================================================
   TCL Hooks for the simulator
   ====================================================================== */
static class MacTdmaClass : public TclClass {
public:
	MacTdmaClass() : TclClass("Mac/Tdma") {}
	TclObject* create(int, const char*const*) {
		return (new MacTdma(&PMIB));
	}
} class_mac_tdma;


// Mac Tdma definitions
// Frame format:
// Pamble Slot1 Slot2 Slot3...
MacTdma::MacTdma(PHY_MIB* p) : 
	Mac(), mhSlot_(this), mhTxPkt_(this), mhRxPkt_(this){
	/* Global variables setting. */
	// Setup the phy specs.
	phymib_ = p;
	
	/* Get the parameters of the link (which in bound in mac.cc, 2M by default),
	   the packet length within one TDMA slot (1500 byte by default), 
	   and the max number of nodes (64) in the simulations.*/
	bind("slot_packet_len_", &slot_packet_len_);
	bind("max_node_num_", &max_node_num_);
	
	//  slot_packet_len_ = 1500;
	//  max_node_num_ = 64;
	// Calculate the slot time based on the MAX allowed data length.
	slot_time_ = DATA_Time(slot_packet_len_);
	
	/* Calsulate the max slot num within on frame from max node num.
	   In the simple case now, they are just equal. 
	*/
	max_slot_num_ = max_node_num_;
	
	/* Much simplified centralized scheduling algorithm for single hop
	   topology, like WLAN etc. 
	*/
	// Initualize the tdma schedule and preamble data structure.
	tdma_schedule_ = new int[max_slot_num_];
	tdma_preamble_ = new int[max_slot_num_];

	/* Do each node's initialization. */
	// Record the initial active node number.
	active_node_++;

	if (active_node_ > max_node_num_) {
		printf("Too many nodes taking part in the simulations, aborting...\n");
		exit(-1);
	}
    
	// Initial channel / transceiver states. 
	tx_state_ = rx_state_ = MAC_IDLE;
	tx_active_ = 0;

	// Initialy, the radio is off. NOTE: can't use radioSwitch(OFF) here.
	radio_active_ = 0;

	// Do slot scheduling.
	re_schedule();

	/* Deal with preamble. */
	// Can't send anything in the first frame.
	slot_count_ = FIRST_ROUND;
	tdma_preamble_[slot_num_] = NOTHING_TO_SEND;

	//Start the Slot timer..
	mhSlot_.start((Packet *) (& intr_), 0);  
}

/* similar to 802.11, no cached node lookup. */
int MacTdma::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "log-target") == 0) {
			logtarget_ = (NsObject*) TclObject::lookup(argv[2]);
			if(logtarget_ == 0)
				return TCL_ERROR;
			return TCL_OK;
		}
	}
	return Mac::command(argc, argv);
}


/* ======================================================================
   Debugging Routines
   ====================================================================== */
void MacTdma::trace_pkt(Packet *p) 
{
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_mac_tdma* dh = HDR_MAC_TDMA(p);
	u_int16_t *t = (u_int16_t*) &dh->dh_fc;

	fprintf(stderr, "\t[ %2x %2x %2x %2x ] %x %s %d\n",
		*t, dh->dh_duration,
		ETHER_ADDR(dh->dh_da), ETHER_ADDR(dh->dh_sa),
		index_, packet_info.name(ch->ptype()), ch->size());
}

void MacTdma::dump(char *fname)
{
	fprintf(stderr, "\n%s --- (INDEX: %d, time: %2.9f)\n", fname, 
		index_, Scheduler::instance().clock());
	
	fprintf(stderr, "\ttx_state_: %x, rx_state_: %x, idle: %d\n", 
		tx_state_, rx_state_, is_idle());
	fprintf(stderr, "\tpktTx_: %lx, pktRx_: %lx, callback: %lx\n", 
		(long) pktTx_, (long) pktRx_, (long) callback_);
}


/* ======================================================================
   Packet Headers Routines
   ====================================================================== */
int MacTdma::hdr_dst(char* hdr, int dst )
{
	struct hdr_mac_tdma *dh = (struct hdr_mac_tdma*) hdr;
	if(dst > -2)
		STORE4BYTE(&dst, (dh->dh_da));
	return ETHER_ADDR(dh->dh_da);
}

int MacTdma::hdr_src(char* hdr, int src )
{
	struct hdr_mac_tdma *dh = (struct hdr_mac_tdma*) hdr;
	if(src > -2)
		STORE4BYTE(&src, (dh->dh_sa));
  
	return ETHER_ADDR(dh->dh_sa);
}

int MacTdma::hdr_type(char* hdr, u_int16_t type) 
{
	struct hdr_mac_tdma *dh = (struct hdr_mac_tdma*) hdr;
	if(type)
		STORE2BYTE(&type,(dh->dh_body));
	return GET2BYTE(dh->dh_body);
}

/* Test if the channel is idle. */
int MacTdma::is_idle() {
	if(rx_state_ != MAC_IDLE)
		return 0;
	if(tx_state_ != MAC_IDLE)
		return 0;
	return 1;
}

/* Do the slot re-scheduling:
   The idea of postphone the slot scheduling for one slot time may be useful.
*/
void MacTdma::re_schedule() {
	static int slot_pointer = 0;
	// Record the start time of the new schedule.
	start_time_ = NOW;
	/* Seperate slot_num_ and the node id: 
	   we may have flexibility as node number changes.
	*/
	slot_num_ = slot_pointer++;
	tdma_schedule_[slot_num_] = (char) index_;
}

/* To handle incoming packet. */
void MacTdma::recv(Packet* p, Handler* h) {
	struct hdr_cmn *ch = HDR_CMN(p);
	
	/* Incoming packets from phy layer, send UP to ll layer. 
	   Now, it is in receiving mode. 
	*/
	if (ch->direction() == hdr_cmn::UP) {
		// Since we can't really turn the radio off at lower level, 
		// we just discard the packet.
		if (!radio_active_) {
			free(p);
			//printf("<%d>, %f, I am sleeping...\n", index_, NOW);
			return;
		}

		sendUp(p);
		//printf("<%d> packet recved: %d\n", index_, tdma_pr_++);
		return;
	}
	
	/* Packets coming down from ll layer (from ifq actually),
	   send them to phy layer. 
	   Now, it is in transmitting mode. */
	callback_ = h;
	state(MAC_SEND);
	sendDown(p);
	//printf("<%d> packet sent down: %d\n", index_, tdma_ps_++);
}

void MacTdma::sendUp(Packet* p) 
{
	struct hdr_cmn *ch = HDR_CMN(p);

	/* Can't receive while transmitting. Should not happen...?*/
	if (tx_state_ && ch->error() == 0) {
		printf("<%d>, can't receive while transmitting!\n", index_);
		ch->error() = 1;
	};

	/* Detect if there is any collision happened. should not happen...?*/
	if (rx_state_ == MAC_IDLE) {
		SET_RX_STATE(MAC_RECV);     // Change the state to recv.
		pktRx_ = p;                 // Save the packet for timer reference.

		/* Schedule the reception of this packet, 
		   since we just see the packet header. */
		double rtime = TX_Time(p);
		assert(rtime >= 0);

		/* Start the timer for receiving, will end when receiving finishes. */
		mhRxPkt_.start(p, rtime);
	} else {
		/* Note: we don't take the channel status into account, 
		   as collision should not happen...
		*/
		printf("<%d>, receiving, but the channel is not idle....???\n", index_);
	}
}

/* Actually receive data packet when RxPktTimer times out. */
void MacTdma::recvDATA(Packet *p){
	/*Adjust the MAC packet size: strip off the mac header.*/
	struct hdr_cmn *ch = HDR_CMN(p);
	ch->size() -= ETHER_HDR_LEN;
	ch->num_forwards() += 1;

	/* Pass the packet up to the link-layer.*/
	uptarget_->recv(p, (Handler*) 0);
}

/* Send packet down to the physical layer. 
   Need to calculate a certain time slot for transmission. */
void MacTdma::sendDown(Packet* p) {
	struct hdr_cmn* ch = HDR_CMN(p);
	struct hdr_mac_tdma* dh = HDR_MAC_TDMA(p);

	/* Update the MAC header, same as 802.11 */
	ch->size() += ETHER_HDR_LEN;

	dh->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
	dh->dh_fc.fc_type       = MAC_Type_Data;
	dh->dh_fc.fc_subtype    = MAC_Subtype_Data;
	
	dh->dh_fc.fc_to_ds      = 0;
	dh->dh_fc.fc_from_ds    = 0;
	dh->dh_fc.fc_more_frag  = 0;
	dh->dh_fc.fc_retry      = 0;
	dh->dh_fc.fc_pwr_mgt    = 0;
	dh->dh_fc.fc_more_data  = 0;
	dh->dh_fc.fc_wep        = 0;
	dh->dh_fc.fc_order      = 0;

	if((u_int32_t)ETHER_ADDR(dh->dh_da) != MAC_BROADCAST)
		dh->dh_duration = DATA_DURATION;
	else
		dh->dh_duration = 0;

	/* buffer the packet to be sent. */
	pktTx_ = p;
}

/* Actually send the packet. */
void MacTdma::send() 
{
	struct hdr_cmn* ch;
	double stime;

	/* Check if there is any packet buffered. */
	if (!pktTx_) {
		printf("<%d>, %f, no packet buffered.\n", index_, NOW);
		return;
	}

	/* Perform carrier sence...should not be collision...? */
	if(!is_idle()) {
		/* Note: we don't take the channel status into account, ie. no collision,
		   as collision should not happen...
		*/
		printf("<%d>, %f, transmitting, but the channel is not idle...???\n", index_, NOW);
		return;
	}

	ch = HDR_CMN(pktTx_);

	stime = TX_Time(pktTx_);
	ch->txtime() = stime;
	
	/* Turn on the radio and transmit! */
	SET_TX_STATE(MAC_SEND);						     
	radioSwitch(ON);

	/* Start a timer that expires when the packet transmission is complete. */
	mhTxPkt_.start(pktTx_->copy(), stime);
	downtarget_->recv(pktTx_, this);        

	pktTx_ = 0;
}

// Turn on / off the radio
void MacTdma::radioSwitch(int i) 
{
	radio_active_ = i;
	//EnergyModel *em = netif_->node()->energy_model();
	if (i == ON) {
		//if (em && em->sleep())
		//em->set_node_sleep(0);
		//    printf("<%d>, %f, turn radio ON\n", index_, NOW); 
		Phy *p;
		p = netif_;
		((WirelessPhy *)p)->node_wakeup();
		return;
	}

	if (i == OFF) {
		//if (em && !em->sleep()) {
		//em->set_node_sleep(1);
		//    netif_->node()->set_node_state(INROUTE);
		Phy *p;
		p = netif_;
		((WirelessPhy *)p)->node_sleep();
		//    printf("<%d>, %f, turn radio OFF\n", index_, NOW);
		return;
	}
}

// make the new preamble.
void MacTdma::makePreamble() 
{
	u_int32_t dst;
	struct hdr_mac_tdma* dh;
	
	// If there is a packet buffered, file its destination to preamble.
	if (pktTx_) {
		dh = HDR_MAC_TDMA(pktTx_);  
		dst = ETHER_ADDR(dh->dh_da);
		//printf("<%d>, %f, write %d to slot %d in preamble\n", index_, NOW, dst, slot_num_);
		tdma_preamble_[slot_num_] = dst;
	} else {
		//printf("<%d>, %f, write NO_PKT to slot %d in preamble\n", index_, NOW, slot_num_);
		tdma_preamble_[slot_num_] = NOTHING_TO_SEND;
	}
}

/* Timers' handlers */
/* Slot Timer:
   For the preamble calculation, we should have it:
   occupy one slot time,
   radio turned on for the whole slot.
*/
void MacTdma::slotHandler(Event *e) 
{
	// Restart timer for next slot.
	mhSlot_.start((Packet *)e, slot_time_);

	// Make a new presamble for next frame.
	if ((slot_count_ == active_node_) || (slot_count_ == FIRST_ROUND)) {
		//printf("<%d>, %f, make the new preamble now.\n", index_, NOW);
		// We should turn the radio on for the whole slot time.
		radioSwitch(ON);

		makePreamble();
		slot_count_ = 0;
		return;
	}

	// If it is the sending slot for me.
	if (slot_count_ == slot_num_) {
		//printf("<%d>, %f, time to send.\n", index_, NOW);
		// We have to check the preamble first to avoid the packets coming in the middle.
		if (tdma_preamble_[slot_num_] != NOTHING_TO_SEND)
			send();
		else
			radioSwitch(OFF);

		slot_count_++;
		return;
	}
 
	// If I am supposed to listen in this slot
	if ((tdma_preamble_[slot_count_] == index_) || ((u_int32_t)tdma_preamble_[slot_count_] == MAC_BROADCAST)) {
		//printf("<%d>, %f, preamble[%d]=%d, I am supposed to receive now.\n", index_, NOW, slot_count_, tdma_preamble_[slot_count_]);
		slot_count_++;

		// Wake up the receive packets.
		radioSwitch(ON);
		return;
	}

	// If I dont send / recv, do nothing.
	//printf("<%d>, %f, preamble[%d]=%d, nothing to do now.\n", index_, NOW, slot_count_, tdma_preamble_[slot_count_]);
	radioSwitch(OFF);
	slot_count_++;
	return;
}

void MacTdma::recvHandler(Event *) 
{
	u_int32_t dst;
	struct hdr_cmn *ch = HDR_CMN(pktRx_);
	struct hdr_mac_tdma *dh = HDR_MAC_TDMA(pktRx_);

	/* Check if any collision happened while receiving. */
	if (rx_state_ == MAC_COLL) 
		ch->error() = 1;

	SET_RX_STATE(MAC_IDLE);
  
	/* check if this packet was unicast and not intended for me, drop it.*/   
	dst = ETHER_ADDR(dh->dh_da);

	// Turn the radio off after receiving the whole packet
	radioSwitch(OFF);

	/* Ordinary operations on the incoming packet */
	// Not a pcket destinated to me.
	if ((dst != MAC_BROADCAST) && (dst != (u_int32_t)index_)) {
		drop(pktRx_);
		return;
	}
  
	/* Now forward packet upwards. */
	recvDATA(pktRx_);
}

/* After transmission a certain packet. Turn off the radio. */
void MacTdma::sendHandler(Event *e) 
{
	//  printf("<%d>, %f, send a packet finished.\n", index_, NOW);

	/* Once transmission is complete, drop the packet. 
	   p  is just for schedule a event. */
	SET_TX_STATE(MAC_IDLE);
	Packet::free((Packet *)e);
  
	// Turn off the radio after sending the whole packet
	radioSwitch(OFF);

	/* unlock IFQ. */
	if(callback_) {
		Handler *h = callback_;
		callback_ = 0;
		h->handle((Event*) 0);
	} 
}
