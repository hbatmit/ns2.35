/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1991-1997 Regents of the University of California.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
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
 */
 
/* 8/02 Tom Kelly - Dynamic resizing of seen buffer */

#include "flags.h"
#include "ip.h"
#include "tcp-sink.h"
#include "hdr_qs.h"

static class TcpSinkClass : public TclClass {
public:
	TcpSinkClass() : TclClass("Agent/TCPSink") {}
	TclObject* create(int, const char*const*) {
		return (new TcpSink(new Acker));
	}
} class_tcpsink;

Acker::Acker() : next_(0), maxseen_(0), wndmask_(MWM), ecn_unacked_(0), 
	ts_to_echo_(0), last_ack_sent_(0)
{
	seen_ = new int[MWS];
	memset(seen_, 0, (sizeof(int) * (MWS)));
}

void Acker::reset() 
{
	next_ = 0;
	maxseen_ = 0;
	memset(seen_, 0, (sizeof(int) * (wndmask_ + 1)));
}	

// dynamically increase the seen buffer as needed
// size must be a factor of two for the wndmask_ to work...
void Acker::resize_buffers(int sz) { 
	int* new_seen = new int[sz];
	int new_wndmask = sz - 1;
	
	if(!new_seen){
		fprintf(stderr, "Unable to allocate buffer seen_[%i]\n", sz);
		exit(1);
	}
	
	memset(new_seen, 0, (sizeof(int) * (sz)));
	
	for(int i = next_; i <= maxseen_+1; i++){
		new_seen[i & new_wndmask] = seen_[i&wndmask_];
	}
	
	delete[] seen_;
	seen_ = new_seen;      
	wndmask_ = new_wndmask;
	return; 
}

void Acker::update_ts(int seqno, double ts, int rfc1323)
{
	// update timestamp if segment advances with ACK.
        // Code changed by Andrei Gurtov.
        if (rfc1323 && seqno == last_ack_sent_ + 1)
               ts_to_echo_ = ts;
        else if (ts >= ts_to_echo_ && seqno <= last_ack_sent_ + 1)
               //rfc1323-bis, update timestamps from duplicate segments
               ts_to_echo_ = ts;
}

// returns number of bytes that can be "delivered" to application
// also updates the receive window (i.e. next_, maxseen, and seen_ array)
int Acker::update(int seq, int numBytes)
{
	bool just_marked_as_seen = FALSE;
	is_dup_ = FALSE;
	// start by assuming the segment hasn't been received before
	if (numBytes <= 0)
		printf("Error, received TCP packet size <= 0\n");
	int numToDeliver = 0;
	while(seq + 1 - next_ >= wndmask_) {
		// next_ is next packet expected; wndmask_ is the maximum
		// window size minus 1; if somehow the seqno of the
		// packet is greater than the one we're expecting+wndmask_,
		// then resize the buffer.
		resize_buffers((wndmask_+1)*2);
	}

	if (seq > maxseen_) {
		// the packet is the highest one we've seen so far
		int i;
		for (i = maxseen_ + 1; i < seq; ++i)
			seen_[i & wndmask_] = 0;
		// we record the packets between the old maximum and
		// the new max as being "unseen" i.e. 0 bytes of each
		// packet have been received
		maxseen_ = seq;
		seen_[maxseen_ & wndmask_] = numBytes;
		// store how many bytes have been seen for this packet
		seen_[(maxseen_ + 1) & wndmask_] = 0;
		// clear the array entry for the packet immediately
		// after this one
		just_marked_as_seen = TRUE;
		// necessary so this packet isn't confused as being a duplicate
	}
	int next = next_;
	if (seq < next) {
		// Duplicate packet case 1: the packet is to the left edge of
		// the receive window; therefore we must have seen it
		// before
#ifdef DEBUGDSACK
		printf("%f\t Received duplicate packet %d\n",Scheduler::instance().clock(),seq);
#endif
		is_dup_ = TRUE;
	}

	if (seq >= next && seq <= maxseen_) {
		// next is the left edge of the recv window; maxseen_
		// is the right edge; execute this block if there are
		// missing packets in the recv window AND if current
		// packet falls within those gaps

		if (seen_[seq & wndmask_] && !just_marked_as_seen) {
		// Duplicate case 2: the segment has already been
		// recorded as being received (AND not because we just
		// marked it as such)
			is_dup_ = TRUE;
#ifdef DEBUGDSACK
			printf("%f\t Received duplicate packet %d\n",Scheduler::instance().clock(),seq);
#endif
		}
		seen_[seq & wndmask_] = numBytes;
		// record the packet as being seen
		while (seen_[next & wndmask_]) {
			// this loop first gets executed if seq==next;
			// i.e., this is the next packet in order that
			// we've been waiting for.  the loop sets how
			// many bytes we can now deliver to the
			// application, due to this packet arriving
			// (and the prior arrival of any segments
			// immediately to the right)

			numToDeliver += seen_[next & wndmask_];
			++next;
		}
		next_ = next;
		// store the new left edge of the window
	}
	return numToDeliver;
}

TcpSink::TcpSink(Acker* acker) : Agent(PT_ACK), acker_(acker), save_(NULL),
	lastreset_(0.0)
{
	bytes_ = 0; 
	bind("bytes_", &bytes_);

	/*
	 * maxSackBlocks_ does wierd tracing things.
	 * don't make it delay-bound yet.
	 */
#if defined(TCP_DELAY_BIND_ALL) && 0
#else /* ! TCP_DELAY_BIND_ALL */
	bind("maxSackBlocks_", &max_sack_blocks_); // used only by sack
#endif /* TCP_DELAY_BIND_ALL */
}

void
TcpSink::delay_bind_init_all()
{
        delay_bind_init_one("packetSize_");
        delay_bind_init_one("ts_echo_bugfix_");
	delay_bind_init_one("ts_echo_rfc1323_");
	delay_bind_init_one("bytes_"); // For throughput measurements in JOBS
        delay_bind_init_one("generateDSacks_"); // used only by sack
	delay_bind_init_one("qs_enabled_");
	delay_bind_init_one("RFC2581_immediate_ack_");
	delay_bind_init_one("SYN_immediate_ack_");
	delay_bind_init_one("ecn_syn_");
#if defined(TCP_DELAY_BIND_ALL) && 0
        delay_bind_init_one("maxSackBlocks_");
#endif /* TCP_DELAY_BIND_ALL */

	Agent::delay_bind_init_all();
}

int
TcpSink::delay_bind_dispatch(const char *varName, const char *localName, TclObject *tracer)
{
        if (delay_bind(varName, localName, "packetSize_", &size_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "ts_echo_bugfix_", &ts_echo_bugfix_, tracer)) return TCL_OK;
	if (delay_bind_bool(varName, localName, "ts_echo_rfc1323_", &ts_echo_rfc1323_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "generateDSacks_", &generate_dsacks_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "qs_enabled_", &qs_enabled_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "RFC2581_immediate_ack_", &RFC2581_immediate_ack_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "SYN_immediate_ack_", &SYN_immediate_ack_, tracer)) return TCL_OK;
	if (delay_bind_bool(varName, localName, "ecn_syn_", &ecn_syn_ ,tracer)) return TCL_OK;
#if defined(TCP_DELAY_BIND_ALL) && 0
        if (delay_bind(varName, localName, "maxSackBlocks_", &max_sack_blocks_, tracer)) return TCL_OK;
#endif /* TCP_DELAY_BIND_ALL */

        return Agent::delay_bind_dispatch(varName, localName, tracer);
}

void Acker::append_ack(hdr_cmn*, hdr_tcp*, int) const
{
}

void Acker::update_ecn_unacked(int value)
{
	ecn_unacked_ = value;
}

int TcpSink::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "reset") == 0) {
			reset();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "resize_buffers") == 0) {
			// no need for this as seen buffer set dynamically
			fprintf(stderr,"DEPRECIATED: resize_buffers\n");
			return (TCL_OK);
		}
	}

	return (Agent::command(argc, argv));
}

void TcpSink::reset() 
{
	acker_->reset();	
	save_ = NULL;
	lastreset_ = Scheduler::instance().clock(); /* W.N. - for detecting */
				/* packets from previous incarnations */
}

void TcpSink::ack(Packet* opkt)
{
	Packet* npkt = allocpkt();
	// opkt is the "old" packet that was received
	// npkt is the "new" packet being constructed (for the ACK)
	double now = Scheduler::instance().clock();

	hdr_tcp *otcp = hdr_tcp::access(opkt);
	hdr_ip *oiph = hdr_ip::access(opkt);
	hdr_tcp *ntcp = hdr_tcp::access(npkt);

	if (qs_enabled_) {
		// QuickStart code from Srikanth Sundarrajan.
		hdr_qs *oqsh = hdr_qs::access(opkt);
		hdr_qs *nqsh = hdr_qs::access(npkt);
	        if (otcp->seqno() == 0 && oqsh->flag() == QS_REQUEST) {
	                nqsh->flag() = QS_RESPONSE;
	                nqsh->ttl() = (oiph->ttl() - oqsh->ttl()) % 256;
	                nqsh->rate() = oqsh->rate(); 
	        }
	        else {
	                nqsh->flag() = QS_DISABLE;
	        }
	}


	// get the tcp headers
	ntcp->seqno() = acker_->Seqno();
	// get the cumulative sequence number to put in the ACK; this
	// is just the left edge of the receive window - 1
	ntcp->ts() = now;
	// timestamp the packet

	if (ts_echo_bugfix_)  /* TCP/IP Illustrated, Vol. 2, pg. 870 */
		ntcp->ts_echo() = acker_->ts_to_echo();
	else
		ntcp->ts_echo() = otcp->ts();
	// echo the original's time stamp

	hdr_ip* oip = hdr_ip::access(opkt);
	hdr_ip* nip = hdr_ip::access(npkt);
	// get the ip headers
	nip->flowid() = oip->flowid();
	// copy the flow id
	
	hdr_flags* of = hdr_flags::access(opkt);
	hdr_flags* nf = hdr_flags::access(npkt);
	hdr_flags* sf;
	if (save_ != NULL)
		sf = hdr_flags::access(save_);
	else 
		sf = 0;
		// Look at delayed packet being acked. 
	if ( (sf != 0 && sf->cong_action()) || of->cong_action() ) 
		// Sender has responsed to congestion. 
		acker_->update_ecn_unacked(0);
	if ( (sf != 0 && sf->ect() && sf->ce())  || 
			(of->ect() && of->ce()) )
		// New report of congestion.  
		acker_->update_ecn_unacked(1);
	if ( (sf != 0 && sf->ect()) || of->ect() )
		// Set EcnEcho bit.  
		nf->ecnecho() = acker_->ecn_unacked();
	if ((!of->ect() && of->ecnecho()) ||
		(sf != 0 && !sf->ect() && sf->ecnecho()) ) {
		 // This is the negotiation for ECN-capability.
		 // We are not checking for of->cong_action() also. 
		 // In this respect, this does not conform to the 
		 // specifications in the internet draft 
		nf->ecnecho() = 1;
		if (ecn_syn_) 
			nf->ect() = 1;
	}
	acker_->append_ack(hdr_cmn::access(npkt),
			   ntcp, otcp->seqno());
	add_to_ack(npkt);
	// the above function is used in TcpAsymSink

        // Andrei Gurtov
        acker_->last_ack_sent_ = ntcp->seqno();
        // printf("ACK %d ts %f\n", ntcp->seqno(), ntcp->ts_echo());
	
	send(npkt, 0);
	// send it
}

void TcpSink::add_to_ack(Packet*)
{
	return;
}


void TcpSink::recv(Packet* pkt, Handler*)
{
	int numToDeliver;
	int numBytes = hdr_cmn::access(pkt)->size();
	// number of bytes in the packet just received
	hdr_tcp *th = hdr_tcp::access(pkt);
	/* W.N. Check if packet is from previous incarnation */
	if (th->ts() < lastreset_) {
		// Remove packet and do nothing
		Packet::free(pkt);
		return;
	}
	acker_->update_ts(th->seqno(),th->ts(),ts_echo_rfc1323_);
	// update the timestamp to echo
	
      	numToDeliver = acker_->update(th->seqno(), numBytes);
	// update the recv window; figure out how many in-order-bytes
	// (if any) can be removed from the window and handed to the
	// application
	if (numToDeliver) {
		bytes_ += numToDeliver;
		recvBytes(numToDeliver);
	}
	// send any packets to the application
      	ack(pkt);
	// ACK the packet
	Packet::free(pkt);
	// remove it from the system
}

static class DelSinkClass : public TclClass {
public:
	DelSinkClass() : TclClass("Agent/TCPSink/DelAck") {}
	TclObject* create(int, const char*const*) {
		return (new DelAckSink(new Acker));
	}
} class_delsink;

DelAckSink::DelAckSink(Acker* acker) : TcpSink(acker), delay_timer_(this)
{
	bind_time("interval_", &interval_);
	// Deleted the line below, since this is bound in TcpSink.
	// bind("bytes_", &bytes_); // useby JOBS
}

void DelAckSink::reset() {
    if (delay_timer_.status() == TIMER_PENDING)
        delay_timer_.cancel();
    TcpSink::reset();
}

void DelAckSink::recv(Packet* pkt, Handler*)
{
	int numToDeliver;
	int numBytes = hdr_cmn::access(pkt)->size();
	hdr_tcp *th = hdr_tcp::access(pkt);
	/* W.N. Check if packet is from previous incarnation */
	if (th->ts() < lastreset_) {
		// Remove packet and do nothing
		Packet::free(pkt);
		return;
	}
	acker_->update_ts(th->seqno(),th->ts(),ts_echo_rfc1323_);
	numToDeliver = acker_->update(th->seqno(), numBytes);
	if (numToDeliver) {
                bytes_ += numToDeliver; // for JOBS
                recvBytes(numToDeliver);
        }
	
        // If there's no timer and the packet is in sequence, set a timer.
        // Otherwise, send the ack and update the timer.
        if (delay_timer_.status() != TIMER_PENDING &&
                                th->seqno() == acker_->Seqno()) {
                // There's no timer, so we can set one and choose
		// to delay this ack.
		// If we're following RFC2581 (section 4.2) exactly,
		// we should only delay the ACK if we're know we're
		// not doing recovery, i.e. not gap-filling.
		// Since this is a change to previous ns behaviour,
		// it's controlled by an optional bound flag.
		// discussed April 2000 in the ns-users list archives.
		if (RFC2581_immediate_ack_ && 
			(th->seqno() < acker_->Maxseen())) {
			// don't delay the ACK since
			// we're filling in a gap
		} else if (SYN_immediate_ack_ && (th->seqno() == 0)) {
			// don't delay the ACK since
			// we should respond to the connection-setup
			// SYN immediately
		} else {
			// delay the ACK and start the timer.
	                save_ = pkt;
        	        delay_timer_.resched(interval_);
                	return;
		}
        }
        // If there was a timer, turn it off.
	if (delay_timer_.status() == TIMER_PENDING) 
		delay_timer_.cancel();
	ack(pkt);
        if (save_ != NULL) {
                Packet::free(save_);
                save_ = NULL;
        }

	Packet::free(pkt);
}

void DelAckSink::timeout(int)
{
	// The timer expired so we ACK the last packet seen.
	if ( save_ != NULL ) {
		Packet* pkt = save_;
		ack(pkt);
		save_ = NULL;
		Packet::free(pkt);
	}
}

void DelayTimer::expire(Event* /*e*/) {
	a_->timeout(0);
}

/* "sack1-tcp-sink" is for Matt and Jamshid's implementation of sack. */

class SackStack {
protected:
	int size_;
	int cnt_;
	struct Sf_Entry {
		int left_;
		int right_;
	} *SFE_;
public:
	SackStack(int); 	// create a SackStack of size (int)
	~SackStack();
	int& head_right(int n = 0) { return SFE_[n].right_; }
	int& head_left(int n = 0) { return SFE_[n].left_; }
	int cnt() { return cnt_; }  	// how big is the stack
	void reset() {
		register int i;
		for (i = 0; i < cnt_; i++)
			SFE_[i].left_ = SFE_[i].right_ = -1;

		cnt_ = 0;
	}

	inline void push(int n = 0) {
 		if (cnt_ >= size_) cnt_ = size_ - 1;  // overflow check
		register int i;
		for (i = cnt_-1; i >= n; i--)
			SFE_[i+1] = SFE_[i];	// not efficient for big size
		cnt_++;
	}

	inline void pop(int n = 0) {
		register int i;
		for (i = n; i < cnt_-1; i++)
			SFE_[i] = SFE_[i+1];	// not efficient for big size
		SFE_[i].left_ = SFE_[i].right_ = -1;
		cnt_--;
	}
};

SackStack::SackStack(int sz)
{
	register int i;
	size_ = sz;
	SFE_ = new Sf_Entry[sz];
	for (i = 0; i < sz; i++)
		SFE_[i].left_ = SFE_[i].right_ = -1;
	cnt_ = 0;
}

SackStack::~SackStack()
{
	delete SFE_;
}

static class Sack1TcpSinkClass : public TclClass {
public:
        Sack1TcpSinkClass() : TclClass("Agent/TCPSink/Sack1") {}
	TclObject* create(int, const char*const*) {
		Sacker* sacker = new Sacker;
		TcpSink* sink = new TcpSink(sacker);
		sacker->configure(sink);
		return (sink);
        }
} class_sack1tcpsink;

static class Sack1DelAckTcpSinkClass : public TclClass {
public:
	Sack1DelAckTcpSinkClass() : TclClass("Agent/TCPSink/Sack1/DelAck") {}
	TclObject* create(int, const char*const*) {
		Sacker* sacker = new Sacker;
		TcpSink* sink = new DelAckSink(sacker);
		sacker->configure(sink);
		return (sink);
	}
} class_sack1delacktcpsink;

void Sacker::configure(TcpSink *sink)
{
	if (sink == NULL) {
		fprintf(stderr, "warning: Sacker::configure(): no TCP sink!\n");
		return;
	}

	TracedInt& nblocks = sink->max_sack_blocks_;
	if (int(nblocks) > NSA) {
		fprintf(stderr, "warning(Sacker::configure): TCP header limits number of SACK blocks to %d, not %d\n", NSA, int(nblocks));
		nblocks = NSA;
	}
	sf_ = new SackStack(int(nblocks));
	nblocks.tracer(this);
	base_nblocks_ = int(nblocks);
	dsacks_ = &(sink->generate_dsacks_);
}

void
Sacker::trace(TracedVar *v)
{
	// we come here if "nblocks" changed
	TracedInt* ti = (TracedInt*) v;

	if (int(*ti) > NSA) {
		fprintf(stderr, "warning(Sacker::trace): TCP header limits number of SACK blocks to %d, not %d\n", NSA, int(*ti));
		*ti = NSA;
	}

	int newval = int(*ti);
	delete sf_;
	sf_ = new SackStack(newval);
	base_nblocks_ = newval;
}

void Sacker::reset() 
{
	sf_->reset();
	Acker::reset();
}

Sacker::~Sacker()
{
	delete sf_;
}

void Sacker::append_ack(hdr_cmn* ch, hdr_tcp* h, int old_seqno) const
{
	// ch and h are the common and tcp headers of the Ack being constructed
	// old_seqno is the sequence # of the packet we just got
	
        int sack_index, i, sack_right, sack_left;
	int recent_sack_left, recent_sack_right;
          
	int seqno = Seqno();
	// the last in-order packet seen (i.e. the cumulative ACK # - 1)

        sack_index = 0;
	sack_left = sack_right = -1;
	// initialization; sack_index=0 and sack_{left,right}= -1

        if (old_seqno < 0) {
                printf("Error: invalid packet number %d\n", old_seqno);
        } else if (seqno >= maxseen_ && (sf_->cnt() != 0))
		sf_->reset();
	// if the Cumulative ACK seqno is at or beyond the right edge
	// of the window, and if the SackStack is not empty, reset it
	// (empty it)
	else if (( (seqno < maxseen_) || is_dup_ ) && (base_nblocks_ > 0)) {
		// Otherwise, if the received packet is to the left of
		// the right edge of the receive window (but not at
		// the right edge), OR if it is a duplicate, AND we
		// can have 1 or more Sack blocks, then execute the
		// following, which computes the most recent Sack
		// block

		if ((*dsacks_) && is_dup_) {
			// Record the DSACK Block
			h->sa_left(sack_index) = old_seqno;
			h->sa_right(sack_index) = old_seqno+1;
			// record the block
			sack_index++;
#ifdef DEBUGDSACK
			printf("%f\t Generating D-SACK for packet %d\n", Scheduler::instance().clock(),old_seqno);
#endif

			
		}

		//  Build FIRST (traditional) SACK block

		// If we already had a DSACK block due to a duplicate
		// packet, and if that duplicate packet is in the
		// receiver's window (i.e. the packet's sequence
		// number is > than the cumulative ACK) then the
		// following should find the SACK block it's a subset
		// of.  If it's <= cum ACK field then the following
		// shouldn't record a superset SACK block for it.

                if (sack_index >= base_nblocks_) {
			printf("Error: can't use DSACK with less than 2 SACK blocks\n");
		} else {
                sack_right=-1;

		// look rightward for first hole 
		// start at the current packet 
                for (i=old_seqno; i<=maxseen_; i++) {
			if (!seen_[i & wndmask_]) {
				sack_right=i;
				break;
			}
		}

		// if there's no hole set the right edge of the sack
		// to be the next expected packet
                if (sack_right == -1) {
			sack_right = maxseen_+1;
                }

		// if the current packet's seqno is smaller than the
		// left edge of the window, set the sack_left to 0
		if (old_seqno <= seqno) {
			sack_left = 0;
			// don't record/send the block
		} else {
			// look leftward from right edge for first hole 
	                for (i = sack_right-1; i > seqno; i--) {
				if (!seen_[i & wndmask_]) {
					sack_left = i+1;
					break;
				}
	                }
			h->sa_left(sack_index) = sack_left;
			h->sa_right(sack_index) = sack_right;
			
			// printf("pkt_seqno: %i cuml_seqno: %i sa_idx: %i sa_left: %i sa_right: %i\n" ,old_seqno, seqno, sack_index, sack_left, sack_right);
			// record the block
			sack_index++;
		}

		recent_sack_left = sack_left;
		recent_sack_right = sack_right;

		// first sack block is built, check the others 
		// make sure that if max_sack_blocks has been made
		// large from tcl we don't over-run the stuff we
		// allocated in Sacker::Sacker()
		int k = 0;
                while (sack_index < base_nblocks_) {

			sack_left = sf_->head_left(k);
			sack_right = sf_->head_right(k);

			// no more history 
			if (sack_left < 0 || sack_right < 0 ||
				sack_right > maxseen_ + 1)
				break;

			// newest ack "covers up" this one 

			if (recent_sack_left <= sack_left &&
			    recent_sack_right >= sack_right) {
				sf_->pop(k);
				continue;
			}

			h->sa_left(sack_index) = sack_left;
			h->sa_right(sack_index) = sack_right;
			
			// printf("pkt_seqno: %i cuml_seqno: %i sa_idx: %i sa_left: %i sa_right: %i\n" ,old_seqno, seqno, sack_index, sack_left, sack_right);
			
			// store the old sack (i.e. move it down one)
			sack_index++;
			k++;
                }


		if (old_seqno > seqno) {
		 	/* put most recent block onto stack */
			sf_->push();
			// this just moves things down 1 from the
			// beginning, but it doesn't push any values
			// on the stack
			sf_->head_left() = recent_sack_left;
			sf_->head_right() = recent_sack_right;
			// this part stores the left/right values at
			// the top of the stack (slot 0)
		}

		} // this '}' is for the DSACK base_nblocks_ >= test;
		  // (didn't feel like re-indenting all the code and 
		  // causing a large diff)
		
        }
	h->sa_length() = sack_index;
	// set the Length of the sack stack in the header
	ch->size() += sack_index * 8;
	// change the size of the common header to account for the
	// Sack strings (2 4-byte words for each element)
}
