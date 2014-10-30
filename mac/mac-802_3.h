#ifndef __mac_802_3_h__
#define __mac_802_3_h__

#include <assert.h>
#include "mac.h"

#define min(x, y)	((x) < (y) ? (x) : (y))
#define ETHER_HDR_LEN		((ETHER_ADDR_LEN << 1) + ETHER_TYPE_LEN)

/*
 10Mbps:
#define IEEE_8023_SLOT		0.000051200	// 512 bit times
#define	IEEE_8023_IFS		0.000009600	// 9.6us interframe spacing
 100Mbps:
#define IEEE_8023_SLOT                0.0000051200    //512 bit time
#define IEEE_8023_IFS                 0.0000009600    //0.96us interframe spacing
*/
#define IEEE_8023_SLOT_BITS             512.0 //when we divide these by the bandwidth
#define IEEE_8023_IFS_BITS               96.0 //we'll have what we want

#define IEEE_8023_ALIMIT	16		// attempt limit
#define IEEE_8023_BLIMIT	10		// backoff limit
#define IEEE_8023_JAMSIZE	32		// bits
#define IEEE_8023_MAXFRAME	1518		// bytes
#define IEEE_8023_MINFRAME	64		// bytes
#define INVALID_UID             -1              // Used for mac level traces
#define INVALID_TIME            -1

/* ======================================================================
   Handler Functions
   ====================================================================== */
class Mac802_3;

class MacHandler : public Handler {
public:
	MacHandler(Mac802_3* m) :  callback(0), mac(m), busy_(false) {}
	virtual void handle(Event *e) = 0;
	virtual inline void cancel();
	bool busy(void) { return busy_; }
	double expire(void) { return intr.time_; }
protected:
	Handler		*callback;
	Mac802_3	*mac;
	Event		intr;
	bool		busy_;
};


class Mac8023HandlerSend : public MacHandler {
 public:
	Mac8023HandlerSend(Mac802_3* m) : MacHandler(m), p_(0) {}
	void handle(Event*);
	void schedule(const Packet *p, double t);
	void cancel();
	const Packet *packet() const { return p_; }
 protected:
	const Packet *p_;
};

class MacHandlerRecv : public MacHandler {
public:
	MacHandlerRecv(Mac802_3* m) : MacHandler(m), p_(0) {}
	void handle(Event*);
	void schedule(Packet *p, double t);
	virtual void cancel();
private:
	Packet *p_;
};

class MacHandlerRetx : public MacHandler {
 public:
	MacHandlerRetx(Mac802_3* m) : MacHandler(m), p_(0), try_(1) {}
	void reset() {
		// before calling reset, you MUST free or drop p_
		if (busy_) cancel();
		try_= 1;
		p_= 0;
	}
	void handle(Event*);
	bool schedule(double delta=0.0);
	void cancel();
	void free() { 
		if (p_) {
			Packet::free(p_);
			p_= 0;
		}
	}
	Packet* packet() const { return p_; }
	void packet(Packet *p) { p_= p; }
private:
	Packet *p_;
	int try_;
};
		
class MacHandlerIFS : public MacHandler {
 public:
	MacHandlerIFS(Mac802_3* m) : MacHandler(m) {}
	void handle (Event*); 
	void schedule(double t);
	void cancel();
};


/* ======================================================================
   MAC data structure
   ====================================================================== */
class Mac802_3 : public Mac {
	friend class MacHandler;
	friend class MacHandlerRecv;
	friend class Mac8023HandlerSend;
	friend class MacHandlerRetx;
	friend class MacHandlerIFS;
 public:
	Mac802_3();

	void recv(Packet* p, Handler* h) {
		BiConnector::recv(p, h);
	}

 protected:
	void sendDown(Packet* p, Handler* h);
	void sendUp(Packet* p, Handler* h);
	/*
	inline int	hdr_dst(char* hdr, u_int32_t dst = -2);
	inline int	hdr_src(char* hdr, u_int32_t src = -2);
	inline int	hdr_type(char* hdr, u_int16_t type = 0);
	*/
	
	void		recv_complete(Packet *p);
	virtual void	resume();

	//protected:
	virtual void	transmit(Packet* p);
	int trace_;      // To turn on MAC level collision traces 
private:
	//void		send(Packet *p, Handler *h);
	void		collision(Packet *p);

	//int		pktTxcnt;

	MacHandlerRecv	mhRecv_;
	MacHandlerRetx  mhRetx_;
	MacHandlerIFS   mhIFS_;
	Mac8023HandlerSend	mhSend_;
	
};


#endif /* __mac_802_3_h__ */
