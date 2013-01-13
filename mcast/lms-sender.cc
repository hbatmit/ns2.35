
/*
 * lms-sender.cc
 * Copyright (C) 2001 by the University of Southern California
 * $Id: lms-sender.cc,v 1.6 2010/03/08 05:54:52 tom_henderson Exp $
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
 * lms-sender.cc
 *
 * This implements the Sending LMS agent, "Agent/LMS/Sender".
 *
 * Christos Papadopoulos. 
 * christos@isi.edu
 */

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "ip.h"
#include "rtp.h"
#include "config.h"
#include "random.h"
#include "trafgen.h"
#include "lms.h"

static int snd_uid_ = 0;

class LmsSender;

class LmsSender: public Agent {
 public:
	LmsSender();
	LmsSender *next_;
	int  command (int argc, const char*const* argv);
	void recv (Packet* pkt, Handler*);

 protected:
	void handle_lms_pkt (Packet* pkt);
	virtual void sendmsg(int nbytes, const char *flags = 0);
	void send_lms_pkt (int, int);
	void send_dmcast (hdr_lms* lh, int seqno, int fid);
	int  add_req (Packet *rq);
	void send_spm ();
	void solicit_naks ();
	void print_stats ();
        void print_all_stats (int drops);
    
	char	uname_[16];	// unique name in the form r#
	int	spm_seqno_;	// spm sequence number
	int	lms_cost_;	// for sender this is usually zero
	int	lms_ttl_;	// solicit nacks this many hops away
	int     packetSize_;    // remember pkt size for retransmissions
	int	lmsPacketSize_;	// packet size for lms packets
	Packet	*req_list_;	// request list to detect duplicate requests
	int	req_list_sz_;	// current size of above list

	// Stats
	int	req_rcvd_;	// number of requests received
	int	dup_reqs_;	// number of duplicate requests received
	int	dmcasts_sent_;	// number of dmcasts sent

 static int     max_dup_naks_;  // max num of duplicate naks    
    
	// int 	maxpkts_;
	int 	seqno_;		// XXX: RTP should handle this

	//LmsSenderTimer lms_sender_timer_;
	
	// int	off_lms_;
	// int	off_ip_;
	// int 	off_rtp_;
	// int 	off_cmn_;
};

static class LmsSenderClass : public TclClass {
public:
	LmsSenderClass() : TclClass("Agent/LMS/Sender") {}
	TclObject* create(int, const char*const*) {
		return (new LmsSender());
	}
} class_lms_sender;


// Obligatory definition of the class static
int LmsSender::max_dup_naks_ = 0;

LmsSender::LmsSender(): Agent(PT_LMS), seqno_(-1)
{
	sprintf (uname_, "sender%d", snd_uid_++);
	
	lms_cost_ = 0;
	spm_seqno_ = 0;
	req_list_ = NULL;
	req_rcvd_ = dup_reqs_ = max_dup_naks_ = 0;
	dmcasts_sent_ = 0;
	bind("lmsPacketSize_", &lmsPacketSize_);
	bind("packetSize_", &packetSize_);
	// bind("off_ip_", &off_ip_);
	// bind("off_lms_", &off_lms_);
	// bind("off_rtp_", &off_rtp_);
	// bind("off_cmn_", &off_cmn_);
	// bind("maxpkts_", &maxpkts_);
}

//
// Send a data packet
//
void LmsSender::sendmsg(int nbytes, const char* )
{
	Packet*   p = allocpkt ();
	hdr_ip* ih = HDR_IP(p);
	hdr_cmn* th = HDR_CMN(p);
	hdr_rtp* rh = HDR_RTP(p); // XXX

	bzero (HDR_LMS(p), sizeof (struct hdr_lms));
	packetSize_ = nbytes;	
	th->size_ = packetSize_;   
	rh->seqno() = ++seqno_;	// XXX

	ih->flowid() = 1; //for coloring in NAM

#ifdef LMS_DEBUG
packet_t t = th->ptype();
const char* nname = packet_info.name(t);
double now = Scheduler::instance().clock();
printf("SNDR: at %f %s sendmsg pkt %d type is %s, size is %d\n\n",
  now, uname_, seqno_, nname, th->size_);
#endif

	target_->recv(p);
}

//
// Send an LMS packet
// set LMS header and adjust size_
//
void LmsSender::send_lms_pkt (int type, int cost)
{
	Packet* p = allocpkt();

	(HDR_CMN(p))->ptype_ = PT_LMS;
	(HDR_IP(p))->flowid() = 7; //Coloring in NAM

	if (type != LMS_SETUP)
		(HDR_CMN(p))->size_ = lmsPacketSize_; 
		//(HDR_CMN(p))->size_ = sizeof(hdr_lms); 
	else 
		(HDR_CMN(p))->size_ = packetSize_; 
		//(HDR_CMN(p))->size_ = sizeof(hdr_lms); 

	hdr_lms* lh = HDR_LMS(p);
	lh->type_ = type;
	lh->cost_ = cost;
	lh->from_ = addr();
//	lh->from_ = port();
	lh->src_ = addr();
	lh->group_ = daddr();
	lh->tp_addr_  = LMS_NOADDR;
	lh->tp_port_ = -1;
	lh->tp_iface_ = LMS_NOIFACE;
	lh->lo_ = lh->hi_ = -1;
	lh->ts_ = Scheduler::instance().clock();

#ifdef LMS_DEBUG
printf ("SNDR: %s send_lms_pkt from %d src %d dst %x type %d cost %d size %d at %f\n\n",
  uname_, lh->from_, lh->src_, lh->group_, lh->type_, lh->cost_,
  (HDR_CMN(p))->size_, lh->ts_);
#endif 
	target_->recv(p);
}

//
// Receive a packet
//
void LmsSender::recv(Packet* pkt, Handler*)
{
	hdr_cmn* h = HDR_CMN(pkt);

	if (h->ptype_ == PT_LMS)
		handle_lms_pkt (pkt);
	else	{
		printf ("ERROR: %s received non LMS pkt type %d\n", uname_, h->ptype_);
		Packet::free(pkt);
		}
}

/*
 * Handle LMS packets
 */
void LmsSender::handle_lms_pkt (Packet* pkt)
{
	int st = 0;

	hdr_lms* lh = HDR_LMS(pkt);
	//hdr_ip* iph = HDR_IP(pkt);
#ifdef LMS_DEBUG
int a1, a2;
#endif
	switch (lh->type())
		{
		case LMS_REQ:
			req_rcvd_++;
			if (lh->src_ != addr())
				{
				printf ("ERROR: %s REQ with wrong source addr %d:%d\n",
					uname_, lh->src_>>8, lh->src_&0xff);
				abort ();
				}
#ifdef LMS_DEBUG
a1 = lh->from(); a2 = lh->src();
printf ("SNDR: %s got LMS_REQ from %d:%d src %d:%d group 0x%x\n\n",
  uname_, a1>>8, a1&0xff, a2>>8, a2&0xff, lh->group());
printf ("SNDR: TP: (%d, %d)\n\n", lh->tp_addr_, lh->tp_iface_);
#endif
			if ((st = add_req (pkt)) != 0)
				for (int i = lh->lo_; i <= lh->hi_; i++)
					send_dmcast (lh, i, 3);
			else dup_reqs_++;

			break;
		case LMS_REFRESH:
			break;
		default:
			printf ("***ERROR: %s Unexpected LMS packet type: %d\n\n", uname_, lh->type());
			abort ();
		}
	if (!st)
		Packet::free(pkt);
}

int LmsSender::command(int argc, const char*const* argv)
{
    if (argc == 2) {
        if (strcmp(argv[1], "lms-setup") == 0) {
            send_lms_pkt (LMS_SETUP, packetSize_);	// XXX
            return (TCL_OK);
        }
        if (strcmp(argv[1], "gather-links") == 0) {
            send_lms_pkt (LMS_LINKS, 0);
            return (TCL_OK);
        }
        if (strcmp(argv[1], "print-stats") == 0) {
            print_stats ();
            return (TCL_OK);
        }
    }
    if (argc == 3)
    {
        if (strcmp(argv[1], "solicit-naks") == 0) {
            lms_ttl_ = atoi (argv[2]);
            solicit_naks ();
            return (TCL_OK);
        }
        if (strcmp(argv[1], "set-replier-cost") == 0)
        {
            lms_cost_  = atoi (argv[2]);
            send_lms_pkt (LMS_REFRESH, lms_cost_);
            return (TCL_OK);
        }
        if (strcmp(argv[1], "print-all-stats") == 0) {
            print_all_stats ( atoi(argv[2]) );
            return (TCL_OK);
        }         
        if (strcmp(argv[1], "send") == 0) {
            sendmsg( atoi (argv[2]), 0);
            return (TCL_OK);
        }
    }
    return (Agent::command(argc, argv));
}

/*
 * Send a LMS_DMCAST in response to LMS_REQ packet
 */
void LmsSender::send_dmcast (hdr_lms* lh, int seqno, int fid)
{
	Packet		*p = allocpkt();
	hdr_rtp		*rh = HDR_RTP(p);
	hdr_ip		*piph = HDR_IP(p);
	hdr_cmn		*ch = HDR_CMN(p);
	hdr_lms		*plh = HDR_LMS(p);

	dmcasts_sent_++;
	rh->seqno() = seqno;
	piph->daddr() = lh->tp_addr_;
	piph->dport() = lh->tp_port_;
	ch->size_= packetSize_;
	piph->flowid() = fid;		// Green for repair; to color packets in NAM

	plh->type_ = LMS_DMCAST;
	plh->from_ = addr();
//	plh->from_ = port();
	plh->src_ = addr();
	plh->group_ = daddr();
	plh->tp_addr_ = lh->tp_addr_;
	plh->tp_port_ = lh->tp_port_;
	plh->tp_iface_ = lh->tp_iface_;
        //const char* nname = packet_info.name(t);
	
	target_->recv(p);
}

// Add a new request to the request list.
// Return 1 if the request was added,
// 0 if this was a duplicate request and was not added.
// Requests are added to the head of the list
// and the list size is limited to 10 (XXX: should have time limit instead)
//
int LmsSender::add_req (Packet *rq)
{
	hdr_lms	*lh = HDR_LMS(rq);
        Packet  *p = req_list_;
        int     i = 0;

        if (!p)
        {
            req_list_ = rq;
            rq->next_ = 0;
            req_list_sz_ = 1;
            return 1;
        }
        while (p)
        {
            if (i++ > 10)
                break;
            hdr_lms *plh = HDR_LMS(p);
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
   printf ("SNDR: %s got %d dup reqs, max is %d\n", uname_, nh->dup_cnt_, max_dup_naks_);
#endif                
                
                if( nh->dup_cnt_ > max_dup_naks_)
                    max_dup_naks_ = nh->dup_cnt_;
                                
                return 0;                
            }
            
            p = p->next_;
        }
        if (i > 10 && p && p->next_)
        {
            Packet::free (p->next_);
            p->next_ = 0;
            req_list_sz_--;
        }
        rq->next_ = req_list_;
        req_list_ = rq;
        req_list_sz_++;
        return 1;
}


void LmsSender::send_spm ()
{
        Packet* p = allocpkt (sizeof (struct lms_spm));
        (HDR_CMN(p))->ptype_ = PT_LMS;
		(HDR_CMN(p))->size_ = sizeof(struct lms_spm) + sizeof(hdr_lms);
        (HDR_IP(p))->flowid() = 7;

        hdr_lms* lh = HDR_LMS(p);
        
        lh->type_ = LMS_SPM;

        struct lms_spm *spm = (struct lms_spm *)p->accessdata();
        spm->spm_seqno_ = spm_seqno_++;
        spm->spm_path_ = addr();
        spm->spm_ts_ = Scheduler::instance().clock();

        target_->recv(p);
}

void LmsSender::solicit_naks ()
{
	Packet* p = allocpkt(sizeof (struct lms_ctl));
	struct lms_ctl  *ctl = (struct lms_ctl*)p->accessdata ();
	(HDR_CMN(p))->size_ = sizeof(struct lms_ctl) + sizeof(hdr_lms); 
	(HDR_CMN(p))->ptype_ = PT_LMS;
    (HDR_IP(p))->flowid() = 7;

	hdr_lms* lh = HDR_LMS(p);
	lh->type_ = LMS_SRC_REFRESH;
	lh->ttl_  = lms_ttl_;
	lh->from_ = addr();
	lh->src_  = addr();
	lh->group_ = daddr();

	ctl->cost_ = lms_cost_;
	ctl->hop_cnt_ = 0;

	target_->recv(p);
}

void LmsSender::print_stats ()
{
        printf ("%s:\n", uname_);
        printf ("\tPackets sent:\t\t%d\n", seqno_);
        printf ("\tRequests received:\t%d\n", req_rcvd_);
        printf ("\tDuplicate Requests:\t%d\n", dup_reqs_);
        printf ("\tDMCASTs sent:\t\t%d\n", dmcasts_sent_);
        printf ("\n");
}

void LmsSender::print_all_stats (int drops)
{
    if (drops) 
        printf ("\t%.5f\t  %d", float(dup_reqs_)/float(drops), max_dup_naks_);
    else 
        printf ("\t0.0\t  0");
}
