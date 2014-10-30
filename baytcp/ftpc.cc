/*
 * Copyright (c) 1994 Regents of the University of California.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
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
 */

/*
 * For use with "full tcp" model.
 */

static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/baytcp/ftpc.cc,v 1.6 2010/03/08 05:54:49 tom_henderson Exp $ (LBL)";

#include "tcp-full-bay.h"
#include "tclcl.h"
#include "trace.h"
#include "random.h"
#include "ftp.h"


static class FtpClientClass : public TclClass {
public:
	FtpClientClass() : TclClass("Agent/BayTcpApp/FtpClient") {}
	TclObject* create(int, const char*const*) {
		return (new FtpClientAgent());
	}
} class_ftpcli;

FtpClientAgent::FtpClientAgent() : BayTcpAppAgent(PT_NTYPE), running_(0), newfile_timer_(this)
{
}

void FtpClientAgent::start()
{
	running_ = 1;
	newfile_timer_.resched(0.);
}

void FtpClientAgent::stop()
{
	running_ = 0;
}

void FtpClientAgent::timeout(int event_type)
{
	if (running_)
		if(event_type == NEW_FILE)	{
			if(sendget())	{
				state_ = REQ_SENT;
				start_trans_ = now();
			}
			else {
				printf("ftpclient:timeout erroneous tcp state\n");
			}
		}
}

//assumes 80 bytes
int FtpClientAgent::sendget()
{
	return tcp_->advance(80, 0);
}

//scheduled only when the tcp connection(s) are upcalling
// ask for another file after a delay to allow connection to close
// 6/8/00 shouldn't need the delay. Consider reducing or removing -kmn
void FtpClientAgent::recv(Packet*, BayFullTcpAgent*, int code)
{
  //at data complete time, schedule a "far out" event to ensure
  // simulator doesn't terminate
  if(running_ && code == DATA_PUSH) {
  state_ = DATA_RCVD;
  newfile_timer_.resched(5.0);
  }
  else if(running_ && code == CONNECTION_END) {
    state_ = END_RCVD;
    newfile_timer_.cancel();
    newfile_timer_.resched(.0);
  }
}

int FtpClientAgent::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "start") == 0) {
			start();
			return(TCL_OK);
		} else if (strcmp(argv[1], "stop") == 0) {
			stop();
			return(TCL_OK);
		}
	} else if(argc == 3) {
		if(strcmp(argv[1], "tcp") == 0) {
			tcp_ = (BayFullTcpAgent*)TclObject::lookup(argv[2]);
			if(tcp_ == 0) {
				tcl.resultf("no such agent %s", argv[2]);
				return(TCL_ERROR);
			}
			return(TCL_OK);
		}
	}
	return (Agent::command(argc, argv));
}


void NewFileTimer::expire(Event *) {
	a_->timeout(NEW_FILE);
}
