/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1999 Regents of the University of California.
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
 *      This product includes software developed by the MASH Research
 *      Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
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
 * Contributed by Tom Henderson, UCB Daedalus Research Group, June 1999
 * speedup hack from Lloyd Wood, 11 October 1999 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/satellite/sattrace.cc,v 1.17 2011/10/02 22:32:34 tom_henderson Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include "packet.h"
#include "ip.h"
#include "tcp.h"
#include "sctp.h"
#include "rtp.h"
#include "srm.h"
#include "flags.h"
#include "address.h"
#include "trace.h"
#include "sattrace.h"
#include "satposition.h"
#include "satnode.h"
#include "sat-hdlc.h"

class SatTraceClass : public TclClass {
public:
	SatTraceClass() : TclClass("Trace/Sat") { }
	TclObject* create(int argc, const char*const* argv) {
		if (argc >= 5) {
			return (new SatTrace(*argv[4]));
		}
		return 0;
	}
} sat_trace_class;

// XXX this should be moved from trace.cc to trace.h 
char* srm_names_[] = {
	SRM_NAMES
};

void SatTrace::format_hdlc(Packet *p, int offset)
{
	struct hdr_hdlc *hh = HDR_HDLC(p);
	struct I_frame *ifr = (struct I_frame *)&(hh->hdlc_fc_);
	struct S_frame *sf = (struct S_frame *)&(hh->hdlc_fc_);
	struct U_frame *uf = (struct U_frame *)&(hh->hdlc_fc_);
	
	switch(hh->fc_type_) {
	case HDLC_I_frame:
		if (pt_->tagged()) {
			sprintf(pt_->buffer() + offset,
				"-hdlc:sa %d -hdlc:da %d -hdlc:ft I -hdlc:r_seq %d -hdlc:s_seq %d",
				hh->saddr(),
				hh->daddr(),
				ifr->recv_seqno, 
				ifr->send_seqno);
			
		// } else if (newtrace_) {
// 			sprintf(pt_->buffer() + offset,
// 				"-P hdlc -Psa %d -Pda %d -Pft I -Pr_seq %d -Ps_seq %d",
// 				hh->saddr(),
// 				hh->daddr(),
// 				ifr->recv_seqno, 
// 				ifr->send_seqno);
		} else {
			sprintf(pt_->buffer() + offset,
				"[%d %d I %d %d]",
				hh->saddr(),
				hh->daddr(),
				ifr->recv_seqno, 
				ifr->send_seqno);
		}
		break;
		
	case HDLC_S_frame:
		if (pt_->tagged()) {
			sprintf(pt_->buffer() + offset,
				"-hdlc:sa %d -hdlc:da %d -hdlc:ft S -hdlc:r_seq %d -hdlc:stype %s",
				hh->saddr(),
				hh->daddr(),
				sf->recv_seqno,
				(sf->stype == RR) ? "RR" :
				(sf->stype == REJ) ? "REJ" :
				(sf->stype == RNR) ? "RNR" :
				(sf->stype == SREJ) ? "SREJ" : "UNKN");
			
		// } else if (newtrace_) {
// 			sprintf(pt_->buffer() + offset,
// 				"-P hdlc -Psa %d -Pda %d -Pft S -Pr_seq %d -Pstype %s",
// 				hh->saddr(),
// 				hh->daddr(),
// 				sf->recv_seqno,
// 				sf->stype == RR ? "RR" :
// 				sf->stype == REJ ? "REJ" :
// 				sf->stype == RNR ? "RNR" :
// 				sf->stype == SREJ ? "SREJ" : "UNKN");
			
		} else {
			sprintf(pt_->buffer() + offset,
				"[%d %d S %d %s]",
				hh->saddr(),
				hh->daddr(),
				sf->recv_seqno,
				sf->stype == RR ? "RR" :
				sf->stype == REJ ? "REJ" :
				sf->stype == RNR ? "RNR" :
				sf->stype == SREJ ? "SREJ" :
				"UNKN");
		}
		break;
		
	case HDLC_U_frame:
		if (pt_->tagged()) {
			sprintf(pt_->buffer() + offset,
				"-hdlc:sa %d -hdlc:da %d -hdlc:ft U -hdlc:utype %s",
				hh->saddr(),
				hh->daddr(),
				uf->utype == SABME ? "SABME" :
				uf->utype == UA ? "UA" :
				uf->utype == DM ? "DM" :
				uf->utype == DISC ? "DISC" : "UNKN");
		
		// } else if (newtrace_) {
// 			sprintf(pt_->buffer() + offset,
// 				"-P hdlc -Psa %d -Pda %d -Pft U -Putype %s",
// 				hh->saddr(),
// 				hh->daddr(),
// 				uf->utype == SABME ? "SABME" :
// 				uf->utype == UA ? "UA" :
// 				uf->utype == DM ? "DM" :
// 				uf->utype == DISC ? "DISC" : "UNKN");
			
		} else {
			sprintf(pt_->buffer() + offset,
				"[%d %d U %s]",
				hh->saddr(),
				hh->daddr(),
				uf->utype == SABME ? "SABME" :
				uf->utype == UA ? "UA" :
				uf->utype == DM ? "DM" :
				uf->utype == DISC ? "DISC" : "UNKN");
		}
		break;
		
	default:
		
		fprintf(stderr, "Unknown HDLC frame type\n");
		exit(1);
	}
	
}



void SatTrace::format(int tt, int s, int d, Packet* p)
{
	int offset = 0;
	hdr_cmn *th = hdr_cmn::access(p);
	hdr_ip *iph = hdr_ip::access(p);
	hdr_tcp *tcph = hdr_tcp::access(p);
	hdr_sctp *sctph = hdr_sctp::access(p);
	hdr_rtp *rh = hdr_rtp::access(p);
	hdr_srm *sh = hdr_srm::access(p); 

	const char* sname = "null";
	int lasth, nexth, snadd;
	Node* n;

	packet_t t = th->ptype();
	const char* name = packet_info.name(t);

        /* SRM-specific */
	if (strcmp(name,"SRM") == 0 || strcmp(name,"cbr") == 0 || strcmp(name,"udp") == 0) {
            if ( sh->type() < 5 && sh->type() > 0 ) {
	        sname = srm_names_[sh->type()];
	    }
	}

	if (name == 0)
		abort();

	int seqno;
	/* UDP's now have seqno's too */
	if (t == PT_RTP || t == PT_CBR || t == PT_UDP || t == PT_EXP ||
	    t == PT_PARETO)
		seqno = rh->seqno();
	else if (t == PT_TCP || t == PT_ACK || t == PT_HTTP || t == PT_FTP ||
	    t == PT_TELNET)
		seqno = tcph->seqno();
	else
		seqno = -1;
        /* 
         * When new flags are added, make sure to change NUMFLAGS
         * in trace.h
         */
        char flags[NUMFLAGS+1];
        for (int i = 0; i < NUMFLAGS; i++)
		flags[i] = '-';
        flags[NUMFLAGS] = 0;

	hdr_flags* hf = hdr_flags::access(p);
	flags[0] = hf->ecn_ ? 'C' : '-';          // Ecn Echo
	flags[1] = hf->pri_ ? 'P' : '-'; 
	flags[2] = '-';
	flags[3] = hf->cong_action_ ? 'A' : '-';   // Congestion Action
	flags[4] = hf->ecn_to_echo_ ? 'E' : '-';   // Congestion Experienced
	flags[5] = hf->fs_ ? 'F' : '-';
	flags[6] = hf->ecn_capable_ ? 'N' : '-';
	flags[7] = 0; // only for SCTP
	
#ifdef notdef
	flags[1] = (iph->flags() & PF_PRI) ? 'P' : '-';
	flags[2] = (iph->flags() & PF_USR1) ? '1' : '-';
	flags[3] = (iph->flags() & PF_USR2) ? '2' : '-';
	flags[5] = 0;
#endif
	char *src_nodeaddr = Address::instance().print_nodeaddr(iph->saddr());
	char *src_portaddr = Address::instance().print_portaddr(iph->sport());
	char *dst_nodeaddr = Address::instance().print_nodeaddr(iph->daddr());
	char *dst_portaddr = Address::instance().print_portaddr(iph->dport());

	// Find position of previous hop and next hop
	double s_lat = -999, s_lon = -999, d_lat = -999, d_lon = -999;
	n = Node::nodehead_.lh_first;
// XXX what if n is not a SatNode?? Need a dynamic cast here, or make sure that
// only sat tracing elements go between sat nodes.
	// SatNode *sn = dynamic_cast<SatNode*>(n);
	assert (n != 0);
	lasth = th->last_hop_;
	nexth = th->next_hop_;
	for (; n; n = n->nextnode() ) {
		SatNode *sn = (SatNode*) n;
		snadd = sn->address();
		if (lasth == snadd) {
			s_lat = RAD_TO_DEG(SatGeometry::get_latitude(sn->position()->coord()));
			s_lon = RAD_TO_DEG(SatGeometry::get_longitude(sn->position()->coord()));
			if (d_lat != -999) 
				break; // Have now found both s and d
		}
		if (nexth == snadd) {
			d_lat = RAD_TO_DEG(SatGeometry::get_latitude(sn->position()->coord())); 
			d_lon = RAD_TO_DEG(SatGeometry::get_longitude(sn->position()->coord()));
			if (s_lat != -999) 
				break; // Have now found both s and d
		}
	}

	if (show_sctphdr_ && t == PT_SCTP) {
	   for(unsigned int i = 0; i < sctph->NumChunks(); i++) {
		   switch(sctph->SctpTrace()[i].eType) {
			case SCTP_CHUNK_INIT:
			case SCTP_CHUNK_INIT_ACK:
			case SCTP_CHUNK_COOKIE_ECHO:
			case SCTP_CHUNK_COOKIE_ACK:
				flags[7] = 'I';              // connection initialization
				break;

			case SCTP_CHUNK_DATA:
				flags[7] = 'D';
				break;

			case SCTP_CHUNK_SACK:
				flags[7] = 'S';
				break;

			case SCTP_CHUNK_FORWARD_TSN:
				flags[7] = 'R';
				break;

			case SCTP_CHUNK_HB:
				flags[7] = 'H';
				break;

			case SCTP_CHUNK_HB_ACK:
				flags[7] = 'B';
				break;
		        default:
				assert (false);
				break;
			}
		   sprintf(pt_->buffer(), "%c %.4f %d %d %s %d %s %d %s.%s %s.%s %d %d %d %d %d %.2f %.2f %.2f %.2f",
			tt,
			pt_->round(Scheduler::instance().clock()),
			lasth,
			nexth,
			name,
			th->size(),
			flags,
			iph->flowid() /* was p->class_ */,
			// iph->src() >> (Address::instance().NodeShift_[1]), 
                        // iph->src() & (Address::instance().PortMask_), 
                        // iph->dst() >> (Address::instance().NodeShift_[1]), 
                        // iph->dst() & (Address::instance().PortMask_),
			src_nodeaddr,
			src_portaddr,
			dst_nodeaddr,
			dst_portaddr,
			sctph->NumChunks(),
			sctph->SctpTrace()[i].uiTsn,
			th->uid(), /* was p->uid_ */
			sctph->SctpTrace()[i].usStreamId,
			sctph->SctpTrace()[i].usStreamSeqNum,
			s_lat,
			s_lon,
			d_lat,
			d_lon);

		   /* The caller already calls pt_->dump() for us, but
		    * since SCTP needs to dump once per chunk, we call
		    * dump ourselves for all but the last * chunk.  
		    */
		   if(i < sctph->NumChunks() - 1)
			   pt_->dump();
		}
	}
	else if (pt_->tagged()) {
		sprintf(pt_->nbuffer(), 
			"%c %g -s %d -d %d -p %s -e %d -c %d -i %d -a %d -x {%s.%s %s.%s %d %s %s}",
			tt,
			Scheduler::instance().clock(),
			s,
 			d,
			name,
			th->size(),
			iph->flowid(),
			th->uid(),
			iph->flowid(),
			src_nodeaddr,
			src_portaddr,
			dst_nodeaddr,
			dst_portaddr,
			seqno,flags,sname);
	} else if (!show_tcphdr_) {
		sprintf(pt_->buffer(), "%c %.4f %d %d %s %d %s %d %s.%s %s.%s %d %d %.2f %.2f %.2f %.2f",
			tt,
			pt_->round(Scheduler::instance().clock()),
			lasth,
			nexth,
			name,
			th->size(),
			flags,
			iph->flowid() /* was p->class_ */,
			// iph->src() >> (Address::instance().NodeShift_[1]), 
                        // iph->src() & (Address::instance().PortMask_), 
                        // iph->dst() >> (Address::instance().NodeShift_[1]), 
                        // iph->dst() & (Address::instance().PortMask_),
			src_nodeaddr,
			src_portaddr,
			dst_nodeaddr,
			dst_portaddr,
			seqno,
			th->uid(), /* was p->uid_ */
			s_lat,
			s_lon,
			d_lat,
			d_lon);
	} else {
		sprintf(pt_->buffer(), 
			"%c %.4f %d %d %s %d %s %d %s.%s %s.%s %d %d %d 0x%x %d %d %.2f %.2f %.2f %.2f",
			tt,
			pt_->round(Scheduler::instance().clock()),
			lasth,
			nexth,
			name,
			th->size(),
			flags,
			iph->flowid(), /* was p->class_ */
		        // iph->src() >> (Address::instance().NodeShift_[1]), 
			// iph->src() & (Address::instance().PortMask_), 
  		        // iph->dst() >> (Address::instance().NodeShift_[1]), 
  		        // iph->dst() & (Address::instance().PortMask_),
			src_nodeaddr,
			src_portaddr,
			dst_nodeaddr,
			dst_portaddr,
			seqno,
			th->uid(), /* was p->uid_ */
			tcph->ackno(),
			tcph->flags(),
			tcph->hlen(),
			tcph->sa_length(),
			s_lat,
			s_lon,
			d_lat,
			d_lon);
	}

	offset = strlen(pt_->buffer());
	if (t == PT_HDLC)
		format_hdlc(p, offset);
	

	if (pt_->namchannel() != 0)
		sprintf(pt_->nbuffer(), 
			"%c -t %g -s %d -d %d -p %s -e %d -c %d -i %d -a %d -x {%s.%s %s.%s %d %s %s}",
			tt,
			Scheduler::instance().clock(),
			s,
 			d,
			name,
			th->size(),
			iph->flowid(),
			th->uid(),
			iph->flowid(),
			src_nodeaddr,
			src_portaddr,
			dst_nodeaddr,
			dst_portaddr,
			seqno,flags,sname);
	delete [] src_nodeaddr;
  	delete [] src_portaddr;
  	delete [] dst_nodeaddr;
   	delete [] dst_portaddr;
}

void SatTrace::traceonly(Packet* p)
{        
	format(type_, src_, dst_, p);
	pt_->dump();
}

//
// we need a DequeTraceClass here because a 'h' event need to go together
// with the '-' event. It's possible to use a postprocessing script, but 
// seems that's inconvient.
//
static class SatDequeTraceClass : public TclClass {
public:
	SatDequeTraceClass() : TclClass("Trace/Sat/Deque") { }
	TclObject* create(int args, const char*const* argv) {
		if (args >= 5)
			return (new SatDequeTrace(*argv[4]));
		return NULL;
	}
} sat_dequetrace_class;


void 
SatDequeTrace::recv(Packet* p, Handler* h)
{
	// write the '-' event first
	format(type_, src_, dst_, p);
	pt_->dump();
	pt_->namdump();

	if (pt_->namchannel() != 0) {
		hdr_cmn *th = hdr_cmn::access(p);
		hdr_ip *iph = hdr_ip::access(p);
		hdr_srm *sh = hdr_srm::access(p);
		const char* sname = "null";   

		packet_t t = th->ptype();
		const char* name = packet_info.name(t);
		
		if (strcmp(name,"SRM") == 0 || strcmp(name,"cbr") == 0 || strcmp(name,"udp") == 0) {
		    if ( sh->type() < 5 && sh->type() > 0  ) {
		        sname = srm_names_[sh->type()];
		    }
		}   

		char *src_nodeaddr = Address::instance().print_nodeaddr(iph->saddr());
		char *src_portaddr = Address::instance().print_portaddr(iph->sport());
		char *dst_nodeaddr = Address::instance().print_nodeaddr(iph->daddr());
		char *dst_portaddr = Address::instance().print_portaddr(iph->dport());

		char flags[NUMFLAGS+1];
		for (int i = 0; i < NUMFLAGS; i++)
			flags[i] = '-';
		flags[NUMFLAGS] = 0;

		hdr_flags* hf = hdr_flags::access(p);
		flags[0] = hf->ecn_ ? 'C' : '-';          // Ecn Echo
		flags[1] = hf->pri_ ? 'P' : '-'; 
		flags[2] = '-';
		flags[3] = hf->cong_action_ ? 'A' : '-';   // Congestion Action
		flags[4] = hf->ecn_to_echo_ ? 'E' : '-';   // Congestion Experienced
		flags[5] = hf->fs_ ? 'F' : '-';
		flags[6] = hf->ecn_capable_ ? 'N' : '-';
	
#ifdef notdef
		flags[1] = (iph->flags() & PF_PRI) ? 'P' : '-';
		flags[2] = (iph->flags() & PF_USR1) ? '1' : '-';
		flags[3] = (iph->flags() & PF_USR2) ? '2' : '-';
		flags[5] = 0;
#endif
		
		sprintf(pt_->nbuffer(), 
			"%c -t %g -s %d -d %d -p %s -e %d -c %d -i %d -a %d -x {%s.%s %s.%s %d %s %s}",
			'h',
			Scheduler::instance().clock(),
			src_,
  			dst_,
			name,
			th->size(),
			iph->flowid(),
			th->uid(),
			iph->flowid(),
			src_nodeaddr,
			src_portaddr,
			dst_nodeaddr,
			dst_portaddr,
			-1, flags, sname);
		pt_->namdump();
		delete [] src_nodeaddr;
		delete [] src_portaddr;
		delete [] dst_nodeaddr;
		delete [] dst_portaddr;
	}

	/* hack: if trace object not attached to anything free packet */
	if (target_ == 0)
		Packet::free(p);
	else
		send(p, h);
}
