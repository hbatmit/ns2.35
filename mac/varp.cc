/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1996 Regents of the University of California.
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
 *	Engineering Group at Lawrence Berkeley Laboratory and the Daedalus
 *	research group at UC Berkeley.
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

#include "varp.h"
#include "mac.h"
#include "packet.h"


static class VARPTableClass : public TclClass {
public:
 	VARPTableClass() : TclClass("VARPTable") {}
 	TclObject* create(int, const char*const*) {
 		return (new VARPTable);
 	}
} class_VARPTable;


VARPTable::VARPTable(void) : TclObject(), maddr_(0), size_(0)
{}


VARPTable::~VARPTable(void) 
{
	delete [] maddr_;
}

void VARPTable::sizeinit(int n)
{
	int *temp = maddr_;
	int osize = size_;
	int i; 
	
	if (size_ == 0)
		size_ = 10;
	while(!(n < size_))
			size_ = 2*size_;
	maddr_ = new int[size_];
	for (i=0;i<osize;i++)
		maddr_[i] = temp[i];
	for (i=osize;i<size_;i++)
		maddr_[i] = -1;
	delete [] temp;
}

int VARPTable::command(int argc, const char*const* argv) {
	if (argc == 4) {
		if (strcmp(argv[1], "mac-addr") == 0) {
			int n = atoi(argv[2]);
			if(!(n < size_))
				sizeinit(n);
			
			maddr_[n] = atoi(argv[3]);
			return (TCL_OK);
		}
	}
	return TclObject::command(argc, argv);
}

int VARPTable::arpresolve(int IPaddr, Packet* p) {
	if (IPaddr > size_)
		return 1;
	int ma = maddr_[IPaddr];
	if (ma >= 0) {
		HDR_MAC(p)->macDA_ = ma;
		return 0;
	}
	return 1;
}
