/*
 * Copyright (c) 1997, 1998 The Regents of the University of California.
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
 *	Group at the University of California, Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be used
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
 * @(#) $Header: /cvsroot/nsnam/ns-2/emulate/tap.h,v 1.2 2002/09/23 23:25:05 alefiyah Exp $ (ISI) 
 */


#ifndef tap_h
#define tap_h

#include "tclcl.h"
#include "net.h"
#include "packet.h"
#include "agent.h"


#define TAPDEBUG 1 
#ifdef TAPDEBUG
#define	TDEBUG(x) { if (TAPDEBUG) fprintf(stderr, (x)); }
#define	TDEBUG2(x,y) { if (TAPDEBUG) fprintf(stderr, (x), (y)); }
#define	TDEBUG3(x,y,z) { if (TAPDEBUG) fprintf(stderr, (x), (y), (z)); }
#define	TDEBUG4(w,x,y,z) { if (TAPDEBUG) fprintf(stderr, (w), (x), (y), (z)); }
#define	TDEBUG5(v,w,x,y,z) { if (TAPDEBUG) fprintf(stderr, (v), (w), (x), (y), (z)); }
#else
#define	TDEBUG(x) { }
#define	TDEBUG2(x,y) { }
#define	TDEBUG3(x,y,z) { }
#define	TDEBUG4(w,x,y,z) { }
#define	TDEBUG5(v,w,x,y,z) { }
#endif

#include <errno.h>

class TapAgent : public Agent, public IOHandler {
public:
        TapAgent();
	int command(int, const char*const*);
	void recv(Packet* p, Handler*);	/* sim->live net */  
private:
	virtual int sendpkt(Packet *);
	virtual void recvpkt();
protected:
	int maxpkt_;		/* max size allocated to recv a pkt */
	void dispatch(int);	/* invoked via scheduler on I/O event */
	int linknet();		/* establish I/O handler */
	Network* net_;		/* live network object */
	double now() { return Scheduler::instance().clock(); }
};



#endif /* tap_h */





