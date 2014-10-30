
/*
 * lms-receiver.cc
 * Copyright (C) 2001 by the University of Southern California
 * $Id: lms-receiver.cc,v 1.5 2010/03/08 05:54:52 tom_henderson Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */

/*
 * Light-Weight Multicast Services (LMS), Reliable Multicast
 *
 * lms-receiver.cc
 *
 * This implements the Receiving LMS agent, "Agent/LMS/Receiver".
 *
 * Christos Papadopoulos. 
 * christos@isi.edu
 */

#include <math.h>
#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "ip.h"
#include "rtp.h"
#include "lms.h"
#include "rcvbuf.h"

#define	LMS_NAK_TIMEOUT	0
#define	LMS_RDL_TIMEOUT	1
#define	LMS_RFR_TIMEOUT	2

#define	LMS_RQL_MAX	10	// requests to remember
#define	LMS_RDL_MAX	10	// rdata to remember

#define	LMS_LOSS_SMPL_IVL	0	// sample loss every LMS_LOSS_SMPL_IVL packets
								// a value of 0 disables adaptive repliers
#define	LMS_COST_THR		1	// change in packet loss required to trigger LMS_REFRESH

static int rcv_uid_ = 0;


class LmsReceiver;
class LmsNak;

//
// Lms Nak Timer
// When this timer expires, the Nak is sent again
//
class LmsNakTimer : public TimerHandler {
public:
        LmsNakTimer(LmsNak *a) : TimerHandler() { a_ = a; }
        void expire(Event *e);
protected:
        LmsNak *a_;
};

//
// LMS RECEIVER
//
class LmsReceiver: public Agent {
 public:
	LmsReceiver();
	LmsReceiver* next_;
	int command (int argc, const char*const* argv);
	void recv (Packet* pkt, Handler*);
	virtual void timeout (int type, int lo, int hi);

 protected:
	void handle_lms_pkt (Packet* pkt);
	void send_refresh ();
	void send_dmcast (hdr_lms* lh, int seqno, int fid);
	int  add_req (Packet *rq);
	void send_nak (int sqn, int lo, int hi);
	void create_nak (int lo, int hi);
	void delete_nak (LmsNak *n);
	LmsNak* find_nak (int i);
	void add_rdl (int rseq, double clock);
	int  exists_rdl (int rseq);
	void print_stats ();
	void print_all_stats (int drops);

	char		uname_[8];	// unique name in the form r#
	RcvBuffer*	rcvbuf_;	// the receive buffer: see rcvbuf.{h,cc}
	int		dataSize_;	// data packet size
	double		rtt_;		// RTT to source
	nsaddr_t	lms_src_;	// address of sender
	nsaddr_t	lms_group_;	// group address

	// LMS related params
	nsaddr_t	upstream_lms_;	// address of upstream LMS element
	int		lms_cost_;	// our current replier cost
	Packet*		rql_;		// request list - used to detect duplicate requests
	int		rql_sz_;	// current request list size
	struct lms_rdl*	rdl_;		// rdata list - used to avoid re-retransmission
	int		rdl_sz_;	// current rdata list size
	LmsNak*		nak_;		// list of currently outstanding naks

	// Loss Sampling
	int		ls_last_rq_;	// # of requests at last loss sampling
	int		ls_last_seq_;	// data seq num at last loss sampling
	int		ls_smpl_ivl_;	// loss sampling interval (in packets)
	int		ls_cntdown_;	// how many packets until next loss sample

	// Receiver Stats
	int		req_sent_;	// number of unique requests sent
	int		req_rcvd_;	// number of requests received
	int		dup_reqs_;	// number of duplicate requests received
 static int             max_dup_naks_;  // max num of duplicate naks accros RXs
    
	// Header offsets
	// int		off_rtp_;
	// int		off_lms_;
	// int		off_ip_;
};

//
// A NAK
//
class LmsNak {
 public:
	LmsNak (LmsReceiver *a, int lo, int hi);
	LmsNak	*next_;
	void	timeout (int type) {
			a_->timeout (type, lo_, hi_); }
	LmsNakTimer	nak_timer_;
	int		lo_;
	int		hi_;
	int		seqn_;
	double		nak_timeout_;
 private:
	LmsReceiver	*a_;
	int		fid_;
};

LmsNak::LmsNak(LmsReceiver *a, int lo, int hi): nak_timer_(this), lo_(lo), hi_(hi), a_(a)
{
	seqn_ = 0;
}

void LmsNakTimer::expire(Event *) {
        a_->timeout(LMS_NAK_TIMEOUT);
}

LmsReceiver	*all_lms_receivers = 0;

static class LmsReceiverClass : public TclClass {
public:
	LmsReceiverClass() : TclClass("Agent/LMS/Receiver") {}
	TclObject* create(int, const char*const*) {
		return (new LmsReceiver());
	}
} class_lms_receiver;



// Obligatory definition of the class static
int LmsReceiver::max_dup_naks_ = 0;

LmsReceiver::LmsReceiver(): Agent(PT_LMS)
{
	sprintf (uname_, "rcv%d", rcv_uid_++);
	lms_cost_ = 0;
	lms_src_ = lms_group_ = LMS_NOADDR;
	rtt_ = 0.0;
	rcvbuf_ = new RcvBuffer;
	req_sent_ = req_rcvd_ = dup_reqs_ = max_dup_naks_ = 0;
	upstream_lms_ =  LMS_NOADDR;
	dataSize_ = 0;
	ls_smpl_ivl_ = LMS_LOSS_SMPL_IVL;
	ls_cntdown_ = ls_smpl_ivl_;
	ls_last_rq_ = ls_last_seq_ = 0;
	nak_ = NULL;
	rql_ = NULL;
	rdl_ = NULL;
	rdl_sz_ = 0;

	this->next_ = all_lms_receivers;
	all_lms_receivers = this;

	bind("lmsPacketSize_", &size_);

	// bind("off_rtp_", &off_rtp_);
	// bind("off_ip_", &off_ip_);
	// bind("off_lms_", &off_lms_);
}

//
// Send request upstream
//
void LmsReceiver::send_nak (int sqn, int lo, int hi)
{
	Packet* p = allocpkt(sizeof (struct lms_nak));
	struct lms_nak	*n = (struct lms_nak*)p->accessdata ();

	hdr_ip* iph = HDR_IP(p);
	hdr_cmn* ch = HDR_CMN(p);  

	ch->size_ = sizeof(struct lms_nak) + sizeof(hdr_lms); 
	iph->daddr() = upstream_lms_;
	iph->flowid() = 8;
#ifdef LMS_DEBUG
printf("at %f %s send nak to upstream lms %d, size is %d\n\n",
  now,uname_, upstream_lms_, (HDR_CMN(p))->size_);
#endif
	hdr_lms* lh = HDR_LMS(p);
	lh->type_   = LMS_REQ;
	lh->from_   = addr();
	lh->src_    = lms_src_;
	lh->group_  = lms_group_;

	lh->tp_addr_ = LMS_NOADDR;
	lh->tp_port_ = -1;
	lh->tp_iface_ = LMS_NOIFACE;
	lh->lo_ = n->nak_lo_ = lo;
	lh->hi_ = n->nak_hi_ = hi;
	n->nak_seqn_ = sqn;
        n->dup_cnt_ = 0;
        
	target_->recv(p);
}

//
// Send a replier refresh upstream.
//
void LmsReceiver::send_refresh ()
{
	Packet* p = allocpkt (sizeof (struct lms_ctl));
	struct lms_ctl	*ctl = (struct lms_ctl*)p->accessdata ();

	hdr_ip* iph = HDR_IP(p);
	hdr_cmn *ch = HDR_CMN(p);  

	ch->size_ = sizeof(struct lms_ctl) + sizeof(hdr_lms); 
	iph->daddr() = upstream_lms_;
	iph->flowid() = 7;		// mark refresh packets black for nam
#ifdef LMS_DEBUG
printf("%s send refresh packet, size is %d\n\n",
  uname_, (HDR_CMN(p))->size_);
#endif
	hdr_lms* lh = HDR_LMS(p);
	lh->type_   = LMS_REFRESH;
	lh->from_   = addr();
	lh->src_    = lms_src_;
	lh->group_  = lms_group_;

	ctl->cost_ = lms_cost_;
	ctl->hop_cnt_ = 0;
        ctl->downstream_lms_.addr_ = addr();
        ctl->downstream_lms_.port_ = port();
        
	target_->recv(p);
}

//
// Receive a packet
//
void LmsReceiver::recv (Packet* pkt, Handler*)
{
	hdr_cmn* h = HDR_CMN(pkt);

	// handle LMS packet
	if (h->ptype_ == PT_LMS)
		{
		handle_lms_pkt (pkt);
		return;
		}

	// DATA packet
	double		clock = Scheduler::instance().clock();
	hdr_rtp*	rh = HDR_RTP(pkt);
	int		rseq = rh->seqno();

	dataSize_ = h->size_;  

#ifdef LMS_DEBUG
printf ("%s got data pkt %d\n", uname_, rseq);
#endif
	// GAP: send retransmission request
	if (rcvbuf_->nextpkt_ <  rseq)
		{
		int lo = rcvbuf_->nextpkt_;
		int hi = rseq - 1;
		req_sent_++;
#ifdef LMS_DEBUG
printf ("%s Sending REQ, lo %d, hi %d\n", uname_, lo, hi);
#endif
		create_nak (lo, hi);
		send_nak (0, lo, hi);
		}
	// RETRANSMISSION
	else if (rcvbuf_->nextpkt_ >  rseq &&
		 !rcvbuf_->exists_pkt (rseq))
		{
		add_rdl (rseq, clock);	// remember rdata

		LmsNak *nak = find_nak (rseq);
		if (nak->lo_ !=  nak->hi_)
			{
			if (rseq == nak->lo_)
				create_nak (rseq+1, nak->hi_);
			else if (rseq == nak->hi_)
				create_nak (nak->lo_, rseq-1);
			else	{
				create_nak (nak->lo_, rseq-1);
				create_nak (rseq+1, nak->hi_);
				}
			}
		delete_nak (nak);
		}

	rcvbuf_->add_pkt (rseq, clock);

	// The LMS replier cost is sampled and refreshed every
	// LMS_LOSS_SMPL_IVL packets - data or retransmnissions.
	// New cost is the number of lost packets during LMS_LOSS_SMPL_IVL.
	// We update cost only if loss >= LMS_COST_THR.
	// The previous value is forgotten, making cost refresh very responsive.
	//
	// (XXX: We need a better algorithm here.
	if (ls_smpl_ivl_ && --ls_cntdown_ == 0)
		{
		int newcost = req_sent_ - ls_last_rq_;
		assert (newcost >= 0);
		if (abs (newcost - lms_cost_) >= LMS_COST_THR)
			{
			lms_cost_ = newcost;
			if (newcost > 0)
				ls_last_rq_ = req_sent_;
#ifdef LMS_DEBUG
printf ("%s NEW LMS_COST %d\n", uname_, lms_cost_);
#endif
			}
		send_refresh ();
		ls_cntdown_ = ls_smpl_ivl_;
		}

	Packet::free(pkt);
}

/*
 * Handle LMS packets
 */
void LmsReceiver::handle_lms_pkt (Packet* pkt)
{
	hdr_lms* lh = HDR_LMS(pkt);
	hdr_ip* iph = HDR_IP(pkt);
	int	st = 0;
#ifdef LMS_DEBUG
int a1, a2;
#endif
	switch (lh->type())
	{
		case LMS_REQ:
		{
			struct lms_nak *n = (struct lms_nak *)pkt->accessdata ();
			req_rcvd_++;
#ifdef LMS_DEBUG
a1 = lh->from(); a2 = lh->src();
printf ("%s got LMS_REQ from %d:%d src %d:%d group 0x%x ",
  uname_, a1>>8, a1&0xff, a2>>8, a2&0xff, lh->group());
printf ("TP: (%d:%d, %d)\n\n", lh->tp_addr_>>8, lh->tp_addr_&0xff, lh->tp_iface_);
#endif
			if ((st = add_req (pkt)) != 0)
				{
					for (int i = lh->lo_; i <= lh->hi_; i++)
						if (!(n->nak_seqn_ == 0 && exists_rdl (i)) &&
							(rcvbuf_->exists_pkt (i)))
							send_dmcast (lh, i, 3);
				}
			else	dup_reqs_++;

			break;
		}
		case LMS_SETUP:
                        {
			lms_src_ = iph->saddr();
			lms_group_ = iph->daddr();
			rtt_ = 2.0*(Scheduler::instance().clock() - lh->ts_);
			upstream_lms_ = lh->from_;
			dataSize_ = lh->cost_;		// XXX

			Tcl& tcl = Tcl::instance();
			char wrk[64];
			sprintf (wrk, "set_max_rtt %f", rtt_);
			tcl.eval (wrk); 

#ifdef LMS_DEBUG
a1 = lh->from(); a2 = lh->src();
//printf("%d %d, %ld %ld\n\n ", a1, a2, iph->saddr(), iph->daddr());
printf("%s the upstream_lms is %d\n\n", uname_, upstream_lms_);
printf ("%s LMS_SETUP from %d:%d src %d:%d group 0x%x RTT %f dataSize %d\n\n",
  uname_, a1>>8, a1&0xff, a2>>8, a2&0xff, lh->group(), rtt_, dataSize_);
#endif
                        send_refresh ();
			break;
			}
		case LMS_SPM:
			{
			struct lms_spm *spm = (struct lms_spm *)pkt->accessdata ();
#ifdef LMS_DEBUG    
printf ("%s LMS_SPM seqno %d, upstream %d:%d\n\n",
  uname_, spm->spm_seqno_, adr>>8, adr&0xff);
#endif
			if (upstream_lms_ != spm->spm_path_)
				upstream_lms_ = spm->spm_path_;
			}
			break;
		case LMS_LINKS:
		case LMS_SRC_REFRESH:
			break;
		default:
			printf ("ERROR: %s Unexpected LMS packet type: %d\n\n", uname_, lh->type());
			abort ();
        }
	if (!st)
		Packet::free(pkt);
}

int LmsReceiver::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "clear") == 0) {
			return (TCL_OK);
		}
                if (strcmp(argv[1], "print-stats") == 0) {
                        print_stats ();
                        return (TCL_OK);
                }
	}
	if (argc == 3)
		{
                if (strcmp(argv[1], "print-all-stats") == 0) {
                        print_all_stats (atoi (argv[2]));
                        return (TCL_OK);
                }
		if (strcmp(argv[1], "set-replier-cost") == 0)
			{
			lms_cost_  = atoi (argv[2]);
                        send_refresh ();
			return (TCL_OK);
			}
		if (strcmp(argv[1], "loss-smpl-ivl") == 0)
			{
			ls_cntdown_ = ls_smpl_ivl_ = atoi (argv[2]);
			return (TCL_OK);
			}
		}
	return (Agent::command(argc, argv));
}

/*
 * Send a LMS_DMCAST in response to LMS_REQ packet
 */
void LmsReceiver::send_dmcast (hdr_lms* lh, int seqno, int fid)
{
	Packet	*p = allocpkt();
	hdr_cmn *ch = HDR_CMN(p);
	hdr_rtp	*rh = HDR_RTP(p);
	hdr_ip	*piph = HDR_IP(p);
#ifdef LMS_DEBUG
printf ("%s Sending DMCAST %d to %d:%d, size is %d\n",
  uname_, seqno, lh->tp_addr_>>8, lh->tp_addr_&0xff, ch->size_);
#endif

	hdr_lms *plh = HDR_LMS(p);
	ch->ptype_ = PT_CBR;
	ch->size() = dataSize_; 

	rh->seqno() = seqno;

	piph->daddr() = lh->tp_addr_;
	piph->dport() = lh->tp_port_;
	piph->flowid() = fid;		// to color packets in NAM

	plh->type_ = LMS_DMCAST;
	plh->from_ = addr();
	plh->src_ = lms_src_;
	plh->group_ = lms_group_;
	plh->tp_addr_ = lh->tp_addr_;
	plh->tp_port_ = lh->tp_port_;
	plh->tp_iface_ = lh->tp_iface_;

	target_->recv(p);
}

// Add a new request to rql_
// Return 1 if the request was added,
// 0 if this was a duplicate request and was not added.
// Requests are added to the head of the list
// and the list size is limited to LMS_RQL_MAX
// (XXX: should have timer to expire old entries)
//
int LmsReceiver::add_req (Packet *rq)
{
	hdr_lms	*lh = HDR_LMS(rq);
        Packet  *p = rql_;
        int     i = 0;

        if (!p)
        {
            rql_ = rq;
            rq->next_ = 0;
            rql_sz_ = 1;
            return 1;
        }

#ifdef LMS_DEBUG
printf("%s REQ: \tlo=%d \thi=%d \ttp_addr=%d \ttp_if=%d\n",
  uname_, lh->lo_, lh->hi_, lh->tp_addr_, lh->tp_iface_);
#endif

      while (p)
        {
            if (++i > LMS_RQL_MAX)
                break;
            
            hdr_lms *plh = HDR_LMS(p);

#ifdef LMS_DEBUG
printf("%s DB: \tlo=%d \thi=%d \ttp_addr=%d \ttp_if=%d\n",
  uname_, plh->lo_, plh->hi_, plh->tp_addr_, plh->tp_iface_);
#endif            
            

            if ((plh->lo_ == lh->lo_) &&
                (plh->hi_ == lh->hi_) &&
                (plh->tp_addr_ == lh->tp_addr_) &&
                (plh->tp_iface_ == lh->tp_iface_)) 
            {
                struct lms_nak *nh = (struct lms_nak *)p->accessdata();
                // increments the dup_cnt_ for this NAK, and updates
                // max_dup_naks_ if appropriate
                ++nh->dup_cnt_;

#ifdef LMS_DEBUG
   printf ("%s got %d dup reqs, max is %d\n", uname_, nh->dup_cnt_, max_dup_naks_);
#endif                
                
                if( nh->dup_cnt_ > max_dup_naks_)
                    max_dup_naks_ = nh->dup_cnt_;
                                
                return 0;
            }
            
            p = p->next_;
        }
        
        if (i > LMS_RQL_MAX && p && p->next_)
        {
            Packet::free (p->next_);
            p->next_ = 0;
            rql_sz_--;
        }

        rq->next_ = rql_;
        rql_ = rq;
        rql_sz_++;
        return 1;
}

void LmsReceiver::print_all_stats (int drops)
{
	LmsReceiver	*r = all_lms_receivers;
	int		nrcvs = 0; 
	int		cnt1 = 0; 
	int		cnt2 = 0; 
	int		dup_replies = 0;
	double          avg_norm_latency = 0.0;
	double		min = 1000000.0, max = 0.0;
        float           aveTotalDupReqs = 0.0;
        
	while (r)
        {
            ++nrcvs;

            // first, we'll add up all dupReqs, and after the loop we'll
            // div. by nrcvrs
            aveTotalDupReqs += float(r->dup_reqs_);

            if (r->rcvbuf_->duplicates_)
            {
                cnt1++;
                dup_replies += r->rcvbuf_->duplicates_;
            }
            if (r->rcvbuf_->pkts_recovered_)
            {
                cnt2++;
                double avl = r->rcvbuf_->delay_sum_/(double)r->rcvbuf_->pkts_recovered_;
                avg_norm_latency += avl / r->rtt_;
                
                double m = r->rcvbuf_->min_delay_/r->rtt_;
                double x = r->rcvbuf_->max_delay_/r->rtt_;
                if (m < min)
                    min = m;
                if (x > max)
                    max = x;
            }
            r = r->next_;
        }
        if(drops) 
            aveTotalDupReqs = (float(aveTotalDupReqs) / float(nrcvs)) / float(drops);
        else 
            aveTotalDupReqs = 0.0;
        
	//
	// Print stats in the form: <avg duplicate replies> <avg recovery latency> <min> <max>
	//
	if (drops)
		{
                    printf ("\t%.5f\t  %d\t%.4f", aveTotalDupReqs, max_dup_naks_,
			    ((double)dup_replies/(double)(nrcvs*drops)) );
		}
	else	{
		printf ("\t0.0\t  0\t0.0");
		}

	//printf ("\t%d/%d/%.2f", nrcvs, cnt1, (cnt1)?((double)dup_replies/(double)cnt1):0.0);
	if (cnt2) 
		{
		printf ("\t%.2f\t%.2f\t%.2f\n", avg_norm_latency/(double)cnt2, min, max);
		}
	else
		{
		printf ("\t0.0\t0.0\t0.0\n");
		}
	printf ("\n");

	//printf ("Avg duplicates: %lf\n", (double)dup_replies / (double) cnt1);
	//printf ("Avg norm latency: %lf\n", avg_norm_latency / (double) cnt2);
}

void LmsReceiver::print_stats ()
{
        printf ("%s:\n", uname_);
        printf ("\tLast packet:\t\t%d\n", rcvbuf_->nextpkt_-1);
        printf ("\tMax packet:\t\t%d\n", rcvbuf_->maxpkt_);
        printf ("\tRequests sent:\t\t%d\n", req_sent_);
        printf ("\tRequests received:\t%d\n", req_rcvd_);
        if (rcvbuf_->pkts_recovered_)
                {
                printf ("\tPackets recovered:\t%d\n", rcvbuf_->pkts_recovered_);
                printf ("\tNormalized latency (min, max, avg):\t%f, %f, %f\n",
                        rcvbuf_->min_delay_/rtt_, rcvbuf_->max_delay_/rtt_,
                        (rcvbuf_->delay_sum_/(double)rcvbuf_->pkts_recovered_)/rtt_);
                }
        printf ("\tDuplicate Replies:\t%d\n", rcvbuf_->duplicates_);
        printf ("\tDuplicate Requests:\t%d\n", dup_reqs_);
        printf ("\tRtt:\t\t\t%f\n", rtt_);
        printf ("\n");
}

void LmsReceiver::timeout (int type, int lo, int hi)
{
	if (type == LMS_NAK_TIMEOUT)
		{
#ifdef LMS_DEBUG
double now= Scheduler::instance().clock();
printf ("at %f %s LMS_NAK_TIMEOUT for %d, %d\n", now, uname_, lo, hi);
#endif
		LmsNak *nak = find_nak (lo);
		send_nak (++nak->seqn_, lo, hi);
		nak->nak_timeout_ *= 2.0;			// exponential back-off
		nak->nak_timer_.resched (nak->nak_timeout_);
		}
}

void LmsReceiver::create_nak (int lo, int hi)
{
#ifdef LMS_DEBUG
printf ("%s Creating NAK %d, %d\n", uname_, lo, hi);
#endif
	LmsNak *nak = new LmsNak (this, lo, hi);
	nak->nak_timeout_ = rtt_ * 2.0;		// set initial timeout to twice rtt
	nak->nak_timer_.resched (nak->nak_timeout_);
	nak->next_ = nak_;
	nak_ = nak;
}

LmsNak *LmsReceiver::find_nak (int i)
{
	for (LmsNak *n = nak_; n; n = n->next_)
		if (n->lo_ <= i && i <= n->hi_)
			return n;
	// NOT REACHED
	printf("ERROR: %s nak %d not found\n", uname_, i);
	abort ();
	return 0;
}

void LmsReceiver::delete_nak (LmsNak *n)
{
	if (n == nak_)
		nak_ = nak_->next_;
	else	{
		LmsNak *cur, *prev = nak_;
		cur = nak_->next_;
		while (cur)
			{
			if (cur == n)
				{
				prev->next_ = cur->next_;
				break;
				}
			prev = cur;
			cur = cur->next_;
			}
		}
	n->nak_timer_.cancel();
#ifdef LMS_DEBUG
printf ("%s Deleting NAK %d, %d\n", uname_, n->lo_, n->hi_);
#endif
	delete n;
}

//
// Remember retransmissions. Uses a linked list
// whose size is limited to LMS_RDL_MAX.
// (Perhaps better to use an array and a timer to expire entries)
//
void LmsReceiver::add_rdl (int rseq, double clock)
{
	struct lms_rdl *r = new (struct lms_rdl);
	r->seqn_ = rseq;
	r->ts_ = clock;
	r->next_ = rdl_;
	rdl_ = r;

	if (++rdl_sz_ > LMS_RDL_MAX)
		{
		struct lms_rdl *r = rdl_;
		for (int i = 0; i < LMS_RDL_MAX; i++)
			r = r->next_;
		delete r->next_;
		r->next_ = NULL;
		--rdl_sz_;
		}
}

int LmsReceiver::exists_rdl (int rseq)
{
	struct lms_rdl *r = rdl_;
	for (; r; r = r->next_)
		if (r->seqn_ == rseq)
			return 1;
	return 0;
}
