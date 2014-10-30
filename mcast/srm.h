/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * srm.h
 * Copyright (C) 1997 by the University of Southern California
 * $Id: srm.h,v 1.22 2005/09/18 23:33:33 tomh Exp $
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
//	Author:		Kannan Varadhan	<kannan@isi.edu>
//	Version Date:	Mon Jun 30 15:51:33 PDT 1997
//
// @(#) $Header: /cvsroot/nsnam/ns-2/mcast/srm.h,v 1.22 2005/09/18 23:33:33 tomh Exp $ (USC/ISI)
//

#ifndef ns_srm_h
#define ns_srm_h

#include <math.h>
#include <tcl.h>

#include "config.h"
//#include "heap.h"
#include "srm-state.h"
#include "srm-headers.h"

class SRMAgent : public Agent {
protected:
	int	dataCtr_;		/* # of data packets sent */
	int	sessCtr_;		/* # of session messages sent */
	int	packetSize_;		/* size of data messages for repr */
	SRMinfo* sip_;			/* Table of sender info */
	Tcl_HashTable*	siphash_;
	int	groupSize_;
	int seqno_;			/* Seqno for CBR packets */
	int app_fid_;
	packet_t app_type_;

	virtual void start() {
		int new_entry = 0;
		long key = addr();

		sip_->sender_   /* is itself */ = addr();
		sip_->distance_ /* to itself */ = 0.0;
		sip_->next_ = NULL;

		siphash_ = new Tcl_HashTable;
		Tcl_InitHashTable(siphash_, TCL_ONE_WORD_KEYS);
		Tcl_HashEntry* he = Tcl_CreateHashEntry(siphash_,
							(char*) key,
							&new_entry);
		Tcl_SetHashValue(he, (ClientData*)sip_);
		groupSize_++;
	}
	SRMinfo* get_state(int sender) {
		assert(siphash_);

		int new_entry = 0;
		long key = sender;
		Tcl_HashEntry* he = Tcl_CreateHashEntry(siphash_,
							(char*) key,
							&new_entry);
		if (new_entry) {
			groupSize_++;
			SRMinfo* tmp = new SRMinfo(sender);
			tmp->next_ = sip_->next_;
			sip_->next_ = tmp;
			Tcl_SetHashValue(he, (ClientData*)tmp);
		}
		return (SRMinfo*)Tcl_GetHashValue(he);
	}
	virtual void cleanup () {
		Tcl_DeleteHashTable(siphash_);
	}
	
	virtual void addExtendedHeaders(Packet*) {}
	virtual void parseExtendedHeaders(Packet*) {}
	virtual int request(SRMinfo* sp, int hi) {
		int miss = 0;
		if (sp->ldata_ >= hi)
			return miss;
		
		int maxsize = ((int)log10(hi + 1) + 2) * (hi - sp->ldata_);
				// 1 + log10(msgid) bytes for the msgid
				// msgid could be 0, if first pkt is lost.
				// 1 byte per msg separator
				// hi - sp->ldata_ msgs max missing
		char* msgids = new char[maxsize + 1];
		*msgids = '\0';
		for (int i = sp->ldata_ + 1; i <= hi; i++)
			if (! sp->ifReceived(i)) {
				(void) sprintf(msgids, "%s %d", msgids, i);
				miss++;
			}
		assert(miss);
		Tcl::instance().evalf("%s request %d %s", name_,
				      sp->sender_, msgids);
		delete[] msgids;
		return miss;
	}

	virtual void recv_data(int sender, int msgid, u_char* data);
	virtual void recv_repr(int round, int sender, int msgid, u_char* data);
	virtual void recv_rqst(int requestr, int round, int sender, int msgid);
	virtual void recv_sess(Packet*, int sessCtr, int* data);

	virtual void send_ctrl(int typ, int rnd, int sndr, int msgid, int sz);
	virtual void send_sess();
public:
	SRMAgent();
	virtual ~SRMAgent();
	virtual int command(int argc, const char*const* argv);
	virtual void recv(Packet* p, Handler* h);
        virtual void sendmsg(int nbytes, const char *flags = 0);
        virtual void send(int nbytes) { sendmsg(nbytes); }
};

class ASRMAgent : public SRMAgent {
	double pdistance_;
	int    requestor_;
public:
	ASRMAgent() {
		bind("pdistance_", &pdistance_);
		bind("requestor_", &requestor_);
	}
protected:
	virtual void addExtendedHeaders(Packet* p) {
		SRMinfo* sp;
		hdr_srm* sh = hdr_srm::access(p);
		hdr_asrm* seh = hdr_asrm::access(p);
		switch (sh->type()) {
		case SRM_RQST:
			sp = get_state(sh->sender());
			seh->distance() = sp->distance_;
			break;
		case SRM_REPR:
			sp = get_state(requestor_);
			seh->distance() = sp->distance_;
			break;
		case SRM_DATA:
		case SRM_SESS:
			seh->distance() = 0.;
			break;
		default:
			assert(0);
			/*NOTREACHED*/
		}
		SRMAgent::addExtendedHeaders(p);
	}
	virtual void parseExtendedHeaders(Packet* p) {
		SRMAgent::parseExtendedHeaders(p);
		hdr_asrm* seh = hdr_asrm::access(p);
		pdistance_ = seh->distance();
	}
};

#endif
