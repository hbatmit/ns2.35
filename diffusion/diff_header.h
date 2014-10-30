
/*
 * diff_header.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: diff_header.h,v 1.4 2005/08/25 18:58:03 johnh Exp $
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
//  Copyright (C) 1998 by Mingzhou Sun. All rights reserved.
//  This software is developed at Rensselaer Polytechnic Institute under 
//  DARPA grant No. F30602-97-C-0274
//  Redistribution and use in source and binary forms are permitted
//  provided that the above copyright notice and this paragraph are
//  duplicated in all such forms and that any documentation, advertising
//  materials, and other materials related to such distribution and use
//  acknowledge that the software was developed by Mingzhou Sun at the
//  Rensselaer  Polytechnic Institute.  The name of the University may not 
//  be used to endorse or promote products derived from this software 
//  without specific prior written permission.
//
// $Header: /cvsroot/nsnam/ns-2/diffusion/diff_header.h,v 1.4 2005/08/25 18:58:03 johnh Exp $

/******************************************************/
/* diff_header.h : Chalermek Intanagonwiwat  08/16/99 */
/******************************************************/

#ifndef ns_diff_header_h
#define ns_diff_header_h

#include "ip.h"

#define INTEREST      1
#define DATA          2
#define DATA_READY    3
#define DATA_REQUEST  4
#define POS_REINFORCE 5
#define NEG_REINFORCE 6
#define INHIBIT       7
#define TX_FAILED     8
#define DATA_STOP     9

#define MAX_ATTRIBUTE 3
#define MAX_NEIGHBORS 30
#define MAX_DATA_TYPE 30

#define ROUTING_PORT 255

#define ORIGINAL    100        
#define SUB_SAMPLED 1         

// For positive reinforcement in simple mode. 
// And for TX_FAILED of backtracking mode.

struct extra_info {
  ns_addr_t sender;            // For POS_REINFORCE and TX_FAILED
  unsigned int seq;           // FOR POS_REINFORCE and TX_FAILED
  int size;                   // For TX_FAILED only.
};


struct hdr_cdiff {
	unsigned char mess_type;
	unsigned int pk_num;
	ns_addr_t sender_id;
	nsaddr_t next_nodes[MAX_NEIGHBORS];
	int      num_next;
	unsigned int data_type;
	ns_addr_t forward_agent_id;

	struct extra_info info;

	double ts_;                       // Timestamp when pkt is generated.
	int    report_rate;               // For simple diffusion only.
	int    attr[MAX_ATTRIBUTE];
	
	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_cdiff* access(const Packet* p) {
		return (hdr_cdiff*) p->access(offset_);
	}
};



#endif
