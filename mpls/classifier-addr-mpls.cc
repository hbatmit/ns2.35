// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-

/*
 * classifier-addr-mpls.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: classifier-addr-mpls.cc,v 1.9 2011/10/02 22:32:34 tom_henderson Exp $
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

//
// Original source contributed by Gaeil Ahn. See below.
//
// $Header: /cvsroot/nsnam/ns-2/mpls/classifier-addr-mpls.cc,v 1.9 2011/10/02 22:32:34 tom_henderson Exp $

// XXX
//
// - Because MPLS header contains pointers, it cannot be used WITH multicast 
//   routing which replicates packets
// - Currently it works only with flat routing.

/**************************************************************************
 * Copyright (c) 2000 by Gaeil Ahn                                   	  *
 * Everyone is permitted to copy and distribute this software.		  *
 * Please send mail to fog1@ce.cnu.ac.kr when you modify or distribute    *
 * this sources.							  *
 **************************************************************************/

/************************************************************
 *                                                          *
 *    File: File for packet switching in MPLS node          *
 *    Author: Gaeil Ahn (fog1@ce.cnu.ac.kr), Dec. 1999      *
 *                                                          *
 ************************************************************/

#include "packet.h"
#include "trace.h"
#include "classifier-addr-mpls.h"

int hdr_mpls::offset_;

static class shimhdreaderClass : public PacketHeaderClass {
public:
	shimhdreaderClass() : PacketHeaderClass("PacketHeader/MPLS", 
						sizeof(hdr_mpls)) {
		bind_offset(&hdr_mpls::offset_);
	}
} class_shimhdreader;

static class MPLSAddrClassifierClass : public TclClass {
public:
	MPLSAddrClassifierClass() : TclClass("Classifier/Addr/MPLS") {}
	virtual TclObject* create(int, const char*const*) {
		return (new MPLSAddressClassifier());
	}
	virtual void bind();
	virtual int method(int argc, const char*const* argv);
} class_mpls_addr_classifier;

void MPLSAddrClassifierClass::bind()
{
	TclClass::bind();
	add_method("minimum-lspid");
	add_method("dont-care");
	add_method("ordered-control?");
	add_method("on-demand?");
	add_method("enable-ordered-control");
	add_method("enable-on-demand");
}

int MPLSAddrClassifierClass::method(int ac, const char*const* av)
{
	Tcl& tcl = Tcl::instance();
	int argc = ac - 2;
	const char*const* argv = av + 2;

	if (argc == 2) {
		if (strcmp(argv[1], "minimum-lspid") == 0) {
			tcl.resultf("%d", MPLS_MINIMUM_LSPID);
			return (TCL_OK);
		} else if (strcmp(argv[1], "dont-care") == 0) {
			tcl.resultf("%d", MPLS_DONTCARE);
			return (TCL_OK);
		} if (strcmp(argv[1], "ordered-control?") == 0) {
			tcl.resultf("%d", 
				    MPLSAddressClassifier::ordered_control_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "on-demand?") == 0) {
			tcl.resultf("%d", MPLSAddressClassifier::on_demand_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "enable-ordered-control") == 0) {
			MPLSAddressClassifier::ordered_control_ = 1;
			return (TCL_OK);
		} else if (strcmp(argv[1], "enable-on-demand") == 0) {
			MPLSAddressClassifier::on_demand_ = 1;
			return (TCL_OK);
		}
	}
	return TclClass::method(ac, av);
}

int MPLSAddressClassifier::on_demand_ = 0;
int MPLSAddressClassifier::ordered_control_ = 0;

MPLSAddressClassifier::MPLSAddressClassifier() : 
	data_driven_(0), control_driven_(0)
{
	PFT_.NB_ = 0;
	ERB_.NB_ = 0;
	LIB_.NB_ = 0;
	ttl_ = 32;
}

void MPLSAddressClassifier::delay_bind_init_all()
{
	delay_bind_init_one("ttl_");
	delay_bind_init_one("trace_mpls_");   
	delay_bind_init_one("label_");
	delay_bind_init_one("enable_reroute_");
	delay_bind_init_one("reroute_option_");
	delay_bind_init_one("data_driven_");
	delay_bind_init_one("control_driven_");
	AddressClassifier::delay_bind_init_all();
}

// Arguments: varName, localName, tracer
int MPLSAddressClassifier::delay_bind_dispatch(const char *vn, 
					       const char* ln, TclObject *t)
{
	if (delay_bind(vn, ln, "ttl_", &ttl_, t))
		return TCL_OK;
	if (delay_bind(vn, ln, "trace_mpls_", &trace_mpls_, t)) 
		return TCL_OK;
	if (delay_bind(vn, ln, "label_", &label_, t)) 
		return TCL_OK;
	if (delay_bind(vn, ln, "enable_reroute_", &enable_reroute_, t))
		return TCL_OK;
	if (delay_bind(vn, ln, "reroute_option_", &reroute_option_, t))
		return TCL_OK;
	if (delay_bind(vn, ln, "data_driven_", &data_driven_, t))
		return TCL_OK;
	if (delay_bind(vn, ln, "control_driven_", &control_driven_, t))
		return TCL_OK;
	return AddressClassifier::delay_bind_dispatch(vn, ln, t);
}

int MPLSAddressClassifier::classify(Packet* p)
{
	int nexthop = MPLSclassify(p);

	if ((enable_reroute_ == 1) && (size_ > 0) && 
	    (is_link_down(nexthop)))
		// Use alternative path if it exist
		nexthop = do_reroute(p);
	if (nexthop == MPLS_GOTO_L3)
		return AddressClassifier::classify(p);
	// XXX Do NOT return -1, which lets the classifier to process this 
	// packet twice!!
	return (nexthop == -1) ? Classifier::ONCE : nexthop;
}

int MPLSAddressClassifier::is_link_down(int node)
{
	Tcl& tcl = Tcl::instance();   
	tcl.evalf("[%s set mpls_mod_] get-link-status %d", name(), node);
	return (strcmp(tcl.result(), "down") == 0) ? 1 : 0;
}

int MPLSAddressClassifier::do_reroute(Packet* p)
{
	int oIface, oLabel, LIBptr;
	PI_.shimhdr_ = GetShimHeader(p);
	int iLabel = PI_.shimhdr_->label_;

	if (aPathLookup(PI_.dst_.addr_, PI_.phb_, oIface, oLabel, LIBptr) == 0)
		return convertL2toL2(iLabel, oIface, oLabel, LIBptr);
	else {
		PI_.shimhdr_ = DelAllShimHeader(PI_.shimhdr_);
		trace("L", size_, iLabel, "Drop(linkFail)", -1, -1, -1);
		switch (reroute_option_) {
		case MPLS_DROPPACKET:
			return -1;
		case MPLS_L3FORWARDING:
			return MPLS_GOTO_L3;
		case MPLS_MAKENEWLSP:
			Tcl& tcl = Tcl::instance();   
			if (!control_driven_)
				tcl.evalf("%s ldp-trigger-by-switch %d", 
					  name(), PI_.dst_.addr_);
			return -1;
		}
		return -1;
	}
}

int MPLSAddressClassifier::convertL3toL2(int oIface, int oLabel, int LIBptr)
{
	int iLabel= -1;
	int ptr;
              
	while (oLabel >= 0) {
		/* penultimate hop */
		if (oLabel == 0)  {
			/* no operation */
			trace("U",size_, iLabel, "Push(penultimate)", 
			      oIface, oLabel, ttl_);
		} else {
			/* push operation in ingerss LSR */
			PI_.shimhdr_ = push(PI_.shimhdr_,oLabel);
			trace("U",size_, iLabel, "Push(ingress)", 
			      oIface, oLabel, ttl_);
		}
		if (LIBptr >= 0) {
			/* stack operation */
			iLabel = oLabel;
			ptr = LIBptr;
			LIBlookup(ptr, oIface, oLabel, LIBptr);
		} else
			break;
	}

	if (oLabel < 0) {
		if (size_ > 0)
			PI_.shimhdr_ = DelAllShimHeader(PI_.shimhdr_);
		trace("U",size_, iLabel , "L3(errorLabel)", -1,-1, -1);
		return MPLS_GOTO_L3;
	}
	if (oIface < 0) {  
		PI_.shimhdr_ = DelAllShimHeader(PI_.shimhdr_);
		trace("U", size_, iLabel, "L3(errorOIF)", -1 , -1, -1);
		return MPLS_GOTO_L3;
	} else
		// Guaranteed returned oIface is >= 0 or MPLS_GOTO_L3
		return oIface;
}

int MPLSAddressClassifier::convertL2toL2(int iLabel, int oIface, 
					 int oLabel, int LIBptr)
{
	int  ttl = PI_.shimhdr_->ttl_;
	int  ptr;

	// push(stack) operation after swap or pop
	if (oLabel == 0) {
		// in penultimate hop
		PI_.shimhdr_ = pop(PI_.shimhdr_);
		trace("L", size_, iLabel, "Pop(penultimate)", 
		      oIface, oLabel, ttl);
	} else if (oLabel > 0) {
		// swap operation 
		swap(PI_.shimhdr_,oLabel);
		trace("L", size_, iLabel, "Swap", oIface, oLabel, ttl);
	} else {
		// Errored Label
		PI_.shimhdr_ = DelAllShimHeader(PI_.shimhdr_);
		trace("L", size_, iLabel, "L3(errorLabel)", -1, -1, -1);
		return MPLS_GOTO_L3;
	}
	while (LIBptr >= 0) {
		// stack operation
		iLabel= oLabel;
		ptr = LIBptr;
		LIBlookup(ptr, oIface, oLabel, LIBptr);
		PI_.shimhdr_ = push(PI_.shimhdr_,oLabel);
		trace("L", size_, iLabel, "Push(tunnel)", 
		      oIface, oLabel, ttl_);
	}   
	if (oIface < 0) {  
		if (size_ > 0)
			PI_.shimhdr_ = DelAllShimHeader(PI_.shimhdr_);
		trace("L",size_, iLabel , "L3(errorOIf)", -1 , -1, -1);
		return MPLS_GOTO_L3;
	}
	// Guaranteed returned oIface >= 0
	return oIface;
}

// Process unlabeled packet
int MPLSAddressClassifier::processIP()
{
	int oIface,oLabel,LIBptr;
	int iLabel = -1;

	// Insert code to manipulate PHB
	if (PFTlookup(PI_.dst_.addr_, PI_.phb_, oIface, oLabel, LIBptr) == 0)
		// Push operation
		return convertL3toL2(oIface,oLabel,LIBptr);

	// L3 forwarding
	// Traffic-driven, triggered by MPLS switch
	if (data_driven_) 
		Tcl::instance().evalf("%s ldp-trigger-by-switch %d", 
				      name(), PI_.dst_.addr_);
	trace("U", size_, iLabel, "L3", -1, -1, -1);               
	return MPLS_GOTO_L3;
}

// Process labeled packet
int MPLSAddressClassifier::processLabelP()
{
	int oIface,oLabel,LIBptr;
	int iLabel = PI_.shimhdr_->label_;

	PI_.shimhdr_ = checkTTL(PI_.shimhdr_);

	if (size_ == 0)
		// TTL check
		return MPLS_GOTO_L3;

	// Label swapping operation 
	if (LIBlookup(-1, iLabel, oIface, oLabel, LIBptr) == 0)
		return convertL2toL2(iLabel,oIface,oLabel,LIBptr);

	PI_.shimhdr_ = DelAllShimHeader(PI_.shimhdr_);
	trace("L",size_, iLabel,"L3(errorLabel)", -1, -1, -1);
	return ( MPLS_GOTO_L3 );
}

int MPLSAddressClassifier::MPLSclassify(Packet* p)
{
	GetIPInfo(p, PI_.dst_, PI_.phb_, PI_.srcnode_);
	PI_.shimhdr_ = GetShimHeader(p);
   
	// XXX Using header size to determine if this is a MPLS-labeled packet
	// is a very bad method. We should have some explicit flag that labels
	// every packet; this flag will only be set on for every MPLS-labeled
	// packet. This can be done by a bitmap field in the common header.
	if (size_ == 0) 
		// Unlabeled packet
		return processIP();

	// Labeled packet 
	int ret = processLabelP();
	if (ret == MPLS_GOTO_L3) {  
		PI_.shimhdr_ = GetShimHeader(p);
		return processIP();
	}
	return ret;   
}

hdr_mpls *MPLSAddressClassifier::checkTTL(hdr_mpls *shimhdr)
{
	shimhdr->ttl_--;
	int ttl   = shimhdr->ttl_;
	int iLabel= shimhdr->label_;
   
	if (ttl == 0) {
		shimhdr = DelAllShimHeader(shimhdr);
		trace("L", size_, iLabel, "L3(TTL=0)", -1, -1, ttl);
	}
	return shimhdr;
}

void MPLSAddressClassifier::GetIPInfo(Packet* p, ns_addr_t &dst,
				      int &phb, int &srcnode)
{
	hdr_ip* iphdr = hdr_ip::access(p);
	dst = iphdr->dst_;
	srcnode = iphdr->src_.addr_;
	phb = MPLS_DEFAULT_PHB;
}

hdr_mpls *MPLSAddressClassifier::GetShimHeader(Packet* p)
{
	hdr_mpls *shimhdr = hdr_mpls::access(p);
	size_ = 0;
	if ((shimhdr->label_ == 0) && (shimhdr->bflag_ == 0) && 
	    (shimhdr->ttl_ == 0)) {
		shimhdr->bflag_ = -1;
		shimhdr->label_ = -1;
		shimhdr->ttl_ = -1;
		shimhdr->top_    = shimhdr;
		shimhdr->nexthdr_= shimhdr;
	} else {
		while (shimhdr->top_ != shimhdr) { 
			shimhdr = shimhdr->top_;
			size_ += 4;
		}
	} 
	return shimhdr;
}

hdr_mpls *MPLSAddressClassifier::DelAllShimHeader(hdr_mpls *shimhdr)
{
	while (shimhdr->bflag_ != -1)
		shimhdr = pop(shimhdr);
	return(shimhdr);
}

hdr_mpls *MPLSAddressClassifier::push(hdr_mpls *shimhdr, int oLabel)
{
	hdr_mpls *newhdr;

	newhdr = (hdr_mpls *) malloc( sizeof(hdr_mpls) );
	newhdr->label_ = oLabel;
	newhdr->bflag_ = 1;
	newhdr->ttl_ = ttl_;
	newhdr->top_  = newhdr;
	newhdr->nexthdr_ = shimhdr;

	shimhdr->top_ = newhdr;
	size_ += 4;
    
	return newhdr;
}

hdr_mpls *MPLSAddressClassifier::pop(hdr_mpls *shimhdr)
{
	shimhdr = shimhdr->nexthdr_;
	free(shimhdr->top_);
	shimhdr->top_ = shimhdr;
	size_ -= 4;
	return shimhdr;
}

void MPLSAddressClassifier::install(int slot, NsObject *target) {
	Tcl& tcl = Tcl::instance();
	if ((slot >= 0) && (slot < nslot_) && 
	    (slot_[slot] != NULL)) {
		if (strcmp(slot_[slot]->name(), target->name()) != 0)
			tcl.evalf("%s routing-update %d %.15g",
				  name(), slot,
				  Scheduler::instance().clock());
		else
			tcl.evalf("%s routing-nochange %d %.15g",
				  name(), slot,
				  Scheduler::instance().clock());
	} else
		tcl.evalf("%s routing-new %d %.15g",
			  name(), slot,
			  Scheduler::instance().clock());
	Classifier::install(slot,target);
}

int MPLSAddressClassifier::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "control-driven?") == 0) {
			tcl.resultf("%d", control_driven_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "data-driven?") == 0) {
			tcl.resultf("%d", data_driven_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "enable-control-driven") == 0) {
			// XXX data-driven and control-driven triggers are
			// exclusive 
			control_driven_ = 1;
			data_driven_ = 0;
			return (TCL_OK);
		} else if (strcmp(argv[1], "enable-data-driven") == 0) {
			control_driven_ = 0;
			data_driven_ = 1;
			return (TCL_OK);
		}
	} else if (argc == 3) {      
		if (strcmp(argv[1], "PFTdump") == 0) {
			// <classifier> PFTdump nodeid*/
			PFTdump(argv[2]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "ERBdump") == 0) {
			ERBdump(argv[2]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "LIBdump") == 0) {
			LIBdump(argv[2]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "get-fec-for-lspid") == 0) {
			// <classifier get-fec-for-lspid LSPid
			int LSPid  = atoi(argv[2]);
			int LIBptr = -1;
			int ERBnb;
			ERBnb = ERBlocate(LSPid, -1, LIBptr);
			if (ERBnb >= 0)
				tcl.resultf("%d", ERB_.Entry_[ERBnb].FEC_);
			else   
				tcl.result("-1");
			return (TCL_OK);
		} 
	} else if (argc == 4) {
		if (strcmp(argv[1], "exist-fec") == 0) {
			// <classifier> exist-fec <fec> <phb>
			int LIBptr;
			int PFTnb = PFTlocate(atoi(argv[2]), atoi(argv[3]),
					      LIBptr);         
			if (PFTnb > -1)
				tcl.result("1");
			else
				tcl.result("-1");
			return (TCL_OK);
		}
		int LIBptr = -1;
		int iLabel, oLabel, iIface, oIface;
		int fec   = atoi(argv[2]);
		int LSPid = atoi(argv[3]);
		int PHB   = LSPid;
		if (LSPid < 0)     // topology-based LSP
			PFTlocate(fec,PHB, LIBptr);
		else               // ER-LSP
			ERBlocate(LSPid,fec, LIBptr);

		if (strcmp(argv[1], "GetInIface") == 0) {
			// <classifier> GetInIface <FEC> <LSPid>
			if (LIBptr > -1) {
				LIBgetIncoming(LIBptr,iIface,iLabel);
				tcl.resultf("%d", iIface);
			} else
				tcl.result("-1");
			return (TCL_OK);
		} else if (strcmp(argv[1], "GetInLabel") == 0) {
			// <classifier> GetInLabel <FEC> <LSPid>
			if (LIBptr > -1) {
				LIBgetIncoming(LIBptr,iIface,iLabel);
				tcl.resultf("%d", iLabel);
			} else 
				tcl.result("-1");
			return (TCL_OK);
		} else if (strcmp(argv[1], "GetOutIface") == 0) {
			// <classifier> GetOutIface <FEC> <phb/LSPid>
			if (LIBptr > -1) {
				LIBlookup(LIBptr, oIface, oLabel, LIBptr);
				tcl.resultf("%d", oIface);
			} else
				tcl.result("-1");
			return (TCL_OK);
		} else if (strcmp(argv[1], "GetOutLabel") == 0) {
			// <classifier> GetOutLabel <FEC> <phb/LSPid>
			if (LIBptr > -1) {
				LIBlookup(LIBptr, oIface, oLabel, LIBptr);
				tcl.resultf("%d", oLabel);
			} else
				tcl.result("-1");
			return (TCL_OK);
		} else if (strcmp(argv[1], "install") == 0) { 
			int slot = atoi(argv[2]);
			//if ((slot >= 0) && (slot < nslot_) && 
			//  (slot_[slot] != NULL)) {
			//	if (strcmp(slot_[slot]->name(),argv[3]) != 0)
			//		tcl.evalf("%s routing-update %s %.15g",
			//			  name(), argv[2],
			//			Scheduler::instance().clock());
			//	else
			//	      tcl.evalf("%s routing-nochange %s %.15g",
			//			name(), argv[2],
			//			Scheduler::instance().clock());
			//} else
			//	tcl.evalf("%s routing-new %s %.15g",
			//		  name(), argv[2],
			//		  Scheduler::instance().clock());
			// Then the control is passed on the the base
			// address classifier!
			//return AddressClassifier::command(argc, argv);
			//not a good idea since it would start an infinite loop incase MPLSAddressClassifier::install is defined; 
			//so call Classifier::install explicitly.
			NsObject* target = (NsObject*)TclObject::lookup(argv[3]);
			//Classifier::install(slot, target);

			install(slot,target);
			return (TCL_OK);
		}
	} else if (argc == 5) {      
		if (strcmp(argv[1], "ErLspBinding") == 0) {
			// <classifier> ErLspBinding <FEC> <PHB> <lspid>
			int addr   = atoi(argv[2]);
			int PHB    = atoi(argv[3]);
			int LSPid  = atoi(argv[4]);
			if ( !ErLspBinding(addr, PHB,-1, LSPid) )
				tcl.result("1");
			else     
				tcl.result("-1");
			return (TCL_OK);
		}
	} else if (argc == 6) {
		// <classifier> ErLspStacking fec0 erlspid0 fec erfecid
		if (strcmp(argv[1], "ErLspStacking") == 0) {
			int erfec0   = atoi(argv[2]);
			int erlspid0 = atoi(argv[3]);
			int erfec    = atoi(argv[4]);
			int erlspid  = atoi(argv[5]);
			if (ErLspStacking(erfec0,erlspid0,erfec,erlspid) == 0)
				tcl.result("1");
			else     
				tcl.result("-1");
			return (TCL_OK);
		} else if (strcmp(argv[1], "FlowAggregation") == 0) {
			// <classifier> FlowAggregation <fineFEC> <finePHB>
			//              <coarseFEC> <coarsePHB>
			int fineaddr   = atoi(argv[2]);
			int finePHB    = atoi(argv[3]);
			int coarseaddr = atoi(argv[4]);
			int coarsePHB  = atoi(argv[5]);
			if (FlowAggregation(fineaddr, finePHB, coarseaddr,
					    coarsePHB) == 0)
				tcl.result("1");
			else     
				tcl.result("-1");
			return (TCL_OK);
		} else if (strcmp(argv[1], "aPathBinding") == 0) {
			// <classifier> aPathBinding <FEC> <PHB> 
			// <erFEC> <LSPid>
			int FEC   = atoi(argv[2]);
			int PHB   = atoi(argv[3]);
			int erFEC = atoi(argv[4]);
			int LSPid = atoi(argv[5]);
			if (aPathBinding(FEC, PHB, erFEC, LSPid) == 0)
				tcl.result("1");
			else     
				tcl.result("-1");
			return (TCL_OK);
		}
	} else if (argc == 8) {      
		int addr  = atoi(argv[2]);
		int LSPid = atoi(argv[3]);
		int LIBptr;
		if (LSPid == MPLS_DEFAULT_PHB)  {
			// topology-based LSP
			int PHB   = LSPid;
			int PFTnb = PFTlocate(addr,PHB,LIBptr);
			if (strcmp(argv[1], "LSPrelease") == 0) {
				if (PFTnb >= 0) {
					// PFT entry exist
					LIBupdate(LIBptr, atoi(argv[4]), 
						  atoi(argv[5]), atoi(argv[6]),
						  atoi(argv[7]));
					if (LIBisdeleted(LIBptr) == 0)
						PFTdeleteLIBptr(LIBptr);
				}
				return (TCL_OK);
			} else if (strcmp(argv[1], "LSPsetup") == 0) {
				if (PFTnb < 0) {
					// PFT entry not exist
					int ptr = LIBinsert(atoi(argv[4]),
							    atoi(argv[5]),
							    atoi(argv[6]),
							    atoi(argv[7]));
					if (ptr > -1) {
						PFTinsert(addr, 
							  MPLS_DEFAULT_PHB,
							  ptr);
						return (TCL_OK);
					} else
						return (TCL_ERROR);
				}
				// PFTnb >= 0
				// PFT entry already exist
				if (LIBptr <= -1) {
					int ptr = LIBinsert(atoi(argv[4]),
							    atoi(argv[5]),
							    atoi(argv[6]),
							    atoi(argv[7]));
					if (ptr > -1) {
						PFTupdate(PFTnb, ptr);
						return (TCL_OK);
					} else
						return (TCL_ERROR);	
				}
				// LIBptr > -1
				// Check whether or not altanative path setup
				if (!((enable_reroute_ == 1) &&
				      (LIB_.Entry_[LIBptr].oIface_ > -1) && 
				      (reroute_option_ == MPLS_MAKENEWLSP) &&
				      (atoi(argv[6]) > -1) && 
				      (is_link_down(LIB_.Entry_[LIBptr].oIface_))
				      )) { 
					LIBupdate(LIBptr, atoi(argv[4]),
						  atoi(argv[5]), atoi(argv[6]),
						  atoi(argv[7]));
					return (TCL_OK);
				}
				PFT_.Entry_[PFTnb].aPATHptr_ = 
					LIBinsert(atoi(argv[4]), atoi(argv[5]),
						  atoi(argv[6]),atoi(argv[7]));
				for (int i=0; i < LIB_.NB_; i++) {
					if (LIB_.Entry_[i].oIface_!=atoi(argv[2]))
						continue;
					for (int k=0; k<PFT_.NB_; k++) {
						if (PFT_.Entry_[k].LIBptr_ != i)
							continue;
						PFT_.Entry_[k].aPATHptr_ = 
						  PFT_.Entry_[PFTnb].aPATHptr_;
					}
				}
				return (TCL_OK);
			}
		} else {
			// ER-LSP
			int ERBnb = ERBlocate(LSPid,addr,LIBptr);
			if (strcmp(argv[1], "LSPrelease") == 0) {
				if ( ERBnb >= 0 ) {
					// ERB entry exist
					LIBupdate(LIBptr, atoi(argv[4]),
						  atoi(argv[5]), atoi(argv[6]),
						  atoi(argv[7]));
					if (LIBisdeleted(LIBptr) == 0) {
						ERBdelete(ERBnb);
						PFTdeleteLIBptr(LIBptr);
					}
				}
				return (TCL_OK);
			} else if (strcmp(argv[1], "LSPsetup") == 0) {
				if (ERBnb < 0) {
					// ERB entry not exist
					int ptr = LIBinsert(atoi(argv[4]),
							    atoi(argv[5]),
							    atoi(argv[6]),
							    atoi(argv[7]));
					if (ptr > -1) {
						ERBinsert(LSPid,addr,ptr);
						return (TCL_OK);
					} else
						return (TCL_ERROR);
				} 
				// ERBnb >= 0
				// ERB entry already exist
				if (LIBptr > -1)
					LIBupdate(LIBptr, atoi(argv[4]), atoi(argv[5]),
						  atoi(argv[6]), atoi(argv[7]));
				else {
					int ptr = 
						LIBinsert(atoi(argv[4]),atoi(argv[5]),
							  atoi(argv[6]),atoi(argv[7]));
					if (ptr > -1)
						ERBupdate(ERBnb, ptr);
					else
						return (TCL_ERROR);
				}
				return (TCL_OK);
			}
		}
	}
  
	return (AddressClassifier::command(argc, argv));
}

//--------------------------------------------------
// PFT(Partial Forwarding Table)
//--------------------------------------------------

void MPLSAddressClassifier::PFTinsert(int FEC, int PHB, int LIBptr)
{
	PFT_.Entry_[PFT_.NB_].FEC_    = FEC;
	PFT_.Entry_[PFT_.NB_].PHB_    = PHB;
	PFT_.Entry_[PFT_.NB_].LIBptr_ = LIBptr;
	PFT_.Entry_[PFT_.NB_].aPATHptr_  = -1;
	PFT_.NB_++;
}

void MPLSAddressClassifier::PFTdelete(int entrynb)
{
	PFT_.Entry_[entrynb].FEC_    = -1;
	PFT_.Entry_[entrynb].PHB_    = -1;
	PFT_.Entry_[entrynb].LIBptr_ = -1;
}

void MPLSAddressClassifier::PFTdeleteLIBptr(int LIBptr)
{
	for (int i = 0; i < PFT_.NB_; i++) {
		if (PFT_.Entry_[i].LIBptr_ != LIBptr)
			continue; 
		PFT_.Entry_[i].FEC_    = -1;
		PFT_.Entry_[i].PHB_    = -1;
		PFT_.Entry_[i].LIBptr_ = -1;
	}  
}

void MPLSAddressClassifier::PFTupdate(int entrynb, int LIBptr)
{
	PFT_.Entry_[entrynb].LIBptr_ = LIBptr;
}

int MPLSAddressClassifier::PFTlocate(int FEC, int PHB, int &LIBptr)
{
	LIBptr = -1;
	if (FEC < 0) 
		return -1;
	for (int i = 0; i < PFT_.NB_; i++)
		if ((PFT_.Entry_[i].FEC_ == FEC) && (PFT_.Entry_[i].PHB_ == PHB)) {
			LIBptr = PFT_.Entry_[i].LIBptr_;
			return i;
		}
	return -1;
}

int MPLSAddressClassifier::PFTlookup(int FEC, int PHB, int &oIface, 
				     int &oLabel, int &LIBptr)
{
	oIface = oLabel = LIBptr = -1;
	if (FEC < 0) 
		return -1;
	for (int i = 0; i < PFT_.NB_; i++)
		if ((PFT_.Entry_[i].FEC_ == FEC) && (PFT_.Entry_[i].PHB_ == PHB))
			return LIBlookup(PFT_.Entry_[i].LIBptr_,
					 oIface, oLabel, LIBptr);
	return -1;
}

void MPLSAddressClassifier::PFTdump(const char *id)
{
	fflush(stdout);
	printf("      --) ___PFT dump___ [node: %s] (--\n", id);
	printf("---------------------------------------------\n");
	printf("     FEC       PHB       LIBptr  AltanativePath\n");
	for (int i = 0; i < PFT_.NB_; i++) {
		if (PFT_.Entry_[i].FEC_ == -1) 
			continue;
		printf("     %d    ", PFT_.Entry_[i].FEC_);
		printf("     %d    ", PFT_.Entry_[i].PHB_);
		printf("     %d    ", PFT_.Entry_[i].LIBptr_);
		printf("     %d\n", PFT_.Entry_[i].aPATHptr_);
	}
	printf("\n");
}

//--------------------------------------------------
// ER-LSP Table
//--------------------------------------------------

void MPLSAddressClassifier::ERBinsert(int LSPid, int FEC, int LIBptr)
{
	ERB_.Entry_[ERB_.NB_].LSPid_  = LSPid;
	ERB_.Entry_[ERB_.NB_].FEC_    = FEC;
	ERB_.Entry_[ERB_.NB_].LIBptr_ = LIBptr;
	ERB_.NB_++;
}
 
void MPLSAddressClassifier::ERBdelete(int entrynb)
{
	ERB_.Entry_[entrynb].FEC_    = -1;
	ERB_.Entry_[entrynb].LSPid_  = -1;
	ERB_.Entry_[entrynb].LIBptr_ = -1;
}

void MPLSAddressClassifier::ERBupdate(int entrynb, int LIBptr)
{
	ERB_.Entry_[entrynb].LIBptr_ = LIBptr;
}

int  MPLSAddressClassifier::ERBlocate(int LSPid, int , int &LIBptr)
{
	LIBptr = -1;
	for (int i = 0; i < ERB_.NB_; i++)
		if (ERB_.Entry_[i].LSPid_ == LSPid) {
			LIBptr = ERB_.Entry_[i].LIBptr_;
			return(i);
		}
	return -1;
}

void MPLSAddressClassifier::ERBdump(const char *id)
{
	fflush(stdout);
	printf("      --) ___ERB dump___ [node: %s] (--\n", id);
	printf("---------------------------------------------\n");
	printf("     FEC       LSPid      LIBptr\n");
	for (int i = 0; i < ERB_.NB_; i++) {
		if (ERB_.Entry_[i].FEC_ == -1) 
			continue;
		printf("     %d    ", ERB_.Entry_[i].FEC_);
		printf("     %d    ", ERB_.Entry_[i].LSPid_);
		printf("     %d\n", ERB_.Entry_[i].LIBptr_);
	}
	printf("\n");
}

//--------------------------------------------------
// LIB (Label Information Base)
//--------------------------------------------------
int MPLSAddressClassifier::LIBinsert(int iIface, int iLabel, 
				     int oIface, int oLabel)
{
	if (LIB_.NB_ == MPLS_MaxLIBEntryNB - 1) {
		fprintf(stderr, 
			"Error in LIBinstall: higher than MaxLIBEntryNB\n");
		return -1;
	}

	LIB_.Entry_[LIB_.NB_].iIface_ = -1;
	LIB_.Entry_[LIB_.NB_].iLabel_ = -1;
	LIB_.Entry_[LIB_.NB_].oIface_ = -1;
	LIB_.Entry_[LIB_.NB_].oLabel_ = -1;

	if (iIface < 0) iIface = -1;
	if (iLabel < 0) iLabel = -1;
	if (oIface < 0) oIface = -1;
	if (oLabel < 0) oLabel = -1;

	LIB_.Entry_[LIB_.NB_].iIface_  = iIface;
	LIB_.Entry_[LIB_.NB_].iLabel_  = iLabel;
	LIB_.Entry_[LIB_.NB_].oIface_  = oIface;
	LIB_.Entry_[LIB_.NB_].oLabel_  = oLabel;
	LIB_.Entry_[LIB_.NB_].LIBptr_  = -1;

	LIB_.NB_++;
	return(LIB_.NB_-1);
}

int MPLSAddressClassifier::LIBisdeleted(int entrynb)
{
	if ((LIB_.Entry_[entrynb].iIface_ == -1) &&
	    (LIB_.Entry_[entrynb].iLabel_ == -1) &&
	    (LIB_.Entry_[entrynb].oIface_ == -1) &&
	    (LIB_.Entry_[entrynb].oLabel_ == -1)) {

		LIB_.Entry_[entrynb].LIBptr_ = -1;
		// delete a entry referencing self in LIB
		for (int i = 0; i < LIB_.NB_; i++)
			if ((LIB_.Entry_[i].LIBptr_ == entrynb))
				LIB_.Entry_[i].LIBptr_ = -1;
		return 0;
	}
	return -1;
}

void MPLSAddressClassifier::LIBupdate(int entrynb, int iIface, int iLabel, 
				      int oIface, int oLabel)
{
	if (iIface != MPLS_DONTCARE)
		LIB_.Entry_[entrynb].iIface_ = iIface;
	if (iLabel != MPLS_DONTCARE)    
		LIB_.Entry_[entrynb].iLabel_ = iLabel;
	if (oIface != MPLS_DONTCARE)
		LIB_.Entry_[entrynb].oIface_ = oIface;
	if (oLabel != MPLS_DONTCARE)
		LIB_.Entry_[entrynb].oLabel_ = oLabel;
}

int MPLSAddressClassifier::LIBlookup(int entrynb, int &oIface, 
				      int &oLabel, int &LIBptr)
{
	oIface = oLabel = LIBptr = -1;

	if (entrynb < 0)
		return -1;
	oIface = LIB_.Entry_[entrynb].oIface_;
	oLabel = LIB_.Entry_[entrynb].oLabel_;
	LIBptr = LIB_.Entry_[entrynb].LIBptr_;
	return 0;
}

int MPLSAddressClassifier::LIBlookup(int , int iLabel, int &oIface, 
				     int &oLabel, int &LIBptr)
{
	oIface = oLabel = LIBptr = -1;
	if (iLabel < 0)
		return -1;
	for (int i = 0; i < LIB_.NB_; i++)
		if ((LIB_.Entry_[i].iLabel_ == iLabel)) {
			oIface = LIB_.Entry_[i].oIface_;
			oLabel = LIB_.Entry_[i].oLabel_;
			LIBptr = LIB_.Entry_[i].LIBptr_;
			return 0;
		} 
	return -1;
}

int MPLSAddressClassifier::LIBgetIncoming(int entrynb, int &iIface, 
					  int &iLabel)
{
	if (entrynb < 0)
		return -1;
	iIface = LIB_.Entry_[entrynb].iIface_;
	iLabel = LIB_.Entry_[entrynb].iLabel_;
	return 0;
}

void MPLSAddressClassifier::LIBdump(const char *id)
{
	fflush(stdout);
	printf("    ___LIB dump___ [node: %s]\n", id);
	printf("---------------------------------------------\n");
	printf("       #       iIface     iLabel      oIface     oLabel    LIBptr\n");
	for (int i = 0; i < LIB_.NB_; i++) {
		if (!LIBisdeleted(i))
			continue;
		printf("     %d: ", i);
		printf("     %d    ", LIB_.Entry_[i].iIface_);
		printf("     %d  ", LIB_.Entry_[i].iLabel_);
		printf("     %d    ", LIB_.Entry_[i].oIface_);
		printf("     %d    ", LIB_.Entry_[i].oLabel_);
		printf("     %d\n", LIB_.Entry_[i].LIBptr_);
	}
	printf("\n");
}

//--------------------------------------------------
// MPLS applications
//--------------------------------------------------

int MPLSAddressClassifier::ErLspStacking(int erFEC0,int erLSPid0, 
					 int erFEC, int erLSPid)
{
	int erLIBptr0  =-1, erLIBptr  =-1;

	if ((ERBlocate(erLSPid0,erFEC0,erLIBptr0) < 0) || (erLIBptr0 < 0))
		return -1;
	if ((ERBlocate(erLSPid,erFEC,erLIBptr) < 0) || (erLIBptr < 0))
		return -1;
	LIB_.Entry_[erLIBptr0].LIBptr_ = erLIBptr;
	return 0;
}

int MPLSAddressClassifier::ErLspBinding(int FEC,int PHB, int erFEC, int LSPid)
{
	int LIBptr=-1;
	int erLIBptr=-1;

	if ((ERBlocate(LSPid, erFEC, erLIBptr) < 0) || (erLIBptr < 0))
		return -1;

	int PFTnb = PFTlocate(FEC,PHB, LIBptr);
	if ((PFTnb < 0))
		PFTinsert(FEC,PHB, erLIBptr);
	else {
		if (LIBptr < 0)
			PFTupdate(PFTnb, LIBptr);
		else {  
			int i = LIBptr;
			while (i < 0) {
				LIBptr = i;;
				i = LIB_.Entry_[LIBptr].LIBptr_;
			}
			LIB_.Entry_[LIBptr].LIBptr_ = erLIBptr;
		}
	}
	return 0;
}

int MPLSAddressClassifier::FlowAggregation(int fineFEC, int finePHB, 
					   int coarseFEC, int coarsePHB)
{
	int fLIBptr=-1;
	int cLIBptr=-1;

	if ((PFTlocate(coarseFEC,coarsePHB, cLIBptr) < 0) || (cLIBptr < 0))
		return -1;

	int PFTnb = PFTlocate(fineFEC,finePHB, fLIBptr);
	if ((PFTnb < 0))
		PFTinsert(fineFEC,finePHB, cLIBptr);
	else {
		if (fLIBptr < 0)
			PFTupdate(PFTnb, cLIBptr);
		else {
			int i=fLIBptr;
			while (i < 0) {
				fLIBptr = i;;
				i = LIB_.Entry_[fLIBptr].LIBptr_;
			}
			LIB_.Entry_[fLIBptr].LIBptr_ = cLIBptr;
		}
	}
        return 0;
}

int MPLSAddressClassifier::aPathBinding(int FEC, int PHB, int erFEC, int LSPid)
{
	int entrynb,tmp,erLIBptr;	
      
	entrynb = PFTlocate(FEC,PHB,tmp);
	if ((entrynb < 0) || (ERBlocate(LSPid,erFEC,erLIBptr) < 0))
		return -1;
	PFT_.Entry_[entrynb].aPATHptr_ = erLIBptr;
	return 0;
}

int MPLSAddressClassifier::aPathLookup(int FEC,int PHB, int &oIface,
				       int &oLabel, int &LIBptr)
{
	oIface = oLabel = LIBptr = -1;

	if (FEC < 0) 
		return -1;
	for (int i = 0; i < PFT_.NB_; i++)
		if ((PFT_.Entry_[i].FEC_ == FEC) && 
		    (PFT_.Entry_[i].PHB_ == PHB))
			return LIBlookup(PFT_.Entry_[i].aPATHptr_,
					 oIface, oLabel, LIBptr);
	return -1;
}

void MPLSAddressClassifier::trace(char *ptype, int psize, int ilabel, 
				  char *op, int oiface, int olabel, int ttl)
{
	if (trace_mpls_ != 1)
		return;
	Tcl::instance().evalf("%s trace-packet-switching " TIME_FORMAT 
			      " %d %d %s %d %s %d %d %d %d",
			      name(),
			      Scheduler::instance().clock(),
			      PI_.srcnode_,
			      PI_.dst_.addr_,
			      ptype,
			      ilabel,
			      op,
			      oiface,
			      olabel,
			      ttl,
			      psize);
}
