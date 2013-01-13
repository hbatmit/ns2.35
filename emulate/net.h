/*
 * Copyright (c) 1991-1994, 1998 Regents of the University of California.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and the Network Research Group at
 *      Lawrence Berkeley Laboratory.
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
 * @(#) $Header: /cvsroot/nsnam/ns-2/emulate/net.h,v 1.9 2010/03/08 05:54:50 tom_henderson Exp $ (LBL)
 */

#ifndef ns_net_h
#define ns_net_h

#include <sys/socket.h>
#include <fcntl.h>

#include "tclcl.h"
#include "iohandler.h"
#include "timer.h"

/* win95 #define's this...*/
#ifdef interface
#undef interface
#endif

class Packet;
typedef void (*netpkt_handler)(void *userdata, Packet *p,
			       const struct timeval &ts);

class Network : public TclObject {
public:
	Network() : mode_(-1) { }
	virtual int command(int argc, const char*const* argv);
	virtual int send(u_char* buf, int len) = 0;
	virtual int recv(u_char* buf, int len, sockaddr& from, double& ts) = 0;
	virtual int recv(netpkt_handler callback, void *clientdata); // callback called for every packet
	virtual int rchannel() = 0;
	virtual int schannel() = 0;
	int mode() { return mode_; }
	static int nonblock(int fd);
	static int parsemode(const char*);  // strings to mode bits
	static char* modename(int);	    // and the reverse
protected:
	int mode_;	// read/write bits (from fcntl.h)
};
#endif
