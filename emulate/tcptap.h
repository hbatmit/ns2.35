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
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/emulate/tcptap.h,v 1.5 2003/02/15 00:46:49 buchheim Exp $ (ISI)
 */

#ifndef tcptap_h
#define tcptap_h

#include "tap.h"
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include "tcp.h"
#include "ip.h"


#define TCP_HEADER_LEN        (sizeof(struct tcphdr))
#define IP_HEADER_LEN         (sizeof(struct ip))
#define DEFAULT_ADV_WINDOW    65535 /* Large enough so that min(cong, adv 
					window) is cong */
#define DEFAULT_EXT_PORT       8192
#define DEFAULT_EXT_ADDR "192.168.123.116"

#define DEFAULT_NS_PORT        16384
#define DEFAULT_NS_ADDR   "192.168.123.253"


#define TCPIP_BASE_PKTSIZE      40      /* base TCP/IP header in real life */
                                        /* Grabbed from tcp-full-bay.h */ 

/* 
   Used in the ns TCP packets created by the TCPTap.  We don't have any 
   more information in the TCPTap Agent to deduce any other reason. 
   So we set it as unknown.
*/
#define REASON_UNKNOWN   10   /* To support TCP debugging in TCP header
			         reason field. */


struct pseudohdr  {
  struct in_addr source_address;
  struct in_addr dest_address;
  unsigned char place_holder;
  unsigned char protocol;
  unsigned short length;
} pseudohdr;




class TCPTapAgent : public TapAgent {
 private:
  struct sockaddr_in nsnode, extnode;
  unsigned short adv_window;
  int dropp;

  unsigned short trans_check(unsigned char, char *, int ,
			     struct in_addr, struct in_addr);
  unsigned short in_cksum(unsigned short *,int);
  void ip_gen(char *, unsigned char, struct in_addr, struct in_addr, 
	      unsigned short, unsigned char );
  void tcp_gen(char *, unsigned short, unsigned short, Packet *);
  void recvpkt();
  int sendpkt(Packet*);
  void processpkt(Packet *, const struct timeval &);
  static void pkt_handler(void *, Packet *, const struct timeval &);
  
 public:
  TCPTapAgent();
  int command(int, const char*const*);

};

#endif /* tcptap_h */






