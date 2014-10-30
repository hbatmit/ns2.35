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
 * Contributed by the Daedalus Research Group, http://daedalus.cs.berkeley.edu
 *
 */

/*
  This is an implementation of the ARQ feature used in HDLC working in ABM (Asynchronous Balanced Mode) which provides a reliable mode of link transfer using acknowledgements.
First cut implementation; 
*/

#ifndef ns_hdlc_h
#define ns_hdlc_h

#include <satlink.h>
#include <timer-handler.h>
#include <drop-tail.h>



//#define HDLC_MWS  1024
//#define HDLC_MWS  8
#define HDLC_MWS 65536
#define HDLC_MWM (HDLC_MWS-1)
#define HDLC_HDR_LEN      sizeof(struct hdr_hdlc);
#define DELAY_ACK_VAL     0.5     // arbitrarily chosen val for which acks are delayed

enum SS_t {RR=0, REJ=1, RNR=2, SREJ=3};
enum COMMAND_t {SNRM, SNRME, SARM, SARME, SABM, SABME, DISC, UA, DM};
enum HDLCFrameType { HDLC_I_frame, HDLC_S_frame, HDLC_U_frame };

class HDLC;
class HdlcTimer;

struct ARQstate {

	int nh_;             // next-hop
	int t_seqno_;        // tx'ing seq no
	int seqno_;          // counter for seq'ing incoming data pkts
	int maxseq_;         // highest seq no sent so far
	int highest_ack_;    // highest ack recvd so far
	int recv_seqno_;     // seq no of data pkts recvd by recvr

	int nrexmit_;        // num of retransmits
	int ntimeouts_;      // num of retx timeouts
	bool closed_;        // whether this connection is closed
	int disconnect_;     
	bool sentDISC_;
	
	int SABME_req_;     // whether a SABME request has been sent or not
	bool sentREJ_;       // to prevent sending dup REJ
	
	Packet *save_;       // packet saved for delayed ack to allow piggybacking
	// at receiving side hold pkts until they are all recvd in order
	// and can be fwded to layers above LL
	Packet** seen_;      // array of pkts seen
	int nextpkt_;       // next packet expected
	int maxseen_;    // max pkt number seen
	
	HdlcTimer *rtx_timer_;
	HdlcTimer *reset_timer_;
	HdlcTimer *delay_timer_; 
	
        // buffer to hold outgoing the pkts until they are ack'ed 
	PacketQueue sendBuf_;
	
	ARQstate *next_;
	
};

class HdlcTimer : public TimerHandler {
public:
	HdlcTimer(HDLC *agent, ARQstate *a, void (HDLC::*callback)(ARQstate *a)) 
		: agent_(agent), a_(a), callback_(callback) {};
	
protected:
	virtual void expire(Event *e);
	HDLC *agent_;
	ARQstate *a_;
	void (HDLC::*callback_)(ARQstate *a);
};

struct HDLCControlFrame {
	int recv_seq;
	int send_seq;
	char frame_command;
};

struct I_frame {
	int recv_seqno;
	bool poll_final_bit;
	int send_seqno;
};
                                                                                                                    
struct S_frame {
        int recv_seqno;
	bool poll_final_bit;
        SS_t stype;
};
                                                                                                                    
struct U_frame {
	bool poll_final_bit;
	COMMAND_t utype;
};



// The hdlc hdr share the same hdr space with hdr_ll
struct hdr_hdlc {
	int saddr_;                            // src mac id
	int daddr_;                           // destination mac id
	HDLCControlFrame hdlc_fc_;
	HDLCFrameType fc_type_;
	int padding_;

	// static int offset_;
//  	inline int& offset() { return offset_; }
//  	static hdr_hdlc* access(const Packet* p) {
//  		return (hdr_hdlc*) p->access(offset_);
//  	}

	inline int saddr() {return saddr_;}
	inline int daddr() {return daddr_;}
	
};

// XXXXXXXXX
// Currently, we assume the link layer shall have one p-2-p connection
// at any given time

// If this needs to change the ARQstate defined below should be used
// to support multiple p-2-p connections, each state for a src-dst pair.



	

// Brief description of HDLC model used :
// Type of station:
// combined station capable of issuing both commands and responses, 

// Link configurations:
// Consists of combined stations and supports both full-duplex and half-duplex transmission. HAlf-duplex for now; will be extended to full-duplex in the future.

// Data transfer mode: asynchronous balanced mode extended (ABME). Either station may initiate transmission without receiving permission from the other combined station. In this mode the frames are numbered and are acknowledged by the receiver. HDLC defines the extended mode to support 7 bits (128bit modulo numbering) for sequence numbers. For our simulation we use 31 bits.

// Initialization: consists of the sender sending a U frame issuing the SABME command (set asynchronous balanced mode/extended); the receiver sends back an UA (unnumbered ackowledged) or can send a DM (disconnected mode) to reject connection setup.

// Data sending: done using I frames by sender. receiver sends back RR S frames indicating the next I frame it expects. S frames are used for both error control as well as flow control. It sends a REJ for any missing frame asking receiver to go-back-N. It can send RNR asking recvr not to send any frames until it again sends RR. It also sends SREJ asking retx of a specific missing pkt.

// Disconnect:
//        Either HDLC entity can initiate a disconnect using a disconnect (DISC) frame. The remote entity must accept the disconnect by replying with a UA and informing its layer 3 user that the connection has been terminated. Any outstanding unacknowledged frames may be recovered by higher layers.


class HDLC : public SatLL {
	friend class HdlcTimer;
	friend class ARQstate;
	
public:
	HDLC();
	virtual void recv(Packet* p, Handler* h);
	inline virtual void hdr_dst(Packet *p, int macDA) { HDR_HDLC(p)->daddr_ = macDA;}
	virtual void timeout(ARQstate *s);

protected:
	// main sending and recving routines
	void recvIncoming(Packet* p);
	void recvOutgoing(Packet* p);
	
	// misc routines
	void inSendBuffer(Packet *p, ARQstate *a);
	int resolveAddr(Packet *p);
	void reset(ARQstate *a);
	
	// queue 
	Packet *getPkt(PacketQueue buf, int seq);
	
	// rtx timer 
	void reset_rtx_timer(ARQstate *a, int backoff);
	void set_rtx_timer(ARQstate *a);
	void cancel_rtx_timer(ARQstate *a) { (a->rtx_timer_)->force_cancel(); }
	void rtt_backoff();
	double rtt_timeout();
	void delayTimeout(ARQstate *s);
	void cancel_delay_timer(ARQstate *a) { (a->delay_timer_)->force_cancel();}
	double reset_timeout();
	void set_reset_timer(ARQstate *a);
	void cancel_reset_timer(ARQstate *a) { (a->reset_timer_)->force_cancel(); }

	// sending routines
	void sendDown(Packet *p);
	void sendMuch(ARQstate *a);
	void output(Packet *p, ARQstate *a, int seqno);
	//Packet *dataToSend(Packet *p);
	void ack(Packet *p);
	Packet *dataToSend(ARQstate *a);
	//void ack();
	void sendUA(Packet *p, COMMAND_t cmd);
	void sendRR(Packet *p, ARQstate *a);
	void sendRNR(Packet *p);
	void sendREJ(Packet *p, ARQstate *a);
	void sendSREJ(Packet *p, int seq);
	void sendDISC(Packet *p);
	
	// receive routines
	void recvIframe(Packet *p);
	void recvSframe(Packet *p);
	void recvUframe(Packet *p);
	void handleSABMErequest(Packet *p);
	void handleUA(Packet *p);
	void handleDISC(Packet *p);
	void handleRR(Packet *p);    
	void handleRNR(Packet *p);
	void handleREJ(Packet *p);
	void handleSREJ(Packet *p);
	void handlePiggyAck(Packet *p, ARQstate *a);

	// go back N error recovery
	void goBackNMode(Packet *p);
	
	// selective repeat  mode
	void selectiveRepeatMode(Packet *p);

	// per src-dst connection state
	ARQstate *newEntry(int next_hop);
	ARQstate *createState(int next_hop);
	ARQstate *checkState(int next_hop); // get state, if no state, create
	void removeState(int nh_); // remove from list of states

	// event-tracing
	//virtual void trace_event(char *eventtype);
	//EventTrace *et_;
	
	// variables
	static int uidcnt_;
	int wndmask_ ;       // window mask
	int wnd_;            // size of window; set from tcl
	int queueSize_;
	double timeout_;        // value of timeout
	int maxTimeouts_;    // max num of timeouts before connection closed
	int selRepeat_;     // if this is set then selective repeat mode is used	                     // otherwise Go Back N mode is used by default
	int delAckVal_;      // time for which ack is delayed 
	int delAck_;        // flag for delayed ack
	
	ARQstate *list_head_;
	
	
};

#endif
