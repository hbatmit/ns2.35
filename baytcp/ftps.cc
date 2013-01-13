/*
 * Copyright (c) 1997 The Regents of the University of California.
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
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
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
 * This agent is paired with one or more ftp clients.
 * It expects to be the target of a FullTcpAgent.
 */
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/baytcp/ftps.cc,v 1.6 2001/09/06 22:09:41 haldar Exp $ ()";

#include "tcp-full-bay.h"
#include "tclcl.h"
#include "random.h"
#include "trace.h"
#include "tcp.h"

class FtpSrvrAgent : public BayTcpAppAgent {
 public:
	FtpSrvrAgent();
	int command(int argc, const char*const* argv);
  void recv(Packet*, BayFullTcpAgent*, int code);
  //void recv(Packet*, BayFullTcpAgent*);
protected:
	double now()  { return Scheduler::instance().clock(); }
	int min_response_;	//number of bytes in min response file
	int max_response_;	//number of bytes in max response file
	int filesize_; 		//file size in Bytes
};


static class FtpSrvrClass : public TclClass {
public:
	FtpSrvrClass() : TclClass("Agent/BayTcpApp/FtpServer") {}
	TclObject* create(int, const char*const*) {
		return (new FtpSrvrAgent());
	}
} class_ftps;

FtpSrvrAgent::FtpSrvrAgent() : BayTcpAppAgent(PT_NTYPE)
{
//	bind("min_byte_response", &min_response_);
//	bind("max_byte_response", &max_response_);
//	bind_time("service_time", &service_time_);
	filesize_ = 0x10000000;
}

//should only be called when a get is received from a client
//need to make sure it goes to the right tcp connection

void FtpSrvrAgent::recv(Packet*, BayFullTcpAgent* tcp, int code)
{
  if(code == DATA_PUSH) {
    int length = filesize_;
    //tells tcp-full with my mods to send FIN when empty
    tcp->advance(length, 1);
  }
}

/*
 * set length
 */
int FtpSrvrAgent::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "file_size") == 0) {
			filesize_ = atoi(argv[2]);
			return (TCL_OK); 
		}
	} 
	return (Agent::command(argc, argv));
}

