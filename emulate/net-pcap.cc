/*-
 * Copyright (c) 1998 The Regents of the University of California.
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
    "@(#) $Header: /cvsroot/nsnam/ns-2/emulate/net-pcap.cc,v 1.24 2011/10/26 14:24:58 tom_henderson Exp $ (LBL)";
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
#endif
#if defined(sun) && defined(__svr4__)
#include <sys/systeminfo.h>
#endif

#if !defined(__linux__)&&!defined(__APPLE__)
#include <net/bpf.h>
#endif

#ifdef __cplusplus
extern "C" {
#include <pcap.h>
}
#else
#include <pcap.h>
#endif

#include "config.h"
#include "scheduler.h"
#include "net.h"
#include "tclcl.h"
#include "packet.h"

/*
 * observations about pcap library
 *	device name is in the ifreq struct sense, should be doc'd
 *	pcap_lookupdev returns a ptr to static data
 *	q: does lookupdev only return devs in the AF_INET addr family?
 *	why does pcap_compile require a netmask? seems odd
 *	would like some way to tell it what buffer to use
 *	arriving packets have the link layer hdr at the beginning, doc
 *	not convenient/possible to open bpf read/write
 *	no real way to know what file (/dev/bpf?) it is using
 *		would be nice if pcap_lookdev helped out more by
 *		returning ifnet or ifreq or whatever structure
 *	pcap_lookupnet makes calls to get our addr, but
 *		then tosses it anyhow, should get us addr and netmask
 *	interface type codes could be via rfc1573
 *		see freebsd net/if_types.h
 *	want a way to set immed mode
 *	pcap_next masks errors by returning 0 if pcap_dispatch fails
 *	a pcap_t carries it's own internal buffer, and
 *		_dispatch gives pointers into it when invoked [eek]
 *	when you open pcap using a file, pcap_fileno always
 *		returns -1; not so convenient
 *	
 */

#define	PNET_PSTATE_INACTIVE	0
#define	PNET_PSTATE_ACTIVE	1

//
// PcapNetwork: a "network" (source or possibly sink of packets)
//	this is a base class only-- the derived classes are:
//	PcapLiveNetwork [a live net; currently bpf + ethernet]
//	PcapFileNetwork [packets from a tcpdump-style trace file]
//

class PcapNetwork : public Network {

public:
	PcapNetwork() : t_firstpkt_(0.0),
		pfd_(-1), pcnt_(0), local_netmask_(0) { }
	int rchannel() { return(pfd_); }
	int schannel() { return(pfd_); }
	virtual int command(int argc, const char*const* argv);

	virtual int open(int mode, const char *) = 0;
	virtual int skiphdr() = 0;
	virtual double gents(pcap_pkthdr*) = 0;		// generate timestamp
	int recv(u_char *buf, int len, sockaddr&, double&); // get from net
	int send(u_char *buf, int len);			// write to net
	int recv(netpkt_handler callback, void *clientdata); // get from net
	void close();
	void reset();

	int filter(const char*);	// compile + install a filter
	int stat_pkts();
	int stat_pdrops();

	double offset_;			// time offset to 1st pkt in a trace
	double t_firstpkt_;		// ts of 1st pkt recvd

protected:
	static void phandler(u_char* u, const pcap_pkthdr* h, const u_char* p);
	static void phandler_callback(u_char* u, const pcap_pkthdr* h, const u_char* p);
	virtual void bindvars() = 0;

	char errbuf_[PCAP_ERRBUF_SIZE];		// place to put err msgs
	char srcname_[PATH_MAX];		// device or file name
	int pfd_;				// pcap fd
	int pcnt_;				// # pkts counted
	int state_;				// PNET_PSTATE_xxx (above)
	int optimize_;				// bpf optimizer enable
	pcap_t* pcap_;				// reference to pcap state
	struct bpf_program bpfpgm_;		// generated program
	struct pcap_stat pcs_;			// status

	unsigned int local_netmask_;	// seems shouldn't be necessary :(
};

//
// PcapLiveNetwork: a live network tap
//

struct NetworkAddress {
        u_int   len_;
        u_char  addr_[16];	// enough for IPv6 ip addr
};

class PcapLiveNetwork : public PcapNetwork {
public:
	PcapLiveNetwork() : local_net_(0), dlink_type_(-1) {
		linkaddr_.len_ = 0;
		netaddr_.len_ = 0;
		bindvars(); reset();
	}
	NetworkAddress& laddr() { return (linkaddr_); }
	NetworkAddress& naddr() { return (netaddr_); }
protected:
	double gents(pcap_pkthdr*) {
		return Scheduler::instance().clock();
	}

	int devtonaddr(const char* name, NetworkAddress&);

	int open(int mode);
	int open(int mode, const char*);
	int command(int argc, const char*const* argv);
	int skiphdr();
	const char*	autodevname();
	void		bindvars();

	int snaplen_;		// # of bytes to grab
	int promisc_;		// put intf into promisc mode?
	double timeout_;
	NetworkAddress	linkaddr_;	// link-layer address
	NetworkAddress	netaddr_;	// network-layer (IP) address

	unsigned int local_net_;
	int dlink_type_;		// data link type (see pcap)
private:
	// XXX somewhat specific to bpf-- this stuff is  a hack until pcap
	// can be fixed to allow for opening the bpf r/w
#ifdef MT_OWN_PCAP
	pcap_t * pcap_open_live(char *, int slen, int prom, int, char *, int);
	int bpf_open(pcap_t *p, char *errbuf, int how);
#endif
};

class PcapFileNetwork : public PcapNetwork {
public:
	int open(int mode, const char *);
	int skiphdr() { return 0; }	// XXX check me
protected:
	double gents(pcap_pkthdr* p) {
		// time stamp of packet is its relative time
		// in the trace file, plus sim start time, plus offset
		double pts = p->ts.tv_sec + p->ts.tv_usec * 0.000001;
		pts -= t_firstpkt_;
		pts += offset_ + Scheduler::instance().clock();
		return (pts);
	}
	void bindvars();
	int command(int argc, const char*const* argv);
};

static class PcapLiveNetworkClass : public TclClass {
public:
	PcapLiveNetworkClass() : TclClass("Network/Pcap/Live") {}
	TclObject* create(int, const char*const*) {
		return (new PcapLiveNetwork);
	}
} net_pcaplive;

static class PcapFileNetworkClass : public TclClass {
public:
	PcapFileNetworkClass() : TclClass("Network/Pcap/File") {}
	TclObject* create(int, const char*const*) {
		return (new PcapFileNetwork);
	}
} net_pcapfile;

//
// defs for base PcapNetwork class
//

void
PcapNetwork::bindvars()
{
	bind_bool("optimize_", &optimize_);
}

void
PcapNetwork::reset()
{
	state_ = PNET_PSTATE_INACTIVE;
	pfd_ = -1;
	pcap_ = NULL;
	*errbuf_ = '\0';
	*srcname_ = '\0';
	pcnt_ = 0;
}

void
PcapNetwork::close()
{
	if (state_ == PNET_PSTATE_ACTIVE && pcap_)
		pcap_close(pcap_);
	reset();
}

/* compile up a bpf program */
/* XXXwe aren't using 'bcast', so don't care about mask... sigh */

int
PcapNetwork::filter(const char *pgm)
{
	if (pcap_compile(pcap_, &bpfpgm_, (char *)pgm,
	    optimize_, local_netmask_) < 0) {
		fprintf(stderr, "pcapnet obj(%s): couldn't compile filter pgm",
			name());
		return -1;
	}
	if (pcap_setfilter(pcap_, &bpfpgm_) < 0) {
		fprintf(stderr, "pcapnet obj(%s): couldn't set filter pgm",
			name());
		return -1;
	}
	return(bpfpgm_.bf_len);
}

/* return number of pkts received */
int
PcapNetwork::stat_pkts()
{
	if (pcap_stats(pcap_, &pcs_) < 0)
		return (-1);

	return (pcs_.ps_recv);
}

/* return number of pkts dropped */
int
PcapNetwork::stat_pdrops()
{
	if (pcap_stats(pcap_, &pcs_) < 0)
		return (-1);

	return (pcs_.ps_drop);
}

#ifndef MIN
#define MIN(x, y) ((x)<(y) ? (x) : (y))
#endif

#include "ether.h"
/* recv is what others call to grab a packet from the pfilter */

struct pcap_singleton {
        struct pcap_pkthdr *hdr;
        const u_char *pkt;
};   

struct pcap_singleton_callback {
        netpkt_handler callback;
        void *clientdata;
        PcapNetwork *net;
};   

void
PcapNetwork::phandler(u_char* userdata, const pcap_pkthdr* ph, const u_char* pkt)
{
	pcap_singleton *ps = (pcap_singleton*) userdata;
	ps->hdr = (pcap_pkthdr*)ph;
	ps->pkt = (u_char*)pkt;
}

void
PcapNetwork::phandler_callback(u_char* userdata, const pcap_pkthdr* ph, const u_char* pkt)
{
	pcap_singleton_callback *ps = (pcap_singleton_callback*) userdata;	

	Packet *p = Packet::alloc(ph->caplen);
	PcapNetwork *inst = ps->net;
	
	if (++(inst->pcnt_) == 1) {
		// mark time stamp of first pkt
		inst->t_firstpkt_ = ph->ts.tv_sec + ph->ts.tv_usec * 0.000001;
	}

	// link layer header will be placed at the beginning from pcap
	int s = inst->skiphdr();	// go to IP header
	memcpy(p->accessdata(), pkt + s, ph->caplen - s);

	ps->callback(ps->clientdata, p, ph->ts);
}

int
PcapNetwork::recv(u_char *buf, int len, sockaddr& /*fromaddr*/, double &ts)
{

	if (state_ != PNET_PSTATE_ACTIVE) {
		fprintf(stderr, "warning: net/pcap obj(%s) read-- not active\n",
			name());
		return -1;
	}

	int pktcnt = 1;		// all in buffer, or until error
	int np;			// counts # of pkts dispatched
	pcap_singleton ps = { 0, 0 };
	np = pcap_dispatch(pcap_, pktcnt, phandler, (u_char*) &ps);
	if (np < 0) {
		fprintf(stderr,
			"PcapNetwork(%s): recv: pcap_dispatch: %s\n",
			    name(), pcap_strerror(errno));
		return (np);
	} else if (np == 0) {
		/* we get here on EOF of a Pcap/File Network */
		return (np);
	} else if (np != pktcnt) {
		fprintf(stderr,
			"PcapNetwork(%s): warning: recv: pcap_dispatch: requested pktcnt (%d) doesn't match actual (%d)\n",
			    name(), pktcnt, np);
	}

	pcap_pkthdr* ph = ps.hdr;

	if (ph == NULL || ps.pkt == NULL) {
		fprintf(stderr,
			"PcapNetwork(%s): recv: pcap_dispatch: no packet present\n",
			    name());
		return (-1);
	}

	if (++pcnt_ == 1) {
		// mark time stamp of first pkt
		t_firstpkt_ = ph->ts.tv_sec + ph->ts.tv_usec * 0.000001;
	}

	int n = MIN(ph->caplen, (unsigned)len);
	ts = gents(ph);	// mark with timestamp
	// link layer header will be placed at the beginning from pcap
	int s = skiphdr();	// go to IP header
	memcpy(buf, ps.pkt + s, n - s);
	return n - s;
}

int
PcapNetwork::recv(netpkt_handler callback, void *clientdata)
{
	if (state_ != PNET_PSTATE_ACTIVE) {
		fprintf(stderr, "warning: net/pcap obj(%s) read-- not active\n",
			name());
		return -1;
	}

	int pktcnt = -1;		// all in buffer, or until error
	int np;			// counts # of pkts dispatched
	pcap_singleton_callback ps = { callback, clientdata, this };
	
	np = pcap_dispatch(pcap_, pktcnt, phandler_callback, (u_char *)&ps);

#ifdef MY_OWN_PCAP	// directly access pcap_t's member
	assert( pcap_->cc == 0 ); // i.e. we have emptied pcap's buffer
#endif // MY_OWN_PCAP
	return np;
}

/* send a packet out through the packet filter */
int
PcapNetwork::send(u_char *buf, int len)
{
	int n;

	if ((n = write(pfd_, buf, len)) < 0)
		perror("write to pcap fd");

	return n;
}

int PcapNetwork::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "close") == 0) {
			close();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "srcname") == 0) {
			tcl.result(srcname_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "pkts") == 0) {
			tcl.resultf("%d", stat_pkts());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "pdrops") == 0) {
			tcl.resultf("%d", stat_pdrops());
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "filter") == 0) {
			if (state_ != PNET_PSTATE_ACTIVE) {
				fprintf(stderr, "net/pcap obj(%s): can't install filter prior to opening data source\n",
					name());
				return (TCL_ERROR);
			}
			int plen;
			if ((plen = filter(argv[2])) < 0) {
				fprintf(stderr, "problem compiling/installing filter program\n");
				return (TCL_ERROR);
			}
			tcl.resultf("%d", plen);
			return (TCL_OK);
		}
	}
	return (Network::command(argc, argv));
}

//
// defs for PcapLiveNetwork
//

#include <fcntl.h>

#include <net/if.h>
int
PcapLiveNetwork::open(int mode, const char *devname)
{
	close();
#ifdef MY_OWN_PCAP
	pcap_ = pcap_open_live((char*) devname, snaplen_, promisc_,
			       int(timeout_ * 1000.), errbuf_, mode);
#else
	pcap_ = pcap_open_live((char*) devname, snaplen_, promisc_,
			       int(timeout_ * 1000.), errbuf_);
#endif // MY_OWN_PCAP
	if (pcap_ == NULL) {
		fprintf(stderr,
		  "pcap/live object (%s) couldn't open packet source %s: %s\n",
			name(), devname, errbuf_);
		return -1;
	}
	mode_ = mode;
	dlink_type_ = pcap_datalink(pcap_);
	pfd_ = pcap_fileno(pcap_);
	strncpy(srcname_, devname, sizeof(srcname_)-1);
	{
		// use SIOCGIFADDR hook in bpf to get link addr
		struct ifreq ifr;
		struct sockaddr *sa = &ifr.ifr_addr;
#ifdef HAVE_SIOCGIFHWADDR
		memset(&ifr, 0, sizeof(struct ifreq));
		strcpy(ifr.ifr_name, devname);
		if (ioctl(pfd_, SIOCGIFHWADDR, &ifr) < 0) {
			fprintf(stderr,
			  "pcap/live (%s) SIOCGIFHWADDR on bpf fd %d\n",
			  name(), pfd_);
		}
#else
		if (ioctl(pfd_, SIOCGIFADDR, &ifr) < 0) {
			fprintf(stderr,
			  "pcap/live (%s) SIOCGIFADDR on bpf fd %d\n",
			  name(), pfd_);
		}
#endif
		if (dlink_type_ != DLT_EN10MB) {
			fprintf(stderr,
				"sorry, only ethernet supported\n");
			return -1;
		}
		linkaddr_.len_ = ETHER_ADDR_LEN;	// for now
		memcpy(linkaddr_.addr_, sa->sa_data, linkaddr_.len_);
	}

	(void) devtonaddr(devname, netaddr_);

	state_ = PNET_PSTATE_ACTIVE;

	if (pcap_lookupnet(srcname_, &local_net_, &local_netmask_, errbuf_) < 0) {
		fprintf(stderr,
		  "warning: pcap/live (%s) couldn't get local IP network info: %s\n",
		  name(), errbuf_) ;
	}
#if !defined(__linux__)&&!defined(__APPLE__)
	{
		int immed = 1;
		if (ioctl(pfd_, BIOCIMMEDIATE, &immed) < 0) {
			fprintf(stderr,
				"warning: pcap/live (%s) couldn't set immed\n",
				name());
			perror("ioctl(BIOCIMMEDIATE)");
		}
	}
#endif
	return 0;
}

/*
 * how many bytes of link-hdr to skip before net-layer hdr
 */

int
PcapLiveNetwork::skiphdr()
{
	switch (dlink_type_) {
	case DLT_NULL:
		return 0;

	case DLT_EN10MB:
		return ETHER_HDR_LEN;

	default:
		fprintf(stderr,
		    "Network/Pcap/Live(%s): unknown link type: %d\n",
			name(), dlink_type_);
	}
	return -1;
}

const char *
PcapLiveNetwork::autodevname()
{
	const char *dname;
	if ((dname = pcap_lookupdev(errbuf_)) == NULL) {
		fprintf(stderr, "warning: PcapNet/Live(%s) : %s\n",
			name(), errbuf_);
		return (NULL);
	}
	return (dname);	// ptr to static data in pcap library
}

/*
 * devtonaddr -- map device name to its IP/Network layer address
 * this routine wouldn't be necessary if pcap_lookupnet gave
 * out the info it gets anyhow
 */

#include <netinet/in.h>

int
PcapLiveNetwork::devtonaddr(const char *devname, NetworkAddress& na)
{
        register int fd;
        ifreq ifr;
                                
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0) {       
                fprintf(stderr,
			"PcapLiveNet(%s): devtoaddr: couldn't create sock\n",
			name());
                return (-1);
        }
        memset(&ifr, 0, sizeof(ifr));
#ifdef linux
        /* XXX Work around Linux kernel bug */
        ifr.ifr_addr.sa_family = AF_INET;
#endif   
        (void)strncpy(ifr.ifr_name, devname, sizeof(ifr.ifr_name));
        if (ioctl(fd, SIOCGIFADDR, (char *)&ifr) < 0) {
                fprintf(stderr, "PcapLiveNetwork(%s): devtoaddr: no addr\n",
			name());
                (void)::close(fd);   
                return (-1);
        }               
	sockaddr* sa = &ifr.ifr_addr;
	if (sa->sa_family != AF_INET) {
                fprintf(stderr,
			"PcapLiveNet(%s): af not AF_INET (%d)\n",
			name(), sa->sa_family);
	}
	sockaddr_in* sin = (sockaddr_in*) sa;
	na.len_ = 4;				// for now, assump IPv4
	memset(na.addr_, 0, sizeof(na.addr_));
	unsigned sz = sizeof(na.addr_);
	if (sizeof(ifr) < sz)
		sz = sizeof(ifr);
	memcpy(na.addr_, &sin->sin_addr, sz);
	return (0);
}


void
PcapLiveNetwork::bindvars()
{
	bind("snaplen_", &snaplen_);
	bind_bool("promisc_", &promisc_);
	bind_time("timeout_", &timeout_);
	bind("offset_", &offset_);
	PcapNetwork::bindvars();
}

void
PcapFileNetwork::bindvars()
{
	bind("offset_", &offset_);
}

int
PcapLiveNetwork::open(int mode)
{
	return (open(mode, autodevname()));
}

int PcapLiveNetwork::command(int argc, const char*const* argv)
{

	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "linkaddr") == 0) {
			/// XXX: only for ethernet now
			tcl.result(Ethernet::etheraddr_string(linkaddr_.addr_));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "netaddr") == 0) {
			if (netaddr_.len_ != 4) {
				fprintf(stderr,
				  "PcapLive(%s): net addr not len 4 (%d)\n",
					name(), netaddr_.len_);
				return (TCL_ERROR);
			}
			tcl.resultf("%d.%d.%d.%d",
				netaddr_.addr_[0],
				netaddr_.addr_[1],
				netaddr_.addr_[2],
				netaddr_.addr_[3]);
			return (TCL_OK);
		}
	} else if (argc == 3) {
		// $obj open mode
		if (strcmp(argv[1], "open") == 0) {
			int mode = parsemode(argv[2]);
			if (open(mode) < 0)
				return (TCL_ERROR);
			tcl.result(srcname_);
			return (TCL_OK);
		}
	} else if (argc == 4) {
		// $obj open mode devicename
		if (strcmp(argv[1], "open") == 0) {
			int mode = parsemode(argv[2]);
			if (open(mode, argv[3]) < 0)
				return (TCL_ERROR);
			tcl.result(srcname_);
			return (TCL_OK);
		}
	}
	return (PcapNetwork::command(argc, argv));
}

//
// defs for PcapFileNetwork
// use a file instead of a live network
//

int
PcapFileNetwork::open(int /*mode*/, const char *filename)
{

	close();
	pcap_ = pcap_open_offline((char*) filename, errbuf_);
	if (pcap_ == NULL) {
		fprintf(stderr,
		  "pcap/file object (%s) couldn't open packet source %s: %s\n",
			name(), filename, errbuf_);
		return -1;
	}
	mode_ = O_RDONLY;	// sorry, that's all for now
	//
	// pcap only ever puts -1 in the pcap_fileno, which
	// isn't so convenient, so do this instead:
	// pfd_ = pcap_fileno(pcap_);
	pfd_ = fileno(pcap_file(pcap_));
	strncpy(srcname_, filename, sizeof(srcname_)-1);
	state_ = PNET_PSTATE_ACTIVE;
	return 0;
}

int PcapFileNetwork::command(int argc, const char*const* argv)
{

	Tcl& tcl = Tcl::instance();
	if (argc == 4) {
		// $obj open mode filename
		if (strcmp(argv[1], "open") == 0) {
			int mode = parsemode(argv[2]);
			if (open(mode, argv[3]) < 0)
				return (TCL_ERROR);
			tcl.resultf("%s", argv[3]);
			return (TCL_OK);
		}
	}
	return (PcapNetwork::command(argc, argv));
}

//
// XXX: the following routines are unfortunately necessary, 
//	because libpcap has no obvious was of making the bpf fd
//	be read-write :(.  The implication here is nasty:
//		our own version of bpf_open and pcap_open_live
//		and the later routine requires the struct pcap internal state

/*   
 * Savefile
 */  
struct pcap_sf {
        FILE *rfile; 
        int swapped;
        int version_major;
        int version_minor;
        u_char *base;
};   
     
struct pcap_md {
        struct pcap_stat stat;
        /*XXX*/ 
        int use_bpf;
        u_long  TotPkts;        /* can't oflow for 79 hrs on ether */
        u_long  TotAccepted;    /* count accepted by filter */
        u_long  TotDrops;       /* count of dropped packets */
        long    TotMissed;      /* missed by i/f during this run */
        long    OrigMissed;     /* missed by i/f before this run */
#ifdef linux
        int pad;
        int skip;
        char *device;
#endif
};   


struct pcap {
        int fd;
        int snapshot;
        int linktype;
        int tzoff;              /* timezone offset */
        int offset;             /* offset for proper alignment */

        struct pcap_sf sf;
        struct pcap_md md;

        /*
         * Read buffer.
         */
        int bufsize;
        u_char *buffer;
        u_char *bp;
        int cc;

        /*
         * Place holder for pcap_next().
         */
        u_char *pkt;

        
        /*
         * Placeholder for filter code if bpf not in kernel.
         */
        struct bpf_program fcode;

        char errbuf[PCAP_ERRBUF_SIZE];
};


/*
 * the routines bpf_open and pcap_open_live really
 * should not be here, and instead should be part of the
 * pcap library.  Unfortunately, if we ever want to writes to
 * the bpf fd, we need to open it r/w, and the normal pcap
 * library does not permit us to do this.  So for now, here
 * are these routines.
 */

#include <net/if.h>

#ifdef MY_OWN_PCAP
int
PcapLiveNetwork::bpf_open(pcap_t *, char *errbuf, int how)
{
        int fd;
        int n = 0;
        char device[sizeof "/dev/bpf000"];

        /*
         * Go through all the minors and find one that isn't in use.
         */
        do {
                (void)sprintf(device, "/dev/bpf%d", n++);
                fd = ::open(device, how, 0);
        } while (fd < 0 && n < 1000 && errno == EBUSY);

        /*
         * XXX better message for all minors used
         */
        if (fd < 0)
                sprintf(errbuf, "%s: %s", device, pcap_strerror(errno));

        return (fd);
}

pcap_t *
PcapLiveNetwork::pcap_open_live(char *device, int snaplen, int promisc, int to_ms, char *ebuf, int how)
{
        int fd;
        struct ifreq ifr;
        struct bpf_version bv;
        u_int v;
        pcap_t *p;

        p = (pcap_t *)malloc(sizeof(*p));
        if (p == NULL) {
                sprintf(ebuf, "malloc: %s", pcap_strerror(errno));
                return (NULL);
        }
        bzero(p, sizeof(*p));
        fd = bpf_open(p, ebuf, how);
        if (fd < 0)
                goto bad;

        p->fd = fd;
        p->snapshot = snaplen;

        if (ioctl(fd, BIOCVERSION, (caddr_t)&bv) < 0) {
                sprintf(ebuf, "BIOCVERSION: %s", pcap_strerror(errno));
                goto bad;
        }
        if (bv.bv_major != BPF_MAJOR_VERSION ||
            bv.bv_minor < BPF_MINOR_VERSION) {
                sprintf(ebuf, "kernel bpf filter out of date");
                goto bad;
        }
        (void)strncpy(ifr.ifr_name, device, sizeof(ifr.ifr_name));
        if (ioctl(fd, BIOCSETIF, (caddr_t)&ifr) < 0) {
                sprintf(ebuf, "%s: %s", device, pcap_strerror(errno));
                goto bad;
        }
        /* Get the data link layer type. */
        if (ioctl(fd, BIOCGDLT, (caddr_t)&v) < 0) {
                sprintf(ebuf, "BIOCGDLT: %s", pcap_strerror(errno));
                goto bad;
        }
#if _BSDI_VERSION - 0 >= 199510
        /* The SLIP and PPP link layer header changed in BSD/OS 2.1 */
        switch (v) {

        case DLT_SLIP:
                v = DLT_SLIP_BSDOS;
                break;
        case DLT_PPP:
                v = DLT_PPP_BSDOS;
                break;
        }
#endif  
        p->linktype = v;

        /* set timeout */
        if (to_ms != 0) {
                struct timeval to;
                to.tv_sec = to_ms / 1000;
                to.tv_usec = (to_ms * 1000) % 1000000;
                if (ioctl(p->fd, BIOCSRTIMEOUT, (caddr_t)&to) < 0) {
                        sprintf(ebuf, "BIOCSRTIMEOUT: %s",
                                pcap_strerror(errno));
                        goto bad;
                }
        }
        if (promisc)
                /* set promiscuous mode, okay if it fails */
                (void)ioctl(p->fd, BIOCPROMISC, NULL); 

        if (ioctl(fd, BIOCGBLEN, (caddr_t)&v) < 0) {
                sprintf(ebuf, "BIOCGBLEN: %s", pcap_strerror(errno));
                goto bad;
        }   
        p->bufsize = v;
        p->buffer = (u_char *)malloc(p->bufsize);
        if (p->buffer == NULL) {
                sprintf(ebuf, "malloc: %s", pcap_strerror(errno));
                goto bad;
        }       

        return (p);
 bad:   
        ::close(fd);
        free(p);
        return (NULL);
}
#endif // MY_OWN_PCAP
