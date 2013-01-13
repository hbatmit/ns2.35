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
    "@(#) $Header: /cvsroot/nsnam/ns-2/trace/traffictrace.cc,v 1.16 2005/08/26 05:05:31 tomh Exp $ (Xerox)";
#endif

/* XXX: have not dealt with errors.  e.g., if something fails during
 * TraceFile::setup(), or TrafficTrace is not pointing to a TraceFile,
 * no guarantee about results.
 */
 
#include <sys/types.h>
#include <sys/stat.h> 
#include <stdio.h>

#include "config.h"
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */


#include "random.h"
/* module to implement a traffic generator in ns driven by a trace
 * file.  records in the trace file consist of 2 32 bit fields, the
 * first indicating the inter-packet time in microseconds, and the
 * second indicating the packet length in bytes.  multiple TraffficTrace
 * objects can use the same trace file, and different TrafficTrace
 * objects can use different trace files.  For each TrafficTrace, a
 * random starting point within the trace file is selected.
 */

#include "object.h"
#include "trafgen.h"

struct tracerec {
        u_int32_t trec_time; /* inter-packet time (usec) */
	u_int32_t trec_size; /* size of packet (bytes */
};


/* object to hold a single trace file */

class TraceFile : public NsObject {
 public:
	TraceFile();
	void get_next(int&, struct tracerec&); /* called by TrafficGenerator
						* to get next record in trace.
						*/
	int setup();  /* initialize the trace file */
	int command(int argc, const char*const* argv);
 private:
	void recv(Packet*, Handler*); /* must be defined for NsObject */
        int status_; 
	char *name_;  /* name of the file in which the trace is stored */
	int nrec_;    /* number of records in the trace file */
	struct tracerec *trace_; /* array holding the trace */
};

/* instance of a traffic generator.  has a pointer to the TraceFile
 * object and implements the interval() function.
 */

class TrafficTrace : public TrafficGenerator {
 public:
	TrafficTrace();
	int command(int argc, const char*const* argv);
	virtual double next_interval(int &);
 protected:
	void timeout();
	TraceFile *tfile_;
	struct tracerec trec_;
	int ndx_;
	void init();
};


static class TraceFileClass : public TclClass {
 public:
	TraceFileClass() : TclClass("Tracefile") {}
	TclObject* create(int, const char*const*) {
		return (new TraceFile());
	}
} class_tracefile;

TraceFile::TraceFile() : status_(0)
{
}

int TraceFile::command(int argc, const char*const* argv)
{

	if (argc == 3) {
		if (strcmp(argv[1], "filename") == 0) {
			name_ = new char[strlen(argv[2])+1];
			strcpy(name_, argv[2]);
			return(TCL_OK);
		}
	}
	return (NsObject::command(argc, argv));
}

void TraceFile::get_next(int& ndx, struct tracerec& t)
{
	t.trec_time = trace_[ndx].trec_time;
	t.trec_size = trace_[ndx].trec_size;

	if (++ndx == nrec_)
		ndx = 0;

}

int TraceFile::setup()
{
	tracerec* t;
	struct stat buf;
	int i;
	FILE *fp;

	/* only open/read the file once (could be shared by multiple
	 * SourceModel's
	 */
	if (! status_) {
		status_ = 1;

		if (stat(name_, (struct stat *)&buf)) {
			printf("could not stat %s\n", name_);
			return -1;
		}

		nrec_ = buf.st_size/sizeof(tracerec);
		unsigned nrecplus = nrec_ * sizeof(tracerec);
		unsigned bufst = buf.st_size;

		//	if ((unsigned)(nrec_ * sizeof(tracerec)) != buf.st_size) {
		if (nrecplus != bufst) {
			printf("bad file size in %s\n", name_);
			return -1;
		}

		trace_ = new struct tracerec[nrec_];

		if ((fp = fopen(name_, "rb")) == NULL) {
			printf("can't open file %s\n", name_);
			return -1;
		}

		for (i = 0, t = trace_; i < nrec_; i++, t++)
			if (fread((char *)t, sizeof(tracerec), 1, fp) != 1) {
				printf("read failed\n");
				return -1 ;
			}
			else {
		
				t->trec_time = ntohl(t->trec_time);
				t->trec_size = ntohl(t->trec_size);
			}

	}

	/* pick a random starting place in the trace file */
	return (int(Random::uniform((double)nrec_)+.5));

}

void TraceFile::recv(Packet*, Handler*)
{
        /* shouldn't get here */
        abort();
}

/**************************************************************/

static class TrafficTraceClass : public TclClass {
 public:
	TrafficTraceClass() : TclClass("Application/Traffic/Trace") {}
	TclObject* create(int, const char*const*) {
	        return(new TrafficTrace());
	}
} class_traffictrace;

TrafficTrace::TrafficTrace()
{
	tfile_ = (TraceFile *)NULL;
}

void TrafficTrace::init()
{
	if (tfile_) 
		ndx_ = tfile_->setup();
}

int TrafficTrace::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	
	if (argc == 3) {
		if (strcmp(argv[1], "attach-tracefile") == 0) {
			tfile_ = (TraceFile *)TclObject::lookup(argv[2]);
			if (tfile_ == 0) {
				tcl.resultf("no such node %s", argv[2]);
				return(TCL_ERROR);
			}
			return(TCL_OK);
		}
	}

	return (TrafficGenerator::command(argc, argv));

}

void TrafficTrace::timeout()
{
        if (! running_)
                return;

        /* send a packet */
	// Note:  May need to set "NEW_BURST" flag in sendmsg() for 
	// signifying a new talkspurt when using vat traces.
	// (see expoo.cc, tcl/ex/test-rcvr.tcl)
	agent_->sendmsg(size_);
        /* figure out when to send the next one */
        nextPkttime_ = next_interval(size_);
        /* schedule it */
        timer_.resched(nextPkttime_);
}


double TrafficTrace::next_interval(int& size)
{
        tfile_->get_next(ndx_, trec_);
	size = trec_.trec_size;
	return(((double)trec_.trec_time)/1000000.0); /* usecs->secs */
}
