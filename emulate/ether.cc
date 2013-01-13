/*-
 * Copyright (c) 1993-1994, 1998 The Regents of the University of California.
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

#if 0
#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/emulate/ether.cc,v 1.4 2000/02/08 23:35:12 salehi Exp $ (LBL)";
#endif
#endif
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>

#include "config.h"
#include "ether.h"

char Ethernet::hex[] = "0123456789abcdef";

void
Ethernet::ether_print(const u_char *bp)
{
        const struct ether_header *ep;
 
        ep = (const struct ether_header *)bp;
	printf("src: %s\n",
	     etheraddr_string(ep->ether_shost));
	printf("dst: %s\n",
	     etheraddr_string(ep->ether_dhost));
	printf("prot: %hx\n",
	     ntohs(ep->ether_type));
}

char *
Ethernet::etheraddr_string(const u_char *ep)
{   
        unsigned i, j;
        register char *cp;
        static char buf[sizeof("00:00:00:00:00:00")];
    
        cp = buf;
        if ((j = *ep >> 4) != 0)
                *cp++ = hex[j]; 
        *cp++ = hex[*ep++ & 0xf]; 
        for (i = 5; (int)--i >= 0;) {
                *cp++ = ':';
                if ((j = *ep >> 4) != 0)
                        *cp++ = hex[j]; 
                *cp++ = hex[*ep++ & 0xf]; 
        }       
        *cp = '\0'; 
	return(buf);
}   

#include <net/if.h>

u_char *
Ethernet::nametoaddr(const char *devname)
{
	static struct ifreq ifr;
	int s;

	memset(&ifr, 0, sizeof(ifr));
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "Ethernet::nametoaddr-- failed socket\n");
		return NULL;
	}
	strncpy(ifr.ifr_name, devname, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCGIFADDR, (char *)&ifr) < 0) {
		fprintf(stderr, "Ethernet::nametoaddr-- failed SIOCGIFADDR\n");
		return NULL;
	}
	return ((u_char*) &ifr.ifr_data);
}
