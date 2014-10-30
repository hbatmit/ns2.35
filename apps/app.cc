/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
 *	This product includes software developed by the Daedalus Research
 *	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
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
 * Contributed by the Daedalus Research Group, http://daedalus.cs.berkeley.edu
 *
 */

#include "app.h"
#include "agent.h"


static class ApplicationClass : public TclClass {
 public:
	ApplicationClass() : TclClass("Application") {}
	TclObject* create(int, const char*const*) {
		return (new Application);
	}
} class_application;


Application::Application() : enableRecv_(0), enableResume_(0)
{
}


int Application::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();

	if (argc == 2) {
		if (strcmp(argv[1], "start") == 0) {
			// enableRecv_ only if recv() exists in Tcl
			tcl.evalf("[%s info class] info instprocs", name_);
			char result[1024];
			sprintf(result, " %s ", tcl.result());
			enableRecv_ = (strstr(result, " recv ") != 0);
			enableResume_ = (strstr(result, " resume ") != 0);
			start();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "stop") == 0) {
			stop();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "agent") == 0) {
			tcl.resultf("%s", agent_->name());
			return (TCL_OK);
		}
	}
	else if (argc == 3) {
		if (strcmp(argv[1], "attach-agent") == 0) {
			agent_ = (Agent*) TclObject::lookup(argv[2]);
			if (agent_ == 0) {
				tcl.resultf("no such agent %s", argv[2]);
				return(TCL_ERROR);
			}
			agent_->attachApp(this);
			return(TCL_OK);
		}
		if (strcmp(argv[1], "send") == 0) {
			send(atoi(argv[2]));
			return(TCL_OK);
		}
	}
	return (Process::command(argc, argv));
}


void Application::start()
{
}


void Application::stop()
{
}


void Application::send(int nbytes)
{
	agent_->sendmsg(nbytes);
}


void Application::recv(int nbytes)
{
	if (! enableRecv_)
		return;
	Tcl& tcl = Tcl::instance();
	tcl.evalf("%s recv %d", name_, nbytes);
}


void Application::resume()
{
	if (! enableResume_)
		return;
	Tcl& tcl = Tcl::instance();
	tcl.evalf("%s resume", name_);
}
