/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linking this file statically or dynamically with other modules is making
 * a combined work based on this file.  Thus, the terms and conditions of
 * the GNU General Public License cover the whole combination.
 *
 * In addition, as a special exception, the copyright holders of this file
 * give you permission to combine this file with free software programs or
 * libraries that are released under the GNU LGPL and with code included in
 * the standard release of ns-2 under the Apache 2.0 license or under
 * otherwise-compatible licenses with advertising requirements (or modified
 * versions of such code, with unchanged license).  You may copy and
 * distribute such a system following the terms of the GNU GPL for this
 * file and the licenses of the other code concerned, provided that you
 * include the source code of that other code when and as the GNU GPL
 * requires distribution of source code.
 *
 * Note that people who make modified versions of this file are not
 * obligated to grant this special exception for their modified versions;
 * it is their choice whether to do so.  The GNU General Public License
 * gives permission to release a modified version without this exception;
 * this exception also makes it possible to release a modified version
 * which carries forward this exception.
 *
 * $Header: /cvsroot/nsnam/ns-2/adc/salink.cc,v 1.6 2005/08/26 05:05:28 tomh Exp $
 */

#include "packet.h"
#include "ip.h"
#include "resv.h"
#include "connector.h"
#include "adc.h"
#include "salink.h"

static class SALinkClass : public TclClass {
public:
	SALinkClass() : TclClass("SALink") {}
	TclObject* create(int, const char*const*) {
		return (new SALink());
	}
}class_salink;

SALink::SALink() : adc_(0), numfl_(-1), tchan_(0), onumfl_(0), last_(-1)
{
	int i;
	for (i=0;i<NFLOWS;i++) {
		pending_[i].flowid=-1;
		pending_[i].status=0;
	}
	bind("src_", &src_);
	bind("dst_", &dst_);

	numfl_.tracer(this);
	numfl_.name("\"Admitted Flows\"");
}


void SALink::recv(Packet *p, Handler *h)
{
	int decide;
	int j;
	
	hdr_cmn *ch=hdr_cmn::access(p);
	hdr_ip *iph=hdr_ip::access(p);
	hdr_resv *rv=hdr_resv::access(p);
	
	//CLEAN THIS UP
	int cl=(iph->flowid())?1:0;
	
	switch(ch->ptype()) {
	case PT_REQUEST:
		decide=adc_->admit_flow(cl,rv->rate(),rv->bucket());
		if (tchan_)
			if (last_ != decide) {
				int n;
				char wrk[50];
				double t = Scheduler::instance().clock();
				sprintf(wrk, "l -t %g -s %d -d %d -S COLOR -c %s", 
					t, src_, dst_, decide ? "MediumBlue" : "red" );
				n = strlen(wrk);
				wrk[n] = '\n';
				wrk[n+1] = 0;
				(void)Tcl_Write(tchan_, wrk, n+1);
				last_ = decide;
			}
		//put decide in the packet
		rv->decision() &= decide;
		if (decide) {
			j=get_nxt();
			pending_[j].flowid=iph->flowid();
			//pending_[j].status=decide;
			numfl_++;
		}
		break;
	case PT_ACCEPT:
	case PT_REJECT:
		break;
	case PT_CONFIRM:
		{
			j=lookup(iph->flowid());
			if (j!=-1) {
				if (!rv->decision()) {
					//decrease the avload for this class 
					adc_->rej_action(cl,rv->rate(),rv->bucket());
					numfl_--;
				}
				pending_[j].flowid=-1;
			}
			break;
		}
	case PT_TEARDOWN:
		{
			adc_->teardown_action(cl,rv->rate(),rv->bucket());
			numfl_--;
			break;
		}
	default:
#ifdef notdef
		error("unknown signalling message type : %d",ch->ptype());
		abort();
#endif
		break;
	}
	send(p,h);
}

int SALink::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	char wrk[500];
	
	if (argc ==3) {
		if (strcmp(argv[1],"attach-adc") == 0 ) {
			adc_=(ADC *)TclObject::lookup(argv[2]);
			if (adc_ ==0 ) {
				tcl.resultf("no such node %s", argv[2]);
				return(TCL_ERROR);
			}
			return(TCL_OK);
		}
		if (strcmp(argv[1], "attach") == 0) {
			int mode;
			const char* id = argv[2];
			tchan_ = Tcl_GetChannel(tcl.interp(), (char*)id, &mode);
			if (tchan_ == 0) {
				tcl.resultf("SALink: trace: can't attach %s for writing", id);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
	}
	if (argc == 2) {
		if (strcmp(argv[1], "add-trace") == 0) {
			if (tchan_) {
				sprintf(wrk, "a -t * -n %s:%d-%d -s %d",
					adc_->type(), src_, dst_, src_);
				int n = strlen(wrk);
				wrk[n] = '\n';
				wrk[n+1] = 0;
				(void)Tcl_Write(tchan_, wrk, n+1);
				numfl_ = 0;
			}
			return (TCL_OK);
		}
	}
	return Connector::command(argc,argv);
}

int SALink::lookup(int flowid)
{
	int i;
	for (i=0;i<NFLOWS;i++)
		if (pending_[i].flowid==flowid)
			return i;
	return(-1);
}

int SALink::get_nxt()
{
	int i;
	for (i=0;i<NFLOWS;i++)
		{
			if (pending_[i].flowid==-1)
				return i;
		}
	printf("Ran out of pending space \n");
	exit(1);
	return i;
}

void SALink::trace(TracedVar* v)
{

	char wrk[500];
	int *p, newval;

	if (strcmp(v->name(), "\"Admitted Flows\"") == 0) {
		p = &onumfl_;
	}
	else {
		fprintf(stderr, "SALink: unknown trace var %s\n", v->name());
		return;
	}

	newval = int(*((TracedInt*)v));

	if (tchan_) {
		int n;
		double t = Scheduler::instance().clock();
		/* f -t 0.0 -s 1 -a SA -T v -n Num -v 0 -o 0 */
		sprintf(wrk, "f -t %g -s %d -a %s:%d-%d -T v -n %s -v %d -o %d",
			t, src_, adc_->type(), src_, dst_, v->name(), newval, *p);
		n = strlen(wrk);
		wrk[n] = '\n';
		wrk[n+1] = 0;
		(void)Tcl_Write(tchan_, wrk, n+1);
		
	}

	*p = newval;

	return;
}
