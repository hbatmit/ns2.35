/*
 * Copyright (c) 1991 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Lawrence Berkeley Laboratory,
 * Berkeley, CA.  The name of the University may not be used to
 * endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/emulate/inet.c,v 1.5 2000/02/08 23:35:13 salehi Exp $ (LBL)";

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#else
#include <sys/param.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "config.h"
#include "inet.h"

u_int32_t
LookupHostAddr(const char *s)
{
	if (isdigit(*s))
		return (u_int32_t)inet_addr(s);
	else {
		struct hostent *hp = gethostbyname(s);
		if (hp == 0)
			/*XXX*/
			return (0);
		return *((u_int32_t **)hp->h_addr_list)[0];
	}
}

u_int32_t
LookupLocalAddr(void)
{
	static u_int32_t local_addr;
	char name[MAXHOSTNAMELEN];
	
	if (local_addr == 0) {
		(void)gethostname(name, sizeof(name));
		local_addr = LookupHostAddr(name);
	}
	return (local_addr);
}

/*
 * A faster replacement for inet_ntoa().
 * Extracted from tcpdump 2.1.
 */
const char *
intoa(u_int32_t addr)
{
	register char *cp;
	register u_int byte;
	register int n;
	static char buf[sizeof(".xxx.xxx.xxx.xxx")];

	NTOHL(addr);
	cp = &buf[sizeof buf];
	*--cp = '\0';

	n = 4;
	do {
		byte = addr & 0xff;
		*--cp = byte % 10 + '0';
		byte /= 10;
		if (byte > 0) {
			*--cp = byte % 10 + '0';
			byte /= 10;
			if (byte > 0)
				*--cp = byte + '0';
		}
		*--cp = '.';
		addr >>= 8;
	} while (--n > 0);

	return cp + 1;
}

char *
InetNtoa(u_int32_t addr)
{
	const char *s = intoa(addr);
	char *p = (char *)malloc(strlen(s) + 1);
	strcpy(p, s);
	return p;
}

char *
LookupHostName(u_int32_t addr)
{
	char *p;
	struct hostent* hp;

	/*XXX*/
	if (IN_MULTICAST(ntohl(addr)))
		return (InetNtoa(addr));

	hp = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);
	if (hp == 0) 
		return InetNtoa(addr);
	p = (char *)malloc(strlen(hp->h_name) + 1);
	strcpy(p, hp->h_name);
	return p;
}

/*  
 * in_cksum --
 *      Checksum routine for Internet Protocol family headers (C Version)
 *	[taken from ping.c]
 */ 
u_short  
in_cksum(addr, len)
        u_short *addr; 
        int len;
{   
        register int nleft = len;       
        register u_short *w = addr;
        register int sum = 0;
        u_short answer = 0;
    
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

#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <stdio.h>
void
print_ip(struct ip *ip)
{
	char buf[64];
	u_short off = ntohs(ip->ip_off);
	printf("IP v:%d, ihl:%d, tos:%d, id:%d, off:%d [df:%d, mf:%d], sum:%d, prot:%d\n",
		ip->ip_v, ip->ip_hl, ip->ip_tos, ntohs(ip->ip_id),
		off & IP_OFFMASK,
		(off & IP_DF) ? 1 : 0,
		(off & IP_MF) ? 1 : 0,
		ip->ip_sum, ip->ip_p);
#ifdef HAVE_ADDR2ASCII
	addr2ascii(AF_INET, &ip->ip_src, 4, buf);
#else
	inet_ntoa(ip->ip_src);
#endif /* HAVE_ADDR2ASCII */
#ifdef HAVE_ADDR2ASCII
	printf("IP len:%d ttl: %d, src: %s, dst: %s\n",
		ntohs(ip->ip_len), ip->ip_ttl, buf,
			addr2ascii(AF_INET, &ip->ip_dst, 4, 0));
#else
	printf("IP len:%d ttl: %d, src: %s, dst: %s\n",
		ntohs(ip->ip_len), ip->ip_ttl, buf,
	       inet_ntoa(ip->ip_src));
#endif /* HAVE_ADDR2ASCII */
}
