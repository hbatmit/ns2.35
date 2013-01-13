/*
 * Copyright (c) 1997 The Regents of the University of California.
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
 *  This product includes software developed by the Network Research
 *  Group at Lawrence Berkeley National Laboratory.
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
 * @(#) $Header: /cvsroot/nsnam/ns-2/baytcp/tcp-full-bay.h,v 1.3 2006/05/30 21:38:42 pradkin Exp $ (LBL)
 */

#ifndef ns_tcp_full_h
#define ns_tcp_full_h

#include "tcp.h"

/*
 * these defines are directly from tcp_var.h in "real" TCP
 * they are used in the 'tcp_flags_' member variable
 */

#define TF_ACKNOW       0x0001          /* ack peer immediately */
#define TF_DELACK       0x0002          /* ack, but try to delay it */
#define TF_NODELAY      0x0004          /* don't delay packets to coalesce */
#define TF_NOOPT        0x0008          /* don't use tcp options */
#define TF_SENTFIN      0x0010          /* have sent FIN */
#define TF_SENTSYN      0x0020          /* have sent SYN */

#define TCPS_CLOSED             0       /* closed */
#define TCPS_LISTEN             1       /* listening for connection */
#define TCPS_SYN_SENT           2       /* active, have sent syn */
#define TCPS_SYN_RECEIVED       3       /* have send and received syn */
#define TCPS_ESTABLISHED        4       /* established */
#define TCPS_FIN_WAIT_1         6       /* have closed, sent fin */
#define TCPS_CLOSING            7       /* closed xchd FIN; await FIN ACK */
#define TCPS_LAST_ACK           8       /* had fin and close; await FIN ACK */
#define TCPS_FIN_WAIT_2         9       /* have closed, fin is acked */

#define TCPIP_BASE_PKTSIZE      40      /* base TCP/IP header in real life */
/* these are used to mark packets as to why we xmitted them */
#define REASON_NORMAL   0  
#define REASON_TIMEOUT  1
#define REASON_DUPACK   2

/* bits for the tcp_flags field below */
/* from tcp.h in the "real" implementation */
/* RST and URG are not used in the simulator */
 
#define TH_FIN  0x01        /* FIN: closing a connection */
#define TH_SYN  0x02        /* SYN: starting a connection */
#define TH_PUSH 0x08        /* PUSH: used here to "deliver" data */
#define TH_ACK  0x10        /* ACK: ack number is valid */

#define PF_TIMEOUT 0x04		/* protocol defined */

/*
 * inserted by kedar
 */

class BayFullTcpAgent;

class BayDelAckTimer : public TimerHandler {
public:
    BayDelAckTimer(BayFullTcpAgent *a) : TimerHandler(), a_(a) { }
protected:
    virtual void expire(Event *);
    BayFullTcpAgent *a_;
};
 
class BayReassemblyQueue : public TcpAgent {
    struct seginfo {
        seginfo* next_;
        seginfo* prev_;
        int startseq_;
        int endseq_;
	int flags_;
    };
public:
    BayReassemblyQueue(int& nxt) : head_(NULL), tail_(NULL),
        rcv_nxt_(nxt) { }
    int empty() { return (head_ == NULL); }
    int add(Packet*);
    void clear();
protected:
    seginfo* head_;
    seginfo* tail_;
    int& rcv_nxt_;
};

/* Added by kmn 8/5/97. A hack to create a template base class for
 * application agents that expect to talk to full tcps. Don't want
 * to do anything fancy since the vint folks should come up with
 * something one of these days.
 * kmn - 8/12/97 yet another hack added to keep a list of agents
 */
 /* vint project never came through. Added in to recv 6/8/00 -kmn */

 #define DATA_PUSH 1
 #define CONNECTION_END 2

class BayTcpAppAgent : public Agent {
 public:
 	BayTcpAppAgent(packet_t ptype) : Agent(ptype) {}
	virtual void recv(Packet*, BayFullTcpAgent*, int) {}
};

class BayFullTcpAgent : public TcpAgent {
 public:
    BayFullTcpAgent();
    ~BayFullTcpAgent();
    void delay_bind_init_all();
    int delay_bind_dispatch(const char *varName, const char *localName,
    			    TclObject *tracer);
    virtual void recv(Packet *pkt, Handler*);
    virtual void timeout(int tno); 	// tcp_timers() in real code
    void advance(int);
    int advance(int, int);	//added 7/30/97 by kmn to pass bytes,
			 	//	set close_on_empty_
    int command(int argc, const char*const* argv);
    int state() { return state_; }

 protected:
    int segs_per_ack_;  // for window updates
    int nodelay_;       // disable sender-side Nagle?
    int data_on_syn_;   // send data on initial SYN?
    int tcprexmtthresh_;    // fast retransmit threshold
    int iss_;       // initial send seq number
    int dupseg_fix_;    // fix bug with dup segs and dup acks?
    int dupack_reset_;  // zero dupacks on dataful dup acks?
    double delack_interval_;

    int headersize();   // a tcp header
    int outflags();     // state-specific tcp header flags
    int predict_ok(Packet*); // predicate for recv-side header prediction
    
    void fast_retransmit(int);  // do a fast-retransmit on specified seg
    inline double now() { return Scheduler::instance().clock(); }

    void reset_rtx_timer(int);  // adjust the rtx timer
    void reset();       // reset to a known point
    void reinit();       // reset to a known point
    void connect();     // do active open
    void listen();      // do passive open
    void usrclosed();   // user requested a close
    int need_send();    // need to send ACK/win-update now?
    void sendpacket(int seqno, int ackno, int pflags, int datalen, int reason, Packet *p=0);
    void output(int seqno, int reason = 0); // output 1 packet
    void send_much(int force, int reason, int maxburst = 0);
    void newack(Packet* pkt);   // process an ACK
    void cancel_rtx_timeout();  // cancel the rtx timeout
    
    /*
     * the following are part of a tcpcb in "real" RFC793 TCP
     */
    int maxseg_;        /* MSS */
    int flags_;     /* controls next output() call */
    int state_;     /* enumerated type: FSM state */
    int rcv_nxt_;       /* next sequence number expected */
    BayReassemblyQueue rq_;    /* TCP reassembly queue */
    /*
     * the following are part of a tcpcb in "real" RFC1323 TCP
     */
    int last_ack_sent_; /* ackno field from last segment we sent */
    /*
     * added 7/30/97 by kmn to allow connection to close with FINs
     *	when empties data and to allow calling recv() on application
     */
    int close_on_empty_;
    BayTcpAppAgent* app_;
    //added 8/12 to start in ack per packet, switch to set value
    int switch_spa_thresh_;
    int first_data_;
    int recover_cause_;
    /*
     *inserted by kedar
     */
    BayDelAckTimer delack_timer_;
};

class BayFullTcpList {
public:
	BayFullTcpList() : tcp(0), next(0) {}
	BayFullTcpAgent* tcp;
	BayFullTcpList* next;
};

#ifdef notdef
class NewRenoBayFullTcpAgent : public BayFullTcpAgent {
public:
	NewRenoBayFullTcpAgent();
protected:
	int	save_maxburst_;		// saved value of maxburst_
	int	recov_maxburst_;	// maxburst lim during recovery
	void pack_action(Packet*);
	void ack_action(Packet*);
};

class TahoeBayFullTcpAgent : public BayFullTcpAgent {
protected:
	void dupack_action();
};

class SackBayFullTcpAgent : public BayFullTcpAgent {
public:
	SackBayFullTcpAgent();
	~SackBayFullTcpAgent();
	void	recv(Packet*, Handler*);
protected:
	int build_options(hdr_tcp*);	// insert opts, return len
	int sack_option_size_;	// base # bytes for sack opt (no blks)
	int sack_block_size_;	// # bytes in a sack block (def: 8)
	int sack_min_;		// first seq# in sack queue
	int sack_max_;		// highest seq# seen in any sack block
	int sack_nxt_;		// next seq# to hole-fill
	int max_sack_blocks_;	// max # sack blocks to send

	BayReassemblyQueue sq_;	// SACK queue, used by sender

	void	reset();
	void	sendpacket(int seqno, int ackno, int pflags, int datalen, int reason);
	void	send_much(int force, int reason, int maxburst = 0);
	void	send_holes(int force, int maxburst);
	void	pack_action(Packet*);
	void	ack_action(Packet*);
	void	dupack_action();
	int	hdrsize(int nblks);
	void	timeout_action();
};
#endif

#endif
