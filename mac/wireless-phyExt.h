/*
 * Copyright (C) 2007 
 * Mercedes-Benz Research & Development North America, Inc.and
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

#ifndef ns_WirelessPhyExt_h
#define ns_WirelessPhyExt_h

#include "phy.h"
#include "propagation.h"
#include "modulation.h"
#include "omni-antenna.h"
#include "mobilenode.h"
#include "timer-handler.h"
#include <list>
#include <packet.h>

enum PhyState {SEARCHING = 0, PreRXing = 1, RXing = 2, TXing = 3};

typedef struct ModulationParam {
	int schemeIndex;
	char schemeName[10];
	double SINR_dB;
	double SINR_ratio;
	int NDBPS; //Data Bits Per Symbol
} ModulationParam;

const struct ModulationParam modulation_table[4] = {
//  mod  name  SINRdB   SINR  NDBPS bit
               	{ 0, "BPSK", 5, 3.1623, 24 }, { 1, "QPSK", 8, 6.3096, 48 }, { 2,
                               	"QAM16", 15, 31.6228, 96 }, { 3, "QAM64", 25, 316.2278, 192 } };

class Phy;
class WirelessPhyExt;
class Propagation;
class PowerMonitor;

//**************************************************************/
class TX_Timer : public TimerHandler {
public:
	TX_Timer(WirelessPhyExt * w) :
		TimerHandler() {
		wirelessPhyExt = w;
	}
protected:
	void expire(Event *e);
private:
	WirelessPhyExt * wirelessPhyExt;
};

class RX_Timer : public TimerHandler {
public:
	RX_Timer(WirelessPhyExt * w) :
		TimerHandler() {
		wirelessPhyExt = w;
	}
	void expire(Event *e);
private:
	WirelessPhyExt * wirelessPhyExt;
};

class PreRX_Timer : public TimerHandler {
public:
	PreRX_Timer(WirelessPhyExt * w) :
		TimerHandler() {
		wirelessPhyExt = w;
	}
	void expire(Event *e);
private:
	WirelessPhyExt * wirelessPhyExt;
};

//**************************************************************/
//                   1.0 WirelessPhyExt 
//**************************************************************/

/*
 The PHY is in the Searching state when it is neither in transmission nor reception of a frame. Within this state, the PHY evaluates each transmission event notification from the Wireless Channel object it is attached to for potentially receivable frames. If a frame arrives with sufficient received signal strength for preamble detection (i.e. SINR > BPSK threshold), the PHY moves into the PreRXing state.
 The PHY stays in the PreRXing state for the duration of preamble and Signal portion of the PLCP header.  If the SINR of this frame stays above the BPSK and 1/2 coding rate reception threshold throughout this period, the PHY moves into the RXing state to stay for the frame duration. If a later arriving frame from the channel has sufficient received signal strength to prevent proper preamble and PLCP header reception for the current frame, the PHY moves back to the Searching state. However, if this later frame has sufficiently higher signal strength for its own preamble to be heard above others, it will trigger preamble capture, which means the PHY stays in the PreRXing state with a reset timer for the new frame.
 Within the RXing state, the PHY handles the reception of the body of the current frame. It monitors the SINR throughout the frame body duration. If the SINR drops below the threshold required by the modulation and coding rate used for the frame body at any time while in this state, the PHY marks the frame with an error flag. After RXing timeout, the PHY moves back to the Searching state. It also passes the frame to the MAC, where the error flag is directly used for the CRC check decision. 
 If the frame body capture feature is enabled, then it is possible for a later arriving frame to trigger the PHY to move back to the PreRXing state for the new frame in the manner described in section 3.3.2. Otherwise, the later arriving frame has no chance of being received and is only tracked by the power monitor as an interference source.
 A transmit command from the MAC will move the PHY into the TXing state for the duration of a frame transmission regardless what the PHY is doing at the moment. The expiration of the transmission timer ends the TXing state. If a frame comes in from the channel when the PHY is in the TXing state, it is ignored and only tracked by the power monitor as interference.
 Usually the MAC will not issue a transmit command while the PHY is in the PreRXing or RXing state because of the carrier sense mechanism. However, the IEEE 802.11 standard mandates the receiver of a unicast data frame addressed to itself to turn around after SIFS and transmit an ACK frame regardless of the channel condition. Similarly, the receiver of a RTS frame, if it has an empty NAV (Network Allocation Vector), will wait for SIFS and then transmit a CTS frame regardless of the channel condition. Such scenarios are represented by the two dashed lines shown in the state machine. The PHY is designed to drop and clean up the frame it is attempting to receive and move into TXing state when this happens.
 The MAC, however, should never issue a transmit command when the PHY is still in the TXing state. The new frame has little chance of being received within its intended audience because others in general have no means to tell that a new frame is suddenly started. Therefore, the PHY is designed to issue an error and halt the simulator when this event happens because it means the MAC above has a critical error in design or implementation.

 */

class WirelessPhyExt : public WirelessPhy {
public:
	WirelessPhyExt();
	//  inline double getAntennaZ() { return ant_->getZ(); }
	inline double getL() const {
		return L_;
	}
	inline double getLambda() const {
		return lambda_;
	}
	inline Node* node(void) const {
		return node_;
	}
	void setState(int newstate);
	int getState();

	virtual int command(int argc, const char*const* argv);
	virtual void dump(void) const;

	//timer handlers
	void handle_TXtimeout();
	void handle_RXtimeout();
	void handle_PreRXtimeout();

	//signalling to MAC layer
	void sendCSBusyIndication();
	void sendCSIdleIndication();

	//ns2 calls
	void sendDown(Packet *p);
	int sendUp(Packet *p);

	int discard(Packet *p, double power, char* reason);
	double getDist(double Pr, double Pt, double Gt, double Gr,
	double hr, double ht, double L, double lambda);
    inline double getAntennaZ() { return ant_->getZ(); }
    inline double getAntennaRxGain() { return ant_->getRxGain(ant_->getX(), ant_->getY(), ant_->getZ(), lambda_); }
    inline double getAntennaTxGain() { return ant_->getTxGain(ant_->getX(), ant_->getY(), ant_->getZ(), lambda_); }
    inline double getPowerMonitorThresh() { return PowerMonitorThresh_; }


private:
	// variables to be bound
	double lambda_; // wavelength (m)
	double CSThresh_; // carrier sense threshold (W) fixed by chipset
	double CPThresh_; // capture threshold
	double RXThresh_; // capture threshold
	double Pt_; // transmitted signal power (W)
	double freq_; // frequency
	double L_; // system loss factor
	double HeaderDuration_; // preamble+SIGNAL
	int BasicModulationScheme_;
	int PreambleCaptureSwitch_; //PreambleCaptureSwitch
	int DataCaptureSwitch_; //DataCaptureSwitch
	double SINR_PreambleCapture_;
	double SINR_DataCapture_;

	int PHY_DBG;
	double trace_dist_;
	double noise_floor_;
	double PowerMonitorThresh_;

	Propagation *propagation_;
	Antenna *ant_;
	PowerMonitor *powerMonitor;
	TX_Timer tX_Timer;
	RX_Timer rX_Timer;
	PreRX_Timer preRX_Timer;
	int state;

	Packet *pkt_RX;
	double SINR_Th_RX; //SINR threshold for decode data according to the modulation scheme
	double power_RX;

	void log(char * event, char* additional); // print out state informration
	double SINR_Th(int modulationScheme);
	inline int initialized() {
		return (node_ && uptarget_ && downtarget_ && propagation_);
	}

	friend class TX_Timer;
	friend class RX_Timer;
	friend class PreRX_Timer;
	friend class PowerMonitor;
};

//**************************************************************/
//                  2.0 PowerMonitor
//**************************************************************/
/*
 The power monitor module corresponds to the PMD (Physical Media Dependent) sub-layer within the PHY. PMD is the only sub-layer that directly interacts with the analog RF signals. Therefore, all information on received signals is processed and managed in this module.
 The power monitor module keeps track of all the noise and interferences experienced by the node individually for their respective durations. Whenever the cumulative interference and noise level rises crosses the carrier sense threshold, it signals the MAC on physical carrier sense status changes. It should be noted that a node's own transmission is treated as carrier sense busy through this signaling interface as well.
 */
//**************************************************************/
enum PowerMonitorState {IDLE = 0, BUSY = 1};

struct interf {
      double Pt;
      double end;
};

class PowerMonitor : public TimerHandler {
public:
	PowerMonitor(WirelessPhyExt *);
	void recordPowerLevel(double power, double duration);
	double getPowerLevel();
	void setPowerLevel(double);
	double SINR(double Pr);
	void expire(Event *); //virtual function, which must be implemented

private:
	double CS_Thresh;
	double monitor_Thresh;//packet with power > monitor_thresh will be recorded in the monitor
	double powerLevel;
	WirelessPhyExt * wirelessPhyExt;
    list<interf> interfList_;
};

#endif /* !ns_WirelessPhyExt_h */
