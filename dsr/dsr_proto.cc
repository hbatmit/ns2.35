

/*
 * dsr_proto.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: dsr_proto.cc,v 1.4 2010/03/08 05:54:50 tom_henderson Exp $
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
//
// Ported from CMU/Monarch's code, appropriate copyright applies.  

/* -*- c++ -*- 

   dsr_proto.cc
   the DSR routing protocol agent
   
   */


#include <packet.h>
#include <object.h>
#include <agent.h>
#include <trace.h>
#include <ip.h>
#include "routecache.h"
#include "dsr_proto.h"

static class DSRProtoClass : public TclClass {
public:
        DSRProtoClass() : TclClass("Agent/DSRProto") {}
        TclObject* create(int, const char*const*) {
                return (new DSRProto);
        }
} class_DSRProto;


DSRProto::DSRProto() : Agent(PT_DSR)
{
  // dst_ = (IP_BROADCAST << 8) | RT_PORT;
  dst_.addr_ = IP_BROADCAST;
  dst_.port_ = RT_PORT;
}


void
DSRProto::recv(Packet* , Handler* )
{
  
}

void
DSRProto::testinit()
{
  //struct hdr_sr hsr;
  hdr_sr hsr;
  
  if (net_id == ID(1,::IP))
    {
      printf("adding route to 1\n");
      hsr.init();
      hsr.append_addr( 1<<8, NS_AF_INET );
      hsr.append_addr( 2<<8, NS_AF_INET );
      hsr.append_addr( 3<<8, NS_AF_INET );
      hsr.append_addr( 4<<8, NS_AF_INET );
      
      routecache->addRoute(Path(hsr.addrs(), hsr.num_addrs()), 0.0, ID(1,::IP));
    }
  
  if (net_id == ID(3,::IP))
    {
      printf("adding route to 3\n");
      hsr.init();
      hsr.append_addr( 3<<8, NS_AF_INET );
      hsr.append_addr( 2<<8, NS_AF_INET );
      hsr.append_addr( 1<<8, NS_AF_INET );
      
      routecache->addRoute(Path(hsr.addrs(), hsr.num_addrs()), 0.0, ID(3,::IP));
    }
}


int
DSRProto::command(int argc, const char*const* argv)
{
  if(argc == 2) 
    {
      if (strcasecmp(argv[1], "testinit") == 0)
	{
	  testinit();
	  return TCL_OK;
	}
    }

  if(argc == 3) 
    {
      if(strcasecmp(argv[1], "tracetarget") == 0) {
	tracetarget = (Trace*) TclObject::lookup(argv[2]);
	if(tracetarget == 0)
	  return TCL_ERROR;
	return TCL_OK;
      }
      else if(strcasecmp(argv[1], "routecache") == 0) {
	routecache = (RouteCache*) TclObject::lookup(argv[2]);
	if(routecache == 0)
	  return TCL_ERROR;
	return TCL_OK;
      }
      else if (strcasecmp(argv[1], "ip-addr") == 0) {
	net_id = ID(atoi(argv[2]), ::IP);
	return TCL_OK;
      }
      else if(strcasecmp(argv[1], "mac-addr") == 0) {
	mac_id = ID(atoi(argv[2]), ::MAC);
      return TCL_OK;

      }
    }
  return Agent::command(argc, argv);
}

void 
DSRProto::noRouteForPacket(Packet *)
{

}
