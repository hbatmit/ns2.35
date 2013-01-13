
/*
 * mac-tdma.h
 * Copyright (C) 1999 by the University of Southern California
 * $Id: mac-tdma.h,v 1.6 2006/02/21 15:20:19 mahrenho Exp $
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
// mac-tdma.h
// by Xuan Chen (xuanc@isi.edu), ISI/USC.
// 
// Preamble TDMA MAC layer for single hop.
// Centralized slot assignment computing.


#ifndef ns_mac_tdma_h
#define ns_mac_tdma_h

// #define DEBUG
//#include <debug.h>

#include "marshall.h"
#include <delay.h>
#include <connector.h>
#include <packet.h>
#include <random.h>
#include <arp.h>
#include <ll.h>
#include <mac.h>

#define GET_ETHER_TYPE(x)		GET2BYTE((x))
#define SET_ETHER_TYPE(x,y)            {u_int16_t t = (y); STORE2BYTE(x,&t);}

/* We are still using these specs for phy layer---same as 802.11. */
/*
 * IEEE 802.11 Spec, section 15.3.2
 *	- default values for the DSSS PHY MIB
 */
#define DSSS_CWMin			31
#define DSSS_CWMax			1023
#define DSSS_SlotTime			0.000020	// 20us
#define	DSSS_CCATime			0.000015	// 15us
#define DSSS_RxTxTurnaroundTime		0.000005	// 5us
#define DSSS_SIFSTime			0.000010	// 10us
#define DSSS_PreambleLength		144		// 144 bits
#define DSSS_PLCPHeaderLength		48		// 48 bits

class PHY_MIB {
public:
	u_int32_t	CWMin;
	u_int32_t	CWMax;
	double		SlotTime;
	double		CCATime;
	double		RxTxTurnaroundTime;
	double		SIFSTime;
	u_int32_t	PreambleLength;
	u_int32_t	PLCPHeaderLength;
};


/* ======================================================================
   Frame Formats
   ====================================================================== */

#define	MAC_ProtocolVersion	0x00
#define MAC_Type_Data		0x02
#define MAC_Subtype_Data	0x00

// Max data length allowed in one slot (byte)
#define MAC_TDMA_MAX_DATA_LEN 1500        

// How many time slots in one frame.
#define MAC_TDMA_SLOT_NUM       32           

// The mode for MacTdma layer's defer timers. */
#define SLOT_SCHE               0
#define SLOT_SEND               1
#define SLOT_RECV               2
#define SLOT_BCAST              3

// Indicate if there is a packet needed to be sent out.
#define NOTHING_TO_SEND         -2
// Indicate if this is the very first time the simulation runs.
#define FIRST_ROUND             -1

// Turn radio on /off
#define ON                       1
#define OFF                      0

/* Quoted from MAC-802.11. */
#define DATA_DURATION           5

/* We are using same header structure as 802.11 currently */
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

struct hdr_mac_tdma {
	struct frame_control	dh_fc;
	u_int16_t		dh_duration;
	u_char			dh_da[ETHER_ADDR_LEN];
	u_char			dh_sa[ETHER_ADDR_LEN];
	u_char			dh_bssid[ETHER_ADDR_LEN];
	u_int16_t		dh_scontrol;
	u_char			dh_body[1]; // XXX Non-ANSI
};

#define ETHER_HDR_LEN				\
	((phymib_->PreambleLength >> 3) +	\
	 (phymib_->PLCPHeaderLength >> 3) +	\
	 offsetof(struct hdr_mac_tdma, dh_body[0] ) +	\
	 ETHER_FCS_LEN)

#define DATA_Time(len)	(8 * (len) / bandwidth_)

/* ======================================================================
   The following destination class is used for duplicate detection.
   ====================================================================== */
// We may need it later for caching...
class Host {
public:
	LIST_ENTRY(Host) link;
	u_int32_t	index;
	u_int32_t	seqno;
};

/* Timers */
class MacTdma;

class MacTdmaTimer : public Handler {
public:
	MacTdmaTimer(MacTdma* m, double s = 0) : mac(m) {
		busy_ = paused_ = 0; stime = rtime = 0.0; slottime_ = s;
	}

	virtual void handle(Event *e) = 0;

	virtual void start(Packet *p, double time);
	virtual void stop(Packet *p);
	virtual void pause(void) { assert(0); }
	virtual void resume(void) { assert(0); }

	inline int busy(void) { return busy_; }
	inline int paused(void) { return paused_; }
	inline double slottime(void) { return slottime_; }
	inline double expire(void) {
		return ((stime + rtime) - Scheduler::instance().clock());
	}


protected:
	MacTdma 	*mac;
	int		busy_;
	int		paused_;
	Event		intr;
	double		stime;	// start time
	double		rtime;	// remaining time
	double		slottime_;
};

/* Timers to schedule transmitting and receiving. */
class SlotTdmaTimer : public MacTdmaTimer {
public:
	SlotTdmaTimer(MacTdma *m) : MacTdmaTimer(m) {}
	void	handle(Event *e);
};

/* Timers to control packet sending and receiving time. */
class RxPktTdmaTimer : public MacTdmaTimer {
public:
	RxPktTdmaTimer(MacTdma *m) : MacTdmaTimer(m) {}

	void	handle(Event *e);
};

class TxPktTdmaTimer : public MacTdmaTimer {
public:
	TxPktTdmaTimer(MacTdma *m) : MacTdmaTimer(m) {}

	void	handle(Event *e);
};

/* TDMA Mac layer. */
class MacTdma : public Mac {
  friend class SlotTdmaTimer;
  friend class TxPktTdmaTimer;
  friend class RxPktTdmaTimer;

 public:
  MacTdma(PHY_MIB* p);
  void		recv(Packet *p, Handler *h);
  inline int	hdr_dst(char* hdr, int dst = -2);
  inline int	hdr_src(char* hdr, int src = -2);
  inline int	hdr_type(char* hdr, u_int16_t type = 0);
  
  /* Timer handler */
  void slotHandler(Event *e);
  void recvHandler(Event *e);
  void sendHandler(Event *e);
  
 protected:
  PHY_MIB		*phymib_;
  
  // Both the slot length and max slot num (max node num) can be configged.
  int			slot_packet_len_;
  int                   max_node_num_;
  
 private:
  int command(int argc, const char*const* argv);

  // Do slot scheduling for the active nodes within one cluster.
  void re_schedule();
  void makePreamble();
  void radioSwitch(int i);

  /* Packet Transmission Functions.*/
  void    sendUp(Packet* p);
  void    sendDown(Packet* p);
  
  /* Actually receive data packet when rxTimer times out. */
  void recvDATA(Packet *p);
  /* Actually send the packet buffered. */
  void send();

  inline int	is_idle(void);
  
  /* Debugging Functions.*/
  void		trace_pkt(Packet *p);
  void		dump(char* fname);
  
  void mac_log(Packet *p) {
    logtarget_->recv(p, (Handler*) 0);
  }
  
  inline double TX_Time(Packet *p) {
    double t = DATA_Time((HDR_CMN(p))->size());

    //    printf("<%d>, packet size: %d, tx-time: %f\n", index_, (HDR_CMN(p))->size(), t);
    if(t < 0.0) {
      drop(p, "XXX");
      exit(1);
    }
    return t;
  }
  
  inline u_int16_t usec(double t) {
    u_int16_t us = (u_int16_t)ceil(t *= 1e6);
    return us;
  };

  /* Timers */
  SlotTdmaTimer mhSlot_;
  TxPktTdmaTimer mhTxPkt_;
  RxPktTdmaTimer mhRxPkt_;

  /* Internal MAC state */
  MacState	rx_state_;	// incoming state (MAC_RECV or MAC_IDLE)
  MacState	tx_state_;	// outgoing state
  
  /* The indicator of the radio. */
  int radio_active_;

  int		tx_active_;	// transmitter is ACTIVE
  
  NsObject*	logtarget_;
  
  /* TDMA scheduling state. 
     Currently, we only use a centralized simplified way to do 
     scheduling. Will work on the algorithm later.*/
  // The max num of slot within one frame.
  static int max_slot_num_;

  // The time duration for each slot.
  static double slot_time_;

  /* The start time for whole TDMA scheduling. */
  static double start_time_;
  
  /* Data structure for tdma scheduling. */
  static int active_node_;            // How many nodes needs to be scheduled

  static int *tdma_schedule_;
  int slot_num_;                      // The slot number it's allocated.

  static int *tdma_preamble_;        // The preamble data structure.

  // When slot_count_ = active_nodes_, a new preamble is needed.
  int slot_count_;
  
  // How many packets has been sent out?
  static int tdma_ps_;
  static int tdma_pr_;
};

double MacTdma::slot_time_ = 0;
double MacTdma::start_time_ = 0;
int MacTdma::active_node_ = 0;
int MacTdma::max_slot_num_ = 0;
int *MacTdma::tdma_schedule_ = NULL;
int *MacTdma::tdma_preamble_ = NULL;

int MacTdma::tdma_ps_ = 0;
int MacTdma::tdma_pr_ = 0;

#endif /* __mac_tdma_h__ */
