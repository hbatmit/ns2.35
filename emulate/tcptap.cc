/*
 * Copyright (c) 1997, 1998 The Regents of the University of California.
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
 * 	This product includes software developed by the MASH Research
 *	Group at the University of California, Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be used
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
    "@(#) $Header: /cvsroot/nsnam/ns-2/emulate/tcptap.cc,v 1.5 2010/03/08 05:54:50 tom_henderson Exp $ (ISI)";
#endif

#include "tcptap.h"

static class TCPTapAgentClass : public TclClass {
 public:
	TCPTapAgentClass() : TclClass("Agent/TCPTap") {}
	TclObject* create(int, const char*const*) {
		return (new TCPTapAgent());
	}
} class_tcptap_agent;



TCPTapAgent::TCPTapAgent() {
    adv_window = DEFAULT_ADV_WINDOW;

    bzero(&extnode, sizeof(struct sockaddr_in));
    extnode.sin_port = DEFAULT_EXT_PORT;
    extnode.sin_addr.s_addr = inet_addr(DEFAULT_EXT_ADDR);
    extnode.sin_family = AF_INET;

    bzero(&nsnode, sizeof(struct sockaddr_in));
    nsnode.sin_port = DEFAULT_NS_PORT;
    nsnode.sin_addr.s_addr = inet_addr(DEFAULT_NS_ADDR);
    nsnode.sin_family = AF_INET;
    
    dropp = 0; 
}



/*
 * Methods to set ip addresses and port numbers from Tcl scripts.
 */
int
TCPTapAgent::command(int argc, const char*const* argv)
{
  if (argc == 3) {
    if (strcmp(argv[1], "nsipaddr") == 0) {
      if ((nsnode.sin_addr.s_addr = inet_addr(argv[2])) 
	  == INADDR_NONE) {
	printf("Error setting ns ip address");
	exit(1);
      }
      return (TCL_OK);
    }	
    
    
    if (strcmp(argv[1], "extipaddr") == 0) {
      if ((extnode.sin_addr.s_addr = inet_addr(argv[2])) 
	  == INADDR_NONE) {
	printf("Error setting external ip address");
	exit(1);
      }
      return (TCL_OK);
    }
    
    if (strcmp(argv[1], "extport") == 0) {
      extnode.sin_port = atoi(argv[2]);
      return (TCL_OK);
    }

    if (strcmp(argv[1], "advertised-window") == 0) {
      adv_window = atoi(argv[2]);
      return (TCL_OK);
    }


    
  } /* if (argc == 3) */
  return (TapAgent::command(argc, argv));
}



unsigned short 
TCPTapAgent::trans_check(unsigned char proto,
			 char *packet,
			 int length,
			 struct in_addr source_address,
			 struct in_addr dest_address)
{
  char *pseudo_packet;
  unsigned short answer;
  
  pseudohdr.protocol = proto;
  pseudohdr.length = htons(length);
  pseudohdr.place_holder = 0;

  pseudohdr.source_address = source_address;
  pseudohdr.dest_address = dest_address;
  
  if((pseudo_packet = (char *) malloc(sizeof(pseudohdr) + length)) == NULL)  {
    perror("malloc");
    exit(1);
  }
  
  memcpy(pseudo_packet,&pseudohdr,sizeof(pseudohdr));
  memcpy((pseudo_packet + sizeof(pseudohdr)),
	 packet,length);
  
  answer = (unsigned short)in_cksum((unsigned short *)pseudo_packet,
				    (length + sizeof(pseudohdr)));
  free(pseudo_packet);
  return answer;
}





/*
 * Taken from the raw-sock program
 */
unsigned short 
TCPTapAgent::in_cksum(unsigned short *addr, int len)
{
  register int sum = 0;
  u_short answer = 0;
  register u_short *w = addr;
  register int nleft = len;
  
  /*
   * Our algorithm is simple, using a 32 bit accumulator (sum), we add
   * sequential 16 bit words to it, and at the end, fold back all the
   * carry bits from the top 16 bits into the lower 16 bits.
   */
  while (nleft > 1)  {
    sum += *w++;
    nleft -= 2;
  }
  
  /* mop up an odd byte, if necessary */
  if (nleft == 1) {
    *(u_char *)(&answer) = *(u_char *)w ;
    sum += answer;
  }
  
  /* add back carry outs from top 16 bits to low 16 bits */
  sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
  sum += (sum >> 16);                     /* add carry */
  answer = ~sum;                          /* truncate to 16 bits */
  return(answer);
  
}



void 
TCPTapAgent::ip_gen(char *packet, unsigned char protocol, 
	    struct in_addr saddr, struct in_addr daddr,
	    unsigned short length, unsigned char ttl)
{

  struct ip *ipheader;
  
  ipheader = (struct ip *) packet;
  bzero((void *) ipheader, IP_HEADER_LEN);

  ipheader->ip_hl = (IP_HEADER_LEN / 4);
  ipheader->ip_v = IPVERSION;

  ipheader->ip_len = length;
  ipheader->ip_id = 0;
  ipheader->ip_ttl = ttl;
  ipheader->ip_p = protocol;
  
  ipheader->ip_src = saddr;
  ipheader->ip_dst = daddr;

  ipheader->ip_len = ntohs(ipheader->ip_len);
  ipheader->ip_off = ntohs(ipheader->ip_off);

  ipheader->ip_sum = (unsigned short) in_cksum((unsigned short *) ipheader,
						sizeof(struct ip));

}




void
TCPTapAgent::tcp_gen(char *packet, unsigned short sport, unsigned short dport, 
	Packet *nsp)
{
  struct tcphdr *tcpheader;

  hdr_tcp* ns_tcphdr = HDR_TCP(nsp);

  tcpheader = (struct tcphdr *) packet;
  memset((char *)tcpheader, '\0', sizeof(struct tcphdr));

#ifndef LINUX_TCP_HEADER

  tcpheader->th_sport = htons(sport);
  tcpheader->th_dport = htons(dport);

  tcpheader->th_seq = htonl(ns_tcphdr->seqno_);
  tcpheader->th_ack = htonl(ns_tcphdr->ackno_);
  
  tcpheader->th_off = (TCP_HEADER_LEN / 4);
  
  tcpheader->th_win = htons(adv_window);

  /* 
     Here we only propogate the "well-known" flags.
     If you want to add other flags, uncomment this line
     and comment the rest.

  tcpheader->th_flags = ns_tcphdr->tcp_flags_;
  */

  tcpheader->th_flags = 0;
  if (ns_tcphdr->tcp_flags_ & TH_FIN) 
    tcpheader->th_flags |= TH_FIN;
  if (ns_tcphdr->tcp_flags_ & TH_SYN) 
    tcpheader->th_flags |= TH_SYN;
  if (ns_tcphdr->tcp_flags_ & TH_RST) 
    tcpheader->th_flags |= TH_RST;
  if (ns_tcphdr->tcp_flags_ & TH_PUSH) 
    tcpheader->th_flags |= TH_PUSH;
  if (ns_tcphdr->tcp_flags_ & TH_ACK) 
    tcpheader->th_flags |= TH_ACK;
  if (ns_tcphdr->tcp_flags_ & TH_URG) 
    tcpheader->th_flags |= TH_URG;

#else  /* LINUX_TCP_HEADER */

#define TH_FIN  0x01
#define TH_SYN  0x02
#define TH_RST  0x04
#define TH_PUSH 0x08
#define TH_ACK  0x10
#define TH_URG  0x20

  tcpheader->source = htons(sport);
  tcpheader->dest = htons(dport);

  tcpheader->seq = htonl(ns_tcphdr->seqno_);
  tcpheader->ack_seq = htonl(ns_tcphdr->ackno_);
  
  tcpheader->doff = (TCP_HEADER_LEN / 4);
  tcpheader->window = htons(adv_window);

  /* Set the appropriate flag bits */
 if (ns_tcphdr->tcp_flags_ & TH_FIN) 
    tcpheader->fin= 1;
  if (ns_tcphdr->tcp_flags_ & TH_SYN) 
    tcpheader->syn = 1;
  if (ns_tcphdr->tcp_flags_ & TH_RST) 
    tcpheader->rst =1;
  if (ns_tcphdr->tcp_flags_ & TH_PUSH) 
    tcpheader->psh = 1;
  if (ns_tcphdr->tcp_flags_ & TH_ACK) 
    tcpheader->ack = 1;
  if (ns_tcphdr->tcp_flags_ & TH_URG) 
    tcpheader->urg =1;

#endif /* End of LINUX_TCP_HEADER */


}

void
TCPTapAgent::pkt_handler(void *clientdata, Packet *p, const struct timeval &ts)
{
  TCPTapAgent *inst = (TCPTapAgent *)clientdata;
  inst->processpkt(p, ts);
}

void
TCPTapAgent::processpkt(Packet *p, const struct timeval &)
{
  struct ip *ipheader;
  struct tcphdr *tcpheader;
  unsigned char *buf;
  
  /* Ip header information from the grabbed packet. */
  unsigned char ttl;
  
  /* Code to drop packet, if needed 
  dropp++;
  if ((dropp % 10) == 0) {
    fprintf(stdout,
	    "Dropping packet number : %d\n", dropp);
    Packet::free(p);
    return ;
    
    } */
 

  ipheader = (struct ip *) p->accessdata();
  if (in_cksum((unsigned short *) ipheader, (ipheader->ip_hl * 4))) {
    fprintf(stderr,
	    "TCPTapAgent(%s): packet received with invalid IP checksum.\n",
	    name());
    drop(p);
    return;
  }

  ttl = ipheader->ip_ttl;
  if (!(--ttl)) {
    fprintf(stderr, 
	    "TCPTapAgent(%s): packet received with ttl zero.\n",
	    name());
    drop(p);
    return;
    
  }    

  buf = p->accessdata(); 
  tcpheader = (struct tcphdr *) (buf + (ipheader->ip_hl * 4));
  
  Packet *nspacket = allocpkt();

  hdr_ip *ns_iphdr = HDR_IP(nspacket);
  ns_iphdr->ttl() = ttl;

  hdr_tcp *ns_tcphdr = HDR_TCP(nspacket);	
#ifndef LINUX_TCP_HEADER 

  ns_tcphdr->seqno() = ntohl(tcpheader->th_seq);
  ns_tcphdr->ackno() = ntohl(tcpheader->th_ack);
  ns_tcphdr->hlen() = (ipheader->ip_hl + tcpheader->th_off) * 4;
  ns_tcphdr->ts() = Scheduler::instance().clock();
  ns_tcphdr->reason() |= REASON_UNKNOWN;

  /* 
     Here we only propogate the "well-known" flags.
     If you want to add other flags, uncomment this line
     and comment the rest.

     ns_tcphdr->flags() = tcpheader->th_flags;
  */

  ns_tcphdr->flags() = 0;
  if (tcpheader->th_flags & TH_FIN)
    ns_tcphdr->flags() |= TH_FIN;
  if (tcpheader->th_flags & TH_SYN) 
        ns_tcphdr->flags() |= TH_SYN;
  if (tcpheader->th_flags & TH_RST) 
        ns_tcphdr->flags() |= TH_RST;
  if (tcpheader->th_flags & TH_PUSH) 
        ns_tcphdr->flags() |= TH_PUSH;
  if (tcpheader->th_flags & TH_ACK) 
        ns_tcphdr->flags() |= TH_ACK;
  if (tcpheader->th_flags & TH_URG) 
        ns_tcphdr->flags() |= TH_URG;

#else /* LINUX_TCP_HEADER */
 
  ns_tcphdr->seqno() = ntohl(tcpheader->seq);
  ns_tcphdr->ackno() = ntohl(tcpheader->ack);
  ns_tcphdr->hlen() = (ipheader->ip_hl + tcpheader->doff) * 4;
  ns_tcphdr->ts() = Scheduler::instance().clock();
  ns_tcphdr->reason() |= REASON_UNKNOWN;

  /* 
     Here we only propogate the "well-known" flags.
     If you want to add other flags, uncomment this line
     and comment the rest.

     ns_tcphdr->flags() = tcpheader->th_flags;
  */

#define TH_FIN  0x01
#define TH_SYN  0x02
#define TH_RST  0x04
#define TH_PUSH 0x08
#define TH_ACK  0x10
#define TH_URG  0x20

  ns_tcphdr->flags() = 0;

  if (tcpheader->fin == 1 )
    ns_tcphdr->flags() |= TH_FIN;
  if (tcpheader->syn == 1 ) 
        ns_tcphdr->flags() |= TH_SYN;
  if (tcpheader->rst == 1 ) 
        ns_tcphdr->flags() |= TH_RST;
  if (tcpheader->psh == 1) 
        ns_tcphdr->flags() |= TH_PUSH;
  if (tcpheader->ack == 1) 
        ns_tcphdr->flags() |= TH_ACK;
  if (tcpheader->urg == 1) 
        ns_tcphdr->flags() |= TH_URG;

#endif  /* LINUX_TCP_HEADER */


  hdr_cmn *ns_cmnhdr = HDR_CMN(nspacket);
  ns_cmnhdr->size() = ntohs(ipheader->ip_len);

  Packet::free(p);

  // inject into simulator
  target_->recv(nspacket);
  return;
}

/*
 * ns scheduler calls TapAgent::dispatch which calls recvpkt.
 * 
 * recvpkt then calls the network (net_) to receive as many packets
 * as there are from the packet capture facility.
 * For every packet received through the callback, it converts to ns
 * FullTcp packet and injects it into the simulator by calling target_->recv
 * 
 */
void
TCPTapAgent::recvpkt()
{
  if (net_->mode() != O_RDWR && net_->mode() != O_RDONLY) {
    fprintf(stderr,
	    "TCPTapAgent(%s): recvpkt called while in write-only mode!\n",
	    name());
    return;
  }
  
  int cc = net_->recv(pkt_handler, this);
  if (cc <= 0) {
    if (cc < 0) {
      perror("recv");
    }
    return;
  }
  TDEBUG4("%f: TCPTapAgent(%s): recvpkt, cc:%d\n", now(), name(), cc);

  // nothing to do coz pkt_handler would have called processpkt()
  // that would have injected packets into the simulator
}




/*
 * simulator schedules TapAgent::recv which calls sendpkt
 *
 * Grabs a ns Full TCP packet, converts it into real TCP packet 
 * and injects onto the network using net_->send
 *
 */
int
TCPTapAgent::sendpkt(Packet* p)
{
  int byteswritten, datalen;
  unsigned char *packet;
  unsigned char received_ttl;
  int hlength = IP_HEADER_LEN + TCP_HEADER_LEN;
  struct tcphdr *tcpheader;

  if (net_->mode() != O_RDWR && net_->mode() != O_WRONLY) {
    fprintf(stderr,
	    "TCPTapAgent(%s): sendpkt called while in read-only mode!\n",
	    name());
    return (-1);
  }
  
  // send packet into the live network
  hdr_cmn* ns_cmnhdr = HDR_CMN(p);
  if (net_ == NULL) {
    fprintf(stderr,
	    "TCPTapAgent(%s): sendpkt attempted with NULL net\n",
	    name());
    drop(p);
    return (-1);
  }
  
  hdr_tcp* ns_tcphdr = HDR_TCP(p);

  hdr_ip * ns_iphdr = HDR_IP(p);
  received_ttl = ns_iphdr->ttl_;

  // Here we check if ns has sent any data in the packet.
  datalen = ns_cmnhdr->size() - ns_tcphdr->hlen();
  packet = (unsigned char *) calloc (1, sizeof(unsigned char) * 
				     (hlength + datalen));
  if (packet == NULL) {
    fprintf(stderr,
	    "TCPTapAgent(%s) : Error %d allocating memory.\n", name(), errno);
    return (-1);
  }


  // Create real world tcp packet.
  ip_gen((char *)packet, (unsigned char) IPPROTO_TCP, 
	 nsnode.sin_addr, extnode.sin_addr, 
	 hlength + datalen, received_ttl);

  tcpheader = (struct tcphdr*) (packet + IP_HEADER_LEN);

  tcp_gen((char *)tcpheader, nsnode.sin_port, extnode.sin_port, p);

#ifndef LINUX_TCP_HEADER
  tcpheader->th_sum = trans_check(IPPROTO_TCP, (char *) tcpheader,
				  sizeof(struct tcphdr) + datalen,
				  nsnode.sin_addr, extnode.sin_addr);
#else 
  tcpheader->check = trans_check(IPPROTO_TCP, (char *) tcpheader,
				  sizeof(struct tcphdr) + datalen,
				  nsnode.sin_addr, extnode.sin_addr);

#endif 

  /* 
     Limits the packets going out to only IP + TCP header. 
     ns will act as an ACK machine.
   */
  byteswritten = net_->send(packet, hlength + datalen);
  if (byteswritten < 0) {
    fprintf(stderr,"TCPTapAgent(%s): sendpkt (%p, %d): %s\n",
	    name(), p->accessdata(), ns_cmnhdr->size(), strerror(errno));
    Packet::free(p);
    free(packet);
    return (-1);
    
  }
  
  free(packet);
  TDEBUG3("TCPTapAgent(%s): sent packet (sz: %d)\n", name(), byteswritten);
  return 0;
}








