/* -*-	Mode:C++; c-basic-offset:8; tab-width:8 -*- */
/*
 * Copyright (c) 1990-1997 Regents of the University of California.
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
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/trace/trace.cc,v 1.82 2009/11/16 05:51:27 tom_henderson Exp $ (LBL)
 */

#include <stdio.h>
#include <stdlib.h>
#include "packet.h"
#include "ip.h"
#include "tcp.h"
#include "sctp.h"
#include "rtp.h"
#include "srm.h"
#include "tfrc.h"
#include "flags.h"
#include "address.h"
#include "trace.h"
#include "rap/rap.h"


//const double Trace::PRECISION = 1.0e+6; 

class TraceClass : public TclClass {
public:
	TraceClass() : TclClass("Trace") { }
	TclObject* create(int argc, const char*const* argv) {
		if (argc >= 5)
			return (new Trace(*argv[4]));
		return 0;
	}
} trace_class;


Trace::Trace(int type)
	: Connector(), callback_(0), pt_(0), type_(type)
{
	bind("src_", (int*)&src_);
	bind("dst_", (int*)&dst_);
	bind("callback_", &callback_);
	bind("show_tcphdr_", &show_tcphdr_);
	bind("show_sctphdr_", &show_sctphdr_);
	pt_ = new BaseTrace;
}

Trace::~Trace()
{
}

/*
 * $trace detach
 * $trace flush
 * $trace attach $fileID
 */
int Trace::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "detach") == 0) {
			pt_->channel(0) ;
			pt_->namchannel(0) ;
			return (TCL_OK);
		}
		if (strcmp(argv[1], "flush") == 0) {
			Tcl_Channel ch = pt_->channel();
			Tcl_Channel namch = pt_->namchannel();
			if (ch != 0) 
				pt_->flush(ch);
				//Tcl_Flush(pt_.channel());
			if (namch != 0)
				//Tcl_Flush(pt_->namchannel());
				pt_->flush(namch);
			return (TCL_OK);
		}
                if (strcmp(argv[1], "tagged") == 0) {
			tcl.resultf("%d", pt_->tagged());
                        return (TCL_OK);
                }
	} else if (argc == 3) {
		if (strcmp(argv[1], "annotate") == 0) {
			if (pt_->channel() != 0)
				annotate(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "attach") == 0) {
			int mode;
			const char* id = argv[2];
			Tcl_Channel ch = Tcl_GetChannel(tcl.interp(), (char*)id,
						  &mode);
			pt_->channel(ch); 
			if (pt_->channel() == 0) {
				tcl.resultf("trace: can't attach %s for writing", id);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "namattach") == 0) {
			int mode;
			const char* id = argv[2];
			Tcl_Channel namch = Tcl_GetChannel(tcl.interp(), 
							   (char*)id, &mode);
			pt_->namchannel(namch); 
			if (pt_->namchannel() == 0) {
				tcl.resultf("trace: can't attach %s for writing", id);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "ntrace") == 0) {
			if (pt_->namchannel() != 0) 
				write_nam_trace(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "tagged") == 0) {
                        int tag;
			if (Tcl_GetBoolean(tcl.interp(),
					   (char*)argv[2], &tag) == TCL_OK) {
				pt_->tagged(tag);
				return (TCL_OK);
			} else return (TCL_ERROR);
                }
	}
	return (Connector::command(argc, argv));
}

void Trace::write_nam_trace(const char *s)
{
	sprintf(pt_->nbuffer(), "%s", s);
	pt_->namdump();
}

void Trace::annotate(const char* s)
{
	if (pt_->tagged()) {
		sprintf(pt_->buffer(),
			"v "TIME_FORMAT" -e {sim_annotation %g %s}",
			Scheduler::instance().clock(), 
			Scheduler::instance().clock(), s);
	} else {
		sprintf(pt_->buffer(),
			"v "TIME_FORMAT" eval {set sim_annotation {%s}}", 
			pt_->round(Scheduler::instance().clock()), s);
	}
	pt_->dump();
	callback();
	sprintf(pt_->nbuffer(), "v -t "TIME_FORMAT" -e sim_annotation %g %s", 
		Scheduler::instance().clock(), 
		Scheduler::instance().clock(), s);
	pt_->namdump();
}


char* srm_names[] = {
        SRM_NAMES
};

int
Trace::get_seqno(Packet* p)
{
	hdr_cmn *th = hdr_cmn::access(p);
	hdr_tcp *tcph = hdr_tcp::access(p);
	hdr_rtp *rh = hdr_rtp::access(p);
        hdr_rap *raph = hdr_rap::access(p);
	hdr_tfrc *tfrch = hdr_tfrc::access(p);
	hdr_tfrc_ack *tfrch_ack = hdr_tfrc_ack::access(p);
	packet_t t = th->ptype();
	int seqno;

	/* UDP's now have seqno's too */
	if (t == PT_RTP || t == PT_CBR || t == PT_UDP || t == PT_EXP ||
	    t == PT_PARETO)
		seqno = rh->seqno();
        else if (t == PT_RAP_DATA || t == PT_RAP_ACK)
                seqno = raph->seqno();
	else if (t == PT_TCP || t == PT_ACK || t == PT_HTTP || t == PT_FTP ||
	    t == PT_TELNET || t == PT_XCP)
		seqno = tcph->seqno();
	else if (t == PT_TFRC)
		seqno = tfrch->seqno;
	else if (t == PT_TFRC_ACK)
                seqno = tfrch_ack->seqno;
	else
		seqno = -1;
 	return seqno;
}

// this function should retain some backward-compatibility, so that
// scripts don't break.
void Trace::format(int tt, int s, int d, Packet* p)
{
	hdr_cmn *th = hdr_cmn::access(p);
	hdr_ip *iph = hdr_ip::access(p);
	hdr_tcp *tcph = hdr_tcp::access(p);
	hdr_sctp *sctph = hdr_sctp::access(p);
	hdr_srm *sh = hdr_srm::access(p); 

	const char* sname = "null";

	packet_t t = th->ptype();
	const char* name = packet_info.name(t);

        /* SRM-specific */
	if (strcmp(name,"SRM") == 0 || strcmp(name,"cbr") == 0 || strcmp(name,"udp") == 0) {
            if ( sh->type() < 5 && sh->type() > 0 ) {
	        sname = srm_names[sh->type()];
	    }
	}

	if (name == 0)
		abort();

	int seqno = get_seqno(p);
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
	flags[5] = hf->fs_ ? 'F' : '-';		   // Fast start: see tcp-fs and tcp-int
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

	if (pt_->tagged()) {
		sprintf(pt_->buffer(), 
			"%c "TIME_FORMAT" -s %d -d %d -p %s -e %d -c %d -i %d -a %d -x {%s.%s %s.%s %d %s %s}",
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
	} else if (show_sctphdr_ && t == PT_SCTP) {
		double timestamp;
		timestamp = Scheduler::instance().clock();
		
		for(unsigned int i = 0; i < sctph->NumChunks(); i++) {
			switch(sctph->SctpTrace()[i].eType) {
			case SCTP_CHUNK_INIT:
			case SCTP_CHUNK_INIT_ACK:
			case SCTP_CHUNK_COOKIE_ECHO:
			case SCTP_CHUNK_COOKIE_ACK:
				flags[7] = 'I';     // connection initialization
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
		
			case SCTP_CHUNK_NRSACK:
				flags[7] = 'N';
				break;
	     
			default:
				assert (false);
			}
			sprintf(pt_->buffer(),
				"%c "TIME_FORMAT" %d %d %s %d %s %d %s.%s %s.%s %d %d %d %d %d",
				tt,
				pt_->round(timestamp),
				s,
				d,
				name,
				th->size(),
				flags,
				iph->flowid(), /* was p->class_ */
				src_nodeaddr,
				src_portaddr,
				dst_nodeaddr,
				dst_portaddr,
				sctph->NumChunks(),
				sctph->SctpTrace()[i].uiTsn,
				th->uid(), /* was p->uid_ */
				sctph->SctpTrace()[i].usStreamId,
				sctph->SctpTrace()[i].usStreamSeqNum);	     

			/* The caller already calls pt_->dump() for us,
			 * but since SCTP needs to dump once per chunk, we
			 * call dump ourselves for all but the last chunk.
			 */
			assert (sctph->NumChunks() >= 1);
			if(i < sctph->NumChunks() - 1)
				pt_->dump();
		}
	} else if (!show_tcphdr_) {
		sprintf(pt_->buffer(), "%c "TIME_FORMAT" %d %d %s %d %s %d %s.%s %s.%s %d %d",
			tt,
			pt_->round(Scheduler::instance().clock()),
			s,
			d,
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
			th->uid() /* was p->uid_ */);
	} else {
		sprintf(pt_->buffer(), 
			"%c "TIME_FORMAT" %d %d %s %d %s %d %s.%s %s.%s %d %d %d 0x%x %d %d",
			tt,
			pt_->round(Scheduler::instance().clock()),
			s,
			d,
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
			tcph->sa_length());
	}
	if (pt_->namchannel() != 0)
		sprintf(pt_->nbuffer(), 
			"%c -t "TIME_FORMAT" -s %d -d %d -p %s -e %d -c %d -i %d -a %d -x {%s.%s %s.%s %d %s %s}",
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

void Trace::recv(Packet* p, Handler* h)
{
	format(type_, src_, dst_, p);
	pt_->dump();
	callback();
	pt_->namdump();
	/* hack: if trace object not attached to anything free packet */
	if (target_ == 0)
		Packet::free(p);
	else
		send(p, h);
}

void Trace::recvOnly(Packet *p)
{
	format(type_, src_, dst_, p);
	pt_->dump();
	callback();
	pt_->namdump();	
	target_->recvOnly(p);
}

void Trace::trace(TracedVar* var)
{
	char tmp[256] = "";
	Scheduler& s = Scheduler::instance();
	if (&s == 0)
		return;

	if (pt_->tagged()) {
		sprintf(pt_->buffer(), "%c "TIME_FORMAT" -a %s -n %s -v %s",
			type_,
			pt_->round(s.clock()),
			var->owner()->name(),
			var->name(),
			var->value(tmp, 256));
	} else {
		// format: use Mark's nam feature code without the '-' prefix
		sprintf(pt_->buffer(), "%c t"TIME_FORMAT" a%s n%s v%s",
			type_,
			pt_->round(s.clock()),
			var->owner()->name(),
			var->name(),
			var->value(tmp, 256));
	}
	pt_->dump();
	callback();
}

void Trace::callback() 
{
	if (callback_) {
		Tcl& tcl = Tcl::instance();
		tcl.evalf("%s handle { %s }", name(), pt_->buffer());
	}
}

//
// we need a DequeTraceClass here because a 'h' event need to go together
// with the '-' event. It's possible to use a postprocessing script, but 
// seems that's inconvient.
//
static class DequeTraceClass : public TclClass {
public:
	DequeTraceClass() : TclClass("Trace/Deque") { }
	TclObject* create(int args, const char*const* argv) {
		if (args >= 5)
			return (new DequeTrace(*argv[4]));
		return NULL;
	}
} dequetrace_class;


DequeTrace::~DequeTrace()
{
}

void 
DequeTrace::recv(Packet* p, Handler* h)
{
	// write the '-' event first
	format(type_, src_, dst_, p);
	pt_->dump();
	callback();
	pt_->namdump();

	if (pt_->namchannel() != 0 ||
	    (pt_->tagged() && pt_->channel() !=0)) {
		hdr_cmn *th = hdr_cmn::access(p);
		hdr_ip *iph = hdr_ip::access(p);
		hdr_srm *sh = hdr_srm::access(p);
		const char* sname = "null";   

		packet_t t = th->ptype();
		const char* name = packet_info.name(t);
		
		if (strcmp(name,"SRM") == 0 || strcmp(name,"cbr") == 0 || strcmp(name,"udp") == 0) {
		    if ( sh->type() < 5 && sh->type() > 0  ) {
		        sname = srm_names[sh->type()];
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
		flags[7] = 0; // only for SCTP
	
#ifdef notdef
		flags[1] = (iph->flags() & PF_PRI) ? 'P' : '-';
		flags[2] = (iph->flags() & PF_USR1) ? '1' : '-';
		flags[3] = (iph->flags() & PF_USR2) ? '2' : '-';
		flags[5] = 0;
#endif
		
		if (pt_->nbuffer() != 0) {
			sprintf(pt_->nbuffer(), 
				"%c -t "TIME_FORMAT" -s %d -d %d -p %s -e %d -c %d -i %d -a %d -x {%s.%s %s.%s %d %s %s}",
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
		}
		if (pt_->tagged() && pt_->buffer() != 0) {
			sprintf(pt_->buffer(), 
				"%c "TIME_FORMAT" -s %d -d %d -p %s -e %d -c %d -i %d -a %d -x {%s.%s %s.%s %d %s %s}",
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
			pt_->dump();
		}

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

