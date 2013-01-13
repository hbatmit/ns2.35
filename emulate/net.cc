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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/emulate/net.cc,v 1.9 2010/03/08 05:54:50 tom_henderson Exp $ (LBL)";
#endif

#include <stdlib.h>
#include <math.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#else
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/time.h>
#endif
#include "net.h"

/*
 * Linux does not have sendmsg
 */
#if defined(__linux__) || defined(WIN32)
#define MAXPACKETSIZE (1500-28)

/*static int
sendmsg(int s, struct msghdr* mh, int flags)
{
	u_char wrkbuf[MAXPACKETSIZE];
	int len = mh->msg_iovlen;
	struct iovec* iov = mh->msg_iov;
	u_char* cp;
	u_char* ep;

	for (cp = wrkbuf, ep = wrkbuf + MAXPACKETSIZE; --len >= 0; ++iov) {
		int plen = iov->iov_len;
		if (cp + plen >= ep) {
			errno = E2BIG;
			return (-1);
		}
		memcpy(cp, iov->iov_base, plen);
		cp += plen;
	}
	return (send(s, (char*)wrkbuf, cp - wrkbuf, flags));
}
*/
#endif

int Network::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		Tcl& tcl = Tcl::instance();
		if (strcmp(argv[1], "flush") == 0) {
			if (mode_ == O_RDWR || mode_ == O_RDONLY) {
				unsigned char buf[1024];
				sockaddr from;		    
				double ts;
				while (recv(buf, sizeof(buf), from, ts) > 0)
					;
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "mode") == 0) {
			tcl.result(modename(mode_));
			return (TCL_OK);
		}
	}
	return (TclObject::command(argc, argv));
}

int
Network::nonblock(int fd)
{       
#ifdef WIN32
	u_long flag = 1;
	if (ioctlsocket(fd, FIONBIO, &flag) == -1) {
		fprintf(stderr,
		    "Network::nonblock(): ioctlsocket: FIONBIO: %lu\n",
		    GetLastError());
		return -1;
	}
#else
        int flags;
	if ((flags = fcntl(fd, F_GETFL, 0)) < 0) {
		perror("Network::nonblock(): fcntl");
		return (-1);
	}
#if defined(hpux) || defined(__hpux)
        flags |= O_NONBLOCK;
#else
        flags |= O_NONBLOCK|O_NDELAY;
#endif
        if (fcntl(fd, F_SETFL, flags) == -1) {
                perror("Network::nonblock(): fcntl: F_SETFL");
		return -1;
        }
#endif
	return 0;
}

int
Network::parsemode(const char *mname)
{
	if (strcmp(mname, "readonly") == 0) {
		return (O_RDONLY);
	} else if (strcmp(mname, "readwrite") == 0) {
		return (O_RDWR);
	} else if (strcmp(mname, "writeonly") == 0) {
		return (O_WRONLY);
	}
	return (::atoi(mname));
}

char *
Network::modename(int mode)
{
	switch (mode) {
	case O_RDONLY:
		return ("readonly");
	case O_WRONLY:
		return ("writeonly");
	case O_RDWR:
		return ("readwrite");
	}
	return ("unknown");
}

int Network::recv(netpkt_handler , void *) {
	Tcl::instance().evalf("%s info class", name());
	fprintf( stderr, "Callback Interface to receiving packets"
			" unsupported in class %s\n",
			Tcl::instance().result() );
	return 0;
} // callback called for every packet

