
/*
 * add_sr.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: add_sr.cc,v 1.4 2010/03/08 05:54:50 tom_henderson Exp $
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
// Other copyrights might apply to parts of this software and are so
// noted when applicable.

/* 
   add_sr.cc
   add a compiled constant source route to a packet.
   for testing purposes
   Ported from CMU/Monarch's code, appropriate copyright applies.  
   */

#include <packet.h>
#include <ip.h>
#include "hdr_sr.h"

#include <connector.h>

class AddSR : public Connector {
public:
  void recv(Packet*, Handler* callback = 0);
  AddSR();

private:
  int off_ip_;
  int off_sr_;
  int off_ll_;
  int off_mac_;
};

static class AddSRClass : public TclClass {
public:
  AddSRClass() : TclClass("Connector/AddSR") {}
  TclObject* create(int, const char*const*) {
    return (new AddSR);
  }
} class_addsr;

AddSR::AddSR()
{
  bind("off_sr_", &off_sr_);
  bind("off_ll_", &off_ll_);
  bind("off_mac_", &off_mac_);
  bind("off_ip_", &off_ip_);
}

void
AddSR::recv(Packet* p, Handler* )
{
  hdr_sr *srh =  hdr_sr::access(p);

  srh->route_request() = 0;
  srh->num_addrs() = 0;
  srh->cur_addr() = 0;
  srh->valid() = 1;
  srh->append_addr( 1<<8, NS_AF_INET );
  srh->append_addr( 2<<8, NS_AF_INET );
  srh->append_addr( 3<<8, NS_AF_INET );
  printf(" added sr %s\n",srh->dump());
  send(p,0);
}
