#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>   
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>


#include "emulate/internet.h"
#include "scheduler.h"

/*  
 * in_cksum --
 *      Checksum routine for Internet Protocol family headers (C Version)
 *      [taken from ping.c]
 */ 
u_short  
Internet::in_cksum(u_short* addr, int len)
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
void
Internet::print_ip(ip *ip)
{   
        u_short off = ntohs(ip->ip_off);
        printf("IP v:%d, ihl:%d, tos:0x%x, id:%d, off:%d [df:%d, mf:%d], "
	       "sum:%d, prot:%d\n",
                ip->ip_v, ip->ip_hl, ip->ip_tos, ntohs(ip->ip_id),
                off & IP_OFFMASK,
                (off & IP_DF) ? 1 : 0,
                (off & IP_MF) ? 1 : 0,
                ip->ip_sum, ip->ip_p);
	printf("IP src:%s, ", inet_ntoa(ip->ip_src));
	printf("dst: %s\n", inet_ntoa(ip->ip_dst));
        printf("IP len:%d ttl: %d\n",
                ntohs(ip->ip_len), ip->ip_ttl);
}   

/*
 * cons up a basic-looking ip header, no options
 * multi-byte quantities are assumed to be in HOST byte order
 */
void
Internet::makeip(ip* iph, u_short len, u_char ttl, u_char proto, in_addr& src, 
in_addr& dst)
{
        u_char *p = (u_char*) iph;
        *p = 0x45;      /* ver + hl */
        iph->ip_tos = 0;
        iph->ip_len = htons(len);
        iph->ip_id = (u_short) Scheduler::instance().clock();   // why not?
        iph->ip_off = 0x0000;   // mf and df bits off, offset zero
        iph->ip_ttl = ttl;
        iph->ip_p = proto;
        memcpy(&iph->ip_src, &src, 4);
        memcpy(&iph->ip_dst, &dst, 4);
        iph->ip_sum = Internet::in_cksum((u_short*) iph, 20);
        return;
}

