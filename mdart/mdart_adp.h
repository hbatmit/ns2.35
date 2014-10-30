/*******************************************************************************
 *                                                                             *
 *   M-DART Routing Protocol                                                   *
 *                                                                             *
 *   Copyright (C) 2006 by Marcello Caleffi                                    *
 *   marcello.caleffi@unina.it                                                 *
 *                                                                             *
 * Redistribution and use in source and binary forms, with or without          *
 * modification, are permitted provided that the following conditions are met: *
 * 1. Redistributions of source code must retain the above copyright notice,   *
 *    this list of conditions and the following disclaimer.                    *
 * 2. Redistributions in binary form must reproduce the above copyright        *
 *    notice, this list of conditions and the following disclaimer in the      *
 *    documentation and/or other materials provided with the distribution.     *
 * 3. The name of the author may not be used to endorse or promote products    *
 *    derived from this software without specific prior written permission.    *
 *                                                                             *
 * This software is provided by the author ``as is'' and any express or        *
 * implied warranties, including, but not limited to, the implied warranties   *
 * of merchantability and fitness for a particular purpose are disclaimed.     *
 * in no event shall the author be liable for any direct, indirect,            *
 * incidental, special, exemplary, or consequential damages (including, but    *
 * not limited to, procurement of substitute goods or services; loss of use,   *
 * data, or profits; or business interruption) however caused and on any       *
 * theory of liability, whether in contract, strict liability, or tort         *
 * (including negligence or otherwise) arising in any way out of the use of    *
 * this software, even if advised of the possibility of such damage.           *
 *                                                                             *
 * The M-DART code has been developed by Marcello Caleffi during his Ph.D. at  *
 * the Department of Biomedical, Electronic and Telecommunications Engineering *
 * University of Naples Federico II, Italy.                                    *
 *                                                                             *
 * In order to give credit and recognition to the author, if you use M-DART    *
 * results or results obtained by modificating the M-DART source code, please  *
 * cite one of the following papers:                                           *
 * - M. Caleffi, L. Paura, "M-DART: Multi-Path Dynamic Address RouTing",       *
 *   Wireless Communications and Mobile Computing, 2010                        *
 * - M. Caleffi, G. Ferraiuolo, L. Paura, "Augmented Tree-based Routing        *
 *   Protocol for Scalable Ad Hoc Networks", Proc. of IEEE MASS '07: IEEE      *
 *   Internatonal Conference on Mobile Adhoc and Sensor Systems, Pisa (Italy), *
 *   October 8-11 2007.                                                        *
 *                                                                             *
 ******************************************************************************/



#ifndef __mdart_adp_h__
#define __mdart_adp_h__


#include <mdart/mdart.h>
#include <mdart/mdart_function.h>
#include <mdart/mdart_dht.h>



//------------------------------------------------------------------------------
// Address Discovery Protocol (ADP)
//------------------------------------------------------------------------------
class MDART;
class ADPDaupTimer;
class ADPDarqTimer;

class ADP {
public:
	// ADP
	ADP(MDART* mdart);
	~ADP();
	// ADP network address function
	void					updateAdd();
	// ADP message functions
	void					checkDarq();
	void					sendDarq(nsaddr_t, int);
	void					recvDarq(Packet*);
	void					sendDarp(Packet*);
	void					recvDarp(Packet*);
	void					sendDaup();
	void					recvDaup(Packet*);
	void					sendDabr();
	void					recvDabr(Packet*);
	// ADP DHT functions
	void					addEntry(nsaddr_t id, nsaddr_t address);
	void					addEntry(nsaddr_t id, nsaddr_t address, double);
	inline uint32_t			DHTSize() {
		return dht_->DHTSize();
	}
	inline void				printDHT() {
		return dht_->printDHT();
	}
	inline nsaddr_t			findAdd(nsaddr_t id) {
		return dht_->findAdd(id);
	}
	inline DHT_Entry*		getEntry(u_int32_t pos) {
		return dht_->getEntry(pos);
	}
	inline nsaddr_t			findId(nsaddr_t add) {
		return dht_->findId(add);
	}
private:
	inline u_int32_t	pckSeqNum() {
		return pckSeqNum_++;
	}
	// ADP private members
	MDART*		mdart_;
	DHT*		dht_;
	ADPDaupTimer*	timer_;
	ADPDarqTimer*	timerRx_;
	u_int32_t	pckSeqNum_;
};



//------------------------------------------------------------------------------
// ADP timers
//------------------------------------------------------------------------------
class ADPDaupTimer : public Handler {
public:
	ADPDaupTimer(ADP* adp) {
		adp_ = adp;
		counter = 0;
		handle((Event*) 0);
	}
	inline void	handle(Event*) {
		if (counter > 0) {
			adp_->sendDaup();
		}
		else {
			counter++;
		}
		double interval_ = ADP_DAUP_FREQ * (0.9 + 0.2 * Random::uniform());
		Scheduler::instance().schedule(this, &intr, interval_);
	}
protected:
	ADP*		adp_;
	Event		intr;
	uint32_t	counter;
};

class ADPDarqTimer : public Handler {
public:
	ADPDarqTimer(ADP* adp) {
		adp_ = adp;
		counter = 0;
		handle((Event*) 0);
	}
	void	handle(Event*) {
		if (counter > 0) {
			adp_->checkDarq();
		}
		else {
			counter++;
		}
		Scheduler::instance().schedule(this, &intr, ADP_DARQ_CHECK_FREQ);
	}
protected:
	ADP*		adp_;
	Event		intr;
	uint32_t	counter;
};



#endif /* __mdart_adp_h_ */
