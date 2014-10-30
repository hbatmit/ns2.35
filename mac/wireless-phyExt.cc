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


#include <math.h>
#include <functional>
#include <wireless-phyExt.h>
#include <ip.h>
#include <agent.h>
#include <trace.h>
#include "mac-802_11Ext.h"
#include <map>
#include <iostream>
#include <ranvar.h>
#include <float.h>

/* ======================================================================
 WirelessPhyExt Interface
 ====================================================================== */

static class WirelessPhyExtClass : public TclClass {
public:
	WirelessPhyExtClass() :
		TclClass("Phy/WirelessPhyExt") {
	}
	TclObject* create(int, const char*const*) {
		return (new WirelessPhyExt);
	}
} class_WirelessPhyExt;

WirelessPhyExt::WirelessPhyExt() :
	WirelessPhy(), tX_Timer(this), rX_Timer(this), preRX_Timer(this) {
	bind("CPThresh_", &CPThresh_); //not used at the moment, BPSK_SNR_ is used in preamble capture
	bind("RXThresh_", &RXThresh_); //not used at the moment
	bind("CSThresh_", &CSThresh_);
	bind("Pt_", &Pt_);
	bind("freq_", &freq_);
	bind("L_", &L_);
	bind("PreambleCaptureSwitch_", &PreambleCaptureSwitch_);
	bind("DataCaptureSwitch_", &DataCaptureSwitch_);
	bind("SINR_PreambleCapture_", &SINR_PreambleCapture_);
	bind("SINR_DataCapture_", &SINR_DataCapture_);
	bind("HeaderDuration_", &HeaderDuration_);
	bind("PHY_DBG_", &PHY_DBG);
	bind("BasicModulationScheme_", &BasicModulationScheme_);
	bind("trace_dist_", &trace_dist_);
	bind("noise_floor_", &noise_floor_);
	bind("PowerMonitorThresh_", &PowerMonitorThresh_);

	lambda_ = SPEED_OF_LIGHT / freq_;
	node_ = 0;
	ant_ = 0;
	propagation_ = 0;
	powerMonitor = new PowerMonitor(this);

	//channel state
	state = SEARCHING;
	pkt_RX = 0;
	power_RX = 0;
}

int WirelessPhyExt::command(int argc, const char*const* argv) {
	TclObject *obj;
	if (argc == 3) {
		if (strcasecmp(argv[1], "setMacAddr")==0) {
			return TCL_OK;
		} else if ( (obj = TclObject::lookup(argv[2])) == 0) {
			fprintf(stderr,"WirelessPhyExt: %s lookup of %s failed\n",
			argv[1], argv[2]);
			return TCL_ERROR;
		} else if (strcmp(argv[1], "propagation") == 0) {
			assert(propagation_ == 0);
			propagation_ = (Propagation*) obj;
			return TCL_OK;
		} else if (strcasecmp(argv[1], "antenna") == 0) {
			ant_ = (Antenna*) obj;
			return TCL_OK;
		} else if (strcasecmp(argv[1], "node") == 0) {
			assert(node_ == 0);
			node_ = (Node *)obj;
			return TCL_OK;
		}
	}
	return WirelessPhy::command(argc, argv);
}

void WirelessPhyExt::sendDown(Packet *p) {
	/*
	 * Sanity Check
	 */
	assert(initialized());
	if (PHY_DBG)
		log("SendDown", "");
	switch (state) {
	case TXing:
		cout << "wrong in MAC logic case 1"<<endl;
		exit(1);
		break;
	case RXing:
		rX_Timer.cancel();
		// case 7
		discard(pkt_RX, power_RX, "INT");
		pkt_RX=0;
		power_RX=0;
		break;
	case PreRXing:
		preRX_Timer.cancel();
		// case 6
		discard(pkt_RX, power_RX, "INT");
		pkt_RX=0;
		power_RX=0;
		break;
	case SEARCHING:
		break;
	default:
		cout << "Only four states valid"<<endl;
		exit(1);
	}

	/*
	 *  Stamp the packet with the interface arguments
	 */
	p->txinfo_.stamp((MobileNode*)node(), ant_->copy(), Pt_, lambda_);

	// Send the packet &  set sender timer
	struct hdr_cmn * cmh = HDR_CMN(p);
	setState(TXing);
	tX_Timer.sched(cmh->txtime());
	powerMonitor->recordPowerLevel(Pt_, cmh->txtime());
	downtarget_->recv(p, this);
}

int WirelessPhyExt::sendUp(Packet *p) {
	/*
	 * Sanity Check
	 */
	assert(initialized());
	assert(p);
	// struct hdr_mac802_11* dh = HDR_MAC802_11(p);
	struct hdr_cmn * cmh = HDR_CMN(p);

	PacketStamp s;
	double Pr;

	if (propagation_) {
		s.stamp((MobileNode*)node(), ant_, 0, lambda_);
		// pass the packet to RF model for the calculation of Pr
		Pr = propagation_->Pr(&p->txinfo_, &s, this);
		powerMonitor->recordPowerLevel(Pr, cmh->txtime());

		if (PHY_DBG) {
			char msg[1000];
			sprintf(msg, "Id: %d Pr: %f PL: %f SINR: %f TXTIME: %f", cmh->uid(), Pr
					*1e9, powerMonitor->getPowerLevel()*1e9, Pr
					/(powerMonitor->getPowerLevel()-Pr), cmh->txtime());
			log("Sendup", msg);
		}

		switch (state) {
		case TXing:
			//case 12
			discard(p, Pr, "TXB");
			setState(TXing);
			break;
		case SEARCHING:
			if ((powerMonitor->SINR(Pr)
					>= modulation_table[BasicModulationScheme_].SINR_ratio)) {
				power_RX = Pr;
				pkt_RX=p->copy();
				setState(PreRXing);
				preRX_Timer.sched(HeaderDuration_); // preamble and PCLP header
				break;
			} else {
				//case 1
				discard(p, Pr, "SXB");
				setState(SEARCHING);
				break;
			}
		case PreRXing:
			if (powerMonitor->SINR(power_RX)
					>= modulation_table[BasicModulationScheme_].SINR_ratio) {
				//case 4
				discard(p, Pr, "PXB");
				if (PHY_DBG)
					log("PCAP 1st SUCC", "");
				setState(PreRXing);
				break;
			} else {
				// the preamble of pkt_RX is corrupted
				// case 2
				discard(pkt_RX, power_RX, "PXB");
				Packet::free(pkt_RX);
				power_RX=0;
				preRX_Timer.cancel();
				pkt_RX=0;
				if (PHY_DBG)
					log("PCAP 1st FAIL", "");

				if (PreambleCaptureSwitch_) {

					if (powerMonitor->SINR(Pr) >= SINR_PreambleCapture_) {
						pkt_RX=p->copy();
						power_RX =Pr;
						if (PHY_DBG)
							log("PCAP 2nd SUCC", "");
						setState(PreRXing);
						preRX_Timer.sched(HeaderDuration_);
						break;
					} else {
						//case 3
						discard(p, Pr, "PXB");
						if (PHY_DBG)
							log("PCAP 2nd FAIL", "");
						setState(SEARCHING);
						break;
					}
				} else {
					discard(p, Pr, "PXB");
					if (PHY_DBG)
						log("PCAP 2nd FAIL N/A", "");
					setState(SEARCHING);
					break;
				}
			}
		case RXing:
			if (powerMonitor->SINR(power_RX) >= SINR_Th_RX) {
				//case 8
				discard(p, Pr, "RXB");
				if (PHY_DBG)
					log("DCAP 1st SUCC", "");
				setState(RXing);
				break;
			} else {
				if (PHY_DBG)
					log("DCAP 1st FAIL", "");
				HDR_CMN(pkt_RX)->error()=1;
				if (DataCaptureSwitch_) {
					if (powerMonitor->SINR(Pr) >= SINR_DataCapture_) {
						// case 9
						rX_Timer.cancel();
						discard(pkt_RX, power_RX, "RXB");
						pkt_RX=0;
						//------ start the prerx of this new packet p---//
						pkt_RX=p->copy();
						power_RX =Pr;
						if (PHY_DBG)
							log("DCAP 2nd SUCC", "");
						setState(PreRXing);
						preRX_Timer.sched(HeaderDuration_);
					} else {
						// case 10
						discard(p, Pr, "RXB");
						if (PHY_DBG)
							log("DCAP 2nd FAIL", "");
						setState(RXing);
					}
				} else {
					// case 11
					discard(p, Pr, "RXB");
					if (PHY_DBG)
						log("DCAP 2nd FAIL N/A", "");
					setState(RXing);
				}
				break;
			}
		default:
			cout<<"packet arrive from chanel at invalid PHY state"<<endl;
			exit(-1);
		}
	}
	return 0; // the incoming MAC frame will be freed by phy.cc.
}

double WirelessPhyExt::SINR_Th(int modulationScheme) {
	if (PHY_DBG) {
		char msg[1000];
		sprintf(msg, ": = %s threshold %f ",
				modulation_table[modulationScheme].schemeName,
				modulation_table[modulationScheme].SINR_ratio);
		log("Mod_Scheme", msg);
	}
	return modulation_table[modulationScheme].SINR_ratio;
}

void WirelessPhyExt::handle_TXtimeout() {
	assert(state==TXing);
	Mac802_11Ext * mac_ = (Mac802_11Ext*)(uptarget_);
	mac_->handleTXEndIndication();
	setState(SEARCHING);
}

void WirelessPhyExt::handle_RXtimeout() {
	assert(state==RXing);
	setState(SEARCHING);
	struct hdr_cmn * cmh = HDR_CMN(pkt_RX);
	Mac802_11Ext * mac_ = (Mac802_11Ext*)(uptarget_);
	if (cmh->error()) {
		//case 5, 9 
		discard(pkt_RX, power_RX, "RXB");
		pkt_RX=0;
		power_RX=0;
		mac_->handleRXEndIndication(0);
	} else {
		mac_->handleRXEndIndication(pkt_RX);
		pkt_RX=0;
		power_RX=0;
	}
}

void WirelessPhyExt::handle_PreRXtimeout() {
	assert(state==PreRXing);

	assert(pkt_RX);
	struct hdr_cmn * cmh = HDR_CMN(pkt_RX);
	SINR_Th_RX = SINR_Th(cmh->mod_scheme_);
	//	cout<<"sinr"<<SINR_Th_RX<<endl;
	if (powerMonitor->SINR(power_RX) < SINR_Th_RX) {
		if (PHY_DBG)
			log("PreRxTimeout", "BodyError");
		cmh->error() = 1;
	}
	setState(RXing);
	rX_Timer.sched(cmh->txtime()-HeaderDuration_); //receiving rest of the MAC frame
	Mac802_11Ext * myUpMAC = (Mac802_11Ext*)(uptarget_);
	myUpMAC->handleRXStartIndication();
}

void WirelessPhyExt::sendCSBusyIndication() {
	Mac802_11Ext * myUpMAC = (Mac802_11Ext*)(uptarget_);
	myUpMAC->handlePHYBusyIndication();
}

void WirelessPhyExt::sendCSIdleIndication() {
	Mac802_11Ext * myUpMAC = (Mac802_11Ext*)(uptarget_);
	myUpMAC->handlePHYIdleIndication();
}

int WirelessPhyExt::discard(Packet *p, double power, char* reason) {
	struct hdr_cmn *ch = HDR_CMN(p);
	double modulation_SINR =SINR_Th(ch->mod_scheme_);

	double Xt, Yt, Zt; // location of transmitter
	double Xr, Yr, Zr; // location of receiver
	PacketStamp s;
	s.stamp((MobileNode*)node(), ant_, 0, lambda_);
	s.getNode()->getLoc(&Xt, &Yt, &Zt);
	p->txinfo_.getNode()->getLoc(&Xr, &Yr, &Zr);
	Xr += p->txinfo_.getAntenna()->getX();
	Yr += p->txinfo_.getAntenna()->getY();
	Zr += p->txinfo_.getAntenna()->getZ();
	Xt += s.getAntenna()->getX();
	Yt += s.getAntenna()->getY();
	Zt += s.getAntenna()->getZ();
	double dX = Xr - Xt;
	double dY = Yr - Yt;
	double dZ = Zr - Zt;
	double d = sqrt(dX * dX + dY * dY + dZ * dZ);

	if (d < trace_dist_) {
		if (power < modulation_SINR * noise_floor_)
			reason = "DND";
		if (power < modulation_table[BasicModulationScheme_].SINR_ratio * noise_floor_ )
			reason = "PND";
		drop(p, reason);
	} else {
		Packet::free(p);
	}
	return 0;
}

void WirelessPhyExt::dump(void) const {
}

void WirelessPhyExt::log(char * event, char* additional) {
	if (PHY_DBG)
		cout<<"L "<<Scheduler::instance().clock()<<" "<<node_->nodeid()<<" "<<"PHY"<<" "<<event<<" "
				<<additional<<endl;
}

void WirelessPhyExt::setState(int newstate) {
	if (PHY_DBG) {
		char msg[1000];
		sprintf(msg, "%d -> %d", state, newstate);
		log("PHYState", msg);
	}

	if ( state == SEARCHING && newstate != SEARCHING ) {
		// indicate MAC busy since we are either receiving or sending
        // only if we have not been busy already
        if (powerMonitor->getPowerLevel() < CSThresh_) {
        	sendCSBusyIndication();
        }
    } else if ( state != SEARCHING && newstate == SEARCHING) {
        // if we are idle AND powerLevel is below CSthresh, indicate MAC idle
        if (powerMonitor->getPowerLevel() < CSThresh_) {
        	sendCSIdleIndication();
        }
	}

	state = newstate;
}

int WirelessPhyExt::getState() {
	return state;
}

void TX_Timer::expire(Event *) {
	wirelessPhyExt->handle_TXtimeout();
	return;
}

void RX_Timer::expire(Event *) {
	wirelessPhyExt->handle_RXtimeout();
	return;
}

void PreRX_Timer::expire(Event *) {
	wirelessPhyExt->handle_PreRXtimeout();
	return;
}

//**************************************************************
PowerMonitor::PowerMonitor(WirelessPhyExt * phy) {
	// initialize, the NOISE is the environmental noise
	wirelessPhyExt = phy;
	CS_Thresh = wirelessPhyExt->CSThresh_; //  monitor_Thresh = CS_Thresh;
	monitor_Thresh = wirelessPhyExt->PowerMonitorThresh_;
	powerLevel = wirelessPhyExt->noise_floor_; // noise floor is -99dbm
}

void PowerMonitor::recordPowerLevel(double signalPower, double duration) {
	// to reduce the number of entries recorded in the interfList
	if (signalPower < monitor_Thresh )
		return;

	interf timerEntry;
    timerEntry.Pt  = signalPower;
    timerEntry.end = Scheduler::instance().clock() + duration;

    list<interf>:: iterator i;
    for (i=interfList_.begin();  i != interfList_.end() && i->end <= timerEntry.end; i++) { }
    interfList_.insert(i, timerEntry);

	resched((interfList_.begin())->end - Scheduler::instance().clock());

    powerLevel += signalPower; // update the powerLevel

    if (wirelessPhyExt->getState() == SEARCHING && powerLevel >= CS_Thresh) {
		wirelessPhyExt->sendCSBusyIndication();
    }
}

double PowerMonitor::getPowerLevel() {
	if (powerLevel > wirelessPhyExt->noise_floor_)
		return powerLevel;
	else
		return wirelessPhyExt->noise_floor_;
}

void PowerMonitor::setPowerLevel(double power) {
	powerLevel = power;
}

double PowerMonitor::SINR(double Pr) {
	if (getPowerLevel()-Pr<=0) {
		//			cout<<"PowerLevel lower than Pr"<<endl;exit(-1);
		//			char msg[1000];
		//			sprintf(msg,"PowerLevel %f lower than Pr %f = %f",getPowerLevel()*1e9,Pr*1e9,(getPowerLevel()-Pr));
		//			wirelessPhyExt->log("PMX",msg);
		//			exit(-1);
		return 0.0; //internal event contention, new msg arrives betweeen the expire of two timers.
	}
	return Pr/(getPowerLevel()-Pr);
}

double WirelessPhyExt::getDist(double Pr, double Pt, double Gt, double Gr,
			    double hr, double ht, double L, double lambda)
{
	if (propagation_) {
		return propagation_->getDist(Pr, Pt, Gt, Gr, hr, ht, L,
					     lambda);
	}
	return 0;
}

void PowerMonitor::expire(Event *) {
	double pre_power = powerLevel;
	double time = Scheduler::instance().clock();

   	list<interf>:: iterator i;
   	i=interfList_.begin();
   	while(i != interfList_.end() && i->end <= time) {
       	powerLevel -= i->Pt;
       	interfList_.erase(i++);
   	}
	if (!interfList_.empty())
		resched((interfList_.begin())->end - Scheduler::instance().clock());

    	char msg[1000];
	sprintf(msg, "Power: %f -> %f", pre_power*1e9, powerLevel*1e9);
	wirelessPhyExt->log("PMX", msg);

	// check if the channel becomes idle ( busy -> idle )
	if (wirelessPhyExt->getState() == SEARCHING && powerLevel < CS_Thresh) {
		wirelessPhyExt->sendCSIdleIndication();
	}
}
