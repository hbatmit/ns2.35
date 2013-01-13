/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * Copyright (C) 1997 by the University of Southern California
 * $Id: net-interface.h,v 1.3 2005/08/25 18:58:02 johnh Exp $
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
 * @(#) $Header: /cvsroot/nsnam/ns-2/common/net-interface.h,v 1.3 2005/08/25 18:58:02 johnh Exp $
 *
 * Ported by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
 */

#ifndef ns_net_int_h
#define ns_net_int_h

#include "connector.h"
#include "packet.h"
#include "lib/bsd-list.h"

class NetworkInterface;
LIST_HEAD(netint_head, NetworkInterface); // declare list head structure

class NetworkInterface : public Connector {
public:
	NetworkInterface() : intf_label_(UNKN_IFACE.value()) { /*NOTHING*/ }
	int command(int argc, const char*const* argv); 
	void recv(Packet* p, Handler* h);
	int32_t intf_label() { return intf_label_; } 
	// list of all networkinterfaces on a node
        inline void insertiface(struct netint_head *head) {
                LIST_INSERT_HEAD(head, this, iface_entry);
        } 
	NetworkInterface* nextiface(void) const { return iface_entry.le_next; }

protected:
	int32_t intf_label_;
	LIST_ENTRY(NetworkInterface) iface_entry;
};

#endif
