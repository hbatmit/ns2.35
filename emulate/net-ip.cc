/*-
 * Copyright (c) 1993-1994, 1998
 * The Regents of the University of California.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and the Network Research Group at
 *      Lawrence Berkeley Laboratory.
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
    "@(#) $Header: /cvsroot/nsnam/ns-2/emulate/net-ip.cc,v 1.20 2003/10/12 21:13:09 xuanc Exp $ (LBL)";
#endif

#include <stdio.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <time.h>
#include <errno.h>
#include <string.h>
#ifdef WIN32
#include <io.h>
#define close closesocket
#else
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
typedef int Socket;
#endif
#if defined(sun) && defined(__svr4__)
#include <sys/systeminfo.h>
#endif

#include "config.h"
#include "net.h"
#include "inet.h"
#include "tclcl.h"
#include "scheduler.h"

//#define	NIPDEBUG	1
#ifdef NIPDEBUG
#define NIDEBUG(x) { if (NIPDEBUG) fprintf(stderr, (x)); }
#define NIDEBUG2(x,y) { if (NIPDEBUG) fprintf(stderr, (x), (y)); }
#define NIDEBUG3(x,y,z) { if (NIPDEBUG) fprintf(stderr, (x), (y), (z)); }
#define NIDEBUG4(w,x,y,z) { if (NIPDEBUG) fprintf(stderr, (w), (x), (y), (z)); }
#define NIDEBUG5(v,w,x,y,z) { if (NIPDEBUG) fprintf(stderr, (v), (w), (x), (y), (z)); }
#else
#define NIDEBUG(x) { }
#define NIDEBUG2(x,y) { }
#define NIDEBUG3(x,y,z) { }
#define NIDEBUG4(w,x,y,z) { }
#define NIDEBUG5(v,w,x,y,z) { }
#endif


/*
 * Net-ip.cc: this file defines the IP and IP/UDP network
 * objects.  IP provides a raw IP interface and support functions
 * [such as setting multicast parameters].  IP/UDP provides a standard
 * UDP datagram interface.
 */

//
// IPNetwork: a low-level (raw) IP network object
//

class IPNetwork : public Network {
public:
	IPNetwork();

        inline int ttl() const { return (mttl_); }	// current mcast ttl
	inline int noloopback_broken() {		// no loopback filter?
		return (noloopback_broken_);
	}
	int setmttl(Socket, int);			// set mcast ttl
	int setmloop(Socket, int);			// set mcast loopback
	int command(int argc, const char*const* argv);	// virtual in Network
	inline Socket rchannel() { return(rsock_); }	// virtual in Network
	inline Socket schannel() { return(ssock_); }	// virtual in Network

        int send(u_char* buf, int len);			// virtual in Network
        int recv(u_char* buf, int len, sockaddr& from, double& ); // virtual in Network

        inline in_addr& laddr() { return (localaddr_); }
        inline in_addr& dstaddr() { return (destaddr_); }

	int add_membership(Socket, in_addr& grp);	// join mcast
	int drop_membership(Socket, in_addr& grp);	// leave mcast

	/* generally useful routines */

	static int bindsock(Socket, in_addr&, u_int16_t, sockaddr_in&);
	static int connectsock(Socket, in_addr&, u_int16_t, sockaddr_in&);
	static int rbufsize(Socket, int);
	static int sbufsize(Socket, int);

protected:
	in_addr destaddr_;		// remote side, if set (network order)
	in_addr localaddr_;		// local side (network order)
        int mttl_;			// multicast ttl to use
	Socket rsock_;			// socket to receive on
	Socket ssock_;			// socket to send on
        int noloopback_broken_;		// couldn't turn (off) mcast loopback
	int loop_;			// do we want loopbacks?
					// (system usually assumes yes)

	void reset(int reconfigure);			// reset + reconfig?
	virtual int open(int mode);	// open sockets/endpoints
	virtual void reconfigure();	// restore state after reset
	int close();

	time_t last_reset_;
};

class UDPIPNetwork : public IPNetwork {
public:
	UDPIPNetwork();

	int send(u_char*, int);
	int recv(u_char*, int, sockaddr&, double&);
	int open(int mode);			// mode only

	int command(int argc, const char*const* argv);
	void reconfigure();
	void add_membership(Socket, in_addr&, u_int16_t); // udp version
protected:
	int bind(in_addr&, u_int16_t port);	// bind to addr/port, mcast ok
	int connect(in_addr& remoteaddr, u_int16_t port);	// connect()
        u_int16_t lport_;	// local port (network order)
        u_int16_t port_;	// remote (dst) port (network order)
};

static class IPNetworkClass : public TclClass {
    public:
	IPNetworkClass() : TclClass("Network/IP") {}
	TclObject* create(int, const char*const*) {
		return (new IPNetwork);
	}
} nm_ip;

static class UDPIPNetworkClass : public TclClass {
    public:
	UDPIPNetworkClass() : TclClass("Network/IP/UDP") {}
	TclObject* create(int, const char*const*) {
		return (new UDPIPNetwork);
	}
} nm_ip_udp;

IPNetwork::IPNetwork() :
	mttl_(0),
        rsock_(-1), 
        ssock_(-1),
        noloopback_broken_(0),
	loop_(1)
{
	localaddr_.s_addr = 0L;
	destaddr_.s_addr = 0L;
	NIDEBUG("IPNetwork: ctor\n");
}

UDPIPNetwork::UDPIPNetwork() :
        lport_(htons(0)), 
        port_(htons(0))
{
	NIDEBUG("UDPIPNetwork: ctor\n");
}

/*
 * UDPIP::send -- send "len" bytes in buffer "buf" out the sending
 * channel.
 *
 * returns the number of bytes written
 */
int
UDPIPNetwork::send(u_char* buf, int len)
{
	int cc = ::send(schannel(), (char*)buf, len, 0);
	NIDEBUG5("UDPIPNetwork(%s): ::send(%d, buf, %d) returned %d\n",
		name(), schannel(), len, cc);

	if (cc < 0) {
		switch (errno) {
		case ECONNREFUSED:
			/* no one listening at some site - ignore */
#if defined(__osf__) || defined(_AIX) || defined(__FreeBSD__)
			/*
			 * Here's an old comment...
			 *
			 * Due to a bug in kern/uipc_socket.c, on several
			 * systems, datagram sockets incorrectly persist
			 * in an error state on receipt of an ICMP
			 * port-unreachable.  This causes unicast connection
			 * rendezvous problems, and worse, multicast
			 * transmission problems because several systems
			 * incorrectly send port unreachables for 
			 * multicast destinations.  Our work around
			 * is to simply close and reopen the socket
			 * (by calling reset() below).
			 *
			 * This bug originated at CSRG in Berkeley
			 * and was present in the BSD Reno networking
			 * code release.  It has since been fixed
			 * in 4.4BSD and OSF-3.x.  It is known to remain
			 * in AIX-4.1.3.
			 *
			 * A fix is to change the following lines from
			 * kern/uipc_socket.c:
			 *
			 *	if (so_serror)
			 *		snderr(so->so_error);
			 *
			 * to:
			 *
			 *	if (so->so_error) {
			 * 		error = so->so_error;
			 *		so->so_error = 0;
			 *		splx(s);
			 *		goto release;
			 *	}
			 *
			 */
			reset(1);
#endif
			break;

		case ENETUNREACH:
		case EHOSTUNREACH:
			/*
			 * These "errors" are totally meaningless.
			 * There is some broken host sending
			 * icmp unreachables for multicast destinations.
			 * UDP probably aborted the send because of them --
			 * try exactly once more.  E.g., the send we
			 * just did cleared the errno for the previous
			 * icmp unreachable, so we should be able to
			 * send now.
			 */
			cc = ::send(schannel(), (char*)buf, len, 0);
			break;

		default:
			fprintf(stderr, "UDPIPNetwork(%s): send failed: %s\n",
				name(), strerror(errno));
			return (-1);
		}
	}
	return cc;	// bytes sent
}
int
UDPIPNetwork::recv(u_char* buf, int len, sockaddr& from, double& ts)
{
	sockaddr_in sfrom;
	int fromlen = sizeof(sfrom);
	int cc = ::recvfrom(rsock_, (char*)buf, len, 0,
			    (sockaddr*)&sfrom, (socklen_t*)&fromlen);
	NIDEBUG5("UDPIPNetwork(%s): ::recvfrom(%d, buf, %d) returned %d\n",
		name(), rsock_, len, cc);
	if (cc < 0) {
		if (errno != EWOULDBLOCK) {
			fprintf(stderr,
				"UDPIPNetwork(%s): recvfrom failed: %s\n",
				name(), strerror(errno));
		}
		return (-1);
	}
	from = *((sockaddr*)&sfrom);

	/*
	 * if we received multicast data and we don't want the look,
	 * there is a chance it is
	 * what we sent if "noloopback_broken_" is set.
	 * If so, filter out the stuff we don't want right here.
	 */
 
	if (!loop_ && noloopback_broken_ &&
	    sfrom.sin_addr.s_addr == localaddr_.s_addr &&
	    sfrom.sin_port == lport_) {
	NIDEBUG2("UDPIPNetwork(%s): filtered out our own pkt\n", name());
		return (0);	// empty
	}

	ts = Scheduler::instance().clock();
	return (cc);	// number of bytes received
}

int
UDPIPNetwork::open(int mode)
{
	if (mode == O_RDONLY || mode == O_RDWR) {
		rsock_ = socket(AF_INET, SOCK_DGRAM, 0);
		if (rsock_ < 0) {
			fprintf(stderr,
	"UDPIPNetwork(%s): open: couldn't open rcv sock\n",
				name());
		}
		nonblock(rsock_);
		int on = 1;
		if (::setsockopt(rsock_, SOL_SOCKET, SO_REUSEADDR, (char *)&on,
				sizeof(on)) < 0) {
			fprintf(stderr,
	"UDPIPNetwork(%s): open: warning: unable set REUSEADDR: %s\n",
				name(), strerror(errno));
		}
#ifdef SO_REUSEPORT
		on = 1;
		if (::setsockopt(rsock_, SOL_SOCKET, SO_REUSEPORT, (char *)&on,
			       sizeof(on)) < 0) {
			fprintf(stderr,
	"UDPIPNetwork(%s): open: warning: unable set REUSEPORT: %s\n",
				name(), strerror(errno));
		}
#endif
		/*
		 * XXX don't need this for the session socket.
		 */	
		if (rbufsize(rsock_, 80*1024) < 0) {
			if (rbufsize(rsock_, 32*1024) < 0) {
				fprintf(stderr,
		"UDPIPNetwork(%s): open: unable to set r bufsize to %d: %s\n",
					name(), 32*1024, strerror(errno));
			}
		}
	}
	if (mode == O_WRONLY || mode == O_RDWR) {
		ssock_ = socket(AF_INET, SOCK_DGRAM, 0);
		if (ssock_ < 0) {
			fprintf(stderr,
	"UDPIPNetwork(%s): open: couldn't open snd sock\n",
				name());
		}
		nonblock(ssock_);
		int firsttry = 80 * 1024;
		int secondtry = 48 * 1024;
		
		if (sbufsize(ssock_, firsttry) < 0) {
			if (sbufsize(ssock_, secondtry) < 0) {
				fprintf(stderr,
  "UDPIPNetwork(%s): open: cannot set send sockbuf size to %d bytes, using default\n",  
				name(), secondtry);
			}
		}

	}
	mode_ = mode;
	NIDEBUG5("UDPIPNetwork(%s): opened network w/mode %d, ssock:%d, rsock:%d\n",
		name(), mode_, rsock_, ssock_);
	return (0);
}

//
// IP/UDP version of add_membership: try binding
//

void
UDPIPNetwork::add_membership(Socket sock, in_addr& addr, u_int16_t port)
{
	int failure = 0;
	sockaddr_in sin;
	if (bindsock(sock, addr, port, sin) < 0)
		failure = 1;
	if (failure) {
		in_addr addr2 = addr;
		addr2.s_addr = INADDR_ANY;
		if (bindsock(sock, addr2, port, sin) < 0)
			failure = 1;
		else
			failure = 0;
	}

	if (IPNetwork::add_membership(sock, addr) < 0)
		failure = 1;

	if (failure) {
		fprintf(stderr,
		"UDPIPNetwork(%s): add_membership: failed bind on mcast addr %s and INADDR_ANY\n",
			name(), inet_ntoa(addr));
	}
}

//
// server-side bind (or mcast subscription)
//
int
UDPIPNetwork::bind(in_addr& addr, u_int16_t port)
{
	NIDEBUG4("UDPIPNetwork(%s): attempt to bind to addr %s, port %d [net order]\n",
		name(), inet_ntoa(addr), ntohs(port));
	if (rsock_ < 0) {
		fprintf(stderr,
		"UDPIPNetwork(%s): bind/listen called before net is open\n",
			name());
		return (-1);
	}
	if (mode_ == O_WRONLY) {
		fprintf(stderr,
		"UDPIPNetwork(%s): attempted bind/listen but net is write-only\n",
			name());
		return (-1);
	}
#ifdef IP_ADD_MEMBERSHIP
        if (IN_CLASSD(ntohl(addr.s_addr))) {
		// MULTICAST case, call UDPIP vers of add_membership
                add_membership(rsock_, addr, port);
        } else
#endif
        {
		// UNICAST case
                sockaddr_in sin;
                if (bindsock(rsock_, addr, port, sin) < 0) {
                        port = ntohs(port);
                        fprintf(stderr,
        "UDPIPNetwork(%s): bind: unable to bind %s [port:%hu]: %s\n",
                                name(), inet_ntoa(addr),
                                port, strerror(errno));
			return (-1);
                }
                /*
                 * MS Windows currently doesn't compy with the Internet Host
                 * Requirements standard (RFC-1122) and won't let us include
                 * the source address in the receive socket demux state.
                 */
#ifndef WIN32
                /*
                 * (try to) connect the foreign host's address to this socket.
                 */
                (void)connectsock(rsock_, addr, 0, sin);
#endif
        }
	localaddr_ = addr;
	lport_ = port;
	return (0);
}

//
// client-side connect
//
int
UDPIPNetwork::connect(in_addr& addr, u_int16_t port)
{
	sockaddr_in sin;
	if (ssock_ < 0) {
		fprintf(stderr,
		"UDPIPNetwork(%s): connect called before net is open\n",
			name());
		return (-1);
	}
	if (mode_ == O_RDONLY) {
		fprintf(stderr,
		"UDPIPNetwork(%s): attempted connect but net is read-only\n",
			name());
		return (-1);
	}

	int rval = connectsock(ssock_, addr, port, sin);
	if (rval < 0)
		return (rval);
	destaddr_ = addr;
	port_ = port;
	last_reset_ = 0;
	return(rval);
}

int
UDPIPNetwork::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		// $udpip port
		if (strcmp(argv[1], "port") == 0) {
			tcl.resultf("%d", ntohs(port_));
			return (TCL_OK);
		}
		// $udpip lport
		if (strcmp(argv[1], "lport") == 0) {
			tcl.resultf("%d", ntohs(lport_));
			return (TCL_OK);
		}
	} else if (argc == 4) {
		// $udpip listen addr port
		// $udpip bind addr port
		if (strcmp(argv[1], "listen") == 0 ||
		    strcmp(argv[1], "bind") == 0) {
			in_addr addr;
			if (strcmp(argv[2], "any") == 0)
				addr.s_addr = INADDR_ANY;
			else
				addr.s_addr = LookupHostAddr(argv[2]);
			u_int16_t port = htons(atoi(argv[3]));
			if (bind(addr, port) < 0) {
				tcl.resultf("%s %hu",
					inet_ntoa(addr), port);
			} else {
				tcl.result("0");
			}
			return (TCL_OK);
		}
		// $udpip connect addr port
		if (strcmp(argv[1], "connect") == 0) {
			in_addr addr;
			addr.s_addr = LookupHostAddr(argv[2]);
			u_int16_t port = htons(atoi(argv[3]));
			if (connect(addr, port) < 0) {
				tcl.resultf("%s %hu",
					inet_ntoa(addr), port);
			} else {
				tcl.result("0");
			}
			return (TCL_OK);
		}
	}
	return (IPNetwork::command(argc, argv));
}

//
// raw IP network recv()
//
int
IPNetwork::recv(u_char* buf, int len, sockaddr& sa, double& ts)
{
	if (mode_ == O_WRONLY) {
		fprintf(stderr,
		    "IPNetwork(%s) recv while in writeonly mode!\n",
			name());
		abort();
	}
	int fromlen = sizeof(sa);
	int cc = ::recvfrom(rsock_, (char*)buf, len, 0, &sa, (socklen_t*)&fromlen);
	if (cc < 0) {
		if (errno != EWOULDBLOCK)
			perror("recvfrom");
		return (-1);
	}
	ts = Scheduler::instance().clock();
	return (cc);
}

//
// we are given a "raw" IP datagram.
// the raw interface appears to want the len and off fields
// in *host* order, so make it this way here
// note also, that it will compute the cksum "for" us... :(
//
int
IPNetwork::send(u_char* buf, int len)
{
	struct ip *ip = (struct ip*) buf;
#ifdef __linux__ 
// For raw sockets on linux the send does not work,
// all packets show up only on the loopback device and are not routed
// to the correct host. Using sendto on a closed socket solves this problem
       ip->ip_len = (ip->ip_len);
       ip->ip_off = (ip->ip_off);
        sockaddr_in sin;
       memset((char *)&sin, 0, sizeof(sin));
       sin.sin_family = AF_INET;
        sin.sin_addr = ip->ip_dst;
       return (::sendto(ssock_, (char*)buf, len, 0,(sockaddr *) &sin,sizeof(sin)));
#else
        ip->ip_len = ntohs(ip->ip_len);
        ip->ip_off = ntohs(ip->ip_off);
        return (::send(ssock_, (char*)buf, len, 0));
#endif
}

int IPNetwork::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "close") == 0) {
			close();
			return (TCL_OK);
		}
		// Old approach uses tcl.result() to get result buffer first
		// char* cp = tcl.result();
		// new approach uses tcl.result(const char*) directly.
		// xuanc, 10/07/2003
		if (strcmp(argv[1], "destaddr") == 0) {
			tcl.result(inet_ntoa(destaddr_));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "localaddr") == 0) {
			tcl.result(inet_ntoa(localaddr_));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "mttl") == 0) {
			tcl.resultf("%d", mttl_);
			return (TCL_OK);
		}
		/* for backward compatability */
		if (strcmp(argv[1], "ismulticast") == 0) {
			tcl.result(IN_CLASSD(ntohl(destaddr_.s_addr)) ?
				"1" : "0");
			return (TCL_OK);
		}
		if (strcmp(argv[1], "addr") == 0) {
			tcl.result(inet_ntoa(destaddr_));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "ttl") == 0) {
			tcl.resultf("%d", mttl_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "interface") == 0) {
			tcl.result(inet_ntoa(localaddr_));
			return (TCL_OK);
		}
	} else if (argc == 3) {
		
		if (strcmp(argv[1], "open") == 0) {
			int mode = parsemode(argv[2]);
			if (open(mode) < 0)
				return (TCL_ERROR);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "add-membership") == 0) {
			in_addr addr;
			addr.s_addr = LookupHostAddr(argv[2]);
			if (add_membership(rchannel(), addr) < 0)
				tcl.result("0");
			else
				tcl.result("1");
			return (TCL_OK);
		}
		if (strcmp(argv[1], "drop-membership") == 0) {
			in_addr addr;
			addr.s_addr = LookupHostAddr(argv[2]);
			if (drop_membership(rchannel(), addr) < 0)
				tcl.result("0");
			else
				tcl.result("1");
			return (TCL_OK);
		}
		if (strcmp(argv[1], "loopback") == 0) {
			int val = atoi(argv[2]);
			if (strcmp(argv[2], "true") == 0)
				val = 1;
			else if (strcmp(argv[2], "false") == 0)
				val = 0;
			if (setmloop(schannel(), val) < 0)
				tcl.result("0");
			else
				tcl.result("1");
			return (TCL_OK);
		}
	}
	return (Network::command(argc, argv));
}
int
IPNetwork::setmttl(Socket s, int ttl)
{
        /* set the multicast TTL */  

#ifdef WIN32
        u_int t = ttl; 
#else 
        u_char t = ttl;
#endif

        t = (ttl > 255) ? 255 : (ttl < 0) ? 0 : ttl; 
        if (::setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL,
                       (char*)&t, sizeof(t)) < 0) {
		fprintf(stderr,
		    "IPNetwork(%s): couldn't set multicast ttl to %d\n",
			name(), t);
                return (-1);
        }
	return (0);
}

/*
 * open a RAW IP socket (will require privilege).
 * turn on HDRINCL, specifying that we will be writing the raw IP header
 */

int
IPNetwork::open(int mode)
{
	// obtain a raw socket we can use to send ip datagrams
	Socket fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (fd < 0) {
		perror("socket(RAW)");
		if (::getuid() != 0 && ::geteuid() != 0) {
			fprintf(stderr,
	  "IPNetwork(%s): open: use of the Network/IP object requires super-user privs\n",
			name());
		}

		return (-1);
	}

	// turn on HDRINCL option (we will be writing IP header)
	// in FreeBSD 2.2.5 (and possibly others), the IP id field
	// is set by the kernel routine rip_output()
	// only if it is non-zero, so we should be ok.
	int one = 1;
	if (::setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
		fprintf(stderr,
	"IPNetwork(%s): open: unable to turn on IP_HDRINCL: %s\n",
			name(), strerror(errno));
		return (-1);
	}
#ifndef __linux__
	// sort of curious, but do a connect() even though we have
	// HDRINCL on.  Otherwise, we get ENOTCONN when doing a send()
	sockaddr_in sin;
	in_addr ia = { INADDR_ANY };
	if (connectsock(fd, ia, 0, sin) < 0) {
		fprintf(stderr,
	"IPNetwork(%s): open: unable to connect : %s\n",
			name(), strerror(errno));
	}
#endif
	rsock_ = ssock_ = fd;
	mode_ = mode;
	NIDEBUG5("IPNetwork(%s): opened with mode %d, rsock_:%d, ssock_:%d\n",
		name(), mode_, rsock_, ssock_);
	return 0;
}

/*
 * close both sending and receiving sockets
 */

int
IPNetwork::close()
{
	if (ssock_ >= 0) {
		(void)::close(ssock_);
		ssock_ = -1;
	}
	if (rsock_ >= 0) {
		(void)::close(rsock_);
		rsock_ = -1;
	}
	return (0);
}

/*
 * add multicast group membership on the socket
 */

int
IPNetwork::add_membership(Socket fd, in_addr& addr)
{

#if defined(IP_ADD_MEMBERSHIP)
	if (IN_CLASSD(ntohl(addr.s_addr))) {
#ifdef notdef
		/*
		 * Try to bind the multicast address as the socket
		 * dest address.  On many systems this won't work
		 * so fall back to a destination of INADDR_ANY if
		 * the first bind fails.
		 */
		sockaddr_in sin;
		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_addr = addr;
		if (::bind(fd, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
			sin.sin_addr.s_addr = INADDR_ANY;
			if (::bind(fd, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
				fprintf(stderr,
	"IPNetwork(%s): add_membership: unable to bind to addr %s: %s\n",
					name(), inet_ntoa(sin.sin_addr),
					strerror(errno));
				return (-1);
			}
		}
#endif
		/* 
		 * XXX This is bogus multicast setup that really
		 * shouldn't have to be done (group membership should be
		 * implicit in the IP class D address, route should contain
		 * ttl & no loopback flag, etc.).  Steve Deering has promised
		 * to fix this for the 4.4bsd release.  We're all waiting
		 * with bated breath.
		 */
		struct ip_mreq mr;

		mr.imr_multiaddr = addr;
		mr.imr_interface.s_addr = INADDR_ANY;
		if (::setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
			       (char *)&mr, sizeof(mr)) < 0) {
			fprintf(stderr, "IPNetwork(%s): add_membership: unable to add membership for addr %s: %s\n",
				name(), inet_ntoa(addr), strerror(errno));
			return (-1);
		}
		NIDEBUG3("IPNetwork(%s): add_membership for grp %s done\n",
			name(), inet_ntoa(addr));
		return (0);
	}
#else
	fprintf(stderr, "IPNetwork(%s): add_membership: host does not support IP multicast\n",
		name());
#endif
	NIDEBUG3("IPNetwork(%s): add_membership for grp %s failed\n",
		name(), inet_ntoa(addr));
	return (-1);
}

/*
 * drop membership from the specified group on the specified socket
 */

int
IPNetwork::drop_membership(Socket fd, in_addr& addr)
{

#if defined(IP_DROP_MEMBERSHIP)
	if (IN_CLASSD(ntohl(addr.s_addr))) {
		struct ip_mreq mr;

		mr.imr_multiaddr = addr;
		mr.imr_interface.s_addr = INADDR_ANY;
		if (::setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, 
			       (char *)&mr, sizeof(mr)) < 0) {
			fprintf(stderr, "IPNetwork(%s): drop_membership: unable to drop membership for addr %s: %s\n",
				name(), inet_ntoa(addr), strerror(errno));
			return (-1);
		}
		NIDEBUG3("IPNetwork(%s): drop_membership for grp %s done\n",
			name(), inet_ntoa(addr));
		return (0);
	}
#else
	fprintf(stderr, "IPNetwork(%s): drop_membership: host does not support IP multicast\n",
		name());
#endif
	NIDEBUG3("IPNetwork(%s): drop_membership for grp %s failed\n",
		name(), inet_ntoa(addr));
	return (-1);
}

int
IPNetwork::bindsock(Socket s, in_addr& addr, u_int16_t port, sockaddr_in& sin)
{
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = port;
	sin.sin_addr = addr;
	return(::bind(s, (struct sockaddr *)&sin, sizeof(sin)));
}

int
IPNetwork::connectsock(Socket s, in_addr& addr, u_int16_t port, sockaddr_in& sin)
{
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = port;
	sin.sin_addr = addr;
	return(::connect(s, (struct sockaddr *)&sin, sizeof(sin)));
}
int 
IPNetwork::sbufsize(Socket s, int cnt)
{   
        return(::setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&cnt, sizeof(cnt)));
}   

int
IPNetwork::rbufsize(Socket s, int cnt)
{   
        return(::setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&cnt, sizeof(cnt)));
}   

int
IPNetwork::setmloop(Socket s, int loop)
{

#ifdef IP_MULTICAST_LOOP
	u_char c = loop;

	if (::setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, &c, sizeof(c)) < 0) {
		/*
		 * If we cannot turn off loopback (Like on the
		 * Microsoft TCP/IP stack), then declare this
		 * option broken so that our packets can be
		 * filtered on the recv path.
		 */
		if (c != loop) {
			noloopback_broken_ = 1;
			loop_ = c;
		}
		return (-1);
	}
	noloopback_broken_ = 0;
#else
	fprintf(stderr, "IPNetwork(%s): msetloop: host does not support IP multicast\n",
		name());
#endif
	loop_ = c;
	return (0);
}

void
IPNetwork::reset(int restart)
{
	time_t t = time(0);
	int d = int(t - last_reset_);
	NIDEBUG2("IPNetwork(%s): reset\n", name());
	if (d > 3) {	// Steve: why?
		last_reset_ = t;
		if (ssock_ >= 0)
			(void)::close(ssock_);
		if (rsock_ >= 0)
			(void)::close(rsock_);
		if (open(mode_) < 0) {
			fprintf(stderr,
			  "IPNetwork(%s): couldn't reset\n",
			  name());
			mode_ = -1;
			return;
		}
		if (restart)
			(void) reconfigure();
	}
}

/*
 * after a reset, we may want to re-establish our state
 * [set up addressing, etc].  Do this here
 */

void
IPNetwork::reconfigure()
{
}

void
UDPIPNetwork::reconfigure()
{
}
