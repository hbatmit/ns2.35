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
 * @(#) $Header: /cvsroot/nsnam/ns-2/common/bi-connector.h,v 1.1 1998/12/08 19:09:02 haldar Exp $
 */

#ifndef ns_new_connector_h
#define ns_new_connector_h

#include "object.h"

/*
 * An NsObject with two targets - up and down.
 */
class BiConnector : public NsObject {
public:
	BiConnector();
	inline NsObject* uptarget() { return uptarget_; }
	NsObject* downtarget() { return downtarget_; }
	virtual void drop(Packet* p);
protected:
	virtual void drop(Packet* p, const char *s);
	int command(int argc, const char*const* argv);
	void recv(Packet*, Handler* callback = 0);
	virtual void sendDown(Packet* p, Handler* h)
		{ downtarget_->recv(p, h); }
	inline virtual void sendUp(Packet* p, Handler* h) 
		{ uptarget_->recv(p, h); }

	NsObject* uptarget_;    // upstream target -->previously 
	// defined as target_ for connector objects then supporting 
	// unidrectional connectivity only.
	NsObject* downtarget_;  // downstream target
	NsObject* drop_;	// drop target for this connector
};

#endif //ns_new_connector







