/*
 * Copyright (c) 1991-1997 Regents of the University of California.
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
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/baytcp/ftp.h,v 1.3 2001/07/19 17:57:02 haldar Exp $ (LBL)
 */
#ifndef ns_ftp_h
#define ns_ftp_h

#include "packet.h"
#include "agent.h"

/* states */
#define REQ_SENT 0 
#define DATA_RCVD 1 
#define END_RCVD 2 
/* events */
#define NEW_FILE 0

class FtpClientAgent;

class NewFileTimer : public TimerHandler {
public:
  NewFileTimer(FtpClientAgent *a) : TimerHandler() {a_ = a;}
protected:
  virtual void expire(Event *e);
  FtpClientAgent *a_;
};


/* added int to recv -kmn 6/8/00 */

class FtpClientAgent : public BayTcpAppAgent {
 public:
	FtpClientAgent();
	int command(int argc, const char*const* argv);
	virtual void recv(Packet*, BayFullTcpAgent*, int);
	//virtual void recv(Packet*, BayFullTcpAgent*);
        virtual void timeout(int);
 protected:

	void start();
	void stop();
	double now() { return Scheduler::instance().clock(); }
	int sendget();
	int running_;
	double start_trans_;
	int state_;
	BayFullTcpAgent* tcp_;
        NewFileTimer newfile_timer_;
};

#endif
