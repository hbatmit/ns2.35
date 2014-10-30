
/*
 * sr_forwarder.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: sr_forwarder.h,v 1.2 2005/08/25 18:58:05 johnh Exp $
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

// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
// Ported from CMU/Monarch's code, appropriate copyright applies.  
/* -*- c++ -*-
   
   sr_forwarder.h
   source route forwarder

   */

#ifndef _sr_forwarder_
#define _sr_forwarder_

#include <object.h>
#include <trace.h>

#include "dsr_proto.h"
#include "requesttable.h"
#include "routecache.h"

/* a source route forwarding object takes packets and if it has a SR
routing header it forwards the packets into target_ according to the
header.  Otherwise, it gives the packet to the noroute_ object in
hopes that it knows what to do with it. */

class SRForwarder : public NsObject {
public:
  SRForwarder();

protected:
  virtual int command(int argc, const char*const* argv);
  virtual void recv(Packet*, Handler* callback = 0);

  NsObject* target_;		/* where to send forwarded pkts */
  DSRProto* route_agent_;	        /* where to send unforwardable pkts */
  RouteCache* routecache_;	/* the routecache */

private:
  void handlePktWithoutSR(Packet *p);
  Trace *tracetarget;
  int off_mac_;
  int off_ll_;
  int off_ip_;
  int off_sr_;
};
#endif //_sr_forwarder_
