// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- 

/*
 * Copyright (C) 1997 by the University of Southern California
 * $Id: delaymodel.h,v 1.7 2005/08/25 18:58:10 johnh Exp $
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
 * Contributed by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
 * 
 * @(#) $Header: /cvsroot/nsnam/ns-2/queue/delaymodel.h,v 1.7 2005/08/25 18:58:10 johnh Exp $ (UCB)
 */

#ifndef ns_delaymodel_h
#define ns_delaymodel_h

#include "connector.h"
#include "ranvar.h"


class DelayModel : public Connector {
public:
	DelayModel();
	void recv(Packet*, Handler*);
	inline double txtime(Packet* p) {
		hdr_cmn *hdr = hdr_cmn::access(p);
		return bandwidth_ ? (hdr->size() * 8. / bandwidth_) : 0;
	}
	double bandwidth() const { return bandwidth_; }

protected:
	int command(int argc, const char*const* argv);
	RandomVariable* ranvar_;
	double bandwidth_;	/* bandwidth of underlying link (bits/sec) */
	//Event intr_;
};

#endif
