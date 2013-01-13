
/*
 * srm-state.h
 * Copyright (C) 1997 by the University of Southern California
 * $Id: srm-state.h,v 1.8 2005/08/25 18:58:08 johnh Exp $
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
// @(#) $Header: /cvsroot/nsnam/ns-2/mcast/srm-state.h,v 1.8 2005/08/25 18:58:08 johnh Exp $ (USC/ISI)
//

#ifndef ns_srm_state_h
#define	ns_srm_state_h

#include <assert.h>

class SRMinfo {
public:
	SRMinfo* next_;
	
	int	sender_;

	/* Session messages */
	int	lsess_;			  /* # of last session msg received */
	int	sendTime_;		  /* Time sess. msg. # sent */
	int	recvTime_;		  /* Time sess. msg. # received */
	double	distance_;
	
        /* Added for ssm, later check if can do for a derived class */
        /* Can do better with space for these flags */
        int     lglbsess_;
        int     llocsess_;
        int     lrepsess_;
        int     repid_;
        int     senderFlag_;
        int     scopeFlag_;
#define ACTIVE 1
#define INACTIVE 0
        int     activeFlag_;
#define SELF_DISTANCE   1
#define REP_DISTANCE    2       
#define NO_DISTANCE     0
        int     distanceFlag_;

	/* Data messages */
	int	ldata_;			  /* # of last data msg sent */
protected:
	char	*received_;		  /* Bit vector of msgs received */
	int	recvmax_;

#define	BITVEC_SIZE_DEFAULT	256	  // for (256 * 8) = 2K messages
	void resize(int id) {
		if (! received_) {
			received_ = new char[BITVEC_SIZE_DEFAULT];
			recvmax_ = BITVEC_SIZE_DEFAULT * sizeof(char);
			(void) memset(received_, '\0', BITVEC_SIZE_DEFAULT);
		}
		if (recvmax_ <= id) {
			int osize, nsize;
			nsize = osize = recvmax_;
			while (nsize <= id)
				nsize *= 2;
			osize /= sizeof(char);
			nsize /= sizeof(char);
			char* nvec = new char[nsize];
			(void) memcpy(nvec, received_, osize);
			(void) memset(nvec + osize, '\0', osize);
			delete[] received_;
			received_ = nvec;
			recvmax_ = nsize;
		}
	}	
		
public:
	SRMinfo(int sender) : next_(0), sender_(sender),
		lsess_(-1), sendTime_(0), recvTime_(0), distance_(1.0),
		senderFlag_(0), activeFlag_(ACTIVE), distanceFlag_(NO_DISTANCE),
		ldata_(-1), received_(0), recvmax_(-1) { 
                    lglbsess_ = -1;
                    llocsess_ = -1;
                    lrepsess_ = -1;
                   }
	~SRMinfo() { delete[] received_; }
	
	char ifReceived(int id) {
		assert(id >= 0);
		if (id >= recvmax_)
			resize(id);
		return (received_[id / 8] & (1 << (id % 8)));
	}
	char setReceived(int id) {
		int obit = ifReceived(id);
		received_[id / 8] |= (1 << (id % 8));
		return obit;
	}
	char resetReceived(int id) {
		int obit = ifReceived(id);
		received_[id / 8] &= ~(1 << (id % 8));
		return obit;
	}
};

#endif /* ns_srm_state_h */
