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



#ifndef __mdart_ndp__
#define __mdart_ndp__



#include <mdart/mdart.h>
#include <mdart/mdart_neighbor.h>



//------------------------------------------------------------------------------
// Defining in advance some used class
//------------------------------------------------------------------------------
class NDPHelloTimer;
class NDPNeighborTimer;
class MDART;



//------------------------------------------------------------------------------
// NDP neighbor set
//------------------------------------------- -----------------------------------
//LIST_HEAD(neighborList, Neighbor);
struct neighborCompare {
	bool operator() (const Neighbor* entry1, const Neighbor* entry2) const {
		assert(entry1);
		assert(entry2);
		if ((entry1->insertionPoint() > entry2->insertionPoint()) || ((entry1->insertionPoint() == entry2->insertionPoint()) && (entry1->id() < entry2->id()))) {
			return 1;
		}
		return 0;
	}
};

typedef std::multiset<Neighbor*, neighborCompare> neighborSet;



//------------------------------------------------------------------------------
// Neigbor Discovery Protocol (NDP)
//------------------------------------------------------------------------------
class NDP {
	friend class MDART;
	public:
		NDP(MDART*);
		~NDP();

		// NDP message functions
		void		sendHello();
		void		recvHello(Packet*);

		// NDP timers functions
		void		startHelloTimer();
		void		startNeighborTimer();

		// NDP neighbor list functions
		void		neighborInsert(nsaddr_t, nsaddr_t, u_int32_t, string, double);
		Neighbor*	neighborLookup(nsaddr_t);
//		void		neighborDelete(nsaddr_t);
		void		neighborPurge();
		void		neighborPrint();
		int			neighborDegree();
		int			realNeighborDegree();
//		void		setNeighborQuality(string);
		string		getNeighborQuality();
		// NDP management functions
		void		selectAddress();
		bool		validateAddress();
		void		updateRoutingTable();
	private:
		MDART*				mdart_;
		// NDP timers
		NDPHelloTimer*		helloTimer_;
		NDPNeighborTimer*	neighborTimer_;
		// NDP neighbor list
		neighborSet*		neighborSet_;
		// HELLO packet sequence number
		inline u_int32_t	helloSeqNum() {
			return helloSeqNum_++;
		}
		u_int32_t			helloSeqNum_;
};


//------------------------------------------------------------------------------
// Defining the NDP timers
//------------------------------------------------------------------------------
class NDPHelloTimer : public Handler {
	public:
		NDPHelloTimer(NDP* ndp) {
			assert(ndp);
			ndp_ = ndp;
		}
		inline void	handle(Event*) {
			ndp_->sendHello();
			double interval_ = NDP_MIN_HELLO_INTERVAL + ((NDP_MAX_HELLO_INTERVAL - NDP_MIN_HELLO_INTERVAL) * Random::uniform());
			assert(interval_ >= 0);
			Scheduler::instance().schedule(this, &intr, interval_);			
		}
	protected:
		NDP*	ndp_;
		Event	intr;
};

class NDPNeighborTimer : public Handler {
	public:
		NDPNeighborTimer(NDP* ndp) {
			assert(ndp);
			ndp_ = ndp;
		}
		void	handle(Event*) {
			ndp_->neighborPurge();
			Scheduler::instance().schedule(this, &intr, NDP_HELLO_INTERVAL);
		}
	protected:
		NDP* ndp_;
		Event intr;
};

#endif /*__mdart_ndp__*/
