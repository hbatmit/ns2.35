
/*
 * srm-headers.h
 * Copyright (C) 1997 by the University of Southern California
 * $Id: srm-headers.h,v 1.11 2005/08/25 18:58:08 johnh Exp $
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
// @(#) $Header: /cvsroot/nsnam/ns-2/mcast/srm-headers.h,v 1.11 2005/08/25 18:58:08 johnh Exp $ (USC/ISI)
//

#ifndef ns_srm_headers_h
#define ns_srm_headers_h

struct hdr_srm {

#define SRM_DATA 1
#define SRM_SESS 2
#define SRM_RQST 3
#define SRM_REPR 4

	int	type_;
	int	sender_;
	int	seqnum_;
	int	round_;

	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_srm* access(const Packet* p) {
		return (hdr_srm*) p->access(offset_);
	}
	
	// per field member functions
	int& type()	{ return type_; }
	int& sender()	{ return sender_; }
	int& seqnum()	{ return seqnum_; }
	int& round()	{ return round_; }
};

#define SRM_NAMES "NULL", "SRM_DATA", "SRM_SESS", "SRM_RQST", "SRM_REPR"

struct hdr_asrm {
	double	distance_;

	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_asrm* access(const Packet* p) {
		return (hdr_asrm*) p->access(offset_);
	}

	// per field member functions
	double& distance()	{ return distance_; }
};

#endif /* ns_srm_headers_.h */
