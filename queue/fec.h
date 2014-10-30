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
 * Contributed by the Daedalus Research Group, UC Berkeley 
 * (http://daedalus.cs.berkeley.edu)
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/queue/fec.h,v 1.1 2001/03/07 18:30:02 jahn Exp $ (UCB)
 */

#ifndef ns_fecmodel_h
#define ns_fecmodel_h

#include "bi-connector.h"
#include "ranvar.h"
#include "packet.h"

/* 
 * Basic object for error models.  This can be used unchanged by error 
 * models that are characterized by a single parameter, the rate of errors 
 * (or equivalently, the mean duration/spacing between errors).  Currently,
 * this includes the uniform and exponentially-distributed models.
 */
class FECModel : public BiConnector {
public:
	FECModel();
	virtual void recv(Packet*, Handler*);
protected:
	virtual int command(int argc, const char*const* argv);
	void addfec(Packet *p);
	void subfec(Packet *p);
	int firstTime_;		// to not corrupt first packet in byte model
	int FECstrength_;       // indicate how many corrupted bits are corrected
	int FECbyte_;
	int debug_;
};
#endif

