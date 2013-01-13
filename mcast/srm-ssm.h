
/*
 * srm-ssm.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: srm-ssm.h,v 1.6 2005/09/22 07:43:33 lacage Exp $
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
// $Header: /cvsroot/nsnam/ns-2/mcast/srm-ssm.h,v 1.6 2005/09/22 07:43:33 lacage Exp $

#ifndef ns_ssmsrm_h
#define ns_ssmsrm_h

#include "config.h"
//#include "heap.h"
#include "srm-state.h"


/* Constants for scope flags and types of session messages */
#define SRM_LOCAL 1
#define SRM_GLOBAL 2
#define SRM_RINFO  3  /* Session Message with reps information */

struct hdr_srm_ext {
	int     repid_;
	int     origTTL_;

	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_srm_ext* access(const Packet* p) {
		return (hdr_srm_ext*) p->access(offset_);
	}

	// per field member functions
	int& repid()	{ return repid_; }
	int& ottl()    { return origTTL_; } 
};

class SSMSRMAgent : public SRMAgent 
{
  int	glb_sessCtr_;		  /* # of global session messages sent */
  int	loc_sessCtr_;		  /* # of local session messages sent */
  int	rep_sessCtr_;		  /* # of rep session messages sent */
  int   scopeFlag_;
  int   groupScope_;              /* Scope of the group, ttl */
  int   localScope_;              /* Scope of the local messages, ttl */
  int   senderFlag_;
  int   repid_;

  void recv_data(int sender, int id, int repid, u_char* data);
  //void recv_repr(int sender, int msgid, int repid, u_char* data);
  void recv_rqst(int requestor, int round, int sender, int msgid, int repid);
  void recv_sess(int sessCtr, int* data, Packet *p);
  void recv_glb_sess(int sessCtr, int* data, Packet *p);
  void recv_loc_sess(int sessCtr, int* data, Packet *p);
  void recv_rep_sess(int sessCtr, int* data, Packet *p);
  void send_ctrl(int type, int round, int sender, int msgid, int size);
  void send_sess();
  void send_glb_sess();
  void send_loc_sess();
  void send_rep_sess();
  void timeout_info();
  int is_active(SRMinfo *sp);
public:
  SSMSRMAgent();
  int command(int argc, const char*const* argv);
  void recv(Packet* p, Handler* h);
};


#endif // ns_srm_ssm_h
