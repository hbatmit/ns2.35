
/*
 * lms-agent.cc
 * Copyright (C) 2001 by the University of Southern California
 * $Id: lms-agent.cc,v 1.3 2005/08/25 18:58:07 johnh Exp $
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
 * lms-agent.cc
 *
 * This implements the network element LMS agent, "Agent/LMS".
 *
 * Christos Papadopoulos. 
 * christos@isi.edu
 */

#include <assert.h>

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "random.h"
#include "lms.h"
#include "ip.h"

static int lms_agent_uid_ = 0;	// each LMS agent has a uid

class LmsAgent : public Agent {
 public:
	LmsAgent ();
	int	command (int argc, const char*const* argv);
	void	recv (Packet*, Handler*);
 protected:
	//inline int 	off_lms() { return off_lms_; };
	NsObject* 	iface2link (int iface);
	NsObject* 	pkt2agent (Packet *pkt);
	void 		send_upstream (Packet *p);
        void            send_downstream (Packet *p);

        inline void
	 send2replier (Packet *p)
		{
		hdr_ip* piph = HDR_IP(p);
		//hdr_cmn* c = HDR_CMN(p);
#ifdef LMS_DEBUG
printf("%s send2replier: Got piph->saddr: %d, downstream_addr: %d. Replier: %d\n", uname_, piph->saddr(), downstream_lms_.addr_, replier_ );
#endif
		if (replier_ != NULL)
			replier_->recv (p);
		else if ((downstream_lms_.addr_ != LMS_NOADDR) &&
			 (piph->saddr() != downstream_lms_.addr_) )
			send_downstream(p);
		else	send_upstream (p); 
		};

	char	uname_[16];	// agent's unique name
	int	lms_enabled_;	// is this agent enabled? Default is YES
	NsObject* replier_;	// agent's replier (link or receiver)
	int	replier_iface_;	// replier interface (-2 if replier is local)
	int	replier_cost_;	// replier cost
	int	replier_dist_;	// replier distance, in #of hops
	nsaddr_t upstream_lms_;	// upstream LMS entity
	ns_addr_t downstream_lms_;// needed for incr.deployment
    
	int	upstream_iface_;// upstream interface (-2 if source is local)
	int	have_rcvr_;	// flag the presence of a receiver on this node

	// Header offsets
	// int	off_ip_;
	// int	off_lms_;
};

//
// Declaration of Agent/LMS
//
static class LmsClass : public TclClass {
public:
	LmsClass() : TclClass("Agent/LMS") {}
	TclObject* create(int, const char*const*) {
		return (new LmsAgent());
	}
} class_lms_agent;


//
// Declaration of PacketHeader/Lms
//

int hdr_lms::offset_;

static class LmsHeaderClass : public PacketHeaderClass {
public:
        LmsHeaderClass() : PacketHeaderClass("PacketHeader/Lms",
					     sizeof(hdr_lms)) {
		bind_offset(&hdr_lms::offset_);
	}
} class_lmshdr;


//
// Initialization routine
//
LmsAgent::LmsAgent() : Agent(PT_LMS)
{
	sprintf (uname_, "lms%d", lms_agent_uid_++);
	replier_  = NULL;
	replier_iface_ = LMS_NOIFACE;
	replier_cost_  = LMS_INFINITY;
	replier_dist_  = 0;
	upstream_lms_  = LMS_NOADDR;
	downstream_lms_.addr_ = LMS_NOADDR;
	downstream_lms_.port_ = LMS_NOPORT;
	have_rcvr_ = 0;

	bind("lms_enabled_", &lms_enabled_);
	bind("packetSize_", &size_);
	// bind("off_ip_", &off_ip_);
	// bind("off_lms_", &off_lms_);
}

//
// Main LMS receive function
//
void LmsAgent::recv (Packet* pkt, Handler*)
{
    hdr_cmn* h = HDR_CMN(pkt);
    hdr_lms* lh = HDR_LMS(pkt);
    //    double now = Scheduler::instance().clock();
    
#ifdef LMS_DEBUG
int a1 = lh->from_; int a2 = lh->src_;
printf ("at %f %s received packet type %d from %d:%d src %d:%d group %x iface %d\n\n", now,uname_, lh->type(), a1>>8, a1&0xff, a2>>8, a2&0xff, lh->group_,h->iface());
#endif

	// if this agent is not enabled, simply pass on the packet
	if (!lms_enabled_)
		{
		target_->recv (pkt);
		return;
		}

	switch (lh->type())
	{
         case LMS_SRC_REFRESH:
		{
		// If we have a replier already, and the upstream adv cost
		// is less than our current cost and the upstream distance
		// is less than our current distance, then choose the
		// upstream link as the replier link.
		struct lms_ctl *c = (struct lms_ctl *)pkt->accessdata ();
		c->hop_cnt_++;
#ifdef LMS_DEBUG
printf ("%s LMS_SRC_REFRESH from iface %d, cost %d, ttl %d\n\n",
uname_, h->iface(), c->cost_, lh->ttl_);
#endif
                 assert (upstream_iface_ == h->iface());

                 if (replier_iface_ == h->iface())
			{
			if (replier_cost_ != c->cost_)
				replier_cost_ = c->cost_;
			if (replier_dist_ != c->hop_cnt_)
				replier_dist_ = c->hop_cnt_;
			}
                 else if (replier_cost_ >= c->cost_ && replier_dist_ > c->hop_cnt_)
			{
#ifdef LMS_DEBUG
printf ("%s chose upstream replier\n\n", uname_);
#endif
			replier_cost_ = c->cost_;
			replier_dist_ = c->hop_cnt_;
			replier_iface_ = h->iface();
			if (h->iface() < 0)
				{
				replier_ = pkt2agent (pkt);
				downstream_lms_.addr_ = LMS_NOADDR;
				downstream_lms_.port_ = LMS_NOPORT;
				}
			else	{
				replier_ = iface2link (h->iface());
				downstream_lms_.addr_ = LMS_NOADDR;
				downstream_lms_.port_ = LMS_NOPORT;
				}
			if (--lh->ttl_ > 0)
				target_->recv (pkt);
			else	Packet::free(pkt);
			return;
			}
                 break;
           }
           
         case LMS_REFRESH:
             {
                 struct lms_ctl *c = (struct lms_ctl *)pkt->accessdata ();
                 c->hop_cnt_++;
#ifdef LMS_DEBUG
printf("%s replier iface %d, h->iface %d, replier cost %d, c cost %d\n",
  uname_, replier_iface_, h->iface(), replier_cost_, c->cost_);
printf("replier_= %d, down_addr_= %d, down_port_ = %d\n\n",
  replier_, downstream_lms_.addr_, downstream_lms_.port_);
#endif     

                 // update from existing replier
                 // adjust cost and distance and send upstream
                 if (replier_iface_ == h->iface())
                 {
                     if (replier_cost_ != c->cost_)
                         replier_cost_ = c->cost_;
                     if (replier_dist_ != c->hop_cnt_)
                         replier_dist_ = c->hop_cnt_;
#ifdef LMS_DEBUG
printf ("%s REPLIER_UPDATE iface %d, cost %d, hops %d\n\n",
   uname_, replier_iface_, replier_cost_, replier_dist_);
#endif

    
                     if (h->iface() < 0)
                     {
                         replier_ = pkt2agent (pkt);
                         have_rcvr_ = 1;
                         downstream_lms_.addr_ = LMS_NOADDR;
                         downstream_lms_.port_ = LMS_NOPORT;
                     }
                     else
                     {	
                         // updates our entry for downstream_lms_ 
                         downstream_lms_ = c->downstream_lms_;
                         // places our own address in downstream_lms_
                         // before sending it upstream.
                         c->downstream_lms_.addr_ = addr();
                         c->downstream_lms_.port_ = port();
                         
                         replier_ = NULL;
                     }
                     send_upstream (pkt);
                 }

                 // found a better replier
                 else if (replier_cost_ > c->cost_)
                 {
                     replier_cost_ = c->cost_;
                     replier_dist_ = c->hop_cnt_;
                     replier_iface_ = h->iface();
                     
                     if (h->iface() < 0)
                     {
                         replier_ = pkt2agent (pkt);
                         have_rcvr_ = 1;
                         downstream_lms_.addr_ = LMS_NOADDR;
                         downstream_lms_.port_ = LMS_NOPORT;
                     }
                     else
                     {	
                         // updates our entry for downstream_lms_ 
                         downstream_lms_ = c->downstream_lms_;
                         // places our own address in downstream_lms_
                         // before sending it upstream.
                         c->downstream_lms_.addr_ = addr();
                         c->downstream_lms_.port_ = port();
                                
                         replier_ = NULL;                                    
                     }
#ifdef LMS_DEBUG
printf ("%s REPLIER iface %d, cost %d, hops %d\n\n",
  uname_, replier_iface_, replier_cost_, replier_dist_);
#endif
                     send_upstream (pkt);
                 }
                 else
                     Packet::free(pkt);
                 return;
             }

	 case LMS_REQ:
             if (replier_iface_ == h->iface())
                 send_upstream (pkt);
             else {
                 // fill turning point info, but only
                 // if no-one else has done so already.
                 if (lh->tp_addr_ == LMS_NOADDR)
                 	{
					lh->tp_addr_  = addr();
					lh->tp_port_  = port();
					lh->tp_iface_ = h->iface_;
                 	}
                 send2replier (pkt);
             	}
             break;

	 case LMS_DMCAST:
             {
#ifdef LMS_DEBUG
a2 = lh->src_;
printf ("LMS-DMCAST at agent %s, iface %d src %d:%d group %d\n\n",
  uname_, lh->tp_iface_, a2>>8, a2&0xff, lh->group_);
#endif
                 hdr_ip* iph = HDR_IP(pkt);
                 NsObject* tgt = iface2link (lh->tp_iface_);

                 if (tgt)
                 {
		   //packet_t t = h->ptype();
                     iph->saddr() = lh->src_;
                     iph->daddr() = lh->group_;
                     tgt->recv (pkt);
                 }
                 else	{
                     printf ("FATAL: %s no such iface %d\n", uname_, lh->tp_iface_);
                     abort ();
                 }
             }
             break;
             
         case LMS_LEAVE:
             {
                 
                 if (replier_iface_ == h->iface())
                 {
                     downstream_lms_.addr_ = LMS_NOADDR;
                     downstream_lms_.port_ = LMS_NOPORT;
                     
                     replier_ = NULL;
                     replier_cost_ = 1000000;
                     replier_iface_ = LMS_NOIFACE;
                     send_upstream (pkt);
                 }
                 else
                     Packet::free(pkt);
                 
                 return;
             }
             
         case LMS_SETUP:
             {
                 
                 if (h->iface() < 0)
                 {
                     // Source is attached - mark it as the replier
                     replier_ = pkt2agent (pkt);
                     replier_iface_ = h->iface();
                     downstream_lms_.addr_ = LMS_NOADDR;
                     downstream_lms_.port_ = LMS_NOPORT;
                     
                     replier_cost_ = 0;		// XXX
                     
#ifdef LMS_DEBUG
printf ("%s REPLIER iface %d, cost %d\n\n", uname_, replier_iface_, replier_cost_);
#endif

                 }
                 upstream_lms_ = lh->from_;
                 upstream_iface_ = h->iface();
                 
#ifdef LMS_DEBUG    
printf ("%s upstream %d\n\n", uname_, upstream_lms_);
#endif
 
                 lh->from_ = addr();
                 Tcl::instance().evalf("[%s set node_] agent %d", name(), port());
                 target_->recv (pkt);
                 return;
             }
             
         case LMS_SPM:
             {
                 if (upstream_lms_ < 0)
                 {
                     struct lms_spm *spm = (struct lms_spm *)pkt->accessdata ();
                     //nsaddr_t adr = spm->spm_path_;
#ifdef LMS_DEBUG    
printf ("%s LMS_SPM seqno %d, upstream %d:%d\n\n",
  uname_, spm->spm_seqno_, adr>>8, adr&0xff);
#endif

                      if (upstream_lms_ != spm->spm_path_)
                          upstream_lms_ = spm->spm_path_;

                      spm->spm_path_ = addr();
                 }
                 break;
             }
             
	 case LMS_LINKS:
             {
                 Tcl& tcl = Tcl::instance();
                 char wrk[64];
                 int  n1 = lh->from();
                 int  n2 = addr();
                 
                 if (n1 != n2 && !have_rcvr_)
                 {
                     sprintf (wrk, "lappend tree_links {%d %d}", n1, n2);
                     tcl.eval (wrk);
                 }
                 
                 lh->from_ = addr();
                 target_->recv (pkt);
                 return;
             }
         default:
             printf ("FATAL: %s uknown LMS packet type: %d\n", uname_, lh->type());
             abort ();
     }          
}

     

int LmsAgent::command (int argc, const char*const* argv)
{
  //Tcl& tcl = Tcl::instance();

	if (argc == 6) {
		if (strcmp(argv[1], "send-lms") == 0) {
			Packet* pkt = allocpkt();
			hdr_lms* ph = HDR_LMS(pkt);

			ph->type() = atoi (argv[2]);
			ph->from() = atoi(argv[3]);
			ph->src() = atoi(argv[4]);
			ph->group() = atoi(argv[5]);

			send(pkt, 0);
			return (TCL_OK);
		}
	}
        
	return (Agent::command(argc, argv));
}


/*
 * Given an interface number,
 * return its outgoing link
 */
NsObject* LmsAgent::iface2link (int iface)
{
	Tcl&	tcl = Tcl::instance();
	char	wrk[64];

	sprintf (wrk, "[%s set node_] ifaceGetOutLink %d", name (), iface);
	tcl.evalc (wrk);
	const char* result = tcl.result ();

#ifdef LMS_DEBUG
printf ("[iface2link] agent %s\n", result);
#endif

	NsObject* obj = (NsObject*)TclObject::lookup(result);
	return (obj);
}

/*
 * Given a packet, determine which agent sent it
 * Agent MUST be attached to this node
 */
NsObject* LmsAgent::pkt2agent (Packet *pkt)
{
	Tcl&		tcl = Tcl::instance();
	char		wrk[64];
	const char		*result;
	int		port;
	NsObject*	agent;
	hdr_ip*		ih = HDR_IP(pkt);
	//nsaddr_t	src = ih->saddr();

	port = ih->sport();

	sprintf (wrk, "[%s set node_] agent %d", name (), port);
	tcl.evalc (wrk);
	result = tcl.result ();

#ifdef LMS_DEBUG
printf ("[pkt2agent] port %d, agent %s\n", port, result);
#endif

	agent = (NsObject*)TclObject::lookup (result);
	return (agent);
}

void LmsAgent::send_upstream (Packet *p)
{
	if (upstream_lms_ < 0)
		{
		printf ("FATAL: %s upstream_lms_ not set!\n", uname_);
		abort ();
		}
	hdr_ip* ih = HDR_IP(p);
	hdr_lms* lh = HDR_LMS(p);

	lh->from_ = addr();
	ih->daddr() = upstream_lms_;
	target_->recv(p);
}


void LmsAgent::send_downstream (Packet *p)
{
    if (downstream_lms_.addr_ < 0)
    {
        printf ("FATAL: %s downstream_lms_ not set!\n", uname_);
        abort ();
    }
    hdr_ip* ih = HDR_IP(p);
    hdr_lms* lh = HDR_LMS(p);
    
    lh->from_ = addr();
    ih->daddr() = downstream_lms_.addr_;
    ih->dport() = downstream_lms_.port_;
    
    target_->recv(p);
}
