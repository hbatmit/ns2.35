/* -*- c++ -*-
   priqueue.cc
   
   A simple priority queue with a remove packet function
   $Id: dsr-priqueue.cc,v 1.2 2002/09/11 20:11:09 buchheim Exp $
   */

#include <object.h>
#include <packet.h>
#include <cmu-trace.h>
#include <dsr-priqueue.h>

#define PRIQUEUE_DEBUG 0

static class CMUPriQueueClass : public TclClass {
public:
  CMUPriQueueClass() : TclClass("CMUPriQueue") {}
  TclObject* create(int, const char*const*) {
    return (new CMUPriQueue);
  }
} class_CMUPriQueue;

/* ======================================================================
   Priority Queue Handler
   ====================================================================== */
void CMUPriQueueHandler::handle(Event*)
{
        qh_ifq->prq_resume();
}       

/* ======================================================================
   CMUPriQueue - public routines
   ====================================================================== */
CMUPriQueue::CMUPriQueue() : Connector(), prq_qh_(this)
{
	int i;

	for(i = 0; i < IFQ_MAX; i++) {
		prq_snd_[i].ifq_head = prq_snd_[i].ifq_tail = 0;
		prq_snd_[i].ifq_len = 0;
		prq_snd_[i].ifq_maxlen = IFQ_MAXLEN;
		prq_snd_[i].ifq_drops = 0;

		last_ifqlen_[i] = 0;
	}
	prq_logtarget_ = 0;		// no logging target by default
	prq_ipaddr_ = 0;
	prq_blocked_ = 0;

	bind("qlen_logthresh_", &qlen_logthresh_);
	bind("fw_logthresh_", &fw_logthresh_);
	assert(qlen_logthresh_ > 0 && fw_logthresh_ > 0);

	bzero(last_ifqlen_, sizeof(last_ifqlen_));
	stat_send_ = stat_recv_ = stat_blocked_ = 0;
}

int
CMUPriQueue::command(int argc, const char*const* argv)
{
	if (argc == 2 && strcasecmp(argv[1], "reset") == 0) {
		Terminate();
		//FALL-THROUGH to give parents a chance to reset
	} else if(argc == 3) {
		if(strcmp(argv[1], "logtarget") == 0) {
			prq_logtarget_ = (Trace*) TclObject::lookup(argv[2]);
			assert(prq_logtarget_);
			return (TCL_OK);
		}
		else if(strcmp(argv[1], "ipaddr") == 0) {
			prq_ipaddr_ = atoi(argv[2]);
			assert(prq_ipaddr_ > 0);
			return (TCL_OK);
		}
	}
	return Connector::command(argc, argv);
}

void
CMUPriQueue::log_stats()
{
	int q, s = 0, d = 0;

	assert(IFQ_MAX == 4);

	/*
	 * avoid logging useless information
	 */
	for(q = 0; q < IFQ_MAX; q++) {
		s += prq_snd_[q].ifq_len;
		d += (prq_snd_[q].ifq_len - last_ifqlen_[q]);
	}
	if(d < 0)
		d = -d;

	/*
	 * Only write an entry to the log file is the queue length
	 * or "delta change" over the last second meets the threshold
	 * level OR, if this node is generally busy (forwarding more
	 * than a certain number of packets per second).
	 */
	if(s >= qlen_logthresh_ || d >= qlen_logthresh_ ||
	   stat_recv_ > fw_logthresh_) {
		if (prq_logtarget_->pt_->tagged()) {
		   // using the new tagged trace format
		   trace("I "TIME_FORMAT" -prq:adr %d -prq:rx %d -prq:tx %d -prq:bl %d -prq:snd0 {%d %d} -prq:snd1 {%d %d} -prq:snd2 {%d %d} -prq:snd3 {%d %d}",
		      Scheduler::instance().clock(),
		      prq_ipaddr_,
		      stat_recv_, stat_send_, stat_blocked_,
		      prq_snd_[0].ifq_len, prq_snd_[0].ifq_len-last_ifqlen_[0],
		      prq_snd_[1].ifq_len, prq_snd_[1].ifq_len-last_ifqlen_[1],
		      prq_snd_[2].ifq_len, prq_snd_[2].ifq_len-last_ifqlen_[2],
		      prq_snd_[3].ifq_len, prq_snd_[3].ifq_len-last_ifqlen_[3]);
		} else {
		   trace("I %.9f _%d_ ifq rx %d tx %d bl %d [%d %d] [%d %d] [%d %d] [%d %d]",
		      Scheduler::instance().clock(),
		      prq_ipaddr_,
		      stat_recv_, stat_send_, stat_blocked_,
		      prq_snd_[0].ifq_len, prq_snd_[0].ifq_len-last_ifqlen_[0],
		      prq_snd_[1].ifq_len, prq_snd_[1].ifq_len-last_ifqlen_[1],
		      prq_snd_[2].ifq_len, prq_snd_[2].ifq_len-last_ifqlen_[2],
		      prq_snd_[3].ifq_len, prq_snd_[3].ifq_len-last_ifqlen_[3]);
		}
	}

	for(q = 0; q < IFQ_MAX; q++) {
		last_ifqlen_[q] = prq_snd_[q].ifq_len;
	}
	stat_send_ = stat_recv_ = stat_blocked_ = 0;
}

void
CMUPriQueue::trace(char* fmt, ...)
{
	va_list ap;
  
	assert(prq_logtarget_);

	va_start(ap, fmt);
	vsprintf(prq_logtarget_->pt_->buffer(), fmt, ap);
	prq_logtarget_->pt_->dump();
	va_end(ap);
}

void
CMUPriQueue::recv(Packet *p, Handler *)
{
#if PRIQUEUE_DEBUG > 0
	prq_validate();
#endif
	stat_recv_++;
	if(prq_blocked_ == 1) {
		stat_blocked_++;
	} else {
		assert(prq_length() == 0);
	}
	prq_enqueue(p);
#if PRIQUEUE_DEBUG > 0
	prq_validate();
#endif
}

void
CMUPriQueue::prq_resume()
{
	Packet *p;

	assert(prq_blocked_);
#if PRIQUEUE_DEBUG > 0
	prq_validate();
#endif
	p = prq_dequeue();
	if (p != 0) {
		stat_send_++;
                target_->recv(p, &prq_qh_);
        } else {
		prq_blocked_ = 0;
        }
}


/*
 * Called at the end of the simulation to purge the IFQ.
 */
void
CMUPriQueue::Terminate()
{
	Packet *p;
	while((p = prq_dequeue())) {
		drop(p, DROP_END_OF_SIMULATION);
	}
}

Packet*
CMUPriQueue::prq_get_nexthop(nsaddr_t id)
{
	int q;
	Packet *p, *pprev = 0;
	struct ifqueue *ifq;

#if PRIQUEUE_DEBUG > 0
	prq_validate();
#endif
	for(q = 0; q < IFQ_MAX; q++) {
		ifq = &prq_snd_[q];
		pprev = 0;
		for(p = ifq->ifq_head; p; p = p->next_) {
			struct hdr_cmn *ch = HDR_CMN(p);

			if(ch->next_hop() == id)
				break;
			pprev = p;
		}

	if(p) {
		if(p == ifq->ifq_head) {
			assert(pprev == 0);

			IF_DEQUEUE(ifq, p);
			/* don't increment drop counter */
#if PRIQUEUE_DEBUG > 0
			prq_validate();
#endif
			return p;
		} else {
			assert(pprev);
			pprev->next_ = p->next_;
	
			if(p == ifq->ifq_tail)
				ifq->ifq_tail = pprev;
			ifq->ifq_len--;

#if PRIQUEUE_DEBUG > 0
			prq_validate();
#endif
			p->next_ = 0;
			return p;
		}
	}
     }

	return (Packet*) 0;
}

int
CMUPriQueue::prq_isfull(Packet *p)
{
	int q = prq_assign_queue(p);
	struct ifqueue *ifq = &prq_snd_[q];

	if(IF_QFULL(ifq))
		return 1;
	else
		return 0;
}

int
CMUPriQueue::prq_length()
{
	int q, tlen = 0;

	for(q = 0; q < IFQ_MAX; q++) {
		tlen += prq_snd_[q].ifq_len;
	}

	return tlen;
}

/* ======================================================================
   CMUPriQueue - private routines
   ====================================================================== */
void
CMUPriQueue::prq_enqueue(Packet *p)
{
	int q = prq_assign_queue(p);
	struct ifqueue *ifq = &prq_snd_[q];

	if(IF_QFULL(ifq)) {
		IF_DROP(ifq);
		drop(p, DROP_IFQ_QFULL);
		return;
	}
	IF_ENQUEUE(ifq, p);

	/*
	 * Start queue if idle...
	 */
	if(prq_blocked_ == 0) {
		p = prq_dequeue();
		prq_blocked_ = 1;

		stat_send_++;
		target_->recv(p, &prq_qh_);
	}
}

Packet*
CMUPriQueue::prq_dequeue(void)
{
	Packet *p;
	int q;

	for(q = 0; q < IFQ_MAX; q++) {
		if(prq_snd_[q].ifq_len > 0) {
			IF_DEQUEUE(&prq_snd_[q], p);
			return p;
		} else {
			assert(prq_snd_[q].ifq_head == 0);
		}
	}
	return (Packet*) 0;
}

int
CMUPriQueue::prq_assign_queue(Packet *p)
{
        struct hdr_cmn *ch = HDR_CMN(p);

	switch(ch->ptype()) {
	case PT_AODV:
	case PT_DSR:
	case PT_IMEP:
	case PT_MESSAGE:	/* used by DSDV */
	case PT_TORA:
		return IFQ_RTPROTO;

	case PT_AUDIO:
	case PT_VIDEO:
		return IFQ_REALTIME;

	case PT_ACK:
		return IFQ_LOWDELAY;

	default:
		return IFQ_NORMAL;
	}
}

void
CMUPriQueue::prq_validate()
{
	int q, qlen;
	Packet *p;
	struct ifqueue *ifq;

	for(q = 0; q < IFQ_MAX; q++) {
		ifq = &prq_snd_[q];
		qlen = 0;
			
		if(ifq->ifq_head == 0) {
			assert(ifq->ifq_len == 0);
			assert(ifq->ifq_head == ifq->ifq_tail);
		} else {

			for(p = ifq->ifq_head; p; p = p->next_) {
				if(p->next_ == 0)
					assert(p == ifq->ifq_tail);
				qlen++;
			}

			assert(qlen == ifq->ifq_len);
			assert(qlen <= ifq->ifq_maxlen);
		}
	}
}
