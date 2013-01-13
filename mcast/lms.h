
/*
 * lms.h
 * Copyright (C) 2001 by the University of Southern California
 * $Id: lms.h,v 1.3 2005/08/25 18:58:07 johnh Exp $
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
 * Light-Weight Multicast Services (LMS), Reliable Multicast
 *
 * lms.h
 *
 * This holds the packet header structures, and packet type constants for
 * the LMS implementation.
 *
 * Christos Papadopoulos. 
 * christos@isi.edu
 */

#ifndef lms_h
#define lms_h

class LmsAgent;

#include "node.h"
#include "packet.h"


#define LMS_NOADDR	-1
#define LMS_NOPORT      -1
#define LMS_NOIFACE	-99
#define LMS_INFINITY	1000000

/*
 * PT_LMS packet types
 */
#define	LMS_SRC_REFRESH	1
#define	LMS_REFRESH	2
#define	LMS_LEAVE	3
#define	LMS_REQ		4
#define	LMS_DMCAST	5
#define	LMS_SETUP	6
#define	LMS_SPM		7
#define	LMS_LINKS	8

/*
 * LMS header
 */
struct hdr_lms {
    int	       	type_;		// packet type
    int	       	ttl_;		// time-to-live
    int	       	cost_;		// cost advertisements for LMS_REFRESH
    nsaddr_t    from_;		// real source of packet for DMCASTs
    nsaddr_t    src_;		// original source of mcast packet
    nsaddr_t    group_;		// mcast group
    nsaddr_t    tp_addr_;	// turning point address
    int	       	tp_port_;	// turning point port id 
    int	       	tp_iface_;	// turning point interface
    int	       	lo_, hi_;	// range of lost packets
    double     	ts_;		// timestamp for RTT estimation
    
    static int offset_;
    inline static int& offset() { return offset_; }
    inline static hdr_lms* access(Packet* p) {
        return (hdr_lms*)p->access(offset_);
    }

    /* per-field member functions */    
    int&	type ()  { return type_;  }
    nsaddr_t&	from ()  { return from_;  }
    nsaddr_t&	src ()   { return src_;   }
    nsaddr_t&   tp_addr ()  { return tp_addr_;  }
    nsaddr_t&  tp_port ()  { return (nsaddr_t&) tp_port_;  }
    nsaddr_t&	group () { return group_; }
};

struct lms_ctl {
    int		hop_cnt_;	// hop counter (similar to ttl)
    int		cost_;		// cost advertisements for LMS_REFRESH
    nsaddr_t	tp_addr_;	// turning point address
    nsaddr_t    tp_port_;       // turning point port id
    ns_addr_t	downstream_lms_;// needed when in incrDeployment       
    int		tp_iface_;	// turning point interface
    int		nak_lo_;
    int		nak_hi_;	// range of lost packets
    double      ts_;            // timestamp
};

struct lms_nak {
	int	nak_lo_;
	int	nak_hi_;
	int	nak_seqn_;
        int     dup_cnt_;    // num of dup requests for each NAK
	packet_t t;
	int	datsize;    
};

struct lms_rdl {
	int		seqn_;
	double		ts_;
	struct lms_rdl	*next_;
};

struct lms_spm {
	int		spm_seqno_;
	nsaddr_t	spm_path_;
	double		spm_ts_;
};

#endif   /* end ifndef lms_h */
