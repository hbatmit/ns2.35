/* -*- c++ -*-
   dsr-priqueue.h
   
   A simple priority queue with a remove packet function
   exported from cmu
   The differentce between this and the ns version is that the ns priority 
   queue maintains one physical queue while giving priority to higher 
   priority packets.
   This priqueue on the other hand maintains separate physical queues
   for different priority levels.
   $Id: dsr-priqueue.h,v 1.1 2002/07/19 02:34:10 haldar Exp $
   */
#ifndef __dsrpriqueue_h__
#define __dsrpriqueue_h__

#include <object.h>
#include <queue.h>
#include <drop-tail.h>
#include <packet.h>
#include "lib/bsd-list.h"
#include <cmu-trace.h>

/* ======================================================================
   The BSD Interface Queues
   ====================================================================== */
struct  ifqueue {
        Packet	*ifq_head;
        Packet	*ifq_tail;
        int     ifq_len;
        int     ifq_maxlen;
        int     ifq_drops;
};
#define IFQ_MAXLEN	50

#define IF_QFULL(ifq)           ((ifq)->ifq_len >= (ifq)->ifq_maxlen)
#define IF_DROP(ifq)            ((ifq)->ifq_drops++)
#define IF_ENQUEUE(ifq, p) {			                \
        (p)->next_ = 0;				                \
        if ((ifq)->ifq_tail == 0)		                \
                (ifq)->ifq_head = p;		                \
        else					                \
                (ifq)->ifq_tail->next_ = (p);	                \
        (ifq)->ifq_tail = (p);			                \
        (ifq)->ifq_len++;			                \
}
#define IF_DEQUEUE(ifq, p) {					\
        (p) = (ifq)->ifq_head;					\
        if (p) {						\
                if (((ifq)->ifq_head = (p)->next_) == 0)	\
                        (ifq)->ifq_tail = 0;			\
                (p)->next_ = 0;					\
                (ifq)->ifq_len--;				\
        }							\
}


/*
 * Control type and number of queues in PriQueue structure.
 */
#define IFQ_RTPROTO	0	/* Routing Protocol Traffic */
#define IFQ_REALTIME	1
#define IFQ_LOWDELAY	2
#define IFQ_NORMAL	3
#define IFQ_MAX		4

typedef int (*PacketFilter)(Packet *, void *);

class CMUPriQueue;

/* ======================================================================
   Handles callbacks for Priority Queues
   ====================================================================== */
class CMUPriQueueHandler : public Handler {
public:
  inline CMUPriQueueHandler(CMUPriQueue *ifq) : qh_ifq(ifq) {}
  void handle(Event*);
private: 
  CMUPriQueue *qh_ifq;
};

/* ======================================================================
   Priority Queues
   ====================================================================== */
class CMUPriQueue : public Connector {
  //friend class CMUPriQueueManager;
public:
  CMUPriQueue();

  int     command(int argc, const char*const* argv);

  /* called by upper layers to enque the packet */
  void    recv(Packet *p, Handler*);
  
  /* called by lower layers to get the next packet */
  void	prq_resume(void);
  
  void	Terminate(void);	/* called at end of simulation */
  
  Packet* prq_get_nexthop(nsaddr_t id);
  int	prq_isfull(Packet *p);
  int	prq_length(void);
  
private:
  int	prq_assign_queue(Packet *p);
  void	prq_enqueue(Packet *p);
  Packet*	prq_dequeue(void);
  void	prq_validate(void);
  
  struct ifqueue	prq_snd_[IFQ_MAX];
  nsaddr_t	prq_ipaddr_;	/* IP Address of this machine */
  Trace*		prq_logtarget_;	/* Used for logging */
  int		prq_blocked_;
  CMUPriQueueHandler	prq_qh_;
	
protected:
  void trace(char* fmt, ...);
  void log_stats(void);

  //TAILQ_ENTRY(CMUPriQueue) prq_list_;
  int qlen_logthresh_;	/* see run.tcl */
  int fw_logthresh_;	/* see run.tcl */
  
  int last_ifqlen_[IFQ_MAX];
  int stat_send_;		/* packets sent to lower layers */
  int stat_recv_;		/* packets received from upper layer */
  int stat_blocked_;	/* blocked on received */
};

#endif /* __priqueue_h__ */
