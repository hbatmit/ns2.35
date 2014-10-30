/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linking this file statically or dynamically with other modules is making
 * a combined work based on this file.  Thus, the terms and conditions of
 * the GNU General Public License cover the whole combination.
 *
 * In addition, as a special exception, the copyright holders of this file
 * give you permission to combine this file with free software programs or
 * libraries that are released under the GNU LGPL and with code included in
 * the standard release of ns-2 under the Apache 2.0 license or under
 * otherwise-compatible licenses with advertising requirements (or modified
 * versions of such code, with unchanged license).  You may copy and
 * distribute such a system following the terms of the GNU GPL for this
 * file and the licenses of the other code concerned, provided that you
 * include the source code of that other code when and as the GNU GPL
 * requires distribution of source code.
 *
 * Note that people who make modified versions of this file are not
 * obligated to grant this special exception for their modified versions;
 * it is their choice whether to do so.  The GNU General Public License
 * gives permission to release a modified version without this exception;
 * this exception also makes it possible to release a modified version
 * which carries forward this exception.
 */

/*
 * Copyright (c) 1994 Regents of the University of California.
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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/adc/simple-intserv-sched.cc,v 1.7 2005/08/26 05:05:28 tomh Exp $ (LBL)";
#endif


//Simple scheduler with 2 service priority levels and protects signalling
//ctrl  packets


#include "config.h"
#include "queue.h"

#define CLASSES 2

class SimpleIntServ : public Queue {
public:
	SimpleIntServ() {
		int i;
		char buf[10];
		for (i=0;i<CLASSES;i++) {
			q_[i] = new PacketQueue;
			qlimit_[i] = 0;
			sprintf(buf,"qlimit%d_",i);
			bind(buf,&qlimit_[i]);
		}
	}
	protected :
	void enque(Packet *);
	Packet *deque();
	PacketQueue *q_[CLASSES];
	int qlimit_[CLASSES];
};


static class SimpleIntServClass : public TclClass {
public:
	SimpleIntServClass() : TclClass("Queue/SimpleIntServ") {}
	TclObject* create(int, const char*const*) {
		return (new SimpleIntServ);
	}
} class_simple_intserv;

void SimpleIntServ::enque(Packet* p)
{
	
	hdr_ip* iph=hdr_ip::access(p);
	int cl=(iph->flowid()) ? 1:0;
	
	if (q_[cl]->length() >= (qlimit_[cl]-1)) {
		hdr_cmn* ch=hdr_cmn::access(p);
		packet_t ptype = ch->ptype();
		if ( (ptype != PT_REQUEST) && (ptype != PT_REJECT) && (ptype != PT_ACCEPT) && (ptype != PT_CONFIRM) && (ptype != PT_TEARDOWN) ) {
			drop(p);
		}
		else {
			q_[cl]->enque(p);
		}
	}
	else {
		q_[cl]->enque(p);
	}
}


Packet *SimpleIntServ::deque()
{
	int i;
	for (i=CLASSES-1;i>=0;i--)
		if (q_[i]->length())
			return q_[i]->deque();
	return 0;
}

