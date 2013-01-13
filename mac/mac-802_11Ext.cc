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

#include "delay.h"
#include "connector.h"
#include "packet.h"
#include "random.h"
#include "mobilenode.h"
#include "arp.h"
#include "ll.h"
#include "mac.h"
#include "mac-802_11Ext.h"
#include "cmu-trace.h"
#include <iostream>

#include "common/agent.h"

PHY_MIBExt::PHY_MIBExt(Mac802_11Ext *parent) {
	parent->bind("SlotTime_", &SlotTime);
	parent->bind("HeaderDuration_", &HeaderDuration);
	parent->bind("SymbolDuration_", &SymbolDuration);
	parent->bind("BasicModulationScheme_", &BasicModulationScheme);
	parent->bind_bool("use_802_11a_flag_", &use_802_11a_flag_);
}

MAC_MIBExt::MAC_MIBExt(Mac802_11Ext *parent) {
	parent->bind("CWMin_", &CWMin);
	parent->bind("CWMax_", &CWMax);
	parent->bind("SIFS_", &SIFSTime);
	parent->bind("RTSThreshold_", &RTSThreshold);
	parent->bind("ShortRetryLimit_", &ShortRetryLimit);
	parent->bind("LongRetryLimit_", &LongRetryLimit);

}

/* ======================================================================
 TCL Hooks for the simulator
 ====================================================================== */
static class Mac802_11ExtClass : public TclClass {
public:
	Mac802_11ExtClass() :
		TclClass("Mac/802_11Ext") {
	}
	TclObject* create(int, const char*const*) {
		return (new Mac802_11Ext());
	}
} class_mac802_11;

/* ======================================================================
 Mac Class Functions
 ====================================================================== */
Mac802_11Ext::Mac802_11Ext() :
	Mac(), bkmgr(this), csmgr(this), txc(this), rxc(this), phymib_(this),
			macmib_(this) {
	bind("MAC_DBG", &MAC_DBG);
	ssrc_ = slrc_ = 0;

	sifs_ = macmib_.getSIFS();
	difs_ = macmib_.getSIFS() + 2*phymib_.getSlotTime();
	eifs_ = macmib_.getSIFS() + difs_ + txtime(phymib_.getACKlen(),
			phymib_.getBasicModulationScheme());
	pifs_ = sifs_ + phymib_.getSlotTime();
	cw_ = macmib_.getCWMin();
	bkmgr.setSlotTime(phymib_.getSlotTime());
	csmgr.setIFS(eifs_);
	sta_seqno_ = 1;
	cache_ = 0;
	cache_node_count_ = 0;	
	txConfirmCallback_=Callback_TXC;
}

int Mac802_11Ext::command(int argc, const char*const* argv) {
	if (argc == 3) {
		if (strcmp(argv[1], "log-target") == 0) {
			logtarget_ = (NsObject*) TclObject::lookup(argv[2]);
			if (logtarget_ == 0)
				return TCL_ERROR;
			return TCL_OK;
		} else if (strcmp(argv[1], "nodes") == 0) {
			if (cache_)
				return TCL_ERROR;
			cache_node_count_ = atoi(argv[2]);
			cache_ = new Host[cache_node_count_ + 1];
			assert(cache_);
			bzero(cache_, sizeof(Host) * (cache_node_count_+1));
			return TCL_OK;
		}

	}
	return Mac::command(argc, argv);
}

/* ======================================================================
 Debugging Routines
 ====================================================================== */
void Mac802_11Ext::trace_pkt(Packet *p) {
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_mac802_11* dh = HDR_MAC802_11(p);
	u_int16_t *t = (u_int16_t*) &dh->dh_fc;

	fprintf(stderr, "\t[ %2x %2x %2x %2x ] %x %s %d\n",
	*t, dh->dh_duration,
	ETHER_ADDR(dh->dh_ra), ETHER_ADDR(dh->dh_ta),
	index_, packet_info.name(ch->ptype()), ch->size());
}

void Mac802_11Ext::dump(char *) {

}

void Mac802_11Ext::log(char* event, char* additional) {
	if (MAC_DBG)
		cout<<"L "<<Scheduler::instance().clock()<<" "<<index_<<" "<<"MAC"<<" "<<event<<" "<<additional
				<<endl;
}

/* ======================================================================
 Packet Headers Routines
 ====================================================================== */
inline int Mac802_11Ext::hdr_dst(char* hdr, int dst) {
	struct hdr_mac802_11 *dh = (struct hdr_mac802_11*) hdr;

	if (dst > -2)
		STORE4BYTE(&dst, (dh->dh_ra));

	return ETHER_ADDR(dh->dh_ra);
}

inline int Mac802_11Ext::hdr_src(char* hdr, int src) {
	struct hdr_mac802_11 *dh = (struct hdr_mac802_11*) hdr;
	if (src > -2)
		STORE4BYTE(&src, (dh->dh_ta));
	return ETHER_ADDR(dh->dh_ta);
}

inline int Mac802_11Ext::hdr_type(char* hdr, u_int16_t type) {
	struct hdr_mac802_11 *dh = (struct hdr_mac802_11*) hdr;
	if (type)
		STORE2BYTE(&type,(dh->dh_body));
	return GET2BYTE(dh->dh_body);
}

/* ======================================================================
 Misc Routines
 ====================================================================== */

void Mac802_11Ext::discard(Packet *p, const char* why) {
	hdr_mac802_11* mh = HDR_MAC802_11(p);
	hdr_cmn *ch = HDR_CMN(p);

	/* if the rcvd pkt contains errors, a real MAC layer couldn't
	 necessarily read any data from it, so we just toss it now */
	if (ch->error() != 0) {
		Packet::free(p);
		//p = 0;
		return;
	}

	switch (mh->dh_fc.fc_type) {
	case MAC_Type_Management:
		drop(p, why);
		return;
	case MAC_Type_Control:
		switch (mh->dh_fc.fc_subtype) {
		case MAC_Subtype_RTS:
			if ((u_int32_t)ETHER_ADDR(mh->dh_ta) ==(u_int32_t)index_) {
				drop(p, why);
				return;
			}
			/* fall through - if necessary */
		case MAC_Subtype_CTS:
		case MAC_Subtype_ACK:
			if ((u_int32_t)ETHER_ADDR(mh->dh_ra) ==(u_int32_t)index_) {
				drop(p, why);
				return;
			}
			break;
		default:
			fprintf(stderr, "invalid MAC Control subtype\n");
			exit(1);
		}
		break;
	case MAC_Type_Data:
		switch (mh->dh_fc.fc_subtype) {
		case MAC_Subtype_Data:
			if ((u_int32_t)ETHER_ADDR(mh->dh_ra) ==(u_int32_t)index_ ||(u_int32_t)ETHER_ADDR(mh->dh_ta) ==(u_int32_t)index_ ||(u_int32_t)ETHER_ADDR(mh->dh_ra) == MAC_BROADCAST) {
				drop(p, why); //check! drop does not work
				// Packet::free(p);
				return;
			}
			break;
		default:
			fprintf(stderr, "invalid MAC Data subtype\n");
			exit(1);
		}
		break;
	default:
		fprintf(stderr, "invalid MAC type (%x)\n", mh->dh_fc.fc_type);
		trace_pkt(p);
		exit(1);
	}
	Packet::free(p);

}

/*new code starts here*/

/*--------------1.0 Channel State module-------------------------------*/
/*----------------IFSTimer NAVTimer and ChannelStateMgr----------------*/
/*---------------------------------------------------------------------*/

//IFSTimer
void IFSTimer::expire(Event *) {
	csmgr->handleIFSTimer();
}

//NAVTimer
void NAVTimer::expire(Event *) {
	csmgr->handleNAVTimer();
}

//ChannelStateMgr
ChannelStateMgr::ChannelStateMgr(Mac802_11Ext * m) {
	mac_ = m;
	channel_state_ = noCSnoNAV;
	ifs_value_ = -1.0;
	ifs_value_ = mac_->eifs_;
	ifsTimer_ = new IFSTimer(this);
	navTimer_ = new NAVTimer(this);
}

void ChannelStateMgr::setChannelState(ChannelState newState) {
	if (mac_->MAC_DBG) {
		char msg[1000];
		sprintf(msg, "%d -> %d", channel_state_, newState);
		mac_->log("ChState", msg);
	}
	channel_state_ = newState;
}

void ChannelStateMgr::setIFS(double tifs) {
	if (mac_->MAC_DBG) {
		char msg[1000];
		sprintf(msg, "%f", tifs);
		mac_->log("ChSetIFS", msg);
	}
	ifs_value_=tifs;
	//the following line of codes is used to solved the race conditon of event scheduling
	//IDleIndication is handled before RXEndIndication
	//therefore the IFSTimer is started before the new ifs_value_ is set in RXEndIndication
	if (ifsTimer_->status()==TIMER_PENDING) // ifsTimer has been scheduled
	{
		ifsTimer_->cancel();
		ifsTimer_->sched(ifs_value_);
	}
	//end of this change
}

void ChannelStateMgr::handlePHYBusyIndication() {
	switch (channel_state_) {
	case noCSnoNAV:
		mac_->bkmgr.handleCSBUSY();
		setChannelState(CSnoNAV);
		break;
	case noCSNAV:
		setChannelState(CSNAV);
		break;
	case WIFS:
		if (ifsTimer_->status()==TIMER_PENDING)
			ifsTimer_->cancel();
		setChannelState(CSnoNAV);
		break;
	default:
		//  the busy indication from phy will be ignored if the channel statemgr is already in CS_BUSY
		break;
	}
}

void ChannelStateMgr::handlePHYIdleIndication() {
	switch (channel_state_) {
	case CSnoNAV:
		ifsTimer_->sched(ifs_value_);
		setChannelState(WIFS);
		break;
	case CSNAV:
		setChannelState(noCSNAV);
		break;
	default:
		// the idle indication from phy will be ignored if the channel statemgr is already CS free
		break;
	}
}

void ChannelStateMgr::handleSetNAV(double t) {
	if (mac_->MAC_DBG) {
		char msg [1000];
		sprintf(msg, "%f", t);
		mac_->log("ChSetNAV", msg);
	}
	switch (channel_state_) {
	case noCSnoNAV:
		navTimer_->update_and_sched(t);
		mac_->bkmgr.handleCSBUSY();
		setChannelState(noCSNAV);
		break;
	case CSnoNAV:
		navTimer_->update_and_sched(t);
		setChannelState(CSNAV);
		break;
	case noCSNAV:
	case CSNAV:
		if (t > navTimer_->remaining()) {
			navTimer_->cancel();
			navTimer_->update_and_sched(t);
		}
		break;
	case WIFS:
		navTimer_->update_and_sched(t);
		if (ifsTimer_->status()==TIMER_PENDING)
			ifsTimer_->cancel();
		setChannelState(noCSNAV);
		//needs further attention
		//cout<<"logic error at ChannelStateMgr, event handleSetNAV"<<endl;
		//exit(-1);
		break;
	default:
		cout<<"logic error at ChannelStateMgr, event handleSetNAV"<<endl;
		exit(-1);
	}
}

void ChannelStateMgr::handleIFSTimer() {
	if (channel_state_ == WIFS) {
		//chi changed the order 
		setChannelState(noCSnoNAV);
		mac_->bkmgr.handleCSIDLE();

	} else {
		cout<<"logic error at ChannelStateMgr, event handleIFSTimer"<<endl;
		exit(-1);
	}
}

void ChannelStateMgr::handleNAVTimer() {
	switch (channel_state_) {
	case noCSNAV:
		ifsTimer_->sched(ifs_value_);
		setChannelState(WIFS);
		break;
	case CSNAV:
		setChannelState(CSnoNAV);
		break;
	default:
		cout<<"logic error at ChannelStateMgr, event handleNAVTimer"<<endl;
		exit(-1);
	}
}
/*---------------2.0 Backoff Module----------------*/
/*--- BackoffTimer_t and BackoffMgr -----------*/
/*---------------------------------------------*/
/* BackoffTimer_t */

BackoffTimer_t::BackoffTimer_t(BackoffMgr *b) {
	bkmgr_ =b;
	tSlot = -1.0; // the real initialization is in int()
	startTime=-1.0;
	remainingSlots_=-1;
}

void BackoffTimer_t::setSlotTime(double t) {
	tSlot = t;
}

int BackoffTimer_t::init(int CW) {
	remainingSlots_ = Random::integer(CW+1); //choose a number within [0,CW]
	startTime = -1.0;
	return remainingSlots_;
}

void BackoffTimer_t::pause() {
	if (startTime ==-1.0) {
		cout<<"logic error at BackoffTimer_t, event pause"<<endl;
		exit(-1);
	}
	remainingSlots_-=(int)(floor((Scheduler::instance().clock()-startTime)/tSlot));
	if (status()==TIMER_PENDING)
		cancel();
	startTime = -1.0;
}

void BackoffTimer_t::run() {
	//chi added for testing remainging slots
	if (remainingSlots_ ==0) {
		remainingSlots_=-1;
		bkmgr_->handleBackoffTimer();
	} else {
		startTime = Scheduler::instance().clock();
		sched(remainingSlots_*tSlot);
	}
}

void BackoffTimer_t::expire(Event *) {
	remainingSlots_=-1;
	bkmgr_->handleBackoffTimer();
	return;
}

BackoffMgr::BackoffMgr(Mac802_11Ext *m) {
	mac_ = m;
	bk_state_ = noBackoff; //needs further attention
	bkTimer_ = new BackoffTimer_t(this);
}

void BackoffMgr::setSlotTime(double t) {
	bkTimer_->setSlotTime(t);
}

void BackoffMgr::setBackoffMgrState(BackoffMgrState newState) {
	if (mac_->MAC_DBG) {
		char msg [1000];
		sprintf(msg, "%d -> %d, rslots: %d", bk_state_, newState,
				bkTimer_->remainingSlots_);
		mac_->log("BkState", msg);
	}
	bk_state_ = newState;
}

void BackoffMgr::handleCSIDLE() {
	switch (bk_state_) {
	case BackoffPause:
		setBackoffMgrState(BackoffRunning);
		bkTimer_->run();
		break;
	case noBackoff:
		//ignore this signal
		break;
	default:
		cout<<"logic error at BackoffMgr, event handleCSIDLE"<<endl;
		exit(-1);
	}
}

void BackoffMgr::handleCSBUSY() {
	switch (bk_state_) {
	case noBackoff:
		//ignore this signal
		break;
	case BackoffPause:
		//ignore this signal
		break;
	case BackoffRunning:

		setBackoffMgrState(BackoffPause);
		bkTimer_->pause();
		break;
	default:
		cout<<"logic error at BackoffMgr, event handleCSBUSY"<<endl;
		exit(-1);
	}
}

void BackoffMgr::handleBKStart(int CW) {
	switch (bk_state_) {
	case noBackoff:
		//Chi changed

		if (mac_->csmgr.getChannelState()==noCSnoNAV) {
			if (bkTimer_->init(CW) == 0) {
				setBackoffMgrState(noBackoff);
				BKDone();

			} else {
				setBackoffMgrState(BackoffRunning);
				bkTimer_->run();

			}
		} else {

			bkTimer_->init(CW);
			setBackoffMgrState(BackoffPause);
		}
		break;
	case BackoffRunning:
		//ignore this signal
		break;
	case BackoffPause:
		//ignore this signal
		break;
	default:
		cout<<"logic error at BackoffMgr, event handleBKStart"<<endl;
		exit(-1);
	}
}

void BackoffMgr::handleBackoffTimer() {
	switch (bk_state_) {
	case BackoffRunning:

		setBackoffMgrState(noBackoff);
		BKDone();
		break;
	default:
		cout<<"logic error at BackoffMgr, event handleBackoffTimer"<<endl;
		exit(-1);
	}
}

void BackoffMgr::BKDone() {
	mac_->handleBKDone();
}
/*----------------------------------------------------*/
/*-------------3.0 reception module-------------------*/
/*---------------PHYBusyIndication PHYIdleIndication--*/
/*---------------RXStartIndication RXEndIndication----*/

void Mac802_11Ext::handlePHYBusyIndication() {
	if (MAC_DBG)
		log("PHYBusyInd", "");
	csmgr.handlePHYBusyIndication();
}

void Mac802_11Ext::handlePHYIdleIndication() {
	if (MAC_DBG)
		log("PHYIdleInd", "");
	csmgr.handlePHYIdleIndication();
}

void Mac802_11Ext::handleRXStartIndication() {
	if (MAC_DBG)
		log("RxStartInd", "");
}

void Mac802_11Ext::handleRXEndIndication(Packet * pktRx_) {
	if (!pktRx_) {
		if (MAC_DBG)
			log("RxEndInd", "FAIL");
		csmgr.setIFS(eifs_);
		return;
	}
	if (MAC_DBG)
		log("RxEndInd", "SUCC");

	hdr_mac802_11 *mh = HDR_MAC802_11(pktRx_);
	u_int32_t dst= ETHER_ADDR(mh->dh_ra);
	u_int8_t type = mh->dh_fc.fc_type;
	u_int8_t subtype = mh->dh_fc.fc_subtype;
	csmgr.setIFS(difs_);

	if (tap_ && type == MAC_Type_Data&& MAC_Subtype_Data == subtype)
		tap_->tap(pktRx_);
	/*
	 * Address Filtering
	 */
	if (dst == (u_int32_t)index_ || dst == MAC_BROADCAST) {
		rxc.handleMsgFromBelow(pktRx_);
	} else {
		/*
		 *  We don't want to log this event, so we just free
		 *  the packet instead of calling the drop routine.
		 */
		if (MAC_DBG)
			log("RxEndInd", "NAV");
		csmgr.handleSetNAV(sec(mh->dh_duration));
		discard(pktRx_, "---");
		return;
	}
}

//-------------------4.0 transmission module----------------//
//---------------------TxTimer and MAC handlers-------------//

// this function is used by transmission module


void Mac802_11Ext::transmit(Packet *p, TXConfirmCallback callback) {
	txConfirmCallback_=callback;
	// check memory leak? 
	if (MAC_DBG)
		log("Tx", "Start");
	downtarget_->recv(p->copy(), this);
}

void Mac802_11Ext::handleTXEndIndication() {
	if (MAC_DBG)
		log("Tx", "Fin");
	switch (txConfirmCallback_) {
	case Callback_TXC:
		txc.handleTXConfirm();
		break;
	case Callback_RXC:
		rxc.handleTXConfirm();
		break;
	default:
		cout<<"logic error at handleTXTimer"<<endl;
	}
}

/*-------------------5.0 protocol coodination module----------
 ---------------------5.1 tx coordination----------------------
 ---------------------TxTimeout and MAC handlers-------------*/

// this function is used by tx coordination to prepare the MAC frame in detail


void Mac802_11Ext::handleBKDone() {
	txc.handleBKDone();
}

// this function is used to handle the packets from the upper layer
void Mac802_11Ext::recv(Packet *p, Handler *h) {
	struct hdr_cmn *hdr = HDR_CMN(p);
	/*
	 * Sanity Check
	 */
	assert(initialized());

	if (hdr->direction() == hdr_cmn::DOWN) {
		if (MAC_DBG)
			log("FROM_IFQ", "");
		//			send(p, h); replaced by txc
		callback_=h;
		txc.handleMsgFromUp(p);
		return;
	} else {
		//the reception of a packet is moved to handleRXStart/EndIndication
		cout<<"logic error at recv, event sendUP"<<endl;
	}
}

//-------------------5.2 rx coordination----------------------------------//
//------------------------------------------------------------------------//
void Mac802_11Ext::recvDATA(Packet *p) {
	struct hdr_mac802_11 *dh = HDR_MAC802_11(p);
	u_int32_t dst, src;

	struct hdr_cmn *ch = HDR_CMN(p);
	dst = ETHER_ADDR(dh->dh_ra);
	src = ETHER_ADDR(dh->dh_ta);
	ch->size() -= phymib_.getHdrLen11();
	ch->num_forwards() += 1;

	if (dst != MAC_BROADCAST) {
		if (src < (u_int32_t) cache_node_count_) {
			Host *h = &cache_[src];

			if (h->seqno && h->seqno == dh->dh_scontrol) {
				if (MAC_DBG)
					log("DUPLICATE", "");
				discard(p, DROP_MAC_DUPLICATE);
				return;
			}
			h->seqno = dh->dh_scontrol;
		} else {
			if (MAC_DBG) {
				static int count = 0;
				if (++count <= 10) {
					log("DUPLICATE", "Accessing MAC CacheArray out of range");
					if (count == 10)
						log("DUPLICATE", "Suppressing additional MAC Cache warnings");
				}
			}
		}
	}

	if (MAC_DBG)
		log("SEND_UP", "");
	uptarget_->recv(p, (Handler*) 0);
}

//-------------------6.0 txtime----------------------------------//
//------------------------------------------------------------------------//

/*
 * txtime()	- pluck the precomputed tx time from the packet header
 */
double Mac802_11Ext::txtime(Packet *p) {
	struct hdr_cmn *ch = HDR_CMN(p);
	double t = ch->txtime();
	if (t < 0.0) {
		drop(p, "XXX");
		exit(1);
	}
	return t;
}

/*
 * txtime()	- calculate tx time for packet of size "psz" bytes 
 *		  at rate "drt" bps
 */
double Mac802_11Ext::txtime(double psz, double drt) {
	int datalen = (int) psz << 3;
	double t = phymib_.getHeaderDuration() + (double)datalen/drt;
	return (t);
}

/*
 * txtime()	- calculate tx time for packet of size "psz" bytes 
 *		  with modulation scheme mod_scheme
 *                this is the function used in reality
 */
double Mac802_11Ext::txtime(double psz, int mod_scheme) {
	int datalen = (int) psz << 3;
	int DBPS = modulation_table[mod_scheme].NDBPS;

	// 802.11p
	if (phymib_.use_802_11a()) {
		datalen = datalen + 16 + 6; // 16 SYMBOL BITS, 6 TAIL BITS
	}
	int symbols = (int)(ceil((double)datalen/DBPS)); // PADDING BITS, FILL SYMBOl

	double t = phymib_.getHeaderDuration() + (double) symbols
			* phymib_.getSymbolDuration();
	return (t);
}

/*new code ends here*/

//-------tx coordination function-------//
void TXC_CTSTimer::expire(Event *) {
	txc_->handleTCTStimeout();
}

void TXC_SIFSTimer::expire(Event *) {
	txc_->handleSIFStimeout();
}

void TXC_ACKTimer::expire(Event *) {
	txc_->handleTACKtimeout();
}

TXC::TXC(Mac802_11Ext *m) :
	txcCTSTimer(this), txcSIFSTimer(this), txcACKTimer(this) {
	mac_=m;
	pRTS=0;
	pDATA=0;
	txc_state_=TXC_Idle;
	shortretrycounter = 0;
	longretrycounter = 0;
	//	rtsretrycounter=0;
	//	dataretrycounter=0;
}

void TXC::handleMsgFromUp(Packet *p) {
	if (mac_->MAC_DBG)
		mac_->log("TXC", "msgFromUp");
	pDATA=p;
	prepareMPDU(pDATA);
	hdr_mac802_11* mh = HDR_MAC802_11(pDATA);
	struct hdr_cmn *ch = HDR_CMN(pDATA);

	if ((u_int32_t)ETHER_ADDR(mh->dh_ra) != MAC_BROADCAST&& (unsigned int)(ch->size())
			>mac_->macmib_.getRTSThreshold()) //should get the right size //check type
	{
		if (mac_->MAC_DBG)
			mac_->log("TXC", "msgFromUp, RTS/CTS");
		generateRTSFrame(pDATA);

	}
	if (mac_->bkmgr.getBackoffMgrState()==noBackoff) {
		if (mac_->csmgr.getChannelState()==noCSnoNAV) {
			if (mac_->MAC_DBG)
				mac_->log("TXC", "msgFromUp, noBackoff, noCSnoNAV");
			if (pRTS!=0) {
				mac_->transmit(pRTS, Callback_TXC);
				setTXCState(TXC_wait_RTSsent);
			} else {
				mac_->transmit(pDATA, Callback_TXC);
				setTXCState(TXC_wait_PDUsent);
			}
		} else {
			if (mac_->MAC_DBG)
				mac_->log("TXC", "msgFromUp, Backoff started");
			if (pRTS!=0) {
				setTXCState(TXC_RTS_pending);
			} else {
				setTXCState(TXC_DATA_pending);
			}
			mac_->bkmgr.handleBKStart(mac_->cw_);
		}
	} else {
		if (mac_->MAC_DBG)
			mac_->log("TXC", "msgFromUp, Backoff pending");
		if (pRTS!=0) {
			setTXCState(TXC_RTS_pending);
		} else {
			setTXCState(TXC_DATA_pending);
		}
	}
}

void TXC::prepareMPDU(Packet *p) {
	hdr_cmn* ch = HDR_CMN(p);
	struct hdr_mac802_11* dh = HDR_MAC802_11(p);
	ch->size() += mac_->phymib_.getHdrLen11();

	dh->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
	dh->dh_fc.fc_type = MAC_Type_Data;
	dh->dh_fc.fc_subtype = MAC_Subtype_Data;
	dh->dh_fc.fc_to_ds = 0;
	dh->dh_fc.fc_from_ds = 0;
	dh->dh_fc.fc_more_frag = 0;
	dh->dh_fc.fc_retry = 0;
	dh->dh_fc.fc_pwr_mgt = 0;
	dh->dh_fc.fc_more_data = 0;
	dh->dh_fc.fc_wep = 0;
	dh->dh_fc.fc_order = 0;
	dh->dh_scontrol = mac_->sta_seqno_++;
	
	ch->txtime() = mac_->txtime(ch->size(), ch->mod_scheme_);

	if ((u_int32_t)ETHER_ADDR(dh->dh_ra) != MAC_BROADCAST) {
		dh->dh_duration = mac_->usec(mac_->txtime(mac_->phymib_.getACKlen(),
				mac_->phymib_.getBasicModulationScheme())
				+ mac_->macmib_.getSIFS());
	} else {
		dh->dh_duration = 0;
	}
}

void TXC::handleBKDone() {
	if (mac_->MAC_DBG)
		mac_->log("TXC", "BK Done");
	switch (txc_state_) {
	case TXC_RTS_pending:
		mac_->transmit(pRTS, Callback_TXC);
		setTXCState(TXC_wait_RTSsent);
		break;
	case TXC_DATA_pending:
		mac_->transmit(pDATA, Callback_TXC);
		setTXCState(TXC_wait_PDUsent);
		break;
	case TXC_Idle: //check : not in the SDL
	case TXC_wait_CTS:
	case TXC_wait_ACK:
		break;
	default:
		cout<<txc_state_<<endl;
		cout<<"logic error at TXC handling BKDone"<<endl;
	}
}

void TXC::handleTXConfirm() {
	if (mac_->MAC_DBG)
		mac_->log("TXC", "TX Confirm");
	hdr_mac802_11* mh = HDR_MAC802_11(pDATA);
	switch (txc_state_) {
	case TXC_wait_PDUsent:
		if ((u_int32_t)ETHER_ADDR(mh->dh_ra) != MAC_BROADCAST) {
			txcACKTimer.sched(mac_->macmib_.getSIFS()+ mac_->txtime(
					mac_->phymib_.getACKlen(),
					mac_->phymib_.getBasicModulationScheme())
					+ DSSS_MaxPropagationDelay);
			setTXCState(TXC_wait_ACK);
		} else {
			Packet::free(pDATA);
			pDATA = 0;
			mac_->bkmgr.handleBKStart(mac_->cw_);
			checkQueue();
		}
		break;
	case TXC_wait_RTSsent:
		txcCTSTimer.sched(mac_->macmib_.getSIFS()+ mac_->txtime(
				mac_->phymib_.getCTSlen(),
				mac_->phymib_.getBasicModulationScheme())
				+ DSSS_MaxPropagationDelay);
		setTXCState(TXC_wait_CTS);
		break;
	default:
		cout<<"logic error at TXC handling txconfirm"<<endl;

	}
}

void TXC::checkQueue() {
	setTXCState(TXC_Idle);
	if (mac_->callback_) {
		if (mac_->MAC_DBG)
			mac_->log("TXC", "checkQueue, empty");
		Handler *h = mac_->callback_;
		mac_->callback_=0;
		h->handle((Event*) 0);
	} else {
		if (mac_->MAC_DBG)
			mac_->log("TXC", "checkQueue, pending");
	}
}

void TXC::handleCTSIndication() {
	if (mac_->MAC_DBG)
		mac_->log("TXC", "CTS indication");
	if (txc_state_==TXC_wait_CTS) {
		Packet::free(pRTS);
		pRTS = 0;
		txcCTSTimer.cancel();
		shortretrycounter = 0;
		setTXCState(TXC_wait_SIFS);
		txcSIFSTimer.sched(mac_->macmib_.getSIFS());
	} else {
		cout<<"txc_state_"<<txc_state_<<endl;
		cout<<"logic error at TXC handling CTS indication"<<endl;
	}
}

void TXC::handleTCTStimeout() {
	if (txc_state_==TXC_wait_CTS) {
		shortretrycounter++;
		mac_->inc_cw();
		if (shortretrycounter >= mac_->macmib_.getShortRetryLimit()) {
			if (mac_->MAC_DBG)
				mac_->log("TXC", "CTS timeout, limit reached");
			mac_->discard(pRTS, DROP_MAC_RETRY_COUNT_EXCEEDED);
			pRTS = 0;
			
			// higher layers feedback support
			struct hdr_cmn *ch = HDR_CMN(pDATA);
			if (ch->xmit_failure_) {
				ch->xmit_reason_ = XMIT_REASON_RTS;
			    ch->xmit_failure_(pDATA->copy(), ch->xmit_failure_data_);
			}
			mac_->discard(pDATA, DROP_MAC_RETRY_COUNT_EXCEEDED);
			pDATA = 0;

			shortretrycounter = 0;
			longretrycounter = 0;
			mac_->cw_=mac_->macmib_.getCWMin();
			mac_->bkmgr.handleBKStart(mac_->cw_);
			checkQueue();
		} else {
			if (mac_->MAC_DBG)
				mac_->log("TXC", "CTS timeout, retry");
			setTXCState(TXC_RTS_pending);
			mac_->bkmgr.handleBKStart(mac_->cw_);
		}

	} else
		cout<<"logic error at TXC handleTCTS timeout"<<endl;
}

void TXC::handleSIFStimeout() {
	if (mac_->MAC_DBG)
		mac_->log("TXC", "SIFS finished");
	if (txc_state_==TXC_wait_SIFS) {
		mac_->transmit(pDATA, Callback_TXC);
		setTXCState(TXC_wait_PDUsent);
	} else
		cout<<"logic error at TXC handling SIFS timeout"<<endl;

}

void TXC::handleTACKtimeout() {
	if (txc_state_==TXC_wait_ACK) {
		hdr_mac802_11* mh = HDR_MAC802_11(pDATA);
		struct hdr_cmn *ch = HDR_CMN(pDATA);

		unsigned int limit = 0;
		unsigned int * counter;
		if ((unsigned int)ch->size() <= mac_->macmib_.getRTSThreshold()) {
			limit = mac_->macmib_.getShortRetryLimit();
			counter = &shortretrycounter;
		} else {
			limit = mac_->macmib_.getLongRetryLimit();
			counter = &longretrycounter;
		}
		(*counter)++;

		mac_->inc_cw();

		if (*counter >= limit) {
			if (mac_->MAC_DBG)
				mac_->log("TXC", "ACK timeout, limit reached");

			// higher layers feedback support
			struct hdr_cmn *ch = HDR_CMN(pDATA);
			if (ch->xmit_failure_) {
				ch->xmit_reason_ = XMIT_REASON_RTS;
			    ch->xmit_failure_(pDATA->copy(), ch->xmit_failure_data_);
			}

			mac_->discard(pDATA, DROP_MAC_RETRY_COUNT_EXCEEDED);
			pDATA = 0;

			shortretrycounter = 0;
			longretrycounter = 0;
			mac_->cw_=mac_->macmib_.getCWMin();
			mac_->bkmgr.handleBKStart(mac_->cw_);
			checkQueue();
		} else {
			if (mac_->MAC_DBG)
				mac_->log("TXC", "ACK timeout, retry");

			if ((u_int32_t)ETHER_ADDR(mh->dh_ra) != MAC_BROADCAST&&(unsigned int)(ch->size())
					> mac_->macmib_.getRTSThreshold()) {
				generateRTSFrame(pDATA);
				setTXCState(TXC_RTS_pending);
			} else {
				setTXCState(TXC_DATA_pending);
			}

			mac_->bkmgr.handleBKStart(mac_->cw_);

		}
	} else
		cout<<"logic error at TXC handling TACK timeout"<<endl;
}

void TXC::generateRTSFrame(Packet * p) {
	Packet *rts = Packet::alloc();
	hdr_cmn* ch = HDR_CMN(rts);
	struct rts_frame *rf = (struct rts_frame*)rts->access(hdr_mac::offset_);

	ch->uid() = 0;
	ch->ptype() = PT_MAC;
	ch->size() = mac_->phymib_.getRTSlen();
	ch->iface() = -2;
	ch->error() = 0;
	ch->mod_scheme_= (ModulationScheme)mac_->phymib_.getBasicModulationScheme();
	bzero(rf, MAC_HDR_LEN);

	rf->rf_fc.fc_protocol_version = MAC_ProtocolVersion;
	rf->rf_fc.fc_type = MAC_Type_Control;
	rf->rf_fc.fc_subtype = MAC_Subtype_RTS;
	rf->rf_fc.fc_to_ds = 0;
	rf->rf_fc.fc_from_ds = 0;
	rf->rf_fc.fc_more_frag = 0;
	rf->rf_fc.fc_retry = 0;
	rf->rf_fc.fc_pwr_mgt = 0;
	rf->rf_fc.fc_more_data = 0;
	rf->rf_fc.fc_wep = 0;
	rf->rf_fc.fc_order = 0;

	struct hdr_mac802_11 *mh = HDR_MAC802_11(p);
	u_int32_t dst= ETHER_ADDR(mh->dh_ra);
	STORE4BYTE(&dst, (rf->rf_ra));
	STORE4BYTE(&(mac_->index_), (rf->rf_ta));

	ch->txtime() = mac_->txtime(ch->size(), ch->mod_scheme_);
	rf->rf_duration = mac_->usec(mac_->macmib_.getSIFS()
			+ mac_->txtime(mac_->phymib_.getCTSlen(),
					mac_->phymib_.getBasicModulationScheme())
			+ mac_->macmib_.getSIFS()+ mac_->txtime(p)+ mac_->macmib_.getSIFS()
			+ mac_->txtime(mac_->phymib_.getACKlen(),
					mac_->phymib_.getBasicModulationScheme()));
	pRTS = rts;
}

void TXC::handleACKIndication() {
	if (mac_->MAC_DBG)
		mac_->log("TXC", "ACK indication");
	if (txc_state_==TXC_wait_ACK ) {
		txcACKTimer.cancel();
		shortretrycounter = 0;
		longretrycounter = 0;
		Packet::free(pDATA);
		pDATA=0;
		mac_->cw_=mac_->macmib_.getCWMin();
		mac_->bkmgr.handleBKStart(mac_->cw_);
		checkQueue();
	} else {
		cout<<"logic error at TXC handling ACKIndication"<<endl;
		cout<<txc_state_<<endl;
	}

}

void TXC::setTXCState(TXCState newstate) {
	if (mac_->MAC_DBG) {
		char msg [1000];
		sprintf(msg, "%d -> %d", txc_state_, newstate);
		if (mac_->MAC_DBG)
			mac_->log("TXCState", msg);
	}
	txc_state_=newstate;

}

//-------tx coordination-------//

//-------rx coordination-------//

void RXC_SIFSTimer::expire(Event *) {
	rxc_->handleSIFStimeout();
}

RXC::RXC(Mac802_11Ext *m) :
	rxcSIFSTimer_(this) {
	mac_=m;
	pCTRL=0;
	rxc_state_=RXC_Idle;
}

void RXC::handleMsgFromBelow(Packet *p) {
	hdr_mac802_11* mh = HDR_MAC802_11(p);
	switch (mh->dh_fc.fc_type) {

	case MAC_Type_Control:
		switch (mh->dh_fc.fc_subtype) {
		case MAC_Subtype_ACK:
			if (mac_->MAC_DBG)
				mac_->log("RXC", "msgFromBelow: ACK");
			mac_->txc.handleACKIndication();
			mac_->mac_log(p);
			setRXCState(RXC_Idle);
			break;

		case MAC_Subtype_RTS:
			if (mac_->MAC_DBG)
				mac_->log("RXC", "msgFromBelow: RTS");
			if (mac_->csmgr.getChannelState()==CSNAV
					|| mac_->csmgr.getChannelState()==noCSNAV) {
				setRXCState(RXC_Idle);
				mac_->mac_log(p);
			} else {
				generateCTSFrame(p);
				mac_->mac_log(p);
				rxcSIFSTimer_.sched(mac_->macmib_.getSIFS());
				setRXCState(RXC_wait_SIFS);
			}
			break;
		case MAC_Subtype_CTS:
			if (mac_->MAC_DBG)
				mac_->log("RXC", "msgFromBelow: CTS");
			mac_->txc.handleCTSIndication();
			mac_->mac_log(p);
			setRXCState(RXC_Idle);
			break;
		default:
			//
			;
		}
		break;

	case MAC_Type_Data:
		if (mac_->MAC_DBG)
			mac_->log("RXC", "msgFromBelow: DATA");
		switch (mh->dh_fc.fc_subtype) {
		case MAC_Subtype_Data:
			if ((u_int32_t)ETHER_ADDR(mh->dh_ra) == (u_int32_t)(mac_->index_)) {
				generateACKFrame(p);
				rxcSIFSTimer_.sched(mac_->macmib_.getSIFS());
				mac_->recvDATA(p);
				setRXCState(RXC_wait_SIFS);
			} else {
				mac_->recvDATA(p);
				setRXCState(RXC_Idle);
			}
			break;
		default:
			cout<<"unkown mac data type"<<endl;
		}
		break;
	default:
		cout<<"logic error at handleincomingframe"<<endl;
	}
}

void RXC::handleSIFStimeout() {
	if (mac_->MAC_DBG)
		mac_->log("RXC", "SIFS finished");
	if (pCTRL != 0) {
		mac_->transmit(pCTRL, Callback_RXC);
	}
	setRXCState(RXC_wait_sent);
}

void RXC::handleTXConfirm() {
	Packet::free(pCTRL);
	pCTRL = 0;
	setRXCState(RXC_Idle);
}

void RXC::generateACKFrame(Packet *p) {
	Packet *ack = Packet::alloc();
	hdr_cmn* ch = HDR_CMN(ack);
	struct ack_frame *af = (struct ack_frame*)ack->access(hdr_mac::offset_);
	struct hdr_mac802_11 *dh = HDR_MAC802_11(p);

	ch->uid() = 0;
	ch->ptype() = PT_MAC;
	ch->size() = mac_->phymib_.getACKlen();
	ch->iface() = -2;
	ch->error() = 0;
	ch->mod_scheme_= (ModulationScheme)mac_->phymib_.getBasicModulationScheme();
	bzero(af, MAC_HDR_LEN);

	af->af_fc.fc_protocol_version = MAC_ProtocolVersion;
	af->af_fc.fc_type = MAC_Type_Control;
	af->af_fc.fc_subtype = MAC_Subtype_ACK;
	af->af_fc.fc_to_ds = 0;
	af->af_fc.fc_from_ds = 0;
	af->af_fc.fc_more_frag = 0;
	af->af_fc.fc_retry = 0;
	af->af_fc.fc_pwr_mgt = 0;
	af->af_fc.fc_more_data = 0;
	af->af_fc.fc_wep = 0;
	af->af_fc.fc_order = 0;

	u_int32_t dst = ETHER_ADDR(dh->dh_ta);
	STORE4BYTE(&dst, (af->af_ra));
	ch->txtime() = mac_->txtime(ch->size(),
			mac_->phymib_.getBasicModulationScheme());
	af->af_duration = 0;
	pCTRL = ack;
}

void RXC::generateCTSFrame(Packet *p) {
	Packet *cts = Packet::alloc();
	hdr_cmn* ch = HDR_CMN(cts);
	struct rts_frame *rf = (struct rts_frame*)p->access(hdr_mac::offset_);
	struct cts_frame *cf = (struct cts_frame*)cts->access(hdr_mac::offset_);
	ch->uid() = 0;
	ch->ptype() = PT_MAC;
	ch->size() = mac_->phymib_.getCTSlen();
	ch->iface() = -2;
	ch->error() = 0;
	ch->mod_scheme_= (ModulationScheme)mac_->phymib_.getBasicModulationScheme();
	bzero(cf, MAC_HDR_LEN);

	cf->cf_fc.fc_protocol_version = MAC_ProtocolVersion;
	cf->cf_fc.fc_type = MAC_Type_Control;
	cf->cf_fc.fc_subtype = MAC_Subtype_CTS;
	cf->cf_fc.fc_to_ds = 0;
	cf->cf_fc.fc_from_ds = 0;
	cf->cf_fc.fc_more_frag = 0;
	cf->cf_fc.fc_retry = 0;
	cf->cf_fc.fc_pwr_mgt = 0;
	cf->cf_fc.fc_more_data = 0;
	cf->cf_fc.fc_wep = 0;
	cf->cf_fc.fc_order = 0;

	u_int32_t dst = ETHER_ADDR(rf->rf_ta);
	STORE4BYTE(&dst, (cf->cf_ra));
	ch->txtime() = mac_->txtime(ch->size(),
			mac_->phymib_.getBasicModulationScheme());
	cf->cf_duration = mac_->usec(mac_->sec(rf->rf_duration)
			- mac_->macmib_.getSIFS()- mac_->txtime(mac_->phymib_.getCTSlen(),
			mac_->phymib_.getBasicModulationScheme()));
	pCTRL = cts;
}

void RXC::setRXCState(RXCState newstate) {
	if (mac_->MAC_DBG) {
		char msg [1000];
		sprintf(msg, "%d -> %d", rxc_state_, newstate);
		mac_->log("RXCState", msg);
	}
	rxc_state_=newstate;
}


double Mac802_11Ext::txtime(int )
{
	/* clobber inherited txtime() */
	abort();
}


//-------rx coordination-------//

