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
 * Contributed by the Daedalus Research Group, U.C.Berkeley
 * http://daedalus.cs.berkeley.edu
 */

/*
 * tcp-asym-fs: TCP mods for asymmetric networks (tcp-asym) with fast start
 * (tcp-fs).
 * 
 * Contributed by Venkat Padmanabhan (padmanab@cs.berkeley.edu), 
 * Daedalus Research Group, U.C.Berkeley
 */

#include "tcp-asym.h"
#include "tcp-fs.h"

/* TCP-FS with NewReno */
class NewRenoTcpAsymFsAgent : public NewRenoTcpAsymAgent, public NewRenoTcpFsAgent {
public:
	NewRenoTcpAsymFsAgent() : NewRenoTcpAsymAgent(), NewRenoTcpFsAgent() {count_bytes_acked_= 1;}

	/* helper functions */
	virtual void output_helper(Packet* pkt) {NewRenoTcpAsymAgent::output_helper(pkt); NewRenoTcpFsAgent::output_helper(pkt);}
	virtual void recv_helper(Packet* pkt) {NewRenoTcpAsymAgent::recv_helper(pkt); NewRenoTcpFsAgent::recv_helper(pkt);}
	virtual void send_helper(int maxburst) {NewRenoTcpFsAgent::send_helper(maxburst);}
	virtual void send_idle_helper() {NewRenoTcpFsAgent::send_idle_helper();}
	virtual void recv_newack_helper(Packet* pkt) {printf("count_bytes_acked_=%d\n", count_bytes_acked_); NewRenoTcpFsAgent::recv_newack_helper(pkt); NewRenoTcpAsymAgent::t_exact_srtt_ = NewRenoTcpFsAgent::t_exact_srtt_;}
	virtual void partialnewack_helper(Packet* pkt) {NewRenoTcpFsAgent::partialnewack_helper(pkt);}

	virtual void set_rtx_timer() {NewRenoTcpFsAgent::set_rtx_timer();}
	virtual void timeout_nonrtx(int tno) {NewRenoTcpFsAgent::timeout_nonrtx(tno);}
	virtual void timeout_nonrtx_helper(int tno) {NewRenoTcpFsAgent::timeout_nonrtx_helper(tno);}
};

static class NewRenoTcpAsymFsClass : public TclClass {
public:
	NewRenoTcpAsymFsClass() : TclClass("Agent/TCP/Newreno/Asym/FS") {}
	TclObject* create(int, const char*const*) {
		return (new NewRenoTcpAsymFsAgent());
	}
} class_newrenotcpasymfs;	



