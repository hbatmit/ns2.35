/* -*-	Mode:C++; c-basic-offset:8; tab-width:8 -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by the MASH Research
 * 	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *

 */

#include "basetrace.h"
#include "tcp.h"

class BaseTraceClass : public TclClass {
public:
       BaseTraceClass() : TclClass("BaseTrace") { }
       TclObject* create(int , const char*const* ) {
		  return (new BaseTrace());
       }
} basetrace_class;

class EventTraceClass : public TclClass {
public:
	EventTraceClass() : TclClass("BaseTrace/Event") { }
	TclObject* create(int , const char*const* ) {
		return (new EventTrace());
	}
} eventtrace_class;


BaseTrace::BaseTrace() 
  : channel_(0), namChan_(0), tagged_(0) 
{
  wrk_ = new char[1026];
  nwrk_ = new char[256];
}

BaseTrace::~BaseTrace()
{
  delete wrk_;
  delete nwrk_;
}

void BaseTrace::dump()
{
	int n = strlen(wrk_);
	if ((n > 0) && (channel_ != 0)) {
		/*
		 * tack on a newline (temporarily) instead
		 * of doing two writes
		 */
		wrk_[n] = '\n';
		wrk_[n + 1] = 0;
 /* -NEW- */
		//printf("%s",wrk_);
		(void)Tcl_Write(channel_, wrk_, n + 1);

 /* END -NEW- */
		//Tcl_Flush(channel_);
		wrk_[n] = 0;
	}

	//  if (callback_) {
//  		Tcl& tcl = Tcl::instance();
//  		tcl.evalf("%s handle { %s }", name(), wrk_);
//  	}
}

void BaseTrace::namdump()
{
	int n = 0;

	/* Otherwise nwrk_ isn't initialized */
	if (namChan_ != 0)
		n = strlen(nwrk_);
	if ((n > 0) && (namChan_ != 0)) {
		/*
		 * tack on a newline (temporarily) instead
		 * of doing two writes
		 */
		nwrk_[n] = '\n';
		nwrk_[n + 1] = 0;
		(void)Tcl_Write(namChan_, nwrk_, n + 1);
		//Tcl_Flush(channel_);
		nwrk_[n] = 0;
	}
}


/*
 * $trace detach
 * $trace flush
 * $trace attach $fileID
 */
int BaseTrace::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "detach") == 0) {
			channel_ = 0;
			namChan_ = 0;
			return (TCL_OK);
		}
		if (strcmp(argv[1], "flush") == 0) {
			if (channel_ != 0) 
				Tcl_Flush(channel_);
			if (namChan_ != 0)
				Tcl_Flush(namChan_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "tagged") == 0) {
			tcl.resultf("%d", tagged());
                        return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "attach") == 0) {
			int mode;
			const char* id = argv[2];
			channel_ = Tcl_GetChannel(tcl.interp(), (char*)id,
						  &mode);
			if (channel_ == 0) {
				tcl.resultf("trace: can't attach %s for writing", id);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "namattach") == 0) {
			int mode;
			const char* id = argv[2];
			namChan_ = Tcl_GetChannel(tcl.interp(), (char*)id,
						  &mode);
			if (namChan_ == 0) {
				tcl.resultf("trace: can't attach %s for writing", id);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "tagged") == 0) {
			int tag;
			if (Tcl_GetBoolean(tcl.interp(),
					   (char*)argv[2], &tag) == TCL_OK) {
				tagged(tag);
				return (TCL_OK);
			} else return (TCL_ERROR);
		}
	}
	return (TclObject::command(argc, argv));
}

/* The eventtrace format is tentative for now; will be extended in future to include data like window size, rtt, segsize, segperack etc */

void EventTrace::trace()
{
  dump();
  namdump();
}


