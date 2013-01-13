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
    "@(#) $Header: /cvsroot/nsnam/ns-2/emulate/iptap.cc,v 1.5 2010/03/08 05:54:50 tom_henderson Exp $ (ISI)";
#endif

#include "iptap.h"


static class IPTapAgentClass : public TclClass {
 public:
	IPTapAgentClass() : TclClass("Agent/IPTap") {}
	TclObject* create(int, const char*const*) {
		return (new IPTapAgent());
	}
} class_iptap_agent;


IPTapAgent::IPTapAgent() 
{
  int i = 0;
  index = 0;
  for (; i < MAX_PACKETS ; i++) {
    ident[i] = -1;
    offset[i] = -1;
  }

}


/*
 * Checking for duplicate packets.  Need to do this, if running
 * emulation on a machine with one interface.  We simply store
 * a list of IP identification fields of the packets that we see.
 * When we see a new packet, we check this list to see if the packet
 * is coming to this interface again (because when writing out the packet
 * using a raw socket, the bpf will pick it up again.)
 */
int
IPTapAgent::isDuplicate(unsigned short id, unsigned short off) 
{
  int i = 0;
  for( ; i < MAX_PACKETS; i++){
    if ((ident[i] == id) && (offset[i] == off)) {
      return 1;
    }
  }
  ident[index % MAX_PACKETS] = id;
  offset[index % MAX_PACKETS] = off;
  index++;
  return 0;
}



/*
 * Taken from the raw-sock program
 */
unsigned short 
IPTapAgent::in_cksum(unsigned short *addr, int len)
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
IPTapAgent::pkt_handler(void *clientdata, Packet *p, const struct timeval &ts)
{
  IPTapAgent *inst = (IPTapAgent *)clientdata;
  inst->processpkt(p, ts);
}

void
IPTapAgent::processpkt(Packet *p, const struct timeval &)
{
  struct ip *ipheader;
  //struct tcphdr *tcpheader;
  unsigned char *buf;
  
  /* Ip header information from the grabbed packet. */
  int iphlen;  
  unsigned short datagramlen;
  unsigned char ttl;
  
  /* TCP header info from the grabbed packet. */
  //unsigned char tcphlen;
  
  /* 
     At this point, all I have to do is to grab the ttl value 
     from the received packet and put it in p's ttl field after
     decrementing it. It's ok, if we don't recalculate the checksum
     of the actual packet, because we'll do it at the end anyways.
  */

  ipheader = (struct ip *) p->accessdata();
  buf = p->accessdata();
  iphlen = ipheader->ip_hl * 4;
  ttl = ipheader->ip_ttl;
  if (!(--ttl)) {
    fprintf(stderr,
	    "IPTapAgent(%s) : received ttl zero.\n",
	    name());
    Packet::free(p);
    return;
  }

   /* Removed test for isDuplicate on 08/19/02 
    * Recommend adding ether dst or adding not ether src instead 
    * since at high packet rates the test cannot keep up and results 
    * in losing packets */
  /* Discard if duplicate. */
  /* if (isDuplicate(ntohs(ipheader->ip_id), ntohs(ipheader->ip_off))) {
    Packet::free(p);
    return;
    } */ 
  
  datagramlen = ntohs(ipheader->ip_len);

  /* Put all the info in the ns headers. */
  hdr_cmn *ch = HDR_CMN(p);
  ch->size() = datagramlen;

  hdr_ip *ih = HDR_IP(p);
  ih->ttl() = ttl;

  // inject into simulator
  target_->recv(p);
  return;
}

/*
 * ns scheduler calls TapAgent::dispatch which calls recvpkt.
 * 
 * recvpkt then calls the network (net_) to receive as many packets
 * as there are from the packet capture facility.
 * For every packet received through the callback, it populates the ns packet
 * ttl value and inject it into the simulator by calling target_->recv
 * 
 */
void
IPTapAgent::recvpkt()
{
  if (net_->mode() != O_RDWR && net_->mode() != O_RDONLY) {
    fprintf(stderr,
	    "IPTapAgent(%s): recvpkt called while in write-only mode!\n",
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
  TDEBUG4("%f: IPTapAgent(%s): recvpkt, cc:%d\n", now(), name(), cc);

  // nothing to do coz pkt_handler would have called processpkt()
  // that would have injected packets into the simulator

}




/*
 * simulator schedules TapAgent::recv which calls sendpkt
 *
 * Grabs a ns packet, converts it into real packet 
 * and injects onto the network using net_->send
 */
int
IPTapAgent::sendpkt(Packet* p)
{
//  int byteswritten;
  unsigned char *packet;
  unsigned char received_ttl;
  //unsigned short dglen;
  struct ip *ipheader;

  if (net_->mode() != O_RDWR && net_->mode() != O_WRONLY) {
    fprintf(stderr,
	    "IPTapAgent(%s): sendpkt called while in read-only mode!\n",
	    name());
    return (-1);
  }
  
  // send packet into the live network
  hdr_cmn* hc = HDR_CMN(p);
  if (net_ == NULL) {
    fprintf(stderr,
	    "IPTapAgent(%s): sendpkt attempted with NULL net\n",
	    name());
    drop(p);
    return (-1);
  }
  
  /*
    At this point, we should grab the ttl field from the ns
    packet, put it in the IP header of the actual packet,
    recalculate the checksum and send the packet on it's
    way (hoping that anything else in the packet hasn't got
    corrupted.
  */

  /* Grab the info from the ns packet. */
  hdr_ip  *ih = HDR_IP(p);
  received_ttl = ih->ttl_;
  if (!received_ttl) {
    /* Should drop the packet if ttl is zero. */
    fprintf(stderr,
	    "IPTapAgent(%s): received packet ttl zero.\n",
	    name());
    drop(p);
    return(-1);
  }


  /* Modify the original packet with the new info. */
  packet = p->accessdata();
  ipheader = (struct ip *) packet;
  ipheader->ip_ttl = received_ttl;
  ipheader->ip_sum = 0;
  ipheader->ip_sum = (unsigned short) in_cksum((unsigned short *) ipheader,
						sizeof(struct ip));

  if (net_->send(p->accessdata(), hc->size()) < 0) {
    fprintf(stderr,
	    "IPTapAgent(%s): sendpkt (%p, %d): %s\n",
	    name(), p->accessdata(), hc->size(), strerror(errno));
    Packet::free(p);
    return (-1);
    
  }
  Packet::free(p);
  TDEBUG3("IPTapAgent(%s): sent packet (sz: %d)\n",
	  name(), hc->size());
  return 0;
}


