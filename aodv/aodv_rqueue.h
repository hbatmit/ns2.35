/*
Copyright (c) 1997, 1998 Carnegie Mellon University.  All Rights
Reserved. 

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The AODV code developed by the CMU/MONARCH group was optimized and tuned by Samir Das and Mahesh Marina, University of Cincinnati. The work was partially done in Sun Microsystems. 

*/


#ifndef __aodv_rqueue_h__
#define __aodv_rqueue_h_

//#include <packet.h>
#include <ip.h>
#include <agent.h>

/*
 * The maximum number of packets that we allow a routing protocol to buffer.
 */
#define AODV_RTQ_MAX_LEN     64      // packets

/*
 *  The maximum period of time that a routing protocol is allowed to buffer
 *  a packet for.
 */
#define AODV_RTQ_TIMEOUT     30	// seconds

class aodv_rqueue : public Connector {
 public:
        aodv_rqueue();

        void            recv(Packet *, Handler*) { abort(); }

        void            enque(Packet *p);

	inline int      command(int argc, const char * const* argv) 
	  { return Connector::command(argc, argv); }

        /*
         *  Returns a packet from the head of the queue.
         */
        Packet*         deque(void);

        /*
         * Returns a packet for destination "D".
         */
        Packet*         deque(nsaddr_t dst);
  /*
   * Finds whether a packet with destination dst exists in the queue
   */
        char            find(nsaddr_t dst);

 private:
        Packet*         remove_head();
        void            purge(void);
	void		findPacketWithDst(nsaddr_t dst, Packet*& p, Packet*& prev);
	bool 		findAgedPacket(Packet*& p, Packet*& prev); 
	void		verifyQueue(void);

        Packet          *head_;
        Packet          *tail_;

        int             len_;

        int             limit_;
        double          timeout_;

};

#endif /* __aodv_rqueue_h__ */
