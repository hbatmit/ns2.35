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



#ifndef __mdart_rqueue_h__
#define __mdart_rqueue_h_



#include <mdart/mdart_function.h>



//------------------------------------------------------------------------------
// Queue container
//------------------------------------------------------------------------------
typedef std::vector<Packet*> queue;



//------------------------------------------------------------------------------
// Queue timer
//------------------------------------------------------------------------------
class MDARTQueue;
class MDARTQueueTimer : public Handler {
public:
	MDARTQueueTimer(MDARTQueue* dht);
	void	handle(Event*);
protected:
	MDARTQueue*	queue_;
	Event		intr;
	u_int32_t	counter;
};



//------------------------------------------------------------------------------
//  MDART packet queue
//------------------------------------------------------------------------------
class MDARTQueue : public Connector {
	public:
		// Queue
		MDARTQueue();
		~MDARTQueue();
		// Default Queue functions
		inline void	recv(Packet *, Handler*) {
			abort();
		}
		inline int	command(int argc, const char * const* argv) {
			return Connector::command(argc, argv);
		}
		// Queue functions
		void		enque(Packet *p);
		Packet*		deque();
		Packet*		deque(nsaddr_t dst);
		bool		isExpired(int);
		Packet*		getPacket(int);
		inline int	size() {
			return (int) queue_->size();
		}
//		bool	expired();
		void		purge();
//		Packet*	findExpiredDarq(Packet* p);		
//		bool findDAExpired(Packet*& p);
		// Debug queue functions
		void		printNumPacket(nsaddr_t dst);
		void		printQueue();
	private:
		// Queue private functions
		void		removeHead();
		void		clear();
		// Queue private functions
		MDARTQueueTimer*	timer_;
		queue*		queue_;
//		Packet*		removeHead();
//		void			find(nsaddr_t dst, Packet*& p, Packet*& prev);
//		bool			findExpired(Packet*& p, Packet*& prev);
//		void			verifyQueue(void);
//		Packet		*head_;
//		Packet		*tail_;
//		int			len_;
//		int			limit_;
//		double		timeout_;
};

#endif
