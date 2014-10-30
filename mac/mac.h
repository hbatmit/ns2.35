/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Daedalus Research
 *	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Contributed by Giao Nguyen, http://daedalus.cs.berkeley.edu/~gnguyen
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/mac/mac.h,v 1.37 2008/01/24 01:53:19 tom_henderson Exp $ (UCB)
 */

#ifndef ns_mac_h
#define ns_mac_h

#include <assert.h>
#include "bi-connector.h"
#include "packet.h"
#include "ip.h"
#include "route.h"
#include "ll.h"
#include "phy.h"
#include "marshall.h"
#include "channel.h"

class Channel;

#define ZERO	0.00000

/*
 * Medium Access Control (MAC)
 */

#define EF_COLLISION 2		// collision error flag

/* ======================================================================
   Defines / Macros used by all MACs.
   ====================================================================== */

#define ETHER_ADDR(x)	(GET4BYTE(x))

#define MAC_HDR_LEN	64

#define MAC_BROADCAST	((u_int32_t) 0xffffffff)
#define BCAST_ADDR -1

#define ETHER_ADDR_LEN	6
#define ETHER_TYPE_LEN	2
#define ETHER_FCS_LEN	4

#define ETHERTYPE_IP	0x0800
#define ETHERTYPE_ARP	0x0806

enum MacState {
	MAC_IDLE	= 0x0000,
	MAC_POLLING	= 0x0001,
	MAC_RECV 	= 0x0010,
	MAC_SEND 	= 0x0100,
	MAC_RTS		= 0x0200,
	MAC_BCN		= 0x0300,
	MAC_CTS		= 0x0400,
	MAC_ACK		= 0x0800,
	MAC_COLL	= 0x1000,
	MAC_MGMT	= 0x1001
};

enum MacFrameType {
	MF_BEACON	= 0x0008, // beaconing
	MF_CONTROL	= 0x0010, // used as mask for control frame
	MF_SLOTS	= 0x001a, // announce slots open for contention
	MF_RTS		= 0x001b, // request to send
	MF_CTS		= 0x001c, // clear to send, grant
	MF_ACK		= 0x001d, // acknowledgement
	MF_CF_END	= 0x001e, // contention free period end
	MF_POLL		= 0x001f, // polling
	MF_DATA		= 0x0020, // also used as mask for data frame
	MF_DATA_ACK	= 0x0021  // ack for data frames
};

struct hdr_mac {
	MacFrameType ftype_;	// frame type
	int macSA_;		// source MAC address
	int macDA_;		// destination MAC address
	u_int16_t hdr_type_;     // mac_hdr type

	double txtime_;		// transmission time
	double sstime_;		// slot start time

	int padding_;

	inline void set(MacFrameType ft, int sa, int da=-1) {
		ftype_ = ft;
		macSA_ = sa;
		if (da != -1)  macDA_ = da;
	}
	inline MacFrameType& ftype() { return ftype_; }
	inline int& macSA() { return macSA_; }
	inline int& macDA() { return macDA_; }
	inline u_int16_t& hdr_type() {return hdr_type_; }

	inline double& txtime() { return txtime_; }
	inline double& sstime() { return sstime_; }

	// Header access methods
	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_mac* access(const Packet* p) {
		return (hdr_mac*) p->access(offset_);
	}
};

/* ===================================================================
   Objects that want to promiscously listen to the packets before
   address filtering must inherit from class Tap in order to plug into
   the tap
   =================================================================*/

class Tap {
public:
	virtual ~Tap () {}
	virtual void tap(const Packet *p) = 0;
	// tap is given all packets received by the host.
	// it must not alter or free the pkt.  If you want to frob it, copy it.
};


class MacHandlerResume : public Handler {
public:
	MacHandlerResume(Mac* m) : mac_(m) {}
	void handle(Event*);
protected:
	Mac* mac_;
};

class MacHandlerSend : public Handler {
public:
	MacHandlerSend(Mac* m) : mac_(m) {}
	void handle(Event*);
protected:
	Mac* mac_;
};


/* ==================================================================
   MAC data structure
   ================================================================*/

class Mac : public BiConnector {
public:
	Mac();
	virtual void recv(Packet* p, Handler* h);
	virtual void sendDown(Packet* p);
	virtual void sendUp(Packet *p);

	virtual void resume(Packet* p = 0);
	virtual void installTap(Tap *t) { tap_ = t; }
	
	inline double txtime(int bytes) {
		return (8. * bytes / bandwidth_);
	}
 	inline double txtime(Packet* p) {
		return 8. * (MAC_HDR_LEN + \
			     (HDR_CMN(p))->size()) / bandwidth_;
	}
	inline double bandwidth() const { return bandwidth_; }
	
	inline int addr() { return index_; }
	inline MacState state() { return state_; }
	inline MacState state(int m) { return state_ = (MacState) m; }
	
        //mac methods to set dst, src and hdt_type in pkt hdrs.
	// note: -1 is the broadcast mac addr.
	virtual inline int hdr_dst(char* hdr, int dst = -2) {
		struct hdr_mac *dh = (struct hdr_mac*) hdr;
		if(dst > -2)
			dh->macDA_ = dst;
		return dh->macDA();
	}
	virtual inline int hdr_src(char* hdr, int src = -2) {
		struct hdr_mac *dh = (struct hdr_mac*) hdr;
		if(src > -2)
			dh->macSA_ = src;
		return dh->macSA();
	}
	virtual inline int hdr_type(char *hdr, u_int16_t type = 0) {
		struct hdr_mac *dh = (struct hdr_mac*) hdr;
		if (type)
			dh->hdr_type_ = type;
		return dh->hdr_type();
	}

private:
        void mac_log(Packet *p) {
                logtarget_->recv(p, (Handler*) 0);
        }
        NsObject*       logtarget_;

protected:
	int command(int argc, const char*const* argv);
	virtual int initialized() { 
		return (netif_ && uptarget_ && downtarget_); 
	}
	int index_;		// MAC address
	double bandwidth_;      // channel bitrate
	double delay_;		// MAC overhead
	int abstract_;         //   MAC support for abstract LAN 
        
	Phy *netif_;            // network interface
        Tap *tap_;              // tap agent
	LL *ll_;             	// LL this MAC is connected to
	Channel *channel_;	// channel this MAC is connected to

	Handler* callback_;	// callback for end-of-transmission
	MacHandlerResume hRes_;	// resume handler
	MacHandlerSend hSend_;	// handle delay send due to busy channel
	Event intr_;

	/*
	 * Internal MAC State
	 */
	MacState state_;	// MAC's current state
	Packet *pktRx_;
	Packet *pktTx_;
};

#endif
