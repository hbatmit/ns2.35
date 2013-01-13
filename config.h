/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1995-1997 The Regents of the University of California.
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
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
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
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/config.h,v 1.58 2007/01/01 17:38:41 mweigle Exp $ (LBL)
 */

#ifndef ns_config_h
#define ns_config_h


#define MEMDEBUG_SIMULATIONS

/* pick up standard types */
#include <sys/types.h>
#if STDC_HEADERS
#include <stdlib.h>
#include <stddef.h>
#endif

/* get autoconf magic */
#ifdef WIN32
#include "autoconf-win32.h"
#else
#include "autoconf.h"
#endif

/* after autoconf (and HAVE_INT64) we can pick up tclcl.h */
#ifndef stand_alone
#ifdef __cplusplus
#include <tclcl.h>
#endif /* __cplusplus */
#endif

/* handle stl and namespaces */


/*
 * add u_char and u_int
 * Note: do NOT use these expecting them to be 8 and 32 bits long...
 * use {,u_}int{8,16,32}_t if you care about size.
 */
/* Removed typedef and included checks in the configure.in 
 typedef unsigned char u_char;
typedef unsigned int u_int;
*/
typedef int32_t nsaddr_t; 
typedef int32_t nsmask_t; 

/* 32-bit addressing support */
struct ns_addr_t {
	int32_t addr_;
	int32_t port_;
#ifdef __cplusplus
	bool isEqual (ns_addr_t const &o) {
		return ((addr_ == o.addr_) && (port_ == o.port_))?true:false;
	}
#endif /* __cplusplus */
};

/* 64-bit integer support */
#ifndef STRTOI64
#if defined(SIZEOF_LONG) && SIZEOF_LONG >= 8
#define STRTOI64 strtol
#define STRTOI64_FMTSTR "%ld"
/*#define STRTOI64(S) strtol((S), NULL, 0) */

#elif defined(HAVE_STRTOQ)
#define STRTOI64 strtoq
#define STRTOI64_FMTSTR "%lld"
/* #define STRTOI64(S) strtoq((S), NULL, 0) */

#elif defined(HAVE_STRTOLL)
#define STRTOI64 strtoll
#define STRTOI64_FMTSTR "%lld"
/* #define STRTOI64(S) strtoll((S), NULL, 0) */
#endif
#endif

#define	NS_ALIGN	(8)	/* byte alignment for structs (eg packet.cc) */


/* some global definitions */
#define TINY_LEN        8
#define SMALL_LEN	32
#define MID_LEN		256
#define BIG_LEN		4096
#define HUGE_LEN	65536
#define TRUE		1
#define FALSE		0

/*
 * get defintions of bcopy and/or memcpy
 * Different systems put them in string.h or strings.h, so get both
 * (with autoconf help).
 */
#ifdef HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif /* HAVE_STRINGS_H */

#ifndef HAVE_BZERO
#define bzero(dest,count) memset(dest,0,count)
#endif
#ifndef HAVE_BCOPY
#define bcopy(src,dest,size) memcpy(dest,src,size)
#endif

#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef HAVE_TIME_H
#include <time.h>
#endif /* HAVE_TIME_H */

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif /* HAVE_ARPA_INET_H */

#if (defined(__hpux) || defined(_AIX)) && defined(__cplusplus)
/* these definitions are perhaps vestigal */
extern "C" {
int strcasecmp(const char *, const char *);
clock_t clock(void);
#if !defined(__hpux)
int gethostid(void);
#endif
time_t time(time_t *);
char *ctime(const time_t *);
}
#endif

#if defined(NEED_SUNOS_PROTOS) && defined(__cplusplus)
extern "C" {
struct timeval;
struct timezone;
int gettimeofday(struct timeval*, ...);
int ioctl(int fd, int request, ...);
int close(int);
int strcasecmp(const char*, const char*);
int srandom(int);	/* (int) for sunos, (unsigned) for solaris */
int random();
int socket(int, int, int);
int setsockopt(int s, int level, int optname, void* optval, int optlen);
struct sockaddr;
int connect(int s, sockaddr*, int);
int bind(int s, sockaddr*, int);
struct msghdr;
int send(int s, void*, int len, int flags);
int sendmsg(int, msghdr*, int);
int recv(int, void*, int len, int flags);
int recvfrom(int, void*, int len, int flags, sockaddr*, int);
int gethostid();
int getpid();
int gethostname(char*, int);
void abort();
}
#endif

#if defined(SOLARIS_MIN_MAX)
/* Macros for min/max. */
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif /* MIN */
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif  /* MAX */
#endif

#if defined(NEED_SUNOS_PROTOS) || defined(solaris)
extern "C" {
#if defined(NEED_SUNOS_PROTOS)
	caddr_t sbrk(int incr);
#endif
	int getrusage(int who, struct rusage* rusage);
}
#endif

#if defined(__SUNPRO_CC)
#include <cmath>

static double log(const int x)
{ return log((double)x); }

static double log10(const int x)
{ return log10((double)x); }

static double pow(const int x, const int y)
{ return std::pow((double)x,(double)y); }

static double pow(const int x, const double y)
{ return std::pow((double)x,y); }
#endif


#ifdef WIN32

#include <windows.h>
#include <winsock.h>
#include <time.h>		/* For clock_t */

#include <minmax.h>
#define NOMINMAX
#undef min
#undef max
#undef abs

#define MAXHOSTNAMELEN	256

#define _SYS_NMLN	9
struct utsname {
	char sysname[_SYS_NMLN];
	char nodename[_SYS_NMLN];
	char release[_SYS_NMLN];
	char version[_SYS_NMLN];
	char machine[_SYS_NMLN];
};

typedef char *caddr_t;

struct iovec {
	caddr_t	iov_base;
	int	iov_len;
};

#ifndef TIMEZONE_DEFINED_
#define TIMEZONE_DEFINED_
struct timezone {
	int tz_minuteswest;
	int tz_dsttime;
};
#endif

typedef int pid_t;
typedef int uid_t;
typedef int gid_t;

#if defined(__cplusplus)
extern "C" {
#endif

int uname(struct utsname *); 
int getopt(int, char * const *, const char *);
int strcasecmp(const char *, const char *);
/* these shouldn't be used/needed, even on windows */
/* #define srandom srand */
/* #define random rand */
int gettimeofday(struct timeval *p, struct timezone *z);
int gethostid(void);
int getuid(void);
int getgid(void);
int getpid(void);
int nice(int);
int sendmsg(int, struct msghdr*, int);
/* Why this is here, inside a #ifdef WIN32 ??
#ifndef WIN32
	time_t time(time_t *);
#endif
*/
#define strncasecmp _strnicmp
#if defined(__cplusplus)
}
#endif

#ifdef WSAECONNREFUSED
#define ECONNREFUSED	WSAECONNREFUSED
#define ENETUNREACH	WSAENETUNREACH
#define EHOSTUNREACH	WSAEHOSTUNREACH
#define EWOULDBLOCK	WSAEWOULDBLOCK
#endif /* WSAECONNREFUSED */

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif /* M_PI */

#endif /* WIN32 */

#ifdef sgi
#include <math.h>
#endif

/* Declare our implementation of snprintf() so that ns etc. can use it. */
#ifndef HAVE_SNPRINTF
#if defined(__cplusplus)
extern "C" {
#endif
	extern int snprintf(char *buf, int size, const char *fmt, ...);
#if defined(__cplusplus)
}
#endif
#endif

/***** These values are no longer required to be hardcoded -- mask and shift values are 
	available from Class Address. *****/

/* While changing these ensure that values are consistent with tcl/lib/ns-default.tcl */
/* #define NODEMASK	0xffffff */
/* #define NODESHIFT	8 */
/* #define PORTMASK	0xff */

#endif
