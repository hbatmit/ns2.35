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
    "@(#) $Header: /cvsroot/nsnam/ns-2/emulate/arp.cc,v 1.8 2000/11/09 17:42:23 haoboy Exp $";
#endif

#include "object.h"
#include "packet.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <memory.h>
#include <stdio.h>
#include <errno.h>

#include "emulate/net.h"
#include "emulate/ether.h"
#include "emulate/internet.h"

// Very very very back hack. Should put this detection in autoconf.
#ifndef ether_aton
extern "C" {
ether_addr* ether_aton(const char *);
}
#endif

//
// arp.cc -- this object may be used within nse as
// an ARP requestor/responder.  Only the request side
// is implemented now [5/98]
// 

class ArpAgent : public NsObject, public IOHandler {
public:
	ArpAgent();
	~ArpAgent();

protected:
	struct acache_entry {
		in_addr	ip;
		ether_addr ether;
		char code;	// 'D' - dynamic, 'P' - publish
	};
		
	char	icode(const char*);
	acache_entry* find(in_addr&);
	void 	insert(in_addr&, ether_addr&, char code);
	void	dispatch(int);
	int	sendreq(in_addr&);
	int	sendresp(ether_addr&, in_addr&, ether_addr&);
	int	resolve(const char* host, char*& result, int sendreq);

	void	doreq(ether_arp*);
	void	doreply(ether_arp*);

	void recv(Packet*, Handler*) { abort(); }

	int command(int, const char*const*);

	Network*	net_;
	ether_header	eh_template_;
	ether_arp	ea_template_;
	ether_addr	my_ether_;
	in_addr		my_ip_;
	int		base_size_;	// size of rcv buf
	u_char*		rcv_buf_;
	acache_entry*	acache_;	// arp mapping cache
	int		nacache_;	// # entries in cache
	int		cur_;		// cur posn in cache
	int		pending_;	// resolve pending?
};

static class ArpAgentClass : public TclClass { 
public:
        ArpAgentClass() : TclClass("ArpAgent") {}
        TclObject* create(int , const char*const*) {
                return (new ArpAgent());
        } 
} class_arpagent;

ArpAgent::ArpAgent() : net_(NULL), pending_(0)
{
	/* dest addr is broadcast */
	eh_template_.ether_dhost[0] = 0xff;
	eh_template_.ether_dhost[1] = 0xff;
	eh_template_.ether_dhost[2] = 0xff;
	eh_template_.ether_dhost[3] = 0xff;
	eh_template_.ether_dhost[4] = 0xff;
	eh_template_.ether_dhost[5] = 0xff;
	/* src addr is mine */
	memcpy(&eh_template_.ether_shost, &my_ether_, ETHER_ADDR_LEN);
	/* type is ARP */
	eh_template_.ether_type = htons(ETHERTYPE_ARP);

	ea_template_.ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
	ea_template_.ea_hdr.ar_pro = htons(ETHERTYPE_IP);
	ea_template_.ea_hdr.ar_hln = ETHER_ADDR_LEN;
	ea_template_.ea_hdr.ar_pln = 4;			/* ip addr len */
	ea_template_.ea_hdr.ar_op = htons(ARPOP_REQUEST);	
	memcpy(&ea_template_.arp_sha, &my_ether_, ETHER_ADDR_LEN);	/* sender hw */
	memset(&ea_template_.arp_spa, 0, 4);			/* sender IP */
	memset(&ea_template_.arp_tha, 0, ETHER_ADDR_LEN);	/* target hw */
	memset(&ea_template_.arp_tpa, 0, 4);			/* target hw */
	base_size_ = sizeof(eh_template_) + sizeof(ea_template_);
	rcv_buf_ = new u_char[base_size_];

	bind("cachesize_", &nacache_);
	acache_ = new acache_entry[nacache_];
	memset(acache_, 0, nacache_*sizeof(acache_entry));
	cur_ = nacache_;
}

ArpAgent::~ArpAgent()
{
	delete[] rcv_buf_;
	delete[] acache_;
}

ArpAgent::acache_entry*
ArpAgent::find(in_addr& target)
{
	int n = nacache_;
	acache_entry* ae = &acache_[n-1];
	while (--n >= 0) {
		if (ae->ip.s_addr == target.s_addr) {
			return (ae);
		}
		--ae;
	}
	return (NULL);
}

char
ArpAgent::icode(const char *how)
{
	if (strcmp(how, "publish") == 0)
		return 'P';

	return 'D';
}

void
ArpAgent::insert(in_addr& target, ether_addr& eaddr, char code)
{
	acache_entry* ae;
	if (--cur_ < 0)
		cur_ = nacache_ - 1;

	ae = &acache_[cur_];
	ae->ip = target;
	ae->ether = eaddr;
	ae->code = code;
//printf("INSERTED inet %s, ether %s\n",
//inet_ntoa(target), Ethernet::etheraddr_string((u_char*)&eaddr));
	return;
}
		
int
ArpAgent::sendreq(in_addr& target)
{
	int pktsz = sizeof(eh_template_) + sizeof(ea_template_);
	if (pktsz < 64)
		pktsz = 64;
	u_char* buf = new u_char[pktsz];
	memset(buf, 0, pktsz);

	ether_header* eh = (ether_header*) buf;
	ether_arp* ea = (ether_arp*) (buf + sizeof(eh_template_));
	*eh = eh_template_;	/* set ether header */
	*ea = ea_template_;	/* set ether/IP arp pkt */
	memcpy(ea->arp_tpa, &target, sizeof(target));

	if (net_->send(buf, pktsz) < 0) {
                fprintf(stderr,
                    "ArpAgent(%s): sendpkt (%p, %d): %s\n",
                    name(), buf, pktsz, strerror(errno));
                return (-1);
	}
	delete[] buf;
	return (0);
}

/*
 * resp: who to send response to
 * tip: the IP address we are responding for
 * tea: the ether address we want to advertise with tip
 */

int
ArpAgent::sendresp(ether_addr& dest, in_addr& tip, ether_addr& tea)
{
	int pktsz = sizeof(eh_template_) + sizeof(ea_template_);
	if (pktsz < 64)
		pktsz = 64;
	u_char* buf = new u_char[pktsz];
	memset(buf, 0, pktsz);

	ether_header* eh = (ether_header*) buf;
	ether_arp* ea = (ether_arp*) (buf + sizeof(eh_template_));

	// destination link layer address is back to sender
	// (called dest here)
	*eh = eh_template_;	/* set ether header */
	memcpy(eh->ether_dhost, &dest, ETHER_ADDR_LEN);

	// set code as ARP reply
	*ea = ea_template_;	/* set ether/IP arp pkt */
	ea->ea_hdr.ar_op = htons(ARPOP_REPLY);

	// make it look like a regular arp reply
	memcpy(ea->arp_tpa, ea->arp_spa, sizeof(in_addr));
	memcpy(ea->arp_tha, ea->arp_sha, sizeof(in_addr));

	memcpy(ea->arp_sha, &tea, ETHER_ADDR_LEN);
	memcpy(ea->arp_spa, &tip, ETHER_ADDR_LEN);

	if (net_->send(buf, pktsz) < 0) {
                fprintf(stderr,
                    "ArpAgent(%s): sendpkt (%p, %d): %s\n",
                    name(), buf, pktsz, strerror(errno));
                return (-1);
	}
	delete[] buf;
	return (0);
}

/*
 * receive pkt from network:
 *	note that net->recv() gives us the pkt starting
 *	just BEYOND the frame header
 */
void
ArpAgent::dispatch(int)
{
	double ts;
	sockaddr sa;
	int cc = net_->recv(rcv_buf_, base_size_, sa, ts);
	if (cc < int(base_size_ - sizeof(ether_header))) {
		if (cc == 0)
			return;
                fprintf(stderr,
                    "ArpAgent(%s): recv small pkt (%d) [base sz:%d]: %s\n",
                    name(), cc, base_size_, strerror(errno));
		return;
	}
	ether_arp* ea = (ether_arp*) rcv_buf_;
	int op = ntohs(ea->ea_hdr.ar_op);


	switch (op) {
	case ARPOP_REPLY:
		doreply(ea);
		break;
	case ARPOP_REQUEST:
		doreq(ea);
		break;
	default:
		fprintf(stderr,
		    "ArpAgent(%s): cannot interpret ARP op %d\n",
		    name(), op);
		return;
	}
	return;
}

/*
 * process an ARP reply frame -- insert into cache
 */
void
ArpAgent::doreply(ether_arp* ea)
{
	/*
	 * reply will be from the replier's point of view,
	 * so, look in the sender ha/pa fields for the info
	 * we want
	 */
	in_addr t;
	ether_addr e;
	memcpy(&t, ea->arp_spa, 4);	// copy IP address
	memcpy(&e, ea->arp_sha, ETHER_ADDR_LEN);
	insert(t, e, 'D');
	return;
}

/*
 * process an ARP request frame
 */

void
ArpAgent::doreq(ether_arp* ea)
{
	in_addr t;
	memcpy(&t, ea->arp_tpa, 4);	// requested IP addr

	acache_entry *ae;
	if ((ae = find(t)) == NULL) {
//printf("doreq: didn't find mapping for IP addr %s\n",
//inet_ntoa(t));
		return;
	}

	if (ae->code == 'P') {
		// return answer to the sender's hardware addr
		ether_addr dst;
		memcpy(&dst, ea->arp_sha, ETHER_ADDR_LEN);
		sendresp(dst, t, ae->ether);
	}
	return;
}


int
ArpAgent::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
                if (strcmp(argv[1], "network") == 0) { 
			if (net_ == NULL)
				tcl.result("");
			else
				tcl.result(net_->name());
			return (TCL_OK);
		}
	} else if (argc == 3) {
                if (strcmp(argv[1], "network") == 0) { 
                        net_ = (Network *)TclObject::lookup(argv[2]);
                        if (net_ != 0) { 
				link(net_->rchannel(), TCL_READABLE);
				return (TCL_OK);
                        } else {
                                fprintf(stderr,
                                "ArpAgent(%s): unknown network %s\n",
                                    name(), argv[2]);
                                return (TCL_ERROR);
                        }       
                        return(TCL_OK);
                }       
		if (strcmp(argv[1], "myether") == 0) {
			my_ether_ = *(::ether_aton((char*)argv[2]));
			memcpy(&eh_template_.ether_shost, &my_ether_,
				ETHER_ADDR_LEN);
			memcpy(&ea_template_.arp_sha,
				&my_ether_, ETHER_ADDR_LEN);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "myip") == 0) {
			u_long a = inet_addr(argv[2]);
			if (a == 0)
				return (TCL_ERROR);
			in_addr ia;
			ia.s_addr = a;
			my_ip_ = ia;
			memcpy(&ea_template_.arp_spa,
				&my_ip_, 4);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "lookup") == 0) {
			char *p = NULL;
			if (resolve(argv[2], p, 0) < 0)
				return (TCL_ERROR);
			if (p)
				tcl.result(p);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "resolve") == 0) {
			char *p = NULL;
			if (resolve(argv[2], p, 1) < 0)
				return (TCL_ERROR);
			if (p)
				tcl.resultf("%s", p);
			return (TCL_OK);
		}
	} else if (argc == 5) {
		// $obj insert iaddr eaddr how
		if (strcmp(argv[1], "insert") == 0) {
			u_long a = inet_addr(argv[2]);
			if (a == 0)
				return (TCL_ERROR);
			in_addr ia;
			ia.s_addr = a;
			ether_addr ea = *(::ether_aton((char*)argv[3]));
			insert(ia, ea, icode(argv[4]));
			return (TCL_OK);
		}
	}

	return (NsObject::command(argc, argv));
}

int
ArpAgent::resolve(const char* host, char*& result, int doreq)
{
	u_long a = inet_addr(host);
	in_addr ia;
	ia.s_addr = a;
	acache_entry* ae;
	if ((ae = find(ia)) == NULL) {
		result = NULL;
		if (doreq)
			return(sendreq(ia));
		return (0);
	}
	result = Ethernet::etheraddr_string((u_char*) &ae->ether);
	return (1);
}
