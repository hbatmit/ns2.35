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



#ifndef __mdart_h__
#define __mdart_h__



#include <mdart/mdart_function.h>
#include <mdart/mdart_ndp.h>
#include <mdart/mdart_dht.h>
#include <mdart/mdart_adp.h>
#include <mdart/mdart_queue.h>
#include <mdart/mdart_table.h>



//------------------------------------------------------------------------------
//  Declaring in advance some utilized class
//------------------------------------------------------------------------------
class NDP;
class MDART;
class ADP;
//class IDP;
class RoutingTable;



//------------------------------------------------------------------------------
// Static Lookup Table
//------------------------------------------------------------------------------
// DHT
//typedef std::map<nsaddr_t, nsaddr_t> lookupTable;



//------------------------------------------------------------------------------
// MDART Timer
//------------------------------------------------------------------------------
class MDARTTimer : public Handler {
	public:
		MDARTTimer(MDART* mdart) {
			mdart_ = mdart;
			numberOfCall_ = 0;
		}
		void 		handle(Event*);
	private:
		MDART*		mdart_;
		int		numberOfCall_;
		Event		intr;
};


//------------------------------------------------------------------------------
// MDART
//------------------------------------------------------------------------------
class MDART : public Agent {
	friend class MDARTTimer;
	friend class NDP;
	friend class ADP;
//	friend class IDP;
	friend class RoutingTable;
public:
	MDART(nsaddr_t);
	~MDART();

	// Standard functions
	int command(int, const char* const*);
	inline NsObject*		getTarget() {
		return target_;
	}

	// Packet functions
	void recv(Packet*, Handler*);
	void macFailed(Packet*);
	
	// Address & routing table functions
	void recvRoutingUpdate();
	void validateAddress();

/*
	// Lookup Table Management Functions
	void			lookupTableClear();
	void			lookupTableRmEntry(nsaddr_t);
	void			lookupTableAddEntry(nsaddr_t, nsaddr_t);
	nsaddr_t		lookupTableLookEntry(nsaddr_t);
	nsaddr_t		lookupTableLookUid(nsaddr_t);
	u_int32_t		lookupTableSize();
	void			lookupTablePrint();
*/
private:
	void	startTimer() {
		mdartTimer_->handle((Event*) 0);
	}
	// Packet functions
	void format(Packet*);
	void forward(Packet*);
	void recvMDART(Packet*);
	void sendDARequest(nsaddr_t dst);
	void sendDAUpdate();
	// Lookup Table
//	static lookupTable	lookupTable_;

	PortClassifier*	dmux_;
	Trace*			logtarget_;
	MDARTTimer*		mdartTimer_;
	NDP*			ndp_;
	ADP*			adp_;
//	IDP*			idp_;
	RoutingTable*	routingTable_;
	MDARTQueue* 				queue_;

	nsaddr_t		address_;
	nsaddr_t		oldAddress_;
	nsaddr_t		id_;		
	int			macFailed_;
	// ETX route metric instead of hop number one
	int			etxMetric_;
};



#endif /* __mdart_h__ */
