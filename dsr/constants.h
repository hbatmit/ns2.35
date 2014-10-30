
/*
 * constants.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: constants.h,v 1.3 2005/08/25 18:58:04 johnh Exp $
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

/*   constants.h

   nice constants to know about that are used in various places
*/

#ifndef _constants_h
#define _constants_h

#include "path.h"

/************** exported from lsnode.cc **************/
extern Time RR_timeout;		// (sec) route request timeout
extern Time arp_timeout;	// (sec) arp request timeout
extern Time retry_arp_period;	// secs time between arps
extern Time rt_rq_period;	// secs length of one backoff period
extern Time rt_rq_max_period;	// secs maximum time between rt reqs
extern Time rt_rep_holdoff_period;
// to determine how long to sit on our rt reply we pick a number
// U(O.0,rt_rep_holdoff_period) + (our route length-1)*rt_rep_holdoff

extern Time process_time; // average time taken to process a packet 

#endif // _constants_h
