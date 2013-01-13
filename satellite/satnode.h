/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1999 Regents of the University of California.
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
 *      This product includes software developed by the MASH Research
 *      Group at the University of California Berkeley.
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
 * @(#) $Header: /cvsroot/nsnam/ns-2/satellite/satnode.h,v 1.5 2001/11/06 06:21:47 tomh Exp $
 *
 * Contributed by Tom Henderson, UCB Daedalus Research Group, June 1999
 */

#ifndef __satnode_h__
#define __satnode_h__

#include <trace.h>
#include <node.h>
#include <stdlib.h>
#include "object.h"
#include "assert.h"

class LinkHandoffMgr;
class SatChannel;
class SatPosition;
class SatRouteAgent;
class SatTrace;

class SatNode : public Node {
 public:
	SatNode();
	// Each SatNode has a ragent_ (may be NULL), position_, a 
	// trace_ (may be NULL), and a hm_ (may be NULL)  
	SatRouteAgent* ragent() { return ragent_; }
	SatPosition* position() { return pos_; }
	SatTrace* trace() { return trace_; }
	// The uplink_ and downlink_ variables allow us to avoid searching
	// through the list of links for these commonly accessed channels
	Channel* uplink() { return ((Channel*) uplink_);} 
	Channel* downlink() { return ((Channel*) downlink_);} 

	// configuration parameters
	static int dist_routing_;
	static int addNode(int);
	static int IsASatNode(int);
 protected:
        int command(int argc, const char*const* argv);
	SatRouteAgent* ragent_;
	SatChannel* uplink_;
	SatChannel* downlink_;
	SatPosition* pos_;
	SatTrace* trace_; // a drop trace for packets that can't be routed
	LinkHandoffMgr* hm_; 
	void dumpSats();
	static int* satnodelist_;
	static int maxsatnodelist_;
};

#endif // __satnode_h__
