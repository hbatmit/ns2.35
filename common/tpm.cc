/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
//

/*
 * tpm.cc
 * Copyright (C) 2001 by the University of Southern California
 * $Id: tpm.cc,v 1.4 2010/03/08 05:54:49 tom_henderson Exp $
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
// Management function to playback TCPDump trace file (after post precssing)
// Xuan Chen (xuanc@isi.edu)
//
#include "tpm.h"

// Timer to send requests
TPMTimer::TPMTimer(TPM* t) {
	tpm = t;
}

void TPMTimer::expire(Event *) {
	//if (e) 
	//	Packet::free((Packet *)e);

	tpm->run();
}


static class TPMClass : public TclClass {
public:
        TPMClass() : TclClass("TPM") {}
        TclObject* create(int, const char*const*) {
		return (new TPM());
	}
} class_tpm;

TPM::TPM() {
	tp_default_ = tp_num_ = 0;
	tp = NULL;
	
	// initialize next request
	next_p.time = 0;
	next_p.saddr = 0;
	next_p.sport = 0;
	next_p.daddr = 0;
	next_p.dport = 0;
	next_p.len = 0;

	// initialize request timer
	timer = new TPMTimer(this);
	start_t = 0;
}

TPM::~TPM() {
	if (fp)
		fclose(fp);
	if (timer)
		delete timer;
}

int TPM::loadLog(const char* filename) {
	fp = fopen(filename, "r");
	if (fp == 0)
		return(0);
	
	return(1);
}

int TPM::start() {
	start_t = Scheduler::instance().clock();
	processLog();
	return(1);
}

int TPM::processLog() {
	int time, sport, dport, len;
	unsigned int saddr, daddr;

	while (!feof(fp)) {
		fscanf(fp, "%d %u %d %u %d %d\n",
		       &time, &saddr, &sport, &daddr, &dport, &len);

		if (time > 0) {
			// save information for next request
			next_p.time = time;
			next_p.saddr = saddr;
			next_p.sport = sport;
			next_p.daddr = daddr;
			next_p.dport = dport;
			next_p.len = len;	
			
			//double now = Scheduler::instance().clock();
			//double delay = time + start_t - now;
			double delay = time * 1.0 / 1000000.0;
			timer->resched(delay);
			
			return(1);
		} else {
			genPkt(saddr, sport, daddr, dport, len);
		}
	}
	return(1);
}

int TPM::run() {
	genPkt(next_p.saddr, next_p.sport, next_p.daddr, next_p.dport, next_p.len);
	processLog();
	return(1);
}

int TPM::genPkt(unsigned int saddr, int sport, unsigned int daddr, int dport, int len) {
	// we can send packets to anybody with twinks...
	if (saddr == daddr) {
	       return(0);
	}

	// use the default TP agent unless specified.
  	unsigned int tp_id = tp_default_;

	if (saddr < (unsigned int)tp_num_) {
		tp_id = saddr;
	}
	printf("tpid %d\n", tp_id);
	tp[tp_id]->sendto(len, saddr, sport, daddr, dport);
	
	return(1);
}

int TPM::command(int argc, const char*const* argv) {
	if (argc == 2) {
		if (strcmp(argv[1], "start") == 0) {
			if (start())
				return(TCL_OK);
			else
				return(TCL_ERROR);
			
		}
		
	} else if (argc == 3) {
		if (strcmp(argv[1], "loadLog") == 0) {
			if (loadLog(argv[2]))
				return(TCL_OK);
			else
				return(TCL_ERROR);

		} else if (strcmp(argv[1], "tp-num") == 0) {
			tp_num_ = atoi(argv[2]);
			tp_default_ = tp_num_ - 1;
			if (tp_default_ < 0)
			  tp_default_ = 0;
			tp = new TPAgent*[tp_num_];
			return(TCL_OK);
		} else if (strcmp(argv[1], "tp-default") == 0) {
		        tp_default_ = atoi(argv[2]);
			return(TCL_OK);
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "tp-reg") == 0) {
			int index = atoi(argv[2]);
			if (index >= tp_num_)
				return(TCL_ERROR);

			tp[index] = (TPAgent *) TclObject::lookup(argv[3]);
			return(TCL_OK);
		}
	}

	printf("Command specified does not exist\n");
	return(TCL_ERROR);
}
