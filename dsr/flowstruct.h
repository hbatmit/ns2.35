
/*
 * flowstruct.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: flowstruct.h,v 1.3 2006/02/21 15:20:18 mahrenho Exp $
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

#ifndef __flow_table_h__
#define __flow_table_h__

extern "C" {
#include <stdio.h>
#include <stdarg.h>
}

#include "path.h"
#include "srpacket.h"
#include "hdr_sr.h"
//#include "net-if.h"
#include <phy.h>
#include "scheduler.h" // for timeout

// end_to_end count - 1, actually, so a misnomer...
#define END_TO_END_COUNT 	2
#define FLOW_TABLE_SIZE		3000
#define ARS_TABLE_SIZE          5

struct ARSTabEnt {
  int uid;
  u_int16_t fid;
  int hopsEarly; /* 0 overloads as invalid */
};

class ARSTable {
 public:
  ARSTable(int size_ = ARS_TABLE_SIZE);
  ~ARSTable();

  void insert(int uid, u_int16_t fid, int hopsEarly);
  int  findAndClear(int uid, u_int16_t fid);
 private:
  ARSTabEnt *table;
  int victim;
  int size;
};

struct DRTabEnt { /* Default Route Table Entry */
  nsaddr_t src;
  nsaddr_t dst;
  u_int16_t fid;
};

class DRTable {
  public:
    DRTable(int size_=FLOW_TABLE_SIZE);
    ~DRTable();
    bool find(nsaddr_t src, nsaddr_t dst, u_int16_t &flow);
    void insert(nsaddr_t src, nsaddr_t dst, u_int16_t flow);
    void flush(nsaddr_t src, nsaddr_t dst);
  private:
    int       size;
    int       maxSize;
    DRTabEnt *table;

    void grow();
};

struct TableEntry {
    // The following three are a key
    nsaddr_t   sourceIP ;	// Source IP Addresss
    nsaddr_t   destinationIP ;	// Destination IP Addresss
    u_int16_t 	flowId ;	// 16bit flow id

    // Ugly hack for supporting the "established end-to-end" concept
    int		count ;		// starts from 0 and when it reaches 
				// END_TO_END_COUNT it means that the 
				// flow has been established end to end.

    nsaddr_t	nextHop;	// According to the draft, this is a MUST.
				// Obviously, said info is also in sourceRoute,
				// but keeping it separate makes my life easier,
				// and does so for free. -- ych 5/5/01

    Time  	lastAdvRt; 	// Last time this route was "advertised"
				// advertisements are essentially source routed
				// packets.

    Time  	timeout ; 	// MUST : Timeout of this flowtable entry
    int		hopCount ; 	// MUST : Hop count
    int		expectedTTL ;	// MUST : Expected TTL
    bool	allowDefault ;	// MUST : If true then this flow
				// can be used as default if the 
				// source is this node and the 
				// flow ID is odd.
				// Default is 'false' 

    Path 	sourceRoute ;	// SHOULD : The complete source route.
				// Nodes not keeping complete source 
				// route information cannot
				// participate in Automatic Route
				// Shortening
};

class FlowTable
{
  public :

    FlowTable(int size_=FLOW_TABLE_SIZE);
    ~FlowTable();

    // Returns a pointer to the entry corresponding to
    // the index that has been passed.
    TableEntry &operator[](int index);

    // Returns the index number for that entry or -1 
    // if it is not found
    int find(nsaddr_t source, 
	     nsaddr_t destination, 
	     u_int16_t flow);

    // If there is an entry corresponding to the arguments of this
    // function then the index corresponding to that entry is
    // returned; returns -1 on error
    int find(nsaddr_t source, 
	     nsaddr_t destination, 
	     const Path& route);

    // Returns index number if successfully created, 
    // else returns -1 ( if entry already exists )
    // or if memory problem.
    int createEntry(nsaddr_t source, 
		    nsaddr_t destination, 
		    u_int16_t flow);

    // If there is a default flow id corresponding to the source and
    // destination addresses passed in the argument then return the index
    bool defaultFlow(nsaddr_t source, nsaddr_t destination, u_int16_t &flow);

    // Generates the next flow id keeping in mind that default flow
    // ids are odd and non-default ones are even.
    u_int16_t generateNextFlowId(nsaddr_t destination, 
				 bool allowDefault) ;

    // Purge all the flows which uses the link from 'from' to 'to'
    void noticeDeadLink(const ID& from, const ID& to) ;

    // promise not to hold indexes into flow table across cleanup()s.
    void cleanup();

    // set our net address, for noticeDeadLink()
    void setNetAddr(nsaddr_t net_id);

  private :

    TableEntry *table ;	// Actual flow state table
    int		size ;		// Number of entries in the table
    int 	maxSize ;	// Maximum possible size

    u_int16_t   counter;	// next flowid to give out
    nsaddr_t    net_addr;	// for noticeDeadLink()
    DRTable     DRTab;

    void grow();
};

#endif	// __flow_table_h__

