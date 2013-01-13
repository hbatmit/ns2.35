/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
 *
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
 * $Header: /cvsroot/nsnam/ns-2/mac/mac-802_11.h,v 1.30 2010/03/08 05:54:52 tom_henderson Exp $
 *
 * Ported from CMU/Monarch's code, nov'98 -Padma.
 * wireless-mac-802_11.h
 */

#ifndef ns_mac_80211_h
#define ns_mac_80211_h

// Added by Sushmita to support event tracing (singal@nunki.usc.edu)
#include "address.h"
#include "ip.h"

#include "mac-timers.h"
#include "marshall.h"
#include <math.h>
#include <stddef.h>
#include <list>

class EventTrace;

#define GET_ETHER_TYPE(x)		GET2BYTE((x))
#define SET_ETHER_TYPE(x,y)            {u_int16_t t = (y); STORE2BYTE(x,&t);}

/* ======================================================================
   Frame Formats
   ====================================================================== */

#define	MAC_ProtocolVersion	0x00

#define MAC_Type_Management	0x00
#define MAC_Type_Control	0x01
#define MAC_Type_Data		0x02
#define MAC_Type_Reserved	0x03

#define MAC_Subtype_RTS		0x0B
#define MAC_Subtype_CTS		0x0C
#define MAC_Subtype_ACK		0x0D
#define MAC_Subtype_Data	0x00

#define MAC_Subtype_80211_Beacon	0x08 
#define MAC_Subtype_AssocReq	0x00
#define MAC_Subtype_AssocRep	0x01
#define MAC_Subtype_Auth	0x0C
#define MAC_Subtype_ProbeReq	0x04
#define MAC_Subtype_ProbeRep	0x05


struct frame_control {
	u_char		fc_subtype		: 4;
	u_char		fc_type			: 2;
	u_char		fc_protocol_version	: 2;

	u_char		fc_order		: 1;
	u_char		fc_wep			: 1;
	u_char		fc_more_data		: 1;
	u_char		fc_pwr_mgt		: 1;
	u_char		fc_retry		: 1;
	u_char		fc_more_frag		: 1;
	u_char		fc_from_ds		: 1;
	u_char		fc_to_ds		: 1;
};

struct rts_frame {
	struct frame_control	rf_fc;
	u_int16_t		rf_duration;
	u_char			rf_ra[ETHER_ADDR_LEN];
	u_char			rf_ta[ETHER_ADDR_LEN];
	u_char			rf_fcs[ETHER_FCS_LEN];
};

struct cts_frame {
	struct frame_control	cf_fc;
	u_int16_t		cf_duration;
	u_char			cf_ra[ETHER_ADDR_LEN];
	u_char			cf_fcs[ETHER_FCS_LEN];
};

struct ack_frame {
	struct frame_control	af_fc;
	u_int16_t		af_duration;
	u_char			af_ra[ETHER_ADDR_LEN];
	u_char			af_fcs[ETHER_FCS_LEN];
};

struct beacon_frame {		
	struct frame_control	bf_fc;
	u_int16_t		bf_duration;
	u_char			bf_ra[ETHER_ADDR_LEN];
	u_char			bf_ta[ETHER_ADDR_LEN];
	u_char			bf_3a[ETHER_ADDR_LEN];
	u_int16_t		bf_scontrol;
	double			bf_timestamp;
	double			bf_bcninterval;
	u_int8_t		bf_datarates[1];
	u_char			bf_fcs[ETHER_FCS_LEN];
};

struct assocreq_frame {
	struct frame_control	acrqf_fc;
	u_int16_t		acrqf_duration;
	u_char			acrqf_ra[ETHER_ADDR_LEN];
	u_char			acrqf_ta[ETHER_ADDR_LEN];
	u_char			acrqf_3a[ETHER_ADDR_LEN];
	u_int16_t		acrqf_scontrol;
	u_char			acrqf_fcs[ETHER_FCS_LEN];
};

struct assocrep_frame {
	struct frame_control	acrpf_fc;
	u_int16_t		acrpf_duration;
	u_char			acrpf_ra[ETHER_ADDR_LEN];
	u_char			acrpf_ta[ETHER_ADDR_LEN];
	u_char			acrpf_3a[ETHER_ADDR_LEN];
	u_int16_t		acrpf_scontrol;
	u_int16_t		acrpf_statuscode;
	u_char			acrpf_fcs[ETHER_FCS_LEN];
};

struct auth_frame {
	struct frame_control	authf_fc;
	u_int16_t		authf_duration;
	u_char			authf_ra[ETHER_ADDR_LEN];
	u_char			authf_ta[ETHER_ADDR_LEN];
	u_char			authf_3a[ETHER_ADDR_LEN];
	u_int16_t		authf_scontrol;
	u_int16_t		authf_algono;
	u_int16_t		authf_seqno;
	u_int16_t		authf_statuscode;
	u_char			authf_fcs[ETHER_FCS_LEN];
};

struct probereq_frame {
	struct frame_control	prrqf_fc;
	u_int16_t		prrqf_duration;
	u_char			prrqf_ra[ETHER_ADDR_LEN];
	u_char			prrqf_ta[ETHER_ADDR_LEN];
	u_char			prrqf_3a[ETHER_ADDR_LEN];
	u_int16_t		prrqf_scontrol;
	u_char			prrqf_fcs[ETHER_FCS_LEN];
};

struct proberep_frame {
	struct frame_control	prrpf_fc;
	u_int16_t		prrpf_duration;
	u_char			prrpf_ra[ETHER_ADDR_LEN];
	u_char			prrpf_ta[ETHER_ADDR_LEN];
	u_char			prrpf_3a[ETHER_ADDR_LEN];
	u_int16_t		prrpf_scontrol;
	double			prrpf_timestamp;
	double			prrpf_bcninterval;
	u_int8_t		prrpf_datarates[1];
	u_char			prrpf_fcs[ETHER_FCS_LEN];
};



// XXX This header does not have its header access function because it shares
// the same header space with hdr_mac.
struct hdr_mac802_11 {
	struct frame_control	dh_fc;
	u_int16_t		dh_duration;
	u_char                  dh_ra[ETHER_ADDR_LEN];
        u_char                  dh_ta[ETHER_ADDR_LEN];
        u_char                  dh_3a[ETHER_ADDR_LEN];
	u_char			dh_4a[ETHER_ADDR_LEN];
	u_int16_t		dh_scontrol;
	u_char			dh_body[1]; // size of 1 for ANSI compatibility
};

struct client_table {
	int client_id;
	int auth_status;
	int assoc_status;
};

struct ap_table {
	int ap_id;
	double ap_power;
};

struct priority_queue {
	int frame_priority;
	struct priority_queue *next;
};
/* ======================================================================
   Definitions
   ====================================================================== */

/* Must account for propagation delays added by the channel model when
 * calculating tx timeouts (as set in tcl/lan/ns-mac.tcl).
 *   -- Gavin Holland, March 2002
 */
#define DSSS_MaxPropagationDelay        0.000002        // 2us   XXXX

class PHY_MIB {
public:
	PHY_MIB(Mac802_11 *parent);

	inline u_int32_t getCWMin() { return(CWMin); }
        inline u_int32_t getCWMax() { return(CWMax); }
	inline double getSlotTime() { return(SlotTime); }
	inline double getBeaconInterval() { return(BeaconInterval); }
	inline double getSIFS() { return(SIFSTime); }
	inline double getPIFS() { return(SIFSTime + SlotTime); }
	inline double getDIFS() { return(SIFSTime + 2 * SlotTime); }
	inline double getEIFS() {
		// see (802.11-1999, 9.2.10)
		return(SIFSTime + getDIFS()
                       + (8 *  getACKlen())/PLCPDataRate);
	}
	inline u_int32_t getPreambleLength() { return(PreambleLength); }
	inline double getPLCPDataRate() { return(PLCPDataRate); }
	
	
	inline u_int32_t getPLCPhdrLen() {
		return((PreambleLength + PLCPHeaderLength) >> 3);
	}

	inline u_int32_t getHdrLen11() {
		return(getPLCPhdrLen() + offsetof(struct hdr_mac802_11, dh_body[0])
                       + ETHER_FCS_LEN);
	}
	
	inline u_int32_t getRTSlen() {
		return(getPLCPhdrLen() + sizeof(struct rts_frame));
	}
	
	inline u_int32_t getCTSlen() {
		return(getPLCPhdrLen() + sizeof(struct cts_frame));
	}
	
	inline u_int32_t getACKlen() {
		return(getPLCPhdrLen() + sizeof(struct ack_frame));
	}
	inline u_int32_t getBEACONlen() {		
		return(getPLCPhdrLen() + sizeof(struct beacon_frame)); 
	}
	inline u_int32_t getASSOCREQlen() {			
		return(getPLCPhdrLen() + sizeof(struct assocreq_frame));
	}
	inline u_int32_t getASSOCREPlen() {		
		return(getPLCPhdrLen() + sizeof(struct assocrep_frame)); 
	}
	inline u_int32_t getAUTHENTICATElen() {		
		return(getPLCPhdrLen() + sizeof(struct auth_frame)); 
	}
	inline u_int32_t getPROBEREQlen() {	
		return(getPLCPhdrLen() + sizeof(struct probereq_frame)); 
	}
	inline u_int32_t getPROBEREPlen() {		
		return(getPLCPhdrLen() + sizeof(struct proberep_frame)); 
	}
 private:


	u_int32_t	CWMin;
	u_int32_t	CWMax;
	double		SlotTime;
	double		SIFSTime;
	double		BeaconInterval;
	u_int32_t	PreambleLength;
	u_int32_t	PLCPHeaderLength;
	double		PLCPDataRate;
};


/*
 * IEEE 802.11 Spec, section 11.4.4.2
 *      - default values for the MAC Attributes
 */
#define MAC_FragmentationThreshold	2346		// bytes
#define MAC_MaxTransmitMSDULifetime	512		// time units
#define MAC_MaxReceiveLifetime		512		// time units




class MAC_MIB {
public:

	MAC_MIB(Mac802_11 *parent);

private:
	u_int32_t	RTSThreshold;
	u_int32_t	ShortRetryLimit;
	u_int32_t	LongRetryLimit;
	u_int32_t	ScanType;
	double		ProbeDelay;
	double		MaxChannelTime;
	double		MinChannelTime;
	double		ChannelTime;
	
public:
	u_int32_t	FailedCount;	
	u_int32_t	RTSFailureCount;
	u_int32_t	ACKFailureCount;
 public:
       inline u_int32_t getRTSThreshold() { return(RTSThreshold);}
       inline u_int32_t getShortRetryLimit() { return(ShortRetryLimit);}
       inline u_int32_t getLongRetryLimit() { return(LongRetryLimit);}
       inline u_int32_t getScanType() { return(ScanType);}	
       inline double getProbeDelay() { return(ProbeDelay);}
       inline double getMaxChannelTime() { return(MaxChannelTime);}
       inline double getMinChannelTime() { return(MinChannelTime);}
       inline double getChannelTime() { return(ChannelTime);}
};


/* ======================================================================
   The following destination class is used for duplicate detection.
   ====================================================================== */
class Host {
public:
	LIST_ENTRY(Host) link;
	u_int32_t	index;
	u_int32_t	seqno;
};


/* ======================================================================
   The actual 802.11 MAC class.
   ====================================================================== */
class Mac802_11 : public Mac {
	friend class DeferTimer;

	friend class BeaconTimer; 
	friend class ProbeTimer;
	friend class BackoffTimer;
	friend class IFTimer;
	friend class NavTimer;
	friend class RxTimer;
	friend class TxTimer;
public:
	Mac802_11();
	void		recv(Packet *p, Handler *h);
	inline int	hdr_dst(char* hdr, int dst = -2);
	inline int	hdr_src(char* hdr, int src = -2);
	inline int	hdr_type(char* hdr, u_int16_t type = 0);
	
	inline int bss_id() { return bss_id_; }
	
	// Added by Sushmita to support event tracing
        void trace_event(char *, Packet *);
        EventTrace *et_;

protected:
	void	backoffHandler(void);
	void	deferHandler(void);
	void	BeaconHandler(void); 
	void	ProbeHandler(void);
	void	navHandler(void);
	void	recvHandler(void);
	void	sendHandler(void);
	void	txHandler(void);

private:
	void	update_client_table(int num, int auth_status, int assoc_status);			
	int	find_client(int num);	
	void	update_ap_table(int num, double power);	
	int 	strongest_ap();
	int	find_ap(int num, double power);
	void 	deletelist();
	void	passive_scan();	
	void	active_scan();
	void	checkAssocAuthStatus();
	int	command(int argc, const char*const* argv);
	

	void 	add_priority_queue(int num);
	void 	push_priority(int num);
	void 	delete_lastnode();
	void	shift_priority_queue();



	/* In support of bug fix described at
	 * http://www.dei.unipd.it/wdyn/?IDsezione=2435	 
	 */
	int bugFix_timer_;
	int infra_mode_;
	double BeaconTxtime_;
	int associated;
	int authenticated;
	int handoff;
	double Pr;
	int ap_temp;
	int ap_addr;
	int tx_mgmt_;
	int associating_node_;
	int authenticating_node_;
	int ScanType_;
	int OnMinChannelTime;
	int OnMaxChannelTime;
	int Recv_Busy_;
	int probe_delay;
	/*
	 * Called by the timers.
	 */
	void		recv_timer(void);
	void		send_timer(void);
	int		check_pktCTRL();
	int		check_pktRTS();
	int		check_pktTx();
	int		check_pktASSOCREQ();  
	int		check_pktASSOCREP();
	int		check_pktBEACON();
	int		check_pktAUTHENTICATE();
	int		check_pktPROBEREQ();  
	int		check_pktPROBEREP();
	
	/*
	 * Packet Transmission Functions.
	 */
	void	send(Packet *p, Handler *h);
	void 	sendRTS(int dst);
	void	sendCTS(int dst, double duration);
	void	sendACK(int dst);
	void	sendDATA(Packet *p);
	void	sendBEACON(int src);		
	void	sendASSOCREQ(int dst);
	void	sendASSOCREP(int dst);
	void	sendPROBEREQ(int dst);
	void	sendPROBEREP(int dst);
	void	sendAUTHENTICATE(int dst);
	void	RetransmitRTS();
	void	RetransmitDATA();
	void	RetransmitASSOCREP();
	void	RetransmitPROBEREP();
	void	RetransmitAUTHENTICATE();

	/*
	 * Packet Reception Functions.
	 */
	void	recvRTS(Packet *p);
	void	recvCTS(Packet *p);
	void	recvACK(Packet *p);
	void	recvDATA(Packet *p);
	void	recvBEACON(Packet *p);		
	void	recvASSOCREQ(Packet *p);
	void	recvASSOCREP(Packet *p);
	void	recvPROBEREQ(Packet *p);
	void	recvPROBEREP(Packet *p);
	void	recvAUTHENTICATE(Packet *p);	

	void		capture(Packet *p);
	void		collision(Packet *p);
	void		discard(Packet *p, const char* why);
	void		rx_resume(void);
	void		tx_resume(void);

	inline int	is_idle(void);

	/*
	 * Debugging Functions.
	 */
	void		trace_pkt(Packet *p);
	void		dump(char* fname);

	inline int initialized() {	
		return (cache_ && logtarget_
                        && Mac::initialized());
	}

	inline void mac_log(Packet *p) {
                logtarget_->recv(p, (Handler*) 0);
        }

	double txtime(Packet *p);
	double txtime(double psz, double drt);
	double txtime(int bytes);

	inline void transmit(Packet *p, double timeout);
	inline void checkBackoffTimer(void);
	inline void postBackoff(int pri);
	inline void setRxState(MacState newState);
	inline void setTxState(MacState newState);


	inline void inc_cw() {
		cw_ = (cw_ << 1) + 1;
		if(cw_ > phymib_.getCWMax())
			cw_ = phymib_.getCWMax();
	}
	inline void rst_cw() { cw_ = phymib_.getCWMin(); }

	inline double sec(double t) { return(t *= 1.0e-6); }
	inline u_int16_t usec(double t) {
		u_int16_t us = (u_int16_t)floor((t *= 1e6) + 0.5);
		return us;
	}
	inline void set_nav(u_int16_t us) {
		double now = Scheduler::instance().clock();
		double t = us * 1e-6;
		if((now + t) > nav_) {
			nav_ = now + t;
			if(mhNav_.busy())
				mhNav_.stop();
			mhNav_.start(t);
		}
	}

protected:
	PHY_MIB         phymib_;
        MAC_MIB         macmib_;

       /* the macaddr of my AP in BSS mode; for IBSS mode
        * this is set to a reserved value IBSS_ID - the
        * MAC_BROADCAST reserved value can be used for this
        * purpose
        */
       int     bss_id_;
       enum    {IBSS_ID=MAC_BROADCAST};
       enum    {
		PASSIVE = 0,
       		ACTIVE = 1
		};

private:
	double		basicRate_;
 	double		dataRate_;
	struct client_table	*client_list;	
	struct ap_table	*ap_list;
	struct priority_queue *queue_head;

	/*
	 * Mac Timers
	 */
	IFTimer		mhIF_;		// interface timer
	NavTimer	mhNav_;		// NAV timer
	RxTimer		mhRecv_;		// incoming packets
	TxTimer		mhSend_;		// outgoing packets

	DeferTimer	mhDefer_;	// defer timer
	BackoffTimer	mhBackoff_;	// backoff timer
	BeaconTimer	mhBeacon_;	// Beacon Timer 
	ProbeTimer	mhProbe_;	//Probe timer, 

	/* ============================================================
	   Internal MAC State
	   ============================================================ */
	double		nav_;		// Network Allocation Vector

	MacState	rx_state_;	// incoming state (MAC_RECV or MAC_IDLE)
	MacState	tx_state_;	// outgoint state
	int		tx_active_;	// transmitter is ACTIVE

	Packet          *eotPacket_;    // copy for eot callback

	Packet		*pktRTS_;	// outgoing RTS packet
	Packet		*pktCTRL_;	// outgoing non-RTS packet
	Packet		*pktBEACON_;	//outgoing Beacon Packet
	Packet		*pktASSOCREQ_;	//Association request
	Packet		*pktASSOCREP_;	// Association response
	Packet		*pktAUTHENTICATE_; //Authentication
	Packet		*pktPROBEREQ_;	//Probe Request
	Packet		*pktPROBEREP_;	//Probe Response

	u_int32_t	cw_;		// Contention Window
	u_int32_t	ssrc_;		// STA Short Retry Count
	u_int32_t	slrc_;		// STA Long Retry Count

	int		min_frame_len_;

	NsObject*	logtarget_;
	NsObject*       EOTtarget_;     // given a copy of packet at TX end


	/* ============================================================
	   Duplicate Detection state
	   ============================================================ */
	u_int16_t	sta_seqno_;	// next seqno that I'll use
	int		cache_node_count_;
	Host		*cache_;


	std::list<struct client_table> client_list1;
	std::list<struct ap_table> ap_list1;


};

#endif /* __mac_80211_h__ */

