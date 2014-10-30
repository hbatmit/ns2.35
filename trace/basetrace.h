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

#ifndef ns_basetrace_h
#define ns_basetrace_h

#include <math.h> //floor
#include "tcp.h"

class BaseTrace : public TclObject {
public:
	BaseTrace();
	~BaseTrace();
	virtual int command(int argc, const char*const* argv);
	virtual void dump();
	virtual void namdump();
	inline char* buffer() { return wrk_ ; }
	inline char *nbuffer() {return nwrk_; }

	inline Tcl_Channel channel() { return channel_; }
	inline void channel(Tcl_Channel ch) {channel_ = ch; }

	inline Tcl_Channel namchannel() { return namChan_; }
	inline void namchannel(Tcl_Channel namch) {namChan_ = namch; }

	void flush(Tcl_Channel channel) { Tcl_Flush(channel); }

	//Default rounding is to 6 digits after decimal
#define PRECISION 1.0E+6
	//According to freeBSD /usr/include/float.h 15 is the number of digits 
	// in a double.  We can specify all of them, because we're rounding to
	// 6 digits after the decimal and and %g removes trailing zeros.
#define TIME_FORMAT "%.15g"
	// annoying way of tackling sprintf rounding platform 
	// differences :
	// use round(Scheduler::instance().clock()) instead of 
	// Scheduler::instance().clock().
	
	static double round (double x, double precision=PRECISION) {
		return (double)floor(x*precision + 0.5)/precision;
	}

	inline bool tagged() { return tagged_; }
	inline void tagged(bool tag) { tagged_ = tag; }
	
protected:
	Tcl_Channel channel_;
	Tcl_Channel namChan_;
	char *wrk_;
	char *nwrk_;
	bool tagged_;
};

class EventTrace : public BaseTrace {
public:
	EventTrace() : BaseTrace() {}
	virtual void trace();
};

#endif // BaseTrace
