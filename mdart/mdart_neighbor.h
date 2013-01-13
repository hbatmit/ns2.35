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



#ifndef __mdart_neighbor__
#define __mdart_neighbor__



#include <mdart/mdart_function.h>



//------------------------------------------------------------------------------
// Neighbor Routing Table
//------------------------------------------------------------------------------
class NeighborTable {
public:
	NeighborTable() {
		int i;
		for (i=0; i<ADDR_SIZE; i++) {
			hopNumber_[i] = INFINITO;
			etxMetric_[i] = RTR_ETX_MAX;
		}
	}
	// Neighbor Routing Table update function
	void update(string);

	// Neighbor Routing Table routing functions
	inline int32_t insertionPoint() const {
		int i;
		for (i=ADDR_SIZE-1; i>-1; i--) {
			if (hopNumber_[i] == INFINITO) {
				return i;
			}
		}
		return i;
	}
	inline nsaddr_t levelId(u_int32_t levelSibling, nsaddr_t id) const {
		int i;
		for (i=levelSibling; i>-1; i--) {
			if (hopNumber_[i] < INFINITO && networkId_[i] < id) {
				id = networkId_[i];
			}
		}
		return id;
	}
	inline nsaddr_t networkId(u_int32_t levelSibling) const {
		nsaddr_t networkId =  INFINITO;
		if (hopNumber_[levelSibling] < INFINITO && networkId_[levelSibling] < networkId) {
			networkId = networkId_[levelSibling];
		}
		return networkId;
	}
	inline nsaddr_t hopNumber(u_int32_t levelSibling) const {
		u_int32_t hopNumber = INFINITO;
		if (hopNumber_[levelSibling] < hopNumber) {
			hopNumber = hopNumber_[levelSibling];
		}
		return hopNumber;
	}
	inline double etxMetric(u_int32_t levelSibling) const {
		double etxMetric = RTR_ETX_MAX;
		if (etxMetric_[levelSibling] < etxMetric) {
			etxMetric = etxMetric_[levelSibling];
		}
		return etxMetric;
	}
	inline bitset<ADDR_SIZE> routeLog(u_int32_t levelSibling) const {
		bitset<ADDR_SIZE> routeLog;
		routeLog.reset();
		if (hopNumber_[levelSibling] < INFINITO) {
			routeLog = routeLog_[levelSibling];
		}
		return routeLog;
	}
	inline u_int32_t routeLog(u_int32_t levelSibling, u_int32_t j) const {
		bitset<ADDR_SIZE> routeLog;
		routeLog.reset();
		if (hopNumber_[levelSibling] < INFINITO) {
			routeLog = routeLog_[levelSibling];
		}
		return (u_int32_t) routeLog[j];
	}

	inline bool entryPresent(u_int32_t levelSibling_) const {
		return (hopNumber_[levelSibling_] < INFINITO);
	}

	// Neighbor Routing Table debug functions
	void print(nsaddr_t address_) const;
private:
	nsaddr_t			networkId_[ADDR_SIZE];
	u_int32_t			hopNumber_[ADDR_SIZE];
	double				etxMetric_[ADDR_SIZE];
	bitset<ADDR_SIZE>	routeLog_[ADDR_SIZE];
};



//------------------------------------------------------------------------------
// Hello container
//------------------------------------------------------------------------------
struct hello {
	double		time_;
	u_int32_t	seqNum_;
};
typedef std::vector<hello> helloVector;



//------------------------------------------------------------------------------
// Neighbor
//------------------------------------------------------------------------------
class Neighbor {
	public:
		Neighbor(nsaddr_t id, nsaddr_t address, u_int32_t helloSeqNum);
//		~Neighbor();
		
		// Neighbor functions
		void purgeHello();
		// Neighbor debug functions
		void printHello();

		// ETX functions
		inline double etxMetric() {
			double etxMetric_ = RTR_ETX_MAX;
			if ((revLinkQuality() * forLinkQuality_) != 0)
				etxMetric_ = 1/(revLinkQuality() * forLinkQuality_);
			if (etxMetric_ > RTR_ETX_MAX)
				etxMetric_ = RTR_ETX_MAX;
			return etxMetric_;
		}
		
		inline double linkQuality() {
			return revLinkQuality() * forLinkQuality_;
		}
		
		inline double revLinkQuality() {
			double revLinkQuality_ = 0;
			helloVector::iterator entry_;
			for(entry_ = helloVector_.begin(); entry_ != helloVector_.end(); ++entry_) {
				revLinkQuality_++;
			}
			if (revLinkQuality_/LQE_MA_EXPECTED_HELLO > 1.0)
				return 1.0;
			return revLinkQuality_/LQE_MA_EXPECTED_HELLO;
		}

		inline void forLinkQuality(double quality) {
			forLinkQuality_ = quality;
#ifdef DEBUG_NEIGHBOR
			fprintf(stdout, "\t\tforLinkQuality_ = %f\n",forLinkQuality_);
#endif
		}

		// Neighbor functions for accessing private data
		inline nsaddr_t id() const {
			return id_;
		}

		inline nsaddr_t address() const {
			return address_;
		}

		inline void address(nsaddr_t address, u_int32_t helloSeqNum) {
			address_ = address;
			expire_ = CURRENT_TIME + NDP_NEIGHBOR_EXPIRE;
			purgeHello();
			addHello(helloSeqNum);
		}

		inline double expire() const {
			return expire_;
		}

//		inline void expire(double expire) {
//			expire_ = expire;
//		}

		inline void addHello(u_int32_t seqNum) {
#ifdef DEBUG_NEIGHBOR
			fprintf(stdout, "%.9f\tNeighbor::addHello(%d)\n", CURRENT_TIME, seqNum);
			printHello();
#endif
			hello hello_;
			hello_.time_ = CURRENT_TIME;
			hello_.seqNum_ = seqNum;
			helloVector_.push_back(hello_);
#ifdef DEBUG_NEIGHBOR
			printHello();
			fprintf(stdout, "\treverse link quality = %f\n", revLinkQuality());
#endif
		}

		// Neighbor Routing Table update function
		inline void updateTable(string str) {
			table_->update(str);
		}

		// Neighbor Routing Table routing functions
		inline int32_t insertionPoint() const {
			return table_->insertionPoint();
		}
		inline nsaddr_t levelId(u_int32_t levelSibling) const {
			return table_->levelId(levelSibling, id_);
		}
		inline nsaddr_t networkId(u_int32_t levelSibling) const {
			return table_->networkId(levelSibling);
		}

		inline u_int32_t hopNumber(u_int32_t levelSibling) const {
			return table_->hopNumber(levelSibling);
		}

		inline double etxMetric(u_int32_t levelSibling) const {
			return table_->etxMetric(levelSibling);
		}

		inline bitset<ADDR_SIZE> routeLog(u_int32_t levelSibling) const {
			return table_->routeLog(levelSibling);
		}

		inline u_int32_t routeLog(u_int32_t levelSibling, u_int32_t j) const {
			return table_->routeLog(levelSibling, j);
		}

		inline bool entryPresent(u_int32_t levelSibling_) const {
			return table_->entryPresent(levelSibling_);
		}

		// Neighbor routing table debug functions
		inline void	 printTable() const {
			table_->print(address_);
		}

private:
	double			forLinkQuality_;
	nsaddr_t		address_;
	nsaddr_t		id_;
	double			expire_;
	NeighborTable*	table_;
	helloVector		helloVector_;
};



#endif /*__mdart_neighbor__*/
