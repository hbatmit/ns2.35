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



#ifndef __mdart_dht_h__
#define __mdart_dht_h__



#include <mdart/mdart_function.h>



//------------------------------------------------------------------------------
// DHT entry
//------------------------------------------------------------------------------
class DHT_Entry {
	public:
		DHT_Entry(nsaddr_t id, nsaddr_t address) {
			id_ = id;
			address_ = address;
			expire_ = CURRENT_TIME + MDART_DHT_ENTRY_TIMEOUT;
		}
		inline nsaddr_t id() const {
			return id_;
		}
		inline nsaddr_t address() const {
			return address_;
		}
		inline void	address(nsaddr_t address) {
			address_ = address;
		}
		inline double	expire() const {
			return expire_;
		}
		inline void expire(double expire) {
			expire_ = expire;
		}
private:
		nsaddr_t		id_;
		nsaddr_t		address_;
		double		expire_;
};



//------------------------------------------------------------------------------
// DHT Table
//------------------------------------------------------------------------------
typedef std::set<DHT_Entry*> dhtTable;



//------------------------------------------------------------------------------
// DHT timer
//------------------------------------------------------------------------------
class DHT;
class DHTTimer : public Handler {
public:
	DHTTimer(DHT* dht);
	void	handle(Event*);
protected:
	DHT*			dht_;
	Event			intr;
	u_int32_t	counter;
};



//------------------------------------------------------------------------------
// DHT
//------------------------------------------------------------------------------
class DHT {
public:
	// DHT
	DHT();
	~DHT();
	// DHT functions
	void		addEntry(nsaddr_t, nsaddr_t);
	void		addEntry(nsaddr_t, nsaddr_t, double);
	void		purge();
	DHT_Entry*	getEntry(u_int32_t);
	nsaddr_t	findAdd(nsaddr_t);
	nsaddr_t	findId(nsaddr_t);
	u_int32_t	DHTSize();
	PacketData*	getDHTUpdate();
//	void		setDHTUpdate(PacketData*);
	// Debug functions
	void		printDHT();
private:
	// DHT private functions
	DHT_Entry*	getEntry(nsaddr_t);
	void		removeEntry(nsaddr_t);
	void		clear();
	// DHT private members
	DHTTimer*	timer_;
	dhtTable*	table_;
};


#endif /* __mdart_dht_h__ */
