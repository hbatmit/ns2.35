/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1993-1997 Regents of the University of California.
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

#ifndef ns_addr_params
#define ns_addr_params

#include "config.h"


class Address : public TclObject {
 public:
	static Address& instance() { return (*instance_); }
	Address();
	~Address();
	char *print_nodeaddr(int address);
	char *print_portaddr(int address);
	char *get_subnetaddr(int address);
	int   get_lastaddr(int address);
	int   get_nodeaddr(int address);
	int   str2addr(const char *str) const;
	int   create_ipaddr(int nodeid, int portid);
	int   hier_addr(int address, int level);

	inline int nodeshift() {return NodeShift_[levels_];}
	inline int nodemask() {return NodeMask_[levels_];}
	inline int set_word_field(int word, int field, int shift, int mask) const {
		return (((field & mask)<<shift) | ((~(mask << shift)) & word));
	}
	// bpl_ or bits per level, 	
	// e.g 32 for level=1, 10/11/11 for level=3.
	// used to check validity of address-str.
	int *bpl_;
	int *NodeShift_;
	int *NodeMask_;
	int McastShift_;
	int McastMask_;
	int levels_;
 protected:
	int command(int argc, const char*const* argv);
	static Address* instance_;
	
};

#endif /* ns_addr_params */
