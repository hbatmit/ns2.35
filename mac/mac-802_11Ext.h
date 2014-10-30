/*
 * Copyright (C) 2007 
 * Mercedes-Benz Research & Development North America, Inc. and
 * University of Karlsruhe (TH)
 * All rights reserved.
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
 
/*
 * This code was designed and developed by:
 * 
 * Qi Chen                 : qi.chen@daimler.com
 * Felix Schmidt-Eisenlohr : felix.schmidt-eisenlohr@kit.edu
 * Daniel Jiang            : daniel.jiang@daimler.com
 * 
 * For further information see: 
 * http://dsn.tm.uni-karlsruhe.de/english/Overhaul_NS-2.php
 */
 
#ifndef ns_mac_80211Ext_h
#define ns_mac_80211Ext_h
#include <stddef.h>
#include "marshall.h"
#include "timer-handler.h"
#define GET_ETHER_TYPE(x)		GET2BYTE((x))
#define SET_ETHER_TYPE(x,y)            {u_int16_t t = (y); STORE2BYTE(x,&t);}
#include "wireless-phyExt.h"

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

struct frame_control {
	u_char fc_subtype : 4;
	u_char fc_type : 2;
	u_char fc_protocol_version : 2;
	u_char fc_order : 1;
	u_char fc_wep : 1;
	u_char fc_more_data : 1;
	u_char fc_pwr_mgt : 1;
	u_char fc_retry : 1;
	u_char fc_more_frag : 1;
	u_char fc_from_ds : 1;
	u_char fc_to_ds : 1;
};

struct rts_frame {
	struct frame_control rf_fc;
	u_int16_t rf_duration;
	u_char rf_ra[ETHER_ADDR_LEN];
	u_char rf_ta[ETHER_ADDR_LEN];
	u_char rf_fcs[ETHER_FCS_LEN];
};

struct cts_frame {
	struct frame_control cf_fc;
	u_int16_t cf_duration;
	u_char cf_ra[ETHER_ADDR_LEN];
	u_char cf_fcs[ETHER_FCS_LEN];
};

struct ack_frame {
	struct frame_control af_fc;
	u_int16_t af_duration;
	u_char af_ra[ETHER_ADDR_LEN];
	u_char af_fcs[ETHER_FCS_LEN];
};

// XXX This header does not have its header access function because it shares
// the same header space with hdr_mac.
struct hdr_mac802_11 {
	struct frame_control dh_fc;
	u_int16_t dh_duration;
	u_char dh_ra[ETHER_ADDR_LEN];
	u_char dh_ta[ETHER_ADDR_LEN];
	u_char dh_3a[ETHER_ADDR_LEN];
	u_int16_t dh_scontrol;
	u_char dh_body[0]; // size of 1 for ANSI compatibility
};

/* ======================================================================
 Definitions
 ====================================================================== */

// #define DSSS_MaxPropagationDelay        0.000002        // 2us   XXXX
#define DSSS_MaxPropagationDelay          0.000010        // 10us   XXXX

class Mac802_11Ext;
class PHY_MIBExt {
public:
	PHY_MIBExt(Mac802_11Ext *parent);
	inline double getSlotTime() {
		return (SlotTime);
	}
	inline double getHeaderDuration() {
		return (HeaderDuration);
	}

	inline double getSymbolDuration() {
		return (SymbolDuration);
	}
	inline int getBasicModulationScheme() {
		return (BasicModulationScheme);
	}

	inline int use_802_11a() {
		return (use_802_11a_flag_);
	}

	inline double getBasicDataRate() {
		return (1.0/SymbolDuration
				* modulation_table[BasicModulationScheme].NDBPS);
	}

	inline u_int32_t getRTSlen() {
		return (sizeof(struct rts_frame));
	}

	inline u_int32_t getCTSlen() {
		return (sizeof(struct cts_frame));
	}

	inline u_int32_t getACKlen() {
		return (sizeof(struct ack_frame));
	}

	inline u_int32_t getHdrLen11() {
		return(offsetof(struct hdr_mac802_11, dh_body[0])
+			ETHER_FCS_LEN);
		}

	private:
		double SlotTime;
		double HeaderDuration;
		double SymbolDuration;
		int BasicModulationScheme;
		int use_802_11a_flag_;
	};

	/*---------------------information for Distributed coordination------*/
#define MAC_FragmentationThreshold	2346		// bytes
#define MAC_MaxTransmitMSDULifetime	512		// time units
#define MAC_MaxReceiveLifetime		512		// time units
class MAC_MIBExt {
public:
	MAC_MIBExt(Mac802_11Ext *parent);
	inline u_int32_t getCWMin() {
		return (CWMin);
	}
	inline u_int32_t getCWMax() {
		return (CWMax);
	}
	inline double getSIFS() {
		return (SIFSTime);
	}
	inline u_int32_t getRTSThreshold() {
		return (RTSThreshold);
	}
	inline u_int32_t getShortRetryLimit() {
		return (ShortRetryLimit);
	}
	inline u_int32_t getLongRetryLimit() {
		return (LongRetryLimit);
	}
private:
	u_int32_t CWMin;
	u_int32_t CWMax;
	double SIFSTime;
	u_int32_t RTSThreshold;
	u_int32_t ShortRetryLimit;
	u_int32_t LongRetryLimit;
};

/* ======================================================================
 The following destination class is used for duplicate detection.
 ====================================================================== */
class Host {
public:
	LIST_ENTRY(Host) link;
	u_int32_t index;
	u_int32_t seqno;
};

/* the new implementation has following modules*/

/* ======================================================================
 1.0 IFSTimer NAVTimer and ChannelStateMgr
 ====================================================================== */
/*
 The channel state manager is responsible for maintaining both the physical and virtual carrier sense statuses for the IEEE 802.11 CSMA mechanism.
 The channel state manager depends on the PHY to update the physical carrier sense status. It expects the PHY to signal channel busy when the total received signal strength rises above carrier sense threshold or when the PHY is in transmission. Similarly, it expects the PHY to signal channel clear when both conditions are gone.
 The channel state manager expects signaling from the reception module for virtual carrier sense status updates as described in the above subsection. Once signaled this way, the channel state manager sets or updates the NAV for the duration specified.
 The channel state manager has five states. The combination of both physical and virtual carrier sense statuses result in four states: NoCSnoNAV, NoCSNAV, CSnoNAV and CSNAV. Additionally, the time spent in InterFrame Spacing (IFS) waiting is also modeled as a state within the channel state manager. This is because the IFS mechanism is essentially a self enforced NAV. As such, the channel state manager treats the Wait IFS state as channel being busy as well.The duration of IFS waiting time depends on the ifs_value parameter, which can be set to DIFS and EIFS. When EDCA is incorporated, it will be necessary to add an additional signaling interface from the transmission coordination module to the channel state manager to advise the AIFS values.  SIFS waiting, however, is handled by the transmission and reception coordination modules directly. This is because SIFS is used in a way unlike that of DIFS and EIFS. Any module that sets a SIFS timer will take action as the timer expires regardless of the channel condition during the SIFS.
 
 The channel state manager reports the joint physical and virtual carrier sense status in response to queries from any other module. That is: it reports CS_IDLE if it is in the NoCSnoNAV state, and CS_BUSY otherwise. It also reports the NAV status to the reception coordination module when queried, to assist the CTS decision.
 The channel state manager actively signals the backoff manger whenever it moves in or out of the NoCSnoNAV state to indicate channel status changes. In turn the backoff manager resumes or pauses its backoff process if it is already in one. 

 */

class ChannelStateMgr;
class IFSTimer : public TimerHandler {
public:
	IFSTimer(ChannelStateMgr *c) :
		TimerHandler() {
		csmgr = c;
	}
protected:
	void expire(Event *e);
private:
	ChannelStateMgr * csmgr;
};

class NAVTimer : public TimerHandler {
public:
	NAVTimer(ChannelStateMgr *c) :
		TimerHandler() {
		csmgr = c;
		expiration = 0;
	}
	
	void update_and_sched(double delay) {
		expiration = delay + Scheduler::instance().clock();
		sched(delay);
	}
	
	double remaining() {
		return (expiration - Scheduler::instance().clock());
	}
	
protected:
	void expire(Event *e);
private:
	ChannelStateMgr *csmgr;
	double expiration;
};

enum ChannelState {noCSnoNAV=0, noCSNAV=1, CSnoNAV=2, CSNAV=3, WIFS=4};

class ChannelStateMgr {
	friend class Mac802_11Ext;
	friend class BackoffMgr;
	friend class IFSTimer;
	friend class NAVTimer;
public:
	ChannelStateMgr(Mac802_11Ext *m);
	void setIFS(double Tifs);
	void handlePHYBusyIndication();
	void handlePHYIdleIndication();
	void handleSetNAV(double Tnav);
	inline ChannelState getChannelState() {
		return channel_state_;
	}
protected:
	void handleIFSTimer();
	void handleNAVTimer();
private:
	Mac802_11Ext * mac_;
	ChannelState channel_state_;
	double ifs_value_;
	IFSTimer *ifsTimer_;
	NAVTimer *navTimer_;

	void setChannelState(ChannelState newState);
};
/* ======================================================================
 2.0 Backoff timer and BackoffMgr
 ====================================================================== */

/*
 The backoff manager maintains the backoff counter to support the collision avoidance mechanism in the IEEE 802.11 MAC. 
 The backoff manager assists the transmission coordination module to run both the regular backoff and post-transmission backoff, but is not aware of the difference between the two. Figure 6 provides a very simplified view on how this is done.
 The backoff manager has three states: No Backoff, Backoff Running, and Backoff Pause. It depends on the signaling of channel carrier sense state from the channel state manger to run or pause the backoff counter. It moves back to the No Backoff state and signals Backoff Done to the transmission coordination module when the backoff counter reaches zero. When EDCA is added, it will be necessary for an additional signaling interface to be implemented between the transmission coordination module and the backoff manager. This is used to trigger a reassessment of the backoff process with a different cw (Contention Window) value. In turn, it could cause the ending of the backoff sooner in cases of higher priority pending frames.
 */
class BackoffMgr;
class BackoffTimer_t : public TimerHandler {
	friend class BackoffMgr;
public:
	BackoffTimer_t(BackoffMgr *b);
	void pause();
	void run();
	int init(int CW);
	void setSlotTime(double);
protected:
	void expire(Event *e);
private:
	double tSlot;
	BackoffMgr * bkmgr_;
	int remainingSlots_;
	double startTime;
};

enum BackoffMgrState {noBackoff,BackoffRunning,BackoffPause};

class BackoffMgr {
	friend class Mac802_11Ext;
	friend class ChannelStateMgr;
	friend class BackoffTimer_t;
public:
	BackoffMgr(Mac802_11Ext *m);
	void handleCSIDLE();
	void handleCSBUSY();
	void handleBKStart(int CW); //the chosen cnt should be passed to BK
	void setSlotTime(double);
	inline BackoffMgrState getBackoffMgrState() {
		return bk_state_;
	}
private:
	Mac802_11Ext * mac_;
	void handleBackoffTimer();
	BackoffMgrState bk_state_;
	BackoffTimer_t * bkTimer_;
	void BKDone();

	void setBackoffMgrState(BackoffMgrState newState);
};


/* ======================================================================
 4.0 TX_Coordination
 ====================================================================== */

/*
 The transmission coordination module manages channel access for packets passed down from the upper layer.
 The state machine in the transmission coordination module roughly divided into two sides depending on whether the RTS/CTS exchange is needed. If the data frame is a broadcast or a unicast with size less than the RTS threshold, it is entirely processed within the right side of the overall state machine. Otherwise, a RTS frame is generated and a sequence of states on the left side is involved before the data frame would be sent.
 
 When the transmission coordination module moves out of the TXC_IDLE state because of a packet coming down from the upper layer, it first checks if a RTS frame should be generated. Afterwards, it starts a backoff process at the backoff manager if there is not one going on already and moves into the RTS Pending or Data Pending state according to the RTS decision.
 If the transmission coordination module is in the RTS Pending or Data Pending state, it instructs the transmission module to transmit the RTS or data frame respectively as soon as receiving a signal indicating Backoff Done from the backoff manager. 
 It is possible for these pending states to be directly bypassed. As shown in Figure 6, if the backoff manager does not have a backoff process currently going on and the channel state manager replies with a CS_IDLE, the transmission coordination module can immediately transmit the RTS or data frame. This is because the standard allows for a radio to start a transmission right away if it has completed a previous post-transmission backoff and the channel, both physically and virtually, has been idle for more than DIFS.
 When the transmission of a RTS frame is completed, the transmission coordination module moves into the Wait CTS state and starts a timer TCTS. If the reception coordination module does not signal the arrival of a CTS frame before the timer expires, it starts another backoff process and moves back to the RTS pending state. It can repeat this process until the short retry limit is reached. If a CTS response comes back in time, then the transmission coordination module waits for SIFS before instructing the transmission module to send the data frame.
 After the data frame transmission, the transmission coordination module moves into the Wait ACK state and starts a TACK timer. If it does not get an ACK reply indication from the reception coordination module before the timer expires, it starts a new backoff process and moves back to RTS Pending or Data Pending state respectively. Again, this is subject to the short or long retry limit respectively.
 In cases of RTS retry or unicast retransmission, the cw parameter is updated before a backoff is requested with the new value.
 The attempted transmission of a frame finishes in three possible ways: 1) it is a broadcast frame and it is transmitted once over the air; 2) it is a unicast frame and the transmission coordination module receives an ACK signaling from the reception coordination module; or 3) one of the retry limits is reached. In all three cases, the transmission coordination module resets the retry counters and the cw parameter, and starts a post-transmission backoff. Afterwards, if there is a packet waiting in the queue, it takes the packet and immediately moves into the RTS Pending or Data Pending state. Otherwise it returns back to the TXC_IDLE state.


 */
class TXC;

class TXC_SIFSTimer : public TimerHandler {
public:
	TXC_SIFSTimer(TXC *t) :
		TimerHandler() {
		txc_ = t;
	}
protected:
	void expire(Event *e);
private:
	TXC * txc_;
};

class TXC_CTSTimer : public TimerHandler {
public:
	TXC_CTSTimer(TXC *t) :
		TimerHandler() {
		txc_ = t;
	}
protected:
	void expire(Event *e);
private:
	TXC * txc_;
};

class TXC_ACKTimer : public TimerHandler {
public:
	TXC_ACKTimer(TXC *t) :
		TimerHandler() {
		txc_ = t;
	}
protected:
	void expire(Event *e);
private:
	TXC * txc_;
};

enum TXCState {TXC_Idle,TXC_wait_RTSsent, TXC_wait_PDUsent, TXC_RTS_pending, TXC_DATA_pending,TXC_wait_CTS,TXC_wait_SIFS,TXC_wait_ACK};

class TXC {
public:
	TXC(Mac802_11Ext *m);
	void setTXCState(TXCState);

	void handleMsgFromUp(Packet *p);
	void handleBKDone();
	void handleTXConfirm();

	void handleTCTStimeout();
	void handleSIFStimeout();
	void handleTACKtimeout();

	void handleACKIndication();
	void handleCTSIndication();

	void checkQueue();
	void prepareMPDU(Packet *p);
	void generateRTSFrame(Packet *p);

private:
	TXCState txc_state_;
	Packet *pRTS;
	Packet *pDATA;
	Mac802_11Ext *mac_;
	TXC_CTSTimer txcCTSTimer;
	TXC_SIFSTimer txcSIFSTimer;
	TXC_ACKTimer txcACKTimer;
	unsigned int shortretrycounter;
	unsigned int longretrycounter;
	//	unsigned int rtsretrycounter;
	//	unsigned int dataretrycounter;
};

/* ======================================================================
 5.0  RX_Coordination
 ====================================================================== */
/*
 The reception coordination module takes from the reception module control and data frames meant for this node. It signals the transmission coordination module when CTS and ACK frames arrive. It is responsible to handle the CTS and ACK responses when RTS and data frames arrive. It also filters the data frames before passing them to the upper layer.
 
 The reception coordination module has only three states: RXC_IDLE, RXC SIFS Wait, and Wait TX Done. The reception coordination module mostly spends time in the RXC_IDLE state and awaits control and data frames from the reception module.
 If a RTS frame arrives, the reception coordination module queries the channel state manager for its NAV status. If the response indicates an active NAV, the RTS frame is simply discarded. Otherwise, the reception coordination module creates a CTS frame and moves into the RXC SIFS Wait state with a SIFS timer set. When the timer expires, it immediately instructs the transmission module to transmit the CTS frame. It then moves into the Wait TX Done state for the transmission duration before returning to the TXC_IDLE state.
 If a unicast data frame arrives, the reception coordination module starts an ACK process similar to the way it handles the CTS response. However, it does not consult the channel state manager for the NAV status. If a CTS or ACK frame arrives, the reception coordination module simply signals the transmission coordination module accordingly.
 The reception coordination module passes data frames to the upper layer. In this process, it is responsible to discard duplicate data frames coming in from the channel, which can be caused by the unicast retransmission mechanism. In these cases, however, it still reacts with an ACK transmission.
 */
class RXC;
class RXC_SIFSTimer : public TimerHandler {
public:
	RXC_SIFSTimer(RXC *r) :
		TimerHandler() {
		rxc_ = r;
	}
protected:
	void expire(Event *e);
private:
	RXC * rxc_;
};

enum RXCState {RXC_Idle,RXC_wait_SIFS,RXC_wait_sent};

class RXC {
public:
	RXC(Mac802_11Ext *m);

	void handleMsgFromBelow(Packet *p);
	void handleSIFStimeout();
	void handleTXConfirm();
	void generateACKFrame(Packet *p);
	void generateCTSFrame(Packet *p);
private:
	RXCState rxc_state_;
	void setRXCState(RXCState);
	Packet *pCTRL;
	Mac802_11Ext *mac_;
	RXC_SIFSTimer rxcSIFSTimer_;
};

/* ======================================================================
 6.0 The actual 802.11 MAC class.
 ====================================================================== */
enum TXConfirmCallback {Callback_TXC,Callback_RXC};

class TXC;
class RXC;
class Mac802_11Ext : public Mac {
	friend class TxTimeout;
	friend class ChannelStateMgr;
	friend class BackoffMgr;
	friend class BackoffTimer_t;
	friend class TXC;
	friend class RXC;

public:
	Mac802_11Ext();
	void recv(Packet *p, Handler *h);
	inline int hdr_dst(char* hdr, int dst = -2);
	inline int hdr_src(char* hdr, int src = -2);
	inline int hdr_type(char* hdr, u_int16_t type = 0);

	void handlePHYBusyIndication();
	void handlePHYIdleIndication();
	void handleRXStartIndication();
	void handleRXEndIndication(Packet *p);
	void handleTXEndIndication();

	int MAC_DBG;

protected:

	void log(char* event, char* additional);
private:

	void handleBKDone();

	void transmit(Packet *p, TXConfirmCallback);
	TXConfirmCallback txConfirmCallback_;


	BackoffMgr bkmgr;
	ChannelStateMgr csmgr;

	TXC txc;
	RXC rxc;

	int command(int argc, const char*const* argv);
	void sendDATA(Packet *p);
	void recvDATA(Packet *p);
	void discard(Packet *p, const char* why);

	/*
	 * Debugging Functions.
	 */
	void trace_pkt(Packet *p);
	void dump(char* fname);

	inline int initialized() {
		return (cache_ && logtarget_ &&Mac::initialized());
	}

	inline void mac_log(Packet *p) {
		logtarget_->recv(p, (Handler*) 0);
	}

	double txtime(Packet *p);
	double txtime(double psz, double drt);
	double txtime(double psz, int mod_scheme);
	double txtime(int bytes);

	inline void inc_cw() {
		cw_ = (cw_ << 1) + 1;
		if (cw_ > macmib_.getCWMax())
			cw_ = macmib_.getCWMax();
	}
	inline void rst_cw() {
		cw_ = macmib_.getCWMin();
	}
	inline double sec(double t) {
		return (t *= 1.0e-6);
	}
	inline u_int16_t usec(double t) {
		u_int16_t us = (u_int16_t)floor((t *= 1e6) + 0.5);
		return us;
	}

protected:
	PHY_MIBExt phymib_;
	MAC_MIBExt macmib_;

private:

	/* ============================================================
	 Internal MAC State
	 ============================================================ */

	u_int32_t cw_; // Contention Window
	u_int32_t ssrc_; // STA Short Retry Count
	u_int32_t slrc_; // STA Long Retry Count
	double sifs_; // Short Interface Space
	double pifs_; // PCF Interframe Space
	double difs_; // DCF Interframe Space
	double eifs_; // Extended Interframe Space

	NsObject* logtarget_;

	/* ============================================================
	 Duplicate Detection state
	 ============================================================ */
	u_int16_t sta_seqno_; // next seqno that I'll use
	int cache_node_count_;
	Host *cache_;
	
};

#endif /* ns_mac_80211Ext_h */

