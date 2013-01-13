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
    "@(#) $Header: /cvsroot/nsnam/ns-2/emulate/ping_responder.cc,v 1.8 2000/09/01 03:04:10 haoboy Exp $";
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
#include "emulate/internet.h"

//
// ping_responder.cc -- this agent may be inserted into nse as
// a real-world responder to ICMP ECHOREQUEST operations.  It's
// used to test emulation mode, mostly (and in particular the rt scheduler)
// 

class PingResponder : public Agent {
public:
	PingResponder() : Agent(PT_LIVE) { }
	void recv(Packet*, Handler*);
protected:
	icmp* validate(int, ip*);
	void reflect(ip*);
};

static class PingResponderClass : public TclClass { 
public:
        PingResponderClass() : TclClass("Agent/PingResponder") {}
        TclObject* create(int , const char*const*) {
                return (new PingResponder());
        } 
} class_pingresponder;

/*
 * receive an ICMP echo request packet from the simulator.
 * Actual IP packet is in the "data" portion of the packet, and
 * is assumed to start with the IP header
 */

void
PingResponder::recv(Packet* pkt, Handler*)
{
	hdr_cmn *ch = HDR_CMN(pkt);
	int psize = ch->size();
	u_char* payload = pkt->accessdata();

	if (payload == NULL) {
		fprintf(stderr, "%f: PingResponder(%s): recv NULL data area\n",
			Scheduler::instance().clock(), name());
		Packet::free(pkt);
		return;
	}

	/*
	 * assume here that the data area contains an IP header!
	 */

	icmp* icmph;
	if ((icmph = validate(psize, (ip*) payload)) == NULL) {
		Internet::print_ip((ip*) payload);
		Packet::free(pkt);
		return;
	}


	/*
	 * tasks: change icmp type to echo-reply, recompute IP hdr cksum,
	 * recompute ICMP cksum
	 */

	icmph->icmp_type = ICMP_ECHOREPLY;
	reflect((ip*) payload);		// like kernel routine icmp_reflect()

	/*
	 * set up simulator packet to go to the correct place
	 */

	Agent::initpkt(pkt);
	ch->size() = psize; // will have been overwrittin by initpkt

	target_->recv(pkt);
	return;
}

/*
 * check a bunch of stuff:
 *	ip vers ok, ip hlen is 5, proto is icmp, len big enough,
 *	not fragmented, cksum ok, saddr not mcast/bcast
 */

icmp*
PingResponder::validate(int sz, ip* iph)
{
	if (sz < 20) {
		fprintf(stderr, "%f: PingResponder(%s): sim pkt too small for base IP header(%d)\n",
			Scheduler::instance().clock(), name(), sz);
		return (NULL);
	}

	int ipver = (*((char*)iph) & 0xf0) >> 4;
	if (ipver != 4) {
		fprintf(stderr, "%f: PingResponder(%s): IP bad ver (%d)\n",
			Scheduler::instance().clock(), name(), ipver);
		return (NULL);
	}

	int iplen = ntohs(iph->ip_len);
	int iphlen = (*((char*)iph) & 0x0f) << 2;
	if (iplen < (iphlen + 8)) {
		fprintf(stderr, "%f: PingResponder(%s): IP dgram not long enough (len: %d)\n",
			Scheduler::instance().clock(), name(), iplen);
		return (NULL);
	}

	if (sz < iplen) {
		fprintf(stderr, "%f: PingResponder(%s): IP dgram not long enough (len: %d)\n",
			Scheduler::instance().clock(), name(), iplen);
		return (NULL);
	}

	if (iphlen != 20) {
		fprintf(stderr, "%f: PingResponder(%s): IP bad hlen (%d)\n",
			Scheduler::instance().clock(), name(), iphlen);
		return (NULL);
	}

	if (Internet::in_cksum((u_short*) iph, iphlen)) {
		fprintf(stderr, "%f: PingResponder(%s): IP bad cksum\n",
			Scheduler::instance().clock(), name());
		return (NULL);
	}

	if (iph->ip_p != IPPROTO_ICMP) {
		fprintf(stderr, "%f: PingResponder(%s): not ICMP (proto: %d)\n",
			Scheduler::instance().clock(), name(), iph->ip_p);
		return (NULL);
	}


	if (iph->ip_off != 0) {
		fprintf(stderr, "%f: PingResponder(%s): fragment! (off: 0x%x)\n",
			Scheduler::instance().clock(), name(), ntohs(iph->ip_off));
		return (NULL);
	}

	if (iph->ip_src.s_addr == 0xffffffff || iph->ip_src.s_addr == 0) {
		fprintf(stderr, "%f: PingResponder(%s): bad src addr (%s)\n",
			Scheduler::instance().clock(), name(),
			inet_ntoa(iph->ip_src));
		return (NULL);
	}

	if (IN_MULTICAST(ntohl(iph->ip_src.s_addr))) {
		fprintf(stderr, "%f: PingResponder(%s): mcast src addr (%s)\n",
			Scheduler::instance().clock(), name(),
			inet_ntoa(iph->ip_src));
		return (NULL);
	}
	icmp* icp = (icmp*) (iph + 1);
	if (Internet::in_cksum((u_short*) icp, iplen - iphlen) != 0) {
		fprintf(stderr,
			"%f: PingResponder(%s): bad ICMP cksum\n",
			Scheduler::instance().clock(), name());
		return (NULL);
	}
	if (icp->icmp_type != ICMP_ECHO) {
		fprintf(stderr, "%f: PingResponder(%s): not echo request (%d)\n",
			Scheduler::instance().clock(), name(),
			icp->icmp_type);
		return (NULL);
	}

	if (icp->icmp_code != 0) {
		fprintf(stderr, "%f: PingResponder(%s): bad code (%d)\n",
			Scheduler::instance().clock(), name(),
			icp->icmp_code);
		return (NULL);
	}
	return (icp);
}

/*
 * reflect: fix up the IP and ICMP info to reflect the packet
 * back from where it came in real life
 *
 * this routine will just assume no IP options on the pkt
 */

void
PingResponder::reflect(ip* iph)
{
	in_addr daddr = iph->ip_dst;
	int iplen = ntohs(iph->ip_len);
	int iphlen = (*((char*)iph) & 0x0f) << 2;

	/* swap src and dest IP addresses on IP header */
	iph->ip_dst = iph->ip_src;
	iph->ip_src = daddr;
	iph->ip_sum = 0;
	iph->ip_sum = Internet::in_cksum((u_short*) iph, iphlen);

	/* recompute the icmp cksum */
	icmp* icp = (icmp*)(iph + 1);	// just past standard IP header
	icp->icmp_cksum = 0;
	icp->icmp_cksum = Internet::in_cksum((u_short*)icp, iplen - iphlen);
}
