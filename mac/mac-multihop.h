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
 *	This product includes software developed by the Daedalus Research
 *	Group at the University of California Berkeley.
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
 * @(#) $Header: /cvsroot/nsnam/ns-2/mac/mac-multihop.h,v 1.7 1998/06/27 01:24:07 gnguyen Exp $ (UCB)
 */

#ifndef ns_mac_multihop_h
#define ns_mac_multihop_h

#include "packet.h"
#include "mac.h"
#include "random.h"
#include "ip.h"

#define MAC_POLLSIZE 40		/* completely arbitrary */
#define MAC_POLLACKSIZE 40	/* completely arbitrary */

/* States of MultihopMac protocol */
#define MAC_IDLE 0x01
#define MAC_POLLING 0x02
#define MAC_RCV 0x04
#define MAC_SND 0x08

#define MAC_TICK 16000		/* 1 tick = 1ms; 16 sub-ticks in a tick */


class MultihopMac;

/*
 * PollEvent class to generate poll events in the multihop
 * network prior to sending and receiving packets.
 */

class PollEvent : public Event {
  public:
	PollEvent(MultihopMac* m, MultihopMac* pm) : 
		backoffTime_(0), mac_(m), peerMac_(pm) { }
	int backoffTime_;	/* valid only for NackPoll */
	inline MultihopMac *peerMac() { return peerMac_; }
  protected:
	MultihopMac *mac_;	/* pointer to my mac */
	MultihopMac *peerMac_;	/* pointer to my peer's mac */
};

/*
 * Handle poll events.
 */
class PollHandler : public Handler {
  public:
	PollHandler(MultihopMac* m) { mac_ = m; }
	void handle(Event *);
protected:
	MultihopMac* mac_;
};

/*
 * Handle pollack events.
 */
class PollAckHandler : public Handler {
  public:
	PollAckHandler(MultihopMac* m) { mac_ = m; }
	void handle(Event *);
protected:
	MultihopMac* mac_;
};

/*
 * Handle pollnack events.
 */
class PollNackHandler : public Handler {
  public:
	PollNackHandler(MultihopMac* m) { mac_ = m; }
	void handle(Event *);
protected:
	MultihopMac* mac_;
};

/*
 * Handle poll timeout events.
 */
class PollTimeoutHandler : public Handler {
  public:
	PollTimeoutHandler(MultihopMac* m) { mac_ = m; }
	void handle(Event *);
protected:
	MultihopMac* mac_;
};

class BackoffHandler : public Handler {
  public: 
	BackoffHandler(MultihopMac *m) { mac_ = m; }
	void handle(Event *);
protected:
	MultihopMac* mac_;
};

/*
class MultihopMacHandler : public Handler {
  public:
	MultihopMacHandler(MultihopMac* m) { mac_ = m; }
	void handle(Event *e);
protected:
	MultihopMac* mac_;
};
*/

class MultihopMac : public Mac {
  public:
	MultihopMac();
	void send(Packet*); /* send data packet (assume POLLed) link */
	void recv(Packet *, Handler *); /* call from higher layer (LL) */
	void poll(Packet *); /* poll peer mac */
	int checkInterfaces(int); /* probe state of this and other MACs */
	void schedulePoll(MultihopMac *); /* schedule poll for later */
	inline int mode() { return mode_; }
	inline int mode(int m) { return mode_ = m; }
	inline MultihopMac *peer() { return peer_; }
	inline MultihopMac *peer(MultihopMac *p) { return peer_ = p; }
	inline double tx_rx() { return tx_rx_; } /* access tx_rx time */
	inline double rx_tx() { return rx_tx_; } /* access rx_tx time */
	inline double rx_rx() { return rx_rx_; } /* access rx_rx time */
	inline double backoffBase() { return backoffBase_; }
	inline double backoffTime() { return backoffTime_; }
	inline double backoffTime(double bt) { return backoffTime_ = bt; }
	inline PollEvent *pendingPE() { return pendingPollEvent_; }
	inline PollEvent *pendingPE(PollEvent *pe) { return pendingPollEvent_ = pe; }
	inline Packet *pkt() { return pkt_; }
	inline PollHandler* ph() { return &ph_; } /* access poll handler */
	inline PollAckHandler* pah() { return &pah_; }
	inline PollNackHandler* pnh() { return &pnh_; }
	inline PollTimeoutHandler* pth() { return &pth_; }
	inline BackoffHandler* bh() { return &bh_; }
//	inline MultihopMacHandler* mh() { return &mh_; }
	inline double pollTxtime(int s) { return (double) s*8.0/bandwidth(); }
  protected:
	int mode_;		/* IDLE/SND/RCV */
	MultihopMac *peer_;	/* peer mac */
	double tx_rx_;		/* Turnaround times: transmit-->recv */
	double rx_tx_;		/* recv-->transmit */
	double rx_rx_;		/* recv-->recv */
	double backoffTime_;
	double backoffBase_;
	PollEvent *pendingPollEvent_; /* pending in scheduler */
	Packet *pkt_;		/* packet stored for poll retries */
	PollHandler ph_;	/* handler for POLL events */
	PollAckHandler pah_;	/* handler for POLL_ACK events */
	PollNackHandler pnh_;	/* handler for POLL_NACK events */
	PollTimeoutHandler pth_;/* handler for POLL_TIMEOUT events */
	BackoffHandler bh_;	/* handler for exponential backoffs */
//	MultihopMacHandler mh_;	/* handle receives of data */
};


#endif
