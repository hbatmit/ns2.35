/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linking this file statically or dynamically with other modules is making
 * a combined work based on this file.  Thus, the terms and conditions of
 * the GNU General Public License cover the whole combination.
 *
 * In addition, as a special exception, the copyright holders of this file
 * give you permission to combine this file with free software programs or
 * libraries that are released under the GNU LGPL and with code included in
 * the standard release of ns-2 under the Apache 2.0 license or under
 * otherwise-compatible licenses with advertising requirements (or modified
 * versions of such code, with unchanged license).  You may copy and
 * distribute such a system following the terms of the GNU GPL for this
 * file and the licenses of the other code concerned, provided that you
 * include the source code of that other code when and as the GNU GPL
 * requires distribution of source code.
 *
 * Note that people who make modified versions of this file are not
 * obligated to grant this special exception for their modified versions;
 * it is their choice whether to do so.  The GNU General Public License
 * gives permission to release a modified version without this exception;
 * this exception also makes it possible to release a modified version
 * which carries forward this exception.
 */
#ifndef lint
static const char rcsid[] =
	"@(#) $Header: /cvsroot/nsnam/ns-2/adc/adc.cc,v 1.8 2005/08/26 05:05:27 tomh Exp $";
#endif

#include "adc.h"
#include <stdlib.h>

ADC::ADC() :bandwidth_(0), tchan_(0)
{
	bind_bw("bandwidth_",&bandwidth_);
	bind_bool("backoff_",&backoff_);
	bind("src_", &src_);
	bind("dst_", &dst_);
	bind_bool("dobump_", &dobump_);
}

int ADC::command(int argc,const char*const*argv)
{
	
	Tcl& tcl = Tcl::instance();
	if (argc==2) {
		if (strcmp(argv[1],"start") ==0) {
			/* $adc start */
			est_[1]->start();
			return (TCL_OK);
		}
	} else if (argc==4) {
		if (strcmp(argv[1],"attach-measmod") == 0) {
			/* $adc attach-measmod $meas $cl */
			MeasureMod *meas_mod = (MeasureMod *)TclObject::lookup(argv[2]);
			if (meas_mod== 0) {
				tcl.resultf("no measuremod found");
				return(TCL_ERROR);
			}
			int cl=atoi(argv[3]);
			est_[cl]->setmeasmod(meas_mod);
			return(TCL_OK);
		} else if (strcmp(argv[1],"attach-est") == 0 ) {
			/* $adc attach-est $est $cl */
			Estimator *est_mod = (Estimator *)TclObject::lookup(argv[2]);
			if (est_mod== 0) {
				tcl.resultf("no estmod found");
				return(TCL_ERROR);
			}
			int cl=atoi(argv[3]);
			setest(cl,est_mod);
			return(TCL_OK);
		}
	}
	else if (argc == 3) {
		if (strcmp(argv[1], "attach") == 0) {
			int mode;
			const char* id = argv[2];
			tchan_ = Tcl_GetChannel(tcl.interp(), (char*)id, &mode);
			if (tchan_ == 0) {
				tcl.resultf("ADC: trace: can't attach %s for writing", id);
				return (TCL_ERROR);
			}
			return (TCL_OK);
			
		}
		if (strcmp(argv[1], "setbuf") == 0) {
			/* some sub classes actually do something here */
			return(TCL_OK);
		}


	}
	return (NsObject::command(argc,argv));
}




