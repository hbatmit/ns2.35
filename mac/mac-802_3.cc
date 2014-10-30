/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * mac-802_3.cc
 * $Id: mac-802_3.cc,v 1.16 2002/06/14 23:15:03 yuri Exp $
 */
#include <packet.h>
#include <random.h>
#include <arp.h>
#include <ll.h> 
#include <mac-802_3.h>

//#define MAC_DEBUG

#ifndef MAC_DEBUG
#define FPRINTF(s, f, t, index, func) do {} while (0)
#else
   static double xtime= 0.0;
#  define FPRINTF(s, f, t, index, func) \
         do { fprintf(s, f, t, index, func); xtime= t; } while (0)
#endif //MAC_DEBUG

#define PRNT_MAC_FUNCS(mac) \
	FPRINTF(stderr, "%.15f : %d : %s\n", \
		Scheduler::instance().clock(), (mac)->index_, __PRETTY_FUNCTION__)

inline void MacHandler::cancel() {
	PRNT_MAC_FUNCS(mac);
	Scheduler& s = Scheduler::instance();
	assert(busy_);
	s.cancel(&intr);
	// No need to free the event intr since it's statically allocated.
	busy_ = 0;
}

inline void Mac8023HandlerSend::cancel() {
	PRNT_MAC_FUNCS(mac);
	assert(busy_);
	Scheduler &s= Scheduler::instance();
	s.cancel(&intr);
	busy_= 0;
	p_= 0;
}

inline void MacHandlerRecv::cancel() {
	PRNT_MAC_FUNCS(mac);
	Scheduler& s = Scheduler::instance();
	assert(busy_ && p_);
	s.cancel(&intr);
	busy_ = 0;
	Packet::free(p_);
	p_= 0;
}

inline void MacHandlerRetx::cancel() {
	PRNT_MAC_FUNCS(mac);
	Scheduler& s = Scheduler::instance();
	assert(busy_ && p_);
	s.cancel(&intr);
}

inline void MacHandlerIFS::cancel() {
	PRNT_MAC_FUNCS(mac);
	//fprintf (stderr, "cancelled dtime= %.15f\n", intr.time_- Scheduler::instance().clock());
	MacHandler::cancel();
}
static class Mac802_3Class : public TclClass {
public:
	Mac802_3Class() : TclClass("Mac/802_3") {}
	TclObject* create(int, const char*const*) {
		return (new Mac802_3);
	}
} class_mac802_3;

void Mac8023HandlerSend::handle(Event*) {
	PRNT_MAC_FUNCS(mac);
	assert(p_);
	/* Transmission completed successfully */
	busy_ = 0;
	p_= 0;
	mac->mhRetx_.free();
	mac->mhRetx_.reset();
	mac->mhIFS_.schedule(mac->netif_->txtime(int(IEEE_8023_IFS_BITS/8.0)));
}

void Mac8023HandlerSend::schedule(const Packet *p, double t) {
	PRNT_MAC_FUNCS(mac);
	Scheduler& s = Scheduler::instance();
	assert(!busy_);
	s.schedule(this, &intr, t);
	busy_ = 1;
	p_= p;
}

void MacHandlerRecv::handle(Event* ) {
	/* Reception Successful */
	PRNT_MAC_FUNCS(mac);
	busy_ = 0;
	mac->recv_complete(p_);
	p_= 0;
}

void MacHandlerRecv::schedule(Packet *p, double t) {
	PRNT_MAC_FUNCS(mac);
	Scheduler& s = Scheduler::instance();
	assert(p && !busy_);
	s.schedule(this, &intr, t);
	busy_ = 1;
	p_ = p;
}

bool MacHandlerRetx::schedule(double delta) {
	PRNT_MAC_FUNCS(mac);
	Scheduler& s = Scheduler::instance();
	assert(p_ && !busy_);
	int k, r;
	if(try_ < IEEE_8023_ALIMIT) {
		k = min(try_, IEEE_8023_BLIMIT);
		r = Random::integer(1 << k);
		s.schedule(this, &intr, r * mac->netif_->txtime((int)(IEEE_8023_SLOT_BITS/8.0)) + delta);
		busy_ = 1;
		return true;
	}
	return false;
}

void MacHandlerRetx::handle(Event *) {
	PRNT_MAC_FUNCS(mac);
	assert(p_);
	busy_= 0;
	++try_;
	mac->transmit(p_);
}

inline void MacHandlerIFS::schedule(double t) {
	PRNT_MAC_FUNCS(mac);
	assert(!busy_);
	Scheduler &s= Scheduler::instance();
	s.schedule(this, &intr, t);
	busy_= 1;
}
inline void MacHandlerIFS::handle(Event*) { 
	PRNT_MAC_FUNCS(mac);
	busy_= 0; 
	mac->resume(); 
}


Mac802_3::Mac802_3() : trace_(0), 
	mhRecv_(this), mhRetx_(this), mhIFS_(this), mhSend_(this) {
        // Bind mac trace variable 
        bind_bool("trace_",&trace_);
}

void Mac802_3::sendUp(Packet *p, Handler *) {
	PRNT_MAC_FUNCS(this);
	/* just received the 1st bit of a packet */
	if (state_ != MAC_IDLE && mhIFS_.busy()) {
#define EPS 1.0e-15 /* can be considered controller clock resolution */
		if (mhIFS_.expire() - Scheduler::instance().clock() > EPS) {
			// This can happen when 3 nodes TX:
			// 1 was TX, then IFS, then TX again;
			// 2,3 were in RX (from 1), then IFS, then TX, then collision with 1 and IFS
			// while 3 in IFS it RX from 2 and vice versa.
			// We ignore it and let the ifs timer take care of things.
		        Packet::free(p);
			return;
		} else {
			// This means that mhIFS_ is about to expire now. We assume that IFS is over 
			// and resume, because we want to give this guy a chance - otherwise the guy 
			// who TX before will TX again (fairness); if we have anything to
			// TX, this forces a collision.
			mhIFS_.cancel();
			resume();
		}
#undef EPS
	} 
	if(state_ == MAC_IDLE) {
		state_ = MAC_RECV;
		assert(!mhRecv_.busy());
		/* the last bit will arrive in txtime seconds */
		mhRecv_.schedule(p, netif_->txtime(p));
	} else {
		collision(p); //received packet while sending or receiving
	}
}

void Mac802_3::sendDown(Packet *p, Handler *h) {
	PRNT_MAC_FUNCS(this);
	assert(initialized());
	assert(h);
	assert(netif_->txtime(IEEE_8023_MINFRAME) > 
	       2*netif_->channel()->maxdelay()); /* max prop. delay is limited by specs: 
						    about 25us for 10Mbps
						    and   2.5us for 100Mbps
						 */

	int size= (HDR_CMN(p)->size() += ETHER_HDR_LEN); //XXX also preamble?
	hdr_mac *mh= HDR_MAC(p);
	mh->padding_= 0;
	if (size > IEEE_8023_MAXFRAME) {
		static bool warnedMAX= false;
		if (!warnedMAX) {
			fprintf(stderr, "Mac802_3: frame is too big: %d\n", size);
			warnedMAX= true;
		}
	} else if (size < IEEE_8023_MINFRAME) {
		// pad it to fit MINFRAME
		mh->padding_= IEEE_8023_MINFRAME - size;
		HDR_CMN(p)->size() += mh->padding_;
	}
	callback_ = h;
	mhRetx_.packet(p); //packet's buffered by mhRetx in case of retransmissions

	transmit(p);
}


void Mac802_3::transmit(Packet *p) {
	PRNT_MAC_FUNCS(this);
	assert(callback_);
	if(mhSend_.packet()) {
		fprintf(stderr, "index: %d\n", index_);
		fprintf(stderr, "Retx Timer: %d\n", mhRetx_.busy());
		fprintf(stderr, "IFS  Timer: %d\n", mhIFS_.busy());
		fprintf(stderr, "Recv Timer: %d\n", mhRecv_.busy());
		fprintf(stderr, "Send Timer: %d\n", mhSend_.busy());
		exit(1);
	}

	/* Perform carrier sense  - if we were sending before, never mind state_ */
	if (mhIFS_.busy() || (state_ != MAC_IDLE)) {
		/* we'll try again when IDLE. It'll happen either when
                   reception completes, or if collision.  Either way,
                   we call resume() */
		return;
	}

	double txtime = netif_->txtime(p);
	/* Schedule transmission of the packet's last bit */
	mhSend_.schedule(p, txtime);

	// pass the packet to the PHY: need to send a copy, 
	// because there may be collision and it may be freed
	Packet *newp = p->copy();
	HDR_CMN(newp)->direction()= hdr_cmn::DOWN; //down
	
	downtarget_->recv(newp);

	state_= MAC_SEND;
}

void Mac802_3::collision(Packet *p) {

	PRNT_MAC_FUNCS(this);

	if (mhIFS_.busy()) mhIFS_.cancel();

	double ifstime= netif_->txtime(int((IEEE_8023_JAMSIZE+IEEE_8023_IFS_BITS)/8.0)); //jam time + ifs
	mhIFS_.schedule(ifstime);

	switch(state_) {
	case MAC_SEND:
	  // If mac trace feature is on generate a collision trace for this packet. 
	        if (trace_)
	           drop(p);
	        else
	           Packet::free(p);
		if (mhSend_.busy()) mhSend_.cancel();
		if (!mhRetx_.busy()) {
			/* schedule retransmissions */
			if (!mhRetx_.schedule(ifstime)) {
				p= mhRetx_.packet();
				hdr_cmn *th = hdr_cmn::access(p);
				HDR_CMN(p)->size() -= (ETHER_HDR_LEN + HDR_MAC(p)->padding_);
				fprintf(stderr,"BEB limit exceeded:Dropping packet %d\n",th->uid());
                                fflush(stderr);
				drop(p); // drop if backed off far enough
				mhRetx_.reset();
			}
		}
		break;
	case MAC_RECV:
	        Packet::free(p);
		// more than 2 packets collisions possible
		if (mhRecv_.busy()) mhRecv_.cancel();
		break;
	default:
		assert("SHOULD NEVER HAPPEN" == 0);
	}
}

void Mac802_3::recv_complete(Packet *p) {
	PRNT_MAC_FUNCS(this);
	assert(!mhRecv_.busy());
	assert(!mhSend_.busy());
	hdr_cmn *ch= HDR_CMN(p);
	/* Address Filtering */
	hdr_mac *mh= HDR_MAC(p);
	int dst= mh->macDA();

	if ((dst != BCAST_ADDR) && (dst != index_)) {
		Packet::free(p);
		goto done;
	}
	/* Strip off the mac header and padding if any */
	ch->size() -= (ETHER_HDR_LEN + mh->padding_);

	/* xxx FEC here */
	if( ch->error() ) {
                fprintf(stderr,"\nChecksum error\nDropping packet");
                fflush(stderr);
		// drop(p);
                Packet::free(p);
		goto done;
	}


	/* we could schedule an event to account for mac-delay */
	
	if ((p->ref_count() > 0) /* so the channel is using ref-copying */
	    && (dst == BCAST_ADDR)) {
		/* make a real copy only if broadcasting, otherwise
		 * all nodes except the receiver are going to free
		 * this packet 
		 */
		uptarget_->recv(p->copy(), (Handler*) 0);
		Packet::free(p); // this will decrement ref counter
	} else {
		uptarget_->recv(p, (Handler*) 0);
	}

 done:
	mhIFS_.schedule(netif_->txtime(int(IEEE_8023_IFS_BITS/8.0)));// wait for one IFS, then resume
}

/* we call resume() in these cases:
   - successful transmission
   - whole packet's received
   - collision and backoffLimit's exceeded
   - collision while receiving */
void Mac802_3::resume() {
	PRNT_MAC_FUNCS(this);
	assert(!mhRecv_.busy());
	assert(!mhSend_.busy());
	assert(!mhIFS_.busy());

	state_= MAC_IDLE;

	if (mhRetx_.packet()) {
		if (!mhRetx_.busy()) {
			// we're not backing off and not sensing carrier right now: send
			transmit(mhRetx_.packet());
		}
	} else {
		if (callback_ && !mhRetx_.busy()) {
			//WARNING: calling callback_->handle may change the value of callback_
			Handler* h= callback_; 
			callback_= 0;
			h->handle(0);
		}
	}
}

