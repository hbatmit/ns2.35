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



#ifndef __mdart_table__
#define __mdart_table__



#include <mdart/mdart_function.h>
#include <mdart/mdart_neighbor.h>
#include <mdart/mdart.h>



//------------------------------------------------------------------------------
// MDART Routing Table Entry
//------------------------------------------------------------------------------
class RoutingEntry {
	public:
		RoutingEntry(nsaddr_t nextHopAdd, nsaddr_t nextHopId, nsaddr_t networkId, u_int32_t hopNumber, double etxMetric, bitset<ADDR_SIZE> routeLog) {
			nextHopId_ = nextHopId;
			nextHopAdd_ = nextHopAdd;
			networkId_ = networkId;
			hopNumber_ = hopNumber;
			etxMetric_ = etxMetric;
			macFailed_ = false;
			routeLog_ = routeLog;
		}

		// Routing Entry data access functions
		inline nsaddr_t nextHopId() const {
			return nextHopId_;
		}
		inline nsaddr_t nextHopAdd() const {
			return nextHopAdd_;
		}
		inline nsaddr_t networkId() const {
			return networkId_;
		}
		inline u_int32_t hopNumber() const {
			return hopNumber_;
		}
		inline double etxMetric() const {
			return etxMetric_;
		}
		inline void upLinkQuality() {
			etxMetric_++;
		}
		inline bitset<ADDR_SIZE> routeLog() const {
			return routeLog_;
		}
		inline bool macFailed() const {
			return macFailed_;
		}
		inline void macFailed(bool macFailed) {
			macFailed_ = macFailed;
		}
	private:
		nsaddr_t			nextHopId_;
		nsaddr_t			nextHopAdd_;
		nsaddr_t			networkId_;
		u_int32_t			hopNumber_;
		double				etxMetric_;
		bool				macFailed_;
		bitset<ADDR_SIZE>	routeLog_;
};



//------------------------------------------------------------------------------
// MDART Routing Table Entry Set
//------------------------------------------------------------------------------
struct setCompare {
	bool operator() (const RoutingEntry* entry1, const RoutingEntry* entry2) const {
		bitset<ADDR_SIZE> n1 (entry1->nextHopAdd());
		bitset<ADDR_SIZE> n2 (entry2->nextHopAdd());
		int i;
		for(i=ADDR_SIZE-1; i>-1; i--) {
			if (n1.test(i) < n2.test(i)) {
				return 1;
			}
			if (n1.test(i) > n2.test(i)) {
				return 0;
			}
		}
		return 0;
	}
};

typedef std::set<RoutingEntry *, setCompare> entrySet;



//------------------------------------------------------------------------------
// MDART Routing Table
//------------------------------------------------------------------------------
class RoutingTable {
	public:
		RoutingTable(MDART* mdart) {
			mdart_ = mdart;
			table_ = new std::vector<entrySet*>;
			int i;
			for(i=0; i<ADDR_SIZE; i++) {
				entrySet* entrySet_ = new entrySet;
				table_->push_back(entrySet_);
			}
		}
		
		// Routing Table management functions		 
		void				clear();
		void				clear(u_int32_t);
		void				purge(nsaddr_t id);
//		PacketData*			getUpdate();
		string				getUpdate();
		void				setUpdate(Neighbor* neighbor_);
		void				macFailed(nsaddr_t nextHop);

		// Routing Table data access functions		 
		nsaddr_t			levelId(u_int32_t);
		nsaddr_t			id(u_int32_t);
		nsaddr_t			networkId(u_int32_t);
		u_int32_t			hopNumber(u_int32_t);
		double				etxMetric(u_int32_t);
		bitset<ADDR_SIZE>	routeLog(u_int32_t);
		

		// Routing Table entries management functions
		void				addEntry(int, nsaddr_t, nsaddr_t, nsaddr_t, u_int32_t, double, bitset<ADDR_SIZE>);
		nsaddr_t			getEntry(nsaddr_t);
		nsaddr_t			DAGetEntry(nsaddr_t);
//		void				removeEntry(nsaddr_t);

		// Routing Table Data Access functions
		inline entrySet*	getSibling(u_int32_t levelSibling) const {
			return table_->at(levelSibling);
		}
		
		// Routing Table debug functions		 
		void				print();
		u_int32_t			size();

	private:
		MDART* mdart_;
		std::vector<entrySet*>* table_;
};


#endif /* __mdart_table__ */
