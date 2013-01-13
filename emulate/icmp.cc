/*    
 * Copyright (c) 1998 Regents of the University of California.
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
 *      This product includes software developed by the Network Research
 *      Group at Lawrence Berkeley National Laboratory.
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
    "@(#) $Header: /cvsroot/nsnam/ns-2/emulate/icmp.cc,v 1.5 2000/09/01 03:04:10 haoboy Exp $";
#endif

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>

#include "agent.h"
#include "scheduler.h"
#include "packet.h"
#include "emulate/net.h"
#include "emulate/internet.h"

#ifndef IPPROTO_GGP
#define IPPROTO_GGP 3
#endif // IPPROTO_GGP

//
// icmp.cc -- a very limited-functionality set of icmp routines
// 

class IcmpAgent : public Agent {
public:
	IcmpAgent();
	void recv(Packet*, Handler*) { abort(); }
protected:
	void	sendredirect(in_addr& me, in_addr& target, in_addr& dest, in_addr& gw);
	int	command(int argc, const char*const* argv);

	int	ttl_;
};

static class IcmpAgentClass : public TclClass { 
public:
        IcmpAgentClass() : TclClass("Agent/IcmpAgent") {}
        TclObject* create(int , const char*const*) {
                return (new IcmpAgent());
        } 
} class_icmpsagent;

IcmpAgent::IcmpAgent() : Agent(PT_LIVE)
{
	bind("ttl_", &ttl_);
}

/*
 * sendredirect -- send a packet to "target" containing a redirect
 * for the network specified by "dst", so that the gateway "gw" is used
 * also, forge the source address so as to appear to come from "me"
 */

void
IcmpAgent::sendredirect(in_addr& me, in_addr& target, in_addr& dst, in_addr& gw)
{
	// make a simulator packet to hold the IP packet, which in turn
	// holds: ip header, icmp header, embedded ip header, plus 64 bits
	// data
	int iplen = sizeof(ip) + 8 + sizeof(ip) + 8;
        Packet* p = allocpkt(iplen);
	hdr_cmn* hc = HDR_CMN(p);
	ip* iph = (ip*) p->accessdata();
	hc->size() = iplen;

	// make an IP packet ready to send to target
	// size will be min icmp + a dummy'd-up IP header
	Internet::makeip(iph, iplen, ttl_, IPPROTO_ICMP, me, target);

	// make an ICMP host redirect, set the gwaddr field
	icmp* icp = (icmp*) (iph + 1);
	icp->icmp_gwaddr = gw;

	// make a dummy IP packet to go in the ICMP data, which will
	// be used to indicate to the end host which routing table
	// entry to update

	ip* dummyhdr = (ip*)((u_char*)icp + 8);	// past icmp hdr
		// deprecated protocol inside
	Internet::makeip(dummyhdr, 20, 254, IPPROTO_GGP, target, dst);
	u_short *port = (u_short*) (dummyhdr + 1);	// past ip hdr
	*port++ = htons(9);	// discard port
	*port = htons(9);	// discard port
	icp->icmp_cksum = 0;
	icp->icmp_type = ICMP_REDIRECT;
	icp->icmp_code = ICMP_REDIRECT_HOST;
	icp->icmp_cksum = Internet::in_cksum((u_short*)icp,
		8 + sizeof(ip) + 8);

	send(p, 0);
	return;
}

int
IcmpAgent::command(int argc, const char*const* argv)
{
	if (argc > 5) {
		// $obj send name src dst [...stuff...]
		if (strcmp(argv[1], "send") == 0) {
			if (strcmp(argv[2], "redirect") == 0 &&
			    argc == 7) {
				// $obj send redirect src target dst gwaddr
				// as src, send to targ, so that it changes
				// its route to dst to use gwaddr
				u_long s, t, d, g;
				s = inet_addr(argv[3]);
				t = inet_addr(argv[4]);
				d = inet_addr(argv[5]);
				g = inet_addr(argv[6]);
				in_addr src, targ, dst, gw;
				src.s_addr = s;
				targ.s_addr = t;
				dst.s_addr = d;
				gw.s_addr = g;
				sendredirect(src, targ, dst, gw);
				return (TCL_OK);
			}
		}
	}
	return (Agent::command(argc, argv));
}
