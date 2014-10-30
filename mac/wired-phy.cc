/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-  *
 *
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
 * @(#) $Header: 
 */

#include <wired-phy.h>


static class WiredPhyClass: public TclClass {
public:
        WiredPhyClass() : TclClass("Phy/WiredPhy") {}
        TclObject* create(int, const char*const*) {
                return (new WiredPhy);
        }
} class_WiredPhy;

WiredPhy::WiredPhy(void) : Phy() 
{
	//propagation_ = 0;
	//bandwidth_ = 10*1e6;		// 10M
	bind_bw("bandwidth_", &bandwidth_);
}
	

int
WiredPhy::command(int argc, const char*const* argv) {
	if (argc == 3) {
		TclObject *obj;
		if( (obj = TclObject::lookup(argv[2])) == 0) {
			fprintf(stderr, "%s lookup failed\n", argv[1]);
			return TCL_ERROR;
		}

		//if (strcmp(argv[1], "propagation") == 0) {
		//assert(propagation_ == 0);
		//propagation_ = (TclObject*) obj;
		//return TCL_OK;
		//}
	}
	
	return Phy::command(argc, argv);
}



void WiredPhy::sendDown(Packet *p)
{
	/*
	 * Sanity Check
	 */
	assert(initialized());

	/*
	 *  Stamp the packet with the interface arguments, txtime and 
	 *  info reqd by recving interface to determine 
	 *  collision/contention
	 */
	
	
	// and then send the packet
	channel_->recv(p, this);
}


int WiredPhy::sendUp(Packet *)
{
	/*
	 * Sanity Check
	 */
	assert(initialized());
	
	/* check with propagation model for collision, channel 
	   contention */
	/* Return one if pkt recv , else return zero.*/
	return 1;
}

	
	
	
